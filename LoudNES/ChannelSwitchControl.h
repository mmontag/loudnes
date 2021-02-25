//
// Created by Matt Montag on 2/21/21.
//

#ifndef LOUDNES_CHANNELSWITCHCONTROL_H
#define LOUDNES_CHANNELSWITCHCONTROL_H

#include "IControl.h"
#include "IColorPickerControl.h"
#include "IVKeyboardControl.h"
#include "IVMeterControl.h"
#include "IVScopeControl.h"
#include "IVMultiSliderControl.h"
#include "IRTTextControl.h"
#include "IVDisplayControl.h"
#include "ILEDControl.h"
#include "IPopupMenuControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A vector "tab" multi switch control. Click tabs to cycle through states. */
class ChannelSwitchControl : public ISwitchControlBase
  , public IVectorBase
{
public:
  enum class ETabSegment { Start, Mid, End };

  /** Constructs a vector tab switch control, linked to a parameter
   * @param bounds The control's bounds
   * @param paramIdx The parameter index to link this control to
   * @param options An initializer list of CStrings for the button labels to override the parameter display text labels. Supply an empty {} list if you don't want to do that.
   * @param label The IVControl label CString
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons */
  ChannelSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const std::initializer_list<const char*>& options = {}, const char* label = "", const IVStyle & style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);

  /** Constructs a vector tab switch control, with an action function (no parameter)
   * @param bounds The control's bounds
   * @param aF An action function to execute when a button is clicked \see IActionFunction
   * @param options An initializer list of CStrings for the button labels. The size of the list decides the number of buttons.
   * @param label The IVControl label CString
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons */
  ChannelSwitchControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);

  virtual ~ChannelSwitchControl() { mTabLabels.Empty(true); }
  void Draw(IGraphics& g) override;
  void OnInit() override;

  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawButton(IGraphics& g, const IRECT& bounds, bool pressed, bool mouseOver, ETabSegment segment, bool disabled);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverButton = -1; ISwitchControlBase::OnMouseOut(); SetDirty(false); }
  void OnResize() override;
  virtual bool IsHit(float x, float y) const override;

  /** returns the label string on the selected tab */
  const char* GetSelectedLabelStr() const;
protected:

  /** @return the index of the entry at the given point or -1 if no entry was hit */
  virtual int GetButtonForPoint(float x, float y) const;

  int mMouseOverButton = -1;
  WDL_TypedBuf<IRECT> mButtons;
  WDL_PtrList<WDL_String> mTabLabels;
  EDirection mDirection;
};

ChannelSwitchControl::ChannelSwitchControl(const IRECT& bounds, int paramIdx, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
  : ISwitchControlBase(bounds, paramIdx, SplashClickActionFunc, (int) options.size())
  , IVectorBase(style)
  , mDirection(direction)
{
  AttachIControl(this, label);
  mText = style.valueText;
  mText.mAlign = mStyle.valueText.mAlign;
  mText.mVAlign = mStyle.valueText.mVAlign;
  mShape = shape;

  for (auto& option : options)
  {
    mTabLabels.Add(new WDL_String(option));
  }
}

ChannelSwitchControl::ChannelSwitchControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
  : ISwitchControlBase(bounds, kNoParameter, aF, static_cast<int>(options.size()))
  , IVectorBase(style)
  , mDirection(direction)
{
  AttachIControl(this, label);
  mText = style.valueText;
  mText.mAlign = mStyle.valueText.mAlign;
  mText.mVAlign = mStyle.valueText.mVAlign;
  mShape = shape;

  for (auto& option : options)
  {
    mTabLabels.Add(new WDL_String(option));
  }
}

void ChannelSwitchControl::OnInit()
{
  ISwitchControlBase::OnInit();

  const IParam* pParam = GetParam();

  if(pParam && mTabLabels.GetSize() == 0) // don't add param display text based labels if allready added via ctor
  {
    for (int i = 0; i < mNumStates; i++)
    {
      mTabLabels.Add(new WDL_String(GetParam()->GetDisplayText(i)));
    }

    if(!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetName());
  }
}

void ChannelSwitchControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
}

void ChannelSwitchControl::DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment, bool disabled)
{
  DrawPressableShape(g, mShape, r, pressed, mouseOver, disabled);
  if (pressed) {
    EVColor color = mouseOver ? kHL : kPR;
    g.FillTriangle(GetColor(color), r.R - 0.1, r.B, r.R - 0.1, r.T, r.R + 9.f, r.MH());
  }
}

void ChannelSwitchControl::DrawWidget(IGraphics& g)
{
  int selected = GetSelectedIdx();
  ETabSegment segment = ETabSegment::Start;

  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];

    if(i > 0)
      segment = ETabSegment::Mid;

    if(i == mNumStates-1)
      segment = ETabSegment::End;

    DrawButton(g, r.GetPadded(0, -1.f, -9.f, -1.f), i == selected, mMouseOverButton == i, segment, IsDisabled() || GetStateDisabled(i));

    if(mTabLabels.Get(i))
    {
      g.DrawText(mStyle.valueText, mTabLabels.Get(i)->Get(), r.GetPadded(-11, 0, -11, 0), &mBlend);
    }
  }
}

int ChannelSwitchControl::GetButtonForPoint(float x, float y) const
{
  for (int i = 0; i < mNumStates; i++)
  {
    if (mButtons.Get()[i].Contains(x, y))
      return i;
  }

  return -1;
}

bool ChannelSwitchControl::IsHit(float x, float y) const
{
  return GetButtonForPoint(x, y) > -1;
}

void ChannelSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  int index = GetButtonForPoint(x, y);
  if (index > -1)
    SetValue(((double) index * (1./(double) (mNumStates-1))));

  SetDirty(true);
}

void ChannelSwitchControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseOverButton = GetButtonForPoint(x, y);

  ISwitchControlBase::OnMouseOver(x, y, mod);

  SetDirty(false);
}

void ChannelSwitchControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));

  mButtons.Resize(0);

  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mWidgetBounds.SubRect(mDirection, mNumStates, i));
  }

  SetDirty(false);
}

const char* ChannelSwitchControl::GetSelectedLabelStr() const
{
  return mTabLabels.Get(GetSelectedIdx())->Get();
}

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE

#endif //LOUDNES_CHANNELSWITCHCONTROL_H
