import os
import marshal
import binascii

# Live-update payload for the Crucible client:
#   objectID   = svc.audio::AudioService
#   methodName = GetAudioBus
#
# The shipped client's audioService.GetAudioBus calls log.logError('Bus voice
# starvation!') when all audio bus channels are busy, but the client's log
# module only exports LogError (capital L). The AttributeError escapes
# Sprite.ApplyAttributes and kills HoloscreenSvc._PlayTemplates, so the CQ
# main screen stops cycling and piles every template into one frame.
#
# We ship a func_code with the SAME signature (self, is3D=False) that only
# changes log.logError -> log.LogError. audio2 / weakref / log are resolved
# from the target module's globals (func_code replacement keeps func_globals).


def GetAudioBus(self, is3D=False):
    for outputChannel, emitterWeakRef in self.busChannels.iteritems():
        if emitterWeakRef is None:
            emitter = audio2.AudEmitter('Bus Channel: ' + str(outputChannel))
            if is3D:
                emitter.SendEvent(unicode('Play_3d_audio_stream_' + str(outputChannel)))
            else:
                emitter.SendEvent(unicode('Play_2d_audio_stream_' + str(outputChannel)))
            self.busChannels[outputChannel] = weakref.ref(emitter, self.AudioBusDeathCallback)
            return (emitter, outputChannel)
    log.LogError('Bus voice starvation!')
    return (None, -1)


code = GetAudioBus.func_code
raw = marshal.dumps(code)

here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'logerror.raw'), 'wb') as f:
    f.write(raw)
with open(os.path.join(here, 'logerror.hex'), 'wb') as f:
    f.write(binascii.hexlify(raw))

# round-trip sanity check
assert marshal.loads(raw).co_name == 'GetAudioBus'
print 'ok bytes=%d argcount=%d' % (len(raw), code.co_argcount)
