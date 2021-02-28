#include "LoudNES.h"
#include "IPlug_include_in_plug_src.h"
#include "StepSequencer.h"
#include "DpcmEditorControl.h"
#include "KnobControl.h"
#include "ChannelSwitchControl.h"


LoudNES::LoudNES(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)->InitMilliseconds("Note Glide Time", 0., 0.0, 30.);
  GetParam(kParamOmniMode)->InitBool("Omni Mode Enabled", true);

  char const* channelStrs[8] = {"Pulse 1", "Pulse 2", "Triangle", "Noise", "DPCM", "VRC6 Pulse 1", "VRC6 Pulse 2", "VRC6 Saw"};
  for (int i = 0; i < 8; i++) {
    string chStr = channelStrs[i];
    GetParam(ParamFromCh(i, kParamChEnabled    ))->InitBool((chStr + " Enabled"      ).c_str(), false);
    GetParam(ParamFromCh(i, kParamChKeyTrack   ))->InitBool((chStr + " Key Track"    ).c_str(), true);
    GetParam(ParamFromCh(i, kParamChVelSens    ))->InitBool((chStr + " Vel Sens"     ).c_str(), true);
    GetParam(ParamFromCh(i, kParamChLegato     ))->InitBool((chStr + " Legato"       ).c_str(), false);
    GetParam(ParamFromCh(i, kParamEnv1LoopPoint))->InitInt ((chStr + " Env 1 Loop"   ).c_str(), 15, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv1RelPoint ))->InitInt ((chStr + " Env 1 Release").c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv1Length   ))->InitInt ((chStr + " Env 1 Length" ).c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv1SpeedDiv ))->InitInt ((chStr + " Env 1 Speed"  ).c_str(), 1, 1, 8,   "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv2LoopPoint))->InitInt ((chStr + " Env 2 Loop"   ).c_str(), 15, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv2RelPoint ))->InitInt ((chStr + " Env 2 Release").c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv2Length   ))->InitInt ((chStr + " Env 2 Length" ).c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv2SpeedDiv ))->InitInt ((chStr + " Env 2 Speed"  ).c_str(), 1, 1, 8,   "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv3LoopPoint))->InitInt ((chStr + " Env 3 Loop"   ).c_str(), 15, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv3RelPoint ))->InitInt ((chStr + " Env 3 Release").c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv3Length   ))->InitInt ((chStr + " Env 3 Length" ).c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv3SpeedDiv ))->InitInt ((chStr + " Env 3 Speed"  ).c_str(), 1, 1, 8,   "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv4LoopPoint))->InitInt ((chStr + " Env 4 Loop"   ).c_str(), 15, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv4RelPoint ))->InitInt ((chStr + " Env 4 Release").c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv4Length   ))->InitInt ((chStr + " Env 4 Length" ).c_str(), 16, 0, 64, "", IParam::kFlagStepped);
    GetParam(ParamFromCh(i, kParamEnv4SpeedDiv ))->InitInt ((chStr + " Env 4 Speed"  ).c_str(), 1, 1, 8,   "", IParam::kFlagStepped);
   }

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
//    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableMultiTouch(true);

#ifdef OS_WEB
    pGraphics->AttachPopupMenuControl();
