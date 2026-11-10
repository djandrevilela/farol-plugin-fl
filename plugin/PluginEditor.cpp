#include "PluginEditor.h"

#include "Util.h"

using namespace farol;

namespace {
const juce::Colour kBg(0xff14161f), kPanel(0xff1d2130), kEdge(0xff2d3348), kText(0xffe2e8f0), kMuted(0xff8a94a8), kAccent(0xfff6ad55), kTeal(0xff4fd1c5);
const juce::Colour kRole[5] = {juce::Colour(0xff68d391), juce::Colour(0xffa0d468), juce::Colour(0xfff6ad55), juce::Colour(0xfff56565), juce::Colour(0xffe53e3e)};
}  // namespace

// =============================================================== DragHandle
void DragHandle::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1);
    g.setColour(kTeal.withAlpha(0.16f));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(kTeal);
    g.drawRoundedRectangle(r, 7.0f, 1.2f);
    g.setColour(kText);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}
void DragHandle::mouseDrag(const juce::MouseEvent& e) {
    if (dragging || e.getDistanceFromDragStart() < 6 || !makeFile) return;
    auto f = makeFile();
    if (!f.existsAsFile()) return;
    dragging = true;
    juce::DragAndDropContainer::performExternalDragDropOfFiles({f.getFullPathName()}, false, this, [this] { dragging = false; });
}

// =============================================================== SlotRow
SlotRow::SlotRow(int index, std::function<void(int)> onSelect, std::function<void(int, int)> onPick) : idx(index), select(std::move(onSelect)) {
    addAndMakeVisible(box);
    box.onChange = [this, onPick] {
        int id = box.getSelectedId();
        if (id > 0) onPick(idx, id - 1);
    };
}
void SlotRow::setContent(const juce::StringArray& items, int selected, int t, const juce::String& r, const juce::String& m, bool isSel, bool isPlay) {
    box.clear(juce::dontSendNotification);
    for (int i = 0; i < items.size(); ++i) box.addItem(items[i], i + 1);
    box.setSelectedId(selected + 1, juce::dontSendNotification);
    tension = t; role = r; mood = m; sel = isSel; playing = isPlay;
    repaint();
}
void SlotRow::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(0, 2);
    g.setColour(sel ? kEdge : kPanel);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(playing ? kTeal : (sel ? kAccent : kEdge));
    g.drawRoundedRectangle(r, 7.0f, playing || sel ? 1.8f : 1.0f);
    g.setColour(kText);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText(juce::String(idx + 1), 10, 4, 22, 20, juce::Justification::centred);
    // barra de tensão
    for (int i = 0; i < 4; ++i) {
        g.setColour(i < tension ? kRole[std::min(4, i + 1)] : kEdge);
        g.fillRoundedRectangle(14.0f + (float) i * 0, 28.0f + (float) (3 - i) * 4.0f, 14.0f, 3.0f, 1.5f);
    }
    g.setColour(kText);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(role, 214, 4, 310, 18, juce::Justification::left, true);
    g.setColour(kMuted);
    g.setFont(juce::FontOptions(11.5f));
    g.drawText(mood, 214, 24, 310, 18, juce::Justification::left, true);
}
void SlotRow::resized() { box.setBounds(40, 11, 166, 26); }

// =============================================================== FarolEditor
juce::String FarolEditor::T(const char* pt, const char* en) const { return U8(L(proc.lang(), pt, en)); }

void FarolEditor::styleBox(juce::TextEditor& t) {
    t.setMultiLine(true, true);
    t.setReadOnly(true);
    t.setCaretVisible(false);
    t.setScrollbarsShown(true);
    t.setColour(juce::TextEditor::backgroundColourId, kPanel);
    t.setColour(juce::TextEditor::textColourId, kText);
    t.setColour(juce::TextEditor::outlineColourId, kEdge);
    t.setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(t);
}

