# Resume here

Updated: 2026-10-29 (Asia/Shanghai)

## Current plan

Decision 0004: Expanded Storage 0.2.0 patches the three native `min(..., 400)`
capacity clamps and persists `house_storage_upgrades = 17` in enabled saves.
Default capacity 600, configurable 400-1000. The vault (0003) is shelved.

## Done (2026-10-29)

- Re-verified game EXE hash: still build 25143593.
- Enumerated every access to `home + 0xbc`; upgrades do not drive NPC progression
  (details in 0004). Located SQLite `aSyscall[]` for save-slot detection.
- `src/capacity.c`: config parser, slot-name matcher, code generator.
- `src/plugin.c`: fail-closed checks, near allocation, patching, slot tracking, log.
- `tests/test_capacity.c`: runs original native bytes and generated code in-process;
  inactive mode identical to vanilla for all sampled inputs; active-mode math,
  one-time raise to 17, null home pointer, unreachable displacements, config
  and slot-name cases. Build also still runs the vault tests.
- DLL loaded in a non-game process: logs "unsupported game build", no crash;
  invalid ini rejected. Imports: Kernel32 + UCRT only.
- `python tools/build.py` produces `dist/ExpandedStorage/` and the release ZIP.

## Live session 1 (2026-10-29 23:30)

- Patch applied in the real process; Improved Inventory installed all its hooks.
- File lifecycle: the save-select screen opens slots 1, 2, 3 read-write for
  ~2 ms each (previews); the loaded slot is then held open for the session.
  "Last read-write open" therefore identified the loaded slot correctly.
- Slot 2: `house_storage_upgrades` 4 -> 17 in the saved file (properties table);
  a day was ended; 51 Storage items intact. Native capacity before was 49 + extra.
- Slot 1 was also opened (vanilla, not enabled): only savefile_timer and
  version fields changed, upgrades still 2. Slot 3 byte-identical.
- Improved Inventory pads the last row only; it never draws empty capacity
  cells, so the grid cannot show whether capacity grew.
- Fix: the "upgrades raised" log line is now also emitted on file close.

## Live session 2 (2026-10-29 23:37)

- Only slot 2 loaded. Two shop purchases went into Storage: 51 -> 53 items.
- `inventory_storage` blob's first u32 is item count + 1: backup 50 (49 items,
  exactly the native (4+3)^2 = 49, i.e. full), now 54 (53 items). Storage now
  holds more than slot 2's vanilla capacity, saved and reloaded.

## Live session 3, mod disabled (2026-10-29 23:41)

- Chainloader log: ExpandedStorage.dll not loaded; its own log untouched.
- Slot 2 loaded and saved by vanilla: 53 Storage items, Trash empty,
  upgrades still 17. Vanilla keeps items above the old native 49.
- Slot 3 byte-identical to the backup throughout; slot 1 unchanged since its
  accidental vanilla open in session 1.

First version verified for slot 2 on the owner's machine.

## Not yet verified

- Storage beyond 400 items (impractical to collect in a test save); covered
  only by offline execution tests of the same generated code.
- Without the mod, more than 400 items: expected overflow to Trash (static).

## Pre-release review (2026-10-30)

- Default capacity 1000; reward increment capped at 17 in enabled saves
  (Improved Inventory's 20-column limit); session-file guard against stray
  slot switches; inactive-state warning dialog. Details in 0004.
- Owner's deployment now `Slots=All`, `Capacity=1000`; all saves backed up first.
- Owner chose MIT and GitHub publication (repository not yet pushed).
- Not live-tested yet: the reward cap and the warning dialog.

## Toolchain

Python 3.13; pinned Zig 0.15.2 under work/toolchains. `python tools/build.py`.
Sandbox runs may hit Zig cache rename errors; the same build works unsandboxed.
