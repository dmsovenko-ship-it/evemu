import os
import marshal
import binascii

# Live-update #9 v4: spaceObject.Billboard.UpdateBillboardContents
#
# BBMAP order: 0 DiffuseMap->Text (0.2 window+curve), 1 DiffuseMap->Text2 (full,
# curved brightness), 5 AdvertMap->Screen, 8 DiffuseMap->Base(hull). The visible
# strip is index 1. v3 tried model.Find('trinity.TriVector4Parameter') but logged
# nothing, which means the lookup returned <=1 params (the guard swallowed it).
# v4 logs the vector parameter list unconditionally (both class spellings) and
# applies the 0.2 UV window to every TextureScaling it finds.


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
    for cls in ('trinity.TriVector4Parameter', 'trinity.Tr2Vector4Parameter'):
        try:
            found = self.model.Find(cls)
            names = []
            for p in found:
                names.append(str(p.name))
            self.LogError('BBVEC ' + cls + ' n=' + str(len(found)) + ' [' + ', '.join(names) + ']')
            hit = 0
            for p in found:
                if p.name == 'TextureScaling':
                    try:
                        p.value = (0.2, 1.0, 1.0, 1.0)
                        hit += 1
                    except:
                        pass
            self.LogError('BBVEC set scaling on ' + str(hit))
        except:
            try:
                self.LogError('BBVEC Find failed: ' + cls + ' ' + str(sys.exc_info()[1]))
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
