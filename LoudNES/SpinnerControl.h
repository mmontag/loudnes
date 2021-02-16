/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc SpinnerControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class ChevronButton : public IVButtonControl {
  using IVButtonControl::IVButtonControl;
public:
  ChevronButton(const IRECT& bounds, IActionFunction aF, const IVStyle& style, bool isUp)
  : IVButtonControl(bounds, aF, "", style, false, false), mUp(isUp) {}

  void DrawWidget(IGraphics &g) override {
//    DrawBackGround(g, mRECT); // TODO: the background doesn't show up
    bool pressed = (bool)GetValue();
    IColor kButtonBg = IColor::FromColorCode(0xd0d0d0);
    IColor kButtonPressedBg = IColor::FromColorCode(0xc0c0c0);
    if (pressed) {
      g.FillRect(kButtonPressedBg, mRECT);
    } else {
      g.FillRect(kButtonBg, mRECT);
      g.FillRect(COLOR_WHITE.WithOpacity(0.67), mRECT.GetFromTop(1.f));
      g.FillRect(COLOR_BLACK.WithOpacity(0.20), mRECT.GetFromBottom(1.f));
    }
    IRECT chev = mRECT.GetCentredInside(9.f, 4.5f);
    if (mUp)
      g.FillTriangle(mStyle.valueText.mFGColor, chev.L, chev.B, chev.R, chev.B, chev.MW(), chev.T);
    else
      g.FillTriangle(mStyle.valueText.mFGColor, chev.L, chev.T, chev.R, chev.T, chev.MW(), chev.B);
  }
protected:
  bool mUp;
};

class GlossyLabel : public ITextControl, public IVectorBase {
public:
  GlossyLabel(IRECT bounds, char *label, IVStyle &style) : ITextControl(bounds, label), IVectorBase(style) {
    mText = style.valueText;
    AttachIControl(this, label);
  }

  void Draw(IGraphics &g) override {

    IColor kBlack = IColor::FromColorCode(0x343434);
    IColor kText = IColor::FromColorCode(0xd0d0d0);
//    DrawBackGround(g, mRECT);
    g.FillRect(kBlack, mRECT);

    if (mStr.GetLength()) {
      g.DrawText(mText.WithFGColor(kText), mStr.Get(), mRECT, &mBlend);
    }

    IRECT glossBox = mRECT.GetPadded(-2.f).FracRectVertical(0.5f, true);
    IPattern gloss = IPattern::CreateLinearGradient(glossBox, EDirection::Vertical,
                                                    {
                                                      IColorStop(COLOR_WHITE.WithOpacity(0.26f), 0.f),
                                                      IColorStop(COLOR_WHITE.WithOpacity(0.12f), 0.65f)
                                                    });
    g.PathRect(glossBox);
    g.PathFill(gloss);
  }
};

/** A "meta control" for a number box with an Inc/Dec button
 * It adds several child buttons 
 * @ingroup IControls */
