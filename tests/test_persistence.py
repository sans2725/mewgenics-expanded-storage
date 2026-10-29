"""Synthetic SQLite transaction model + real C codec, NEVER a Mewgenics save.

The model establishes the proposed protocol's behavior, not that Mewgenics uses
this schema or preserves arbitrary keys. Only newly created out/tests/*.sqlite
databases are opened. No paths from user/game configuration are accepted.
"""
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest
import uuid

ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT/'out/tests'
TESTS.mkdir(parents=True, exist_ok=True)
U8 = C.c_ubyte
IDENTITY = bytes(range(1, 17))
BUILD = bytes.fromhex('4127cd6a792ae528bca6f65a8873dd61789591937d87656c2b586a5e30eb77ea')

class Vault(C.Structure):
    _fields_ = [('build', U8*32), ('campaign', U8*16), ('revision', C.c_uint64),
                ('capacity', C.c_uint32), ('count', C.c_uint32),
                ('payload', C.POINTER(U8)), ('payload_bytes', C.c_uint32)]

LIB = C.CDLL(str(ROOT/'out/vault_test.dll'))  # shelved vault codec, not shipped
LIB.es_vault_encode.argtypes = [C.POINTER(Vault), C.POINTER(U8), C.c_size_t, C.POINTER(C.c_size_t)]
LIB.es_vault_encode.restype = C.c_int
LIB.es_vault_decode.argtypes = [C.POINTER(U8), C.c_size_t, C.POINTER(U8), C.POINTER(U8), C.POINTER(Vault)]
LIB.es_vault_decode.restype = C.c_int

def encode(items, revision):
    # Deliberately synthetic; do not mistake this JSON for native item encoding.
    payload = json.dumps(items, sort_keys=True, separators=(',', ':')).encode()
    buf = (U8*len(payload)).from_buffer_copy(payload)
    v = Vault((U8*32).from_buffer_copy(BUILD), (U8*16).from_buffer_copy(IDENTITY),
              revision, 600, len(items), buf, len(payload))
    out = (U8*(96+len(payload)))();written = C.c_size_t()
    result = LIB.es_vault_encode(C.byref(v), out, len(out), C.byref(written))
    if result: raise ValueError(f'encode failed: {result}')
    return bytes(out[:written.value])

def decode(blob):
    data = (U8*len(blob)).from_buffer_copy(blob);v = Vault()
    result = LIB.es_vault_decode(data, len(data), (U8*32).from_buffer_copy(BUILD),
                                 (U8*16).from_buffer_copy(IDENTITY), C.byref(v))
    if result: raise ValueError(f'decode failed: {result}')
    return json.loads(C.string_at(v.payload, v.payload_bytes)), v.revision

def connect(name):
    # Only generated fixture basenames under the workspace are accepted.
    if Path(name).name != name or not name.startswith('fixture-') or not name.endswith('.sqlite'):
        raise ValueError('Not a synthetic fixture name')
    path = TESTS/name
    if path.is_symlink() or path.resolve().parent != TESTS.resolve():
        raise ValueError('Fixture escaped the test directory')
    return sqlite3.connect(path, isolation_level=None)

def state(db):
    rows = dict(db.execute('SELECT key,value FROM fixture_properties'))
    normal = json.loads(rows['normal'])
    vault, revision = decode(rows['es_vault_v1'])
    return normal, vault, revision

def transfer(db, direction, index=0, expected_revision=0, crash=None):
    db.execute('BEGIN IMMEDIATE')
    try:
        normal, vault, revision = state(db)
        if revision != expected_revision: raise ValueError('stale revision')
        source, dest = (normal, vault) if direction == 'deposit' else (vault, normal)
        if direction not in ('deposit', 'withdraw'): raise ValueError('bad direction')
        # Simulated current native capacity is 10, not an imposed mod capacity.
        if len(dest) >= (600 if direction == 'deposit' else 10): raise ValueError('full')
        dest.append(source.pop(index))
        db.execute('UPDATE fixture_properties SET value=? WHERE key=?', (json.dumps(normal), 'normal'))
        if crash == 'after_normal': os._exit(91)
        db.execute('UPDATE fixture_properties SET value=? WHERE key=?', (encode(vault, revision+1), 'es_vault_v1'))
        if crash == 'after_vault': os._exit(92)
        db.execute('COMMIT')
        if crash == 'after_commit': os._exit(93)
    except BaseException:
        if db.in_transaction: db.execute('ROLLBACK')
        raise

