"""Fetch public research into ignored work; never execute downloaded code."""
import pathlib
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES = {
    'mgmp.zip': 'https://codeload.github.com/SanTertrust/mgmp/zip/refs/heads/main',
    'inventory.zip': 'https://codeload.github.com/anitosq/mewgenics-mods/zip/5ef2fd860ac0a759c7f0ea378312428f799e5bcf',
    'mewjector.h': 'https://raw.githubusercontent.com/githubuser508/mewjector/main/mewjector.h',
    'mewjector-LICENSE': 'https://raw.githubusercontent.com/githubuser508/mewjector/main/LICENSE',
    'zig-index.json': 'https://ziglang.org/download/index.json',
}
if __name__ == '__main__':
    dest = ROOT / 'work' / 'sources'
    dest.mkdir(parents=True, exist_ok=True)
    for name, url in SOURCES.items():
        if not (dest / name).exists():
            with urllib.request.urlopen(url, timeout=45) as response:
                (dest / name).write_bytes(response.read())
        print(name, (dest / name).stat().st_size)
