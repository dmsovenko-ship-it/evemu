import os
import marshal
import binascii

# Live-update payload for the Crucible client:
#   objectID   = svc.billboard::BillboardMgr
#   methodName = Update
#
# Problem: BillboardMgr.Run() sets self.advertPath ONCE at session start, and
# Billboard.UpdateBillboardContents() always re-applies that stored path. The
# server re-emits the billboard ball every 90s -> Billboard.Assemble ->
# LateAssembleUpdate -> billboardSvc.Update(self), but it only re-applies the
# SAME textures, so nothing visibly changes.
#
# Also: photo.GetTextureFromURL() caches by URL (browserCache), and the advert
# URL is stable, so a plain re-call returns the cached DDS. We pass ignoreCache=1
# to force a fresh download each time (the cache entry is simply replaced, so
# the client cache does not grow).
#
# func_code replacement keeps the ORIGINAL func_globals (billboardMgr module):
# blue / type / len / str / getattr resolve there.


def Update(self, model=None):
    try:
        params = self.patch.GetWebRequestParameters()
        if type(params) in (list, tuple):
            params = params[0] if len(params) else ''
        elif params is None:
            params = ''
        url = 'http://router.iks-online.net:26006/billboard.asp?' + str(params)
        tex, tWidth, tHeight = self.photo.GetTextureFromURL(url, None, 1)
        if tex is not None:
            path = tex.resPath
            if 'none.dds' not in path:
                self.advertPath = path
    except:
        pass
    try:
        self.UpdateBounty()
    except:
        pass
    self._BillboardMgr__UpdateBillboard(model)


code = Update.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'billboard_update.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'billboard_update.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

assert marshal.loads(raw).co_name == 'Update'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
