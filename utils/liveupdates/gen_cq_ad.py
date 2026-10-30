import os, marshal, binascii

def SetDefaultPlaylist(self):
    class _CQAd(object):
        def __init__(self, **kw):
            self.parent = kw.get('parent')
        def Play(self, data):
            try:
                path = None
                if data is not None:
                    path = data.get('img')
                if not path:
                    try:
                        path = sm.GetService('billboard').advertPath
                    except:
                        path = None
                if not path:
                    path = 'res:/UI/Texture/classes/CQLoadingScreen/loadingScreen.png'
                if path:
                    self.sprite = uicls.Sprite(parent=self.parent, texturePath=path, align=uiconst.TOALL)
            except:
                pass

    self.playlist = [
     (cqscreen.templates.SOV, self.GetSOVTemplateData),
     (cqscreen.templates.CareerAgent, self.GetCareerAgentTemplateData),
     (cqscreen.templates.Incursion, self.GetIncursionTemplateData),
     (cqscreen.templates.ShipExposure, self.GetShipExposureTemplateData),
     (cqscreen.templates.RacialEpicArc, self.GetRacialEpicArcTemplateData),
     (cqscreen.templates.CharacterInfo, self.GetNPEEpicArcTemplateData),
     (cqscreen.templates.CharacterInfo, self.GetWantedTemplateData),
     (cqscreen.templates.Plex, self.GetPlexTemplateData),
     (cqscreen.templates.AuraMessage, self.GetSkillTrainingTemplateData),
     (cqscreen.templates.AuraMessage, self.GetCloneStatusTemplateData),
     (cqscreen.templates.VirtualGoodsStore, self.GetVirtualGoodsStoreTemplateData)]
    customVideos = self.GetCustomVideoPlaylist()
    if customVideos:
        self.playlist = customVideos + self.playlist
    try:
        self.playlist = [(_CQAd, util.KeyVal(img=None))] + self.playlist
    except:
        pass

code = SetDefaultPlaylist.func_code
raw = marshal.dumps(code)
here = os.path.dirname(os.path.abspath(__file__))
open(os.path.join(here, 'cq_ad.raw'), 'wb').write(raw)
open(os.path.join(here, 'cq_ad.hex'), 'wb').write(binascii.hexlify(raw))
assert marshal.loads(raw).co_name == 'SetDefaultPlaylist'
print 'ok bytes=%d' % len(raw)