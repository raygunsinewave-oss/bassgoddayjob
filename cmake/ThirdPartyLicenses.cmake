# Builds Resources text listing every third-party component that is compiled into
# the plugin, with versions read straight from the sources used for this build.
function(bgdj_generate_licenses JUCE_DIR OUT_FILE)
    file(STRINGS "${JUCE_DIR}/modules/juce_core/juce_core.h" _juce_ver REGEX "^[ ]*version:")
    string(REGEX REPLACE ".*version:[ ]*" "" _juce_ver "${_juce_ver}")

    set(_vst3_dir "${JUCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK")
    file(STRINGS "${_vst3_dir}/pluginterfaces/vst/vsttypes.h" _vst_ver REGEX "define kVstVersionString")
    string(REGEX REPLACE ".*\"(.*)\".*" "\\1" _vst_ver "${_vst_ver}")

    set(_txt "THIRD-PARTY COMPONENTS\n(generated at build time from the sources used for this build)\n\n")
    string(APPEND _txt "JUCE ${_juce_ver}\nCopyright (c) Raw Material Software Limited.\nUsed under the JUCE 8 licence (AGPLv3 or JUCE commercial / Starter licence).\nhttps://juce.com/legal/juce-8-licence/\n\n")
    string(APPEND _txt "Steinberg ${_vst_ver} SDK\nVST is a registered trademark of Steinberg Media Technologies GmbH.\n")
    if(EXISTS "${_vst3_dir}/LICENSE.txt")
        file(READ "${_vst3_dir}/LICENSE.txt" _vst_lic)
        string(APPEND _txt "${_vst_lic}\n")
    endif()
    string(APPEND _txt "\nAudio Unit SDK (Apple Inc.) - macOS builds only.\n\n")
    string(APPEND _txt "FONTS\nBASSGODSDAYJOB bundles no font files. Text is drawn with the operating system's default UI fonts, under their system licences.\n\n")
    string(APPEND _txt "END POEM\nEnd Poem by Julian Gough. Released into the public domain under CC0.\n")
    file(WRITE "${OUT_FILE}" "${_txt}")
endfunction()
