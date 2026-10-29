# Project rules

- Read docs/STATUS.md and all docs/decisions/*.md before continuing. Decision 0004
  (2026-10-29) is the current plan: a runtime patch of the three native 400 clamps
  plus persisting Storage upgrades = 17. The vault (0003) is shelved, not shipped.
- Do not access the old workspace. Handoff originals are in the parent directory.
- Authorization (2026-10-29): build, deploy to Mewtator and live-test on save
  slot 2 only, after a verified backup of all three slots. Slots 1 and 3 are the
  owner's real campaigns: never test in them and never restore over them without
  being asked. The owner launches the game and plays; read logs, not saves.
- Game target: Steam build 25143593, SHA256
  4127CD6A792AE528BCA6F65A8873DD61789591937D87656C2B586A5E30EB77EA.
- Any check failure must leave the game unmodified (fail closed).
- Preserve NPC progression; do not infer semantics from field names alone.
- Keep proprietary binaries, saves, analysis dumps and downloaded references in
  ignored work/ only, and never copy the game executable into this repository.
- Record evidence, design changes, test results and next steps in docs/STATUS.md.
- Published under MIT at https://github.com/sans2725/mewgenics-expanded-storage (owner sans2725). Ask before pushing
  new releases or changing the license.
