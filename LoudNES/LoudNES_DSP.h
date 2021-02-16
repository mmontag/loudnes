#pragma once

#include "MidiSynth.h"
#include "NesApu.h"
#include "NesVoice.h"
#include "NesDpcm.h"

using namespace iplug;

template<typename T>
class LoudNESDSP
{
public:
#pragma mark -
  LoudNESDSP()
  {
      shared_ptr<Simple_Apu> nesApu = mNesApu = make_shared<Simple_Apu>();
      shared_ptr<NesDpcm> nesDpcm = make_shared<NesDpcm>();

      NesApu::InitializeNoteTables(); // TODO: kill this singleton stuff
      NesApu::InitAndReset(nesApu, 44100, NesApu::APU_EXPANSION_VRC6, 0, nullptr);
      nesApu->dmc_reader([](void* nesDpcm_, cpu_addr_t addr) -> int {
        return static_cast<NesDpcm*>(nesDpcm_)->GetSampleForAddress(addr - 0xc000);
      }, nesDpcm.get());

      mNesChannels = make_shared<NesChannels>(
        NesChannelPulse(nesApu,     NesApu::Channel::Pulse1,      NesEnvelopes()),
        NesChannelPulse(nesApu,     NesApu::Channel::Pulse2,      NesEnvelopes()),
        NesChannelTriangle(nesApu,  NesApu::Channel::Triangle,    NesEnvelopes()),
        NesChannelNoise(nesApu,     NesApu::Channel::Noise,       NesEnvelopes()),
        NesChannelDpcm(nesApu,      NesApu::Channel::Dpcm,        nesDpcm),
        NesChannelVrc6Pulse(nesApu, NesApu::Channel::Vrc6Pulse1,  NesEnvelopes()),
        NesChannelVrc6Pulse(nesApu, NesApu::Channel::Vrc6Pulse2,  NesEnvelopes()),
        NesChannelVrc6Saw(nesApu,   NesApu::Channel::Vrc6Saw,     NesEnvelopes())
      );
      SetActiveChannel(NesApu::Channel::Pulse1);

      for (int i = 0; i < mNesChannels->numChannels; i++) {
        auto channelVoice = new NesVoice<T>(nesApu, mNesChannels->allChannels[i]);
        auto channelSynth = new MidiSynth(VoiceAllocator::kPolyModeMono, MidiSynth::kDefaultBlockSize);
        mChannelSynths.emplace_back(channelSynth);
        mChannelSynths[i]->AddVoice(channelVoice, 0);
      }

      // TODO: Paraphonic mode? (one synth with several voices; assign each voice to a different NES channel)

      // TODO: Portamento?
      // mSynth.SetNoteGlideTime(0.5); // portamento
  }

  void SetActiveChannel(NesApu::Channel channel) {
    for (auto ch : mNesChannels->allChannels) {
      if (ch->mChannel == channel) {
        mNesEnvelope1 = &(ch->mEnvs.volume);
        mNesEnvelope2 = &(ch->mEnvs.duty);
        mNesEnvelope3 = &(ch->mEnvs.arp);
        mNesEnvelope4 = &(ch->mEnvs.pitch);

        mNesEnvs = (ch->mEnvs.allEnvs);
        break;
      }
    }
  }


  void SetChannelEnabled(NesApu::Channel channel, bool enabled) {
    mNesApu->enable_channel(channel, enabled);
  }

  void ProcessBlock(T** inputs, T** outputs, int nOutputs, int nFrames, double qnPos = 0., bool transportIsRunning = false, double tempo = 120.)
  {
    // clear outputs
    for(auto i = 0; i < nOutputs; i++)
    {
      memset(outputs[i], 0, nFrames * sizeof(T));
    }

    for (auto &synth : mChannelSynths) {
      synth->ProcessBlock(nullptr, outputs, 0, nOutputs, nFrames);
    }

    while (mNesApu->samples_avail() < nFrames) {
      for (auto channel : mNesChannels->allChannels) {
        channel->UpdateAPU();
      }
      // TODO: this updates the APU state at 60 hz, introducing jitter and up to 16ms latency. acceptable?
      mNesApu->end_frame();
    }

    mNesApu->read_samples(mNesBuffer, nFrames);
    for (int i = 0; i < nFrames; i++) {
      int idx = i;
      T smpl = mNesBuffer[i] / 32767.0;
      outputs[0][idx] += smpl;
      outputs[1][idx] += smpl;
    }
  }

  void Reset(double sampleRate, int blockSize)
  {
    for (auto &synth : mChannelSynths) {
      synth->SetSampleRateAndBlockSize(sampleRate, blockSize);
      synth->Reset();
    }
  }

  void ProcessMidiMsg(const IMidiMsg& msg)
  {
    if (mOmniMode) {
      for (auto synth : mChannelSynths) synth->AddMidiMsgToQueue(msg);
    } else if (msg.Channel() < mNesChannels->numChannels) {
      mChannelSynths[msg.Channel()]->AddMidiMsgToQueue(msg);
    }
  }

  void SetParam(int paramIdx, double value)
  {
    pair<int, int> chParam = ResolveParamToChannelParam(paramIdx);
    int ch = chParam.first;
    int param = chParam.second;

    if (ch > -1) {
      switch (param) {
        case kParamChEnabled:
          SetChannelEnabled(NesApu::Channel(ch), value > 0.5);
          break;
        case kParamChVelSens:
          mNesChannels->allChannels[ch]->SetVelSens(value > 0.5);
          break;
        case kParamChLegato:
          mChannelSynths[ch]->SetLegato(value > 0.5);
          break;
        case kParamChKeyTrack:
          mNesChannels->allChannels[ch]->SetKeyTrack(value > 0.5);
          break;
        default:
          int env = (param - 4) / 4;
          int envParam = (param - 4) % 4;
          auto nesEnv = mNesChannels->allChannels[ch]->mEnvs.allEnvs[env];
          switch(envParam) {
            case kParamEnvLoopPoint:
              nesEnv->SetLoop(value);
              break;
            case kParamEnvRelPoint:
              nesEnv->SetRelease(value);
              break;
            case kParamEnvLength:
              nesEnv->SetLength(value);
              break;
            case kParamEnvSpeedDiv:
              nesEnv->SetSpeedDivider(value);
              break;
            default:
              break;
          }
      }
      return;
    }

    switch (paramIdx) {
      case kParamNoteGlideTime:
        for (auto synth : mChannelSynths) synth->SetNoteGlideTime(value / 1000.);
        break;

      case kParamOmniMode:
        mOmniMode = value > 0.5;
        for (auto synth : mChannelSynths) synth->Reset();
        break;

      default:
        break;
    }
  }
  
public:
  NesEnvelope* mNesEnvelope1;
  NesEnvelope* mNesEnvelope2;
  NesEnvelope* mNesEnvelope3;
  NesEnvelope* mNesEnvelope4;

  array<NesEnvelope*, 4>mNesEnvs;

  shared_ptr<NesChannels> mNesChannels;
  vector<MidiSynth*> mChannelSynths;
  shared_ptr<Simple_Apu> mNesApu;
  int16_t mNesBuffer[32768];
  bool mOmniMode;
};