class PersistenceTests(unittest.TestCase):
    def fixture(self):
        name = f'fixture-{uuid.uuid4().hex}.sqlite';db = connect(name)
        self.addCleanup(db.close)
        db.execute('PRAGMA journal_mode=WAL')
        db.execute('PRAGMA synchronous=FULL')
        db.execute('CREATE TABLE fixture_properties (key TEXT PRIMARY KEY, value BLOB NOT NULL)')
        # No real game fields are assumed; distinctive opaque fixture data checks
        # identity and properties across all 600 records.
        items = [{'fixture_id': i, 'opaque': hashlib.sha256(str(i).encode()).hexdigest(),
                  'fixture_label': '物品-'+str(i), 'fixture_values': [i%11, -1, None, True]} for i in range(600)]
        db.execute('INSERT INTO fixture_properties VALUES (?,?)', ('normal', json.dumps(items[:10])))
        db.execute('INSERT INTO fixture_properties VALUES (?,?)', ('es_vault_v1', encode(items[10:], 0)))
        return name, db, items

    def assert_items(self, db, expected):
        normal, vault, _ = state(db)
        actual = sorted(normal+vault, key=lambda x: x['fixture_id'])
        self.assertEqual(actual, expected)

    def test_600_records_round_trip_and_repeat_rejected(self):
        _, db, items = self.fixture()
        transfer(db, 'deposit');self.assert_items(db, items)
        with self.assertRaisesRegex(ValueError, 'stale'): transfer(db, 'deposit')
        self.assert_items(db, items)
        transfer(db, 'withdraw', index=590, expected_revision=1)
        self.assert_items(db, items)
        self.assertEqual(state(db)[2], 2)

    def test_process_crash_atomicity(self):
        for point, committed in [('after_normal',False), ('after_vault',False), ('after_commit',True)]:
            with self.subTest(point=point):
                name, db, items = self.fixture()
                child = subprocess.run([sys.executable, __file__, '--crash-fixture', name, point],
                                       cwd=ROOT, capture_output=True, text=True)
                self.assertEqual(child.returncode, {'after_normal':91,'after_vault':92,'after_commit':93}[point], child.stderr)
                normal, vault, revision = state(db)
                self.assertEqual((len(normal),len(vault),revision), (9,591,1) if committed else (10,590,0))
                self.assert_items(db, items)

    def test_absent_mod_does_not_restore_used_items(self):
        _, db, items = self.fixture();transfer(db, 'deposit')
        before = db.execute("SELECT value FROM fixture_properties WHERE key='es_vault_v1'").fetchone()[0]
        normal, vault, _ = state(db);consumed = normal.pop(0)
        # A synthetic 'vanilla session' updates only its own key.
        db.execute("UPDATE fixture_properties SET value=? WHERE key='normal'", (json.dumps(normal),))
        after = db.execute("SELECT value FROM fixture_properties WHERE key='es_vault_v1'").fetchone()[0]
        self.assertEqual(before, after)
        self.assert_items(db, [item for item in items if item != consumed])
        self.assertEqual(len(vault),591)

    def test_full_normal_storage_refuses_withdrawal(self):
        _, db, items = self.fixture()
        with self.assertRaisesRegex(ValueError,'full'): transfer(db, 'withdraw')
        self.assert_items(db, items);self.assertEqual(state(db)[2],0)

    def test_corrupt_archive_preserved_and_transfer_refused(self):
        _, db, _ = self.fixture()
        blob = bytearray(db.execute("SELECT value FROM fixture_properties WHERE key='es_vault_v1'").fetchone()[0]);blob[-1]^=1
        db.execute("UPDATE fixture_properties SET value=? WHERE key='es_vault_v1'",(blob,))
        normal = db.execute("SELECT value FROM fixture_properties WHERE key='normal'").fetchone()[0]
        with self.assertRaises(ValueError): transfer(db, 'deposit')
        self.assertEqual(db.execute("SELECT value FROM fixture_properties WHERE key='normal'").fetchone()[0],normal)
        self.assertEqual(db.execute("SELECT value FROM fixture_properties WHERE key='es_vault_v1'").fetchone()[0],blob)

    def test_wrong_executable_and_unreadable_refused(self):
        exe = ROOT/'out/es_preflight.exe'
        self.assertEqual(subprocess.run([exe,'--game',ROOT/'README.md'],capture_output=True).returncode,1)
        self.assertEqual(subprocess.run([exe,'--game',TESTS/'missing-executable'],capture_output=True).returncode,2)

if __name__ == '__main__':
    if len(sys.argv)==4 and sys.argv[1]=='--crash-fixture':
        db=connect(sys.argv[2]);transfer(db,'deposit',crash=sys.argv[3]);raise SystemExit('Crash point not reached')
    unittest.main(verbosity=2)