#endif

    const IVStyle style {
      true, // Show label
      true, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        COLOR_BLACK, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(11.f, DEFAULT_TEXT_FGCOLOR, "Univers", EAlign::Near, EVAlign::Middle), // Label text
      IText(15.f, DEFAULT_TEXT_FGCOLOR, "Normal", EAlign::Center, EVAlign::Middle), // Value text
      false, // Hide mouse
      true,  // Show frame
      false, // Show shadows
      DEFAULT_EMBOSS,
      DEFAULT_ROUNDNESS,
      DEFAULT_FRAME_THICKNESS,
      DEFAULT_SHADOW_OFFSET,
      DEFAULT_WIDGET_FRAC
    };

    const IVStyle channelButtonStyle {
      true, // Show label
      true, // Show value
      {
        COLOR_TRANSPARENT, // Background
        IColor::FromColorCode(0x181818), // Foreground
        IColor::FromColorCode(0x181818), // Pressed
        IColor::FromColorCode(0x181818), // Frame
        IColor::FromColorCode(0x282828), // Highlight
        DEFAULT_SHCOLOR, // Shadow
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(11.f, IColor::FromColorCode(0xCECECE), "Univers", EAlign::Near, EVAlign::Middle), // Label text
      IText(15.f, IColor::FromColorCode(0xCECECE), "Bold", EAlign::Near, EVAlign::Middle), // Value text
      false, // Hide mouse
      false,  // Show frame
      false, // Show shadows
      DEFAULT_EMBOSS,
      DEFAULT_ROUNDNESS,
      DEFAULT_FRAME_THICKNESS,
      DEFAULT_SHADOW_OFFSET,
      DEFAULT_WIDGET_FRAC
    };

    const IVStyle noLabelStyle {
      false, // Show label
      false, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        COLOR_BLACK, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        COLOR_BLACK, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(12.f, EAlign::Center), // Label text
      DEFAULT_VALUE_TEXT,
      false, // Hide mouse
      true,  // Show frame
      false  // Show shadows
    };

//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("Univers", UNIVERS_FN);
    pGraphics->LoadFont("Normal", NORMAL_FN);
    pGraphics->LoadFont("Bold", BOLD_FN);
    const ISVG switchOnSvg = pGraphics->LoadSVG(SWITCH_ON_FN);
    const ISVG switchOffSvg = pGraphics->LoadSVG(SWITCH_OFF_FN);
    const ISVG channelOnSvg = pGraphics->LoadSVG(CHANNEL_ON_FN);
    const ISVG channelOffSvg = pGraphics->LoadSVG(CHANNEL_OFF_FN);
    pGraphics->EnableTooltips(true);

    IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(
      new IPanelControl(b, IPattern::CreateLinearGradient(0, 0, b.W(), b.H(),
                                                          {
                                                            IColorStop(IColor::FromColorCodeStr("#C0C0C0"), 0),
                                                            IColorStop(IColor::FromColorCodeStr("#828282"), 1)
                                                          }))
    );
    b = pGraphics->GetBounds().GetPadded(-PLUG_PADDING);

#pragma mark - Keyboard

    IRECT keyboardBounds = b.GetFromBottom(120);
    IRECT wheelsBounds = keyboardBounds.ReduceFromLeft(100.f);
    pGraphics->AttachControl(new IVKeyboardControl(keyboardBounds, 24, 96, false), kCtrlTagKeyboard);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5)), kCtrlTagBender);
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectHorizontal(0.5, true),
                                               IMidiMsg::EControlChangeMsg::kModWheel), kCtrlTagModWheel);
    pGraphics->SetQwertyMidiKeyHandlerFunc([pGraphics](const IMidiMsg &msg) {
      dynamic_cast<IVKeyboardControl *>(
        pGraphics->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(
        msg.NoteNumber(), msg.StatusMsg() == IMidiMsg::kNoteOn);
    });

#pragma mark - Channel Panel

    b = b.GetReducedFromBottom(137);
    IRECT channelPanel = b.GetFromLeft(120);
    IRECT channelButtonRect = channelPanel.GetFromTop(34.f);

    pGraphics->AttachControl(new ChannelSwitchControl(channelPanel.GetFromTop(channelButtonRect.H() * 8), [this](IControl* pCaller){
      const int ch = static_cast<IVTabSwitchControl*>(pCaller)->GetSelectedIdx();
      bool isDpcm = ch == NesApu::Channel::Dpcm;
      GetUI()->GetControlWithTag(kCtrlTagDpcmEditor)->Hide(!isDpcm);
      GetUI()->ForControlInGroup("StepSequencers", [=](IControl& control) {
        control.Hide(isDpcm);
      });
      GetUI()->ForControlInGroup("Knobs", [=](IControl& control) {
        control.Hide(isDpcm);
      });

      // Reassign channel-specific toggles (Key track, Velocity sensitivity, Legato)
      GetUI()->GetControlWithTag(kCtrlTagKeyTrack)->SetParamIdx(ParamFromCh(ch, kParamChKeyTrack));
      GetUI()->GetControlWithTag(kCtrlTagVelSens)->SetParamIdx(ParamFromCh(ch, kParamChVelSens));
      GetUI()->GetControlWithTag(kCtrlTagLegato)->SetParamIdx(ParamFromCh(ch, kParamChLegato));

      // Reassign all step sequencer knobs
      for (int i = 0; i < 16; i++) {
        GetUI()->GetControlWithTag(kCtrlTagKnobs + i)->SetParamIdx(ParamFromCh(ch, kParamEnv1LoopPoint + i));
      }

      mDSP.SetActiveChannel(NesApu::Channel(ch));
      UpdateStepSequencers();
      SendCurrentParamValuesFromDelegate();
//      }, label, style), kNoTag, "NES");
    }, {"PULSE 1", "PULSE 2", "TRIANGLE", "NOISE", "DPCM", "PULSE 3", "PULSE 4", "SAW"},
      nullptr, channelButtonStyle, EVShape::Rectangle, EDirection::Vertical), kNoTag, "NES");

    auto paramTuples = vector<tuple<NesApu::Channel, string>>{{NesApu::Channel::Pulse1,     "Pulse 1"},
                                                              {NesApu::Channel::Pulse2,     "Pulse 2"},
                                                              {NesApu::Channel::Triangle,   "Triangle"},
                                                              {NesApu::Channel::Noise,      "Noise"},
                                                              {NesApu::Channel::Dpcm,       "DPCM"},
                                                              {NesApu::Channel::Vrc6Pulse1, "Pulse 3"},
                                                              {NesApu::Channel::Vrc6Pulse2, "Pulse 4"},
                                                              {NesApu::Channel::Vrc6Saw,    "Saw"}};
    for (auto paramTuple : paramTuples) {
      auto ch = get<NesApu::Channel>(paramTuple);
      auto label = get<string>(paramTuple).c_str();

      pGraphics->AttachControl(new ISVGSwitchControl(channelButtonRect.GetFromRight(32.f).GetTranslated(-9.f, 0), { channelOffSvg, channelOnSvg }, ParamFromCh(ch, kParamChEnabled)), kNoTag, "NES");
//      pGraphics->AttachControl(new IVToggleControl(channelButtonRect.GetFromRight(40.f).GetPadded(-4.f), ParamFromCh(ch, kParamChEnabled), label, noLabelStyle), kNoTag, "NES");
////      pGraphics->AttachControl(new IVButtonControl(channelButtonRect.GetReducedFromRight(40.f), [this, ch](IControl* pCaller){
//      pGraphics->AttachControl(new IVTabSwitchControl(channelButtonRect.GetReducedFromRight(40.f), [this, ch](IControl* pCaller){
//        bool isDpcm = ch == NesApu::Channel::Dpcm;
//        GetUI()->GetControlWithTag(kCtrlTagDpcmEditor)->Hide(!isDpcm);
//        GetUI()->ForControlInGroup("StepSequencers", [=](IControl& control) {
//          control.Hide(isDpcm);
//        });
//        GetUI()->ForControlInGroup("Knobs", [=](IControl& control) {
//          control.Hide(isDpcm);
//        });
//
//        // Reassign channel-specific toggles (Key track, Velocity sensitivity, Legato)
//        GetUI()->GetControlWithTag(kCtrlTagKeyTrack)->SetParamIdx(ParamFromCh(ch, kParamChKeyTrack));
//        GetUI()->GetControlWithTag(kCtrlTagVelSens)->SetParamIdx(ParamFromCh(ch, kParamChVelSens));
//        GetUI()->GetControlWithTag(kCtrlTagLegato)->SetParamIdx(ParamFromCh(ch, kParamChLegato));
//
//        // Reassign all step sequencer knobs
//        for (int i = 0; i < 16; i++) {
//          GetUI()->GetControlWithTag(kCtrlTagKnobs + i)->SetParamIdx(ParamFromCh(ch, kParamEnv1LoopPoint + i));
//        }
//
//        mDSP.SetActiveChannel(ch);
//        UpdateStepSequencers();
//        SendCurrentParamValuesFromDelegate();
////      }, label, style), kNoTag, "NES");
//      }, {"one", "two"}, label, style, EVShape::Rectangle, EDirection::Vertical), kNoTag, "NES");
      channelButtonRect.Translate(0, channelButtonRect.H());
    }


#pragma mark - Keyboard Controls

    const float kToggleSwitchWidth = 26;
    const float kToggleSwitchHeight = 20;

    IVStyle keyboardControlLabelStyle = style.WithValueText(style.valueText.WithAlign(EAlign::Near)).WithDrawFrame(false);
    channelButtonRect.B = channelButtonRect.T + kToggleSwitchHeight;

    channelButtonRect.Translate(0, channelButtonRect.H()); // Vertical space

    auto keyboardParamTuples = vector<tuple<EControlTags, EChParams, string>>{
      {kCtrlTagKeyTrack, kParamChKeyTrack, "Key Track"},
      {kCtrlTagVelSens, kParamChVelSens, "Velocity Sens"},
      {kCtrlTagLegato, kParamChLegato, "Legato"}
    };

    for (auto keyboardParamTuple : keyboardParamTuples) {
      auto labelControl = new IVLabelControl(channelButtonRect.GetReducedFromRight(kToggleSwitchWidth), get<string>(keyboardParamTuple).c_str(), keyboardControlLabelStyle);
      pGraphics->AttachControl(labelControl);
      auto switchControl = new ISVGSwitchControl(channelButtonRect.GetFromRight(kToggleSwitchWidth), { switchOffSvg, switchOnSvg }, ParamFromCh(0, get<EChParams>(keyboardParamTuple)));
      pGraphics->AttachControl(switchControl, get<EControlTags>(keyboardParamTuple));
      channelButtonRect.Translate(0, channelButtonRect.H());
    }

    channelButtonRect.Translate(0, channelButtonRect.H()); // Vertical space

    pGraphics->AttachControl(new IVLabelControl(channelButtonRect.GetReducedFromRight(kToggleSwitchWidth), "Omni Mode", keyboardControlLabelStyle));
    auto omniButton = new ISVGSwitchControl(channelButtonRect.GetFromRight(kToggleSwitchWidth), { switchOffSvg, switchOnSvg }, kParamOmniMode);
    omniButton->SetTooltip("When Omni Mode is on, all NES channels receive events from all MIDI channels. "
                           "When Omni Mode is off, each MIDI channel is mapped to a different NES channel. \n"
                           "If your plugin host doesn't support multichannel plugins, "
                           "or if you don't know what this means, leave Omni Mode on.");
    pGraphics->AttachControl(omniButton, kNoTag, "NES");
    channelButtonRect.Translate(0, channelButtonRect.H());

    channelButtonRect.B = channelButtonRect.T + 40.f;
    pGraphics->AttachControl(new IVButtonControl(channelButtonRect, [=](IControl *pCaller) {
      static bool hide = false;
      hide = !hide;
      pGraphics->GetControlWithTag(kCtrlTagKeyboard)->Hide(hide);
      pGraphics->GetControlWithTag(kCtrlTagBender)->Hide(hide);
      pGraphics->GetControlWithTag(kCtrlTagModWheel)->Hide(hide);
      pGraphics->Resize(PLUG_WIDTH, hide ? PLUG_HEIGHT - keyboardBounds.H() - PLUG_PADDING : PLUG_HEIGHT, pGraphics->GetDrawScale());
    }, "Toggle Keyboard", DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE)));

