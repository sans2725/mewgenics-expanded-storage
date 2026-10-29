# Validation gates

## Current scope

Build only. Do not launch the game or touch player saves. The owner will later
clear and create one of the three slots. No existing slot is authorized.

## Offline checks

- Compile Windows x64 DLL/CLI with warnings treated as errors.
- Undefined-behavior sanitizer on portable C tests.
- Encode/decode 600 distinct synthetic records; reject every truncation and
  every single-byte corruption of that fixture.
- Wrong build/campaign, invalid limits, overlapping buffers and stale transactions.
- Real child-process termination before/after synthetic SQLite writes and commit.
- Simulated vanilla play consuming an ordinary item without resurrecting it.
- Full native-capacity model refuses withdrawal without changing either key.
- Corrupt archive is preserved and prevents transfer.

## Later game tests (not authorized or performed yet)

Before first deployment, identify the exact newly created campaign, take a
verified backup of all existing slots, document restore steps, and inspect the
enabled Mewtator DLL list. The module is loaded process-wide before slot selection;
campaign binding must be proved before any item mutation is enabled. Avoid opening
the primary campaign during initial tests. Backups are necessary even with guards.

1. Read-only probe on the new slot; identify save lifecycle and native structures.
2. A uniquely named test blob: save, exit, run without the mod, save again, and
   verify preservation. Test deletion/recreation of this test slot separately.
3. One ordinary expendable item through native serialize/deserialize, compare all
   meaningful properties. Then charged/durable/cursed/variant items.
4. Deposit/withdraw one item with same-transaction commit and visible UI feedback.
   Reject full Storage, uncertain commit, malformed payload and wrong campaign.
5. Verify 600 vault items, repeated reloads, normal NPC donations and the user's
   lowered-donation mod, without advancing NPC progress through the extension.
6. Remove Expanded Storage: native containers remain within native limits, play
   normally, save/exit, reload the mod, verify the vault and no duplicated items.
7. Repeat without Improved Inventory; native-only sessions must not consume vault
   items. Later reinstall Improved Inventory and verify view/transfer behavior.
8. Quit/restart, interrupted writes on disposable copies, restored whole-save
   backups, and conflict/rollback handling. Do not claim cloud merge safety.
9. Compatibility with only prerequisites, then user's full mod set; verify cursor,
   search/filter/scroll, frame time and all Storage acquisition paths.

Any unexplained item loss, duplication, cross-campaign effect, changed NPC progress,
or archive disappearance is a release blocker. Never advertise a general public
version based solely on synthetic tests.
