//
//  NesChannelState.h
//  All
//
//  Created by Matt Montag on 5/20/20.
//

#ifndef NesChannelState_h
#define NesChannelState_h

#include "NesApu.h"
#include "NesDpcm.h"
#include "NesEnvelope.h"
#include <algorithm>
#include <utility>

using namespace std;

class NesChannel
{
public:
  NesChannel(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes)
  : mNesApu(std::move(nesApu))
  , mEnvs(nesEnvelopes)
  , mChannel(channel)
  , mNoteTable(NesApu::GetNoteTableForChannel(channel))
  // TODO: pal, numN163Channels
  {}

  virtual int GetPeriod() {
    int arpNote = mEnvs.arp.GetValueAndAdvance();
    int finePitch = mEnvs.pitch.GetValueAndAdvance();

    // Absolute fine pitch mode + note table lookup (becomes ineffective at lower notes)
    //    int basePeriod = mNoteTable[mBaseNote - mNoteTableMidiOffset + arpNote] / (1.f + pow(2.f, finePitch / 72.f));
    //    int basePeriod = mNoteTable[mBaseNote - mNoteTableMidiOffset + arpNote] - finePitch;
    //    int period = clamp(basePeriod / mPitchBendRatio, 8, NesApu::GetMaxPeriodForChannel(mChannel));
    //    return period;

    // Relative fine pitch mode - doesn't use note table
    static const double clockNtsc = 1789773 / 16.0;
    static const double note0Freq = 8.1757989156;

    double note = mBaseNote + arpNote + (finePitch / 12.0);
    double freq = note0Freq * pow(2.0, note / 12.0) * mPitchBendRatio;
    int idealPeriod;
    if (mChannel == NesApu::Vrc6Saw)
      idealPeriod = (ushort)((clockNtsc * 16.0) / (freq * 14.0) - 0.5);
    else if (mChannel == NesApu::Triangle)
      idealPeriod = (ushort)(clockNtsc / (freq * 2.0) - 0.5);
    else
      idealPeriod = (ushort)(clockNtsc / freq - 0.5);

    int period = clamp(idealPeriod, 8, NesApu::GetMaxPeriodForChannel(mChannel));
    return period;
  }

  virtual int GetVolume() {
    int envVolume = mEnvs.volume.GetValueAndAdvance();
    // Simple multiply https://docs.google.com/spreadsheets/d/1i1xJdoUZuDM50SogPGg270OP6oX1rjiDNVBMh6yfQiw/edit#gid=1871770382
    return ceil(envVolume * mVelocity);
  }

  virtual int GetDuty() {
    // 2A03 pulse duty really only has 3 levels:
    // 1/8 (0), 1/4 (1), and 1/2 (2). The last is 1/4 inverted (3).
    return mEnvs.duty.GetValueAndAdvance() % 4;
  }

  // TODO rename something like Advance() or EndFrame()
  virtual void UpdateAPU() {
  }

  virtual void SetPitchBend(float pitchBend) {
    if (mPitchBend != pitchBend) {
      mPitchBendRatio = pow(2.f, pitchBend);
      mPitchBend = pitchBend;
    }
  }

  virtual void Trigger(int baseNote, double velocity, bool isRetrigger = true) {
    mBaseNote = mKeyTrack ? baseNote : 64;
    if (isRetrigger) {
      mVelocity = mVelSens ? velocity : 1.f;
      mEnvs.volume.Trigger();
      mEnvs.arp.Trigger();
      mEnvs.pitch.Trigger();
      mEnvs.duty.Trigger();
    }
  }

  virtual void Release() {
    mEnvs.volume.Release();
    mEnvs.arp.Release();
    mEnvs.pitch.Release();
    mEnvs.duty.Release();
  }

  virtual void SetKeyTrack(bool enabled) {
    mKeyTrack = enabled;
  }

  virtual void SetVelSens(bool enabled) {
    mVelSens = enabled;
  }

  virtual void Serialize(iplug::IByteChunk &chunk) {
    for (auto env : mEnvs.allEnvs) {
      env->Serialize(chunk);
    }
    // TODO: serialize keytrack, vel sens, legato
  }

  virtual int Deserialize(const iplug::IByteChunk &chunk, int startPos) {
    int pos = startPos;
    for (auto env : mEnvs.allEnvs) {
      pos = env->Deserialize(chunk, pos);
    }
    return pos;
  }

  //protected:
  shared_ptr<Simple_Apu> mNesApu;
  NesApu::Channel mChannel;
  array<ushort, 97> mNoteTable;
  int mBaseNote = 48;
  int mNoteTableMidiOffset = 24;
  NesEnvelopes mEnvs;
  float mPitchBendRatio = 1;
  float mPitchBend = 0;
  float mVelocity;
  bool mKeyTrack = true;
  bool mVelSens = true;
};

class NesChannelPulse : public NesChannel
{
public:
  int mRegOffset = 0;
  int mPrevPeriodHi = 1000;

