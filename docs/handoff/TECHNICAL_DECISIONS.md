# Mewgenics Expanded Storage — Technical Decisions and Development Handoff

## 1. Purpose and decision states

This file records decisions that affect implementation, migration, testing, and publication. It is intended to prevent a new development conversation from repeating settled discussion or turning recommendations into unacknowledged assumptions.

Decision states:

- **Accepted:** established by user requirement or prior agreement.
- **Proposed:** preferred direction that still needs explicit confirmation or a technical spike.
- **Open:** not decided.
- **Rejected/Deferred:** should not be reintroduced without new evidence.

## 2. Decision log

### TD-001 — Create the real project in a new English-only path

- **Status:** Accepted.
- **Decision:** The development project must use a new root directory with conventional English names.
- **Reason:** The current workspace contains Chinese path components, which may create compatibility issues in native build scripts, compilers, packaging tools, or dependency managers.
- **Constraints:**
  - Do not modify or delete the old project/workspace.
  - Do not copy unknown configuration wholesale.
  - Do not rename game-required identifiers, localization keys, visible translated text, or asset names solely to avoid non-English content.
- **Suggested but not selected path:** `C:\dev\mewgenics-expanded-storage`.
- **Next action:** The user or new conversation must choose and authorize the exact new root.

### TD-002 — Keep Expanded Storage as an independent companion DLL

- **Status:** Accepted.
- **Decision:** Implement a separate DLL instead of modifying Improved Inventory's binary.
- **Reason:** Separation reduces licensing and maintenance risk, allows independent versioning, and avoids coupling UI improvements to capacity logic.
- **Consequence:** Compatibility must be achieved through game-level hooks and Mewjector hook chaining.

### TD-003 — Target one exact game build first

- **Status:** Accepted.
- **Decision:** Initial support is limited to Mewgenics 1.1.21239, Steam build 25143593, verified SHA-256:

  ```text
  4127CD6A792AE528BCA6F65A8873DD61789591937D87656C2B586A5E30EB77EA
  ```

- **Reason:** Native addresses and structure layouts are reverse engineered and update-sensitive.
- **Consequence:** Unsupported builds must fail closed. Additional builds require separate signatures and validation.

### TD-004 — Use Mewjector API v3 rather than a private loader

- **Status:** Accepted.
- **Decision:** Integrate with the already installed Mewjector API v3.
- **Reason:** It is designed for Mewgenics DLL mods and exposes chainable hooks, registration, shared logging, base-address lookup, and hook verification.
- **Consequence:** The implementation should resolve/check the API at runtime and use unique ownership names.
- **Version note:** The installed loader was identified as Mewjector v3.5 with API v3. The API version is the compatibility contract; the exact loader release still needs to be recorded in reproducible environment documentation.

### TD-005 — Use Mewtator for the end-user workflow

- **Status:** Accepted.
- **Decision:** The release should be importable and manageable through Mewtator, and the documented launch path should be Mewtator.
- **Reason:** This matches the user's existing setup and dependency/load-order workflow.
- **Known launch behavior:** Steam may ask the user to confirm launch parameters. Failure to confirm previously prevented successful startup.

### TD-006 — Delegate large-inventory presentation to Improved Inventory

- **Status:** Accepted in principle.
- **Decision:** Do not recreate scrolling, search, filtering, and readable inventory icons in the first release.
- **Target:** Improved Inventory v0.3.7 on build 25143593.
- **Reason:** Those features already exist and are the basis of the user's desired combined experience.
- **Open point:** Hard versus soft dependency declaration.

### TD-007 — First prove a finite expansion, not unlimited capacity

- **Status:** Proposed, strongly recommended.
- **Decision:** Use a small bounded proof such as 101 or 120 items, then introduce a configurable finite limit.
- **Reason:** “Unlimited” is not meaningful in a native application and hides unknown memory, counter, UI, serialization, and performance bounds.
- **Deferred:** Unlimited or effectively unbounded mode.
- **Open point:** Default and maximum values for a public release.

### TD-008 — Prefer a shared logical hook over scattered patches

