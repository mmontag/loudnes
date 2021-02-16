//
//  NesApu.hpp
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/20/20.
//
//  Portions inspired by Mathieu Gauthier's FamiStudio.
//  https://github.com/BleuBleu/FamiStudio/blob/master/FamiStudio/Source/Player/NesApu.cs
//

#ifndef NesApu_hpp
#define NesApu_hpp

#include <stdio.h>
#include "NesSndEmu/Simple_Apu.h"

using namespace std;

class NesApu
{
public:
  static const int APU_SONG       = 0;
  static const int APU_INSTRUMENT = 1;
  static const int APU_WAV_EXPORT = 2;
  static const int APU_COUNT      = 3;

  static const int APU_EXPANSION_NONE    = 0;
  static const int APU_EXPANSION_VRC6    = 1;
  static const int APU_EXPANSION_VRC7    = 2;
  static const int APU_EXPANSION_FDS     = 3;
  static const int APU_EXPANSION_MMC5    = 4;
  static const int APU_EXPANSION_NAMCO   = 5;
  static const int APU_EXPANSION_SUNSOFT = 6;

  static const int APU_PL1_VOL        = 0x4000;
  static const int APU_PL1_SWEEP      = 0x4001;
  static const int APU_PL1_LO         = 0x4002;
  static const int APU_PL1_HI         = 0x4003;
  static const int APU_PL2_VOL        = 0x4004;
  static const int APU_PL2_SWEEP      = 0x4005;
  static const int APU_PL2_LO         = 0x4006;
  static const int APU_PL2_HI         = 0x4007;
  static const int APU_TRI_LINEAR     = 0x4008;
  static const int APU_TRI_LO         = 0x400a;
  static const int APU_TRI_HI         = 0x400b;
  static const int APU_NOISE_VOL      = 0x400c;
  static const int APU_NOISE_LO       = 0x400e;
  static const int APU_NOISE_HI       = 0x400f;
  static const int APU_DMC_FREQ       = 0x4010;
  static const int APU_DMC_RAW        = 0x4011;
  static const int APU_DMC_START      = 0x4012;
  static const int APU_DMC_LEN        = 0x4013;
  static const int APU_SND_CHN        = 0x4015;
  static const int APU_FRAME_CNT      = 0x4017;

  static const int VRC6_CTRL          = 0x9003;
  static const int VRC6_PL1_VOL       = 0x9000;
  static const int VRC6_PL1_LO        = 0x9001;
  static const int VRC6_PL1_HI        = 0x9002;
  static const int VRC6_PL2_VOL       = 0xA000;
  static const int VRC6_PL2_LO        = 0xA001;
  static const int VRC6_PL2_HI        = 0xA002;
  static const int VRC6_SAW_VOL       = 0xB000;
  static const int VRC6_SAW_LO        = 0xB001;
  static const int VRC6_SAW_HI        = 0xB002;

  static const int VRC7_SILENCE       = 0xe000;
  static const int VRC7_REG_SEL       = 0x9010;
  static const int VRC7_REG_WRITE     = 0x9030;

  static const int VRC7_REG_LO_1      = 0x10;
  static const int VRC7_REG_LO_2      = 0x11;
  static const int VRC7_REG_LO_3      = 0x12;
  static const int VRC7_REG_LO_4      = 0x13;
  static const int VRC7_REG_LO_5      = 0x14;
  static const int VRC7_REG_LO_6      = 0x15;
  static const int VRC7_REG_HI_1      = 0x20;
  static const int VRC7_REG_HI_2      = 0x21;
  static const int VRC7_REG_HI_3      = 0x22;
  static const int VRC7_REG_HI_4      = 0x23;
  static const int VRC7_REG_HI_5      = 0x24;
  static const int VRC7_REG_HI_6      = 0x25;
  static const int VRC7_REG_VOL_1     = 0x30;
  static const int VRC7_REG_VOL_2     = 0x31;
  static const int VRC7_REG_VOL_3     = 0x32;
  static const int VRC7_REG_VOL_4     = 0x33;
  static const int VRC7_REG_VOL_5     = 0x34;
  static const int VRC7_REG_VOL_6     = 0x35;

  static const int FDS_WAV_START      = 0x4040;
  static const int FDS_VOL_ENV        = 0x4080;
  static const int FDS_FREQ_LO        = 0x4082;
  static const int FDS_FREQ_HI        = 0x4083;
  static const int FDS_SWEEP_ENV      = 0x4084;
  static const int FDS_SWEEP_BIAS     = 0x4085;
  static const int FDS_MOD_LO         = 0x4086;
  static const int FDS_MOD_HI         = 0x4087;
  static const int FDS_MOD_TABLE      = 0x4088;
  static const int FDS_VOL            = 0x4089;
  static const int FDS_ENV_SPEED      = 0x408A;

