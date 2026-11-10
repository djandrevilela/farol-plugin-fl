#pragma once
// Sintetizador de pré-escuta, polifónico e simples (saws desafinados + filtro + envolvente). DSP puro.
#include <array>
#include <cmath>

class PreviewSynth {
public:
    void prepare(double sampleRate) { sr = sampleRate; allOff(true); }
    void allOff(bool hard = false) {
        for (auto& v : voices) {
            if (hard) v.active = false;
            else v.gate = false;
        }
    }
    void noteOn(int note, int vel) {
        Voice* v = nullptr;
        for (auto& x : voices) if (!x.active) { v = &x; break; }
        if (!v) { v = &voices[next++ % voices.size()]; }
        v->active = true; v->gate = true; v->note = note; v->env = 0; v->vel = vel / 127.f;
        v->p1 = v->p2 = 0; v->lp = 0;
    }
    void noteOff(int note) {
        for (auto& v : voices) if (v.active && v.gate && v.note == note) v.gate = false;
    }
    void render(float* L, float* R, int n) {
        const float aInc = 1.f / (0.012f * (float) sr), rDec = std::exp(-1.f / (0.28f * (float) sr));
        const float a = 1.f - std::exp(-6.2831853f * 2200.f / (float) sr);
        for (auto& v : voices) {
            if (!v.active) continue;
            const double f = 440.0 * std::pow(2.0, (v.note - 69) / 12.0);
            const double i1 = f * 0.9966 / sr, i2 = f * 1.0034 / sr;
            for (int i = 0; i < n; ++i) {
                v.p1 += i1; v.p1 -= std::floor(v.p1);
                v.p2 += i2; v.p2 -= std::floor(v.p2);
                float s = (float) (v.p1 + v.p2 - 1.0);  // dois saws
                v.lp += a * (s - v.lp);
                if (v.gate) v.env = std::min(1.f, v.env + aInc);
                else { v.env *= rDec; if (v.env < 1e-4f) { v.active = false; break; } }
                const float o = v.lp * v.env * v.vel * 0.16f;
                L[i] += o;
                R[i] += o;
            }
        }
    }

private:
    struct Voice { bool active = false, gate = false; int note = 60; float env = 0, vel = 1, lp = 0; double p1 = 0, p2 = 0; };
    std::array<Voice, 24> voices;
    size_t next = 0;
    double sr = 44100;
};
