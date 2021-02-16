//
//  NesVoice.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/20/20.
//

#pragma once

#include "MidiSynth.h"
#include "NesChannel.h"
#include "NesApu.h"

using namespace iplug;

template<typename T>
class NesVoice : public SynthVoice   {
public:
  NesVoice(shared_ptr<Simple_Apu> nesApu, NesChannel* nesChannel) :
  mNesChannel(nesChannel),
  mNesApu(nesApu) {}

  bool GetBusy() const override
  {
    return true;
    // TODO: look into idling for NES voice. Ensure that MIDI activity turns on envelopes
    // return mNesChannels->pulse1.mEnvs.arp.GetState() != NesEnvelope::ENV_OFF;
  }

  void Trigger(double level, bool isRetrigger) override
  {
    DBGMSG("Trigger mKey %d - level %0.2f\n", mKey, level);

    mNesChannel->Trigger(mKey, level, isRetrigger);
  }

  void Release() override
  {
    mNesChannel->Release();
  }

  void ProcessSamplesAccumulating(T** inputs, T** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
    // inputs to the synthesizer can just fetch a value every block, like this:
    double pitchBend = mInputs[kVoiceControlPitchBend].endValue;
    mNesChannel->SetPitchBend(pitchBend);
  }

public:
  NesChannel* mNesChannel;
  shared_ptr<Simple_Apu> mNesApu;
};

