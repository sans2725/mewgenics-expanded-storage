#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "capacity.h"

/* Storage capacity patch for Steam build 25143593.
 *
 * Every check runs before any byte is written; on a mismatch nothing is
 * patched. Each of the three native min(capacity, 400) sites jumps to a small
 * generated routine that computes min(native + bonus, limit). While no enabled
 * campaign is open, bonus is 0 and limit is 400, i.e. exactly vanilla.
 *
 * The open campaign is recognised from SQLite's own system-call table: the
 * game opens steamcampaign0N.sav read-write when a slot is loaded. Mewjector
 * declarations are independent minimal declarations of its public ABI. */

#define ES_VERSION "0.2.0"

typedef int (__cdecl *MjGetVersion)(void);
typedef void (__cdecl *MjLog)(const char *, const char *, ...);
typedef HANDLE (WINAPI *CreateFileWFn)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *CloseHandleFn)(HANDLE);

typedef struct {
    const char *name;
    void *current;
    void *fallback;
} SqliteSyscall;

typedef struct {
    volatile LONG bonus, limit, persist, persist_count;
} CaveData;

static uint8_t *game;
static EsConfig config;
static MjLog mj_log;
static HANDLE log_file = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION lock;
static CaveData *cells;
static CreateFileWFn original_create;
static CloseHandleFn original_close;
static volatile LONG active_slot;      /* last campaign opened read-write */
static LONG logged_persist;
static struct { HANDLE handle; int slot, writable; } open_files[8];

static void log_line(const char *format, ...) {
    char text[512];
    SYSTEMTIME now;
    GetLocalTime(&now);
    int n = snprintf(text, sizeof(text), "%02u:%02u:%02u.%03u ", now.wHour, now.wMinute, now.wSecond,
                     now.wMilliseconds);
    va_list args;
    va_start(args, format);
    int m = vsnprintf(text + n, sizeof(text) - (size_t)n - 2, format, args);
    va_end(args);
    if(m < 0) return;
    size_t len = strlen(text);
    text[len++] = '\r';
    text[len++] = '\n';
    EnterCriticalSection(&lock);
    if(log_file != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(log_file, text, (DWORD)len, &written, NULL);
    }
    LeaveCriticalSection(&lock);
}

static int slot_enabled(int slot) {
    return slot >= 1 && slot <= 3 && (config.slot_mask & (1u << slot));
}

static void apply_slot(int slot) {
    int enabled = config.slot_mask == ES_ALL_SLOTS || slot_enabled(slot);
    InterlockedExchange(&cells->bonus, enabled ? config.capacity - ES_VANILLA_LIMIT : 0);
    InterlockedExchange(&cells->limit, enabled ? ES_MAX_CAPACITY : ES_VANILLA_LIMIT);
    InterlockedExchange(&cells->persist, enabled);
}

static void report_persist(void) {
    LONG count = cells->persist_count;
    if(count != InterlockedExchange(&logged_persist, count))
        log_line("Storage upgrades raised to %d in slot %ld; the game keeps 400 slots in this save even without the mod.",
                 ES_PERSIST_UPGRADES, active_slot);
}

static HANDLE WINAPI hook_create(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                                 DWORD disposition, DWORD flags, HANDLE template_file) {
    HANDLE result = original_create(path, access, share, security, disposition, flags, template_file);
    DWORD error = GetLastError();
    report_persist();
    int slot = es_campaign_slot(path);
    if(slot && result != INVALID_HANDLE_VALUE) {
        int writable = (access & GENERIC_WRITE) != 0, other_session = 0;
        EnterCriticalSection(&lock);
        /* The save-select screen opens every slot read-write for a moment; the
         * loaded slot then stays open for the whole session. While that session
         * file is open, another slot's open must not switch the capacity back to
         * vanilla, or the overflow check would move items above 400 to the Trash. */
        for(unsigned i = 0; i < sizeof(open_files) / sizeof(open_files[0]); i++)
            if(open_files[i].handle && open_files[i].writable && open_files[i].slot != slot) other_session = 1;
        for(unsigned i = 0; i < sizeof(open_files) / sizeof(open_files[0]); i++)
            if(!open_files[i].handle) {
                open_files[i].handle = result; open_files[i].slot = slot; open_files[i].writable = writable;
                break;
            }
        LeaveCriticalSection(&lock);
        if(writable && other_session) {
            log_line("Slot %d opened while another slot's session file is open; capacity unchanged.", slot);
        } else if(writable) {
            LONG previous = InterlockedExchange(&active_slot, slot);
            apply_slot(slot);
            if(previous != slot && slot_enabled(slot))
                log_line("Slot %d opened read-write: expansion ACTIVE (capacity %d, limit %d).",
                         slot, config.capacity, ES_MAX_CAPACITY);
            else if(previous != slot)
                log_line("Slot %d opened read-write: not enabled in ExpandedStorage.ini, vanilla capacity.", slot);
        }
        log_line("open slot %d %s handle=%p", slot, writable ? "read-write" : "read-only", result);
    }
    SetLastError(error);
    return result;
}

