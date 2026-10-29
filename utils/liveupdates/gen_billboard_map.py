import os
import marshal
import binascii

# Live-update #9 v12: spaceObject.Billboard.UpdateBillboardContents
#
# Facts learned from BBREF/BBMAP logs:
#  - the advert IS re-downloaded (new cache:/ files every 90s) but assigning a
#    texture on a live model never re-renders it, not even with
#    SetResource(None) + resourcePath reset;
#  - the VISIBLE text strip is DiffuseMap index 1.
#
# So v12 refreshes by REBUILDING the ball's model locally: UnSync() + Assemble()
# (the ball object stays -> the client camera keeps its target), and every
# rebuild alternates "advert + news" / "wanted portrait + wanted caption".
# The marquee re-reads its parameters from self._bbTrans so it survives rebuilds.


def UpdateBillboardContents(self, advertPath, facePath):
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return

    def _settex(name, path, idx):
        try:
            ps = self.model.Find('trinity.TriTexture2DParameter')
            same = []
            for p in ps:
                if p.name == name:
                    same.append(p)
            if len(same) > idx:
                q = same[idx]
                try:
                    q.SetResource(None)
                except:
                    pass
                try:
                    q.resourcePath = ''
                except:
                    pass
                try:
                    q.resourcePath = path
                except:
                    pass
        except:
            pass

    flip = getattr(self, '_bbFlip', 0) + 1
    self._bbFlip = flip
    showWanted = (flip % 2 == 0) and facePath
    if showWanted:
        face = facePath
        vis = 'cache:/Temp/bounty_caption.dds'
        other = 'cache:/Temp/headlines.dds'
    else:
        face = advertPath
        vis = 'cache:/Temp/headlines.dds'
        other = 'cache:/Temp/bounty_caption.dds'

    self._bbAd = advertPath
    self._bbFace = facePath
    _settex('AdvertMap', advertPath, 0)
    _settex('FaceMap', face, 0)
    _settex('DiffuseMap', other, 0)
    _settex('DiffuseMap', vis, 1)
    self.LogError('BBMAP12 flip=' + str(flip) + ' wanted=' + str(showWanted) + ' ad=' + str(advertPath))

    try:
        found = self.model.Find('trinity.Tr2Vector4Parameter')
        scales = []
        trans = []
        for p in found:
            if p.name == 'TextureScaling':
                scales.append(p)
            elif p.name == 'TextureTranslation':
                trans.append(p)
        for p in scales:
            try:
                p.value = (0.2, 1.0, 1.0, 1.0)
            except:
                pass
        self._bbTrans = trans

        if not getattr(self, '_bbmq', 0):
            self._bbmq = 1

            def _marquee():
                try:
                    period = 200000000
                    prev = -1.0
                    while not getattr(self, 'released', 1):
                        x = (blue.os.GetWallclockTime() % period) * 0.8 / period
                        if x < prev or x - prev > 0.003:
                            prev = x
                            tr = getattr(self, '_bbTrans', None)
                            if tr:
                                for t in tr:
                                    try:
                                        t.value = (x, 0.0, 0.0, 0.0)
                                    except:
                                        pass
                        blue.pyos.BeNice()
                except:
                    pass

            uthread.pool('Billboard::Marquee', _marquee)

        if not getattr(self, '_bbrl', 0):
            self._bbrl = 1

            def _refresher():
                try:
                    last = blue.os.GetWallclockTime()
                    while not getattr(self, 'released', 1):
                        now = blue.os.GetWallclockTime()
                        if now - last > 900000000:
                            last = now
                            try:
                                self.Assemble()
                                self.LogError('BBREFRESH rebuild')
                            except:
                                try:
                                    self.LogError('BBREFRESH failed: ' + str(sys.exc_info()[1]))
                                except:
                                    pass
                        blue.pyos.BeNice()
                except:
                    pass

            uthread.pool('Billboard::Refresh', _refresher)

        self.LogError('BBMQ v12 trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
    except:
        try:
            self.LogError('BBMQ failed: ' + str(sys.exc_info()[1]))
        except:
            pass


code = UpdateBillboardContents.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'billboard_map.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'billboard_map.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

assert marshal.loads(raw).co_name == 'UpdateBillboardContents'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
