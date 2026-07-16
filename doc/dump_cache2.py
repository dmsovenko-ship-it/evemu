"""Dump human-readable info from cache2 files"""
import os

bulk = r'C:\Program Files (x86)\CCP\EVE\bulkdata'

for fname in sorted(os.listdir(bulk)):
    if not fname.endswith('.cache2'):
        continue
    path = os.path.join(bulk, fname)
    sz = os.path.getsize(path)
    with open(path, 'rb') as f:
        data = f.read()

    # Extract printable strings with length-prefix format
    strings = []
    i = 8
    while i < len(data):
        length = data[i]
        if 2 < length < 60 and i + 1 + length <= len(data):
            chunk = data[i+1:i+1+length]
            try:
                s = chunk.decode('ascii')
                if s[0].isalpha() and all(c.isalnum() or c in '._' for c in s):
                    strings.append(s)
                    i += 1 + length
                    continue
            except:
                pass
        i += 1

    if strings:
        seen = set()
        unique = []
        for s in strings:
            if s not in seen:
                seen.add(s)
                unique.append(s)
        summary = ', '.join(unique[:12])
        print(f'{fname} ({sz:>8}B): {summary}')

print(f'\nTotal: {len(os.listdir(bulk))} cache2 files')