  static const int MMC5_PL1_VOL       = 0x5000;
  static const int MMC5_PL1_SWEEP     = 0x5001;
  static const int MMC5_PL1_LO        = 0x5002;
  static const int MMC5_PL1_HI        = 0x5003;
  static const int MMC5_PL2_VOL       = 0x5004;
  static const int MMC5_PL2_SWEEP     = 0x5005;
  static const int MMC5_PL2_LO        = 0x5006;
  static const int MMC5_PL2_HI        = 0x5007;
  static const int MMC5_SND_CHN       = 0x5015;

  static const int N163_SILENCE       = 0xe000;
  static const int N163_ADDR          = 0xf800;
  static const int N163_DATA          = 0x4800;

  static const int N163_REG_FREQ_LO   = 0x78;
  static const int N163_REG_PHASE_LO  = 0x79;
  static const int N163_REG_FREQ_MID  = 0x7a;
  static const int N163_REG_PHASE_MID = 0x7b;
  static const int N163_REG_FREQ_HI   = 0x7c;
  static const int N163_REG_PHASE_HI  = 0x7d;
  static const int N163_REG_WAVE      = 0x7e;
  static const int N163_REG_VOLUME    = 0x7f;

  static const int S5B_ADDR           = 0xc000;
  static const int S5B_DATA           = 0xe000;

  static const int S5B_REG_LO_A       = 0x00;
  static const int S5B_REG_HI_A       = 0x01;
  static const int S5B_REG_LO_B       = 0x02;
  static const int S5B_REG_HI_B       = 0x03;
  static const int S5B_REG_LO_C       = 0x04;
  static const int S5B_REG_HI_C       = 0x05;
  static const int S5B_REG_NOISE      = 0x06;
  static const int S5B_REG_TONE       = 0x07;
  static const int S5B_REG_VOL_A      = 0x08;
  static const int S5B_REG_VOL_B      = 0x09;
  static const int S5B_REG_VOL_C      = 0x0a;
  static const int S5B_REG_ENV_LO     = 0x0b;
  static const int S5B_REG_ENV_HI     = 0x0c;
  static const int S5B_REG_SHAPE      = 0x0d;
  static const int S5B_REG_IO_A       = 0x0e;
  static const int S5B_REG_IO_B       = 0x0f;

  // 2A03 period is 11 bits.
  static const int MaximumPeriod11Bit = 0x7ff;  // 2047
  static const int MaximumPeriod12Bit = 0xfff;  // 4095
  static const int MaximumPeriod15Bit = 0x7fff; // 32767
  static const int MaximumPeriod16Bit = 0xffff; // 65535

  static array<ushort, 97> NoteTableNTSC;
  static array<ushort, 97> NoteTablePAL;
  static array<ushort, 97> NoteTableVrc6Saw;
  static array<ushort, 97> NoteTableVrc7;
  static array<ushort, 97> NoteTableFds;
//  static array<array<ushort, 97>, 8> NoteTableN163;

  enum Channel {
    Pulse1 = 0,
    Pulse2 = 1,
    Triangle = 2,
    Noise = 3,
    Dpcm = 4,
    Vrc6Pulse1 = 5,
    Vrc6Pulse2 = 6,
    Vrc6Saw = 7,
    Vrc7Fm1,
    Vrc7Fm2,
    Vrc7Fm3,
    Vrc7Fm4,
    Vrc7Fm5,
    Vrc7Fm6,
    FdsWave,
    Mmc5Square1,
    Mmc5Square2,
    Mmc5Dpcm,
    N163Wave1,
    N163Wave2,
    N163Wave3,
    N163Wave4,
    N163Wave5,
    N163Wave6,
    N163Wave7,
    N163Wave8,
    S5BSquare1,
    S5BSquare2,
    S5BSquare3,
    Count
  };

  using ushort = unsigned short;

  NesApu() {}

  static void InitializeNoteTables() {
    const double BaseFreq = 32.7032; /// C0

    double clockNtsc = 1789773 / 16.0;
//    double clockPal  = 1662607 / 16.0;

    for (int i = 0; i < 96; ++i) {
      auto octave = i / 12;
      auto freq = BaseFreq * pow(2.0, i / 12.0);

      NoteTableNTSC[i]    = (ushort)(clockNtsc / freq - 0.5);
      printf("%d, %0.4f, %d\n", i, freq, NoteTableNTSC[i]);
//      NoteTablePAL[i]     = (ushort)(clockPal  / freq - 0.5);
      NoteTableVrc6Saw[i] = (ushort)((clockNtsc * 16.0) / (freq * 14.0) - 0.5);
      NoteTableFds[i]     = (ushort)((freq * 65536.0) / (clockNtsc / 1.0) + 0.5);
      NoteTableVrc7[i]    = octave == 0 ? (ushort)(freq * 262144.0 / 49716.0 + 0.5) : (ushort)(NoteTableVrc7[i % 12] << octave);

//            for (int j = 0; j < 8; j++)
//              NoteTableN163[j][i] = (ushort)fmin(0xffff, ((freq * (j + 1) * 983040.0) / clockNtsc) / 4);
    }
  }

