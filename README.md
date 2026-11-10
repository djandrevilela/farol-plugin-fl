# Farol (VST3) — aprende enquanto produzes · learn while you produce

**PT-PT** e **EN-GB** na mesma interface (menu de idioma no topo). Plugin irmão do Harmonia, Alvorada e Zenite.
Tudo é **determinístico e baseado em teoria** (sem aleatoriedade, sem IA): cada frase vem de regras de harmonia funcional
(graus da escala, função do acorde, movimento da raiz, notas comuns, círculo de quintas).
Both languages share one interface (language menu at the top). Everything is deterministic and theory-based (no randomness, no AI).
No copied songs, no artist or product names.

## 1. Modo "explica-me" · "Explain me" mode
- Escolhes o tom, a escala (maior, menor natural, dórico, mixolídio, menor harmónica) e até 8 acordes (com ou sem sétimas).
  Pick the key, the scale and up to 8 chords (triads or 7ths).
- Cada acorde mostra: **função** (tónica, subdominante, dominante...), **tensão 0–4**, **sensação**, **porquê** (ex.: "contém a sensível, que quer subir para a tónica: por isso cria tensão"),
  uma **dica de produção** e, a partir do 2.º acorde, **porque é que a transição funciona** (movimento da raiz, notas partilhadas, cadência autêntica/plagal/enganadora/modal).
  Each chord shows its function, tension, feel, *why*, a production tip and why the transition from the previous chord works.
- **"A seguir podes ir para"**: até 3 acordes seguintes, cada um com o motivo; um clique aplica-o ao slot seguinte.
  **What can come next**: up to 3 follow-up chords with reasons; one click applies it.
- **Resumo da progressão** (curva de tensão, ponto mais tenso, cadência final, se soa resolvida ou em loop) e **Exemplos** (4 progressões genéricas por família de escala).
- **A tua última nota**: toca no teclado MIDI e o Farol diz o grau da escala, se é estável ou instável, a que acorde pertence (raiz/terça/quinta/sétima) e para onde resolver.
  Play a note on your MIDI keyboard and Farol tells you its scale degree, stability, role in the chord and where to resolve.
- Os acordes tocam com um som de pré-escuta (em loop com **▶ Ouvir**, ou a seguir o transporte do FL). Arrasta o **MIDI dos acordes** para o piano roll e a **lição (.txt)** com todas as explicações.

## 2. Compatibilidade de samples · Sample compatibility
Indica o tom da sample (maior, menor ou **nota única**, como um vocal chop) e, se souberes, o BPM. O Farol diz:
- **quantos semitons transpor** (o mínimo que dá o melhor encaixe com a escala do projeto; tonalidades relativas não precisam de transposição);
- que notas ficam em comum, se uma nota destoa e o que evitar;
- **que acorde do teu tom** passa a ser a tónica da sample e **que acordes combinam** por baixo (com a nota como raiz, terça ou quinta);
- a alternativa de **mudar o tom do projeto** em vez da sample, e se a transposição é segura para vozes;
- **o esticar de BPM** (também em meio tempo/dobro) e quando é melhor cortar em chops.
Tell Farol the sample's key (major, minor or a single note such as a vocal chop) and optionally its BPM, and it says how many semitones to transpose,
which notes clash, which chord of your key the sample's tonic becomes, which chords fit underneath, the alternative of moving the project key, and BPM stretching advice.

## Usar no FL Studio · Using it in FL Studio
1. Copia `Farol.vst3` para `C:\Program Files\Common Files\VST3\` e faz *Find installed plugins*. Copy it there and rescan.
2. Adiciona o Farol como gerador. **▶ Ouvir** toca a progressão sem teres de dar play ao FL. Add Farol as a generator; **▶ Listen** plays the progression on its own.
3. A saída MIDI do plugin leva os acordes (canal 1) para outro instrumento. The plugin's MIDI output carries the chords (channel 1).

## Compilar · Build
- **GitHub Actions**: push da pasta, separador *Actions* → artefacto `Farol-VST3`.
- **Windows (VS 2022 + CMake + Git)**:
  ```
  cmake -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release --target Farol_VST3
  ```
- **Só motor + testes**: `cmake -B build -DFAROL_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`
- **Teste de áudio offline** (opcional): `-DFAROL_BUILD_RENDER_TEST=ON` → `FarolRenderTest`.

## Estado e limites · Status and limits
- Motor testado (94 mil verificações: acordes de todos os tons/escalas, textos em PT e EN para todos os acordes/transições/notas, transposição ótima de samples). Compila em Linux e o teste de áudio offline passa.
- **Não foi ouvido nem testado dentro do FL Studio** (interface, encaixe visual dos textos, arrastar ficheiros). Not tested inside FL Studio.
- As explicações seguem a harmonia funcional ocidental (pop/EDM); não substituem um professor, mas cobrem o essencial. The explanations follow Western functional harmony.
- A deteção do tom da sample é **manual** (tu indicas o tom); o plugin não analisa áudio. Sample key is entered manually; the plugin does not analyse audio.
- JUCE é AGPLv3 ou comercial. O nome "Farol" é provisório: confirma que está livre. JUCE is AGPLv3 or commercial; the name is provisional.
