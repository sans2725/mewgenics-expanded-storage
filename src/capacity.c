#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "capacity.h"

/* Each site computes (upgrades + 3)^2 + extra in edx and then clamps it:
 *   add edx,[rcx+0x114] ; mov reg,0x190 ; cmp edx,reg ; cmovl reg,edx
 * The next instruction always rewrites the flags, and edx is left unchanged. */
const EsSite es_sites[ES_SITE_COUNT] = {
    {0x21189d, 19, ES_REG_R15D, {0x03,0x91,0x14,0x01,0x00,0x00, 0x41,0xbf,0x90,0x01,0x00,0x00,
                                 0x41,0x3b,0xd7, 0x44,0x0f,0x4c,0xfa}},  /* Storage UI */
    {0x214054, 16, ES_REG_EAX,  {0x03,0x91,0x14,0x01,0x00,0x00, 0xb8,0x90,0x01,0x00,0x00,
                                 0x3b,0xd0, 0x0f,0x4c,0xc2}},            /* insertion check */
    {0x336025, 19, ES_REG_R15D, {0x03,0x91,0x14,0x01,0x00,0x00, 0x41,0xbf,0x90,0x01,0x00,0x00,
                                 0x41,0x3b,0xd7, 0x44,0x0f,0x4c,0xfa}},  /* overflow to Trash */
};

const EsSite es_reward_site =
    {0x1c1a6d, 13, ES_REG_EAX, {0x48,0x8b,0x05,0x2c,0x8f,0x21,0x01, 0xff,0x80,0xbc,0x00,0x00,0x00}};

static int set_error(char *error, size_t size, const char *message, int line) {
    if(error && size) snprintf(error, size, "line %d: %s", line, message);
    return 0;
}

