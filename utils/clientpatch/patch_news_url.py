"""
Config-driven patcher for the EVE Crucible client's compiled.code.

Rewrites the station holoscreen news RSS URLs (holoscreenSvc.RSS_FEEDS) to our
own feed, so the in-game news ticker works without a hosts file and survives
server moves (the embedded value is a DOMAIN+PORT; DNS and NAT are the config
layers - see patch_config.json).

Usage:
  python patch_news_url.py            # reads patch_config.json, patches in place
  python patch_news_url.py --verify   # only verify the current file (no writes)

The script:
  1. extracts the 3DES key from blue.dll (exponent-of-one RSA trick, CryptoAPI)
  2. decrypts the holoscreenSvc entry of compiled.code (3DES + zlib)
  3. splices the new URL strings into the marshal stream (self-describing:
     rewrites the length prefix + bytes, so the stream stays valid)
  4. re-encrypts (zlib + 3DES) and repacks the outer pickle (protocol 2 =
     python 2.7 compatible, same as the original)
  5. verifies the result by decrypting it again
  6. writes compiled.code.patched next to the original (never touches the original)
"""
import ctypes
import ctypes.wintypes
import marshal  # noqa: F401  (kept for parity with evedec tooling)
import zlib
import pickle
import os
import sys
import json
import struct

sys.path.insert(0, r'C:\opencode-projects\evemu\doc')
import evedec_crucible as dec

HERE = os.path.dirname(os.path.abspath(__file__))
CONFIG_PATH = os.path.join(HERE, 'patch_config.json')
BLUE_PATH = r'C:\opencode-projects\other\blue.dll'
CODE_PATH = os.environ.get('PATCH_CODE_PATH', r'C:\opencode-projects\other\compiled.code')
TARGET_ENTRY = 'script:/ui/services/holoscreenSvc.py'
# station billboards: news headlines + ad texture (billboardMgr)
TARGET_ENTRY2 = 'script:/parklife/billboardMgr.py'
# billboard "Read News" menu + in-game browser home page
TARGET_ENTRY3 = 'script:/ui/control/browser/browserutil.py'
TARGET_ENTRY4 = 'script:/ui/services/menusvc.py'
TARGETS = (TARGET_ENTRY2,)

DEFAULT_CONFIG = {
    "news_url_base": "http://router.iks-online.net:26006",
    "old_url_base": "http://www.eveonline.com"
}

advapi32 = dec.advapi32
kernel32 = dec.kernel32
HCRYPTKEY = dec.HCRYPTKEY
HCRYPTHASH = dec.HCRYPTHASH


def load_config():
    if os.path.isfile(CONFIG_PATH):
        with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
            cfg = json.load(f)
    else:
        cfg = dict(DEFAULT_CONFIG)
        with open(CONFIG_PATH, 'w', encoding='utf-8') as f:
            json.dump(cfg, f, indent=2)
        print('config written:', CONFIG_PATH)
    for k in DEFAULT_CONFIG:
        cfg.setdefault(k, DEFAULT_CONFIG[k])
    return cfg


def crypt_encrypt(hKey, data):
    """3DES encrypt with PKCS#5 padding (Final=TRUE) - mirror of decrypt_entry"""
    buf = ctypes.create_string_buffer(data, len(data) + 8)  # room for padding
    dwSize = ctypes.c_uint32(len(data))
    if not advapi32.CryptEncrypt(hKey, HCRYPTHASH(0), True, 0, buf, ctypes.byref(dwSize)):
        err = kernel32.GetLastError()
        raise Exception('CryptEncrypt failed, error=%s' % err)
    return buf.raw[:dwSize.value]


