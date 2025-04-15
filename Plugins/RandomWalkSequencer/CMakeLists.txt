project(RandomWalkSequencer VERSION 0.0.1)

set (BaseTargetName RandomWalkSequencer)

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "davidRivas"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT TRUE  # Add this line explicitly
        AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE DaFR
        PLUGIN_CODE RWS1
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "RandomWalkSequencer")

juce_generate_juce_header(${BaseTargetName})

target_sources(${BaseTargetName} PRIVATE
        Source/PluginProcessor.cpp
        Source/RandomWalkSequencer.cpp
        Source/RandomWalkSequencerEditor.cpp)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0  # This is important
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_VST3=0
        # Add specific VST3 category for MIDI effects
        JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
        PRIVATE
        JUCE_MIDI_EFFECT=1)

target_link_libraries(${BaseTargetName} PRIVATE
        juce_audio_utils
        juce_audio_processors
        shared_plugin_helpers
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags)