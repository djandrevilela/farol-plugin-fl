# Farol — project principles (read before touching the code) / princípios do projeto

VST3 (JUCE) teaching plugin for EDM producers with little theory: "explain me" mode + sample key compatibility.
Sister of Harmonia (chords), Alvorada (melody/bass/synth) and Zenite (structure/energy).

## Non-negotiable rules (set by the author)
1. **No randomness, no AI.** Every explanation and suggestion is deterministic, built from music-theory rules
   (scale degrees, chord functions, root motion, common tones, circle of fifths). Same inputs => same outputs.
   Forbidden: `rand`, `std::random_*`, ML/LLM calls.
2. **Only things that work**: suggestions come from functional harmony; sample advice is computed from pitch-class sets.
3. **No copyright issues**: no copied songs, no artist or commercial product names. Only generic, unprotectable conventions.
4. **Bilingual**: every user-visible string exists in pt-PT and en-GB (`L(lang, pt, en)` in core). Tests check both.
5. `core/` is pure C++17 with tests in `tests/`; `plugin/` only orchestrates (JUCE).

## Structure
- `core/Farol`            theory, chord options, explanations, suggestions, note explanations, sample advice
- `plugin/Synth.h`        small polyphonic preview synth
- `plugin/PluginProcessor` chord sequencer (host clock or internal), live-note capture, MIDI export, lesson export
- `plugin/PluginEditor`   UI
