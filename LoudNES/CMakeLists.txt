# Note: Not a full-functioning CMake project. Just used to enable CLion project awareness.

cmake_minimum_required(VERSION 3.10)
project(LoudNES)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(NesSndEmu)
include_directories(NesSndEmu/boost)
include_directories(NesSndEmu/nes_apu)
include_directories(resources)
include_directories(../iPlug2/IPlug)
include_directories(../iPlug2/IPlug/APP)
include_directories(../iPlug2/IPlug/Extras)
include_directories(../iPlug2/IPlug/Extras/Synth)
include_directories(../iPlug2/IGraphics)
include_directories(../iPlug2/IGraphics/Controls)
include_directories(../iPlug2/WDL)

add_definitions(-DIPLUG_EDITOR -DIPLUG_DSP -DAPP_API)

add_executable(ChipSmasher
        NesSndEmu/boost/config.hpp
        NesSndEmu/boost/cstdint.hpp
        NesSndEmu/boost/static_assert.hpp
        NesSndEmu/nes_apu/apu_snapshot.cpp
        NesSndEmu/nes_apu/apu_snapshot.h
        NesSndEmu/nes_apu/blargg_common.h
        NesSndEmu/nes_apu/blargg_source.h
        NesSndEmu/nes_apu/Blip_Buffer.cpp
        NesSndEmu/nes_apu/Blip_Buffer.h
        NesSndEmu/nes_apu/Blip_Synth.h
        NesSndEmu/nes_apu/emu2149.c
        NesSndEmu/nes_apu/emu2149.h
        NesSndEmu/nes_apu/emu2413.c
        NesSndEmu/nes_apu/emu2413.h
        NesSndEmu/nes_apu/Multi_Buffer.cpp
        NesSndEmu/nes_apu/Multi_Buffer.h
        NesSndEmu/nes_apu/Nes_Apu.cpp
        NesSndEmu/nes_apu/Nes_Apu.h
        NesSndEmu/nes_apu/Nes_Fds.cpp
        NesSndEmu/nes_apu/Nes_Fds.h
        NesSndEmu/nes_apu/Nes_Mmc5.cpp
        NesSndEmu/nes_apu/Nes_Mmc5.h
        NesSndEmu/nes_apu/Nes_Namco.cpp
        NesSndEmu/nes_apu/Nes_Namco.h
        NesSndEmu/nes_apu/Nes_Oscs.cpp
        NesSndEmu/nes_apu/Nes_Oscs.h
        NesSndEmu/nes_apu/Nes_Sunsoft.cpp
        NesSndEmu/nes_apu/Nes_Sunsoft.h
        NesSndEmu/nes_apu/Nes_Vrc6.cpp
        NesSndEmu/nes_apu/Nes_Vrc6.h
        NesSndEmu/nes_apu/Nes_Vrc7.cpp
        NesSndEmu/nes_apu/Nes_Vrc7.h
        NesSndEmu/nes_apu/Nonlinear_Buffer.cpp
        NesSndEmu/nes_apu/Nonlinear_Buffer.h
        NesSndEmu/DllWrapper.cpp
        NesSndEmu/Simple_Apu.cpp
        NesSndEmu/Simple_Apu.h
        resources/AUv3Framework.h
        resources/resource.h
        LoudNES.cpp
        LoudNES.h
        LoudNES_DSP.h
        config.h
        DpcmEditorControl.h
        NesApu.h
        NesChannel.h
        NesDpcm.h
        NesEnvelope.h
        NesVoice.h
        StepSequencer.h
        KnobControl.h)
