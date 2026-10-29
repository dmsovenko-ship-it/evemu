import os
import marshal
import binascii

# Live-update #9 v10: spaceObject.Billboard.UpdateBillboardContents
#
# v8 proved the reload trick works: param.SetResource(None) -> resourcePath = ''
# -> resourcePath = new path actually swaps the texture. v10 adds a per-ball
# timeline: 16s of "advert + news headline", then 8s of "wanted portrait + wanted
# caption" (the model's own AdvertToFaceFadeCurve does not run, so we drive it).
# The marquee (0.2 UV window + TextureTranslation.x) stays.



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

    self._bbAd = advertPath
    self._bbFace = facePath
    _settex('AdvertMap', advertPath, 0)
    _settex('FaceMap', advertPath, 0)
    _settex('DiffuseMap', 'cache:/Temp/headlines.dds', 0)
    _settex('DiffuseMap', 'cache:/Temp/bounty_caption.dds', 1)
    self.LogError('BBMAP10 ad=' + str(advertPath) + ' face=' + str(facePath))

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
        if trans and not getattr(self, '_bbmq', 0):
            self._bbmq = 1

            def _marquee():
                try:
                    period = 200000000
                    prev = -1.0
                    while not getattr(self, 'released', 1):
                        x = (blue.os.GetWallclockTime() % period) * 0.8 / period
                        if x < prev or x - prev > 0.003:
                            prev = x
                            for t in trans:
                                try:
                                    t.value = (x, 0.0, 0.0, 0.0)
                                except:
                                    pass
                        blue.pyos.BeNice()
                except:
                    pass

            uthread.pool('Billboard::Marquee', _marquee)

        if not getattr(self, '_bbtl', 0):
            self._bbtl = 1

            def _timeline():
                try:
                    prev = -1
                    while not getattr(self, 'released', 1):
                        sec = (blue.os.GetWallclockTime() / 10000000) % 24
                        phase = 0
                        if sec >= 16 and self._bbFace:
                            phase = 1
                        if phase != prev:
                            prev = phase
                            if phase == 0:
                                _settex('FaceMap', self._bbAd, 0)
                                _settex('DiffuseMap', 'cache:/Temp/headlines.dds', 0)
                            else:
                                _settex('FaceMap', self._bbFace, 0)
                                _settex('DiffuseMap', 'cache:/Temp/bounty_caption.dds', 0)
                        blue.pyos.BeNice()
                except:
                    pass

            uthread.pool('Billboard::Timeline', _timeline)

        self.LogError('BBMQ v10 trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
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