  NesChannelPulse(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes)
  {
    // 0x4000 for Square1, 0x4004 for Square2
    mRegOffset = mChannel * 4;
  }

  void UpdateAPU() override {
    int duty = GetDuty();
    int volume = 0;

    if (mEnvs.arp.GetState() != NesEnvelope::ENV_OFF) {
      int period = GetPeriod();
      volume = GetVolume();

      int periodLo = (period >> 0) & 0xff;
      int periodHi = (period >> 8) & 0x07;
      int deltaHi = periodHi - mPrevPeriodHi;

      if (deltaHi != 0) {
        // TODO: verify sweep is working, and get smoothVibrato from some setting
        bool smoothVibrato = true;
        if (smoothVibrato && abs(deltaHi) == 1) { // originally && !IsSeeking()
          // Blaarg's smooth vibrato technique using the sweep
          // to avoid resetting the phase. Cool stuff.
          // http://forums.nesdev.com/viewtopic.php?t=231

          // reset frame counter in case it was about to clock
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0x40);
          // be sure low 8 bits of timer period are $FF ($00 when negative)
          mNesApu->write_register(NesApu::APU_PL1_LO + mRegOffset, deltaHi < 0 ? 0x00 : 0xff);
          // sweep enabled, shift = 7, set negative flag.
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + mRegOffset, deltaHi < 0 ? 0x8f : 0x87);
          // clock sweep immediately
          mNesApu->write_register(NesApu::APU_FRAME_CNT, 0xc0);
          // disable sweep
          mNesApu->write_register(NesApu::APU_PL1_SWEEP + mRegOffset, 0x08);
        } else {
          mNesApu->write_register(NesApu::APU_PL1_HI + mRegOffset, periodHi);
        }

        mPrevPeriodHi = periodHi;
      }

      mNesApu->write_register(NesApu::APU_PL1_LO + mRegOffset, periodLo);
    }

    // duty is shifted to 2 most-significant bits
    mNesApu->write_register(NesApu::APU_PL1_VOL + mRegOffset, (duty << 6) | 0x30 | volume);
    NesChannel::UpdateAPU();
  }
};

class NesChannelTriangle : public NesChannel
{
public:
  NesChannelTriangle(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes) {
    mNoteTableMidiOffset = 12;
  }

  int GetVolume() override {
    return mEnvs.volume.GetValueAndAdvance() ? 0xff : 0x80;
  }

  void UpdateAPU() override {
    if (mEnvs.volume.GetState() != NesEnvelope::ENV_OFF) {
      int volume = GetVolume();
      int period = GetPeriod();
      int periodLo = (period >> 0) & 0xff;
      int periodHi = (period >> 8) & 0x07;

      mNesApu->write_register(NesApu::APU_TRI_LO, periodLo);
      mNesApu->write_register(NesApu::APU_TRI_HI, periodHi);
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0x80 | volume);
    } else {
      mNesApu->write_register(NesApu::APU_TRI_LINEAR, 0x80);
    }

    NesChannel::UpdateAPU();
  }
};

class NesChannelNoise : public NesChannel
{
public:
  NesChannelNoise(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes) : NesChannel(nesApu, channel, nesEnvelopes) {}

  int GetPeriod() override {
    return (mBaseNote + mEnvs.arp.GetValueAndAdvance()) & 0x0f;
  }

  void UpdateAPU() override {
    if (mEnvs.volume.GetState() != NesEnvelope::ENV_OFF) {
      int volume = GetVolume();
      int duty = GetDuty();
      int period = GetPeriod();

      mNesApu->write_register(NesApu::APU_NOISE_LO, (period  ^ 0x0f) | ((duty << 7) & 0x80));
      mNesApu->write_register(NesApu::APU_NOISE_VOL, 0xf0 | volume);
    } else {
      mNesApu->write_register(NesApu::APU_NOISE_VOL, 0xf0);
    }

    NesChannel::UpdateAPU();
  }
};

class NesChannelDpcm : public NesChannel
{
public:
  NesChannelDpcm(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, shared_ptr<NesDpcm> nesDpcm)
  : NesChannel(nesApu, channel, NesEnvelopes())
  , mNesDpcm(std::move(nesDpcm))
  {}

  void Trigger(int baseNote, double velocity, bool isRetrigger) override {
    mBaseNote = baseNote;
    mDpcmTriggered = true;
  }

  void Release() override {
    mDpcmReleased = true;
  }