FarolEditor::FarolEditor(FarolProcessor& p) : juce::AudioProcessorEditor(&p), proc(p) {
    setLookAndFeel(&lnf);
    lnf.setColour(juce::ComboBox::backgroundColourId, kPanel);
    lnf.setColour(juce::PopupMenu::backgroundColourId, kPanel);
    lnf.setColour(juce::Slider::thumbColourId, kAccent);
    lnf.setColour(juce::Slider::trackColourId, kAccent.withAlpha(0.5f));
    lnf.setColour(juce::ToggleButton::tickColourId, kAccent);
    lnf.setColour(juce::TextButton::buttonColourId, kPanel);

    auto combo = [&](juce::ComboBox& c, const char* id, const juce::StringArray& items) {
        c.addItemList(items, 1);
        addAndMakeVisible(c);
        cAtt.push_back(std::make_unique<CA>(proc.apvts, id, c));
    };
    auto toggle = [&](juce::ToggleButton& t, const char* id) {
        addAndMakeVisible(t);
        bAtt.push_back(std::make_unique<BA>(proc.apvts, id, t));
    };
    auto slider = [&](juce::Slider& s, const char* id) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
        addAndMakeVisible(s);
        sAtt.push_back(std::make_unique<SA>(proc.apvts, id, s));
    };
    juce::StringArray keys, scales, modes{"-", "-", "-"};
    for (auto* k : kKeyNames) keys.add(k);
    for (int i = 0; i < kNumScales; ++i) scales.add("-");
    combo(langBox, "lang", juce::StringArray{TR("Portugu\xC3\xAAs (PT)"), "English (UK)"});
    combo(keyBox, "key", keys);
    combo(scaleBox, "scale", scales);
    combo(sampleKeyBox, "sKey", keys);
    combo(sampleModeBox, "sMode", modes);
    toggle(seventhTg, "seventh");
    toggle(explainTg, "explain");
    toggle(auditionTg, "audition");
    slider(bpmSlider, "bpm");
    slider(sampleBpmSlider, "sBpm");
    for (auto* l : {&bpmL, &sKeyL, &sModeL, &sBpmL, &progL, &liveL, &sampleL, &nextL}) {
        l->setColour(juce::Label::textColourId, kMuted);
        l->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(*l);
    }
    headline.setColour(juce::Label::textColourId, kAccent);
    headline.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(headline);

    for (int i = 0; i < kMaxSlots; ++i)
        addAndMakeVisible(rows.add(new SlotRow(i,
            [this](int s) { sel = s; refresh(); },
            [this](int slot, int opt) { proc.setParam("c" + juce::String(slot), (float) opt); sel = slot; })));
    for (auto* b : {&addBtn, &remBtn, &exBtn}) addAndMakeVisible(*b);
    addBtn.onClick = [this] { proc.setParam("slots", (float) std::min(kMaxSlots, proc.slots() + 1)); };
    remBtn.onClick = [this] { proc.setParam("slots", (float) std::max(2, proc.slots() - 1)); sel = std::min(sel, proc.slots() - 2); };
    exBtn.onClick = [this] { proc.loadExample(exampleIdx); exampleIdx = (exampleIdx + 1) % 4; sel = 0; };
    for (int i = 0; i < 3; ++i) {
        addAndMakeVisible(sug[i]);
        sug[i].onClick = [this, i] {
            if (sugOption[i] < 0) return;
            int target = sel + 1;
            if (target >= proc.slots()) { proc.setParam("slots", (float) std::min(kMaxSlots, proc.slots() + 1)); }
            if (target < kMaxSlots) { proc.setParam("c" + juce::String(target), (float) sugOption[i]); sel = target; }
        };
    }
    for (auto* t : {&explainBox, &summaryBox, &liveBox, &sampleBox}) styleBox(*t);

    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory);
    dragMidi.makeFile = [this, tmp] {
        auto f = tmp.getChildFile(proc.suggestedFileName("Acordes") + ".mid");
        return proc.writeMidiFile(f) ? f : juce::File();
    };
    dragLesson.makeFile = [this, tmp] {
        auto f = tmp.getChildFile(proc.suggestedFileName(proc.lang() == Lang::PT ? "Licao" : "Lesson") + ".txt");
        return proc.writeLessonFile(f) ? f : juce::File();
    };
    addAndMakeVisible(dragMidi);
    addAndMakeVisible(dragLesson);

    setSize(1120, 830);
    proc.addChangeListener(this);
    // reage ao idioma
    langBox.onChange = [this] { juce::MessageManager::callAsync([safe = juce::Component::SafePointer<FarolEditor>(this)] { if (safe) safe->refresh(); }); };
    sampleKeyBox.onChange = [this] { refresh(); };
    sampleModeBox.onChange = [this] { refresh(); };
    sampleBpmSlider.onValueChange = [this] { refresh(); };
    bpmSlider.onValueChange = [this] { refresh(); };
    refresh();
    startTimerHz(10);
}

