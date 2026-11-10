#pragma once
// Farol core: teoria, explicações ("explica-me") e compatibilidade de samples. C++17 puro, determinístico, bilingue (pt-PT / en-GB).
#include <string>
#include <vector>

namespace farol {

enum class Lang { PT, EN };
enum class ScaleType { Major, Minor, Dorian, Mixolydian, HarmonicMinor };
constexpr int kNumScales = 5;
enum class Quality { Major, Minor, Dim, Aug };
enum class Role { Tonic, Supertonic, Mediant, Subdominant, DominantMajor, DominantMinor, Submediant, Leading, Subtonic };

struct Key { int root = 9; ScaleType scale = ScaleType::Minor; };

std::string L(Lang l, const char* pt, const char* en);
std::string scaleName(ScaleType, Lang);
std::string roleName(Role, Lang);
std::string noteName(int pc, const Key&);
bool isMajorFamily(ScaleType);
std::vector<int> scalePcs(const Key&);  // 7 classes de altura absolutas, a partir da tónica

struct ChordOption {
    int degree = 0;         // 0..6 diatónico, -1 emprestado
    int rootPc = 0;         // absoluto
    Quality quality = Quality::Major;
    Role role = Role::Tonic;
    bool borrowed = false;
    int tension = 0;        // 0..4
    std::string roman;      // com sétima se ativa
    std::string name;       // ex.: "Am7"
    std::vector<int> pcs;   // classes de altura (raiz primeiro)
    std::vector<int> midi;  // posição fechada, raiz entre 48 e 59
    int seventhInterval = 0;  // 0 = sem sétima
};

// Opções de acorde do tom: 0..6 graus, depois emprestados (Maior: bVII, bVI, iv; Menor/Dórico: V)
std::vector<ChordOption> chordOptions(const Key&, bool seventh);

struct Explanation {
    std::string name, roman, role, mood, why, extra, tip;
    int tension = 0;
};
Explanation explainChord(const ChordOption&, const Key&, bool seventh, Lang);
std::string explainTransition(const ChordOption& prev, const ChordOption& cur, const Key&, Lang);
struct Suggestion { int option = 0; std::string reason; };
std::vector<Suggestion> suggestNext(const std::vector<ChordOption>& opts, int current, const Key&, Lang);
std::string explainProgression(const std::vector<ChordOption>& chosen, const Key&, Lang);
std::string explainNote(int midiNote, const Key&, const ChordOption* chord, Lang);

// Progressões de exemplo (índices de opções), 4 por família de escala
std::vector<std::vector<int>> exampleProgressions(ScaleType);
const char* exampleName(ScaleType, int index, Lang);

// ---- Compatibilidade de samples ----
enum class SampleKind { Major, Minor, SingleNote };
struct SampleIn { int root = 0; SampleKind kind = SampleKind::Minor; double bpm = 0; };
struct SampleAdvice {
    int shift = 0;            // semitons a aplicar à sample (-6..+6)
    int overlap = 7;          // notas em comum (0..7) depois de transpor
    std::string headline;     // frase curta
    std::vector<std::string> lines;
};
SampleAdvice adviseSample(const SampleIn&, const Key& project, double projectBpm, Lang);

// Texto completo da lição (exportação)
std::string lessonText(const std::vector<ChordOption>& chosen, const std::vector<ChordOption>& all, const Key&, bool seventh, Lang);

}  // namespace farol