advapi32.CryptEncrypt = getattr(advapi32, 'CryptEncrypt', None)
if advapi32.CryptEncrypt is not None:
    advapi32.CryptEncrypt.argtypes = [HCRYPTKEY, HCRYPTHASH, ctypes.c_bool, ctypes.c_uint32,
                                      ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    advapi32.CryptEncrypt.restype = ctypes.c_bool


def find_marshal_string(blob, needle):
    """Locate a marshal string object CONTAINING `needle`.

    Python 2.7 marshal string types ('t' interned, 's' string, 'u' unicode)
    use a 4-BYTE little-endian length prefix (the 1-byte variant is py3.4+).
    The needle may be a substring of the actual string, so we scan backwards
    for a valid [type][len4] header whose length covers the needle.
    Returns (start, end, old_str_bytes) covering [type][len4][bytes].
    """
    pos = 0
    while True:
        idx = blob.find(needle, pos)
        if idx < 0:
            return None
        # scan backwards for the marshal header (the string may be longer than needle)
        maxback = min(idx, 300)
        for start in range(idx - 5, idx - maxback, -1):
            tbyte = blob[start]
            if tbyte in (ord('t'), ord('s'), ord('u')):
                slen = int.from_bytes(blob[start + 1:start + 5], 'little')
                str_start = start + 5
                str_end = str_start + slen
                if str_start <= idx and idx + len(needle) <= str_end and str_end <= len(blob):
                    return (start, str_end, blob[str_start:str_end])
        pos = idx + 1


def splice_string(blob, loc, new_str):
    """Replace the marshal string at `loc` with new_str (updates the 4-byte length)."""
    start, end, _old = loc
    tbyte = blob[start]
    newlen = len(new_str)
    if newlen > 0x7FFFFFFF:
        raise Exception('new string too long')
    return blob[:start] + bytes([tbyte]) + newlen.to_bytes(4, 'little') + new_str + blob[end:]


def main():
    verify_only = '--verify' in sys.argv
    cfg = load_config()
    base = cfg['news_url_base'].rstrip('/')
    old_base = cfg['old_url_base'].rstrip('/')

    # Final decision: start from the UNPATCHED original client and change exactly
    # ONE thing - the billboard news line URL. The ad image, the click links and
    # the RSS feeds all stay at their original CCP values (the images/baked ad
    # animation is what the client had before we ever touched it).
    replacements = [
        (old_base + '/mb/news-headlines.asp', base + '/mb/news-headlines.asp'),
    ]

    print('=== crypto init ===')
    blue_data = dec.read_blue_dll(BLUE_PATH)
    rsa_pub, tdes_blob = dec.extract_keys(blue_data)
    hProv = dec.HCRYPTPROV()
    if not advapi32.CryptAcquireContextA(ctypes.byref(hProv), None, None,
                                         dec.PROV_RSA_FULL, dec.CRYPT_VERIFYCONTEXT):
        raise Exception('CryptAcquireContext failed')
    hOneKey = dec.create_exponent_one_key(hProv)
    hTdesKey = dec.import_crypt_blob(hProv, hOneKey, tdes_blob)

    print('=== load compiled.code ===')
    with open(CODE_PATH, 'rb') as f:
        data = f.read()
    magic, all_pickled, signature = pickle.loads(data, encoding='bytes')
    all_dict = pickle.loads(all_pickled, encoding='bytes')
    codes = all_dict[b'code'] if b'code' in all_dict else all_dict['code']

    print('=== patching %s + %s ===' % (TARGET_ENTRY, TARGET_ENTRY2))
    new_codes = []
    patched = False
    target_orig_encrypted = {}
    for entry in codes:
        (filename, type_val), (encrypted_code, hash_val) = entry
        fname = filename.decode('utf-8', errors='replace') if isinstance(filename, bytes) else filename
        if fname not in TARGETS:
            new_codes.append(entry)
            continue
        target_orig_encrypted[fname] = encrypted_code

        raw = dec.decrypt_entry(hProv, hTdesKey, encrypted_code)
        print('decrypted: %d bytes' % len(raw))

        if verify_only:
            for old, new in replacements:
                present = old.encode('utf-8') in raw
                print(('  HAS   ' if present else '  none  ') + old)
            continue

        blob = raw
        for old, new in replacements:
            oldb = old.encode('utf-8')
            newb = new.encode('utf-8')
            while True:
                loc = find_marshal_string(blob, oldb)
                if loc is None:
                    break
                blob = splice_string(blob, loc, newb)
                print('patched [%s]: %s -> %s' % (fname.split('/')[-1], old, new))

        if blob == raw:
            print('WARNING: nothing changed in %s (already patched?)' % fname)
        else:
            patched = True
        new_encrypted = crypt_encrypt(hTdesKey, zlib.compress(blob))
        new_codes.append(((filename, type_val), (new_encrypted, hash_val)))
        patched = True

    if verify_only:
        return
    if not patched:
        raise Exception('target entry not found in compiled.code')

    # verify round-trip before writing anything
    print('=== verify round-trip ===')
    ok = False
    for (filename, type_val), (encrypted_code, hash_val) in new_codes:
        fname = filename.decode('utf-8', errors='replace') if isinstance(filename, bytes) else filename
        if fname not in TARGETS:
            continue
        raw2 = dec.decrypt_entry(hProv, hTdesKey, encrypted_code)
        assert base.encode('utf-8') in raw2, 'patched URL not found after round-trip (%s)' % fname
        assert old_base.encode('utf-8') + b'/feed' not in raw2, 'old feed URL still present (%s)' % fname
        assert old_base.encode('utf-8') + b'/mb/' not in raw2, 'old headlines URL still present (%s)' % fname
        ok = True
    if not ok:
        raise Exception('verification failed')

    if isinstance(all_dict, dict) and b'code' in all_dict:
        all_dict[b'code'] = new_codes
    else:
        all_dict['code'] = new_codes

    # NOTE: re-pickling the whole container with python3 would INFLATE every
    # binary blob ~1.6x (proto-2 pickles bytes as a latin1-str rendered in
    # UTF-8: each byte >= 0x80 becomes two bytes). Instead patch BOTH pickle
    # streams surgically - the original is protocol 2 where BINSTRING ('T')
    # stores raw bytes, and pickle streams are sequential (no absolute
    # offsets; 'q' BINPUT memo refs are object ids, not positions).
    enc0_map = target_orig_encrypted   # fname -> ORIGINAL encrypted blob
    if not enc0_map:
        raise Exception('target entries were not processed')
    new_ap = None
    if isinstance(all_pickled, (bytes, bytearray)):
        apb = bytes(all_pickled)
        for fname, enc0 in enc0_map.items():
            marker = apb.find(enc0)
            if marker < 0:
                raise Exception('original encrypted blob not found inside the inner pickle (%s)' % fname)
            rec_start = marker - 5        # 'T' + 4-byte length
            if apb[rec_start] != 0x54:    # 'T'
                raise Exception('unexpected opcode before blob: 0x%02X' % apb[rec_start])
            old_len = int.from_bytes(apb[rec_start + 1:rec_start + 5], 'little')
            if old_len != len(enc0):
                raise Exception('inner blob length mismatch (%s): %d != %d' % (fname, old_len, len(enc0)))
            # find the matching NEW encrypted blob for this entry
            new_enc = None
            for (filename, type_val), (encrypted_code, hash_val) in new_codes:
                fname2 = filename.decode('utf-8', errors='replace') if isinstance(filename, bytes) else filename
                if fname2 == fname:
                    new_enc = encrypted_code
                    break
            if new_enc is None:
                raise Exception('no new blob for %s' % fname)
            apb = (apb[:rec_start]
                   + b'T' + len(new_enc).to_bytes(4, 'little') + new_enc
                   + apb[marker + len(enc0):])
        new_ap = apb

    if new_ap is not None:
        # outer: replace the whole inner-pickle string record the same way
        apos = data.find(all_pickled)
        if apos < 0:
            raise Exception('inner pickle not found inside the outer pickle')
        rec_start = apos - 5
        if data[rec_start] != 0x54:   # 'T'
            raise Exception('unexpected outer opcode: 0x%02X' % data[rec_start])
        old_len = int.from_bytes(data[rec_start + 1:rec_start + 5], 'little')
        if old_len != len(all_pickled):
            raise Exception('outer length mismatch: %d != %d' % (old_len, len(all_pickled)))
        out = (data[:rec_start]
               + b'T' + len(new_ap).to_bytes(4, 'little') + new_ap
               + data[apos + len(all_pickled):])
    else:
        out = pickle.dumps((magic, all_pickled, signature), protocol=2)

    # sanity: the repacked file must parse and contain our entries
    m2, ap2, s2 = pickle.loads(out, encoding='bytes')
    d2 = pickle.loads(ap2, encoding='bytes')
    c2 = d2[b'code'] if b'code' in d2 else d2['code']
    found = set()
    for (filename, type_val), (encrypted_code2, hash_val2) in c2:
        fname = filename.decode('utf-8', errors='replace') if isinstance(filename, bytes) else filename
        if fname in TARGETS:
            raw2 = dec.decrypt_entry(hProv, hTdesKey, encrypted_code2)
            assert base.encode('utf-8') in raw2, 'patched URL missing in %s' % fname
            found.add(fname)
    assert found == set(TARGETS), 'target entries missing after repack: %s (missing %s)' % (found, set(TARGETS) - found)

    out_path = CODE_PATH + '.patched'
    with open(out_path, 'wb') as f:
        f.write(out)
    print('=== written: %s (%d bytes, original %d) ===' % (out_path, len(out), len(data)))
    print('next: test in a COPY of the client, then distribute via launcher update')
    print('cleaning up crypto handles')
    advapi32.CryptDestroyKey(hOneKey)
    advapi32.CryptDestroyKey(hTdesKey)
    advapi32.CryptReleaseContext(hProv, 0)


if __name__ == '__main__':
    main()
