#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

class StepSequencer;

const int kNumPresets = 8;
const int kNumChannels = 8;
const int kEnvelopeSteps = 64;
//const int kNumEnvParams = 68;

enum EEnvParams {
  kParamEnvLoopPoint = 0,
  kParamEnvRelPoint,
  kParamEnvLength,
  kParamEnvSpeedDiv,

  kNumEnvParams
};

// Channel params must be used with ParamFromCh.
enum EChParams {
  kParamChEnabled = 0,
  kParamChKeyTrack,
  kParamChVelSens,
  kParamChLegato,
  // 16 envelope parameters, must be contiguous
  kParamEnv1LoopPoint,
  kParamEnv1RelPoint,
  kParamEnv1Length,
  kParamEnv1SpeedDiv,
  kParamEnv2LoopPoint,
  kParamEnv2RelPoint,
  kParamEnv2Length,
  kParamEnv2SpeedDiv,
  kParamEnv3LoopPoint,
  kParamEnv3RelPoint,
  kParamEnv3Length,
  kParamEnv3SpeedDiv,
  kParamEnv4LoopPoint,
  kParamEnv4RelPoint,
  kParamEnv4Length,
  kParamEnv4SpeedDiv,

  kNumChParams
};

enum EParams
{
  kParamGain = 0,
  kParamNoteGlideTime,
  kParamOmniMode,
  kParamChannelBase,

  kNumParams = kParamChannelBase + kNumChParams * kNumChannels
};

std::pair<int, int> ResolveParamToChannelParam(int paramIdx) {
  if (paramIdx >= kParamChannelBase) {
    int ch = (paramIdx - kParamChannelBase) / kNumChParams;
    int param = (paramIdx - kParamChannelBase) % kNumChParams;
    return {ch, param};
  }
  return {-1, -1};
}

#if IPLUG_DSP
// will use EParams in LoudNES_DSP.h
#include "LoudNES_DSP.h"
#endif

enum EControlTags
{
  kCtrlTagKeyboard = 0,
  kCtrlTagBender,
  kCtrlTagModWheel,
  kCtrlTagKeyTrack,
  kCtrlTagVelSens,
  kCtrlTagLegato,
  kCtrlTagEnvelope1, // TODO: rename to StepSeq?
  kCtrlTagEnvelope2,
  kCtrlTagEnvelope3,
  kCtrlTagEnvelope4,
  kCtrlTagDpcmEditor,
  kCtrlTagEnv1LoopPoint,
  kCtrlTagEnv1RelPoint,
  kCtrlTagEnv1Length,
  kCtrlTagEnv1SpeedDiv,
  kCtrlTagEnv2LoopPoint,
  kCtrlTagEnv2RelPoint,
  kCtrlTagEnv2Length,
  kCtrlTagEnv2SpeedDiv,
  kCtrlTagEnv3LoopPoint,
  kCtrlTagEnv3RelPoint,
  kCtrlTagEnv3Length,
  kCtrlTagEnv3SpeedDiv,
  kCtrlTagEnv4LoopPoint,
  kCtrlTagEnv4RelPoint,
  kCtrlTagEnv4Length,
  kCtrlTagEnv4SpeedDiv,

  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class LoudNES final : public Plugin
{
public:
  explicit LoudNES(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
  void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  bool SerializeState(IByteChunk &chunk) const override;

  void OnPresetsModified() override;

  int UnserializeState(const IByteChunk &chunk, int startPos) override;

  static int ParamFromCh(int channelNum, int subParam) {
    return kParamChannelBase + channelNum * kNumChParams + subParam;
  }

private:
  LoudNESDSP<iplug::sample> mDSP;
  // TODO: Figure out why ISender works best with queue size 8
  ISender<1, 8, int> mEnvelopeVisSender;

#endif

  void UpdateStepSequencers();

  // TODO(montag): Code smell. State flows in two directions.
  //
  // This is invoked in the param change callback functions because the envelopes have "coupled" params
  // (e.g., loop point must be less than length), and those are computed in the NesEnvelope class.
  // Therefore, the UI must update NesEnvelope, then immediately read the state back out after the
  // constraints are enforced.
  //
  // I did not want the UI to know about the constraints between the parameters.
  //
  // This func is also invoked when a preset is loaded, because the NesChannels are updated directly
  // during preset deserialization rather than being updated through the param system (I think).
  //
  // In any case, there is room for improvement here.
  void UpdateStepSequencerAndParamsFromEnv(int paramEnvLoopPoint, NesEnvelope *env, StepSequencer* seq);
};
