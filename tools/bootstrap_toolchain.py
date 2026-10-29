"""Install the pinned compiler within ignored work/, without changing PATH."""
import hashlib
import pathlib
import urllib.request
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
VERSION = '0.15.2'
NAME = f'zig-x86_64-windows-{VERSION}'
URL = f'https://ziglang.org/download/{VERSION}/{NAME}.zip'
SHA256 = '3a0ed1e8799a2f8ce2a6e6290a9ff22e6906f8227865911fb7ddedc3cc14cb0c'

if __name__ == '__main__':
    dest = ROOT / 'work' / 'toolchains'
    dest.mkdir(parents=True, exist_ok=True)
    archive = dest / (NAME + '.zip')
    if not archive.exists():
        partial = archive.with_suffix('.partial')
        urllib.request.urlretrieve(URL, partial)
        if hashlib.sha256(partial.read_bytes()).hexdigest() != SHA256:
            raise SystemExit('Compiler archive checksum mismatch; not extracted')
        partial.replace(archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise SystemExit('Compiler archive checksum mismatch')
    with zipfile.ZipFile(archive) as z:
        for member in z.infolist():
            target = (dest / member.filename).resolve()
            if not target.is_relative_to(dest.resolve()):
                raise SystemExit('Unsafe archive path')
        z.extractall(dest)
    print(dest / NAME / 'zig.exe')
