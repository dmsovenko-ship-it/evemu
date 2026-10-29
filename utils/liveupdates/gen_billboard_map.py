import os
import marshal
import binascii

# Live-update #9 v7 (diagnostic): spaceObject.Billboard.UpdateBillboardContents
# Texture swaps after the first assignment do not render (the first one at login
# does). Dump dir() of the model and of the FaceMap texture parameter once per
# ball so we can find the reload/invalidate API, keep the marquee, keep the ad on
# FaceMap (the visible map) so a change would be obvious.


def UpdateBillboardContents(self, advertPath, facePath):
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return
    self.SetMap('AdvertMap', advertPath)
    self.SetMap('FaceMap', advertPath)
    self.SetMap('DiffuseMap', 'cache:/Temp/headlines.dds', 1)
    self.SetMap('DiffuseMap', 'cache:/Temp/bounty_caption.dds')
    try:
        if not getattr(self, '_bbdir', 0):
            self._bbdir = 1
            self.LogError('BBDIR model: ' + ' '.join([str(x) for x in dir(self.model)]))
            try:
                ps = [p for p in self.model.Find('trinity.TriTexture2DParameter') if p.name == 'FaceMap']
                if ps:
                    self.LogError('BBDIR facemap: ' + ' '.join([str(x) for x in dir(ps[0])]))
            except:
                pass
    except:
        pass
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
        self.LogError('BBMQ v7 trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
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