- **Status:** Proposed pending reverse engineering.
- **Decision:** First search for a capacity getter/full predicate/common insertion boundary. Patch individual callers only when evidence shows they bypass the shared path.
- **Reason:** One validated invariant is easier to maintain and less likely to produce inconsistent behavior.
- **Constraint:** Do not assume the documented `Inventory::insert_item` candidate is the final hook without resolving conflicting address notes and observing runtime behavior.

### TD-009 — Use executable verification and fail-closed initialization

- **Status:** Accepted.
- **Decision:** Verify the target binary before hook installation and leave vanilla behavior untouched on any mismatch.
- **Reason:** A stale native hook can crash the game or corrupt a save.
- **Proposed mechanism:** Exact hash for the initial build plus byte signatures and structural sanity checks where practical.

### TD-010 — Use copied test saves only during risky validation

- **Status:** Accepted.
- **Decision:** Never make the first capacity or persistence experiment on the user's main campaign.
- **Reason:** Item loss and save corruption are plausible until the full path is verified.
- **Consequence:** Testing instructions must name the copied slot, backup, expected item count, and rollback procedure.

### TD-011 — Develop locally before GitHub publication

- **Status:** Accepted workflow recommendation.
- **Decision:** Build and review locally first; connect GitHub only when a useful baseline exists.
- **Reason:** GitHub credentials are not required to write, build, test, or locally review the project.
- **Security rule:** The user must never paste GitHub passwords, one-time codes, private keys, or personal access tokens into the chat. Later authentication should use the user's browser, Git Credential Manager, or GitHub CLI device flow on the user's computer.
- **Consequence:** Repository creation, pushing, and pull-request creation are separate explicit actions.

### TD-012 — Do not distribute proprietary or user-specific files

- **Status:** Accepted.
- **Decision:** Publish only original source, permitted dependency material, documentation, configuration examples, tests, and intentional release artifacts.
- **Exclude:** Game executables/assets, saves, personal logs, Steam identifiers, credentials, Improved Inventory DLL, and unreviewed third-party binaries.

### TD-013 — License is not final

- **Status:** Open; MIT proposed.
- **Decision needed:** Choose the repository license before public publication.
- **Current recommendation:** MIT, matching the permissive style of relevant upstream modding projects.
- **Constraint:** If upstream code is copied, preserve its copyright/license notices and document the reused portions.

### TD-014 — Diagnose duplicate DLL loading before live hooks

- **Status:** Accepted prerequisite.
- **Decision:** Audit the Mewtator manifest and Mewjector log before installing the experimental DLL.
- **Reason:** Two physical Improved Inventory copies were observed. Multiple installations of the same native hook can invalidate compatibility testing.
- **Constraint:** Do not delete or move either existing copy without user approval.

### TD-015 — Treat the cursor issue as an environment risk, not a storage feature

- **Status:** Accepted classification.
- **Decision:** Record and reproduce the missing-cursor problem before deciding whether it belongs in this project.
- **Reason:** The accessible history does not establish its cause. Folding an unrelated input bug into the storage MVP would expand scope without evidence.
- **Escalation rule:** If Expanded Storage changes reproduce or worsen it, it becomes a release blocker.

## 3. Technology selection status

### 3.1 Programming language

- **Status:** Open.
- **Candidates:**
  - **C or C++ with MSVC:** Natural fit for a Windows x64 DLL and Mewjector's C-style exported API/header. Mature debugger and disassembler integration.
  - **Zig:** Improved Inventory's native source/build workflow uses Zig, which may simplify studying or sharing build conventions with that project.
- **Do not assume:** The prior discussion mentioned “C++/Zig” as alternatives; it did not select one.
- **Selection spike:** Build a minimal DLL that resolves Mewjector API v3, verifies the executable, logs once, and unloads cleanly. Compare build reproducibility, header interoperability, debugging, and binary size.

### 3.2 Build system

- **Status:** Open.
- **Candidates:** `CMake` with MSVC or `build.zig` with a pinned Zig toolchain.
- **Requirement:** One documented command must produce the release DLL from a clean English-path checkout.
- **Do not assume:** No compiler, CMake, Ninja, Visual Studio Build Tools, or Zig version has been confirmed installed.

### 3.3 Hooking and native API

