#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>
#include <memory>

#include "PluginProcessor.h"

class DragHandle : public juce::Component {
public:
    std::function<juce::File()> makeFile;
    juce::String text;
    void paint(juce::Graphics&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

private:
    bool dragging = false;
};

class SlotRow : public juce::Component {
public:
    SlotRow(int index, std::function<void(int)> onSelect, std::function<void(int, int)> onPick);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override { select(idx); }
    void setContent(const juce::StringArray& items, int selected, int tension, const juce::String& role, const juce::String& mood, bool isSelected, bool isPlaying);

private:
    int idx;
    std::function<void(int)> select;
    juce::ComboBox box;
    int tension = 0;
    juce::String role, mood;
    bool sel = false, playing = false;
};

class FarolEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer {
public:
    explicit FarolEditor(FarolProcessor&);
    ~FarolEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();
    void updateLive();
    juce::String T(const char* pt, const char* en) const;
    void styleBox(juce::TextEditor&);

    FarolProcessor& proc;
    juce::LookAndFeel_V4 lnf;
    int sel = 0, exampleIdx = 0, lastLiveCount = 0, lastPlaying = -2;

    juce::ComboBox langBox, keyBox, scaleBox, sampleKeyBox, sampleModeBox;
    juce::ToggleButton seventhTg, explainTg, auditionTg;
    juce::Slider bpmSlider, sampleBpmSlider;
    juce::Label bpmL, sKeyL, sModeL, sBpmL, progL, liveL, sampleL, headline, nextL;
    juce::TextButton addBtn, remBtn, exBtn, sug[3];
    int sugOption[3] = {-1, -1, -1};
    juce::TextEditor explainBox, summaryBox, liveBox, sampleBox;
    DragHandle dragMidi, dragLesson;
    juce::OwnedArray<SlotRow> rows;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
    std::vector<std::unique_ptr<SA>> sAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FarolEditor)
};
