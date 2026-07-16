import pickle, struct, ctypes, zlib, uncompyle6

HCRYPTPROV = ctypes.c_size_t; HCRYPTKEY = ctypes.c_size_t; HCRYPTHASH = ctypes.c_size_t
advapi32 = ctypes.windll.advapi32
advapi32.CryptAcquireContextA.argtypes = [ctypes.POINTER(HCRYPTPROV), ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
advapi32.CryptAcquireContextA.restype = ctypes.c_bool
advapi32.CryptGenKey.argtypes = [HCRYPTPROV, ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(HCRYPTKEY)]
advapi32.CryptGenKey.restype = ctypes.c_bool
advapi32.CryptDestroyKey.argtypes = [HCRYPTKEY]; advapi32.CryptDestroyKey.restype = ctypes.c_bool
advapi32.CryptExportKey.argtypes = [HCRYPTKEY, HCRYPTKEY, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
advapi32.CryptExportKey.restype = ctypes.c_bool
advapi32.CryptImportKey.argtypes = [HCRYPTPROV, ctypes.c_void_p, ctypes.c_uint32, HCRYPTKEY, ctypes.c_uint32, ctypes.POINTER(HCRYPTKEY)]
advapi32.CryptImportKey.restype = ctypes.c_bool
advapi32.CryptDecrypt.argtypes = [HCRYPTKEY, HCRYPTHASH, ctypes.c_bool, ctypes.c_uint32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
advapi32.CryptDecrypt.restype = ctypes.c_bool
advapi32.CryptReleaseContext.argtypes = [HCRYPTPROV, ctypes.c_uint32]; advapi32.CryptReleaseContext.restype = ctypes.c_bool

TDES_SIG = b'\x01\x02\x00\x00\x03\x66\x00\x00\x00\xa4\x00\x00'
with open(r'C:\Program Files (x86)\CCP\EVE\bin\blue.dll', 'rb') as f: blue_data = f.read()
tdes_idx = blue_data.find(TDES_SIG); tdes_blob = blue_data[tdes_idx:tdes_idx + 0x8C]

def make_key(hProv):
    AT_KEYEXCHANGE = 1; CRYPT_EXPORTABLE = 0x00000001; PRIVATEKEYBLOB = 7
    hKey = HCRYPTKEY()
    advapi32.CryptGenKey(hProv, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, ctypes.byref(hKey))
    dwSize = ctypes.c_uint32(0)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, None, ctypes.byref(dwSize))
    keyblob = ctypes.create_string_buffer(dwSize.value)
    advapi32.CryptExportKey(hKey, HCRYPTKEY(0), PRIVATEKEYBLOB, 0, keyblob, ctypes.byref(dwSize))
    advapi32.CryptDestroyKey(hKey)
    buf = bytearray(keyblob.raw[:dwSize.value]); dwBitLen = struct.unpack_from('<I', buf, 12)[0]
    buf[16:20] = b'\x01\x00\x00\x00'; ptr = 20 + dwBitLen // 8 + dwBitLen // 16 + dwBitLen // 16
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

with open(r'C:\Program Files (x86)\CCP\EVE\script\compiled.code', 'rb') as f: cc = f.read()
all_dict = pickle.loads(pickle.loads(cc, encoding='bytes')[1], encoding='bytes')
all_dict = {k.decode('utf-8', errors='replace') if isinstance(k, bytes) else k: v for k, v in all_dict.items()}

for (filename, type_val), (encrypted_code, hash_val) in all_dict['code']:
    if isinstance(filename, bytes): filename = filename.decode('utf-8', errors='replace')
    if 'pathfinder' in filename:
        hProv = HCRYPTPROV()
        advapi32.CryptAcquireContextA(ctypes.byref(hProv), None, None, 1, 0xF0000000)
        hTdesKey = make_key(hProv)
        buf = ctypes.create_string_buffer(encrypted_code)
        dwSize = ctypes.c_uint32(len(encrypted_code))
        advapi32.CryptDecrypt(hTdesKey, HCRYPTHASH(0), 1, 0, buf, ctypes.byref(dwSize))
        dec = zlib.decompress(buf.raw[:dwSize.value])
        advapi32.CryptDestroyKey(hTdesKey); advapi32.CryptReleaseContext(hProv, 0)
        with open(r'C:\EVE_unpacked\extracted\pathfinder.pyc', 'wb') as f:
            f.write(struct.pack('<H', 62211) + b'\x0d\x0a' + struct.pack('<I', hash_val))
            f.write(dec)
        with open(r'C:\EVE_unpacked\extracted\pathfinder.py', 'w', encoding='utf-8') as f:
            uncompyle6.decompile_file(r'C:\EVE_unpacked\extracted\pathfinder.pyc', f)
        print('OK: pathfinder')