- **Selected:** Mewjector API v3 chainable hooks.
- **Avoid:** Shipping a second generic injector/loader or directly overwriting another mod's trampoline.
- **Open:** Whether any additional lightweight disassembly/signature library is needed. Prefer minimal dependencies.

### 3.4 Testing tools

- **Status:** Partially open.
- **Required capabilities:**
  - Native unit tests for configuration, signature selection, validation, and bounds logic.
  - A read-only inventory/save inspection or comparison tool.
  - Structured runtime logs.
  - Manual in-game test checklist.
- **Constraint:** A save editor may be used as an independent inspection aid, but not as the product's capacity mechanism.

### 3.5 Reverse-engineering tools

- **Status:** Not selected/installed in the accessible history.
- **Possible tools:** Ghidra, IDA Free/Pro if already licensed, x64dbg, Visual Studio debugger, PE inspection tools, and signature scanners.
- **Requirement:** Record tool versions and avoid committing proprietary analysis databases or game binaries.

## 4. Recommended project structure

This is a recommendation only; no directories have been created.

```text
mewgenics-expanded-storage/
├── .github/
│   └── workflows/
├── config/
│   └── ExpandedStorage.example.ini
├── docs/
│   ├── compatibility.md
│   ├── research.md
│   ├── testing.md
│   └── uninstall.md
├── include/
│   └── third_party/
├── packaging/
│   └── mewtator/
├── src/
│   ├── config/
│   ├── hooks/
│   ├── platform/
│   └── expanded_storage_entry.*
├── tests/
├── tools/
├── .gitignore
├── AGENTS.md
├── CHANGELOG.md
├── LICENSE
├── README.md
└── <build-system files>
```

Naming rules:

- Use lowercase kebab-case for the repository and ordinary folders where practical.
- Follow the chosen language's conventions for source filenames and symbols.
- Preserve exact third-party API identifiers and game identifiers.
- Keep generated build output outside source directories, such as `build/` or `out/`, and ignore it in Git.

## 5. Recommended initialization sequence

The new conversation should proceed in this order after the user chooses the new English root:

1. Verify the new root is distinct from the old workspace.
2. Inspect but do not copy old files unless an item is explicitly reviewed and selected.
3. Initialize a fresh Git repository locally.
4. Add a restrictive `.gitignore` before any analysis artifacts are generated.
5. Copy these three handoff documents into `docs/handoff/` or link to them explicitly.
6. Record the exact executable build/hash without copying the executable.
7. Fetch or vendor only license-compatible Mewjector API headers at a pinned revision.
8. Select language/build tool through the minimal-DLL spike.
9. Add a root `AGENTS.md` containing project-specific safety and review rules.
10. Create the read-only diagnostic probe before any capacity-changing hook.
11. Add tests and a copied-save protocol before live mutation.
12. Create a GitHub repository only after local build/review is useful and the user explicitly authorizes publication.

## 6. Tools and versions to confirm before implementation

The new environment audit should record, without installing anything until authorized:

- Windows version and architecture.
- `git --version`.
- Candidate compiler version: MSVC Build Tools/Visual Studio or Zig.
- Candidate build tool version: CMake/Ninja or Zig.
- Debugger/disassembler availability and version.
- Mewtator version.
- Mewjector loader version and exported API version.
- Improved Inventory version and actual loaded DLL path.
- Mewgenics build ID and executable SHA-256.
- Active Mewtator manifest and effective DLL load order.
- Location and backup status of the authorized test save.

Do not install global dependencies, change PATH, or modify the game directory merely to complete the audit.

## 7. Development phases

### Phase 0 — Environment and safety baseline

- Choose English project root.
- Audit tools and versions.
- Confirm only one Improved Inventory instance loads.
- Reproduce or explicitly defer the cursor issue.
- Establish copied-save and rollback procedure.

**Exit criterion:** Reproducible current environment record and a safe test campaign.

### Phase 1 — Read-only loader probe

- Build the smallest DLL.
- Verify Mewjector API v3 and game hash.
- Register a unique mod name.
- Log initialization exactly once.
- Read and log only non-sensitive inventory counts or decisions necessary for research.

**Exit criterion:** Clean load/unload, fail-closed mismatch test, no save changes.

### Phase 2 — Locate capacity enforcement

