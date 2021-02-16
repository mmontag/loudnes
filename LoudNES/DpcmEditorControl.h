//
//  DpcmEditorControl.h
//  APP
//
//  Created by Matt Montag on 6/10/20.
//

#ifndef DpcmEditorControl_h
#define DpcmEditorControl_h

#include <utility>
#include <fstream>

#include "IControl.h"
#include "NesDpcm.h"
#include "SpinnerControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using namespace std;

IColor kGreen = IColor::FromColorCode(0x6fCf97);
IColor kRed = IColor::FromColorCode(0xc82010);
IColor kBlack = IColor::FromColorCode(0x343434);

class DpcmPatchEditControl : public IControl, IVectorBase {
public:
  DpcmPatchEditControl(const IRECT &bounds, const IVStyle &style, shared_ptr<NesDpcmPatch> patch, vector<shared_ptr<NesDpcmSample>>& samples)
  : IControl(bounds, nullptr), mPatch(patch), mSamples(samples)
  , IVectorBase(style) {
    mSampleMenu.SetFunction([this](IPopupMenu* pMenu) {
      int idx = pMenu->GetChosenItemIdx();
      mPatch->sampleIdx = idx;
      GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });
    });
  }

  void OnAddSampleClick() {
    printf("add sample\n");
    WDL_String path;
    WDL_String filename;
    GetUI()->PromptForFile(filename, path, EFileAction::Open, "dmc");
    if (filename.GetLength() > 0) {
      ifstream input(filename.Get(), ios::binary);
      auto sample = make_shared<NesDpcmSample>(vector<char>(istreambuf_iterator<char>(input), {}),
                                               filename.get_filepart());
      mSamples.push_back(sample);
      mPatch->sampleIdx = (int)mSamples.size() - 1;
    }
  }

  void OnAttached() override {
    float bHeight = 32.f;
    float bMargin = 8.f;
    IRECT box = mRECT.GetFromTop(bHeight);

    // Sample Menu
    IActionFunction menuFunc = [this](IControl *control) {
      mSampleMenu.Clear(false);
      for (const auto& s : mSamples) {
        mSampleMenu.AddItem(s->name.c_str());
      }
      SplashClickActionFunc(control);
      float x, y;
      GetUI()->GetMouseDownPoint(x, y);
      GetUI()->CreatePopupMenu(*control, mSampleMenu, x, y);
    };
    GetUI()->AttachControl(new IVButtonControl(box,
                                               menuFunc,
                                               "[ Choose Sample ]",
                                               mStyle),
                           kNoTag, "DpcmEditor");
    box.Translate(0, box.H() + bMargin);

    // Add Sample Button
    GetUI()->AttachControl(new IVButtonControl(box,
                                               [&](IControl *control) { OnAddSampleClick(); },
                                               "Add Sample...",
                                               mStyle), kNoTag, "DpcmEditor");
    box.Translate(0, box.H() + bMargin);
    box.B += 16;

    // Pitch Spinner
    IActionFunction spinnerFunc = [this](IControl *control) {
      mPatch->pitch = (int) dynamic_cast<SpinnerControl *>(control)->GetRealValue();
      GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });
    };
    GetUI()->AttachControl(mPitch = new SpinnerControl(box,
                                                       kNoParameter,
                                                       spinnerFunc,
                                                       "Pitch",
                                                       mStyle,
                                                       15.f,
                                                       0.f,
                                                       15.f), kNoTag, "DpcmEditor");
    box.Translate(0, box.H() + bMargin);

    // Loop Checkbox
    IActionFunction toggleFunc = [this](IControl *control) {
      mPatch->loop = (bool) control->GetValue();
      GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });
    };
    GetUI()->AttachControl(mLoop = new IVToggleControl(box,
                                                       toggleFunc,
                                                       "Loop",
                                                       mStyle),
                           kNoTag, "DpcmEditor");
  }

  void SetPatch(shared_ptr<NesDpcmPatch> patch) {
    mPatch = patch;
    mPitch->SetValueFromDelegate(patch->pitch);
    mLoop->SetValueFromUserInput(patch->loop);
    GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });
  }

  void Draw(IGraphics &g) override {

  }

  shared_ptr<NesDpcmPatch> mPatch;
