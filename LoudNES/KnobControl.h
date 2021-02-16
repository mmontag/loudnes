//
// Created by Matt Montag on 6/26/20.
//

#ifndef CHIPSMASHER_KNOBCONTROL_H
#define CHIPSMASHER_KNOBCONTROL_H

#include "IControl.h"

class KnobControl : public IKnobControlBase, public IVectorBase {
public:
  KnobControl(const IRECT &bounds, int paramIdx, const char *label = "", const IVStyle &style = DEFAULT_STYLE,
              bool valueIsEditable = false, bool valueInWidget = false, float a1 = -135.f, float a2 = 135.f,
              float aAnchor = -135.f, EDirection direction = EDirection::Vertical, double gearing = 4.f,
              float trackSize = 2.f) : IKnobControlBase(bounds, paramIdx, direction, gearing),
                                    IVectorBase(style, false, valueInWidget), mAngle1(a1), mAngle2(a2),
                                    mAnchorAngle(aAnchor) {
    DisablePrompt(!valueIsEditable);
    mText = style.valueText;
    mHideCursorOnDrag = mStyle.hideCursor;
    mShape = EVShape::Ellipse;
    mTrackSize = trackSize;
    AttachIControl(this, label);
  }

  KnobControl(const IRECT &bounds, IActionFunction aF, const char *label = "", const IVStyle &style = DEFAULT_STYLE,
              bool valueIsEditable = false, bool valueInWidget = false, float a1 = -135.f, float a2 = 135.f,
              float aAnchor = -135.f, EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING,
              float trackSize = 2.f) : IKnobControlBase(bounds, kNoParameter, direction, gearing),
                                    IVectorBase(style, false, valueInWidget), mAngle1(a1), mAngle2(a2),
                                    mAnchorAngle(aAnchor) {
    DisablePrompt(!valueIsEditable);
    mText = style.valueText;
    mHideCursorOnDrag = mStyle.hideCursor;
    mShape = EVShape::Ellipse;
    mTrackSize = trackSize;
    SetActionFunction(aF);
    AttachIControl(this, label);
  }

  IRECT MakeRects(const IRECT& parent, bool hasHandle = false) {
    mLabelBounds = parent.GetFromBottom(16.f).GetTranslated(0, -16.f);
    mValueBounds = parent.GetFromBottom(16.f);
    mWidgetBounds = parent.GetReducedFromBottom(32.f);
    IRECT clickableArea = mWidgetBounds;
    return clickableArea;
  }

  void Draw(IGraphics &g) {
    DrawBackground(g, mRECT);
    DrawLabel(g);
    DrawWidget(g);
    DrawValue(g, mValueMouseOver);
  }

  IRECT GetKnobDragBounds() {
    IRECT r;

    if (mWidgetBounds.W() > mWidgetBounds.H())
      r = mWidgetBounds.GetCentredInside(mWidgetBounds.H() / 2.f, mWidgetBounds.H());
    else
      r = mWidgetBounds.GetCentredInside(mWidgetBounds.W(), mWidgetBounds.W() / 2.f);

    return r;
  }

  void DrawWidget(IGraphics &g) {
    float widgetRadius; // The radius out to the indicator track arc

    if (mWidgetBounds.W() > mWidgetBounds.H())
      widgetRadius = (mWidgetBounds.H() / 2.f);
    else
      widgetRadius = (mWidgetBounds.W() / 2.f);

    const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH();

    widgetRadius -= (mTrackSize / 2.f);

    IRECT knobHandleBounds = mWidgetBounds.GetCentredInside((widgetRadius - mTrackToHandleDistance) * 2.f);
    const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
    DrawIndicatorTrack(g, angle, cx, cy, widgetRadius);
    DrawPressableShape(g, /*mShape*/ EVShape::Ellipse, knobHandleBounds, mMouseDown, mMouseIsOver, IsDisabled());
    DrawPointer(g, angle, cx, cy, knobHandleBounds.W() / 2.f);
  }

  void DrawIndicatorTrack(IGraphics &g, float angle, float cx, float cy, float radius) {
    if (mTrackSize > 0.f) {
      g.DrawArc(GetColor(kX1),
                cx,
                cy,
                radius,
                angle >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle - angle),
                angle >= mAnchorAngle ? angle : mAnchorAngle,
                &mBlend,
                mTrackSize);
    }
  }

  void DrawPointer(IGraphics &g, float angle, float cx, float cy, float radius) {
    g.DrawRadialLine(GetColor(kFR),
                     cx,
                     cy,
                     angle,
                     mInnerPointerFrac * radius,
                     mOuterPointerFrac * radius,
                     &mBlend,
                     mPointerThickness);
  }

  void OnMouseDown(float x, float y, const IMouseMod &mod) {
    if (mStyle.showValue && mValueBounds.Contains(x, y)) {
      PromptUserInput(mValueBounds);
    } else {
      IKnobControlBase::OnMouseDown(x, y, mod);
    }

    SetDirty(false, kNoValIdx);
  }

  void OnMouseUp(float x, float y, const IMouseMod &mod) {
    IKnobControlBase::OnMouseUp(x, y, mod);
    SetDirty(true, kNoValIdx);
  }

  void OnMouseOver(float x, float y, const IMouseMod &mod) {
    if (mStyle.showValue && !mDisablePrompt)
      mValueMouseOver = mValueBounds.Contains(x, y);

    IKnobControlBase::OnMouseOver(x, y, mod);

    SetDirty(false, kNoValIdx);
  }

  void OnResize() {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false, kNoValIdx);
  }

  bool IsHit(float x, float y) const {
    if (!mDisablePrompt) {
      if (mValueBounds.Contains(x, y))
        return true;
    }

    return mWidgetBounds.Contains(x, y);
  }

  void SetDirty(bool push, int valIdx) {
    IKnobControlBase::SetDirty(push);

    const IParam *pParam = GetParam();

    if (pParam)
      pParam->GetDisplayWithLabel(mValueStr);
  }

  void OnInit() {
    const IParam *pParam = GetParam();

    if (pParam) {
      pParam->GetDisplayWithLabel(mValueStr);

      if (!mLabelStr.GetLength())
        mLabelStr.Set(pParam->GetName());
    }
  }

  void OnMouseOut() override {
    mValueMouseOver = false;
    IKnobControlBase::OnMouseOut();
  }

  void SetInnerPointerFrac(float frac) { mInnerPointerFrac = frac; }

  void SetOuterPointerFrac(float frac) { mOuterPointerFrac = frac; }

  void SetPointerThickness(float thickness) { mPointerThickness = thickness; }

protected:

  float mTrackToHandleDistance = 4.f;
  float mInnerPointerFrac = 0.1f;
  float mOuterPointerFrac = 1.f;
  float mPointerThickness = 2.5f;
  float mAngle1, mAngle2;
  float mAnchorAngle; // for bipolar arc
  bool mValueMouseOver = false;

};

#endif //CHIPSMASHER_KNOBCONTROL_H