static BOOL WINAPI hook_close(HANDLE handle) {
    report_persist();
    int slot = 0;
    EnterCriticalSection(&lock);
    for(unsigned i = 0; i < sizeof(open_files) / sizeof(open_files[0]); i++)
        if(open_files[i].handle == handle) { slot = open_files[i].slot; open_files[i].handle = NULL; break; }
    LeaveCriticalSection(&lock);
    BOOL result = original_close(handle);
    if(slot) {
        DWORD error = GetLastError();
        log_line("close slot %d handle=%p", slot, handle);
        SetLastError(error);
    }
    return result;
}

static void open_log(HMODULE self, wchar_t *dir, size_t dir_size) {
    DWORD n = GetModuleFileNameW(self, dir, (DWORD)dir_size);
    if(!n || n >= dir_size) { dir[0] = 0; return; }
    wchar_t *slash = wcsrchr(dir, L'\\');
    if(slash) slash[1] = 0;
    wchar_t path[MAX_PATH + 32];
    _snwprintf(path, MAX_PATH + 32, L"%lsExpandedStorage.log", dir);
    path[MAX_PATH + 31] = 0;
    log_file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static int load_config(const wchar_t *dir) {
    wchar_t path[MAX_PATH + 32];
    _snwprintf(path, MAX_PATH + 32, L"%lsExpandedStorage.ini", dir);
    path[MAX_PATH + 31] = 0;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    char text[4096] = {0}, error[128] = {0};
    if(file == INVALID_HANDLE_VALUE) {
        log_line("ExpandedStorage.ini not found; using defaults.");
    } else {
        DWORD size = 0;
        BOOL ok = ReadFile(file, text, sizeof(text) - 1, &size, NULL);
        CloseHandle(file);
        if(!ok || size == sizeof(text) - 1) { log_line("ExpandedStorage.ini unreadable or too large."); return 0; }
    }
    if(!es_parse_config(text, &config, error, sizeof(error))) {
        log_line("ExpandedStorage.ini rejected (%s).", error);
        return 0;
    }
    return 1;
}

static const char *verify_game(void) {
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)game;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(game + dos->e_lfanew);
    if(nt->FileHeader.TimeDateStamp != ES_PE_TIMESTAMP || nt->OptionalHeader.SizeOfImage != ES_PE_IMAGE_SIZE)
        return "unsupported game build (only Steam build 25143593 is supported)";
    for(int i = 0; i <= ES_SITE_COUNT; i++) {
        const EsSite *site = i < ES_SITE_COUNT ? &es_sites[i] : &es_reward_site;
        if(memcmp(game + site->rva, site->expected, site->length))
            return "capacity code differs from the expected bytes (another mod may patch Storage capacity)";
    }
    SqliteSyscall *create = (SqliteSyscall *)(game + ES_RVA_SQLITE_CREATEFILEW);
    SqliteSyscall *close = (SqliteSyscall *)(game + ES_RVA_SQLITE_CLOSEHANDLE);
    if(create->name != (const char *)(game + ES_RVA_STR_CREATEFILEW) ||
       close->name != (const char *)(game + ES_RVA_STR_CLOSEHANDLE) ||
       create->current != game + ES_RVA_THUNK_CREATEFILEW || close->current != game + ES_RVA_THUNK_CLOSEHANDLE)
        return "SQLite file layer differs from the expected build";
    return NULL;
}

