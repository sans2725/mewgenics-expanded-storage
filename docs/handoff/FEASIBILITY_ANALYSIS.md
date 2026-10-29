# Mewgenics Expanded Storage — Feasibility Analysis

## 1. Executive conclusion

**Overall assessment: high feasibility, with a critical live-validation gap.**

The available evidence strongly supports that Mewgenics serializes Storage as a variable-length collection rather than a fixed 100-element save array. Improved Inventory already solves most of the presentation problem for large collections. Mewjector API v3 provides a supported-by-the-loader mechanism for multiple DLL mods to install chainable hooks.

The unproven step is the complete gameplay transition from 100 to 101 Storage items. The original game may enforce the limit in one or more capacity predicates and may assume a fixed number of native drawer/background UI objects. Until item 101 is inserted, displayed, saved, reloaded, and removed on a copied test campaign, expansion beyond 100 must be described as a well-supported hypothesis rather than a finished capability.

## 2. Evidence classification

### 2.1 Verified directly on the user's computer

The following facts were checked locally during the prior conversation:

- `Mewgenics.exe` is Steam build `25143593`.
- Its SHA-256 is:

  ```text
  4127CD6A792AE528BCA6F65A8873DD61789591937D87656C2B586A5E30EB77EA
  ```

- This hash exactly matches the executable documented in the relevant reverse-engineering work.
- Mewtator and Mewjector are installed in the game environment.
- The user successfully launched DLL mods after accepting Steam's launch-parameter confirmation.
- The user reported that the two enabled DLL mods loaded successfully in game.
- Improved Inventory v0.3.7 is present.
- More than one physical Improved Inventory DLL copy exists under the mods tree; whether both are reachable by the active loader manifest was not verified.

No files or saves were modified during the feasibility analysis.

### 2.2 Verified by public source/research documentation

The following conclusions are documented by public projects targeting the same executable hash:

#### Variable-length native inventory and serialization

The independent multiplayer reverse-engineering notes identify:

- A native `Inventory` singleton.
- Separate Backpack, Storage, Trash, and another inventory bucket.
- `inventory_storage` at the researched native object offset `Inventory+120`.
- Inventory buckets implemented as intrusive doubly linked lists.
- A serializer that walks those lists and serializes each `Equipment` record.

This supports the conclusion that Storage data is not serialized as a fixed array of exactly 100 entries. It does **not** by itself prove that all normal gameplay/UI code accepts entry 101.