#pragma mark - Presets

    MakeDefaultPreset(nullptr, kNumPresets);

    pGraphics->AttachControl(new IVBakedPresetManagerControl(b.ReduceFromTop(40).GetFromRight(300), style.WithLabelText({15.f, EVAlign::Middle}))); // "./presets", "nesvst"));

#pragma mark - Step Sequencers

    const int kKnobHeight = 64.f;

    const IRECT editorPanel = b.GetReducedFromLeft(136);

    auto createEnvelopePanel = [=](IRECT rect, const char* label, float minVal, float maxVal, int envIdx, int baseParam, int ctrlTag, IColor color) {

      auto stepSeq = new StepSequencer(rect.GetReducedFromBottom(kKnobHeight + 16.f),
                                       label,
                                       style.WithColor(kFG, color).WithColor(kBG, IColor::FromColorCodeStr("#141414")),
                                       64,
                                       1.f/float(maxVal - minVal),
                                       nullptr);
      pGraphics->AttachControl(stepSeq, ctrlTag, "StepSequencers");

      IRECT knobBox = rect.GetFromBottom(kKnobHeight);
      IVStyle knobStyle = style
        .WithLabelText(style.labelText.WithAlign(EAlign::Center).WithFont("Normal").WithSize(15.f))
        .WithValueText(style.valueText.WithFont("Bold"));

      int loopParam = baseParam + 0;
      int relParam  = baseParam + 1;
      int lenParam  = baseParam + 2;
      int spdParam  = baseParam + 3;

      // Loop
      auto lpC = new KnobControl(knobBox.SubRectHorizontal(4, 0), loopParam, "Loop", knobStyle, false, false);
      lpC->SetActionFunction([=](IControl *pCaller) {
//        mDSP.mNesEnvs[envIdx]->SetLoop(pCaller->GetParam()->Int());

        UpdateStepSequencerAndParamsFromEnv(baseParam, mDSP.mNesEnvs[envIdx], stepSeq);
        SendCurrentParamValuesFromDelegate();
        stepSeq->SetDirty(false);
      });
      pGraphics->AttachControl(lpC, kCtrlTagKnobs + envIdx * 4 + 0, "Knobs");
      stepSeq->SetLoopPoint(GetParam(kParamEnv1LoopPoint)->Int());

      // Release
      auto rpC = new KnobControl(knobBox.SubRectHorizontal(4, 1), relParam, "Release", knobStyle, false, false);
      rpC->SetActionFunction([=](IControl *pCaller) {
//        mDSP.mNesEnvs[envIdx]->SetRelease(pCaller->GetParam()->Int());
        UpdateStepSequencerAndParamsFromEnv(baseParam, mDSP.mNesEnvs[envIdx], stepSeq);
        SendCurrentParamValuesFromDelegate();
        stepSeq->SetDirty(false);
      });
      pGraphics->AttachControl(rpC, kCtrlTagKnobs + envIdx * 4 + 1, "Knobs");
      stepSeq->SetReleasePoint(GetParam(kParamEnv1LoopPoint)->Int());

      // Length
      auto lC = new KnobControl(knobBox.SubRectHorizontal(4, 2), lenParam, "Length", knobStyle, false, false);
      lC->SetActionFunction([=](IControl *pCaller) {
//        mDSP.mNesEnvs[envIdx]->SetLength(pCaller->GetParam()->Int());
        UpdateStepSequencerAndParamsFromEnv(baseParam, mDSP.mNesEnvs[envIdx], stepSeq);
        SendCurrentParamValuesFromDelegate();
        stepSeq->SetDirty(false);
      });
      pGraphics->AttachControl(lC, kCtrlTagKnobs + envIdx * 4 + 2, "Knobs");
      stepSeq->SetLength(GetParam(kParamEnv1LoopPoint)->Int());

      // Speed
      auto sC = new KnobControl(knobBox.SubRectHorizontal(4, 3), spdParam, "Speed", knobStyle, false, false);
      sC->SetActionFunction([=](IControl *pCaller) {
        mDSP.mNesEnvs[envIdx]->SetSpeedDivider(pCaller->GetParam()->Int());
      });
      pGraphics->AttachControl(sC, kCtrlTagKnobs + envIdx * 4 + 3, "Knobs");
    };

    IRECT envPanel = editorPanel.GetPadded(8);
    createEnvelopePanel(envPanel.GetGridCell(0, 2, 2).GetPadded(-8), "VOLUME", 0, 15, 0, ParamFromCh(0, kParamEnv1LoopPoint), kCtrlTagEnvelope1, IColor::FromColorCodeStr("#CC2626"));
    createEnvelopePanel(envPanel.GetGridCell(1, 2, 2).GetPadded(-8), "DUTY", 0, 7, 1, ParamFromCh(1, kParamEnv2LoopPoint), kCtrlTagEnvelope2, IColor::FromColorCodeStr("#DE5E33"));
    createEnvelopePanel(envPanel.GetGridCell(2, 2, 2).GetPadded(-8), "PITCH", -12, 12, 2, ParamFromCh(2, kParamEnv3LoopPoint), kCtrlTagEnvelope3, IColor::FromColorCodeStr("#53AD8E"));
    createEnvelopePanel(envPanel.GetGridCell(3, 2, 2).GetPadded(-8), "FINE PITCH", -12, 12, 3, ParamFromCh(3, kParamEnv4LoopPoint), kCtrlTagEnvelope4, IColor::FromColorCodeStr("#747ACD"));

    UpdateStepSequencers();

