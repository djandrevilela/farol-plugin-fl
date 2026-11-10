// Teste de áudio offline (opcional): sequenciador, notas ao vivo, exportações.
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    FarolProcessor proc;
    const double sr = 44100.0;
    const int bs = 512;
    proc.setPlayConfigDetails(0, 2, sr, bs);
    proc.prepareToPlay(sr, bs);
    juce::AudioBuffer<float> buf(2, bs);
    int fails = 0;
    proc.apvts.getParameter("audition")->setValueNotifyingHost(1.f);
    for (int sev = 0; sev < 2; ++sev) {
        proc.setParam("seventh", (float) sev);
        proc.setParam("slots", 4);
        proc.regenerateForTest();
        double peak = 0, ss = 0;
        long cnt = 0, ons = 0;
        bool bad = false;
        for (int b = 0; b < 1800; ++b) {  // ~20 s
            juce::MidiBuffer mb;
            if (b == 100) mb.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 5);
            buf.clear();
            proc.processBlock(buf, mb);
            for (const auto m : mb) if (m.getMessage().isNoteOn()) ++ons;
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < bs; ++s) {
                    float x = buf.getSample(ch, s);
                    if (!std::isfinite(x)) bad = true;
                    peak = std::max(peak, (double) std::abs(x));
                    ss += x * x;
                    ++cnt;
                }
        }
        const double rms = std::sqrt(ss / (double) cnt);
        const bool ok = !bad && peak > 0.02 && peak < 1.0 && rms > 0.003 && ons > 0 && proc.lastNote() == 64;
        std::printf("7ths=%d: pico %.3f rms %.4f notas %ld %s\n", sev, peak, rms, ons, ok ? "" : "<-- PROBLEMA");
        if (!ok) ++fails;
    }
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto f = tmp.getChildFile("farol_test.mid"), t = tmp.getChildFile("farol_test.txt");
    bool w1 = proc.writeMidiFile(f), w2 = proc.writeLessonFile(t);
    juce::MidiFile mf;
    juce::FileInputStream is(f);
    const bool rd = is.openedOk() && mf.readFrom(is) && mf.getNumTracks() == 2;
    std::printf("MIDI %s | lição %s | releitura %s\n", w1 ? "ok" : "FALHOU", w2 ? "ok" : "FALHOU", rd ? "ok" : "FALHOU");
    if (!w1 || !w2 || !rd) ++fails;
    std::printf(fails == 0 ? "Tudo OK.\n" : "%d problema(s).\n", fails);
    return fails == 0 ? 0 : 1;
}
