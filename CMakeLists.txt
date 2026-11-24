cmake_minimum_required(VERSION 3.22)

# ---- Platform defaults (must be set before project()) -----------------------
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()

project(BASSGODSDAYJOB VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(BGDJ_BUILD_TESTS "Build the DSP unit-test runner" ON)
set(BGDJ_JUCE_PATH "" CACHE PATH "Path to a local JUCE 8 checkout (empty = fetch JUCE 8.0.4)")
set(BGDJ_UPDATE_REPO "" CACHE STRING "GitHub owner/repo queried by 'Check for updates' (empty = feature reports it is not configured)")

# ---- JUCE --------------------------------------------------------------------
if(BGDJ_JUCE_PATH)
    add_subdirectory(${BGDJ_JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
    set(BGDJ_JUCE_SOURCE_DIR ${BGDJ_JUCE_PATH})
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(juce)
    set(BGDJ_JUCE_SOURCE_DIR ${juce_SOURCE_DIR})
endif()

# ---- Third-party license list, generated from the build inputs ---------------
include(cmake/ThirdPartyLicenses.cmake)
bgdj_generate_licenses(${BGDJ_JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated/ThirdPartyLicenses.txt)

# ---- Embedded resources (factory presets, credits text, licenses) -----------
file(GLOB_RECURSE BGDJ_PRESET_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Resources/presets/factory/*.bgdjpreset)
list(LENGTH BGDJ_PRESET_FILES BGDJ_PRESET_COUNT)
message(STATUS "BASSGODSDAYJOB: embedding ${BGDJ_PRESET_COUNT} factory presets")

juce_add_binary_data(BGDJ_BinaryData
    NAMESPACE BinaryData
    HEADER_NAME BinaryData.h
    SOURCES
        ${CMAKE_SOURCE_DIR}/Resources/credits/end_poem.txt
        ${CMAKE_BINARY_DIR}/generated/ThirdPartyLicenses.txt
        ${BGDJ_PRESET_FILES})

# ---- Source lists ------------------------------------------------------------
set(BGDJ_CORE_SOURCES
    Source/Common/Parameters.cpp
    Source/DSP/Wavetable.cpp
    Source/DSP/FactoryWavetables.cpp
    Source/DSP/NoiseBank.cpp
    Source/DSP/Filter.cpp
    Source/DSP/Tuning.cpp
    Source/Engine/ModMatrix.cpp
    Source/Engine/Shapes.cpp
    Source/Engine/Voice.cpp
    Source/Engine/SynthEngine.cpp
    Source/DSP/FX/FxModules.cpp
    Source/DSP/FX/FxRack.cpp
    Source/Analysis/Loudness.cpp
    Source/Analysis/FeatureExtractor.cpp
    Source/Scene/CrowdMood.cpp
    Source/Scene/Reactions.cpp
    Source/Presets/PresetFormat.cpp)

set(BGDJ_PLUGIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets/PresetManager.cpp
    Source/Presets/ExpansionManager.cpp
    Source/Settings/AppSettings.cpp
    Source/Settings/Diagnostics.cpp
    Source/Analysis/AnalysisData.cpp
    Source/Scene/ClubScene.cpp
    Source/Scene/SceneArt.cpp
    Source/Scene/CrowdSfx.cpp
    Source/UI/Theme.cpp
    Source/UI/Widgets.cpp
    Source/UI/Header.cpp
    Source/UI/PresetBrowser.cpp
    Source/UI/GifEncoder.cpp
    Source/UI/WavetableEditor.cpp
    Source/UI/CurveEditors.cpp
    Source/UI/Tabs/SynthTab.cpp
    Source/UI/Tabs/FxTab.cpp
    Source/UI/Tabs/VisualizerTab.cpp
    Source/UI/Tabs/Visualizers.cpp
    Source/UI/Tabs/TweaksTab.cpp
    Source/UI/Tabs/SettingsTab.cpp
    Source/UI/Tabs/CreditsTab.cpp)

set(BGDJ_COMMON_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    BGDJ_VERSION_STRING="${PROJECT_VERSION}"
    BGDJ_UPDATE_REPO="${BGDJ_UPDATE_REPO}"
    BGDJ_FACTORY_PRESET_COUNT=${BGDJ_PRESET_COUNT})

# ---- Plugin ------------------------------------------------------------------
juce_add_plugin(BASSGODSDAYJOB
    PRODUCT_NAME                "BASSGODSDAYJOB"
    COMPANY_NAME                "Raymond Duarte (Raygun)"
    COMPANY_WEBSITE             ""
    BUNDLE_ID                   com.raygun.bassgodsdayjob
    PLUGIN_MANUFACTURER_CODE    Rayg
    PLUGIN_CODE                 Bgdj
    DESCRIPTION                 "Bass synthesizer with a club that reacts to your sound"
    VERSION                     ${PROJECT_VERSION}
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    AU_SANDBOX_SAFE             TRUE
    HARDENED_RUNTIME_ENABLED    TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "The standalone app can use an audio input as the compressor sidechain.")

target_sources(BASSGODSDAYJOB PRIVATE ${BGDJ_CORE_SOURCES} ${BGDJ_PLUGIN_SOURCES})
target_include_directories(BASSGODSDAYJOB PRIVATE Source)
target_compile_definitions(BASSGODSDAYJOB PUBLIC ${BGDJ_COMMON_DEFS})
target_link_libraries(BASSGODSDAYJOB
    PRIVATE
        BGDJ_BinaryData
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# ---- DSP unit tests ------------------------------------------------------------
if(BGDJ_BUILD_TESTS)
    juce_add_console_app(BGDJ_Tests PRODUCT_NAME "BGDJ_Tests")
    target_sources(BGDJ_Tests PRIVATE ${BGDJ_CORE_SOURCES} Tests/TestMain.cpp Tests/DspTests.cpp Tests/EngineTests.cpp Tests/PresetTests.cpp Tests/SceneTests.cpp)
    target_include_directories(BGDJ_Tests PRIVATE Source)
    target_compile_definitions(BGDJ_Tests PRIVATE ${BGDJ_COMMON_DEFS} JUCE_UNIT_TESTS=1 BGDJ_TESTS=1)
    target_link_libraries(BGDJ_Tests
        PRIVATE
            BGDJ_BinaryData
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags)
    enable_testing()
    add_test(NAME BGDJ_DSP_Tests COMMAND BGDJ_Tests)
endif()
