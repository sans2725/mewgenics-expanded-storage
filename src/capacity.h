#ifndef ES_CAPACITY_H
#define ES_CAPACITY_H
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

/* Portable pieces of the Storage capacity patch: configuration, save-slot
 * recognition and generation of the replacement machine code. Nothing here
 * touches process memory, so it is covered by offline tests. */

#define ES_VANILLA_LIMIT 400      /* the game's own min(..., 400) clamp */
#define ES_MAX_CAPACITY 1000      /* stays below Improved Inventory's 1024 drawers */
#define ES_DEFAULT_CAPACITY 1000
#define ES_PERSIST_UPGRADES 17    /* (17 + 3)^2 = 400: vanilla alone keeps 400 slots */
#define ES_SITE_COUNT 3
#define ES_CAVE_SIZE 128

enum { ES_REG_EAX = 0, ES_REG_R15D = 1 };

typedef struct {
    uint32_t rva;                 /* first byte replaced: add edx,[rcx+0x114] */
    uint8_t length;               /* bytes replaced up to the end of cmovl */
    uint8_t reg;                  /* register receiving the clamped capacity */
    uint8_t expected[19];
} EsSite;

extern const EsSite es_sites[ES_SITE_COUNT];

/* Butch's storage_expansion reward: mov rax,[home]; inc dword [rax+0xbc].
 * Improved Inventory only lays out grids of at most 20 columns, i.e.
 * upgrades <= 17, so enabled saves stop increasing there. */
extern const EsSite es_reward_site;

/* RVAs on Steam build 25143593. */
#define ES_RVA_HOME_GLOBAL 0x13da9a0u  /* pointer to home state; +0xbc upgrades */
#define ES_RVA_SQLITE_CREATEFILEW 0x12fa3c8u /* SQLite aSyscall[] entry */
#define ES_RVA_SQLITE_CLOSEHANDLE 0x12fa398u
#define ES_RVA_STR_CREATEFILEW 0x11376e8u
#define ES_RVA_STR_CLOSEHANDLE 0x1137600u
#define ES_RVA_THUNK_CREATEFILEW 0xd19d91u
#define ES_RVA_THUNK_CLOSEHANDLE 0xd19dafu
#define ES_PE_TIMESTAMP 0x6a9c51e5u
#define ES_PE_IMAGE_SIZE 0x1574000u

typedef struct {
    int capacity;                 /* ES_VANILLA_LIMIT..ES_MAX_CAPACITY */
    unsigned slot_mask;           /* bit n set: campaign slot n (1-3) enabled */
} EsConfig;

#define ES_ALL_SLOTS 0xeu

/* Parses ExpandedStorage.ini text. Returns 1 on success, 0 with a message in
 * error on any invalid or unknown setting. Missing keys keep their defaults. */
int es_parse_config(const char *text, EsConfig *out, char *error, size_t error_size);

/* Returns 1-3 for a path whose file name is (steam)campaign0N.sav, else 0.
 * Journal and backup files deliberately do not match. */
int es_campaign_slot(const wchar_t *path);

/* Addresses of the data cells the generated code reads or updates. */
typedef struct {
    uint64_t bonus;               /* int32 added to the native capacity */
    uint64_t limit;               /* int32 upper bound for the result */
    uint64_t persist;             /* int32: nonzero raises upgrades to 17 */
    uint64_t persist_count;       /* int32 incremented on each raise */
    uint64_t home_global;         /* the game's home-state pointer variable */
} EsCaveData;

/* Writes the replacement for one site at cave_address. Returns the byte count,
 * or 0 if out is too small or a displacement does not fit in 32 bits. */
size_t es_build_cave(uint8_t *out, size_t out_size, uint64_t cave_address,
                     const EsSite *site, uint64_t site_address, const EsCaveData *data);

/* Writes the replacement for the reward increment: increments as vanilla when
 * persist is 0, otherwise only while upgrades are below 17. */
size_t es_build_reward_cave(uint8_t *out, size_t out_size, uint64_t cave_address,
                            uint64_t site_address, const EsCaveData *data);

/* Fills site->length bytes with jmp rel32 to target plus NOP padding. */
int es_build_jump(uint8_t *out, const EsSite *site, uint64_t site_address, uint64_t target);

#endif
