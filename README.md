# Expanded Storage for Mewgenics / 喵喵仓库扩容

[English](#english) | [中文](#中文)

---

## English

**Keep every rare drop.** Expanded Storage raises your home Storage from the
vanilla cap to **1000 slots** (configurable from 400 to 1000), without touching
Butch's quests, donations or rewards.

> Pairs best with **[Improved Inventory](https://github.com/anitosq/mewgenics-mods/tree/main/mods/improved-inventory)**
> by anitosq ([Nexus](https://www.nexusmods.com/mewgenics/mods/526)): scrolling
> grids, search and filters keep a 1000-item Storage readable. Strongly recommended.

### Features

- Storage holds up to 1000 items (default 1000, set your own in `ExpandedStorage.ini`).
- Works on existing saves and new games. Choose which save slots are expanded.
- Butch's progression is untouched. Mods that lower donation requirements still work.
- Your save keeps **400 slots even if you launch the game without this mod**
  (see [How it works](#how-it-works)).
- Exact game-version check: on any other build it stays off and shows a warning.
- Tiny native DLL, no save editor, no file replacement.

### Requirements

| | |
|---|---|
| Game | Mewgenics 1.1.21239, **Steam build 25143593**, Windows x64 |
| Loader | [Mewjector](https://github.com/githubuser508/mewjector) (API v3) |
| Manager | Mewtator (recommended) |
| Recommended | [Improved Inventory](https://www.nexusmods.com/mewgenics/mods/526) 0.3.7 |

### Install

1. Download `ExpandedStorage-<version>.zip` from [Releases](../../releases).
2. In Mewtator, import the ZIP and enable **Expanded Storage** (and Improved Inventory).
3. Launch the game through Mewtator.
4. Optional: `ExpandedStorage.log` in the mod folder should say
   `Patched 3 capacity sites and the upgrade reward`.

### Settings (`ExpandedStorage.ini`)

```ini
Capacity=1000   ; 400-1000. Restart the game after changing it.
Slots=All       ; All, or a list such as 2 or 1,3
```

### How it works

The game computes Storage capacity as `min((upgrades + 3)² + extra, 400)` in
three places. This mod replaces that limit with `min(native + bonus, 1000)` for
enabled save slots and is byte-for-byte vanilla otherwise.

The first time an enabled save computes its capacity, its Storage upgrade level
is raised to **17**, the game's own value for a 20×20 = 400-slot Storage. This is
written into your save by the game's normal saving, so the save keeps 400 slots
without the mod. Butch's regular storage reward stops adding levels at 17, so the
grid never exceeds 20 columns (Improved Inventory's maximum). Repeat rewards keep
working. Nothing else in your save is changed.

### ⚠️ Read before playing without the mod

The game itself only supports 400 slots. Whenever the mod is **not active**
(disabled, launched without Mewtator, after a **game update**, or a slot removed
from `Slots`), Storage falls back to 400 slots, and **items beyond 400 are moved
to the Trash** when you enter your home. The same happens if you lower `Capacity`
below the number of items you hold.

- After a game update the mod shows a warning dialog and stays off until it is
  updated. With more than 400 items, quit before entering your home.
- To uninstall: bring Storage down to 400 items or fewer first.
- Consider pausing Steam auto-updates for Mewgenics while you rely on > 400 slots.

### Compatibility

- **Improved Inventory 0.3.7**: tested together. Its 1024-item limit counts
  Storage **and** Trash together; with a nearly full 1000-slot Storage, keep the
  Trash small or Improved Inventory falls back to the vanilla layout.
- **Donation/upgrade-cost mods** (e.g. Bigger Storage Early): designed to be compatible; they change when rewards arrive, this mod changes capacity.
- **Other mods that change the same Storage limit**: Expanded Storage detects the
  changed code and stays off rather than conflicting.
- Without Improved Inventory, the vanilla screen shows a fixed 20×20 grid and
  cannot scroll, so a Storage above 400 items is impractical to browse.

### FAQ

**Will it reset my progress?** No. Only the Storage upgrade level is raised, to 17.
Butch's favor, quotas and quests are separate data.

**Can I go above 1000?** Not in this version: Improved Inventory lays out at most
1024 items, and Storage is unusable without it at that size.

**Does it work on GOG/other builds?** Only Steam build 25143593. Otherwise it stays off.

### Building from source

Requires Windows, Python 3.13 and network access once for the pinned compiler.

```powershell
python tools/bootstrap_toolchain.py
python tools/build.py
```

The build runs all tests and writes `dist/ExpandedStorage/` and
`dist/ExpandedStorage-<version>.zip`. Technical design and evidence:
[`docs/decisions/0004-capacity-patch.md`](docs/decisions/0004-capacity-patch.md).

### Credits

- **anitosq**, author of [Improved Inventory](https://github.com/anitosq/mewgenics-mods/tree/main/mods/improved-inventory)
  (MIT). Their confirmation that the limit can be removed inspired this mod.
  Improved Inventory is recommended and linked, not bundled; none of its code is
  included here. Please download it from the author's pages and support their work.
- [Mewjector](https://github.com/githubuser508/mewjector) (MIT, Mewjector
  Contributors): DLL loader. Only its public API is declared here.
- [mgmp](https://github.com/SanTertrust/mgmp) (MIT, SanTertrust): reverse-engineering
  notes used as research leads; addresses were re-verified, no code copied.

### License

[MIT](LICENSE). Mewgenics and its assets belong to their owners; this repository
contains no game files.

---

## 中文

**稀有掉落再也不用扔。** 仓库扩容把家里的仓库上限从原版提升到 **1000 格**（可在 400–1000
之间自定义），不影响 Butch 的任务、捐猫进度和奖励。

> 强烈推荐搭配 anitosq 的 **[Improved Inventory（库存改进）](https://github.com/anitosq/mewgenics-mods/tree/main/mods/improved-inventory)**
> （[Nexus](https://www.nexusmods.com/mewgenics/mods/526)）：滚动网格、搜索和筛选，让 1000
> 件物品的仓库依然清晰好用。

### 功能

- 仓库最多可放 1000 件物品（默认 1000，可在 `ExpandedStorage.ini` 中修改）。
- 支持已有存档和新存档，可以指定哪些存档槽位扩容。
- 不改动 Butch 的进度；降低捐猫要求的模组照常生效。
- **即使某次没有加载本模组，存档也保留 400 格**（见[工作原理](#工作原理)）。
- 严格校验游戏版本：遇到其他版本时自动停用并弹窗提醒。
- 体积很小的原生 DLL，不用存档编辑器，不替换游戏文件。

### 前置要求

| | |
|---|---|
| 游戏 | Mewgenics 1.1.21239，**Steam 版本号 25143593**，Windows x64 |
| 加载器 | [Mewjector](https://github.com/githubuser508/mewjector)（API v3） |
| 模组管理器 | Mewtator（推荐） |
| 推荐搭配 | [Improved Inventory](https://www.nexusmods.com/mewgenics/mods/526) 0.3.7 |

### 安装

1. 从 [Releases](../../releases) 下载 `ExpandedStorage-<版本>.zip`。
2. 在 Mewtator 中导入 ZIP，启用 **Expanded Storage**（以及 Improved Inventory）。
3. 通过 Mewtator 启动游戏。
4. 可选：模组文件夹里的 `ExpandedStorage.log` 应显示
   `Patched 3 capacity sites and the upgrade reward`。

### 设置（`ExpandedStorage.ini`）

```ini
Capacity=1000   ; 400-1000，修改后需重启游戏
Slots=All       ; All 表示全部槽位，也可以写 2 或 1,3
```

### 工作原理

游戏在三处用 `min((升级等级 + 3)² + 额外格数, 400)` 计算仓库容量。本模组在启用的存档槽位里把它
替换为 `min(原生容量 + 加成, 1000)`，未启用的槽位与原版逐字节一致。

启用的存档第一次计算容量时，仓库升级等级会被提升到 **17**，这是游戏自己表示 20×20 = 400 格的
数值。它通过游戏正常存档写入，所以即使没有模组，存档也保留 400 格。Butch 的常规扩容奖励在 17
级后不再增加等级，保证网格不超过 20 列（Improved Inventory 的上限）；重复奖励照常有效。存档的
其他内容不受影响。

### ⚠️ 不加载模组游玩前必读

游戏本身最多只支持 400 格。只要本模组**没有生效**（被禁用、不通过 Mewtator 启动、**游戏更新**
后、或把某个槽位从 `Slots` 中去掉），仓库就会回到 400 格，进入家园时**超过 400 件的物品会被移入
垃圾桶**。把 `Capacity` 调低到少于现有物品数时也一样。

- 游戏更新后，模组会弹窗提示并保持停用，直到模组更新。仓库超过 400 件时请在进入家园前退出。
- 卸载前请先把仓库降到 400 件以内。
- 如果依赖 400 格以上，建议暂停 Mewgenics 的 Steam 自动更新。

### 兼容性

- **Improved Inventory 0.3.7**：已一起实测。它的 1024 件上限是仓库**加**垃圾桶合计；仓库接近
  1000 件时请保持垃圾桶较空，否则它会退回原版布局。
- **捐猫/升级成本类模组**（如 Bigger Storage Early）：设计上兼容；它们改变奖励何时到来，本模组改变容量。
- **其他修改同一仓库上限的模组**：本模组会检测到代码已被改动并自动停用，不会冲突。
- 不装 Improved Inventory 时，原版界面是固定的 20×20 网格且不能滚动，超过 400 件时难以浏览。

### 常见问题

**会重置进度吗？** 不会。只把仓库升级等级提升到 17；Butch 的好感、捐猫要求和任务是独立数据。

**能超过 1000 吗？** 当前版本不行：Improved Inventory 最多排布 1024 件，没有它时这么大的仓库无法使用。

**支持 GOG 或其他版本吗？** 只支持 Steam 版本号 25143593，其他版本会自动停用。

### 从源码构建

需要 Windows、Python 3.13，首次构建需联网下载固定版本的编译器。

```powershell
python tools/bootstrap_toolchain.py
python tools/build.py
```

构建会运行全部测试，并生成 `dist/ExpandedStorage/` 和 `dist/ExpandedStorage-<版本>.zip`。技术设计与
证据见 [`docs/decisions/0004-capacity-patch.md`](docs/decisions/0004-capacity-patch.md)。

### 致谢

- **anitosq**，[Improved Inventory](https://github.com/anitosq/mewgenics-mods/tree/main/mods/improved-inventory)
  的作者（MIT 许可）。作者确认仓库上限可以去除，启发了本模组。本模组只推荐并链接 Improved Inventory，
  不打包其文件，也不包含其任何代码。请到作者的页面下载并支持他的作品。
- [Mewjector](https://github.com/githubuser508/mewjector)（MIT，Mewjector Contributors）：DLL 加载器，
  本项目仅声明其公开 API。
- [mgmp](https://github.com/SanTertrust/mgmp)（MIT，SanTertrust）：逆向研究线索，所有地址均已重新验证，
  未复制代码。

### 许可证

[MIT](LICENSE)。Mewgenics 及其素材归原权利人所有；本仓库不包含任何游戏文件。