#pragma mark - DPCM Editor

    auto dpcmEditor = new DpcmEditorControl(editorPanel, style, mDSP.mNesChannels->dpcm.mNesDpcm);
    pGraphics->AttachControl(dpcmEditor, kCtrlTagDpcmEditor, "DpcmEditor");
    dpcmEditor->Hide(true);

  };
#endif
}

void LoudNES::OnPresetsModified() {
  printf("-- Presets modified\n");
  UpdateStepSequencers();
  GetUI()->ForControlInGroup("DpcmEditor", [](IControl &control) { control.SetDirty(false); });

  IPluginBase::OnPresetsModified();
}

void LoudNES::UpdateStepSequencerAndParamsFromEnv(int paramEnvLoopPoint, NesEnvelope* env, StepSequencer* seq) {
  // Update params
  GetParam(paramEnvLoopPoint + 0)->Set(env->mLoopPoint);
  GetParam(paramEnvLoopPoint + 1)->Set(env->mReleasePoint);
  GetParam(paramEnvLoopPoint + 2)->Set(env->mLength);
  GetParam(paramEnvLoopPoint + 3)->Set(env->mSpeedDivider);

  // Update Step Sequencer
  seq->SetLoopPoint(env->mLoopPoint);
  seq->SetReleasePoint(env->mReleasePoint);
  seq->SetLength(env->mLength);
}