static char *trim(char *s) {
    while(*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while(end > s && isspace((unsigned char)end[-1])) *--end = 0;
    return s;
}

static int equals_ci(const char *a, const char *b) {
    while(*a && *b) {
        if(tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int parse_slots(char *value, unsigned *mask) {
    if(equals_ci(value, "all")) { *mask = ES_ALL_SLOTS; return 1; }
    unsigned result = 0;
    for(char *p = value; *p;) {
        while(*p == ' ' || *p == '\t' || *p == ',') p++;
        if(!*p) break;
        if(*p < '1' || *p > '3') return 0;
        result |= 1u << (*p - '0');
        p++;
        if(*p && *p != ',' && *p != ' ' && *p != '\t') return 0;
    }
    if(!result) return 0;
    *mask = result;
    return 1;
}

int es_parse_config(const char *text, EsConfig *out, char *error, size_t error_size) {
    if(!text || !out) return 0;
    EsConfig config = {ES_DEFAULT_CAPACITY, ES_ALL_SLOTS};
    int line_number = 0;
    while(*text) {
        char line[256];
        size_t n = strcspn(text, "\r\n");
        line_number++;
        if(n >= sizeof(line)) return set_error(error, error_size, "line too long", line_number);
        memcpy(line, text, n);
        line[n] = 0;
        text += n;
        if(*text == '\r') text++;
        if(*text == '\n') text++;
        char *s = line;
        if(line_number == 1 && (unsigned char)s[0] == 0xef && (unsigned char)s[1] == 0xbb &&
           (unsigned char)s[2] == 0xbf) s += 3;
        s = trim(s);
        if(!*s || *s == ';' || *s == '#' || *s == '[') continue;
        char *eq = strchr(s, '=');
        if(!eq) return set_error(error, error_size, "expected key=value", line_number);
        *eq = 0;
        char *key = trim(s), *value = trim(eq + 1);
        char *comment = strpbrk(value, ";#");
        if(comment) { *comment = 0; value = trim(value); }
        if(equals_ci(key, "Capacity")) {
            char *end;
            long v = strtol(value, &end, 10);
            if(!*value || *end || v < ES_VANILLA_LIMIT || v > ES_MAX_CAPACITY)
                return set_error(error, error_size, "Capacity must be a whole number from 400 to 1000", line_number);
            config.capacity = (int)v;
        } else if(equals_ci(key, "Slots")) {
            if(!parse_slots(value, &config.slot_mask))
                return set_error(error, error_size, "Slots must be All or a list of 1, 2 and 3", line_number);
        } else {
            return set_error(error, error_size, "unknown setting", line_number);
        }
    }
    *out = config;
    return 1;
}

int es_campaign_slot(const wchar_t *path) {
    if(!path) return 0;
    const wchar_t *name = path;
    for(const wchar_t *p = path; *p; p++)
        if(*p == L'\\' || *p == L'/') name = p + 1;
    if(!_wcsnicmp(name, L"steam", 5)) name += 5;
    if(wcslen(name) != 14 || _wcsnicmp(name, L"campaign0", 9) || _wcsicmp(name + 10, L".sav")) return 0;
    return name[9] >= L'1' && name[9] <= L'3' ? name[9] - L'0' : 0;
}

typedef struct {
    uint8_t *out;
    size_t size, at;
    uint64_t base;
    int ok;
} Emitter;

static void emit(Emitter *e, const uint8_t *bytes, size_t n) {
    if(!e->ok || e->at + n > e->size) { e->ok = 0; return; }
    memcpy(e->out + e->at, bytes, n);
    e->at += n;
}

static void emit_rel32(Emitter *e, uint64_t target, size_t trailing) {
    /* Displacement is relative to the end of the instruction: this field plus
     * any immediate bytes that follow it. */
    int64_t delta = (int64_t)(target - (e->base + e->at + 4 + trailing));
    if(delta < INT32_MIN || delta > INT32_MAX) { e->ok = 0; return; }
    int32_t d = (int32_t)delta;
    uint8_t b[4] = {(uint8_t)d, (uint8_t)(d >> 8), (uint8_t)(d >> 16), (uint8_t)(d >> 24)};
    emit(e, b, 4);
}

static void patch_rel8(Emitter *e, size_t at) {
    if(!e->ok) return;
    size_t delta = e->at - (at + 1);
    if(delta > 127) { e->ok = 0; return; }
    e->out[at] = (uint8_t)delta;
}

size_t es_build_cave(uint8_t *out, size_t out_size, uint64_t cave_address,
                     const EsSite *site, uint64_t site_address, const EsCaveData *data) {
    if(!out || !site || !data) return 0;
    Emitter e = {out, out_size, 0, cave_address, 1};
    int r15 = site->reg == ES_REG_R15D;

    emit(&e, site->expected, 6);                       /* add edx,[rcx+0x114] */

    /* Raise the persisted upgrade count while the game itself is reading the
     * home-state pointer on its own thread, so the pointer is known to be live. */
    emit(&e, (const uint8_t[]){0x83,0x3d}, 2);         /* cmp dword [rip+persist],0 */
    emit_rel32(&e, data->persist, 1);
    emit(&e, (const uint8_t[]){0x00, 0x74, 0x00}, 3);  /* je skip */
    size_t skip = e.at - 1;
    emit(&e, (const uint8_t[]){0x50, 0x48,0x8b,0x05}, 4); /* push rax; mov rax,[rip+home] */
    emit_rel32(&e, data->home_global, 0);
    emit(&e, (const uint8_t[]){0x48,0x85,0xc0, 0x74,0x00}, 5); /* test rax,rax; jz done */
    size_t null_done = e.at - 1;
    emit(&e, (const uint8_t[]){0x83,0xb8,0xbc,0x00,0x00,0x00,ES_PERSIST_UPGRADES, 0x7d,0x00}, 9);
    size_t high_done = e.at - 1;                       /* cmp [rax+0xbc],17; jge done */
    emit(&e, (const uint8_t[]){0xc7,0x80,0xbc,0x00,0x00,0x00,ES_PERSIST_UPGRADES,0x00,0x00,0x00}, 10);
    emit(&e, (const uint8_t[]){0xf0,0xff,0x05}, 3);    /* lock inc dword [rip+count] */
    emit_rel32(&e, data->persist_count, 0);
    patch_rel8(&e, null_done);
    patch_rel8(&e, high_done);
    emit(&e, (const uint8_t[]){0x58}, 1);              /* pop rax */
    patch_rel8(&e, skip);

    /* reg = min(edx + bonus, limit); edx keeps the native value. */
    if(r15) emit(&e, (const uint8_t[]){0x41,0x89,0xd7, 0x44,0x03,0x3d}, 6);
    else emit(&e, (const uint8_t[]){0x89,0xd0, 0x03,0x05}, 4);
    emit_rel32(&e, data->bonus, 0);
    if(r15) emit(&e, (const uint8_t[]){0x44,0x3b,0x3d}, 3);
    else emit(&e, (const uint8_t[]){0x3b,0x05}, 2);
    emit_rel32(&e, data->limit, 0);
    if(r15) emit(&e, (const uint8_t[]){0x44,0x0f,0x4f,0x3d}, 4);
    else emit(&e, (const uint8_t[]){0x0f,0x4f,0x05}, 3);
    emit_rel32(&e, data->limit, 0);

    emit(&e, (const uint8_t[]){0xe9}, 1);              /* jmp back */
    emit_rel32(&e, site_address + site->length, 0);
    return e.ok ? e.at : 0;
}

size_t es_build_reward_cave(uint8_t *out, size_t out_size, uint64_t cave_address,
                            uint64_t site_address, const EsCaveData *data) {
    if(!out || !data) return 0;
    Emitter e = {out, out_size, 0, cave_address, 1};
    emit(&e, (const uint8_t[]){0x48,0x8b,0x05}, 3);    /* mov rax,[rip+home] */
    emit_rel32(&e, data->home_global, 0);
    emit(&e, (const uint8_t[]){0x83,0x3d}, 2);         /* cmp dword [rip+persist],0 */
    emit_rel32(&e, data->persist, 1);
    emit(&e, (const uint8_t[]){0x00, 0x74,0x00}, 3);   /* je increment */
    size_t vanilla = e.at - 1;
    emit(&e, (const uint8_t[]){0x83,0xb8,0xbc,0x00,0x00,0x00,ES_PERSIST_UPGRADES, 0x7d,0x00}, 9);
    size_t capped = e.at - 1;                          /* cmp [rax+0xbc],17; jge done */
    patch_rel8(&e, vanilla);
    emit(&e, (const uint8_t[]){0xff,0x80,0xbc,0x00,0x00,0x00}, 6); /* inc dword [rax+0xbc] */
    patch_rel8(&e, capped);
    emit(&e, (const uint8_t[]){0xe9}, 1);              /* jmp back */
    emit_rel32(&e, site_address + es_reward_site.length, 0);
    return e.ok ? e.at : 0;
}

int es_build_jump(uint8_t *out, const EsSite *site, uint64_t site_address, uint64_t target) {
    if(!out || !site || site->length < 5) return 0;
    int64_t delta = (int64_t)(target - (site_address + 5));
    if(delta < INT32_MIN || delta > INT32_MAX) return 0;
    int32_t d = (int32_t)delta;
    out[0] = 0xe9;
    out[1] = (uint8_t)d; out[2] = (uint8_t)(d >> 8); out[3] = (uint8_t)(d >> 16); out[4] = (uint8_t)(d >> 24);
    memset(out + 5, 0x90, site->length - 5u);
    return 1;
}
