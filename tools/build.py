"""Build, test and package Expanded Storage. Never deploys or opens a player save."""
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
ZIG = ROOT / 'work/toolchains/zig-x86_64-windows-0.15.2/zig.exe'
OUT = ROOT / 'out'
VERSION = '0.2.0'
DIST = ROOT / 'dist'
PACKAGE = ROOT / 'packaging/ExpandedStorage'

def run(args, env):
    subprocess.run([str(x) for x in args], cwd=ROOT, env=env, check=True)

def main():
    if not ZIG.is_file():
        raise SystemExit('Run python tools/bootstrap_toolchain.py first.')
    if subprocess.check_output([str(ZIG), 'version'], text=True).strip() != '0.15.2':
        raise SystemExit('Unexpected compiler version')
    OUT.mkdir(exist_ok=True)
    temp = ROOT/'work/compiler-temp'
    temp.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT/'work/zig-cache'),
               ZIG_LOCAL_CACHE_DIR=str(ROOT/'work/zig-local-cache'), TEMP=str(temp), TMP=str(temp))
    common = [ZIG, 'cc', '-target', 'x86_64-windows-gnu', '-std=c11',
              '-Wall', '-Wextra', '-Werror', '-O2', '-Isrc']
    run(common + ['-shared', 'src/capacity.c', 'src/plugin.c', '-luser32', '-o', OUT/'ExpandedStorage.dll'], env)
    run(common + ['src/preflight.c', 'src/preflight_main.c', '-lbcrypt', '-municode',
                  '-o', OUT/'es_preflight.exe'], env)
    run(common + ['-fsanitize=undefined', '-fno-sanitize-recover=all',
                  'src/capacity.c', 'tests/test_capacity.c', '-o', OUT/'test_capacity.exe'], env)
    run([OUT/'test_capacity.exe'], env)
    run(common + ['-fsanitize=undefined', '-fno-sanitize-recover=all',
                  'src/vault.c', 'tests/test_vault.c', '-o', OUT/'test_vault.exe'], env)
    run([OUT/'test_vault.exe'], env)
    run(common + ['-shared', 'src/vault.c', '-o', OUT/'vault_test.dll'], env)
    run([sys.executable, 'tests/test_persistence.py'], env)
    sources = sorted(p for folder in ('src', 'tests', 'tools') for p in (ROOT/folder).glob('*') if p.is_file())
    manifest = {
        'version': VERSION, 'compiler': 'zig 0.15.2', 'target': 'x86_64-windows-gnu',
        'default_capacity': 1000, 'max_capacity': 1000, 'persisted_upgrades': 17,
        'game_build': '25143593',
        'game_sha256': '4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea',
        'artifacts': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in (OUT/'ExpandedStorage.dll', OUT/'es_preflight.exe')},
        'sources': {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
    }
    (OUT/'build-info.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    package(manifest)
    print(f'Built, tested and packaged Expanded Storage {VERSION} into dist/.')

def package(manifest):
    target = DIST / 'ExpandedStorage'
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(PACKAGE, target)
    shutil.copy2(OUT/'ExpandedStorage.dll', target)
    if (ROOT/'LICENSE').is_file():
        shutil.copy2(ROOT/'LICENSE', target)
    info = {k: manifest[k] for k in ('version', 'compiler', 'game_build', 'game_sha256', 'artifacts')}
    (target/'build-info.json').write_text(json.dumps(info, indent=2)+'\n', encoding='utf-8')
    archive = DIST / f'ExpandedStorage-{VERSION}.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for path in sorted(target.rglob('*')):
            z.write(path, path.relative_to(DIST).as_posix())

if __name__ == '__main__':
    main()
