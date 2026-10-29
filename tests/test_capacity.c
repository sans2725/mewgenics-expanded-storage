#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "capacity.h"

/* Executes the generated replacement code against the original native bytes
 * in this process. Each fake function mirrors a site: rcx points at the
 * object whose +0x114 field is added, edx holds (upgrades + 3)^2. */

typedef int (*SiteFn)(void *object, int squared);

typedef struct {
    volatile LONG bonus, limit, persist, persist_count;
    void *home_global;
    uint8_t home[0x100];
    uint8_t object[0x200];
} Cells;

static uint8_t *region;
static Cells *cells;

static uint8_t *emit_fn(uint8_t *at, const EsSite *site, const uint8_t *block, int patched,
                        uint8_t *cave) {
    /* push r15 ; <site block> ; mov eax,r15d ; pop r15 ; ret   (eax sites: <block> ; ret) */
    uint8_t *p = at;
    int r15 = site->reg == ES_REG_R15D;
    if(r15) { *p++ = 0x41; *p++ = 0x57; }
    uint8_t *site_at = p;
    if(patched) {
        EsCaveData data = {(uint64_t)(uintptr_t)&cells->bonus, (uint64_t)(uintptr_t)&cells->limit,
                           (uint64_t)(uintptr_t)&cells->persist, (uint64_t)(uintptr_t)&cells->persist_count,
                           (uint64_t)(uintptr_t)&cells->home_global};
        size_t n = es_build_cave(cave, ES_CAVE_SIZE, (uint64_t)(uintptr_t)cave, site,
                                 (uint64_t)(uintptr_t)site_at, &data);
        assert(n > 0 && n <= ES_CAVE_SIZE);
        assert(es_build_jump(p, site, (uint64_t)(uintptr_t)site_at, (uint64_t)(uintptr_t)cave));
    } else {
        memcpy(p, block, site->length);
    }
    p += site->length;
    if(r15) { *p++ = 0x44; *p++ = 0x89; *p++ = 0xf8; *p++ = 0x41; *p++ = 0x5f; }
    *p++ = 0xc3;
    return p;
}

static int upgrades(void) { return *(int *)(cells->home + 0xbc); }
static void set_upgrades(int v) { *(int *)(cells->home + 0xbc) = v; }
static void set_extra(int v) { *(int *)(cells->object + 0x114) = v; }

static void test_config(void) {
    EsConfig c;
    char error[128];
    assert(es_parse_config("", &c, error, sizeof(error)) && c.capacity == 1000 && c.slot_mask == ES_ALL_SLOTS);
    assert(es_parse_config("\xef\xbb\xbf[ExpandedStorage]\r\n; comment\r\nCapacity = 750 ; note\r\nSlots=2\r\n",
                           &c, error, sizeof(error)));
    assert(c.capacity == 750 && c.slot_mask == (1u << 2));
    assert(es_parse_config("slots = 1, 3\ncapacity=1000\n", &c, error, sizeof(error)) &&
           c.slot_mask == ((1u << 1) | (1u << 3)) && c.capacity == 1000);
    assert(es_parse_config("Slots=all\nCapacity=400", &c, error, sizeof(error)) && c.slot_mask == ES_ALL_SLOTS);
    const char *bad[] = {"Capacity=399", "Capacity=1001", "Capacity=6O0", "Capacity=", "Slots=4", "Slots=",
                         "Slots=12", "Slots=0", "Unknown=1", "no equals sign"};
    for(unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        EsConfig untouched = {123, 0};
        assert(!es_parse_config(bad[i], &untouched, error, sizeof(error)));
        assert(untouched.capacity == 123 && error[0]);
    }
}

static void test_slots(void) {
    assert(es_campaign_slot(L"C:\\Users\\x\\saves\\steamcampaign02.sav") == 2);
    assert(es_campaign_slot(L"C:/x/STEAMCAMPAIGN01.SAV") == 1);
    assert(es_campaign_slot(L"campaign03.sav") == 3);
    assert(!es_campaign_slot(L"C:\\x\\steamcampaign02.sav-journal"));
    assert(!es_campaign_slot(L"C:\\x\\steamcampaign02.sav-wal"));
    assert(!es_campaign_slot(L"C:\\x\\steamcampaign04.sav"));
    assert(!es_campaign_slot(L"C:\\x\\test00.sav"));
    assert(!es_campaign_slot(L"C:\\x\\backups\\steamcampaign02_2026-02-26_22-02.savbackup"));
    assert(!es_campaign_slot(NULL));
}