protected:
  vector<shared_ptr<NesDpcmSample>>& mSamples;
  IPopupMenu mSampleMenu{"DPCM Sample"};
  SpinnerControl* mPitch;
  IVToggleControl* mLoop;

};

class DpcmPatchRowControl : public IControl, public IVectorBase {
public:
  DpcmPatchRowControl(const IRECT &bounds, const IVStyle &style, shared_ptr<NesDpcmPatch> patch, vector<shared_ptr<NesDpcmSample>>& samples, function<void(DpcmPatchRowControl*)> selectCb)
    : IControl(bounds, nullptr), IVectorBase(style), mPatch(patch), mSamples(samples), mSelectCb(std::move(selectCb)) {
    mDblAsSingleClick = true;
    mRECT.PixelSnap();
  }

  void Draw(IGraphics &g) override {
    if(mIsSelected) g.FillRect(kGreen, mRECT.GetReducedFromBottom(1.f));
    
    char patchStr[256];
    const IColor color = mIsSelected ? kBlack : kGreen;
    if (mPatch) {
      const char *sampleName = mPatch->sampleIdx > -1 ? mSamples[mPatch->sampleIdx]->name.c_str() : "(No sample)";
      sprintf(patchStr, "%s (Pitch %d, Loop %d)", sampleName, mPatch->pitch, mPatch->loop);
      g.DrawText(mStyle.valueText.WithFGColor(color).WithAlign(EAlign::Near).WithVAlign(EVAlign::Middle).WithSize(15.f),
                 patchStr,
                 mRECT.GetReducedFromTop(1.f).GetReducedFromLeft(8.f),
                 &mBlend);
    }

    g.FillRect(kGreen.WithOpacity(0.2f), mRECT.GetFromBottom(1.f));
  }

//  void OnMouseOver(float x, float y, const IMouseMod &mod) override {
//    if (mod.L) {
//      mSelectCb(this);
//    }
//    IControl::OnMouseOver(x, y, mod);
//  }

  void OnMouseDown(float x, float y, const IMouseMod &mod) override {
    mMouseDown = true;
    IControl::OnMouseDown(x, y, mod);
    mSelectCb(this);
  }

  void OnMouseUp(float x, float y, const IMouseMod &mod) override {
    mMouseDown = false;
    IControl::OnMouseUp(x, y, mod);
  }

  bool mIsSelected = false;
  shared_ptr<NesDpcmPatch> mPatch;
protected:
  vector<shared_ptr<NesDpcmSample>>& mSamples;
  function<void(DpcmPatchRowControl*)> mSelectCb;
  bool mMouseDown;
};

class DpcmEditorControl : public IControl, public IVectorBase {
public:
  DpcmEditorControl(const IRECT &bounds, const IVStyle &style, shared_ptr<NesDpcm> nesDpcm)
    : IControl(bounds, nullptr), IVectorBase(style), mNesDpcm(nesDpcm) {
    mDblAsSingleClick = true;
  }

  void OnPatchSelected(DpcmPatchRowControl* row) {
    // render the patch editor with this patch
    mPatchEditor->SetPatch(row->mPatch);

    for (auto r : mPatchRows) {
      r->mIsSelected = r == row;
    }
  }

  void OnAttached() override {
    IRECT box = mRECT.GetReducedFromRight(128.f + 48.f).GetPadded(-2.f).GetFromTop(24.f).GetTranslated(0, mTopMargin);
    int idx = 0;
    for (auto it = mNesDpcm->mNoteMap.rbegin(); it != mNesDpcm->mNoteMap.rend(); ++it) {
      auto patch = *it;
      auto control = new DpcmPatchRowControl(box, mStyle, patch, mNesDpcm->mSamples, [this, patch](DpcmPatchRowControl* c) {
        OnPatchSelected(c);
      });
      GetUI()->AttachControl(control, kNoTag, "DpcmEditor");
      mPatchRows.push_back(control);
      box.Translate(0, 24.f);
      idx++;
    }
    GetUI()->AttachControl(mPatchEditor = new DpcmPatchEditControl(mRECT.GetFromRight(120.f).GetReducedFromBottom(100.f),
                                                                   mStyle,
                                                                   mNesDpcm->mNoteMap.at(0),
                                                                   mNesDpcm->mSamples),
                           kNoTag, "DpcmEditor");
  }

