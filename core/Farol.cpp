#include "Farol.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace farol {

std::string L(Lang l, const char* pt, const char* en) { return l == Lang::PT ? pt : en; }

static const int kIv[kNumScales][7] = {
    {0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}, {0, 2, 3, 5, 7, 9, 10}, {0, 2, 4, 5, 7, 9, 10}, {0, 2, 3, 5, 7, 8, 11}};

bool isMajorFamily(ScaleType t) { return t == ScaleType::Major || t == ScaleType::Mixolydian; }

std::string scaleName(ScaleType t, Lang l) {
    switch (t) {
        case ScaleType::Major: return L(l, "Maior", "Major");
        case ScaleType::Minor: return L(l, "Menor natural", "Natural minor");
        case ScaleType::Dorian: return L(l, "Dórico", "Dorian");
        case ScaleType::Mixolydian: return L(l, "Mixolídio", "Mixolydian");
        case ScaleType::HarmonicMinor: return L(l, "Menor harmónica", "Harmonic minor");
    }
    return "";
}

std::string roleName(Role r, Lang l) {
    switch (r) {
        case Role::Tonic: return L(l, "Tónica", "Tonic");
        case Role::Supertonic: return L(l, "Supertónica (prepara o dominante)", "Supertonic (prepares the dominant)");
        case Role::Mediant: return L(l, "Mediante (tónica suave)", "Mediant (soft tonic)");
        case Role::Subdominant: return L(l, "Subdominante", "Subdominant");
        case Role::DominantMajor: return L(l, "Dominante", "Dominant");
        case Role::DominantMinor: return L(l, "Dominante menor (modal)", "Minor dominant (modal)");
        case Role::Submediant: return L(l, "Superdominante (cor emocional)", "Submediant (emotional colour)");
        case Role::Leading: return L(l, "Sensível (diminuto)", "Leading-tone (diminished)");
        case Role::Subtonic: return L(l, "Subtónica", "Subtonic");
    }
    return "";
}

