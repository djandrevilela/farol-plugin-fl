#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>
#include <memory>
#include <vector>

#include "Farol.h"
#include "Synth.h"

constexpr int kMaxSlots = 8;
extern const char* const kKeyNames[12];

struct Seq {
    std::vector<std::vector<int>> chords;  // notas MIDI de cada compasso
    std::vector<std::string> names;
};

class FarolProcessor : public juce::AudioProcessor,
                       public juce::ChangeBroadcaster,
                       private juce::AsyncUpdater,
                       private juce::AudioProcessorValueTreeState::Listener {
public:
    FarolProcessor();
    ~FarolProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Farol"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- API do editor ----
    juce::AudioProcessorValueTreeState apvts;
    farol::Lang lang() const { return pi("lang") == 0 ? farol::Lang::PT : farol::Lang::EN; }
    farol::Key key() const { return {juce::jlimit(0, 11, pi("key")), (farol::ScaleType) juce::jlimit(0, farol::kNumScales - 1, pi("scale"))}; }
    bool seventh() const { return pb("seventh"); }
    bool explainOn() const { return pb("explain"); }
    void regenerateForTest() { regenerate(); }
    int slots() const { return juce::jlimit(2, kMaxSlots, pi("slots")); }
    std::vector<farol::ChordOption> options() const { return farol::chordOptions(key(), seventh()); }
    int slotOption(int slot) const;  // índice válido nas opções
    std::vector<farol::ChordOption> chosen() const;
    void setParam(const juce::String& id, float realValue);
    void loadExample(int index);

    int lastNote() const { return lastNoteVal.load(); }
    int lastNoteCount() const { return lastNoteCounter.load(); }
    int playingChord() const { return uiChord.load(); }
    double currentBpmForAdvice() const { double h = lastHostBpm.load(); return h > 0 ? h : (double) pf("bpm"); }

    bool writeMidiFile(const juce::File&) const;
    bool writeLessonFile(const juce::File&) const;
    juce::String suggestedFileName(const juce::String& what) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void regenerate();

    float pf(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    int pi(const juce::String& id) const { return (int) std::lround(pf(id)); }
    bool pb(const juce::String& id) const { return pf(id) > 0.5f; }

    mutable juce::SpinLock dataLock;
    std::shared_ptr<const Seq> published, audioData;
    std::vector<std::shared_ptr<const Seq>> graveyard;
    juce::StringArray listenIds;

    std::atomic<float>* pAudition = nullptr;
    std::atomic<float>* pBpm = nullptr;
    std::atomic<float>* pVol = nullptr;
    PreviewSynth synth;
    struct Ev { int off; bool on; int note; int vel; };
    Ev ev[256];
    int nEv = 0;
    std::vector<int> sounding;
    juce::MidiBuffer outMidi;
    bool seqRunning = false, lastAudition = false;
    double expectedP0 = 0.0, auditionBeat = 0.0;
    std::atomic<int> lastNoteVal{-1}, lastNoteCounter{0}, uiChord{-1};
    std::atomic<double> lastHostBpm{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FarolProcessor)
};
