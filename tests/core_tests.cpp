#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

#include "Farol.h"

using namespace farol;
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); } } while (0)

int main() {
    // acordes diatónicos de Lá menor e Dó maior
    {
        auto m = chordOptions({9, ScaleType::Minor}, false);
        CHECK(m.size() == 8);
        const char* exp[7] = {"Am", "Bdim", "C", "Dm", "Em", "F", "G"};
        for (int i = 0; i < 7; ++i) CHECK(m[(size_t) i].name == exp[i]);
        CHECK(m[7].name == "E" && m[7].role == Role::DominantMajor && m[7].borrowed);
        auto M = chordOptions({0, ScaleType::Major}, false);
        CHECK(M.size() == 10);
        const char* exp2[10] = {"C", "Dm", "Em", "F", "G", "Am", "Bdim", "Bb", "Ab", "Fm"};
        for (int i = 0; i < 10; ++i) CHECK(M[(size_t) i].name == exp2[i]);
        auto m7 = chordOptions({9, ScaleType::Minor}, true);
        CHECK(m7[0].name == "Am7" && m7[4].name == "Em7" && m7[2].name == "Cmaj7" && m7[6].name == "G7" && m7[1].name == "Bm7b5");
        auto M7 = chordOptions({0, ScaleType::Major}, true);
        CHECK(M7[4].name == "G7" && M7[0].name == "Cmaj7");
    }
    // para todos os tons/escalas/sétimas: invariantes
    for (int root = 0; root < 12; ++root)
        for (int sc = 0; sc < kNumScales; ++sc)
            for (int sev = 0; sev < 2; ++sev)
                for (int lg = 0; lg < 2; ++lg) {
                    Key k{root, (ScaleType) sc};
                    Lang l = lg ? Lang::EN : Lang::PT;
                    auto opts = chordOptions(k, sev);
                    CHECK(opts.size() >= 7 && opts.size() <= 10);
                    auto pcs = scalePcs(k);
                    for (size_t i = 0; i < opts.size(); ++i) {
                        const auto& c = opts[i];
                        CHECK(c.pcs.size() == (sev ? 4u : 3u));
                        CHECK(c.midi.size() == c.pcs.size());
                        CHECK(c.midi.front() >= 48 && c.midi.front() <= 59 && c.midi.back() < 80);
                        CHECK(c.tension >= 0 && c.tension <= 4);
                        if (!c.borrowed)  // diatónico: todas as notas na escala
                            for (int pc : c.pcs) CHECK(std::find(pcs.begin(), pcs.end(), pc) != pcs.end());
                        auto e = explainChord(c, k, sev, l);
                        CHECK(!e.name.empty() && !e.roman.empty() && !e.role.empty() && !e.mood.empty() && !e.why.empty() && !e.tip.empty());
                        auto sg = suggestNext(opts, (int) i, k, l);
                        CHECK(!sg.empty() && sg.size() <= 3);
                        for (auto& s : sg) CHECK(s.option != (int) i && s.option < (int) opts.size() && !s.reason.empty());
                        for (size_t j = 0; j < opts.size(); ++j) CHECK(!explainTransition(opts[i], opts[j], k, l).empty());
                    }
                    CHECK(opts[0].role == Role::Tonic && opts[0].tension == 0);
                    auto ex = exampleProgressions(k.scale);
                    CHECK(ex.size() == 4);
                    for (auto& p : ex) for (int idx : p) CHECK(idx >= 0 && idx < (int) opts.size());
                    for (int i = 0; i < 4; ++i) CHECK(std::string(exampleName(k.scale, i, l)).size() > 3);
                    // determinismo
                    CHECK(explainChord(opts[1], k, sev, l).why == explainChord(opts[1], k, sev, l).why);
                    // notas: todas as 128 produzem texto
                    for (int n = 36; n < 96; ++n) { CHECK(!explainNote(n, k, nullptr, l).empty()); CHECK(!explainNote(n, k, &opts[0], l).empty()); }
                }
    // PT e EN diferem para o mesmo acorde
    {
        Key k{9, ScaleType::Minor};
        auto o = chordOptions(k, false);
        for (auto& c : o) CHECK(explainChord(c, k, false, Lang::PT).why != explainChord(c, k, false, Lang::EN).why);
        CHECK(explainChord(o[4], k, false, Lang::PT).why.find("sensível") != std::string::npos);
        CHECK(explainChord(o[4], k, false, Lang::EN).why.find("leading note") != std::string::npos);
        // transição Em -> Am é cadência com a quinta a cair
        auto t = explainTransition(o[4], o[0], k, Lang::EN);
        CHECK(t.find("fourth") != std::string::npos);
        auto r = explainTransition(o[7], o[0], k, Lang::PT);
        CHECK(r.find("cadência autêntica") != std::string::npos);
    }
    // explicação de notas
    {
        Key k{0, ScaleType::Major};
        auto o = chordOptions(k, false);
        CHECK(explainNote(71, k, nullptr, Lang::EN).find("leading note") != std::string::npos);
        CHECK(explainNote(64, k, &o[0], Lang::EN).find("third of C") != std::string::npos);
        CHECK(explainNote(62, k, &o[0], Lang::EN).find("not part of") != std::string::npos);
        CHECK(explainNote(61, k, nullptr, Lang::EN).find("Outside the scale") != std::string::npos);
        CHECK(explainNote(60, k, &o[0], Lang::PT).find("raiz") != std::string::npos);
    }
    // progressão: Am - F - C - G
    {
        Key k{9, ScaleType::Minor};
        auto o = chordOptions(k, false);
        std::vector<ChordOption> p = {o[0], o[5], o[2], o[6]};
        auto s = explainProgression(p, k, Lang::EN);
        CHECK(s.find("Tension curve") != std::string::npos);
        auto l1 = lessonText(p, o, k, false, Lang::PT), l2 = lessonText(p, o, k, false, Lang::PT);
        CHECK(l1 == l2 && l1.size() > 400);
    }
    // samples: exatidão da transposição
    {
        Key proj{9, ScaleType::Minor};  // Lá menor
        // sample em Ré menor -> Lá menor: tonalidades vizinhas, melhor +-5 mas com sobreposição
        // Dó maior é relativa de Lá menor: sem transposição
        auto a = adviseSample({0, SampleKind::Major, 0}, proj, 120, Lang::EN);
        CHECK(a.shift == 0 && a.overlap == 7);
        // mesmo tom
        a = adviseSample({9, SampleKind::Minor, 0}, proj, 120, Lang::PT);
        CHECK(a.shift == 0 && a.overlap == 7 && a.headline.find("Mesmo tom") != std::string::npos);
        // Si menor -> Lá menor: -2 (mesmas 7 notas de Lá menor? Si menor = Ré maior; Lá menor = Dó maior; -2 mapeia Ré maior para Dó maior)
        a = adviseSample({11, SampleKind::Minor, 0}, proj, 120, Lang::EN);
        CHECK(a.shift == -2 && a.overlap == 7);
        // regra geral: para qualquer sample maior/menor, o resultado transposto tem a máxima sobreposição e |shift|<=6
        for (int r = 0; r < 12; ++r)
            for (int kd = 0; kd < 2; ++kd)
                for (int pr = 0; pr < 12; ++pr)
                    for (int ps = 0; ps < kNumScales; ++ps) {
                        Key pj{pr, (ScaleType) ps};
                        auto adv = adviseSample({r, kd ? SampleKind::Major : SampleKind::Minor, 0}, pj, 120, Lang::EN);
                        CHECK(std::abs(adv.shift) <= 6 && adv.overlap >= 0 && adv.overlap <= 7 && !adv.headline.empty());
                        // nenhuma outra transposição com menor |shift| tem sobreposição melhor
                        auto pcs = scalePcs(pj);
                        const int iv[2][7] = {{0, 2, 3, 5, 7, 8, 10}, {0, 2, 4, 5, 7, 9, 11}};
                        for (int s = -6; s <= 6; ++s) {
                            int o = 0;
                            for (int x : iv[kd]) o += std::find(pcs.begin(), pcs.end(), (r + x + s + 24) % 12) != pcs.end();
                            CHECK(o <= adv.overlap);
                        }
                        auto pt = adviseSample({r, kd ? SampleKind::Major : SampleKind::Minor, 0}, pj, 120, Lang::PT);
                        CHECK(pt.shift == adv.shift);
                    }
        // nota única
        a = adviseSample({4, SampleKind::SingleNote, 0}, proj, 120, Lang::EN);  // Mi está em Lá menor
        CHECK(a.shift == 0);
        a = adviseSample({3, SampleKind::SingleNote, 0}, proj, 120, Lang::EN);  // Ré# fora da escala
        CHECK(a.shift != 0 && std::abs(a.shift) == 1);
        // BPM
        a = adviseSample({9, SampleKind::Minor, 100}, proj, 128, Lang::EN);
        bool found = false;
        for (auto& l : a.lines) found |= l.find("+28%") != std::string::npos;
        CHECK(found);
        a = adviseSample({9, SampleKind::Minor, 70}, proj, 140, Lang::EN);  // 70 -> dobro = 140
        found = false;
        for (auto& l : a.lines) found |= l.find("double time") != std::string::npos && l.find(" 0%") != std::string::npos;
        CHECK(found);
    }
    std::printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
