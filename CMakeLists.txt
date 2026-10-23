cmake_minimum_required(VERSION 3.22)

project(PBoard VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PBOARD_BUILD_TESTS "Build the offline DSP test" ON)

# JUCE is fetched automatically. To use a local copy instead, configure with
#   -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

set(PBOARD_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PBOARD_FORMATS AU)
endif()

juce_add_plugin(PBoard
    COMPANY_NAME                  "Tofu"
    BUNDLE_ID                     com.tofu.pboard
    PLUGIN_MANUFACTURER_CODE      Tofu
    PLUGIN_CODE                   Pbrd
    FORMATS                       ${PBOARD_FORMATS}
    PRODUCT_NAME                  "pboard"
    IS_SYNTH                      FALSE
    NEEDS_MIDI_INPUT              FALSE
    NEEDS_MIDI_OUTPUT             FALSE
    IS_MIDI_EFFECT                FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS   FALSE
    COPY_PLUGIN_AFTER_BUILD       FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT    "pboard needs audio input to process your guitar."
    VST3_CATEGORIES               Fx Distortion
    AU_MAIN_TYPE                  kAudioUnitType_Effect
)

target_sources(PBoard PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(PBoard PRIVATE Source)

target_compile_definitions(PBoard PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PBoard
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(PBOARD_BUILD_TESTS)
    enable_testing()
    add_executable(pboard_dsp_test tests/dsp_test.cpp)
    target_include_directories(pboard_dsp_test PRIVATE Source)
    add_test(NAME dsp_test COMMAND pboard_dsp_test)
endif()