  void UpdateAPU() override {
    if (mDpcmTriggered) {
      mDpcmTriggered = false;
      mNesApu->write_register(NesApu::APU_SND_CHN, 0x0f);

      auto patch = mNesDpcm->GetDpcmPatchForNote(mBaseNote);

      if (patch && patch->sampleIdx > -1) {
        auto sample = mNesDpcm->mSamples[patch->sampleIdx];
        /// $4012 AAAA.AAAA
        /// Sample address = %11AAAAAA.AA000000 = $C000 + (A * 64)
        mNesApu->write_register(NesApu::APU_DMC_START, mNesDpcm->GetAddressForSample(sample) / 64); // >> 6
        /// $4013 LLLL.LLLL
        /// Sample length = %0000LLLL.LLLL0001 = (L * 16) + 1 bytes.
        /// Specify the length of the sample in 16 byte increments by writing a value to $4013.
        /// i.e. $01 means 17 bytes length and $02 means 33 bytes.
        /// For an actual length of 513 bytes, write 32 to $4013. (513 - 1)/16 = 32
        mNesApu->write_register(NesApu::APU_DMC_LEN, sample->length() >> 4); // >> 4
        mNesApu->write_register(NesApu::APU_DMC_FREQ, patch->pitch | (patch->loop ? 0x40 /* 0100 0000 */ : 0x00));
        mNesApu->write_register(NesApu::APU_DMC_RAW, 32); // Starting sample
        mNesApu->write_register(NesApu::APU_SND_CHN, 0x1f); // 0001 1111
      }
    }
    if (mDpcmReleased) {
      mDpcmReleased = false;
      mNesApu->write_register(NesApu::APU_SND_CHN, 0x0f);
    }

    NesChannel::UpdateAPU();
  }

  void Serialize(iplug::IByteChunk &chunk) override {
    mNesDpcm->Serialize(chunk);
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos) override {
    return mNesDpcm->Deserialize(chunk, startPos);
  }

  shared_ptr<NesDpcm> mNesDpcm;
protected:
  bool mDpcmTriggered;
  bool mDpcmReleased;
};

class NesChannelVrc6Pulse : public NesChannel
{
public:
  int mRegOffset = 0;

  NesChannelVrc6Pulse(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes)
    : NesChannel(nesApu, channel, nesEnvelopes) {
    mRegOffset = (mChannel - NesApu::Channel::Vrc6Pulse1) * 0x1000;
  }

  virtual int GetDuty() {
    // VRC6 pulse channels duty has 8 levels; 0: 1/16, 1: 2/16,... 7: 8/16
    return mEnvs.duty.GetValueAndAdvance();
  }

  void UpdateAPU() override {
    int duty = GetDuty();

    if (mEnvs.arp.GetState() == NesEnvelope::ENV_OFF) {
      mNesApu->write_register(NesApu::VRC6_PL1_VOL + mRegOffset, duty << 4);
    } else {
      int period = GetPeriod();
      int volume = GetVolume();
      mNesApu->write_register(NesApu::VRC6_PL1_LO  + mRegOffset, ((period >> 0) & 0xff));
      mNesApu->write_register(NesApu::VRC6_PL1_HI  + mRegOffset, ((period >> 8) & 0x0f) | 0x80);
      // duty is shifted to 4 most-significant bits
      mNesApu->write_register(NesApu::VRC6_PL1_VOL + mRegOffset, (duty << 4) | volume);
    }

    NesChannel::UpdateAPU();
  }
};

class NesChannelVrc6Saw : public NesChannel
{
public:
  NesChannelVrc6Saw(shared_ptr<Simple_Apu> nesApu, NesApu::Channel channel, const NesEnvelopes &nesEnvelopes)
    : NesChannel(nesApu, channel, nesEnvelopes) {}

  void UpdateAPU() override {
    if (mEnvs.arp.GetState() == NesEnvelope::ENV_OFF) {
      mNesApu->write_register(NesApu::VRC6_SAW_VOL, 0x00);
    } else {
      int period = GetPeriod();
      int volume = GetVolume();

      mNesApu->write_register(NesApu::VRC6_SAW_LO , ((period >> 0) & 0xff));
      mNesApu->write_register(NesApu::VRC6_SAW_HI , ((period >> 8) & 0x0f) | 0x80);
      mNesApu->write_register(NesApu::VRC6_SAW_VOL, volume << 2);
    }

    NesChannel::UpdateAPU();
  }
};


struct NesChannels {
  NesChannels(NesChannelPulse p1, NesChannelPulse p2, NesChannelTriangle t, NesChannelNoise n,
                       NesChannelDpcm d, NesChannelVrc6Pulse vp1, NesChannelVrc6Pulse vp2, NesChannelVrc6Saw s)
    : pulse1(std::move(p1))
    , pulse2(std::move(p2))
    , triangle(std::move(t))
    , noise(std::move(n))
    , dpcm(std::move(d))
    , vrc6pulse1(vp1)
    , vrc6pulse2(vp2)
    , vrc6saw(s)
    , allChannels({&pulse1, &pulse2, &triangle, &noise, &dpcm, &vrc6pulse1, &vrc6pulse2, &vrc6saw}) {}

  NesChannelPulse     pulse1;
  NesChannelPulse     pulse2;
  NesChannelTriangle  triangle;
  NesChannelNoise     noise;
  NesChannelDpcm      dpcm;
  NesChannelVrc6Pulse vrc6pulse1;
  NesChannelVrc6Pulse vrc6pulse2;
  NesChannelVrc6Saw   vrc6saw;
  const int numChannels = 8;

  vector<NesChannel*> allChannels;
};

#endif /* NesChannelState_h */
