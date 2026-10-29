import os
import marshal
import binascii

# Live-update payload for the Crucible client:
#   objectID   = svc.billboard::BillboardMgr
#   methodName = Update
#
# BillboardMgr.Run() stores self.advertPath ONCE per session and
# Billboard.UpdateBillboardContents() always re-applies that same path, so the
# server's periodic billboard ball re-emit (Client::SendServiceCall +
# SystemManager::ResendBillboards) had no visible effect.
#
# photo.GetTextureFromURL() caches by URL in browserCache, so a stable advert URL
# always returns the cached DDS (and trinity will not reload a texture whose path
# did not change). We append a wallclock cache-buster so the cache misses, a new
# file name is produced and the model actually reloads it.
#
# func_code replacement keeps the ORIGINAL func_globals (billboardMgr module):
# blue / sys / type / len / str resolve there.


def Update(self, model=None):
    try:
        params = self.patch.GetWebRequestParameters()
        if type(params) in (list, tuple):
            params = params[0] if len(params) else ''
        elif params is None:
            params = ''
        url = 'http://router.iks-online.net:26006/billboard.asp?' + str(params) + '&t=' + str(blue.os.GetWallclockTime())
        tex, tWidth, tHeight = self.photo.GetTextureFromURL(url)
        if tex is not None:
            path = tex.resPath
            if 'none.dds' not in path:
                self.advertPath = path
    except:
        try:
            self.LogError('BBLU8 advert refresh failed:', str(sys.exc_info()[1]))
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