struct SeqGroup {
  int ctrlTag;
  int param;
  NesEnvelope* env;
};

void LoudNES::UpdateStepSequencers() {
  // update all envelope values
  for (auto seqGroup : vector<SeqGroup>{{kCtrlTagEnvelope1, kParamEnv1LoopPoint, mDSP.mNesEnvelope1},
                                        {kCtrlTagEnvelope2, kParamEnv2LoopPoint, mDSP.mNesEnvelope2},
                                        {kCtrlTagEnvelope3, kParamEnv3LoopPoint, mDSP.mNesEnvelope3},
                                        {kCtrlTagEnvelope4, kParamEnv4LoopPoint, mDSP.mNesEnvelope4}}) {
    auto seq = dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(seqGroup.ctrlTag));
    auto nesEnv = seqGroup.env;
    for (int i = 0; i < 64; i++) {
      seq->SetValue(float(nesEnv->mValues[i] - nesEnv->mMinVal) / float(nesEnv->mMaxVal - nesEnv->mMinVal), i);
    }

    UpdateStepSequencerAndParamsFromEnv(seqGroup.param, nesEnv, seq);

    seq->SetActionFunc([nesEnv](int stepIdx, float value) {
      nesEnv->mValues[stepIdx] = round(iplug::Lerp((float)nesEnv->mMinVal, (float)nesEnv->mMaxVal, value));
    });
    seq->SetSlidersDirty();
  }
}