Source: [Mewgenics reverse-engineering notes for the matching executable](https://github.com/SanTertrust/mgmp/blob/main/CLAUDE.md)

#### Improved Inventory presentation and conservation tests

Improved Inventory's published research reports:

- A temporary campaign copy successfully exercised 37 transfers into Trash and scrolled beyond the original 36 visible positions.
- Transferred items were returned and save conservation was checked.
- A synthetic browser fixture handled 1,000 items; this proves UI logic in the fixture, not full in-game performance.
- An observed save contained 94 serialized Storage records while the active native presentation exposed 92 drawers and 100 Storage backgrounds.
- The reason for the two-record discrepancy was not fully resolved; quest/special-item visibility is one relevant factor.
- The implementation intentionally operates on existing native drawers and does not manufacture missing entries.
- The shared layout routine caps columns at 20, not the total inventory record count.

Sources:

- [Inventory feasibility research](https://github.com/anitosq/mewgenics-mods/blob/main/docs/research/inventory-feasibility.md)
- [Native inventory notes](https://github.com/anitosq/mewgenics-mods/blob/main/docs/research/native-inventory-notes.md)
- [Improved Inventory project README](https://github.com/anitosq/mewgenics-mods/blob/main/mods/improved-inventory/README.md)

#### Loader/API compatibility mechanism

Mewjector API v3 exposes functions including chainable hook installation, hook queries, namespace/name registration, game-base lookup, shared logging, integrity verification, and API-version queries. This is a suitable mechanism for coexistence between multiple native DLL mods when used correctly.

Source: [Mewjector repository and API overview](https://github.com/githubuser508/mewjector)

### 2.3 Strong inferences, not yet live-verified

The evidence supports these inferences:

1. **The vanilla 100-item maximum is probably not a save-format hard limit.** The variable-length linked-list serializer is the strongest reason.
2. **The effective limit is probably enforced by gameplay capacity logic and/or presentation allocation.** The observed 100 Storage backgrounds and fewer active drawers point toward a presentation/capacity boundary.
3. **A separate native DLL can likely raise the limit while Improved Inventory handles browsing.** The loader supports chained hooks and Improved Inventory already handles scrolling over native drawer collections.
4. **A finite expansion is safer than an unbounded one.** Save size, native object allocation, UI rebuild work, and unknown counters all grow with item count.

These are engineering hypotheses to test, not release claims.

### 2.4 Unverified hypotheses

The following must remain explicitly unverified until tested:

- The 101st Storage item can be created through normal gameplay without an additional native drawer-allocation hook.
- Every relevant insertion path shares one capacity predicate.
- Raising a single comparison or capacity getter is sufficient.
- Improved Inventory v0.3.7 displays, filters, and transfers items above 100 without modification.
- Save/load remains safe at 120, 200, 300, or 500 items.
- Vanilla Mewgenics can load a save containing more than 100 Storage items after Expanded Storage is disabled.
- Duplicate copies of the same DLL cannot be loaded by the current Mewtator/Mewjector configuration.
- The prior missing-cursor problem is unrelated to native hook ordering.

## 3. Relevant native research leads

All addresses below are research leads pinned to the documented executable. They must be revalidated in the actual binary and must not be copied blindly into production code.

Improved Inventory's native notes identify candidate presentation functions at these relative virtual addresses:

- `0x211700`: shared Storage/Trash grid layout.
- `0x211460`: lookup rebuild.
- `0x210590`: native drawer/layout rebuild.
- `0x213DC0`: native single-item transfer callback.
- `0x2137E0`: bind item identity and sort metadata.

Observed owner fields include:

- Offsets `64/68`: drawer count/array.
- Offsets `74/78`: Storage background count/array.
- Offsets `84/88`: Trash background count/array.

There is a conflict or evolution in public notes around the common insertion function:

- Improved Inventory feasibility notes mention an `Inventory::insert_item` string-reference candidate near RVA `0x2E13F0`, explicitly not yet validated as the desired transfer operation.
- Independent reverse engineering identifies `glaiel::Inventory::insert_item` at virtual address `0x1402E0920` for the matching executable and reports 36 callers.

This discrepancy must be resolved through disassembly, cross-references, signatures, and runtime observation. Production code must not assume either address is correct solely because it appears in a document.

## 4. Proposed technical implementation

### 4.1 Architecture

The recommended architecture is a separate Windows x64 DLL, tentatively `ExpandedStorage.dll`, loaded by Mewjector and managed by Mewtator.

Responsibilities should be separated as follows:

- **Expanded Storage:** logical capacity, any required native drawer allocation, compatibility checks, configuration, logging, and safety behavior.
- **Improved Inventory:** scrolling grid, search, filtering, readable icons, and existing inventory presentation enhancements.
- **Mewjector:** DLL loading and chainable hook API.
- **Mewtator:** installation, enable/disable state, dependency/load order, and game launch.

### 4.2 Likely hook strategy

The research phase should locate, in this order:

1. The function that calculates or returns maximum home Storage capacity.
2. The predicate that reports Storage full.
3. The common item insertion routine and the callers used at home and after adventure.
4. The native drawer/background allocation or rebuild path.
5. Save/load boundaries and any post-load clamping or truncation.

The preferred intervention is the narrowest shared logical-capacity hook. Additional presentation allocation hooks should be added only if item 101 proves that the game cannot create the required native objects automatically.

### 4.3 Compatibility strategy

- Resolve Mewjector API v3 at runtime and check its version.
- Register a unique mod/owner name.
- Use chainable hooks rather than overwriting another mod's trampoline.
- Detect unsupported game builds before hook installation.
- Query existing hooks where useful and log relevant coexistence information.
- Avoid modifying `ImprovedInventory.dll`, its SWFs, or its installed files.
- Detect or at least diagnose duplicate Expanded Storage loading.

## 5. Dependencies

### Required runtime dependencies

- Windows x64.
- Mewgenics 1.1.21239 / Steam build 25143593 for the first supported release.
- Mewjector with API v3.
- Mewtator for the intended end-user install/launch flow, although Mewjector may technically load a DLL without it.

### Companion UI dependency

- Improved Inventory v0.3.7 is the tested target and should be installed for the intended experience.
- Whether it is technically mandatory is still an open decision.

### Development dependencies

Not yet selected or confirmed. Candidate stacks are documented in `TECHNICAL_DECISIONS.md`.

## 6. Technical limitations

### 6.1 No official native modding SDK

Mewgenics is a native C++ game using proprietary engine/data structures. The work relies on reverse engineering and therefore carries update and memory-safety risk.

### 6.2 Exact-build sensitivity

Addresses, structure layouts, and signatures can change with any game update. The first release should support only the verified build unless a new build is separately analyzed and tested.

### 6.3 Presentation objects may remain bounded

The save container can be variable-length while the home UI still preallocates 100 backgrounds or another fixed number of drawers. Logical expansion may therefore require controlled creation/reuse of presentation objects.

### 6.4 Multiple insertion paths

Adventure return, manual transfer, unequip, rewards, and other systems may check capacity at different layers. A partial fix could silently work in one scenario and fail in another.

### 6.5 Large-inventory performance

Improved Inventory has already documented performance work for large inventories. This project must not advertise a high maximum until opening, sorting, filtering, scrolling, saving, and loading are measured in game.

### 6.6 Downgrade uncertainty

Even if the vanilla serializer reads all records, the vanilla UI may hide over-limit items or prevent operations after the mod is disabled. Removal safety is unresolved.

## 7. Rejected or deferred approaches

### 7.1 UI-only expansion

**Rejected as insufficient.** Drawing or scrolling more cells does not change the gameplay check that refuses new items when Storage is full.

### 7.2 Save-editor workflow as the product

**Rejected for normal use.** Public save editors demonstrate that inventory records and storage-upgrade properties are editable, but repeatedly editing a save does not provide the requested seamless in-game experience and increases user-error risk.

### 7.3 Patching Improved Inventory's DLL

**Rejected.** It would tightly couple the project to another author's binary, complicate updates and licensing, and increase hook conflicts. A companion DLL targeting game behavior is cleaner.

### 7.4 Unlimited capacity in the first release

**Deferred/rejected for v1.** “Unlimited” creates unclear memory, UI, save-size, performance, and integer-boundary behavior. A configurable finite range should be proven first.

### 7.5 Blind fixed-address binary patch

**Rejected.** A hardcoded patch without executable verification, signature validation, and fail-closed behavior is too risky for a save-affecting native mod.

### 7.6 Testing first on the primary campaign

**Rejected.** All insertion and persistence experiments must use a copied campaign or dedicated test slot with a recoverable backup.

### 7.7 Relying only on the `storage_upgrades` save property

**Not established and therefore deferred.** Save editors expose the property, and an existing “Bigger Storage Early” mod can expose the vanilla 10-by-10 capacity earlier. Neither fact proves that setting an upgrade value beyond the authored maximum creates safe capacity beyond 100.

## 8. Required validation program

### Gate 1 — Static and read-only analysis

- Confirm executable hash.
- Obtain symbols/signatures only from legally distributable research/code.
- Identify capacity messages, predicates, insert call sites, and allocation paths.
- Map Improved Inventory's hooks and ensure proposed hook points do not collide.
- Confirm Mewjector API v3 behavior and hook priorities.

### Gate 2 — Diagnostic probe

- Build a DLL that performs version checks and logs read-only Storage counts/capacity decisions.
- Do not alter save data or capacity yet.
- Confirm one-time loading and detect duplicate DLL instances.

### Gate 3 — Minimal expansion proof

- Copy a campaign into an authorized test slot.
- Reach exactly 100 ordinary visible Storage items.
- Configure or hard-limit the experiment to 101 or 120.
- Insert item 101 through a normal in-game action.
- Verify visibility, selection, transfer out, save, full exit, and reload.
- Compare item counts and representative serialized fields before and after.

### Gate 4 — Path coverage

- Test manual transfer.
- Test adventure-return loot.
- Test unequip-to-Storage behavior.
- Test any identified reward/shop/event insertion paths.
- Test Trash interactions and quest/special-item filtering.

### Gate 5 — Stress, compatibility, and removal

- Test the proposed default and maximum capacity.
- Measure UI and save/load performance.
- Test with Improved Inventory only, then with the user's usual enabled mods.
- Test duplicate/incorrect load order.
- Test disabling Expanded Storage only after making backups, and determine a documented safe removal procedure.

### Gate 6 — Packaging and reproducibility

- Rebuild from a clean English-path checkout.
- Import the release ZIP through Mewtator.
- Verify dependency and version warnings.
- Audit the repository and release artifact for game files, saves, credentials, account IDs, and personal paths.

## 9. Source and licensing notes

- Improved Inventory's source repository is publicly available and marked MIT. If implementation code is reused rather than merely studied, preserve the required copyright and license notice.
- Mewjector's public API/header and repository license must be reviewed before vendoring or redistributing files.
- Do not redistribute Mewgenics binaries, extracted assets, saves, or decompiler databases.
- Prefer documenting compatible dependencies rather than bundling their binaries.

Primary references:

- [Improved Inventory on Nexus Mods](https://www.nexusmods.com/mewgenics/mods/526)
- [Improved Inventory source repository](https://github.com/anitosq/mewgenics-mods)
- [Inventory feasibility research](https://github.com/anitosq/mewgenics-mods/blob/main/docs/research/inventory-feasibility.md)
- [Native inventory notes](https://github.com/anitosq/mewgenics-mods/blob/main/docs/research/native-inventory-notes.md)
- [Mewjector](https://github.com/githubuser508/mewjector)
- [Independent matching-build reverse engineering](https://github.com/SanTertrust/mgmp/blob/main/CLAUDE.md)
- [Mewtator](https://github.com/dancomstock/mewtator)

## 10. Final feasibility statement

The data model appears capable of storing a variable number of Storage items, the UI companion already handles scrolling collections, and the loader supports cooperative native hooks. These three facts make the project technically credible.

The project should proceed, but its first milestone must be treated as an experiment: locate the real capacity boundary, prove item 101 end to end on a copied save, and only then commit to public capacity numbers or an implementation architecture beyond the narrow hook actually required.

