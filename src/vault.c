#include "vault.h"
#include <string.h>

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static void put32(uint8_t *p, uint32_t n) {
    for (unsigned i=0;i<4;i++) p[i]=(uint8_t)(n>>(8*i));
}
static uint64_t get64(const uint8_t *p) {
    return (uint64_t)get32(p) | (uint64_t)get32(p+4)<<32;
}
static void put64(uint8_t *p,uint64_t n) {put32(p,(uint32_t)n);put32(p+4,(uint32_t)(n>>32));}
static int nonzero(const uint8_t *p,size_t n) {
    unsigned b=0;for(size_t i=0;i<n;i++)b|=p[i];return b!=0;
}
/* Accidental-corruption checksum, not authentication. Includes all metadata;
 * the checksum's own four bytes are defined as zero during calculation. */
static uint32_t checksum(const uint8_t *p,size_t n) {
    uint32_t crc=UINT32_MAX;
    for(size_t i=0;i<n;i++) {
        crc^=(i>=20 && i<24)?0:p[i];
        for(unsigned b=0;b<8;b++)crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
static ESResult valid(const ESVault *v) {
    if(!v || !nonzero(v->build,32) || !nonzero(v->campaign,16))return ES_ARGUMENT;
    if(v->capacity==0 || v->capacity>ES_MAX_CAPACITY || v->count>v->capacity ||
       v->payload_bytes>ES_MAX_PAYLOAD)return ES_BOUNDS;
    if((v->payload_bytes && !v->payload) || (v->count && !v->payload_bytes))return ES_ARGUMENT;
    return ES_OK;
}
const char *es_result_name(ESResult r) {
    static const char *names[]={"ok","invalid_argument","out_of_bounds","invalid_format",
        "checksum_mismatch","wrong_build","wrong_campaign","invalid_state","stale_revision","uncertain_commit"};
    return (unsigned)r<sizeof(names)/sizeof(names[0])?names[r]:"unknown_result";
}
ESResult es_vault_encode(const ESVault *v,uint8_t *out,size_t bytes,size_t *written) {
    if(written)*written=0;
    if(!out || !written)return ES_ARGUMENT;
    ESResult result=valid(v);if(result!=ES_OK)return result;
    size_t total=ES_HEADER_BYTES+(size_t)v->payload_bytes;
    if(bytes<total)return ES_BOUNDS;
    /* In-place or overlapping encodes are deliberately rejected. */
    uintptr_t ob=(uintptr_t)out,pb=(uintptr_t)v->payload,vb=(uintptr_t)v;
    if(ob>UINTPTR_MAX-total || vb>UINTPTR_MAX-sizeof(*v) ||
       pb>UINTPTR_MAX-v->payload_bytes)return ES_ARGUMENT;
    if((ob<vb+sizeof(*v) && vb<ob+total) ||
       (v->payload_bytes && ob<pb+v->payload_bytes && pb<ob+total))return ES_ARGUMENT;
    memset(out,0,ES_HEADER_BYTES);memcpy(out,"MESVAULT",8);
    put32(out+8,1);put32(out+12,ES_HEADER_BYTES);put32(out+16,(uint32_t)total);
    memcpy(out+24,v->build,32);memcpy(out+56,v->campaign,16);
    put64(out+72,v->revision);put32(out+80,v->capacity);put32(out+84,v->count);
    put32(out+88,v->payload_bytes);
    if(v->payload_bytes)memcpy(out+ES_HEADER_BYTES,v->payload,v->payload_bytes);
    put32(out+20,checksum(out,total));*written=total;return ES_OK;
}
ESResult es_vault_decode(const uint8_t *data,size_t bytes,const uint8_t build[32],
                         const uint8_t campaign[16],ESVault *out) {
    if(!out)return ES_ARGUMENT;
    /* Output may not alias the input or expected identities. */
    uintptr_t ob=(uintptr_t)out,db=(uintptr_t)data;
    if(ob>UINTPTR_MAX-sizeof(*out) || db>UINTPTR_MAX-bytes)return ES_ARGUMENT;
    if(data && ob<db+bytes && db<ob+sizeof(*out))return ES_ARGUMENT;
    uint8_t b[32],c[16];
    if(!data || !build || !campaign){memset(out,0,sizeof(*out));return ES_ARGUMENT;}
    memcpy(b,build,32);memcpy(c,campaign,16);memset(out,0,sizeof(*out));
    if(bytes<ES_HEADER_BYTES || bytes>ES_HEADER_BYTES+ES_MAX_PAYLOAD)return ES_BOUNDS;
    if(memcmp(data,"MESVAULT",8) || get32(data+8)!=1 || get32(data+12)!=ES_HEADER_BYTES ||
       get32(data+16)!=bytes || get32(data+92)!=0 ||
       get32(data+88)!=bytes-ES_HEADER_BYTES)return ES_FORMAT;
    if(get32(data+20)!=checksum(data,bytes))return ES_CHECKSUM;
    if(memcmp(data+24,b,32))return ES_WRONG_BUILD;
    if(memcmp(data+56,c,16))return ES_WRONG_CAMPAIGN;
    ESVault v={0};memcpy(v.build,b,32);memcpy(v.campaign,c,16);
    v.revision=get64(data+72);v.capacity=get32(data+80);v.count=get32(data+84);
    v.payload_bytes=get32(data+88);v.payload=data+ES_HEADER_BYTES;
    ESResult result=valid(&v);if(result==ES_OK)*out=v;return result;
}
ESResult es_tx_attach(ESTransaction *tx,const uint8_t campaign[16],uint64_t revision) {
    if(!tx || !campaign || !nonzero(campaign,16))return ES_ARGUMENT;
    if(tx->phase!=ES_DETACHED)return ES_STATE;
    memcpy(tx->campaign,campaign,16);tx->revision=revision;tx->phase=ES_READY;return ES_OK;
}
ESResult es_tx_prepare(ESTransaction *tx,const uint8_t campaign[16],uint64_t revision) {
    if(!tx || !campaign)return ES_ARGUMENT;
    if(tx->phase!=ES_READY)return ES_STATE;
    if(memcmp(tx->campaign,campaign,16))return ES_WRONG_CAMPAIGN;
    if(tx->revision!=revision || revision==UINT64_MAX)return ES_STALE;
    tx->phase=ES_PREPARED;return ES_OK;
}
ESResult es_tx_finish(ESTransaction *tx,ESOutcome outcome) {
    if(!tx)return ES_ARGUMENT;
    if(tx->phase!=ES_PREPARED)return ES_STATE;
    if(outcome==ES_COMMITTED){tx->revision++;tx->phase=ES_READY;return ES_OK;}
    if(outcome==ES_ROLLED_BACK){tx->phase=ES_READY;return ES_OK;}
    tx->phase=ES_FROZEN;return ES_UNCERTAIN;
}
ESResult es_tx_detach(ESTransaction *tx) {
    if(!tx)return ES_ARGUMENT;
    if(tx->phase==ES_PREPARED || tx->phase==ES_FROZEN)return ES_STATE;
    memset(tx,0,sizeof(*tx));return ES_OK;
}
