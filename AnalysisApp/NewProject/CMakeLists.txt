cmake_minimum_required(VERSION 3.6)
project("GuitarMidiAnalysisApp")


# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# I should use pkg-config (https://cmake.org/cmake/help/latest/module/FindPkgConfig.html) to find all modules for juce, since pkg-config is used 
#in the juce make file

include_directories(${CMAKE_SOURCE_DIR}/JuceLibraryCode ${CMAKE_SOURCE_DIR}/JuceLibraryCode/modules /usr/include/freetype2)
set(JUCELIBSRC ${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_audio_basics.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_audio_devices.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_audio_formats.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_audio_processors.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_audio_utils.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_core.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_data_structures.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_dsp.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_events.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_graphics.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_gui_basics.cpp
${CMAKE_SOURCE_DIR}/JuceLibraryCode/include_juce_gui_extra.cpp )


set(SRCS ${CMAKE_SOURCE_DIR}/Source/Main.cpp
${CMAKE_SOURCE_DIR}/Source/MainComponent.cpp
${CMAKE_SOURCE_DIR}/Source/FFTElement.cpp)

add_executable(GuitarMidiAnalysisApp ${SRCS} ${JUCELIBSRC})

target_link_libraries(GuitarMidiAnalysisApp alsa freetype2 libcurl)