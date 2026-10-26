import os
import marshal
import binascii

# Live-update payload for the Crucible client:
#   objectID   = svc.billboard::BillboardMgr
#   methodName = GenerateAdvert
#
# The shipped client does:
#   URL = 'http://.../billboard.asp?%s' % self.patch.GetWebRequestParameters()
# which raises "TypeError: not all arguments converted during string formatting"
# (our compiled.code URL patch / build leaves the format string broken). The
# exception escapes Billboard::LateAssembleUpdate -> sm.GetService('billboard')
# -> StartServiceAndWaitForRunningState, which then blocks forever and the whole
# space scene never finishes loading.
#
# Replacement keeps the same signature (self) and is fully exception-safe: it
# never uses %-formatting, tolerates a tuple/None from GetWebRequestParameters,
# fetches OUR portal advert and returns the texture path (or None on any error).
# func_code replacement keeps the ORIGINAL func_globals (billboardMgr module),
# so blue / str / type / len resolve there.


def GenerateAdvert(self):
    try:
        params = self.patch.GetWebRequestParameters()
        if type(params) in (list, tuple):
            params = params[0] if len(params) else ''
        elif params is None:
            params = ''
        url = 'http://router.iks-online.net:26006/billboard.asp?' + str(params)
        tex, tWidth, tHeight = self.photo.GetTextureFromURL(url)
        codefile = blue.classes.CreateInstance('blue.ResFile')
        if not codefile.Open(tex.resPath):
            return
        if 'none.dds' in tex.resPath:
            return
        return tex.resPath
    except:
        return


code = GenerateAdvert.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'billboard_advert.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'billboard_advert.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

assert marshal.loads(raw).co_name == 'GenerateAdvert'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
