#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

#include "PluginEditor.h"
#include "Util.h"

using namespace farol;

const char* const kKeyNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

namespace {
juce::String cId(int i) { return "c" + juce::String(i); }
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout FarolProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto choice = [&](const String& id, const String& name, StringArray items, int def) { l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, items, def)); };
    auto flt = [&](const String& id, const String& name, float lo, float hi, float def) { l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, NormalisableRange<float>(lo, hi), def)); };
    auto boo = [&](const String& id, const String& name, bool def) { l.add(std::make_unique<AudioParameterBool>(ParameterID{id, 1}, name, def)); };
    auto intp = [&](const String& id, const String& name, int lo, int hi, int def) { l.add(std::make_unique<AudioParameterInt>(ParameterID{id, 1}, name, lo, hi, def)); };
    StringArray keys;
    for (auto* k : kKeyNames) keys.add(k);
    StringArray scales;
    for (int i = 0; i < kNumScales; ++i) scales.add(U8(scaleName((ScaleType) i, Lang::EN)));

    choice("lang", "Language", StringArray{TR("Portugu\xC3\xAAs (PT)"), "English (UK)"}, 0);
    choice("key", "Key", keys, 9);
    choice("scale", "Scale", scales, 1);
    boo("seventh", "7ths", false);
    boo("explain", "Explain mode", true);
    intp("slots", "Chords", 2, kMaxSlots, 4);
    const int def[4] = {0, 5, 2, 6};
    for (int i = 0; i < kMaxSlots; ++i) intp(cId(i), "Chord " + String(i + 1), 0, 9, i < 4 ? def[i] : 0);
    flt("bpm", "BPM", 60.f, 200.f, 120.f);
    boo("audition", "Listen", false);
    choice("sKey", "Sample key", keys, 0);
    choice("sMode", "Sample type", StringArray{"Major", "Minor", "Single note"}, 1);
    flt("sBpm", "Sample BPM", 0.f, 200.f, 0.f);
    flt("gVol", "Volume", 0.f, 1.f, 0.7f);
    return l;
}

FarolProcessor::FarolProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "FAROL", createLayout()) {
    pAudition = apvts.getRawParameterValue("audition");
    pBpm = apvts.getRawParameterValue("bpm");
    pVol = apvts.getRawParameterValue("gVol");
    juce::StringArray ids{"key", "scale", "seventh", "slots"};
    for (int i = 0; i < kMaxSlots; ++i) ids.add(cId(i));
    for (auto& id : ids) apvts.addParameterListener(id, this);
    listenIds = ids;
    sounding.reserve(16);
    regenerate();
}

FarolProcessor::~FarolProcessor() {
    cancelPendingUpdate();
    for (auto& id : listenIds) apvts.removeParameterListener(id, this);
}

void FarolProcessor::prepareToPlay(double sr, int) {
    synth.prepare(sr);
    sounding.clear();
    seqRunning = false;
}

