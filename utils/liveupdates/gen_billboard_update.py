import os
import marshal
import binascii

# Live-update #8 v2: svc.billboard::BillboardMgr -> Update
#
# The server re-emits the billboard balls every 90s (Client::ProcessClient ->
# SystemManager::ResendBillboards) and the client rebuilds the model. v1 of this
# patch downloaded a fresh advert inside Update, so the reassembled ball showed
# the baked Quafe texture until the download finished (the "flash").
#
# The download now happens in a prefetch loop inside the billboard ball itself
# (live-update #9), so Update only re-picks the wanted portrait and delegates to
# the original __UpdateBillboard, which applies the already cached advert path
# immediately.


def Update(self, model=None):
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