#if IPLUG_DSP
void LoudNES::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames, mTimeInfo.mPPQPos, mTimeInfo.mTransportIsRunning);

  // 1/60 sec = 735 samples @ 44100 hz

  // 20% cpu while 4 envs running
//  if (GetUI() && GetUI()->GetControlWithTag(kCtrlTagEnvelope1)) {
//    dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(kCtrlTagEnvelope1))->SetHighlightIdx(mDSP.mNesEnvelope1->GetStep());
//    dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(kCtrlTagEnvelope2))->SetHighlightIdx(mDSP.mNesEnvelope2->GetStep());
//    dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(kCtrlTagEnvelope3))->SetHighlightIdx(mDSP.mNesEnvelope3->GetStep());
//    dynamic_cast<StepSequencer *>(GetUI()->GetControlWithTag(kCtrlTagEnvelope4))->SetHighlightIdx(mDSP.mNesEnvelope4->GetStep());
//  }

//  if (ss1) {
//    ss1->SetHighlightIdx(mDSP.mNesEnvelope1->GetStep());
//    ss2->SetHighlightIdx(mDSP.mNesEnvelope2->GetStep());
//    ss3->SetHighlightIdx(mDSP.mNesEnvelope3->GetStep());
//    ss4->SetHighlightIdx(mDSP.mNesEnvelope4->GetStep());
//  }

  // 6% cpu while 4 envs running @ 4096 buffer size
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope1, {mDSP.mNesEnvelope1->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope2, {mDSP.mNesEnvelope2->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope3, {mDSP.mNesEnvelope3->GetStep()}});
  mEnvelopeVisSender.PushData({kCtrlTagEnvelope4, {mDSP.mNesEnvelope4->GetStep()}});

  // Smoother display update, but more CPU usage, and bad to do from the DSP thread.
  // mEnvelopeVisSender.TransmitData(*this);
}