FarolEditor::~FarolEditor() {
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void FarolEditor::timerCallback() {
    const int pl = proc.playingChord();
    if (pl != lastPlaying) { lastPlaying = pl; refresh(); }
    if (proc.lastNoteCount() != lastLiveCount) { lastLiveCount = proc.lastNoteCount(); updateLive(); }
}

void FarolEditor::updateLive() {
    if (proc.lastNote() < 0) { liveBox.setText(T("Toca uma nota no teclado MIDI e eu explico-a.", "Play a note on your MIDI keyboard and I will explain it."), false); return; }
    auto opts = proc.options();
    const int pl = proc.playingChord();
    const ChordOption* chord = nullptr;
    auto chosen = proc.chosen();
    if (pl >= 0 && pl < (int) chosen.size()) chord = &chosen[(size_t) pl];
    else if (sel < (int) chosen.size()) chord = &chosen[(size_t) sel];
    liveBox.setText(U8(explainNote(proc.lastNote(), proc.key(), chord, proc.lang())), false);
}

void FarolEditor::refresh() {
    const Lang lg = proc.lang();
    const Key key = proc.key();
    const bool sev = proc.seventh();
    auto opts = proc.options();
    auto chosen = proc.chosen();
    sel = juce::jlimit(0, (int) chosen.size() - 1, sel);

    // textos dependentes do idioma
    for (int i = 0; i < kNumScales; ++i) scaleBox.changeItemText(i + 1, U8(scaleName((ScaleType) i, lg)));
    sampleModeBox.changeItemText(1, T("Maior", "Major"));
    sampleModeBox.changeItemText(2, T("Menor", "Minor"));
    sampleModeBox.changeItemText(3, T("Nota \xC3\xBA" "nica", "Single note"));
    scaleBox.setText(U8(scaleName(key.scale, lg)), juce::dontSendNotification);
    seventhTg.setButtonText(T("S\xC3\xA9timas", "7ths"));
    explainTg.setButtonText(T("Modo explica-me", "Explain mode"));
    auditionTg.setButtonText(T("\xE2\x96\xB6 Ouvir (loop)", "\xE2\x96\xB6 Listen (loop)"));
    bpmL.setText("BPM", juce::dontSendNotification);
    progL.setText(T("PROGRESS\xC3\x83O (clica numa linha para a explicar)", "PROGRESSION (click a row to explain it)"), juce::dontSendNotification);
    nextL.setText(T("A SEGUIR PODES IR PARA (clica para aplicar)", "WHAT CAN COME NEXT (click to apply)"), juce::dontSendNotification);
    liveL.setText(T("A TUA \xC3\x9A" "LTIMA NOTA (teclado MIDI)", "YOUR LAST NOTE (MIDI keyboard)"), juce::dontSendNotification);
    sampleL.setText(T("COMPATIBILIDADE DE SAMPLES", "SAMPLE COMPATIBILITY"), juce::dontSendNotification);
    sKeyL.setText(T("Tom da sample", "Sample key"), juce::dontSendNotification);
    sModeL.setText(T("Tipo", "Type"), juce::dontSendNotification);
    sBpmL.setText(T("BPM da sample (0 = n\xC3\xA3o sei)", "Sample BPM (0 = unknown)"), juce::dontSendNotification);
    addBtn.setButtonText(T("+ acorde", "+ chord"));
    remBtn.setButtonText(T("- acorde", "- chord"));
    exBtn.setButtonText(T("Exemplo \xE2\x96\xB8", "Example \xE2\x96\xB8"));
    dragMidi.text = T("Arrastar MIDI dos acordes", "Drag chords MIDI");
    dragLesson.text = T("Arrastar li\xC3\xA7\xC3\xA3o (.txt)", "Drag lesson (.txt)");
    dragMidi.repaint();
    dragLesson.repaint();

    // linhas da progressão
    juce::StringArray items;
    for (auto& c : opts) items.add(U8(c.name + "   " + c.roman));
    const int playing = proc.playingChord();
    for (int i = 0; i < rows.size(); ++i) {
        rows[i]->setVisible(i < (int) chosen.size());
        if (i >= (int) chosen.size()) continue;
        auto e = explainChord(chosen[(size_t) i], key, sev, lg);
        rows[i]->setContent(items, proc.slotOption(i), e.tension, U8(e.role), U8(e.mood), i == sel, i == playing);
    }
    summaryBox.setText(U8(explainProgression(chosen, key, lg)), false);

    // explicação do acorde escolhido
    {
        auto e = explainChord(chosen[(size_t) sel], key, sev, lg);
        juce::String t = U8(e.name) + "  (" + U8(e.roman) + ")   -   " + U8(e.role) + "\n" + T("Tens\xC3\xA3o: ", "Tension: ") + juce::String(e.tension) + "/4\n\n";
        if (proc.explainOn()) {
            t += T("Sensa\xC3\xA7\xC3\xA3o: ", "Feel: ") + U8(e.mood) + "\n\n";
            t += T("Porqu\xC3\xAA: ", "Why: ") + U8(e.why) + "\n\n";
            if (!e.extra.empty()) t += U8(e.extra) + "\n\n";
            if (sel > 0) t += T("Vindo do acorde anterior: ", "Coming from the previous chord: ") + U8(explainTransition(chosen[(size_t) sel - 1], chosen[(size_t) sel], key, lg)) + "\n\n";
            t += T("Dica de produ\xC3\xA7\xC3\xA3o: ", "Production tip: ") + U8(e.tip);
        } else t += T("(Modo explica-me desligado: liga-o em cima para ver o porqu\xC3\xAA de cada acorde.)", "(Explain mode is off: switch it on above to see why each chord works.)");
        explainBox.setText(t, false);
    }
    // sugestões
    {
        auto sg = suggestNext(opts, proc.slotOption(sel), key, lg);
        for (int i = 0; i < 3; ++i) {
            sugOption[i] = i < (int) sg.size() ? sg[(size_t) i].option : -1;
            sug[i].setVisible(sugOption[i] >= 0);
            if (sugOption[i] >= 0)
                sug[i].setButtonText(U8(opts[(size_t) sugOption[i]].name + " (" + opts[(size_t) sugOption[i]].roman + ")  -  " + (proc.explainOn() ? sg[(size_t) i].reason : std::string())));
        }
    }
    updateLive();

    // samples
    {
        SampleIn in;
        in.root = juce::jlimit(0, 11, (int) std::lround(proc.apvts.getRawParameterValue("sKey")->load()));
        const int m = (int) std::lround(proc.apvts.getRawParameterValue("sMode")->load());
        in.kind = m == 0 ? SampleKind::Major : m == 1 ? SampleKind::Minor : SampleKind::SingleNote;
        in.bpm = proc.apvts.getRawParameterValue("sBpm")->load();
        const double pb = proc.currentBpmForAdvice();
        auto a = adviseSample(in, key, pb, lg);
        headline.setText(U8(a.headline), juce::dontSendNotification);
        juce::String t;
        for (auto& l : a.lines) t += juce::String(juce::CharPointer_UTF8("\xE2\x80\xA2 ")) + U8(l) + "\n";
        sampleBox.setText(t, false);
    }
    repaint();
}

void FarolEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kAccent);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("FAROL", 12, 6, 80, 22, juce::Justification::left);
    g.setColour(kPanel);
    g.fillRoundedRectangle(juce::Rectangle<float>(12, 602, 1096, 218), 8.0f);
}