class SpinnerControl : public IControl
                         , public IVectorBase
{
public:
  SpinnerControl(const IRECT& bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr, const char* label = "", const IVStyle& style = DEFAULT_STYLE, double defaultValue = 50.f, double minValue = 1.f, double maxValue = 100.f, const char* fmtStr = "%0.0f")
  : IControl(bounds, paramIdx, actionFunc)
  , IVectorBase(style.WithDrawShadows(false)
                .WithValueText(style.valueText.WithVAlign(EVAlign::Middle)))
  , mFmtStr(fmtStr)
  , mMinValue(minValue)
  , mMaxValue(maxValue)
  , mRealValue(defaultValue)
  {
    assert(defaultValue >= minValue && defaultValue <= maxValue);
    
    AttachIControl(this, label);
  }
   
  void OnInit() override
  {
    if(GetParam())
    {
      mMinValue = GetParam()->GetMin();
      mMaxValue = GetParam()->GetMax();
      mRealValue = GetParam()->GetDefault();
    }
  }
  
  void Draw(IGraphics& g) override
  {
    DrawLabel(g);
    
    if(mMouseIsOver)
      g.FillRect(GetColor(kHL), mTextReadout->GetRECT());
  }
  
  void OnResize() override
  {
    MakeRects(mRECT, false);
    
    if(mIncButton && mDecButton)
    {
      IRECT sections = mWidgetBounds;
      mTextReadout->SetTargetAndDrawRECTs(sections.ReduceFromLeft(sections.W() * 0.75f));
      mIncButton->SetTargetAndDrawRECTs(sections.FracRectVertical(0.5f, true));
      mDecButton->SetTargetAndDrawRECTs(sections.FracRectVertical(0.5f, false));
      SetTargetRECT(mTextReadout->GetRECT());
    }
  }
  
  void OnAttached() override
  {
    IRECT sections = mWidgetBounds;
    GetUI()->AttachControl(mTextReadout = new GlossyLabel(sections.ReduceFromLeft(sections.W() * 0.75f), "0", mStyle));
    
    mTextReadout->SetStrFmt(32, mFmtStr.Get(), mRealValue);

    GetUI()->AttachControl(mIncButton = new ChevronButton(sections.FracRectVertical(0.5f, true), mIncrementFunc, mStyle, true));
    GetUI()->AttachControl(mDecButton = new ChevronButton(sections.FracRectVertical(0.5f, false), mDecrementFunc, mStyle, false));
  }
  
  void OnMouseDown(float x, float y, const IMouseMod &mod) override
  {
    if (mHideCursorOnDrag)
      GetUI()->HideMouseCursor(true, true);

    if(GetParam())
      GetDelegate()->BeginInformHostOfParamChangeFromUI(GetParamIdx());
  }
  
  void OnMouseUp(float x, float y, const IMouseMod &mod) override
  {
    if (mHideCursorOnDrag)
      GetUI()->HideMouseCursor(false);
    
    if(GetParam())
      GetDelegate()->EndInformHostOfParamChangeFromUI(GetParamIdx());
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod) override
  {
    double gearing = IsFineControl(mod, true) ? mSmallIncrement : mLargeIncrement;
    mRealValue -= (double(dY) * gearing);
    OnValueChanged();
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod &mod) override
  {
    if(mTextReadout->GetRECT().Contains(x, y))
      GetUI()->CreateTextEntry(*this, mText, mTextReadout->GetRECT(), mTextReadout->GetStr());
  }
  
  void OnTextEntryCompletion(const char* str, int valIdx) override
  {
    mRealValue = atof(str);
    OnValueChanged();
  }
  
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    double gearing = IsFineControl(mod, true) ? mSmallIncrement : mLargeIncrement;
    double inc = (d > 0.f ? 1. : -1.) * gearing;
    mRealValue += inc;
    OnValueChanged();
  }

  void SetValueFromDelegate(double value, int valIdx = 0) override
  {
    if(GetParam())
    {
      mRealValue = GetParam()->FromNormalized(value);
    } else {
      mRealValue = value;
    }
    OnValueChanged(true);

    IControl::SetValueFromDelegate(value, valIdx);
  }

  void SetValueFromUserInput(double value, int valIdx = 0) override
  {
    if(GetParam())
    {
      mRealValue = GetParam()->FromNormalized(value);
    } else {
      mRealValue = value;
    }
    OnValueChanged(true);

    IControl::SetValueFromUserInput(value, valIdx);
  }

  void Hide(bool hide) override
  {
    IControl::Hide(hide);
    mTextReadout->Hide(hide);
    mIncButton->Hide(hide);
    mDecButton->Hide(hide);
  }
  
    void SetStyle(const IVStyle& style) override
  {
    IVectorBase::SetStyle(style);
    mTextReadout->SetStyle(style);
    mIncButton->SetStyle(style);
    mDecButton->SetStyle(style);
  }
  
  bool IsFineControl(const IMouseMod& mod, bool wheel) const
  {
    #ifdef PROTOOLS
    #ifdef OS_WIN
      return mod.C;
    #else
      return wheel ? mod.C : mod.R;
    #endif
    #else
      return (mod.C || mod.S);
    #endif
  }
  
  void OnValueChanged(bool preventAction = false)
  {
    mRealValue = Clip(mRealValue, mMinValue, mMaxValue);
    
    mTextReadout->SetStrFmt(32, mFmtStr.Get(), mRealValue);
    
    if(!preventAction && GetParam())
      SetValue(GetParam()->ToNormalized(mRealValue));
    
    SetDirty(!preventAction);
  }
  
  double GetRealValue() const { return mRealValue; }
  
protected:
  
  IActionFunction mIncrementFunc = [this](IControl* pCaller) { mRealValue += mLargeIncrement; OnValueChanged(); };
  IActionFunction mDecrementFunc = [this](IControl* pCaller) { mRealValue -= mLargeIncrement; OnValueChanged(); };
  GlossyLabel* mTextReadout = nullptr;
  IVButtonControl* mIncButton = nullptr;
  IVButtonControl* mDecButton = nullptr;
  WDL_String mFmtStr;
  double mLargeIncrement = 1.f;
  double mSmallIncrement = 0.1f;
  double mMinValue;
  double mMaxValue;
  double mRealValue = 0.f;
  bool mHideCursorOnDrag = true;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
