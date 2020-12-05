#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)

project(CRYPT_SYNTH_PLUGIN VERSION 0.3.0)

add_subdirectory(JUCE)

juce_add_plugin(CryptSynthPlugin
        VERSION 0.2.0                               # Set this if the plugin version is different to the project version
        ICON_BIG icon.png                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        ICON_SMALL icon.png
        COMPANY_NAME Vitling                          # Specify the name of the plugin's author
        IS_SYNTH TRUE                       # Is this a synth or an effect?
        NEEDS_MIDI_INPUT TRUE               # Does the plugin need midi input?
        NEEDS_MIDI_OUTPUT FALSE              # Does the plugin need midi output?
        IS_MIDI_EFFECT FALSE                 # Is this plugin a MIDI effect?
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE    # Does the editor need keyboard focus?
        COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
        PLUGIN_MANUFACTURER_CODE Vitl               # A four-character manufacturer id with at least one upper-case character
        PLUGIN_CODE Crpt                            # A unique four-character plugin id with at least one upper-case character
        DESCRIPTION "Hyper-Unison Synthesiser from Bow Church/Vitling"
        VST3_CATEGORIES "Instrument Synth Stereo"
        AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
        FORMATS VST3 AU                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
        PRODUCT_NAME "Crypt")        # The name of the final executable, which can differ from the target name


juce_generate_juce_header(CryptSynthPlugin)

target_sources(CryptSynthPlugin PRIVATE
        CryptPlugin.cpp)

target_compile_definitions(CryptSynthPlugin
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_binary_data(CryptSynthPluginData SOURCES ui.png)


target_link_libraries(CryptSynthPlugin PRIVATE
        CryptSynthPluginData
        juce::juce_audio_utils)