std::string noteName(int pc, const Key& k) {
    static const char* sh[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    static const char* fl[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
    pc = ((pc % 12) + 12) % 12;
    bool flats;
    const int r = k.root;
    if (isMajorFamily(k.scale)) {
        // tom maior equivalente: Mixolídio tem a tónica uma quinta acima da maior que partilha as notas
        int maj = k.scale == ScaleType::Major ? r : (r + 5) % 12;
        flats = maj == 5 || maj == 10 || maj == 3 || maj == 8 || maj == 1;
    } else {
        int rel = (r + 3) % 12;  // maior relativa
        if (k.scale == ScaleType::Dorian) rel = (r + 10) % 12;
        flats = rel == 5 || rel == 10 || rel == 3 || rel == 8 || rel == 1;
    }
    bool allWhite = true;
    for (int i = 0; i < 7; ++i) {
        int x = (k.root + kIv[(int) k.scale][i]) % 12;
        if (x == 1 || x == 3 || x == 6 || x == 8 || x == 10) allWhite = false;
    }
    if (allWhite && !flats) return pc == 6 ? "F#" : fl[pc];  // tons sem acidentes: notas cromáticas com bemóis (Bb, Eb, Ab)
    return flats ? fl[pc] : sh[pc];
}

std::vector<int> scalePcs(const Key& k) {
    std::vector<int> v;
    for (int i = 0; i < 7; ++i) v.push_back((k.root + kIv[(int) k.scale][i]) % 12);
    return v;
}

// ------------------------------------------------------------------ acordes
static Role roleFor(int d, Quality q) {
    switch (d) {
        case 0: return Role::Tonic;
        case 1: return Role::Supertonic;
        case 2: return Role::Mediant;
        case 3: return Role::Subdominant;
        case 4: return q == Quality::Major ? Role::DominantMajor : Role::DominantMinor;
        case 5: return Role::Submediant;
        default: return q == Quality::Dim ? Role::Leading : Role::Subtonic;
    }
}

static int tensionFor(Role r) {
    switch (r) {
        case Role::Tonic: return 0;
        case Role::Mediant: case Role::Submediant: return 1;
        case Role::Supertonic: case Role::Subdominant: case Role::Subtonic: case Role::DominantMinor: return 2;
        case Role::DominantMajor: return 3;
        case Role::Leading: return 4;
    }
    return 0;
}

static ChordOption build(const Key& k, int degree, int rootOffset, Quality q, Role role, bool borrowed, int sev, bool seventh) {
    ChordOption c;
    c.degree = degree;
    c.rootPc = (k.root + rootOffset) % 12;
    c.quality = q;
    c.role = role;
    c.borrowed = borrowed;
    c.tension = tensionFor(role);
    if (role == Role::DominantMajor && borrowed) c.tension = 3;
    int t = q == Quality::Major || q == Quality::Aug ? 4 : 3;
    int f = q == Quality::Dim ? 6 : q == Quality::Aug ? 8 : 7;
    c.pcs = {c.rootPc, (c.rootPc + t) % 12, (c.rootPc + f) % 12};
    c.seventhInterval = seventh ? sev : 0;
    if (seventh) c.pcs.push_back((c.rootPc + sev) % 12);
    static const char* up[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    static const char* lo[7] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
    std::string rn;
    if (borrowed) {
        if (isMajorFamily(k.scale)) rn = rootOffset == 10 ? "bVII" : rootOffset == 8 ? "bVI" : "iv";
        else rn = "V";
    } else {
        rn = (q == Quality::Major || q == Quality::Aug) ? up[degree] : lo[degree];
    }
    if (q == Quality::Dim) rn += "\xC2\xB0";
    if (q == Quality::Aug) rn += "+";
    if (seventh) rn += sev == 11 ? "maj7" : "7";
    c.roman = rn;
    std::string nm = noteName(c.rootPc, k);
    if (!seventh) {
        nm += q == Quality::Minor ? "m" : q == Quality::Dim ? "dim" : q == Quality::Aug ? "aug" : "";
    } else {
        switch (q) {
            case Quality::Major: nm += sev == 11 ? "maj7" : "7"; break;
            case Quality::Minor: nm += sev == 11 ? "m(maj7)" : "m7"; break;
            case Quality::Dim: nm += sev == 9 ? "dim7" : "m7b5"; break;
            case Quality::Aug: nm += sev == 11 ? "aug(maj7)" : "aug7"; break;
        }
    }
    c.name = nm;
    std::vector<int> sorted;
    const int base = 48 + c.rootPc;
    for (int pc : c.pcs) sorted.push_back(base + ((pc - c.rootPc + 12) % 12));
    std::sort(sorted.begin(), sorted.end());
    c.midi = sorted;
    return c;
}

std::vector<ChordOption> chordOptions(const Key& k, bool seventh) {
    const int* iv = kIv[(int) k.scale];
    std::vector<ChordOption> out;
    for (int d = 0; d < 7; ++d) {
        int r = iv[d], t = (iv[(d + 2) % 7] - r + 12) % 12, f = (iv[(d + 4) % 7] - r + 12) % 12, s = (iv[(d + 6) % 7] - r + 12) % 12;
        Quality q = (t == 4 && f == 7) ? Quality::Major : (t == 3 && f == 7) ? Quality::Minor : (t == 3 && f == 6) ? Quality::Dim : Quality::Aug;
        out.push_back(build(k, d, r, q, roleFor(d, q), false, s, seventh));
    }
    if (k.scale == ScaleType::Major) {
        out.push_back(build(k, -1, 10, Quality::Major, Role::Subtonic, true, 11, seventh));
        out.push_back(build(k, -1, 8, Quality::Major, Role::Submediant, true, 11, seventh));
        out.push_back(build(k, -1, 5, Quality::Minor, Role::Subdominant, true, 10, seventh));
    } else if (k.scale == ScaleType::Minor || k.scale == ScaleType::Dorian) {
        out.push_back(build(k, -1, 7, Quality::Major, Role::DominantMajor, true, 10, seventh));
    }
    return out;
}

// ------------------------------------------------------------------ explicações
static std::string moodFor(const ChordOption& c, const Key& k, Lang l) {
    const bool maj = c.quality == Quality::Major;
    const bool fam = isMajorFamily(k.scale);
    switch (c.role) {
        case Role::Tonic: return maj ? L(l, "brilhante e estável", "bright and settled") : L(l, "escuro mas estável", "dark but settled");
        case Role::Supertonic: return L(l, "suave, a preparar movimento", "gentle, preparing movement");
        case Role::Mediant: return L(l, "suave e sonhador", "soft and dreamy");
        case Role::Subdominant: return maj ? L(l, "aberto e luminoso", "open and bright") : L(l, "melancólico e profundo", "melancholic and deep");
        case Role::DominantMajor: return L(l, "tenso, pede resolução", "tense, asking for resolution");
        case Role::DominantMinor: return L(l, "melancólico, tensão suave", "melancholic, gentle tension");
        case Role::Submediant: return fam ? L(l, "emotivo, agridoce", "emotional, bittersweet") : L(l, "luminoso e esperançoso (o 'lift')", "bright and hopeful (the 'lift')");
        case Role::Leading: return L(l, "instável e urgente", "unstable and urgent");
        case Role::Subtonic: return L(l, "épico e aberto", "epic and open");
    }
    return "";
}

static std::string whyFor(const ChordOption& c, const Key& k, Lang l) {
    const bool fam = isMajorFamily(k.scale);
    switch (c.role) {
        case Role::Tonic:
            return L(l, "É a casa: todas as notas da escala apontam para este acorde, por isso soa a repouso e a resolução.",
                     "It is home: every note of the scale points back to this chord, so it sounds like rest and resolution.");
        case Role::Supertonic:
            return L(l, "Prepara o dominante: a raiz deste acorde está a uma quinta do dominante, e essa queda de quinta (ii para V) é um dos movimentos mais naturais da música.",
                     "It prepares the dominant: this chord's root sits a fifth above the dominant, and that falling fifth (ii to V) is one of the most natural motions in music.");
        case Role::Mediant:
            return L(l, "Partilha duas notas com a tónica, por isso soa a uma tónica mais suave: cria movimento sem te afastar de casa.",
                     "It shares two notes with the tonic, so it sounds like a softer tonic: it adds movement without leaving home.");
        case Role::Subdominant:
            return c.quality == Quality::Major
                       ? L(l, "Afasta-se de casa sem criar urgência: dá espaço e abertura antes de voltares à tónica ou subires ao dominante.",
                           "It moves away from home without urgency: it gives space and openness before you go back to the tonic or rise to the dominant.")
                       : L(l, "É um acorde menor a afastar-se de casa: a mesma função de abertura, mas com uma sombra melancólica (o 4.º grau menor).",
                           "A minor chord moving away from home: the same opening function, but with a melancholic shadow (the minor 4th degree).");
        case Role::DominantMajor:
            return L(l, "Contém a sensível, a nota a meio tom da tónica que quer subir para ela: é por isso que cria tensão e pede resolução.",
                     "It contains the leading note, the note a semitone below the tonic that wants to rise to it: that is why it creates tension and asks for resolution.");
        case Role::DominantMinor:
            return L(l, "Sem sensível (a nota a meio tom da tónica), o 5.º grau menor puxa menos: cria tensão suave e modal, mais melancólica do que urgente.",
                     "Without a leading note (the note a semitone below the tonic), the minor 5th degree pulls less: gentle, modal tension, more melancholic than urgent.");
        case Role::Submediant:
            return fam ? L(l, "É o relativo menor da tónica: partilha duas notas com ela e traz a emoção agridoce típica de versos e refrões.",
                           "It is the relative minor of the tonic: it shares two notes with it and brings the bittersweet emotion typical of verses and choruses.")
                       : L(l, "É o acorde maior que traz luz e subida dentro do tom menor (o 'lift'): partilha duas notas com a tónica mas muda a cor.",
                           "It is the major chord that brings light and lift inside the minor key: it shares two notes with the tonic but changes the colour.");
        case Role::Leading:
            return L(l, "Acorde diminuto: duas terças menores empilhadas, muito instável por natureza. A sensível lá dentro quer subir para a tónica, por isso pede resolução imediata.",
                     "A diminished chord: two stacked minor thirds, unstable by nature. The leading note inside it wants to rise to the tonic, so it asks for immediate resolution.");
        case Role::Subtonic:
            return L(l, "A raiz está um tom abaixo da tónica: dá uma abertura épica e volta à tónica sem a força do dominante.",
                     "Its root sits a whole step below the tonic: it gives an epic opening and returns to the tonic without the force of the dominant.");
    }
    return "";
}

static std::string tipFor(Role r, Lang l) {
    switch (r) {
        case Role::Tonic: return L(l, "Bom sítio para a melodia repousar em notas longas e para o drop assentar.", "A good place for the melody to rest on long notes and for the drop to land.");
        case Role::Supertonic: return L(l, "Mantém o baixo simples: este acorde existe para levar ao dominante.", "Keep the bass simple: this chord exists to lead to the dominant.");
        case Role::Mediant: return L(l, "Usa-o para mexer o pad sem o baixo se afastar muito.", "Use it to move the pad without the bass travelling far.");
        case Role::Subdominant: return L(l, "Bom momento para abrir o filtro ou alargar o stereo antes de subir.", "A good moment to open the filter or widen the stereo image before rising.");
        case Role::DominantMajor: return L(l, "Põe aqui o fill ou o riser e prende a melodia às notas do acorde.", "Put the fill or riser here and tie the melody to the chord notes.");
        case Role::DominantMinor: return L(l, "Bom para versos: tensão sem urgência. Segue para a tónica ou para o VI.", "Good for verses: tension without urgency. Follow with the tonic or the VI.");
        case Role::Submediant: return L(l, "Perfeito para versos e quebras emocionais (piano, pad).", "Perfect for verses and emotional breakdowns (piano, pad).");
        case Role::Leading: return L(l, "Usa-o curto (um tempo ou menos) e resolve logo na tónica.", "Keep it short (a beat or less) and resolve straight to the tonic.");
        case Role::Subtonic: return L(l, "Dá um refrão épico: experimenta tónica, este acorde, e de volta.", "Gives an epic chorus: try the tonic, this chord, and back again.");
    }
    return "";
}

Explanation explainChord(const ChordOption& c, const Key& k, bool seventh, Lang l) {
    Explanation e;
    e.name = c.name;
    e.roman = c.roman;
    e.role = roleName(c.role, l);
    e.mood = moodFor(c, k, l);
    e.why = whyFor(c, k, l);
    e.tip = tipFor(c.role, l);
    e.tension = c.tension;
    std::string x;
    if (c.borrowed) {
        if (isMajorFamily(k.scale))
            x += L(l, "Este acorde é emprestado do tom menor paralelo: traz uma cor mais escura e dramática a um tom maior. ",
                   "This chord is borrowed from the parallel minor: it brings a darker, more dramatic colour to a major key. ");
        else
            x += L(l, "Este acorde é emprestado da escala menor harmónica: a sensível que ele traz reforça a atração para a tónica. ",
                   "This chord is borrowed from the harmonic minor scale: the leading note it brings strengthens the pull to the tonic. ");
    }
    if (c.quality == Quality::Dim && c.role != Role::Leading)
        x += L(l, "É diminuto: instável por natureza, usa-o de passagem. ", "It is diminished: unstable by nature, use it in passing. ");
    if (c.quality == Quality::Aug)
        x += L(l, "É aumentado: soa ambíguo e sonhador, pede movimento. ", "It is augmented: ambiguous and dreamy, it asks for movement. ");
    if (seventh)
        x += L(l, "A sétima acrescenta cor e uma tensão extra: soa mais rico e menos 'simples'. ", "The seventh adds colour and a little extra tension: richer and less plain. ");
    e.extra = x;
    return e;
}

std::string explainTransition(const ChordOption& p, const ChordOption& c, const Key& k, Lang l) {
    std::string s;
    const int m = ((c.rootPc - p.rootPc) % 12 + 12) % 12;
    switch (m) {
        case 0: s += L(l, "A raiz mantém-se: só muda a cor do acorde. ", "The root stays: only the chord colour changes. "); break;
        case 5: s += L(l, "A raiz sobe uma quarta (ou desce uma quinta): é o movimento mais forte e natural entre acordes. ", "The root rises a fourth (or falls a fifth): the strongest, most natural motion between chords. "); break;
        case 7: s += L(l, "A raiz sobe uma quinta (ou desce uma quarta): soa a abertura e energia, menos conclusivo. ", "The root rises a fifth (or falls a fourth): it sounds open and energetic, less conclusive. "); break;
        case 2: case 10: s += L(l, "A raiz move-se um tom: movimento suave, de passagem. ", "The root moves by a whole step: smooth, passing motion. "); break;
        case 1: case 11: s += L(l, "A raiz move-se meio tom: movimento apertado e tenso. ", "The root moves by a semitone: tight, tense motion. "); break;
        case 6: s += L(l, "A raiz salta um trítono: salto muito tenso. ", "The root jumps a tritone: a very tense leap. "); break;
        default: s += L(l, "A raiz move-se por terças: os acordes partilham notas e a mudança soa suave. ", "The root moves by a third: the chords share notes and the change sounds smooth. "); break;
    }
    std::vector<std::string> common;
    for (int a : p.pcs)
        for (int b : c.pcs)
            if (a == b) { common.push_back(noteName(a, k)); break; }
    if (common.empty()) s += L(l, "Não partilham notas: a mudança é nítida. ", "They share no notes: the change is clear-cut. ");
    else {
        std::string names;
        for (size_t i = 0; i < common.size(); ++i) names += (i ? ", " : "") + common[i];
        s += L(l, "Partilham ", "They share ") + std::to_string(common.size()) + L(l, common.size() == 1 ? " nota (" : " notas (", common.size() == 1 ? " note (" : " notes (") + names +
             L(l, "): deixa-as no mesmo sítio para uma condução de vozes suave. ", "): keep them in place for smooth voice leading. ");
    }
    if (c.tension > p.tension) s += L(l, "A tensão sobe. ", "Tension rises. ");
    else if (c.tension < p.tension) s += L(l, "A tensão desce: sente-se alívio. ", "Tension falls: you feel relief. ");
    else s += L(l, "A tensão mantém-se. ", "Tension stays level. ");
    const bool pDom = p.role == Role::DominantMajor || p.role == Role::Leading;
    if (pDom && c.role == Role::Tonic) s += L(l, "Isto é uma resolução (cadência autêntica): a tensão do dominante é libertada na tónica.", "This is a resolution (authentic cadence): the dominant's tension is released on the tonic.");
    else if (p.role == Role::Subdominant && c.role == Role::Tonic) s += L(l, "Isto é uma cadência plagal (o 'amém'): suave e conclusiva.", "This is a plagal cadence (the 'amen'): gentle and conclusive.");
    else if (p.role == Role::DominantMajor && c.role == Role::Submediant) s += L(l, "Isto é uma cadência enganadora: esperavas a tónica, mas vem o acorde emocional: surpresa agradável.", "This is a deceptive cadence: you expected the tonic but the emotional chord arrives: a pleasant surprise.");
    else if (p.role == Role::Subtonic && c.role == Role::Tonic) s += L(l, "Isto é uma cadência modal (do tom abaixo para a tónica): épica e sem a força do dominante.", "This is a modal cadence (from the step below to the tonic): epic, without the dominant's force.");
    return s;
}

std::vector<Suggestion> suggestNext(const std::vector<ChordOption>& opts, int cur, const Key& k, Lang l) {
    std::vector<Suggestion> out;
    if (cur < 0 || cur >= (int) opts.size()) return out;
    const bool fam = isMajorFamily(k.scale);
    std::vector<Role> rank;  // DominantMajor representa "dominante" (cai para menor se não existir)
    switch (opts[(size_t) cur].role) {
        case Role::Tonic: rank = fam ? std::vector<Role>{Role::Subdominant, Role::Submediant, Role::Supertonic} : std::vector<Role>{Role::Subtonic, Role::Submediant, Role::Subdominant}; break;
        case Role::Supertonic: rank = {Role::DominantMajor, Role::Subdominant, Role::Leading}; break;
        case Role::Mediant: rank = {Role::Submediant, Role::Subdominant, Role::Supertonic}; break;
        case Role::Subdominant: rank = {Role::DominantMajor, Role::Tonic, Role::Supertonic}; break;
        case Role::DominantMajor: case Role::DominantMinor: rank = {Role::Tonic, Role::Submediant, Role::Subdominant}; break;
        case Role::Submediant: rank = {Role::Subdominant, Role::Supertonic, Role::DominantMajor}; break;
        case Role::Leading: rank = {Role::Tonic, Role::Submediant, Role::Mediant}; break;
        case Role::Subtonic: rank = {Role::Tonic, Role::Submediant, Role::Subdominant}; break;
    }
    for (Role r : rank) {
        int pick = -1;
        for (size_t i = 0; i < opts.size() && pick < 0; ++i)
            if ((int) i != cur && opts[i].role == r && !opts[i].borrowed) pick = (int) i;
        if (pick < 0)
            for (size_t i = 0; i < opts.size() && pick < 0; ++i)
                if ((int) i != cur && opts[i].role == r) pick = (int) i;
        if (pick < 0 && r == Role::DominantMajor)
            for (size_t i = 0; i < opts.size() && pick < 0; ++i)
                if ((int) i != cur && opts[i].role == Role::DominantMinor) pick = (int) i;
        if (pick < 0) continue;
        std::string why;
        switch (opts[(size_t) pick].role) {
            case Role::Tonic: why = L(l, "resolve a tensão e volta a casa", "resolves the tension and returns home"); break;
            case Role::Supertonic: why = L(l, "prepara o dominante", "prepares the dominant"); break;
            case Role::Mediant: why = L(l, "movimento suave por terças", "soft movement by thirds"); break;
            case Role::Subdominant: why = L(l, "afasta-se de casa com abertura", "moves away from home with openness"); break;
            case Role::DominantMajor: why = L(l, "sobe a tensão antes de resolver", "raises tension before resolving"); break;
            case Role::DominantMinor: why = L(l, "tensão suave e melancólica", "gentle, melancholic tension"); break;
            case Role::Submediant: why = L(l, "muda a emoção sem voltar a casa", "changes the emotion without returning home"); break;
            case Role::Leading: why = L(l, "urgência máxima: resolve logo a seguir na tónica", "maximum urgency: resolve to the tonic right after"); break;
            case Role::Subtonic: why = L(l, "abertura épica a um tom da tónica", "epic opening a whole step from the tonic"); break;
        }
        bool dup = false;
        for (auto& s : out) dup |= s.option == pick;
        if (!dup) out.push_back({pick, why});
    }
    return out;
}

std::string explainProgression(const std::vector<ChordOption>& ch, const Key& k, Lang l) {
    std::string s;
    if (ch.empty()) return s;
    s += L(l, "Tom: ", "Key: ") + noteName(k.root, k) + " " + scaleName(k.scale, l) + ". ";
    bool hasTonic = false;
    int peak = 0, peakIdx = 0;
    for (size_t i = 0; i < ch.size(); ++i) {
        hasTonic |= ch[i].role == Role::Tonic;
        if (ch[i].tension > peak) { peak = ch[i].tension; peakIdx = (int) i; }
    }
    s += L(l, "Curva de tensão (0 a 4): ", "Tension curve (0 to 4): ");
    for (size_t i = 0; i < ch.size(); ++i) s += (i ? " - " : "") + std::to_string(ch[i].tension);
    s += ". ";
    if (peak > 0) s += L(l, "O ponto de maior tensão é o acorde ", "The tensest point is chord ") + std::to_string(peakIdx + 1) + " (" + ch[(size_t) peakIdx].name + "). ";
    if (!hasTonic) s += L(l, "Não contém a tónica: soa suspensa, sem casa. ", "It does not contain the tonic: it sounds suspended, without a home. ");
    const auto& last = ch.back();
    if (last.role == Role::Tonic) s += L(l, "Termina na tónica: soa resolvida e fechada. ", "It ends on the tonic: it sounds resolved and closed. ");
    else if (last.role == Role::DominantMajor || last.role == Role::Leading || last.role == Role::DominantMinor)
        s += L(l, "Termina em tensão: em loop, o último acorde pede para voltar ao início, o que funciona muito bem em drops. ", "It ends on tension: in a loop the last chord asks to go back to the start, which works very well in drops. ");
    else s += L(l, "Termina num acorde aberto: em loop dá uma sensação de movimento contínuo. ", "It ends on an open chord: in a loop it gives a feeling of continuous movement. ");
    if (ch.size() >= 2) {
        const auto& pr = ch[ch.size() - 2];
        if (pr.role == Role::DominantMajor && last.role == Role::Tonic) s += L(l, "A cadência final é autêntica (dominante, tónica). ", "The final cadence is authentic (dominant, tonic). ");
        else if (pr.role == Role::Subdominant && last.role == Role::Tonic) s += L(l, "A cadência final é plagal (subdominante, tónica). ", "The final cadence is plagal (subdominant, tonic). ");
        else if (pr.role == Role::DominantMajor && last.role == Role::Submediant) s += L(l, "A cadência final é enganadora. ", "The final cadence is deceptive. ");
    }
    return s;
}

std::string explainNote(int midi, const Key& k, const ChordOption* chord, Lang l) {
    const int pc = ((midi % 12) + 12) % 12;
    const auto sc = scalePcs(k);
    int deg = -1;
    for (int i = 0; i < 7; ++i) if (sc[(size_t) i] == pc) deg = i;
    std::string s = noteName(pc, k) + ": ";
    static const char* degPt[7] = {"tónica", "supertónica", "mediante", "subdominante", "dominante", "superdominante", "sensível/subtónica"};
    static const char* degEn[7] = {"tonic", "supertonic", "mediant", "subdominant", "dominant", "submediant", "leading note/subtonic"};
    if (deg >= 0) {
        static const char* ord[7] = {"1st", "2nd", "3rd", "4th", "5th", "6th", "7th"};
        s += l == Lang::PT ? std::to_string(deg + 1) + ".\xC2\xBA grau (" : std::string(ord[deg]) + " degree (";
        s += l == Lang::PT ? degPt[deg] : degEn[deg];
        s += "). ";
        if (deg == 0 || deg == 2 || deg == 4) s += L(l, "Nota estável: soa segura e podes ficar nela. ", "Stable note: it sounds safe and you can rest on it. ");
        else if (deg == 3 || deg == 5) s += L(l, "Nota meio-estável: pede um pequeno movimento. ", "Semi-stable note: it asks for a small movement. ");
        else s += L(l, "Nota instável: cria tensão. ", "Unstable note: it creates tension. ");
        if (deg == 1) s += L(l, "Tende a descer para a tónica. ", "It tends to fall to the tonic. ");
        if (deg == 6 && ((sc[6] + 1) % 12) == sc[0]) s += L(l, "É a sensível: quer subir meio tom para a tónica. ", "It is the leading note: it wants to rise a semitone to the tonic. ");
        if (deg == 3) s += L(l, "Tende a descer para a terça (3.º grau). ", "It tends to fall to the 3rd degree. ");
        if (deg == 5) s += L(l, "Tende a descer para a quinta (5.º grau). ", "It tends to fall to the 5th degree. ");
    } else {
        int down = 0, up = 0;
        while (std::find(sc.begin(), sc.end(), ((pc - down) % 12 + 12) % 12) == sc.end()) ++down;
        while (std::find(sc.begin(), sc.end(), (pc + up) % 12) == sc.end()) ++up;
        s += L(l, "Fora da escala: nota cromática, dá cor e tensão. Resolve por meio tom para ", "Outside the scale: a chromatic note, it adds colour and tension. Resolve by a semitone to ") +
             noteName(down <= up ? pc - down : pc + up, k) + ". ";
        if (!chord) return s;
    }
    if (chord) {
        int idx = -1;
        for (size_t i = 0; i < chord->pcs.size(); ++i) if (chord->pcs[i] == pc) idx = (int) i;
        if (idx == 0) s += L(l, "É a raiz do acorde ", "It is the root of ") + chord->name + L(l, ": a nota mais sólida.", ": the most solid note.");
        else if (idx == 1) s += L(l, "É a terça do acorde ", "It is the third of ") + chord->name + L(l, ": define se soa maior ou menor, soa doce.", ": it defines major or minor and sounds sweet.");
        else if (idx == 2) s += L(l, "É a quinta do acorde ", "It is the fifth of ") + chord->name + L(l, ": soa aberta e neutra.", ": it sounds open and neutral.");
        else if (idx == 3) s += L(l, "É a sétima do acorde ", "It is the seventh of ") + chord->name + L(l, ": soa rica e a pedir movimento.", ": it sounds rich and asks for movement.");
        else if (deg >= 0) {
            int best = 99, bn = pc;
            for (int cp : chord->pcs) {
                int d1 = (cp - pc + 12) % 12, d2 = (pc - cp + 12) % 12;
                int d = std::min(d1, d2);
                if (d < best) { best = d; bn = cp; }
            }
            s += L(l, "Não pertence ao acorde ", "It is not part of ") + chord->name + L(l, ": é uma nota de passagem com tensão leve. Resolve para ", ": it is a passing note with mild tension. Resolve to ") + noteName(bn, k) + ".";
        }
    }
    return s;
}

// ------------------------------------------------------------------ exemplos
std::vector<std::vector<int>> exampleProgressions(ScaleType t) {
    switch (t) {
        case ScaleType::Major: return {{5, 3, 0, 4}, {0, 4, 5, 3}, {1, 4, 0, 5}, {0, 7, 3, 0}};
        case ScaleType::Mixolydian: return {{5, 3, 0, 4}, {0, 4, 5, 3}, {1, 4, 0, 5}, {0, 6, 3, 0}};
        case ScaleType::Minor: return {{0, 5, 2, 6}, {0, 3, 6, 2}, {0, 6, 5, 7}, {0, 3, 7, 0}};
        case ScaleType::Dorian: return {{0, 5, 2, 6}, {0, 3, 6, 2}, {0, 3, 0, 6}, {0, 3, 7, 0}};
        case ScaleType::HarmonicMinor: return {{0, 5, 2, 6}, {0, 3, 6, 2}, {0, 5, 3, 4}, {0, 3, 4, 0}};
    }
    return {};
}

const char* exampleName(ScaleType t, int i, Lang l) {
    const bool fam = isMajorFamily(t);
    static const char* mp[4] = {"Subida emocional", "Clássica de pop", "Cadência com supertónica", "Épica (tom abaixo)"};
    static const char* me[4] = {"Emotional lift", "Pop staple", "Supertonic cadence", "Epic (step below)"};
    static const char* np[4] = {"Subida épica", "Sombria com subdominante", "Descida dramática", "Cadência com tensão"};
    static const char* ne[4] = {"Epic rise", "Dark with subdominant", "Dramatic descent", "Cadence with tension"};
    i = std::max(0, std::min(3, i));
    return fam ? (l == Lang::PT ? mp[i] : me[i]) : (l == Lang::PT ? np[i] : ne[i]);
}

// ------------------------------------------------------------------ samples
SampleAdvice adviseSample(const SampleIn& in, const Key& proj, double projBpm, Lang l) {
    SampleAdvice a;
    const auto projPcs = scalePcs(proj);
    auto inProj = [&](int pc) { return std::find(projPcs.begin(), projPcs.end(), ((pc % 12) + 12) % 12) != projPcs.end(); };
    auto chordsWith = [&](int pc) {
        std::string s;
        auto opts = chordOptions(proj, false);
        for (const auto& c : opts) {
            if (c.borrowed) continue;
            for (size_t i = 0; i < 3; ++i)
                if (c.pcs[i] == pc) {
                    if (!s.empty()) s += ", ";
                    s += c.name + " (" + c.roman + ", " + (i == 0 ? L(l, "raiz", "root") : i == 1 ? L(l, "terça", "third") : L(l, "quinta", "fifth")) + ")";
                }
        }
        return s;
    };
    if (in.kind == SampleKind::SingleNote) {
        int best = 0;
        if (!inProj(in.root)) {
            int down = 0, up = 0;
            while (!inProj(in.root - down)) ++down;
            while (!inProj(in.root + up)) ++up;
            best = down <= up ? -down : up;
        }
        a.shift = best;
        a.overlap = best == 0 ? 7 : 6;
        const int pc = ((in.root + best) % 12 + 12) % 12;
        if (best == 0) a.headline = L(l, "A nota ", "The note ") + noteName(in.root, proj) + L(l, " já está no teu tom: não precisas de transpor.", " is already in your key: no need to transpose.");
        else a.headline = L(l, "Transpõe a sample ", "Transpose the sample ") + std::string(best > 0 ? "+" : "") + std::to_string(best) + L(l, " semitons (", " semitones (") + noteName(in.root, proj) + " -> " + noteName(pc, proj) + ").";
        if (best != 0) a.lines.push_back(L(l, "A nota original está fora da escala do projeto: soaria desafinada contra os acordes. Se a quiseres mesmo, usa-a como nota de tensão curta.",
                                           "The original note is outside the project scale: it would clash with the chords. If you really want it, use it as a short tension note."));
        a.lines.push_back(L(l, "Acordes do teu tom que a contêm: ", "Chords in your key that contain it: ") + chordsWith(pc) + ".");
        a.lines.push_back(L(l, "Como raiz o acorde soa mais estável; como terça soa doce; como quinta soa aberto.", "As the root the chord sounds most stable; as the third it sounds sweet; as the fifth it sounds open."));
    } else {
        const std::vector<int> iv = in.kind == SampleKind::Major ? std::vector<int>{0, 2, 4, 5, 7, 9, 11} : std::vector<int>{0, 2, 3, 5, 7, 8, 10};
        int bestS = 0, bestO = -1;
        for (int s = -6; s <= 6; ++s) {
            int o = 0;
            for (int x : iv) if (inProj(in.root + x + s)) ++o;
            if (o > bestO || (o == bestO && (std::abs(s) < std::abs(bestS) || (std::abs(s) == std::abs(bestS) && s < bestS)))) { bestO = o; bestS = s; }
        }
        a.shift = bestS;
        a.overlap = bestO;
        const int newRoot = ((in.root + bestS) % 12 + 12) % 12;
        const std::string sName = noteName(in.root, proj) + (in.kind == SampleKind::Major ? "" : "m");
        if (bestS == 0 && bestO == 7) {
            const bool same = newRoot == proj.root && (in.kind == SampleKind::Major) == isMajorFamily(proj.scale);
            a.headline = same ? L(l, "Mesmo tom: usa a sample sem transpor.", "Same key: use the sample as it is.")
                              : L(l, "Tonalidades relativas: usa a sample sem transpor.", "Relative keys: use the sample as it is.");
            if (!same) a.lines.push_back(L(l, "Partilham exatamente as mesmas 7 notas, só muda a nota 'casa'. A tónica da sample vai soar como um acorde diferente no teu tom.",
                                           "They share exactly the same 7 notes, only the 'home' note changes. The sample's tonic will sound like a different chord in your key."));
        } else {
            a.headline = L(l, "Transpõe a sample ", "Transpose the sample ") + (bestS > 0 ? "+" : "") + std::to_string(bestS) + L(l, " semitons (", " semitones (") + sName + " -> " + noteName(newRoot, proj) + (in.kind == SampleKind::Major ? "" : "m") + ").";
            a.lines.push_back(L(l, "Fica com ", "It ends up sharing ") + std::to_string(bestO) + L(l, " das 7 notas em comum com o teu tom.", " of the 7 notes with your key."));
            if (bestO == 6)
                for (int x : iv) if (!inProj(in.root + x + bestS)) a.lines.push_back(L(l, "Uma nota difere: ", "One note differs: ") + noteName(in.root + x + bestS, proj) + L(l, ". Evita acordes que a contrariem ou usa-a só em notas curtas.", ". Avoid chords that fight it or keep it to short notes."));
            if (bestO <= 5) a.lines.push_back(L(l, "Os tons estão distantes: considera outra sample ou mudar o tom do projeto.", "The keys are far apart: consider another sample or changing the project key."));
            const int altRoot = ((proj.root - bestS) % 12 + 12) % 12;
            a.lines.push_back(L(l, "Alternativa: em vez de mexer na sample, muda o tom do projeto para ", "Alternative: instead of changing the sample, move the project key to ") + noteName(altRoot, proj) + " " + scaleName(proj.scale, l) + ".");
        }
        auto opts = chordOptions(proj, false);
        for (const auto& c : opts)
            if (!c.borrowed && c.rootPc == newRoot) a.lines.push_back(L(l, "A tónica da sample passa a ser o acorde ", "The sample's tonic becomes the chord ") + c.name + " (" + c.roman + L(l, ") do teu tom: usa esse acorde (e os vizinhos) por baixo.", ") of your key: use that chord (and its neighbours) underneath."));
        a.lines.push_back(L(l, "Acordes que combinam: ", "Chords that fit: ") + chordsWith(newRoot) + ".");
    }
    const int ab = std::abs(a.shift);
    if (ab == 0) a.lines.push_back(L(l, "Sem transposição: o timbre mantém-se natural.", "No transposition: the timbre stays natural."));
    else if (ab <= 2) a.lines.push_back(L(l, "Transposição pequena (até 2 semitons): segura para vozes e instrumentos.", "Small shift (up to 2 semitones): safe for vocals and instruments."));
    else if (ab <= 4) a.lines.push_back(L(l, "Transposição média: nota-se no timbre (as vozes ficam mais 'esquilo' ao subir ou 'pesadas' ao descer). Se puderes, usa a opção que preserva o timbre (formantes).", "Medium shift: you will hear it in the timbre (vocals get 'chipmunk' going up or 'heavy' going down). If you can, use the option that preserves the timbre (formants)."));
    else a.lines.push_back(L(l, "Transposição grande: o timbre muda muito. Considera mudar o tom do projeto em vez da sample.", "Large shift: the timbre changes a lot. Consider moving the project key instead of the sample."));
    a.lines.push_back(L(l, "No FL Studio, o botão de afinação (pitch) do canal da sample aceita semitons.", "In FL Studio, the sample channel's pitch knob accepts semitones."));
    if (in.bpm > 0 && projBpm > 0) {
        const double mults[3] = {1.0, 0.5, 2.0};
        int bi = 0;
        double bd = 1e9;
        for (int i = 0; i < 3; ++i) {
            double d = std::abs(projBpm / (in.bpm * mults[i]) - 1.0);
            if (d < bd - 1e-12) { bd = d; bi = i; }
        }
        const double ratio = projBpm / (in.bpm * mults[bi]);
        const int pct = (int) std::lround((ratio - 1.0) * 100.0);
        std::string t = L(l, "BPM: ", "BPM: ") + std::to_string((int) std::lround(in.bpm)) + " -> " + std::to_string((int) std::lround(projBpm));
        if (bi == 1) t += L(l, " (usa a sample em meio tempo)", " (use the sample at half time)");
        if (bi == 2) t += L(l, " (usa a sample em dobro do tempo)", " (use the sample at double time)");
        t += L(l, ": esticar ", ": stretch ") + std::string(pct > 0 ? "+" : "") + std::to_string(pct) + "%. ";
        if (std::abs(pct) <= 3) t += L(l, "Praticamente sem perda de qualidade.", "Practically no quality loss.");
        else if (std::abs(pct) <= 8) t += L(l, "Aceitável, com pequenas marcas em notas longas.", "Acceptable, with slight artefacts on long notes.");
        else t += L(l, "Esticar tanto deixa artefactos: corta a sample em chops e reposiciona-os em vez de esticar.", "Stretching this much leaves artefacts: slice the sample into chops and reposition them instead.");
        a.lines.push_back(t);
    }
    return a;
}

std::string lessonText(const std::vector<ChordOption>& ch, const std::vector<ChordOption>&, const Key& k, bool seventh, Lang l) {
    std::string s = L(l, "FAROL - lição da tua progressão\n", "FAROL - lesson on your progression\n");
    s += explainProgression(ch, k, l) + "\n\n";
    for (size_t i = 0; i < ch.size(); ++i) {
        auto e = explainChord(ch[i], k, seventh, l);
        s += std::to_string(i + 1) + ". " + e.name + " (" + e.roman + ") - " + e.role + " - " + L(l, "tensão ", "tension ") + std::to_string(e.tension) + "/4\n";
        s += "   " + L(l, "Sensação: ", "Feel: ") + e.mood + "\n   " + L(l, "Porquê: ", "Why: ") + e.why + "\n";
        if (!e.extra.empty()) s += "   " + e.extra + "\n";
        s += "   " + L(l, "Dica de produção: ", "Production tip: ") + e.tip + "\n";
        if (i > 0) s += "   " + L(l, "Vindo de ", "Coming from ") + ch[i - 1].name + ": " + explainTransition(ch[i - 1], ch[i], k, l) + "\n";
        s += "\n";
    }
    return s;
}

}  // namespace farol
