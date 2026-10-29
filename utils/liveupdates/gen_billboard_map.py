import os
import marshal
import binascii

# Live-update #9 v3: spaceObject.Billboard.UpdateBillboardContents
#
# BBMAP order: 0 DiffuseMap->Text (0.2 window+curve), 1 DiffuseMap->Text2
# (full texture, curved brightness), 5 AdvertMap->Screen, 8 DiffuseMap->Base(hull).
# The VISIBLE strip turned out to be index 1 (Text2): the swap made the wanted
# caption appear there, static, because Text2 has no TextureScaling window and its
# own curve does not move the texture (SetMap freezes the high detail mesh).
#
# So: keep the headline on the visible panel (index 1) and give that panel the
# 0.2 UV window that the Text panel has, which is what the marquee needs.


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
    self.SetMap('DiffuseMap', 'cache:/Temp/headlines.dds', 1)
    self.SetMap('DiffuseMap', 'cache:/Temp/bounty_caption.dds')
    try:
        scales = []
        for p in self.model.Find('trinity.TriVector4Parameter'):
            if p.name == 'TextureScaling':
                scales.append(p)
        if len(scales) > 1:
            scales[1].value = (0.2, 1.0, 1.0, 1.0)
            self.LogError('BBSCALE set window on visible panel')
    except:
        try:
            self.LogError('BBSCALE failed', str(sys.exc_info()[1]))
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
