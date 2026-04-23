cmake_minimum_required(VERSION 3.15)
project(SymphonicApexMaster VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(SymphonicApexMaster
    COMPANY_NAME "Your Name DSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "YNDP"
    PLUGIN_CODE "Sam1" 
    FORMATS VST3
    PRODUCT_NAME "Symphonic Apex Master")

target_sources(SymphonicApexMaster PRIVATE PluginProcessor.cpp)

target_link_libraries(SymphonicApexMaster PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors)

target_compile_definitions(SymphonicApexMaster PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0)