static uint8_t *allocate_near(uintptr_t center, size_t size) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    uintptr_t step = info.dwAllocationGranularity;
    for(uintptr_t distance = step * 16; distance < 0x60000000u; distance += step) {
        uintptr_t below = (center - distance) & ~(step - 1);
        void *p = VirtualAlloc((void *)below, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if(p) return p;
        p = VirtualAlloc((void *)((center + ES_PE_IMAGE_SIZE + distance) & ~(step - 1)), size,
                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if(p) return p;
    }
    return NULL;
}

static const char *install(void) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    size_t page = info.dwPageSize;
    uint8_t *memory = allocate_near((uintptr_t)game, page * 2);
    if(!memory) return "no executable memory near the game image";
    uint8_t *code = memory;
    cells = (CaveData *)(memory + page);
    cells->bonus = 0;
    cells->limit = ES_VANILLA_LIMIT;
    cells->persist = 0;
    cells->persist_count = 0;
    EsCaveData data = {(uint64_t)(uintptr_t)&cells->bonus, (uint64_t)(uintptr_t)&cells->limit,
                       (uint64_t)(uintptr_t)&cells->persist, (uint64_t)(uintptr_t)&cells->persist_count,
                       (uint64_t)(uintptr_t)(game + ES_RVA_HOME_GLOBAL)};
    /* Index ES_SITE_COUNT is the reward increment; the others are capacity sites. */
    const EsSite *sites[ES_SITE_COUNT + 1];
    uint8_t jumps[ES_SITE_COUNT + 1][32];
    for(int i = 0; i <= ES_SITE_COUNT; i++) {
        sites[i] = i < ES_SITE_COUNT ? &es_sites[i] : &es_reward_site;
        uint8_t *cave = code + i * ES_CAVE_SIZE;
        uint64_t site = (uint64_t)(uintptr_t)(game + sites[i]->rva);
        size_t built = i < ES_SITE_COUNT ?
            es_build_cave(cave, ES_CAVE_SIZE, (uint64_t)(uintptr_t)cave, sites[i], site, &data) :
            es_build_reward_cave(cave, ES_CAVE_SIZE, (uint64_t)(uintptr_t)cave, site, &data);
        if(!built || !es_build_jump(jumps[i], sites[i], site, (uint64_t)(uintptr_t)cave)) {
            VirtualFree(memory, 0, MEM_RELEASE);
            return "patch code could not be generated";
        }
    }
    DWORD old;
    if(!VirtualProtect(code, page, PAGE_EXECUTE_READ, &old)) {
        VirtualFree(memory, 0, MEM_RELEASE);
        return "cannot protect patch code";
    }
    FlushInstructionCache(GetCurrentProcess(), code, page);
    /* Until the cells are switched on, every patched site behaves as vanilla,
     * so a failure part-way leaves the game unchanged in effect. */
    for(int i = 0; i <= ES_SITE_COUNT; i++) {
        uint8_t *site = game + sites[i]->rva;
        if(!VirtualProtect(site, sites[i]->length, PAGE_EXECUTE_READWRITE, &old))
            return "cannot unprotect game code";
        memcpy(site, jumps[i], sites[i]->length);
        VirtualProtect(site, sites[i]->length, old, &old);
        FlushInstructionCache(GetCurrentProcess(), site, sites[i]->length);
    }
    SqliteSyscall *create = (SqliteSyscall *)(game + ES_RVA_SQLITE_CREATEFILEW);
    SqliteSyscall *close = (SqliteSyscall *)(game + ES_RVA_SQLITE_CLOSEHANDLE);
    original_create = (CreateFileWFn)create->current;
    original_close = (CloseHandleFn)close->current;
    InterlockedExchangePointer(&close->current, (void *)hook_close);
    InterlockedExchangePointer(&create->current, (void *)hook_create);
    return NULL;
}

static DWORD WINAPI warn_inactive(void *unused) {
    (void)unused;
    MessageBoxW(NULL,
        L"Expanded Storage is NOT active (see ExpandedStorage.log). Storage is back to the game's "
        L"400-slot limit, usually because the game was updated.\n\n"
        L"If your Storage holds more than 400 items, quit before entering your home, or the game "
        L"will move the extra items to the Trash. Wait for a mod update.\n\n"
        L"仓库扩容模组未生效（详见 ExpandedStorage.log）。仓库已恢复为游戏原版的 400 格上限，"
        L"通常是因为游戏更新了。\n\n"
        L"如果仓库里超过 400 件物品，请在进入家园前退出游戏，否则多出的物品会被移入垃圾桶。"
        L"请等待模组更新。",
        L"Expanded Storage", MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    return 0;
}

static void start(HMODULE self) {
    InitializeCriticalSection(&lock);
    HMODULE loader = GetModuleHandleW(L"version.dll");
    MjGetVersion version = loader ? (MjGetVersion)(void *)GetProcAddress(loader, "MJ_GetVersion") : NULL;
    if(version && version() >= 3) mj_log = (MjLog)(void *)GetProcAddress(loader, "MJ_Log");
    wchar_t dir[MAX_PATH];
    open_log(self, dir, MAX_PATH);
    log_line("Expanded Storage " ES_VERSION " loading.");
    game = (uint8_t *)GetModuleHandleW(NULL);
    const char *problem = dir[0] ? NULL : "cannot resolve the mod folder";
    if(!problem && !load_config(dir)) problem = "invalid ExpandedStorage.ini";
    if(!problem) problem = verify_game();
    if(!problem) problem = install();
    if(problem) {
        log_line("INACTIVE: %s. The game runs unmodified.", problem);
        if(mj_log) mj_log("ExpandedStorage", "Inactive: %s.", problem);
        /* The dialog runs on its own thread once the loader lock is released. */
        HANDLE thread = CreateThread(NULL, 0, warn_inactive, NULL, 0, NULL);
        if(thread) CloseHandle(thread);
        return;
    }
    char slots[16] = "All";
    if(config.slot_mask != ES_ALL_SLOTS) {
        size_t n = 0;
        for(int s = 1; s <= 3; s++)
            if(slot_enabled(s)) n += (size_t)snprintf(slots + n, sizeof(slots) - n, n ? ",%d" : "%d", s);
    }
    if(config.slot_mask == ES_ALL_SLOTS) apply_slot(0);
    log_line("Patched 3 capacity sites and the upgrade reward. Capacity %d (limit %d) for slots %s.",
             config.capacity, ES_MAX_CAPACITY, slots);
    if(mj_log) mj_log("ExpandedStorage", "Loaded %s: Storage capacity %d for slots %s.", ES_VERSION, config.capacity, slots);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if(reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        start(instance);
    }
    return TRUE;
}