static void test_execution(void) {
    region = VirtualAlloc(NULL, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    assert(region);
    cells = (Cells *)(region + 0x8000);
    cells->home_global = cells->home;
    uint8_t *code = region;
    uint8_t *caves = region + 0x4000;
    SiteFn original[ES_SITE_COUNT], patched[ES_SITE_COUNT];
    for(int i = 0; i < ES_SITE_COUNT; i++) {
        original[i] = (SiteFn)(void *)code;
        code = emit_fn(code, &es_sites[i], es_sites[i].expected, 0, NULL);
        patched[i] = (SiteFn)(void *)code;
        code = emit_fn(code, &es_sites[i], NULL, 1, caves + i * ES_CAVE_SIZE);
    }
    FlushInstructionCache(GetCurrentProcess(), region, 0x8000);

    /* Inactive cells are exactly vanilla, including negative or huge inputs. */
    cells->bonus = 0; cells->limit = ES_VANILLA_LIMIT; cells->persist = 0;
    int samples[] = {-5, 0, 9, 100, 121, 399, 400, 401, 625, 100000};
    int extras[] = {-1, 0, 1, 20, 300};
    for(int i = 0; i < ES_SITE_COUNT; i++)
        for(unsigned a = 0; a < sizeof(samples) / sizeof(samples[0]); a++)
            for(unsigned b = 0; b < sizeof(extras) / sizeof(extras[0]); b++) {
                set_extra(extras[b]);
                set_upgrades(3);
                int want = original[i](cells->object, samples[a]);
                int got = patched[i](cells->object, samples[a]);
                assert(want == got);
                assert(upgrades() == 3 && cells->persist_count == 0);
            }

    /* Active: min(native + bonus, limit), and upgrades raised to 17 once. */
    cells->bonus = 600 - ES_VANILLA_LIMIT; cells->limit = ES_MAX_CAPACITY; cells->persist = 1;
    set_extra(0);
    for(int i = 0; i < ES_SITE_COUNT; i++) {
        set_upgrades(4);
        LONG before = cells->persist_count;
        assert(patched[i](cells->object, 49) == 49 + 200);   /* uses the value already in edx */
        assert(upgrades() == ES_PERSIST_UPGRADES && cells->persist_count == before + 1);
        assert(patched[i](cells->object, 400) == 600);
        assert(cells->persist_count == before + 1);           /* no repeated raise */
        set_extra(7);                                          /* Butch repeat rewards still count */
        assert(patched[i](cells->object, 400) == 607);
        set_upgrades(18);                                      /* a later native upgrade is kept */
        assert(patched[i](cells->object, 441) == 441 + 7 + 200);
        assert(upgrades() == 18);
        assert(patched[i](cells->object, 900) == ES_MAX_CAPACITY);
        set_extra(0);
    }

    /* A missing home object is skipped without a write. */
    cells->home_global = NULL;
    set_upgrades(2);
    LONG before = cells->persist_count;
    assert(patched[1](cells->object, 25) == 225);
    assert(upgrades() == 2 && cells->persist_count == before);
    cells->home_global = cells->home;

    /* Displacements that cannot reach are refused rather than truncated. */
    uint8_t buffer[ES_CAVE_SIZE];
    EsCaveData distant = {0x100000000000ull, 0, 0, 0, 0};
    assert(!es_build_cave(buffer, sizeof(buffer), 0x1000, &es_sites[0], 0x2000, &distant));
    assert(!es_build_cave(buffer, 20, 0x1000, &es_sites[0], 0x2000, &(EsCaveData){0x1100, 0x1104, 0x1108, 0x110c, 0x1200}));
    uint8_t jump[32];
    assert(!es_build_jump(jump, &es_sites[0], 0x1000, 0x100000001000ull));
    assert(!es_build_reward_cave(buffer, sizeof(buffer), 0x1000, 0x2000, &(EsCaveData){0, 0, 0, 0, 0x100000000000ull}));
}

static void test_reward(void) {
    /* The original reward bytes address the game's global directly, so the
     * fake function only runs the replacement: <jmp cave + nops> ; ret. */
    uint8_t *fn = region + 0x3000, *cave = region + 0x4000 + 3 * ES_CAVE_SIZE;
    EsCaveData data = {(uint64_t)(uintptr_t)&cells->bonus, (uint64_t)(uintptr_t)&cells->limit,
                       (uint64_t)(uintptr_t)&cells->persist, (uint64_t)(uintptr_t)&cells->persist_count,
                       (uint64_t)(uintptr_t)&cells->home_global};
    assert(es_build_reward_cave(cave, ES_CAVE_SIZE, (uint64_t)(uintptr_t)cave, (uint64_t)(uintptr_t)fn, &data));
    assert(es_build_jump(fn, &es_reward_site, (uint64_t)(uintptr_t)fn, (uint64_t)(uintptr_t)cave));
    fn[es_reward_site.length] = 0xc3;
    FlushInstructionCache(GetCurrentProcess(), region, 0x8000);
    void (*reward)(void) = (void (*)(void))(void *)fn;

    cells->persist = 0;                        /* not enabled: always +1, as vanilla */
    set_upgrades(16); reward(); assert(upgrades() == 17);
    reward(); assert(upgrades() == 18);
    cells->persist = 1;                        /* enabled: +1 only below 17 */
    set_upgrades(4); reward(); assert(upgrades() == 5);
    set_upgrades(16); reward(); assert(upgrades() == 17);
    reward(); assert(upgrades() == 17);
    set_upgrades(25); reward(); assert(upgrades() == 25);
}

int main(void) {
    test_config();
    test_slots();
    test_execution();
    test_reward();
    puts("capacity tests passed: config, slot names, native-equivalence, active mode and reward cap");
    return 0;
}
