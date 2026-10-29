# Mewgenics Expanded Storage — Project Requirements

## 1. Document purpose

This document is a standalone handoff for a new Codex conversation and a new project directory. It consolidates the product requirements discussed before development began.

The project is currently in the requirements and feasibility stage. No storage-expansion mod source code has been written, no new project has been initialized, and no GitHub repository has been created as part of the prior work.

Statements in this document use the following labels:

- **Confirmed requirement**: explicitly requested by the user or repeatedly agreed during discussion.
- **Proposed requirement**: recommended during feasibility analysis but not yet explicitly finalized by the user.
- **Open question**: must be decided or experimentally verified before it can be treated as a commitment.

## 2. Project background

### 2.1 Target game and environment

- Game: **Mewgenics**
- Platform: **Windows x64, Steam**
- Target game version: **Mewgenics 1.1.21239**
- Target Steam build: **25143593**
- Verified local executable SHA-256:

  ```text
  4127CD6A792AE528BCA6F65A8873DD61789591937D87656C2B586A5E30EB77EA
  ```

- Verified local game location at the time of analysis:

  ```text
  <Steam library>\steamapps\common\Mewgenics
  ```

The local build number and executable hash exactly match the build used by the relevant public reverse-engineering notes. This does not authorize copying the game executable into the new project.

### 2.2 Existing mod environment

The user already uses:

- **Mewtator v1.0.0** as the mod manager.
- **Mewjector v3.5 / API v3** as the DLL loader and hook API.
- **Improved Inventory v0.3.7** as the scrolling/search/filter inventory UI mod.

Earlier troubleshooting established that DLL mod loading works when Mewgenics is launched through Mewtator and the Steam launch-parameter confirmation is accepted. The user reported that the two enabled DLL mods loaded successfully in game.

An earlier session also produced a missing in-game mouse cursor after DLL mods loaded. The accessible history does not contain a confirmed diagnosis or resolution for that cursor issue. It is not a feature requirement for Expanded Storage, but it is an environmental risk that can interfere with testing and must not be assumed resolved.

### 2.3 Core idea

Working project name: **Mewgenics Expanded Storage**.

The user wants a companion mod for Improved Inventory that expands the home Storage capacity beyond the vanilla maximum. The motivation is to preserve rare items and collect more equipment combinations instead of discarding valuable adventure loot when Storage is full.

Improved Inventory already presents large inventories as a readable scrolling grid. This project should change the underlying gameplay capacity while allowing Improved Inventory to remain responsible for the enhanced presentation.

The name is a working name, not a final branding decision.

## 3. Product goal

The desired end state is:

1. The player enables both Improved Inventory and Expanded Storage through Mewtator.
2. Storage accepts more items than the vanilla 10-by-10 / 100-slot limit.
3. Additional items are visible and usable through Improved Inventory's scrolling, search, and filtering interface.
4. Normal game paths that place items into Storage behave consistently rather than rejecting items at the old limit.
5. Extra items survive save, exit, and reload without loss or property corruption.
6. Unsupported game versions fail safely instead of applying stale binary hooks.
7. The project can be published as the user's first open-source GitHub project without distributing copyrighted game files or third-party mod binaries.

## 4. Functional requirements

### FR-01 — Expand the logical Storage capacity

- **Status:** Confirmed requirement.
- **Description:** Permit the home Storage container to hold more than the vanilla maximum of 100 items.
- **Expected behavior:** When the stored-item count reaches 100, the next valid item can still be placed into Storage up to the configured or supported new limit.
- **Use cases:** Keeping rare adventure loot, maintaining multiple equipment combinations, collecting item sets, and postponing destructive inventory cleanup.
- **Confirmed implementation requirement:** The change must affect actual gameplay storage behavior, not merely draw additional empty cells.
- **Open details:** Final default capacity and maximum supported capacity have not been approved. Values such as 120 for the first experiment and 200 or 300 for a public release were recommendations, not final requirements.

### FR-02 — Work with Improved Inventory

- **Status:** Confirmed requirement.
- **Description:** Operate alongside Improved Inventory v0.3.7 on the target build.
- **Expected behavior:** Extra stored items remain reachable through the existing scrolling grid and its search/filter features.
- **Use cases:** Browsing a collection larger than one screen without shrinking icons to unreadable sizes.
- **Confirmed implementation requirement:** Do not patch or replace `ImprovedInventory.dll` directly. Prefer an independent companion DLL and game-level hooks.
- **Open details:** Whether Improved Inventory is declared as a hard Mewtator dependency or a strongly recommended soft dependency remains undecided. The mod may be logically usable without it but would likely have an unacceptable vanilla UI above 100 items.

### FR-03 — Cover all relevant item-entry paths

- **Status:** Confirmed requirement at the behavioral level; implementation paths remain to be mapped.
- **Description:** Expansion should not work only for one drag-and-drop action.
- **Expected behavior:** Storage capacity is applied consistently wherever the game legitimately attempts to insert an item into home Storage.
- **Use cases to validate:**
  - Manual transfers at home.
  - Returning from an adventure with loot.
  - Unequipping items into Storage.
  - Rewards, events, shops, or other systems that may insert directly into Storage.
  - Transfers involving Trash when Storage is involved.
