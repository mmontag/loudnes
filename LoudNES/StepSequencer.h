//
//  StepSequencer.h
//  LoudNES-macOS
//
//  Created by Matt Montag on 5/27/20.
//

#ifndef StepSequencer_h
#define StepSequencer_h


#include <utility>

#include "IControl.h"

using StepSeqFunc = std::function<void(int stepIdx, float value)>;

constexpr int kNumStepParams = 4; // Four step sequencer parameters: loop, length, release, speed

/** A base class for mult-strip/slider controls, such as multi-sliders, meters */
class StepSequencer : public IControl, public IVectorBase {
public:

  /** Constructs a vector multi slider control that is linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param grain The smallest value increment of the sliders
   * @param lowParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param direction The direction of the sliders
   * @param minSliderValue Defines the minimum value of each slider
   * @param maxSliderValue Defines the maximum value of each slider */
  StepSequencer(const IRECT &bounds, const char *label, int lowParamIdx,
                const IVStyle &style = DEFAULT_STYLE, int maxNSliders = 1,
                float grain = 1.f, StepSeqFunc aF = nullptr)
  : IControl(bounds)
  , IVectorBase(style)
  , mActionFunc(std::move(aF))
  , mGrain(grain)
  , mNumSliders(maxNSliders)
  {
    mSliderPadding = 1.f;

    // Sliders use mNumSliders and mSliderVals, NOT NVals() and mVals[]
    // the latter is the number of parameters, which is always kNumStepParams
    SetNVals(kNumStepParams + mNumSliders);
    for (int i = 0; i < kNumStepParams; i++) {
      SetParamIdx(lowParamIdx + i, i);
    }

//    mSliderVals.resize(mNumSliders);
    for (int i = 0; i < mNumSliders; i++)
    {
//      SetParamIdx(lowParamIdx + i, i); // or kNoParameter
      mSliderBounds.Add(IRECT());
//      mSliderVals[i] = 0.;
    }

    AttachIControl(this, label);
  }

  virtual ~StepSequencer()
  {
  }

  IRECT MakeRects(const IRECT& parent, bool hasHandle = false) {
    IVectorBase::MakeRects(parent, hasHandle);
    mLoopInfoBounds = mWidgetBounds.GetPadded(-3.0f).GetFromTop(4.0f);
    mInsetBounds = mWidgetBounds.GetPadded(-3.0f).GetReducedFromTop(8.0f);

    return mWidgetBounds;
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mWidgetBounds);
    DrawWidget(g);
    DrawLabel(g);

    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  int GetValIdxForPos(float x, float y) const override
  {
    int nVals = mNumSliders; // used to be NVals()

    for (auto v = 0; v < nVals; v++)
    {
      if (mSliderBounds.Get()[v].Contains(x, y))
      {
        return v + kNumStepParams;
      }
    }

    return kNoValIdx;
  }

  virtual void MakeSliderRects(const IRECT& bounds)
  {
    int nVals = mNumSliders;
    int dir = static_cast<int>(mDirection); // 0 = horizontal, 1 = vertical
    for (int ch = 0; ch < nVals; ch++)
    {
      mSliderBounds.Get()[ch] = bounds.SubRect(EDirection(!dir), mNumSliders, ch).
                                     GetPadded(0, -mSliderPadding * (float) dir, -mSliderPadding * (float) !dir, 0);
    }
  }

  void SetLength(int length) {
    mNumSliders = max(1, length);
    MakeSliderRects(mInsetBounds);
    SetSlidersDirty();
    SetDirty(false);
  }

  void SetLoopPoint(int idx) {
    mLoopPoint = idx;
    SetDirty(false);
  }

  void SetReleasePoint(int idx) {
    mReleasePoint = idx;
    SetDirty(false);
  }

  void SetSlidersDirty() {
    if (mLayerSliders) mLayerSliders->Invalidate();
  }

//  int ctr = 0;

  void DrawWidget(IGraphics& g) override
  {
//    if (ctr % 10 == 0)
//      DBGMSG("Drawing StepSequencer %d\n", ctr);
//    ctr++;
    // TODO: index 2 is bound to the length parameter
    // TODO: is this the right place to check for changes to the length parameter?
    if (mNumSliders != GetParam(2)->Int()) {
      SetLength(GetParam(2)->Int());
    }

    if (!g.CheckLayer(mLayerGrid)) {
      g.StartLayer(this, mInsetBounds);

      // Draw grid lines
      int nSteps = ceil(1.f/mGrain);
      IRECT line = mInsetBounds.GetFromTop(1).GetTranslated(0, -0.5f);
      float gapHeight = mInsetBounds.H() / nSteps;
      for (int i = 1; i < nSteps; i++) {
        g.FillRect(COLOR_WHITE.WithOpacity(0.1), line.GetTranslated(0.f, i * gapHeight), &mBlend);;
      }

      mLayerGrid = g.EndLayer();
    }

    if(!g.CheckLayer(mLayerSliders)) {
      g.StartLayer(this, mInsetBounds);

      // Draw sliders
      for (int ch = 0; ch < mNumSliders; ch++) {
        DrawSlider(g, mSliderBounds.Get()[ch], ch);
      }

      mLayerSliders = g.EndLayer();
    }

//    if (!g.CheckLayer(mLayerPlayhead)) {
//      IRECT r = mSliderBounds.Get()[0];
//      g.StartLayer(this, r);
//      g.FillRect(COLOR_RED, r, &BLEND_50);
//      mLayerPlayhead = g.EndLayer();
//    }

    g.DrawBitmap(mLayerGrid->GetBitmap(), mInsetBounds);

//    IRECT r = mSliderBounds.Get()[0];
//    g.DrawBitmap(mLayerPlayhead->GetBitmap(), mInsetBounds.GetHShifted(r.W() * mHighlightIdx));
    g.DrawBitmap(mLayerSliders->GetBitmap(), mInsetBounds);

    g.FillRect(COLOR_WHITE.WithOpacity(0.1), mLoopInfoBounds);
    IRECT loopRect = mLoopInfoBounds.GetFromRight((mNumSliders - GetParam(0)->Int()) * mLoopInfoBounds.W() / mNumSliders);
    IRECT relsRect = mLoopInfoBounds.GetFromRight((mNumSliders - GetParam(1)->Int()) * mLoopInfoBounds.W() / mNumSliders);
    IColor c1 = GetColor(kFG);
    IColor c2 = IColor::LinearInterpolateBetween(c1, COLOR_BLACK, 0.33);
    g.FillRect(c1, loopRect, &mBlend);
    g.FillRect(c2, relsRect, &mBlend);

    if (mHighlightIdx >= 0 && mHighlightIdx < mNumSliders) {
      IRECT r = mSliderBounds.Get()[mHighlightIdx];
      g.FillRect(COLOR_WHITE, r, &BLEND_25);
    }

  }

