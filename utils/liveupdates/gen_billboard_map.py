import os
import marshal
import binascii

# Live-update #9 v13: spaceObject.Billboard.UpdateBillboardContents
#
# The server re-emits the billboard ball every 90s (the only thing that actually
# re-renders a new texture), so the client must not rebuild anything itself.
#
# What the client does here:
#  - applies AdvertMap/FaceMap and the visible text strip (index 1) from the
#    paths the service already holds, alternating "advert + news" / "wanted
#    portrait + wanted caption" on each reassemble;
#  - runs the marquee (0.2 UV window + TextureTranslation.x, 20s per pass) and
#    re-reads its parameters from self._bbTrans so it survives model rebuilds;
#  - runs a PREFETCH loop every 60s: downloads the next advert (cache-buster),
#    re-renders the news headlines and the wanted caption/portrait and stores them
#    on the billboard service, so the next server re-emit applies an already
#    downloaded texture (no Quafe flash).


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

    def _todds(path):
        try:
            import uiutil
            dev = trinity.device
            surface = dev.CreateOffscreenPlainSurface(8, 8, trinity.TRIFMT_A8R8G8B8, trinity.TRIPOOL_SYSTEMMEM)
            data = surface.LoadSurfaceFromFile(blue.ResFile(path))
            w = data['Width']
            h = data['Height']
            bw, bh = uiutil.GetBuffersize(w), uiutil.GetBuffersize(h)
            tex = dev.CreateTexture(bw, bh, 1, 0, trinity.TRIFMT_A8R8G8B8, trinity.TRIPOOL_MANAGED)
            dev.UpdateSurface(surface, None, tex.GetSurfaceLevel(0))
            out = blue.paths.ResolvePathForWriting('cache:/Temp/wanted.dds')
            tex.SaveToDDS(out)
            return 'cache:/Temp/wanted.dds'
        except:
            return path

    try:
        svc0 = sm.GetService('billboard')
        now0 = blue.os.GetWallclockTime()
        if now0 - getattr(svc0, '_bbFlipStamp', 0) > 300000000:
            svc0._bbFlipStamp = now0
            svc0._bbFlipSvc = getattr(svc0, '_bbFlipSvc', 0) + 1
        flip = getattr(svc0, '_bbFlipSvc', 1)
    except:
        flip = getattr(self, '_bbFlip', 0) + 1
        self._bbFlip = flip
    showWanted = (flip % 2 == 0) and facePath
    if showWanted:
        face = _todds(facePath)
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
    self.LogError('BBMAP13 flip=' + str(flip) + ' wanted=' + str(showWanted != None and showWanted != False))

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
                        blue.pyos.synchro.SleepWallclock(50)
                except:
                    pass

            uthread.pool('Billboard::Marquee', _marquee)

        if not getattr(self, '_bbpf', 0):
            self._bbpf = 1

            def _prefetch():
                try:
                    last = 0
                    while not getattr(self, 'released', 1):
                        now = blue.os.GetWallclockTime()
                        if last == 0 or now - last > 600000000:
                            last = now
                            try:
                                svc = sm.GetService('billboard')
                                try:
                                    params = svc.patch.GetWebRequestParameters()
                                    if type(params) in (list, tuple):
                                        params = params[0] if len(params) else ''
                                    elif params is None:
                                        params = ''
                                    url = 'http://router.iks-online.net:26006/billboard.asp?' + str(params) + '&t=' + str(now)
                                    tex, tw, th = sm.GetService('photo').GetTextureFromURL(url)
                                    if tex is not None and 'none.dds' not in tex.resPath:
                                        svc.advertPath = tex.resPath
                                        self._bbAd = tex.resPath
                                except:
                                    pass
                                try:
                                    svc.DoNewsHeadlines()
                                except:
                                    pass
                                try:
                                    svc.UpdateBounty()
                                    if svc.facePath:
                                        self._bbFace = svc.facePath
                                except:
                                    pass
                                self.LogError('BBPF ' + str(svc.advertPath) + ' / ' + str(svc.facePath))
                            except:
                                pass
                        blue.pyos.synchro.SleepWallclock(500)
                except:
                    pass

            uthread.pool('Billboard::Prefetch', _prefetch)

        self.LogError('BBMQ v13 trans=' + str(len(trans)) + ' scales=' + str(len(scales)))
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