void FarolEditor::resized() {
    int x = 96;
    langBox.setBounds(x, 8, 140, 26); x += 148;
    keyBox.setBounds(x, 8, 64, 26); x += 72;
    scaleBox.setBounds(x, 8, 150, 26); x += 160;
    seventhTg.setBounds(x, 8, 80, 26); x += 84;
    explainTg.setBounds(x, 8, 140, 26); x += 150;
    bpmL.setBounds(x, 8, 34, 26); x += 34;
    bpmSlider.setBounds(x, 8, 190, 26); x += 200;
    auditionTg.setBounds(x, 8, 140, 26);
    progL.setBounds(12, 42, 540, 18);
    for (int i = 0; i < rows.size(); ++i) rows[i]->setBounds(12, 62 + i * 50, 540, 50);
    addBtn.setBounds(12, 466, 100, 26);
    remBtn.setBounds(118, 466, 100, 26);
    exBtn.setBounds(224, 466, 110, 26);
    dragMidi.setBounds(344, 466, 200, 26);
    dragLesson.setBounds(344, 496, 200, 26);
    summaryBox.setBounds(12, 528, 540, 66);
    explainBox.setBounds(564, 42, 544, 286);
    nextL.setBounds(564, 334, 540, 18);
    for (int i = 0; i < 3; ++i) sug[i].setBounds(564, 354 + i * 32, 544, 28);
    liveL.setBounds(564, 456, 540, 18);
    liveBox.setBounds(564, 476, 544, 118);
    sampleL.setBounds(24, 606, 300, 18);
    sKeyL.setBounds(24, 630, 120, 20); sampleKeyBox.setBounds(24, 650, 90, 26);
    sModeL.setBounds(130, 630, 120, 20); sampleModeBox.setBounds(130, 650, 140, 26);
    sBpmL.setBounds(24, 690, 260, 20); sampleBpmSlider.setBounds(24, 712, 260, 26);
    headline.setBounds(300, 606, 800, 24);
    sampleBox.setBounds(300, 634, 796, 178);
}
