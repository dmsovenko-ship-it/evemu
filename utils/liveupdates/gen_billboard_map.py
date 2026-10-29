import os
import marshal
import binascii

# Live-update #9 v8: spaceObject.Billboard.UpdateBillboardContents
#
# BBDIR showed TriTexture2DParameter has SetResource/resource/resourcePath, and the
# planet code (planet.ResourceCallback) reloads a texture with
#   param.SetResource(None); param.resourcePath = ''  -> later SetResource(tex)
# SetMap only assigns resourcePath, which renders the FIRST time and is ignored
# afterwards. v8 detaches the resource and re-assigns the path for AdvertMap and
# FaceMap (the visible map), keeping the text marquee from v6.


def UpdateBillboardContents(self, advertPath, facePath):
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return
    try:
        texs = self.model.Find('trinity.TriTexture2DParameter')
        n = 0
        for p in texs:
            if p.name in ('AdvertMap', 'FaceMap'):
                try:
                    p.SetResource(None)
                except:
                    pass
                try:
                    p.resourcePath = ''
                except:
                    pass
                try:
                    p.resourcePath = advertPath
                    n = n + 1
                except:
                    pass
        self.LogError('BBRELOAD ' + str(n) + ' -> ' + str(advertPath))
    except:
        try:
            self.LogError('BBRELOAD failed: ' + str(sys.exc_info()[1]))
        except:
            pass
    self.SetMap('DiffuseMap', 'cache:/Temp/headlines.dds', 1)
    self.SetMap('DiffuseMap', 'cache:/Temp/bounty_caption.dds')
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
        self.LogError('BBMQ v8 trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
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
