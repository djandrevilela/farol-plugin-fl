cmake_minimum_required(VERSION 3.22)
project(Farol VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()

option(FAROL_BUILD_PLUGIN "Compilar o plugin VST3 (descarrega o JUCE)" ON)
option(FAROL_COPY_AFTER_BUILD "Copiar o VST3 para a pasta de plugins do sistema após compilar" OFF)
option(FAROL_BUILD_RENDER_TEST "Compilar o teste de áudio offline (precisa do JUCE)" OFF)

set(FAROL_CORE_SOURCES core/Farol.cpp)

# --- Motor + testes (sem JUCE) ---
add_library(farol_core STATIC ${FAROL_CORE_SOURCES})
target_include_directories(farol_core PUBLIC core)
add_executable(farol_core_tests tests/core_tests.cpp)
target_link_libraries(farol_core_tests PRIVATE farol_core)
enable_testing()
add_test(NAME core COMMAND farol_core_tests)

# --- Plugin VST3 ---
if(FAROL_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)

  juce_add_plugin(Farol
    COMPANY_NAME "Harmonia Suite"
    PLUGIN_MANUFACTURER_CODE Hrml
    PLUGIN_CODE Far1
    FORMATS VST3
    PRODUCT_NAME "Farol"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${FAROL_COPY_AFTER_BUILD})

  target_sources(Farol PRIVATE ${FAROL_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
  target_include_directories(Farol PRIVATE core plugin)
  target_compile_definitions(Farol PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(Farol
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

  if(FAROL_BUILD_RENDER_TEST)
    juce_add_console_app(FarolRenderTest PRODUCT_NAME "FarolRenderTest")
    target_sources(FarolRenderTest PRIVATE ${FAROL_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp tests/render_test.cpp)
    target_include_directories(FarolRenderTest PRIVATE core plugin)
    target_compile_definitions(FarolRenderTest PRIVATE
      JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(FarolRenderTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
  endif()
endif()