void LoudNES::OnIdle()
{
  // More jittery display update (not smooth 60 fps), but less CPU usage.
  mEnvelopeVisSender.TransmitData(*this);
}

void LoudNES::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void LoudNES::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;

  int status = msg.StatusMsg();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }

handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void LoudNES::OnParamChange(int paramIdx)
{
//  printf("OnParamChange paramIdx %d - value %d\n", paramIdx, GetParam(paramIdx)->Int());
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

bool LoudNES::SerializeState(IByteChunk &chunk) const {
  for (auto channel : mDSP.mNesChannels->allChannels) {
    channel->Serialize(chunk);
  }
  return IPluginBase::SerializeState(chunk);
}

int LoudNES::UnserializeState(const IByteChunk &chunk, int startPos) {
  int pos = startPos;
  for (auto channel : mDSP.mNesChannels->allChannels) {
    pos = channel->Deserialize(chunk, pos);
  }
  return IPluginBase::UnserializeState(chunk, pos);
}

bool LoudNES::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  printf("OnMessage msgTag %d - ctrlTag %d - dataSize %d\n", msgTag, ctrlTag, dataSize);
  if(ctrlTag == kCtrlTagBender && msgTag == IWheelControl::kMessageTagSetPitchBendRange)
  {
    const int bendRange = *static_cast<const int*>(pData);
    for (auto synth : mDSP.mChannelSynths) synth->SetPitchBendRange(bendRange);
  }

  return false;
}
#endif
