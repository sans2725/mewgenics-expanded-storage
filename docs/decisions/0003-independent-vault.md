# Independent vault: accepted direction and implementation boundaries

Date: 2026-10-23

The owner accepted an independent persistent extension: preserve 500+ total
capacity, prioritize item safety, and allow archived items to be temporarily
unavailable when the mod is absent. Ordinary Storage and its NPC progression stay
native. Initial engineering target: 600 vault entries in addition to ordinary
Storage. This is a development target, not an advertised tested capacity.

## Persistence choice

Prefer a uniquely named blob (`es_vault_v1`, provisional) in the same game save
transaction as ordinary inventory. A separate sidecar is not the default because
separate commits can disagree after a crash, a rollback or cloud synchronization.
We must prove vanilla preserves the key through normal saves before enabling it.

Exact-build static evidence:
- MewSaveFile blob writer `0x22cf40` calls the SQL write layer `0xa0e650`.
- The blob writer's five-character table name resolves to `files` in this build.
  Inventory bucket writing reaches `0x22d640`, then `0x22cf40`; the corresponding
  load path reaches `0x22e860`, then `0x22d090`. These call chains are checked;
  unknown-key retention and the item schema still need further validation.
- SQL write layer uses `INSERT OR REPLACE`, not a fixed struct file overwrite.
- SQL transaction begin `0xa0dba0` issues BEGIN when nesting depth is zero and
  increments depth at offset `0x28`.
- SQL transaction end `0xa0dc40` decrements depth and issues COMMIT at zero.
- Inventory bucket save driver `0x2e1bf0` begins a transaction and writes native
  inventory bucket keys. Bucket writer/reader candidates are `0x2e20d0` and
  `0x2e2260`; they reach native item serialization rather than copying pointers.

These facts support investigation, not an atomicity guarantee. SQL failure status,
outer commit timing, unknown-key cleanup, resets and rollback need verification.
Some unrelated native routines DELETE rows/tables, so INSERT alone is insufficient.

## Runtime invariants

1. Vault items are not also present in normal Storage, Backpack, Trash or equipped
   slots. Never simulate a large ordinary inventory and hide the overflow.
2. Deposit commits the native removal AND vault addition together. Withdrawal
   commits the vault removal AND native insertion together. There is no automatic
   restoration from a stale inventory snapshot after vanilla play.
3. Withdrawal checks current native free capacity. A full native warehouse does
   not eject an item into Trash to make room.
4. Preserve the whole serialized item, not merely its type/count. Runtime pointers
   and process object IDs must not become persistent identity.
5. A campaign-specific random identity belongs in the saved extension record.
   Slot 1/2/3 and a file pathname alone are insufficient after a slot is recreated.
6. No blanket mutation on DLL load or at the title screen. Activation is explicit,
   campaign-scoped and must not apply to another currently occupied slot.
7. Invalid data is retained for recovery, never replaced with an empty vault.
   Unknown commit outcome freezes further transfers until a durable reread resolves
   it. No blind retry, auto-respawn or "best effort" deletion.
8. No native memory or inventory changes occur until the game adapter proves all
   prerequisites. The development DLL currently exposes no mutation path.

## Implemented portable core

`src/vault.c` implements a 96-byte little-endian envelope, schema 1, with exact
length checks, a 16 MiB payload ceiling, count/capacity checks, executable/campaign
identity checks and CRC32 over header plus payload. CRC32 detects accidental
corruption, not malicious modification. Capacity is bounded at 2048 for format
validation, NOT a claim of UI or game support at 2048.

The payload is opaque. Its metadata count is NOT a substitute for validating the
native bucket's actual count; the future adapter must validate both. The core's
transaction state machine rejects stale/repeated actions and freezes ambiguous
outcomes. It neither persists files nor removes game items by itself.

The SQLite test model writes two synthetic keys in one transaction and kills the
child process at several write points. This validates the proposed protocol on
SQLite, not the actual game ABI or transaction/error handling.

## Remaining prerequisites for a playable candidate

- Full native item/bucket serialization and reconstruction with item-property
  comparison, correct allocator ownership and no collisions on restored IDs.
- Native save lifecycle, transaction result handling, rollback and table/key
  preservation, including a vanilla session after mod use.
- A reliable campaign binding and deliberate test-slot enrollment flow.
- UI integration that exposes the independent bucket and safe transfers without
  overwriting Improved Inventory. Its static drawer/background capacities need
  combined-panel limits and realistic item-count testing.
- Runtime safeguards, dependency/executable checks before hooks, and Mewtator
  packaging only once those paths exist.

No code should silently turn this foundation into a runtime limit patch or an
external save editor as a shortcut.
