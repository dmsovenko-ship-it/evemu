"""
EVE Crucible compiled.code decrypter/decompiler
Uses exponent-of-one RSA trick via Windows CryptoAPI to extract 3DES key from blue.dll
"""
import ctypes
import ctypes.wintypes
import marshal
import zlib
import pickle
import os
import sys
import struct

# WinCrypt constants
PROV_RSA_FULL = 1
CRYPT_VERIFYCONTEXT = 0xF0000000
CRYPT_NEWKEYSET = 0x00000008
AT_KEYEXCHANGE = 1
AT_SIGNATURE = 2
CRYPT_EXPORTABLE = 0x00000001
PUBLICKEYBLOB = 6
PRIVATEKEYBLOB = 7
SIMPLEBLOB = 1
PLAINTEXTKEYBLOB = 8
CALG_RSA_KEYX = 0x0000A400
CALG_3DES = 0x00006603

# CryptAPI types
HCRYPTPROV = ctypes.c_size_t  # ULONG_PTR
HCRYPTKEY = ctypes.c_size_t
HCRYPTHASH = ctypes.c_size_t

advapi32 = ctypes.windll.advapi32
kernel32 = ctypes.windll.kernel32

# Set argtypes for CryptAPI functions
advapi32.CryptAcquireContextA.argtypes = [ctypes.POINTER(HCRYPTPROV), ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
advapi32.CryptAcquireContextA.restype = ctypes.c_bool
advapi32.CryptGenKey.argtypes = [HCRYPTPROV, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(HCRYPTKEY)]
advapi32.CryptGenKey.restype = ctypes.c_bool
advapi32.CryptDestroyKey.argtypes = [HCRYPTKEY]
advapi32.CryptDestroyKey.restype = ctypes.c_bool
advapi32.CryptExportKey.argtypes = [HCRYPTKEY, HCRYPTKEY, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
advapi32.CryptExportKey.restype = ctypes.c_bool
advapi32.CryptImportKey.argtypes = [HCRYPTPROV, ctypes.c_void_p, ctypes.c_uint32, HCRYPTKEY, ctypes.c_uint32, ctypes.POINTER(HCRYPTKEY)]
advapi32.CryptImportKey.restype = ctypes.c_bool
advapi32.CryptDecrypt.argtypes = [HCRYPTKEY, HCRYPTHASH, ctypes.c_bool, ctypes.c_uint32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
advapi32.CryptDecrypt.restype = ctypes.c_bool
advapi32.CryptReleaseContext.argtypes = [HCRYPTPROV, ctypes.c_uint32]
advapi32.CryptReleaseContext.restype = ctypes.c_bool

def find_pattern(data, pattern):
    idx = data.find(pattern)
    if idx >= 0:
        return idx
    return -1

def read_blue_dll(path):
    with open(path, 'rb') as f:
        data = f.read()
    print(f"Read blue.dll: {len(data)} bytes")
    return data

def extract_keys(blue_data):
    RSA_PUB = b'\x06\x02\x00\x00\x00\x24\x00\x00\x52\x53\x41\x31'
    TDES_SIG = b'\x01\x02\x00\x00\x03\x66\x00\x00\x00\xA4\x00\x00'
    RSA_LEN = 0x94
    TDES_LEN = 0x8C

    rsa_idx = find_pattern(blue_data, RSA_PUB)
    if rsa_idx < 0:
        raise Exception("RSA public key not found in blue.dll")
    rsa_pub = blue_data[rsa_idx:rsa_idx+RSA_LEN]
    print(f"Found RSA public key at offset {rsa_idx}, {len(rsa_pub)} bytes")

    tdes_idx = find_pattern(blue_data, TDES_SIG)
    if tdes_idx < 0:
        raise Exception("3DES crypt blob not found in blue.dll")
    tdes_blob = blue_data[tdes_idx:tdes_idx+TDES_LEN]
    print(f"Found 3DES crypt blob at offset {tdes_idx}, {len(tdes_blob)} bytes")

    return rsa_pub, tdes_blob

def create_exponent_one_key(hProv):
    """Create an RSA key where all exponents are 1 (the exponent-of-one trick)"""
    hKey = HCRYPTKEY()

    # Generate a fresh key
    if not advapi32.CryptGenKey(hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, ctypes.byref(hKey)):
        raise Exception("CryptGenKey failed")

    # Export as PRIVATEKEYBLOB
    dwSize = ctypes.c_uint32(0)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, None, ctypes.byref(dwSize))
    keyblob = ctypes.create_string_buffer(dwSize.value)
    if not advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, keyblob, ctypes.byref(dwSize)):
        raise Exception("CryptExportKey failed")
    advapi32.CryptDestroyKey(hKey)

    # Parse blob to modify exponents
    buf = bytearray(keyblob.raw[:dwSize.value])

    # Get bit length from bytes 12-15
    dwBitLen = struct.unpack_from('<I', buf, 12)[0]

    # Convert pubexp in rsapubkey to 1 (bytes 16-19)
    buf[16] = 1
    buf[17] = 0
    buf[18] = 0
    buf[19] = 0

    ptr = 20  # after pubexp
    # Skip modulus
    ptr += dwBitLen // 8
    # Skip prime1
    ptr += dwBitLen // 16
    # Skip prime2
    ptr += dwBitLen // 16

    # exponent1 = 1
    for n in range(dwBitLen // 16):
        if n == 0:
            buf[ptr + n] = 1
        else:
            buf[ptr + n] = 0
    ptr += dwBitLen // 16

    # exponent2 = 1
    for n in range(dwBitLen // 16):
        if n == 0:
            buf[ptr + n] = 1
        else:
            buf[ptr + n] = 0
    ptr += dwBitLen // 16

    # Skip coefficient
    ptr += dwBitLen // 16

    # privateExponent = 1
    for n in range(dwBitLen // 8):
        if n == 0:
            buf[ptr + n] = 1
        else:
            buf[ptr + n] = 0

    # Re-import the modified key
    hOneKey = HCRYPTKEY()
    buf_bytes = bytes(buf)
    if not advapi32.CryptImportKey(hProv, buf_bytes, len(buf_bytes), HCRYPTKEY(0), 0, ctypes.byref(hOneKey)):
        raise Exception("CryptImportKey (one-key) failed")

    return hOneKey

def import_crypt_blob(hProv, hOneKey, tdes_blob):
    """Import the 3DES SIMPLEBLOB using the exponent-of-one key"""
    hTdesKey = HCRYPTKEY()
    if not advapi32.CryptImportKey(hProv, tdes_blob, len(tdes_blob), hOneKey, 0, ctypes.byref(hTdesKey)):
        err = kernel32.GetLastError()
        err = kernel32.GetLastError()
        raise Exception(f"CryptImportKey (3DES) failed, error={err}")

    return hTdesKey

def export_plaintext_key(hKey):
    """Export a key as PLAINTEXTKEYBLOB to get raw key bytes"""
    dwSize = ctypes.c_uint32(0)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PLAINTEXTKEYBLOB, 0, None, ctypes.byref(dwSize))
    buf = ctypes.create_string_buffer(dwSize.value)
    if not advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PLAINTEXTKEYBLOB, 0, buf, ctypes.byref(dwSize)):
        raise Exception("CryptExportKey (plaintext) failed")

    data = buf.raw[:dwSize.value]
    # PLAINTEXTKEYBLOB format:
    # BLOBHEADER (8 bytes)
    # ALG_ID (4 bytes) = CALG_3DES
    # key_size (4 bytes) = 24 (192 bits, but 3DES uses 168 bits)
    # key_data (key_size bytes)
    alg = struct.unpack_from('<I', data, 8)[0]
    key_size = struct.unpack_from('<I', data, 12)[0]
    key_data = data[16:16+key_size]
    print(f"Exported 3DES key: alg=0x{alg:08X}, size={key_size} bytes")
    return key_data

def decrypt_entry(hProv, hKey, encrypted_data):
    """Decrypt a single code entry: 3DES decrypt + zlib decompress"""
    buf = ctypes.create_string_buffer(encrypted_data)
    dwSize = ctypes.c_uint32(len(encrypted_data))

    if not advapi32.CryptDecrypt(hKey, HCRYPTHASH(0), 1, 0, buf, ctypes.byref(dwSize)):
        err = kernel32.GetLastError()
        raise Exception(f"CryptDecrypt failed, error={err}")

    decrypted = buf.raw[:dwSize.value]

    # zlib decompress
    try:
        decompressed = zlib.decompress(decrypted)
        return decompressed
    except Exception as e:
        print(f"zlib decompress failed: {e}, raw size={len(decrypted)}, first bytes={decrypted[:20].hex()}")
        raise

def decrypt_compiled_code(hProv, hKey, compiled_code_path, output_dir):
    """Decrypt and decompile all entries from compiled.code"""
    with open(compiled_code_path, 'rb') as f:
        data = f.read()

    # cPickle.load (Python 2 format)
    magic, all_pickled, signature = pickle.loads(data, encoding='bytes')
    print(f"Magic: {magic}, signature: {signature[:16].hex()}...")

    all_dict = pickle.loads(all_pickled, encoding='bytes')
    # Convert bytes keys to str
    if isinstance(all_dict, dict):
        all_dict = {k.decode('utf-8', errors='replace') if isinstance(k, bytes) else k: v for k, v in all_dict.items()}
    codes = all_dict['code']
    print(f"Total code entries: {len(codes)}")

    os.makedirs(output_dir, exist_ok=True)

    target_files = [
        'script:/parklife/autopilot.py',
        'root:/../carbon/common/script/net/SessionChangeGPCS.py',
        'root:/../carbon/common/script/sys/sessions.py',
        'root:/common/script/sys/eveSessions.py',
    ]

    for (filename, type_val), (encrypted_code, hash_val) in codes:
        if isinstance(filename, bytes):
            filename = filename.decode('utf-8', errors='replace')
        if isinstance(type_val, bytes):
            type_val = type_val.decode('utf-8', errors='replace')
        if filename not in target_files:
            continue

        print(f"\n=== Processing: {filename} ({type}, hash={hash_val}) ===")
        print(f"Encrypted size: {len(encrypted_code)} bytes")

        try:
            decompressed = decrypt_entry(hProv, hKey, encrypted_code)
            print(f"Decompressed size: {len(decompressed)} bytes")

            print(f"Decompressed size: {len(decompressed)} bytes")
            print(f"Decompressed first bytes: {decompressed[:8].hex()}")

            # Save raw decompressed bytes and .pyc file
            safe_name = filename.replace(':/', '_').replace('/', '_').replace('\\', '_').replace('.', '_')
            raw_path = os.path.join(output_dir, f"{safe_name}.raw")
            with open(raw_path, 'wb') as f:
                f.write(decompressed)
            print(f"Saved raw: {raw_path}")

            pyc_path = os.path.join(output_dir, f"{safe_name}.pyc")
            with open(pyc_path, 'wb') as f:
                # Python 2.7 magic + timestamp + code bytes
                f.write(b'\x03\xf3\x0d\x0a' + struct.pack('<II', int(hash_val), 0))
                f.write(decompressed)
            print(f"Saved pyc: {pyc_path}")

            # Try decompile with various tools
            import subprocess
            py_path = os.path.join(output_dir, f"{safe_name}.py")
            sys.stdout.flush()
            for tool in ['uncompyle6', 'decompyle3']:
                try:
                    cmd = [sys.executable, '-m', tool, '-o', str(py_path), str(pyc_path)]
                    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                    if result.returncode == 0:
                        print(f"Decompiled with {tool}: {py_path}")
                        break
                    else:
                        print(f"{tool} failed: {result.stderr[:200]}")
                except Exception as e:
                    print(f"{tool} error: {e}")

        except Exception as e:
            print(f"Failed to process {filename}: {e}")
            import traceback
            traceback.print_exc()


def export_raw_keys(hProv, rsa_pub, tdes_blob):
    """Export RSA public key and 3DES key to files (for debug)"""
    with open(os.path.join(output_dir, 'ccp.keys.pub'), 'wb') as f:
        f.write(rsa_pub)

    with open(os.path.join(output_dir, 'ccp.keys.crypt'), 'wb') as f:
        f.write(tdes_blob)

    print(f"Saved ccp.keys.pub ({len(rsa_pub)} bytes) and ccp.keys.crypt ({len(tdes_blob)} bytes)")


def decrypt_with_pycryptodome(rsa_pub, tdes_blob):
    """
    Alternative: try to extract the 3DES key using PyCryptodome directly
    by parsing the SIMPLEBLOB and using the public key.
    This works IF CCP encrypted the session key with the public key.
    """
    try:
        from Crypto.PublicKey import RSA
        from Crypto.Cipher import PKCS1_v1_5

        # Parse RSA PUBLICKEYBLOB
        # BLOBHEADER (8) + RSAPUBKEY (magic+bitlen+pubexp=12) + modulus (bitlen/8)
        pub_blob = rsa_pub
        bit_len = struct.unpack_from('<I', pub_blob, 12)[0]
        pub_exp = struct.unpack_from('<I', pub_blob, 16)[0]
        mod_bytes = pub_blob[20:20+bit_len//8]
        n = int.from_bytes(mod_bytes, 'little')
        e = pub_exp

        print(f"RSA key: {bit_len} bits, e={e}, n={hex(n)[:40]}...")

        # Parse SIMPLEBLOB
        # BLOBHEADER (8) + export_alg (4) + encrypted_data
        encrypted_key = tdes_blob[12:]  # skip header
        print(f"Encrypted key size: {len(encrypted_key)} bytes")

        # Try to decrypt with public key (standard PKCS1 encryption uses public key for encrypt)
        # If CCP used public key to encrypt, we need the private key to decrypt
        # This won't work with just the public key
        rsa_key = RSA.construct((n, e))
        cipher = PKCS1_v1_5.new(rsa_key)
        # cipher.decrypt() requires private key
        # Can't do this with only public key
        print("PyCryptodome approach: need private key for decryption")

    except ImportError:
        print("PyCryptodome not available")
    except Exception as e:
        print(f"PyCryptodome error: {e}")


if __name__ == '__main__':
    eve_dir = r"C:\Program Files (x86)\CCP\EVE"
    output_dir = r"C:\EVE_unpacked\extracted"

    blue_path = os.path.join(eve_dir, "bin", "blue.dll")
    compiled_path = os.path.join(eve_dir, "script", "compiled.code")

    print("=== Step 1: Extract keys from blue.dll ===")
    blue_data = read_blue_dll(blue_path)
    rsa_pub, tdes_blob = extract_keys(blue_data)

    print("\n=== Step 2: Initialize CryptoAPI context ===")
    hProv = HCRYPTPROV()
    if not advapi32.CryptAcquireContextA(
        ctypes.byref(hProv), None, None, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT
    ):
        raise Exception("CryptAcquireContext failed")

    print("\n=== Step 3: Create exponent-of-one RSA key ===")
    hOneKey = create_exponent_one_key(hProv)
    print("Exponent-of-one key created")

    print("\n=== Step 4: Import 3DES crypt blob ===")
    hTdesKey = import_crypt_blob(hProv, hOneKey, tdes_blob)
    print("3DES key imported")

    print("\n=== Step 5: Decrypt compiled.code ===")
    decrypt_compiled_code(hProv, hTdesKey, compiled_path, output_dir)

    # Cleanup
    advapi32.CryptDestroyKey(hOneKey)
    advapi32.CryptDestroyKey(hTdesKey)
    advapi32.CryptReleaseContext(hProv, 0)

    print("\nDone!")
