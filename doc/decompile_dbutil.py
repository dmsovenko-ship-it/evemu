import ctypes, struct, zlib, marshal, pickle

HCRYPTPROV = ctypes.c_size_t
HCRYPTKEY = ctypes.c_size_t
HCRYPTHASH = ctypes.c_size_t

advapi32 = ctypes.windll.advapi32

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

RSA_PUB_SIG = b'\x06\x02\x00\x00\x00\x24\x00\x00\x52\x53\x41\x31'
TDES_SIG = b'\x01\x02\x00\x00\x03\x66\x00\x00\x00\xa4\x00\x00'

def get_3des_key(hProv):
    AT_KEYEXCHANGE = 1
    CRYPT_EXPORTABLE = 0x00000001
    PRIVATEKEYBLOB = 7

    with open(r'C:\Program Files (x86)\CCP\EVE\bin\blue.dll', 'rb') as f:
        blue_data = f.read()
    tdes_idx = blue_data.find(TDES_SIG)
    tdes_blob = blue_data[tdes_idx:tdes_idx + 0x8C]

    hKey = HCRYPTKEY()
    advapi32.CryptGenKey(hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, ctypes.byref(hKey))
    dwSize = ctypes.c_uint32(0)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, None, ctypes.byref(dwSize))
    keyblob = ctypes.create_string_buffer(dwSize.value)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, keyblob, ctypes.byref(dwSize))
    advapi32.CryptDestroyKey(hKey)

    buf = bytearray(keyblob.raw[:dwSize.value])
    dwBitLen = struct.unpack_from('<I', buf, 12)[0]
    buf[16:20] = b'\x01\x00\x00\x00'
    ptr = 20 + dwBitLen // 8 + dwBitLen // 16 + dwBitLen // 16
    for n in range(dwBitLen // 16): buf[ptr + n] = 1 if n == 0 else 0
    ptr += dwBitLen // 16
    for n in range(dwBitLen // 16): buf[ptr + n] = 1 if n == 0 else 0
    ptr += dwBitLen // 16 + dwBitLen // 16
    for n in range(dwBitLen // 8): buf[ptr + n] = 1 if n == 0 else 0

    hOneKey = HCRYPTKEY()
    advapi32.CryptImportKey(hProv, bytes(buf), len(buf), HCRYPTKEY(0), 0, ctypes.byref(hOneKey))
    hTdesKey = HCRYPTKEY()
    advapi32.CryptImportKey(hProv, tdes_blob, len(tdes_blob), hOneKey, 0, ctypes.byref(hTdesKey))
    advapi32.CryptDestroyKey(hOneKey)
    return hTdesKey

def decrypt(hTdesKey, encrypted):
    buf = ctypes.create_string_buffer(encrypted)
    dwSize = ctypes.c_uint32(len(encrypted))
    advapi32.CryptDecrypt(hTdesKey, HCRYPTHASH(0), 1, 0, buf, ctypes.byref(dwSize))
    decrypted = buf.raw[:dwSize.value]
    return zlib.decompress(decrypted)

# Read compiled.code
with open(r'C:\Program Files (x86)\CCP\EVE\script\compiled.code', 'rb') as f:
    cc = f.read()

magic, all_pickled, signature = pickle.loads(cc, encoding='bytes')
all_dict = pickle.loads(all_pickled, encoding='bytes')
all_dict = {k.decode('utf-8', errors='replace') if isinstance(k, bytes) else k: v for k, v in all_dict.items()}
codes = all_dict['code']

PROV_RSA_FULL = 1
CRYPT_VERIFYCONTEXT = 0xF0000000
hProv = HCRYPTPROV()
advapi32.CryptAcquireContextA(ctypes.byref(hProv), None, None, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)

# List target files
targets = [
    ('root:/../carbon/common/script/util/dbutil.py', 'dbutil'),
    ('root:/../carbon/common/script/net/machoNet.py', 'machoNet'),
    ('root:/common/script/sys/eveCfg.py', 'eveCfg'),
]

extracted_dir = r'C:\EVE_unpacked\extracted'
for target_name, short_name in targets:
    for (filename, type_val), (encrypted_code, hash_val) in codes:
        if isinstance(filename, bytes):
            filename = filename.decode('utf-8', errors='replace')
        if filename == target_name:
            print(f"Extracting: {filename}")
            hTdesKey = get_3des_key(hProv)
            data = decrypt(hTdesKey, encrypted_code)
            advapi32.CryptDestroyKey(hTdesKey)

            PYTHON27_MAGIC = 62211
            pyc_path = f'{extracted_dir}/{short_name}.pyc'
            with open(pyc_path, 'wb') as f:
                f.write(struct.pack('<H', PYTHON27_MAGIC) + b'\x0d\x0a')
                f.write(struct.pack('<I', hash_val))
                f.write(data)

            import uncompyle6
            py_path = f'{extracted_dir}/{short_name}.py'
            with open(py_path, 'w', encoding='utf-8') as f:
                uncompyle6.decompile_file(pyc_path, f)
            print(f"  -> {py_path}")
            break

advapi32.CryptReleaseContext(hProv, 0)
print("Done!")