void FarolProcessor::setParam(const juce::String& id, float v) {
    if (auto* p = apvts.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}

void FarolProcessor::parameterChanged(const juce::String&, float) { triggerAsyncUpdate(); }
void FarolProcessor::handleAsyncUpdate() {
    regenerate();
    sendChangeMessage();
}

int FarolProcessor::slotOption(int slot) const {
    const int n = (int) options().size();
    return juce::jlimit(0, n - 1, pi(cId(juce::jlimit(0, kMaxSlots - 1, slot))));
}

std::vector<ChordOption> FarolProcessor::chosen() const {
    auto o = options();
    std::vector<ChordOption> v;
    for (int i = 0; i < slots(); ++i) v.push_back(o[(size_t) slotOption(i)]);
    return v;
}

void FarolProcessor::loadExample(int index) {
    auto ex = exampleProgressions(key().scale);
    if (ex.empty()) return;
    auto& p = ex[(size_t) juce::jlimit(0, (int) ex.size() - 1, index)];
    setParam("slots", (float) p.size());
    for (size_t i = 0; i < p.size(); ++i) setParam(cId((int) i), (float) p[i]);
}

void FarolProcessor::regenerate() {
    auto v = std::make_shared<Seq>();
    for (const auto& c : chosen()) {
        v->chords.push_back(c.midi);
        v->names.push_back(c.name);
    }
    {
        juce::SpinLock::ScopedLockType sl(dataLock);
        if (published) graveyard.push_back(published);
        published = v;
    }
    graveyard.erase(std::remove_if(graveyard.begin(), graveyard.end(), [](const auto& p) { return p.use_count() == 1; }), graveyard.end());
}

void FarolProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    {
        juce::SpinLock::ScopedTryLockType tl(dataLock);
        if (tl.isLocked()) audioData = published;
    }
    bool playing = false, hasPpq = false;
    double ppq = 0.0, bpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; hasPpq = true; }
            if (auto b = pos->getBpm()) bpm = *b;
        }
    if (bpm > 0) lastHostBpm = bpm; else bpm = (double) pBpm->load();
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double bps = bpm / 60.0 / sr;
    const bool audition = pAudition->load() > 0.5f;
    bool useClock = false;
    double p0 = 0.0;
    if (playing && hasPpq) { useClock = true; p0 = ppq; }
    else if (audition) {
        if (!lastAudition) auditionBeat = 0.0;
        useClock = true;
        p0 = auditionBeat;
        auditionBeat += n * bps;
    }
    lastAudition = audition;
    const double p1 = p0 + n * bps;
    double from = p0;
    outMidi.clear();
    nEv = 0;
    auto addEv = [&](int off, bool on, int note, int vel) {
        if (nEv >= 256) return;
        Ev e{juce::jlimit(0, n - 1, off), on, note, vel};
        int j = nEv++;
        while (j > 0 && ev[j - 1].off > e.off) { ev[j] = ev[j - 1]; --j; }
        ev[j] = e;
        auto m = on ? juce::MidiMessage::noteOn(1, note, (juce::uint8) vel) : juce::MidiMessage::noteOff(1, note);
        outMidi.addEvent(m, e.off);
    };
    auto releaseAll = [&](int off) {
        for (int nt : sounding) addEv(off, false, nt, 0);
        sounding.clear();
    };
    const Seq* sq = audioData.get();
    const int count = sq ? (int) sq->chords.size() : 0;
    if (useClock && count > 0) {
        const bool cont = seqRunning && std::abs(p0 - expectedP0) <= 0.05;
        if (cont) from = expectedP0; else releaseAll(0);
        expectedP0 = p1;
        seqRunning = true;
        long long k = (long long) std::ceil(from / 4.0 - 1e-9);
        for (; (double) k * 4.0 < p1; ++k) {
            const double t = (double) k * 4.0;
            if (t < from - 1e-9) continue;
            const int off = (int) ((t - p0) / bps);
            releaseAll(off);
            const auto& ch = sq->chords[(size_t) (((k % count) + count) % count)];
            for (int nt : ch) { addEv(off, true, nt, 92); sounding.push_back(nt); }
        }
        uiChord = (int) (((long long) std::floor(p0 / 4.0) % count + count) % count);
    } else {
        if (seqRunning) releaseAll(0);
        seqRunning = false;
        uiChord = -1;
    }
    // teclado ao vivo: toca no synth e regista a última nota para o modo "explica-me"
    for (const auto meta : midi) {
        auto m = meta.getMessage();
        if (m.isNoteOn()) {
            addEv(meta.samplePosition, true, m.getNoteNumber(), m.getVelocity());
            lastNoteVal = m.getNoteNumber();
            ++lastNoteCounter;
        } else if (m.isNoteOff()) addEv(meta.samplePosition, false, m.getNoteNumber(), 0);
    }
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;
    int pos = 0;
    for (int i = 0; i < nEv; ++i) {
        if (ev[i].off > pos) { synth.render(L + pos, R + pos, ev[i].off - pos); pos = ev[i].off; }
        if (ev[i].on) synth.noteOn(ev[i].note, ev[i].vel); else synth.noteOff(ev[i].note);
    }
    if (n > pos) synth.render(L + pos, R + pos, n - pos);
    const float vol = pVol->load();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* d = buffer.getWritePointer(ch);
        for (int i = 0; i < n; ++i) d[i] = std::tanh(d[i] * vol * 1.5f);
    }
    midi.swapWith(outMidi);
}

juce::String FarolProcessor::suggestedFileName(const juce::String& what) const {
    return "Farol_" + juce::String(noteName(key().root, key())).replace("#", "s") + "_" + what;
}

bool FarolProcessor::writeMidiFile(const juce::File& f) const {
    const auto ch = chosen();
    if (ch.empty()) return false;
    const int tpq = 960;
    juce::MidiMessageSequence conductor;
    conductor.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4));
    const double bpm = lastHostBpm.load() > 0 ? lastHostBpm.load() : (double) pf("bpm");
    conductor.addEvent(juce::MidiMessage::tempoMetaEvent((int) std::lround(60000000.0 / bpm)));
    juce::MidiMessageSequence seq;
    seq.addEvent(juce::MidiMessage::textMetaEvent(3, "Farol"));
    for (size_t i = 0; i < ch.size(); ++i) {
        const double t0 = (double) i * 4 * tpq;
        conductor.addEvent(juce::MidiMessage::textMetaEvent(6, U8(ch[i].name + " (" + ch[i].roman + ")")), t0);
        for (int nt : ch[i].midi) {
            seq.addEvent(juce::MidiMessage::noteOn(1, nt, (juce::uint8) 92), t0);
            seq.addEvent(juce::MidiMessage::noteOff(1, nt), t0 + 4 * tpq - 4);
        }
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(tpq);
    mf.addTrack(conductor);
    mf.addTrack(seq);
    f.deleteFile();
    juce::FileOutputStream os(f);
    return os.openedOk() && mf.writeTo(os);
}

bool FarolProcessor::writeLessonFile(const juce::File& f) const {
    f.deleteFile();
    return f.replaceWithText(U8(lessonText(chosen(), options(), key(), seventh(), lang())), false, false, "\n");
}

void FarolProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void FarolProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    cancelPendingUpdate();
    regenerate();
    sendChangeMessage();
}

juce::AudioProcessorEditor* FarolProcessor::createEditor() { return new FarolEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FarolProcessor(); }
