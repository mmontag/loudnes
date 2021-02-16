//
//  NesEnvelope.h
//  ChipSmasher-macOS
//
//  Created by Matt Montag on 5/27/20.
//

#ifndef NesEnvelope_h
#define NesEnvelope_h

#include "ISender.h"
#include "IPlugStructs.h"

const int kMaxSteps = 64;

class NesEnvelope {
public:
  enum State {
    ENV_INITIAL,
    ENV_RELEASE,
    ENV_OFF
  };

  NesEnvelope(int defaultValue, int minVal, int maxVal)
  : mMinVal(minVal)
  , mMaxVal(maxVal) {
    mValues.fill(defaultValue);
  }

  void Trigger() {
    mState = ENV_INITIAL;
    mStep = 0;
  }

  void Release() {
    mStep = mReleasePoint * mSpeedDivider;
    if (mReleasePoint < mLength) {
      mState = ENV_RELEASE;
    } else {
      mState = ENV_OFF;
    }
  }

  int GetValueAndAdvance() {
    int step = mStep / mSpeedDivider;
    mStep++;
    switch (mState) {
      case ENV_INITIAL:
        if (mStep >= mReleasePoint * mSpeedDivider)
          mStep = mLoopPoint * mSpeedDivider;
      // Fall through - no break
      case ENV_RELEASE:
        if (mStep >= mLength * mSpeedDivider)
          mState = ENV_OFF;
        break;
      case ENV_OFF:
        return 0;
    }
    assert(step < mValues.size());
    return mValues.at(step);
  }

  int GetStep() {
    return mState == ENV_OFF ? -1 : mStep / mSpeedDivider;
  }

  State GetState() {
    return mState;
  }

  void SetLength(int length) {
    mLength = clamp(length, 1, kMaxSteps);
    if (mReleasePoint > mLength) mReleasePoint = mLength;
    if (mLoopPoint >= mLength) mLoopPoint = mLength - 1;
  }

  void SetSpeedDivider(int speedDivider) {
    int newSpeedDivider = clamp(speedDivider, 1, 8);
    mStep = min(mStep * (float)newSpeedDivider / mSpeedDivider, kMaxSteps * newSpeedDivider - 1);
    mSpeedDivider = newSpeedDivider;
  }

  void SetLoop(int loopPoint) {
    mLoopPoint = clamp(loopPoint, 0, kMaxSteps - 1);
    if (mReleasePoint <= mLoopPoint) mReleasePoint = mLoopPoint + 1;
    if (mLength <= mLoopPoint) mLength = mLoopPoint + 1;
  }

  void SetRelease(int releasePoint) {
    mReleasePoint = clamp(releasePoint, 1, kMaxSteps);
    if (mLoopPoint >= mReleasePoint) mLoopPoint = mReleasePoint - 1;
    if (mLength < mReleasePoint) mLength = mReleasePoint;
  }

  void Serialize(iplug::IByteChunk &chunk) const {
//    printf("sizeof mValues %d\n", sizeof(mValues));
    chunk.PutBytes(mValues.data(), sizeof(mValues));
  }

  int Deserialize(const iplug::IByteChunk &chunk, int startPos = 0) {
    int pos = startPos;
    printf("Deserializing envelope from chunk %p, size %d, startPos %d\n", &chunk, chunk.Size(), startPos);
    for (int i = 0; i < kMaxSteps; i++) {
      int* addr = &mValues[i];
//      printf("-- Chunk pos: %d, Target address %p\n", startPos + i * 4, addr);
      pos = chunk.Get(addr, pos);
//      pos = chunk.Get(addr, startPos + i * 4);
    }
    return pos;
  }

  array<int, kMaxSteps> mValues = {0};
  int mStep = 0;
  int mLoopPoint = 15;
  int mReleasePoint = 16;
  int mLength = 16;
  int mSpeedDivider = 1;
  int mMinVal = 0;
  int mMaxVal = 15;

protected:
  NesEnvelope::State mState = ENV_OFF;
};

struct NesEnvelopes {
  NesEnvelopes()
    : volume(NesEnvelope(15, 0, 15))
    , duty(NesEnvelope(2, 0, 7))
    , arp(NesEnvelope(0, -12, 12))
    , pitch(NesEnvelope(0, -12, 12))
    , allEnvs({&volume, &duty, &arp, &pitch}) {
    printf("Initialized NesEnvelopes\n");
  }

  NesEnvelopes(const NesEnvelopes& other)
    : volume(other.volume)
    , duty(other.duty)
    , arp(other.arp)
    , pitch(other.pitch)
    , allEnvs({&volume, &duty, &arp, &pitch}) {
    printf("Copied NesEnvelopes\n");
  }

  ~NesEnvelopes() {
    printf("Destroying NesEnvelopes\n");
  }

  NesEnvelope volume;
  NesEnvelope duty;
  NesEnvelope arp;
  NesEnvelope pitch;

  array<NesEnvelope*, 4> allEnvs;
};

#endif /* NesEnvelope_h */
