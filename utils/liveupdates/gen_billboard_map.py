import os
import marshal
import binascii

# Live-update #9 (diagnostic): spaceObject.Billboard.UpdateBillboardContents
#   objectID   = spaceObject.Billboard::Billboard
#   methodName = UpdateBillboardContents
#
# Logs the real texture-parameter order of the billboard model, then runs the
# original mapping unchanged. The client's SetMap('DiffuseMap', path, idx) picks
# the idx-th parameter named "DiffuseMap" in model.Find() order, and we need to
# know which panel (Text, windowed 0.2 + scrolling curve, or Text2, full+faded)
# it actually lands on.


def UpdateBillboardContents(self, advertPath, facePath):
    try:
        found = self.model.Find('trinity.TriTexture2DParameter')
        out = []
        for p in found:
            out.append(str(p.name) + '|' + str(getattr(p, 'resourcePath', '')))
        self.LogError('BBMAP ' + ' ; '.join(out))
    except:
        try:
            self.LogError('BBMAP failed', str(sys.exc_info()[1]))
        except:
            pass
    self.LogInfo('UpdateBillboardWithPictureAndText:', advertPath, facePath)
    if self.model is None:
        return
    self.SetMap('FaceMap', facePath)
    self.SetMap('AdvertMap', advertPath)
    self.SetMap('DiffuseMap', 'cache:/Temp/headlines.dds', 1)
    self.SetMap('DiffuseMap', 'cache:/Temp/bounty_caption.dds')


code = UpdateBillboardContents.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'billboard_map.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'billboard_map.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

assert marshal.loads(raw).co_name == 'UpdateBillboardContents'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
