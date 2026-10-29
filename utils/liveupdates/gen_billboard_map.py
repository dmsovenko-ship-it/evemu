import os
import marshal
import binascii

# Live-update #10: spaceObject.Billboard.UpdateBillboardContents
#
# BBMAP (live-update #9) showed the real model texture order:
#   0 DiffuseMap -> Text   (TextureScaling U=0.2 window + TextScrollingCurve)
#   1 DiffuseMap -> Text2  (no scaling, Brightness driven by Text2FadeCurve)
#   ...
#   5 AdvertMap  -> Screen
#   8 DiffuseMap -> Base (hull)
#
# The client put the news headline on index 1 (Text2 - faded, no window) and the
# wanted caption on index 0 (Text - the scrolling window), so the marquee text
# was never the headline. Swap them: headlines -> Text (index 0), bounty -> Text2.


def UpdateBillboardContents(self, advertPath, facePath):
    try:
        found = self.model.Find('trinity.TriTexture2DParameter')
        out = []
        for p in found:
            out.append(str(p.name) + '|' + str(getattr(p, 'resourcePath', '')))
        self.LogError('BBMAP ' + ' ; '.join(out))
    except:
        pass
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return
    self.SetMap('FaceMap', facePath)
    self.SetMap('AdvertMap', advertPath)
    self.SetMap('DiffuseMap', 'cache:/Temp/headlines.dds')
    self.SetMap('DiffuseMap', 'cache:/Temp/bounty_caption.dds', 1)


code = UpdateBillboardContents.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'billboard_map.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'billboard_map.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

assert marshal.loads(raw).co_name == 'UpdateBillboardContents'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