  void SetHighlightIdx(int chIdx) {
    if (mHighlightIdx != chIdx) {
      mHighlightIdx = chIdx;
      SetDirty(false, clamp(chIdx, 0, NVals() - 1));
    }
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage) {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<1, int> d;
      pos = stream.Get(&d, pos);
      SetHighlightIdx(d.vals[0]);
    }
  }

  
  void SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, double minClip = 0., double maxClip = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = mNumSliders; // NVals();

    float value = 0.;
    int sliderTest = -1;

    if (direction == EDirection::Vertical) {
      value = 1.f - (y - bounds.T) / bounds.H();

      for (auto i = 0; i < nVals; i++) {
        if (mSliderBounds.Get()[i].Contains(x, mSliderBounds.Get()[i].MH())) {
          sliderTest = i;
          break;
        }
      }
    } else {
      value = (x - bounds.L) / bounds.W();

      for (auto i = 0; i < nVals; i++) {
        if (mSliderBounds.Get()[i].Contains(mSliderBounds.Get()[i].MW(), y)) {
          sliderTest = i;
          break;
        }
      }
    }

    value = std::ceil( value / mGrain ) * mGrain;

    if (sliderTest > -1)
    {
      mSliderHit = sliderTest;

      float oldValue = GetSliderValue(sliderTest);
      float newValue = Clip(value, 0.f, 1.f);
      if (newValue != oldValue) {
        mLayerSliders->Invalidate();
        SetSliderValue(newValue, sliderTest);
        OnNewValue(sliderTest, newValue);
        SetDirty(false);
      }

      if (mPrevSliderHit != -1)
      {
        if (abs(mPrevSliderHit - mSliderHit) > 1 /*|| shiftClicked*/)
        {
          int lowBounds, highBounds;

          if (mPrevSliderHit < mSliderHit)
          {
            lowBounds = mPrevSliderHit;
            highBounds = mSliderHit;
          }
          else
          {
            lowBounds = mSliderHit;
            highBounds = mPrevSliderHit;
          }

          for (auto i = lowBounds; i < highBounds; i++)
          {
            double frac = (double)(i - lowBounds) / double(highBounds-lowBounds);

            double oldValue = GetSliderValue(i);
            double newValue = std::ceil(iplug::Lerp(GetSliderValue(lowBounds), GetSliderValue(highBounds), frac) / mGrain) * mGrain;
            if (newValue != oldValue) {
              mLayerSliders->Invalidate();
              SetSliderValue(newValue, i);
              OnNewValue(i, newValue);
              SetDirty(false);
            }
          }
        }
      }
      mPrevSliderHit = mSliderHit;
    }
    else
    {
      mSliderHit = -1;
    }
  }
  
  void SetSliderValue(double value, int sliderIdx) {
    SetValue(value, sliderIdx + kNumStepParams);
  }
  
  double GetSliderValue(int sliderIdx) {
    return GetValue(sliderIdx + kNumStepParams);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (!mod.S)
      mPrevSliderHit = -1;

    SnapToMouse(x, y, mDirection, mInsetBounds);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    SnapToMouse(x, y, mDirection, mInsetBounds);
  }

  //override to do something when an individual slider is dragged
  virtual void OnNewValue(int sliderIdx, double val) {
    if (mActionFunc) {
      mActionFunc(sliderIdx, val);
    }
  }

  virtual void DrawSlider(IGraphics& g, const IRECT& r, int chIdx)
  {
    IRECT fillRect = r.FracRect(mDirection, static_cast<float>(GetSliderValue(chIdx)));
    g.FillRect(GetColor(kFG), fillRect, &mBlend);
  }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    MakeSliderRects(mInsetBounds);
    SetDirty(false);
  }

  void SetActionFunc(const StepSeqFunc &mActionFunc) {
    StepSequencer::mActionFunc = mActionFunc;
  }

protected:
  EDirection mDirection = EDirection::Vertical;
  WDL_TypedBuf<IRECT> mSliderBounds;
  IRECT mInsetBounds;
  IRECT mLoopInfoBounds;
  float mSliderPadding = 0.;
  float mGrain = 0.001f;
  int mHighlightIdx = -1;
  int mLoopPoint = 0;
  int mReleasePoint = 0;
  int mNumSliders = 1;
  StepSeqFunc mActionFunc;
protected:
  ILayerPtr mLayerGrid;
  ILayerPtr mLayerPlayhead;
  ILayerPtr mLayerSliders;

  int mPrevSliderHit = -1;
  int mSliderHit = -1;
};

//BEGIN_IPLUG_NAMESPACE
//BEGIN_IGRAPHICS_NAMESPACE

#endif /* StepSequencer_h */
