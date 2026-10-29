#include "vault.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void codec(void) {
    uint8_t payload[600*32];
    for(size_t i=0;i<sizeof(payload);i++)payload[i]=(uint8_t)(i*37);
    ESVault v={.build={1},.campaign={2},.revision=42,.capacity=600,.count=600,
        .payload=payload,.payload_bytes=sizeof(payload)};
    size_t size=ES_HEADER_BYTES+sizeof(payload),written=99;
    uint8_t *encoded=malloc(size),*changed=malloc(size);assert(encoded && changed);
    assert(es_vault_encode(&v,encoded,size-1,&written)==ES_BOUNDS && written==0);
    assert(es_vault_encode(&v,encoded,size,&written)==ES_OK && written==size);
    ESVault decoded;
    assert(es_vault_decode(encoded,size,v.build,v.campaign,&decoded)==ES_OK);
    assert(decoded.count==600 && decoded.capacity==600 && decoded.revision==42);
    assert(decoded.payload_bytes==sizeof(payload) && !memcmp(decoded.payload,payload,sizeof(payload)));
    /* All truncated representations must be rejected, with no partial output. */
    for(size_t n=0;n<size;n++) {
        memset(&decoded,0xff,sizeof(decoded));
        assert(es_vault_decode(encoded,n,v.build,v.campaign,&decoded)!=ES_OK);
        assert(decoded.payload==NULL && decoded.count==0);
    }
    /* Every byte, including envelope metadata and checksum, is covered. */
    for(size_t n=0;n<size;n++) {
        memcpy(changed,encoded,size);changed[n]^=0x80;
        assert(es_vault_decode(changed,size,v.build,v.campaign,&decoded)!=ES_OK);
    }
    uint8_t other[32]={3};
    assert(es_vault_decode(encoded,size,other,v.campaign,&decoded)==ES_WRONG_BUILD);
    assert(es_vault_decode(encoded,size,v.build,other,&decoded)==ES_WRONG_CAMPAIGN);
    v.count=601;assert(es_vault_encode(&v,encoded,size,&written)==ES_BOUNDS);
    v.count=0;v.payload=NULL;v.payload_bytes=0;
    assert(es_vault_encode(&v,encoded,size,&written)==ES_OK && written==96);
    assert(es_vault_decode(encoded,written,v.build,v.campaign,&decoded)==ES_OK);
    v.count=1;assert(es_vault_encode(&v,encoded,size,&written)==ES_ARGUMENT);
    v.count=0;v.capacity=0;assert(es_vault_encode(&v,encoded,size,&written)==ES_BOUNDS);
    v.capacity=ES_MAX_CAPACITY+1;assert(es_vault_encode(&v,encoded,size,&written)==ES_BOUNDS);
    v.capacity=600;v.payload=encoded;v.payload_bytes=1;
    assert(es_vault_encode(&v,encoded,size,&written)==ES_ARGUMENT);
    free(changed);free(encoded);
}
static void transactions(void) {
    uint8_t first[16]={1},other[16]={2};ESTransaction tx={0};
    assert(es_tx_prepare(&tx,first,0)==ES_STATE);
    assert(es_tx_attach(&tx,first,0)==ES_OK);
    assert(es_tx_prepare(&tx,other,0)==ES_WRONG_CAMPAIGN);
    assert(es_tx_prepare(&tx,first,1)==ES_STALE);
    assert(es_tx_prepare(&tx,first,0)==ES_OK);
    assert(es_tx_prepare(&tx,first,0)==ES_STATE);
    assert(es_tx_detach(&tx)==ES_STATE);
    assert(es_tx_finish(&tx,ES_ROLLED_BACK)==ES_OK && tx.revision==0);
    assert(es_tx_prepare(&tx,first,0)==ES_OK);
    assert(es_tx_finish(&tx,ES_COMMITTED)==ES_OK && tx.revision==1);
    assert(es_tx_finish(&tx,ES_COMMITTED)==ES_STATE);
    assert(es_tx_prepare(&tx,first,0)==ES_STALE);
    assert(es_tx_prepare(&tx,first,1)==ES_OK);
    assert(es_tx_finish(&tx,ES_OUTCOME_UNKNOWN)==ES_UNCERTAIN);
    assert(tx.phase==ES_FROZEN && tx.revision==1);
    assert(es_tx_prepare(&tx,first,1)==ES_STATE);
    assert(es_tx_attach(&tx,other,0)==ES_STATE);
    assert(es_tx_detach(&tx)==ES_STATE);
    ESTransaction max={0};assert(es_tx_attach(&max,first,UINT64_MAX)==ES_OK);
    assert(es_tx_prepare(&max,first,UINT64_MAX)==ES_STALE);
    assert(es_tx_detach(&max)==ES_OK);
    assert(es_tx_attach(&max,other,0)==ES_OK);
}
int main(void) {
    codec();transactions();
    puts("PASS: 600-record synthetic payload, every-byte corruption, every truncation, identity and transaction guards.");
    return 0;
}
