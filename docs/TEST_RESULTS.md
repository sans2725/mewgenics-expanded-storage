# Development foundation results

Date: 2026-10-23
Version: 0.1.0-dev.1

Build: Windows x64, Zig 0.15.2, C11, -O2, -Wall -Wextra -Werror.
Portable C test binary additionally used -fsanitize=undefined and
-fno-sanitize-recover=all. No warnings or sanitizer failures in the successful run.

## Passed

- C codec: 600-record synthetic binary payload; all 19,296 single-byte corruption
  cases and all 19,296 truncated lengths rejected; identity, length, capacity,
  buffer overlap and transaction guards passed.
- Python persistence suite: 6 test methods passed. Covers 600 distinct synthetic
  records, duplicate/stale actions, full normal storage, corrupted archive,
  simulated absent-mod item consumption, and executable refusal paths.
- Crash tests terminated child processes after the normal-inventory write, after
  the vault write, and after commit. The first two recovered the old pair of
  records; the last recovered the new pair. Total item identities/properties were
  preserved in the synthetic database.
- Preflight CLI read the installed Mewgenics.exe and matched the exact target hash.
- DLL PE machine is 0x8664; nine expected core/diagnostic exports present.
  Imports are BCrypt, Kernel32 and Windows Universal CRT API sets.

## Not established by these results

No Mewgenics launch, mod installation, save editing or live item transfers were
performed. The test database is explicitly synthetic, not the game's schema.
Its JSON fixtures are not the native item format. These tests do not establish
native item fidelity, campaign isolation during game transitions, unknown-key
retention, Improved Inventory compatibility, runtime performance or uninstall
safety. The DLL has no game mutation path and is not a playable mod.

## Build failures resolved

An unsupported GNU linker timestamp option was removed. Sandbox compilation then
hit temporary/cache write failures; approved execution outside the sandbox built
successfully using project-local compiler/cache/temp paths. No global toolchain
installation or game-directory change was made.