- Map capacity getter/full predicate/common insertion routine.
- Resolve the conflicting documented insertion addresses.
- Identify drawer/background allocation behavior.
- Document signatures, invariants, and caller coverage.

**Exit criterion:** Evidence-backed hook proposal with no capacity mutation yet.

### Phase 3 — 101-item proof

- Implement the narrowest bounded hook.
- Use a copied campaign.
- Insert, view, transfer, save, exit, and reload item 101.
- Compare inventory records and representative item fields.

**Exit criterion:** Repeatable end-to-end proof or a documented failure that identifies the next missing hook.

### Phase 4 — Complete behavior and configuration

- Cover all discovered insertion paths.
- Add validated configuration and logging.
- Define dependency/load order.
- Establish safe behavior for invalid states.

**Exit criterion:** Feature-complete beta at a finite test capacity.

### Phase 5 — Stress, downgrade, and compatibility testing

- Test proposed release limits.
- Measure UI/save performance.
- Test disable/uninstall procedure.
- Test normal user mod set after isolated validation.
- Confirm the cursor remains unaffected.

**Exit criterion:** Advertised limits and removal guidance supported by evidence.

### Phase 6 — Open-source release

- Finalize name and license.
- Clean README/build/install/test documentation.
- Reproduce build from clean checkout.
- Audit repository contents.
- Create GitHub repository with user-controlled browser authentication.
- Push, open PR, run local and GitHub review, tag, and publish a release ZIP.

**Exit criterion:** Public repository and release artifact contain no prohibited files and match the tested DLL.

## 8. Recommended Git and review workflow

- Start local-first on `main` with documentation and the minimal build skeleton.
- Use short feature branches such as `codex/storage-probe` and `codex/capacity-mvp` if the active Codex environment expects the `codex/` prefix.
- Keep commits small enough to review: environment/build, probe, signatures, capacity behavior, persistence tests, packaging.
- Run local diff review before pushing.
- After GitHub connection, use pull requests and optionally request Codex review with `@codex review`.
- Keep deterministic formatting/lint rules in CI; reserve `AGENTS.md` review rules for high-impact invariants such as fail-closed version checks, save safety, and prohibited file handling.

## 9. Suggested repository review rules

The following are candidates for the future root `AGENTS.md`; they are not created by this handoff task:

```markdown
## Code Review Rules

### Save safety

- Flag any test or runtime path that can modify a primary campaign without an explicit copied-slot guard and backup procedure.
  Safe path: operate on an authorized copied test save and verify rollback before mutation.

### Unsupported builds

- Flag native hooks installed before executable/API validation or on a failed signature/layout check.
  Safe path: fail closed and preserve vanilla behavior.

### Distribution boundaries

- Flag game binaries, extracted assets, saves, personal logs/IDs, credentials, or unapproved third-party binaries added to source control or release archives.
  Safe path: publish original source, permitted headers, documentation, and reproducible build metadata only.
```

## 10. Historical questions already answered

The new conversation should not spend time re-establishing these unless new evidence appears:

- **Is GitHub login required to begin coding?** No. Local creation, building, Git history, and review can happen first.
- **Should the user send credentials to Codex?** No.
- **Does the local game match the public reverse-engineering target?** Yes, exact build and hash match.
- **Can Improved Inventory already scroll large item collections?** Yes for its tested UI scenarios; this does not yet prove Storage item 101.
- **Is the save representation obviously fixed to 100 records?** Available research says no; it is a variable-length linked-list serialization.
- **Is unlimited Storage already proven?** No.
- **Should Improved Inventory be modified in place?** No; use a companion DLL.
- **Should the first live test use the main campaign?** No.

## 11. Outstanding decisions for the new model to surface

Before writing capacity-changing code, explicitly obtain or establish:

1. The new absolute English project path.
2. The chosen language and build system after a minimal spike.
3. The user-authorized copied test slot.
4. The MVP capacity and public default/max proposal.
5. Hard versus soft Improved Inventory dependency.
6. Cursor issue disposition.
7. Final repository name and license before publication.

## 12. Handoff boundary

This handoff contains no mod implementation and makes no claim that item 101 has succeeded. The correct next action is environment initialization and a read-only loader/research probe in a new English-path project—not a release-ready capacity patch and not a modification of the user's existing game or saves.
