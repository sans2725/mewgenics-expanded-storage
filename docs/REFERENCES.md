# Provenance

- Mewjector API 3: https://github.com/githubuser508/mewjector
  Public ABI for diagnostic GetVersion/Log declarations. Runtime implementation
  is original project code; no third-party mod implementation was copied.
- Improved Inventory 0.3.7 reference commit:
  https://github.com/anitosq/mewgenics-mods/tree/5ef2fd860ac0a759c7f0ea378312428f799e5bcf
  Presentation limits/architecture inspected. No redistribution of its DLL/assets.
- mgmp reverse-engineering leads: https://github.com/SanTertrust/mgmp
  Several comments contain older addresses. All RVAs documented as target evidence
  were checked against the local hash-matched executable. No mgmp code vendored.
- Zig 0.15.2 official archive: https://ziglang.org/download/0.15.2/zig-x86_64-windows-0.15.2.zip
  Download hash is pinned in tools/bootstrap_toolchain.py. Compiler stays in ignored
  work/ and is not included in the project's output package.
- Native analysis: capstone 5.0.6 and pefile 2024.8.26, project-local work/python.
  These are optional development dependencies, not DLL runtime dependencies.

The original Bigger Storage Early page and Wiki informed the initial investigation,
but the current 400 clamp and overflow-path conclusions are based on local machine
code. Public anecdotes are not treated as proof of 500+ or no-mod persistence.
