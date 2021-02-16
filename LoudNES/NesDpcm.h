//
//  NesDpcm.h
//  APP
//
//  Created by Matt Montag on 6/9/20.
//

#ifndef NesDpcm_h
#define NesDpcm_h

#include "./resources/dmc/tmnt3hey.h"
#include "./resources/dmc/TinyToon1.h"
#include "./resources/dmc/TinyToon2.h"
#include "./resources/dmc/TinyToon3.h"
#include "./resources/dmc/TinyToon4.h"
#include "./resources/dmc/TinyToon5.h"
#include "./resources/dmc/TinyToon6.h"
#include <map>
#include <utility>

using namespace std;

struct NesDpcmSample {
  string name = "(No Sample)";
  vector<char> data;

  NesDpcmSample(unsigned char *bytes, int length, string name) {
    data.assign(bytes, bytes + length);

    for (auto it = next(data.rbegin()); it != data.rend(); ++it) {
      // Reduce pop at end of DMCs that have been padded with 0x00.
      // Pad with 0x55 to create silence instead.
      if (*it == 0) *it = 0x55; // 01010101
      else break;
    }

    this->name = std::move(name);
  }

  NesDpcmSample(vector<char> bytes, string name) {
    data = std::move(bytes);
    this->name = std::move(name);
  }

  NesDpcmSample() = default;

  void Serialize(iplug::IByteChunk &chunk) {
    // 1
    chunk.PutStr(name.c_str());
    // 2
    unsigned long size = data.size();
    chunk.Put(&size);
    // 3
    chunk.PutBytes(data.data(), (int)data.size() * sizeof(char));
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos) {
    int pos = startPos;
    // 1
    WDL_String nameStr;
    pos = chunk.GetStr(nameStr, pos);
    this->name = nameStr.Get();
    // 2
    unsigned long size = 0;
    pos = chunk.Get(&size, pos);
    // 3
    data.clear();
    for (int i = 0; i < size; i++) {
      char v = 0;
      pos = chunk.Get(&v, pos);
      data.push_back(v);
    }
    return pos;
  }

  int length() const {
    return (int) data.size();
  }
};

struct NesDpcmPatch {
  int pitch = 15;
  bool loop = false;
  int sampleIdx = -1;

  NesDpcmPatch() = default;

  void Serialize(iplug::IByteChunk &chunk) {
    // 1
    chunk.Put(&pitch);
    // 2
    chunk.Put(&loop);
    // 3
    chunk.Put(&sampleIdx);
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos) {
    int pos = startPos;
    // 1
    pos = chunk.Get(&pitch, pos);
    // 2
    pos = chunk.Get(&loop, pos);
    // 3
    pos = chunk.Get(&sampleIdx, pos);
    return pos;
  }

  explicit NesDpcmPatch(int s) : sampleIdx(s) {};
  NesDpcmPatch(int p, int l, int s) : pitch(p), loop(l), sampleIdx(s) {};
};

class NesDpcm {
public:
  NesDpcm() {
    // TODO: decide on bundled DPMC samples
    mSamples.push_back(make_shared<NesDpcmSample>(TMNT3__E300_dmc, TMNT3__E300_dmc_len, "TMNT3 Hey"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C000_dmc, TinyToonA2__C000_dmc_len, "TinyToon 1"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C1C0_dmc, TinyToonA2__C1C0_dmc_len, "TinyToon 2"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C2C0_dmc, TinyToonA2__C2C0_dmc_len, "TinyToon 3"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C340_dmc, TinyToonA2__C340_dmc_len, "TinyToon 4"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C580_dmc, TinyToonA2__C580_dmc_len, "TinyToon 5"));
    mSamples.push_back(make_shared<NesDpcmSample>(TinyToonA2__C740_dmc, TinyToonA2__C740_dmc_len, "TinyToon 6"));
    for (int i = 0; i < 12; i++)
      mNoteMap.push_back(make_shared<NesDpcmPatch>());
  }

  void AddSample(shared_ptr<NesDpcmSample> sample) {
    mSamples.push_back(sample);
  }

  char GetSampleForAddress(int offset) {
    int addr = 0;
    for (const auto &s : mSamples) {
      if (offset >= addr && offset < addr + s->data.size())
        return s->data[offset - addr];
      addr = (addr + s->data.size() + 63) & 0xffc0; // limit to 16384, align to 64 bit
    }
    return 0x55; // 01010101 (dpcm silence)
  }

  int GetAddressForSample(shared_ptr<NesDpcmSample> sample) {
    int addr = 0;
    for (const auto &s : mSamples) {
      if (s == sample) {
        return addr;
      }
      addr = (addr + s->data.size() + 63) & 0xffc0;
    }

    return addr;
  }

  int GetAddressForSample(int sampleIdx) {
    int addr = 0;
    for (int i = 0; i < mSamples.size(); i++) {
      if (i == sampleIdx) return addr;
      addr = (addr + mSamples[i]->data.size() + 63) & 0xffc0;
    }

    return addr;
  }

  void Serialize(iplug::IByteChunk &chunk) {
    // 1
    unsigned long numSamples = mSamples.size();
    chunk.Put(&numSamples);
    // 2
    for (const auto& sample : mSamples) {
      sample->Serialize(chunk);
    }
    // 3
    unsigned long numPatches = mNoteMap.size();
    chunk.Put(&numPatches);
    // 4
    for (const auto& patch : mNoteMap) {
      patch->Serialize(chunk);
    }
    printf("Serializing DPCM %p to chunk %p. numSamples %ld, numPatches %ld\n", this, &chunk, numSamples, numPatches);
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos = 0) {
    int pos = startPos;
    // 1
    unsigned long numSamples = 0;
    pos = chunk.Get(&numSamples, pos);
    // 2
    mSamples.clear();
    for (int i = 0; i < numSamples; i++) {
      auto sample = make_shared<NesDpcmSample>();
      pos = sample->Deserialize(chunk, pos);
      mSamples.push_back(sample);
    }
    // 3
    unsigned long numPatches = 0;
    pos = chunk.Get(&numPatches, pos);
    // 4
    for (int i = 0; i < numPatches; i++) {
      pos = mNoteMap[i]->Deserialize(chunk, pos);
    }
    printf("Deserializing DPCM %p from chunk %p. numSamples %ld, numPatches %ld\n", this, &chunk, numSamples, numPatches);
    return pos;
  }

  shared_ptr<NesDpcmPatch> GetDpcmPatchForNote(int note) {
    if (mNoteMap.empty()) return nullptr;
    return mNoteMap.at(note % mNoteMap.size());
  }

  vector<shared_ptr<NesDpcmSample>> mSamples;
  vector<shared_ptr<NesDpcmPatch>> mNoteMap;
};

#endif /* NesDpcm_h */
