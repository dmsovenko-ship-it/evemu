import os
import marshal
import binascii

# Live-update #9 v5: spaceObject.Billboard.UpdateBillboardContents
#
# BBVEC showed the model exposes Tr2Vector4Parameter params:
#   [TextureScaling, TextureTranslation, DiffuseColor,
#    TextureScaling, TextureTranslation, DiffuseColor,
#    HologramFxFactors2, HologramFxFactors, ScrollerColor, ...]
# Setting .value works (set scaling on 2 succeeded), but the model's own
# TextScrollingCurve does not move the texture, so v5 sets the 0.2 UV window and
# runs its own marquee loop that advances TextureTranslation.x (0..0.8, wrap).
# The loop runs in a uthread tasklet and stops when the ball is released.


def UpdateBillboardContents(self, advertPath, facePath):
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return
    self.SetMap('FaceMap', facePath)
    self.SetMap('AdvertMap', advertPath)
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
        self.LogError('BBMQ trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
        if trans and not getattr(self, '_bbmq', 0):
            self._bbmq = 1

            def _marquee():
                try:
                    x = 0.0
                    while not getattr(self, 'released', 1):
                        for t in trans:
                            try:
                                t.value = (x, 0.0, 0.0, 0.0)
                            except:
                                pass
                        x = x + 0.004
                        if x > 0.8:
                            x = 0.0
                        blue.pyos.BeNice()
                except:
                    pass

            uthread.pool('Billboard::Marquee', _marquee)
            self.LogError('BBMQ marquee started')
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
