# Native 400-slot limit blocks the proposed persistence mechanism

Date: 2026-10-23

Status: verified static evidence; owner accepted an independently persisted vault.
The requested 500+ capacity is retained. A 400-slot product is not substituted.

## Exact target

Mewgenics 1.1.21239 / Steam build 25143593, executable SHA-256:
`4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea`.
Addresses below are RVAs, not absolute pointers. Analysis reads the installed
executable without executing or modifying it. No campaign contents were read.

## Findings

1. The home state singleton pointer is at RVA `0x13da9a0`.
   Its signed 32-bit field at offset `0xbc` is serialized under
   `house_storage_upgrades`. Serialization reads it at `0x206177`; load stores
   the result of the save integer reader at `0x2068c9`.
2. The regular `storage_expansion` reward increments this field by ONE at
   `0x1c1a74`. In this branch the reward's numerical value is not multiplied
   into the increment. Replacing a data value of 1 with 600 is not a solution.
3. The `rep_reward_count` reward uses a different NPC-associated integer
   (`0x1c2d72` reads the numerical reward, `0x1c2d77` adds it). We have not
   established that arbitrary changes to this integer preserve NPC progression.
4. At `0x211883` the storage UI reads upgrades, adds 3, squares the result,
   and adds a counter from `[global(0x13dac30)+0x5a8]+0x114`. At `0x2118a3`
   it loads **400** (`0x190`) and selects the smaller capacity. The same
   calculation appears at `0x21403a` and `0x33600b`.
5. The routine containing `0x33600b` compares the item count against the capped
   result. For excess items it calls `0x2e19b0` with destination 2 at `0x3361e0`.
   In that transfer function, destination 2 inserts into the container at
   inventory offset `0xb8`; public reverse-engineering notes identify this as
   Trash. Storage is offset `0x78`. The native code removes the item from the
   original containers before inserting into the destination. This is a real
   overflow transfer path, not just a display-size limit.

The derived formula for nonnegative, non-overflowing native values is:

`effective_capacity = min((house_storage_upgrades + 3)^2 + extra_counter, 400)`

The exact lifecycle triggers of all cleanup calls remain to be tested. No live
item loss experiment has been run. We do NOT claim load immediately deletes
items. We do have sufficient evidence to reject a guarantee that raising the
upgrade field alone preserves 500+ ordinary Storage items without the mod.

## Consequences

- The Wiki's endless progression does not establish an unbounded effective
  capacity on this executable. A successful 400-item example does not test 401.
- A larger persisted upgrade value still meets the original 400 clamp when the
  DLL is absent. Runtime patches stop protecting items when the DLL is absent.
- Normal rewards increment the upgrade value instead of resetting it to a small
  square. That resolves one concern but does not resolve the 400 clamp.
- Improved Inventory only changes presentation. Its current source declares
  a 1024-entry drawer array and an 800-entry background array; these are not a
  promise of safe 600-item support (multiple panels consume entries). It does
  not remove this native cleanup limit. Combined behavior needs live testing.

## Decision boundary

Do not ship a silent 400-slot downgrade, a dangerous 600-slot runtime patch, or
an unsupported claim of uninstall safety. Build and validate a non-mutating
preflight foundation and vault core. The owner accepted an independently persisted
extension on 2026-10-23: excess items can be unavailable without the mod and
restored with it. The extension must keep items out of vanilla's overflow path;
game integration is not yet implemented or proven safe.

That design requires full item serialization, campaign identity (not just slot
number), atomic transfer/recovery, rollback and cloud-conflict behavior, and UI
integration. A journal alone cannot solve duplicate restoration after vanilla
play. These must be resolved before an item-moving build is usable.

## Reproduce

Run `tools/inspect_native.py` against the matching installed executable with
`--function 211700 --function 213dc0 --function 335ae0 --function 2e19b0`.
Use `--function 1c18e0 --function 205e80 --function 2062c0` for rewards/persistence.
Keep disassembly outputs in ignored `work/`; never redistribute game binaries.

References (public source is a lead, local bytes are the target evidence):
- https://github.com/SanTertrust/mgmp/blob/main/src/core/mgmp_addresses.h
- https://github.com/anitosq/mewgenics-mods/tree/5ef2fd860ac0a759c7f0ea378312428f799e5bcf/mods/improved-inventory