- **Confirmed implementation requirement:** Prefer a shared capacity predicate or common insertion path over scattered call-site patches.
- **Open details:** The complete call graph is not yet known. Each path must be observed on the target executable before coverage can be claimed.

### FR-04 — Persist expanded inventories safely

- **Status:** Confirmed requirement.
- **Description:** Items beyond slot 100 must survive normal save/load cycles.
- **Expected behavior:** After saving, quitting, and reloading, item count and meaningful item properties remain intact.
- **Validation fields:** Item type/name, rarity-related state, stat fields, charges or durability where applicable, curse/equipment-related state, and container membership.
- **Known nuance:** Runtime object IDs may be regenerated during load; identity should therefore be validated using serialized item data rather than assuming process-local IDs persist.
- **Open details:** Ordering changes may be normal. The acceptable ordering behavior should be documented after observation.

### FR-05 — Configurable bounded capacity

- **Status:** Proposed requirement, strongly recommended for the first public release.
- **Description:** Read a finite Storage capacity from a simple configuration file.
- **Expected behavior:** A player can choose a supported capacity without rebuilding the DLL; invalid values are rejected or clamped safely and logged.
- **Recommended initial direction:** Use a finite value such as 120 during the first live proof and later support a documented range, potentially 100–500.
- **Open details:** Configuration filename, syntax, default value, range, and whether changes require restart are not finalized.

### FR-06 — Safe version compatibility guard

- **Status:** Confirmed implementation requirement.
- **Description:** Verify the game executable before installing hooks.
- **Expected behavior:** On an unsupported build or hash, the DLL logs a clear incompatibility message and remains inactive.
- **Reason:** This is a reverse-engineered native game with version-specific addresses and layouts. Applying stale hooks can corrupt memory or saves.
- **Open details:** The final guard may use the exact SHA-256, signature scanning plus structural validation, or both. Exact-address-only patching is not acceptable as the only protection.

### FR-07 — Diagnostic logging

- **Status:** Proposed requirement.
- **Description:** Produce concise diagnostic output through Mewjector's shared logging API and, if useful, a mod-specific log.
- **Expected behavior:** Logs report DLL initialization, detected build, configured limit, installed hooks, compatibility with Improved Inventory, refused operations, and fail-closed conditions.
- **Privacy requirement:** Do not log save contents, Steam identifiers, GitHub credentials, or unnecessary personal paths.
- **Open details:** Final verbosity levels and log destination are undecided.

### FR-08 — Performance remains playable

- **Status:** Confirmed quality requirement.
- **Description:** Larger Storage must not cause unacceptable pauses, excessive per-frame work, or unstable UI rebuilds.
- **Expected behavior:** Opening, scrolling, searching, filtering, inserting, saving, and loading remain usable at every advertised capacity.
- **Open details:** Quantitative performance thresholds have not been chosen. Tests should cover at least the proposed default and maximum supported capacity.

### FR-09 — Safe downgrade and uninstall guidance

- **Status:** Proposed requirement, required before public release.
- **Description:** Prevent or clearly document risky removal while a save contains more than 100 stored items.
- **Expected behavior:** The README and logs explain how to reduce Storage to the vanilla limit before disabling the mod, unless testing proves vanilla safely preserves and exposes over-limit data.
- **Open details:** Automatic refusal, warning-only behavior, or a future migration tool must be chosen after vanilla reload testing.

### FR-10 — Mewtator-friendly distribution

- **Status:** Confirmed requirement.
- **Description:** Package the release in a structure that Mewtator can import, enable, order, and launch.
- **Expected behavior:** Installation does not require manually overwriting game resources or another mod's DLL.
- **Open details:** Exact manifest schema, dependency declaration, load order, and ZIP layout must be verified against the current Mewtator documentation and a real import test.

### FR-11 — Open-source publication

- **Status:** Confirmed user goal.
- **Description:** Prepare the project for publication as the user's first GitHub open-source project.
- **Expected contents:** Source code, build instructions, configuration example, compatibility table, test notes, installation guide, troubleshooting, credits, and license.
- **Must not publish:** Game executable/assets, user saves, crash logs containing personal data, Steam account identifiers, GitHub secrets, Improved Inventory DLL, or Mewjector binaries unless their licenses and redistribution terms explicitly allow it and redistribution is intentionally chosen.
- **Open details:** Repository owner/name and final license are not yet finalized. MIT was recommended.

## 5. Non-functional and safety requirements

### NFR-01 — Preserve user saves

- Never use the user's primary campaign as the first test target.
- Make an explicit backup or use a temporary copied test slot.
- Record before/after item counts and representative item properties.
- Treat save corruption, silent item loss, or inaccessible over-limit items as release blockers.

### NFR-02 — Fail closed