  static array<ushort, 97> GetNoteTableForChannel(Channel channel) {
    switch (channel)
    {
      case Channel::Vrc6Saw:
        return NoteTableVrc6Saw;
      case Channel::FdsWave:
        return NoteTableFds;
        //      case Channel::N163Wave1:
        //      case Channel::N163Wave2:
        //      case Channel::N163Wave3:
        //      case Channel::N163Wave4:
        //      case Channel::N163Wave5:
        //      case Channel::N163Wave6:
        //      case Channel::N163Wave7:
        //      case Channel::N163Wave8:
        //        return NoteTableN163[numN163Channels - 1];
      case Channel::Vrc7Fm1:
      case Channel::Vrc7Fm2:
      case Channel::Vrc7Fm3:
      case Channel::Vrc7Fm4:
      case Channel::Vrc7Fm5:
      case Channel::Vrc7Fm6:
        return NoteTableVrc7;
      default:
        return NoteTableNTSC;
        //        return pal ? NoteTablePAL : NoteTableNTSC;
    }
  }

  static ushort GetMaxPeriodForChannel(Channel channel) {
    switch(channel) {
      case Channel::FdsWave:
      case Channel::Vrc6Saw:
      case Channel::Vrc6Pulse1:
      case Channel::Vrc6Pulse2:
        return MaximumPeriod12Bit;
      case Channel::Vrc7Fm1:
      case Channel::Vrc7Fm2:
      case Channel::Vrc7Fm3:
      case Channel::Vrc7Fm4:
      case Channel::Vrc7Fm5:
      case Channel::Vrc7Fm6:
        return MaximumPeriod15Bit;
      case Channel::N163Wave1:
      case Channel::N163Wave2:
      case Channel::N163Wave3:
      case Channel::N163Wave4:
      case Channel::N163Wave5:
      case Channel::N163Wave6:
      case Channel::N163Wave7:
      case Channel::N163Wave8:
        return MaximumPeriod16Bit;
      default:
        return MaximumPeriod11Bit;
    }
  }

  static void InitAndReset(shared_ptr<Simple_Apu> nesApu, int sampleRate, int expansion, int numExpansionChannels, int (*dmcCallback)( void*, cpu_addr_t )) {
    nesApu->sample_rate(sampleRate, false);
    nesApu->set_audio_expansion(expansion);
    if (dmcCallback)
      nesApu->dmc_reader(dmcCallback, NULL);
    nesApu->reset();
    nesApu->write_register(APU_SND_CHN,    0x0f); // enable channels, stop DMC
    nesApu->write_register(APU_TRI_LINEAR, 0x80); // disable triangle length counter
    nesApu->write_register(APU_NOISE_HI,   0x00); // load noise length
    nesApu->write_register(APU_PL1_VOL,    0x30); // volumes to 0
    nesApu->write_register(APU_PL2_VOL,    0x30);
    nesApu->write_register(APU_NOISE_VOL,  0x30);
    nesApu->write_register(APU_PL1_SWEEP,  0x08); // no sweep
    nesApu->write_register(APU_PL2_SWEEP,  0x08);
    // These were the default values in Nes_Snd_Emu, review eventually.
    // FamiTracker by default has -24, 12000 respectively.
    const double treble = -8.87;
    const int    cutoff =  8800;
    nesApu->treble_eq(APU_EXPANSION_NONE, treble, cutoff, sampleRate);

    switch (expansion)
    {
      case APU_EXPANSION_VRC6:
        nesApu->write_register(VRC6_CTRL, 0x00);  // No halt, no octave change
        nesApu->treble_eq(expansion, treble, cutoff, sampleRate);
        break;
      case APU_EXPANSION_FDS:
        // These are taken from FamiTracker. They smooth out the waveform extremely nicely!
        //nesApu->treble_eq(expansion, -48, 1000, sampleRate);
        nesApu->treble_eq(expansion, -15, 2000, sampleRate);
        break;
      case APU_EXPANSION_MMC5:
        nesApu->write_register(MMC5_SND_CHN, 0x03); // Enable both square channels.
        break;
      case APU_EXPANSION_VRC7:
        nesApu->write_register(VRC7_SILENCE, 0x00); // Enable VRC7 audio.
        break;
      case APU_EXPANSION_NAMCO:
        // This is mainly because the instrument player might not update all the channels all the time.
        nesApu->write_register(N163_ADDR, N163_REG_VOLUME);
        nesApu->write_register(N163_DATA, (numExpansionChannels - 1) << 4);
        nesApu->treble_eq(expansion, -15, 4000, sampleRate);
        break;
      case APU_EXPANSION_SUNSOFT:
        nesApu->write_register(S5B_ADDR, S5B_REG_TONE);
        nesApu->write_register(S5B_DATA, 0x38); // No noise, just 3 tones for now.
        break;
    }
  }
};

array<ushort, 97> NesApu::NoteTableNTSC = {};
array<ushort, 97> NesApu::NoteTablePAL = {};
array<ushort, 97> NesApu::NoteTableVrc6Saw = {};
array<ushort, 97> NesApu::NoteTableVrc7 = {};
array<ushort, 97> NesApu::NoteTableFds = {};
//array<array<ushort, 97>, 8> NesApu::NoteTableN163 = {};

#endif /* NesApu_hpp */
