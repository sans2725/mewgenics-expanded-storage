# Capacity patch replaces the vault as the first release

Date: 2026-10-29 (Asia/Shanghai)

Status: accepted by the owner. Supersedes 0003 as the delivery plan; the vault
code stays in the repository as possible later safety work and is not shipped.

## Why the direction changed

The author of Improved Inventory confirmed on Nexus (2026-10-23) that removing
the Storage limit works in a personal mod. Decision 0002 had already located the
only blocker: three native `min(capacity, 400)` clamps. The vault needed native
item serialization, SQL transaction integration and a new UI, which could not be
finished or verified in one day. The owner chose, on 2026-10-29:

1. Patch the three clamps at runtime (same idea as the author's personal mod).
2. Hybrid persistence: raise `house_storage_upgrades` to 17 in enabled saves, so
   vanilla alone keeps 400 slots; capacity above 400 is a runtime bonus.
3. Default capacity 600, configurable 400-1000 (Improved Inventory has 1024 drawers).
4. Live testing today on save slot 2, after backing up all three slots.

Accepted risk: without the mod, items above 400 are expected to go to the Trash
via the overflow path at `0x335ae0` (static evidence; not live-tested). The player
README states this and gives the uninstall procedure.

## Mechanism (exact build 25143593)

Each site computes `edx = (upgrades + 3)^2 + extra` and then clamps:

| Site | RVA range replaced | Result register | Role |
|---|---|---|---|
| 1 | `0x21189d`-`0x2118b0` | r15d | Storage UI |
| 2 | `0x214054`-`0x214064` | eax | insertion check |
| 3 | `0x336025`-`0x336038` | r15d | overflow transfer to Trash |

The replaced bytes (`add edx,[rcx+0x114]; mov reg,400; cmp; cmovl`) become a
`jmp` to generated code: `reg = min(edx + bonus, limit)`, with edx unchanged and
flags rewritten by the following native `cmp` at every site. Inactive values
(bonus 0, limit 400) are exactly vanilla; `tests/test_capacity.c` executes the
original bytes and the generated code side by side to prove it.

When the enabled campaign is open, the same generated code raises
`[home + 0xbc]` to 17 if lower. It runs on the game thread at the moment the
game itself dereferences the home pointer, so the write never targets freed
memory. Nothing else writes game state.

## Is upgrades = 17 safe for NPC progression?

Every access to `home + 0xbc` in the EXE was enumerated:

- `0x1c1a74` Butch's `storage_expansion` reward: `inc` (adds one, never resets).
- The three capacity sites above.
- `0x335b6b`, `0x337954`: passed to the `storage` scene object (likely the
  furniture's visual stage).
- `0x1d2828`: copied into a named-variable table as `house_storage_upgrades`.
  No game data file references that name (`resources.gpak` searched).
- `0x1d4271` inside `0x1d3b90`: legacy save migration for save version <= 14,
  rebuilding the value from `upgrade_storage_N` flags. New saves skip it.
- Save/load (`0x206177`, `0x2068c9`).

Favor, quota and reward stages are separate state. Raising the value does not
advance Butch's progression; his later regular rewards add +1 (larger squares,
still clamped at 400 without the mod).

## Save-slot scoping

SQLite is statically linked. Its `aSyscall[]` table is writable `.data`:
`CreateFileW` at RVA `0x12fa3c8`, `CloseHandle` at `0x12fa398`. The mod swaps
those two pointers (after verifying names and original thunks), records the last
`(steam)campaign0N.sav` opened with write access, and applies the bonus and the
persistence write only when that slot is enabled in `ExpandedStorage.ini`.
Journal and backup files never match. All opens/closes are logged so the
first live session can confirm the game's file lifecycle.

## Fail-closed checks before any write

PE timestamp `0x6a9c51e5` and image size `0x1574000`; exact bytes at all three
sites (also stops a second copy of this DLL and conflicting capacity mods);
SQLite table names and thunks; config parses. Any failure: no patch, logged.

## Additions on 2026-10-30 (pre-release review)

- Default capacity 1000 (owner request). Improved Inventory accepts at most 1024
  drawers for Storage and Trash combined (`n > IQ_MAX_DRAWERS` aborts its layout).
- Improved Inventory also requires `2 <= columns <= 20` with `columns = upgrades + 3`.
  Butch's regular reward (`0x1c1a6d`: `mov rax,[home]; inc dword [rax+0xbc]`)
  would push enabled saves past 17. In enabled saves the replacement increments
  only below 17; disabled saves increment as vanilla. Repeat rewards use the
  separate extra counter and keep adding capacity.
- The vanilla grid draws `(upgrades + 3)^2` background cells, so it shows 20x20
  regardless of capacity; items beyond 400 need Improved Inventory to browse.
- While one slot's session file stays open, read-write opens of other slots no
  longer switch capacity, so a stray open cannot drop to 400 mid-session.
- When inactive for any reason, a warning dialog (own thread) tells the player
  that Storage is back to 400 and items above 400 would go to the Trash.