  void Draw(IGraphics &g) override {
    // TODO: reduce redraws of entire DPCM editor
    printf("Drawing DPCM Editor for DPCM at %p\n", mNesDpcm.get());
    auto patch = mPatchEditor->mPatch;
    if (patch) {
      // TODO: consistent naming of all IRECT variables in the project. box/b/r/rect/bounds...?
      IRECT box = mRECT.GetFromBottom(128.f);
      IRECT glossBox = box.GetPadded(-2.f).FracRectVertical(0.5f, true);
      IPattern gloss = IPattern::CreateLinearGradient(glossBox, EDirection::Vertical,
                                                      {
                                                        IColorStop(COLOR_WHITE.WithOpacity(0.26f), 0.f),
                                                        IColorStop(COLOR_WHITE.WithOpacity(0.12f), 0.65f)
                                                      });
      g.FillRoundRect(kBlack, box, 3.f);
      box.Pad(-2.f);
      if (patch->sampleIdx > -1) {
        auto sample = mNesDpcm->mSamples[patch->sampleIdx];
        float x0 = box.L, y0 = box.MH();
        float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        float yStep = box.H() / 64.f; // DPCM range is 64 steps
        float xStep = box.W() / (sample->data.size() * 8.f); // bits
        bool clipped = false;
        for (unsigned char byte : sample->data) {
          for (int i = 0; i < 8; i++) {
            g.DrawLine(clipped ? kRed : kGreen, x0 + x1, y0 + y1 * yStep, x0 + x2, y0 + y2 * yStep, nullptr, 1.f);
            x2 = x1;
            y2 = y1;
            x1 += xStep;
            y1 += (byte >> i & 1) ? -1 : 1;

            if (y1 > 31 || y1 < -31) {
              clipped = true;
              y1 = clamp(y1, -31, 31);
            } else {
              clipped = false;
            }
          }
        }
      }
      g.PathRect(glossBox);
      g.PathFill(gloss);
    }

    // Patch list
    IRECT box = mRECT.GetReducedFromBottom(136.f).GetReducedFromRight(128.f).GetReducedFromTop(mTopMargin);
    IRECT glossBox = box.GetPadded(-2.f).FracRectVertical(0.5f, true);
    IPattern gloss = IPattern::CreateLinearGradient(glossBox, EDirection::Vertical,
                                                    {
                                                      IColorStop(COLOR_WHITE.WithOpacity(0.26f), 0.f),
                                                      IColorStop(COLOR_WHITE.WithOpacity(0.12f), 0.65f)
                                                    });
    g.FillRoundRect(kBlack, box, 3.f);

    box.Pad(-2.f);

    // Piano
    float rowHeight = 24.f;
    IRECT pianoBox = box.GetFromRight(48.f).GetFromTop(rowHeight * 12);
    g.FillRect(IColor::FromColorCode(0xc4c4c4), pianoBox);
    g.DrawRect(kBlack, pianoBox, 0, 2.f);
    g.DrawGrid(kBlack, pianoBox.GetTranslated(0, -2.f), 100, (pianoBox.H() + 8.f) / 7.f, nullptr, 2.f);
    IRECT blackKeyBox = pianoBox.GetFromTop(rowHeight).GetFromLeft(rowHeight);
    for (auto i : {1, 3, 5, 8, 10}) {
      g.FillRect(kBlack, blackKeyBox.GetTranslated(0.f, rowHeight * i));
    }

    g.PathRect(glossBox);
    g.PathFill(gloss);


  }

  void Hide(bool hide) override {
    GetUI()->ForControlInGroup("DpcmEditor", [this, hide](IControl &control) {
      if (&control != this)
        control.Hide(hide);
    });
    IControl::Hide(hide);
  }

  shared_ptr<NesDpcm> mNesDpcm;
  DpcmPatchEditControl *mPatchEditor;
  vector<DpcmPatchRowControl*> mPatchRows;
  float mTopMargin{24.0};
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif /* DpcmEditorControl_h */