- Unsupported executable: no hooks.
- Failed signature or layout validation: no hooks.
- Missing required dependency: warn and remain inactive if safe operation cannot be guaranteed.
- Unexpected container state: log and preserve vanilla behavior rather than guessing.

### NFR-03 — Avoid mod conflicts

- Use Mewjector API v3 chainable hooks and unique registration names.
- Avoid modifying Improved Inventory's binary.
- Verify hook order and coexistence with Improved Inventory v0.3.7.
- Test with only required mods first, then with the user's normal mod set.

### NFR-04 — English development paths

- The new project root and all newly created development folders should use short, conventional English names.
- Suggested roots include `C:\dev\mewgenics-expanded-storage` or another user-selected English-only path.
- Do not rename game-required resource identifiers, exported names, localization keys, in-game display strings, or compatibility-sensitive filenames merely to make them English.
- Do not copy the old workspace wholesale.
- Do not modify or delete the old workspace.

### NFR-05 — Reproducible builds

- A clean checkout should be buildable from documented tool versions and commands.
- Third-party source/header versions should be pinned or vendored in compliance with their licenses.
- Release artifacts should identify the supported game build and mod version.

## 6. Expected player experience

The intended experience is deliberately simple:

1. Install and enable Mewjector support, Improved Inventory, and Expanded Storage through Mewtator.
2. Launch the game through Mewtator and accept Steam's launch-parameter confirmation if displayed.
3. Continue using Storage normally.
4. When Storage exceeds 100 items, continue browsing it with Improved Inventory's scrolling/search/filter controls.
5. Save and reload normally without manually editing the save file.

The player should not need a save editor, a debug console, or repeated manual file changes during ordinary play.

## 7. MVP acceptance criteria

The first technical MVP is complete only when all of the following are demonstrated on a copied test campaign:

1. The DLL loads through Mewjector API v3 on the exact supported executable.
2. It refuses to activate when the executable verification is intentionally made to fail.
3. A Storage inventory at 100 items accepts item 101 through at least one normal in-game path.
4. The extra item is visible and retrievable with Improved Inventory enabled.
5. The test save can be saved, the game fully exited, and the campaign reloaded.
6. The reloaded inventory still contains all expected items and checked properties.
7. Moving an over-limit item back out of Storage works.
8. Logs clearly show activation, selected capacity, and hook status.
9. No primary save is modified.

The MVP does not by itself prove every acquisition path, maximum capacity, performance target, or uninstall behavior. Those belong to later validation gates.

## 8. Public-release acceptance criteria

Before a public release, additionally require:

- Coverage of every relevant item-entry path identified during reverse engineering.
- Stress tests at the advertised default and maximum capacity.
- Repeated save/load testing and at least one rollback/downgrade test.
- Verified Mewtator import and dependency/load-order behavior.
- Clear unsupported-build behavior.
- No duplicate hook installation when duplicate DLL files exist on disk.
- Clean repository with no game files, saves, credentials, personal IDs, or unrelated old-project files.
- Build instructions reproduced from a clean checkout.
- README, license, credits, changelog, and release ZIP.

## 9. Out of scope for the initial release

- Rewriting Improved Inventory's search/filter/scroll UI.
- Editing the user's save file as the normal mechanism for expansion.
- Supporting arbitrary historical or future Mewgenics builds without validation.
- A mathematically unlimited inventory.
- Cloud synchronization changes.
- Inventory changes to Backpack, Trash, equipped items, furniture storage, or other containers unless required for safe Storage transfer behavior.
- Resolving the previously observed missing mouse cursor unless it is reproduced and shown to be caused by Expanded Storage or to block required testing.

## 10. Known environment cautions

- Two physical Improved Inventory DLL copies were observed under the Mewtator mods tree: one in the normalized `ImprovedInventory` directory and another under an older nested `ImprovedInventory-0.3.7` extraction directory. The active Mewtator manifest was not audited during the handoff task. Before live hook testing, confirm that only one copy is loaded. Do not delete either copy without explicit user approval.
- The Steam launch dialog previously prevented DLL-mod startup until the user confirmed the launch parameters.
- The missing-cursor issue is unresolved in the accessible history.

## 11. Open product decisions

The new development conversation must not silently decide these:

1. Final project and repository name.
2. Final public license; MIT is only the current recommendation.
3. Default capacity and advertised maximum.
4. Whether Improved Inventory is a hard or soft dependency.
5. Configuration format and location.
6. Exact downgrade/uninstall policy.
7. Quantitative performance thresholds.
8. Which copied campaign/test slot the user authorizes for live testing.
9. Whether the unresolved cursor issue must be fixed before storage work proceeds.

## 12. Information gaps

- No original game source or official modding SDK is available; all native behavior is reverse engineered.
- No successful insertion of Storage item 101 has yet been recorded in the accessible history.
- No compiler or build-system installation has been confirmed.
- No final language/build-tool choice has been made.
- No GitHub repository has been created or connected.
- No official response from the Improved Inventory author to the user's feature question is present in the accessible history.

