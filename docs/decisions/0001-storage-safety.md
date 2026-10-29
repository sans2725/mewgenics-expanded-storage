# Storage safety and first candidate

Date: 2026-10-23 (Asia/Shanghai)

## Accepted intent

Build an independently distributable companion for Improved Inventory 0.3.7.
Target more than 500 slots; 600 is a proposed first verification target, not a
proven maximum. Preserve normal NPC rewards and reduced-donation mods.

100 is the end of regular storage upgrades, NOT a vanilla hard limit. Native
endless rewards already add slots. The handoff files are historical originals;
their hard-limit assumption is superseded by this decision.

## Preferred mechanism, conditional on evidence

Use persistent native storage state which vanilla reads without this DLL and
which NPC rewards cannot reset. Avoid advancing NPC favor or reward stages.
Do not use an external save editor as the player workflow. Persist via normal
game save mechanisms. Do not claim no-mod safety until tested without both mods,
including donations, adventure return, ending a day and saving again.

The simpler runtime-only capacity override is insufficient for the owner's
absence-safety goal. If no independent native state exists, document the conflict
instead of silently using that fallback.

## Scope and authorization

The owner authorized local construction. All three slots are occupied. They will
clear/create one later. Build only: no launch, deployment or save mutation now.
A candidate must default to inactive and require an explicit campaign scope.
Loading before the title screen must not mutate all campaigns.

## Toolchain

No compiler was detected in PATH. Prefer a project-local pinned Zig 0.15.2 C
toolchain for the initial narrow Windows x64 DLL (also used by the UI companion),
avoiding global installation. This is an implementation choice; no prior language
was finalized. Keep native interfaces C-compatible. Analysis uses project-local
capstone 5.0.6 and pefile 2024.8.26; neither is a runtime dependency.
