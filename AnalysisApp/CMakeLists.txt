#cmake_minimum_required(VERSION 3.6)
#project("GuitarMidiAnalysisApp")

find_package(PkgConfig)
configure_file(${CMAKE_SOURCE_DIR}/src/include/config.hpp.in include/config.hpp)
include_directories(${PROJECT_BINARY_DIR}/src/include)
pkg_check_modules (FT freetype2)
pkg_check_modules (ALSA alsa)
pkg_check_modules (CURL libcurl)
pkg_check_modules (WEBKITGTK webkit2gtk-4.0)
pkg_check_modules (GTKX11 gtk+-x11-3.0)
# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# I should use pkg-config (https://cmake.org/cmake/help/latest/module/FindPkgConfig.html) to find all modules for juce, since pkg-config is used 
#in the juce make file

include_directories(${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode 
${CMAKE_SOURCE_DIR}/AnalysisApp/Source
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/modules 
${CMAKE_SOURCE_DIR}/src/include
${FT_INCLUDE_DIRS} 
${ALSA_INCLUDE_DIRS} 
${CURL_INCLUDE_DIRS} 
${WEBKITGTK_INCLUDE_DIRS} 
${GTKX11_INCLUDE_DIRS})

set(JUCELIBSRC ${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_audio_basics.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_audio_devices.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_audio_formats.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_audio_processors.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_audio_utils.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_core.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_data_structures.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_dsp.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_events.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_graphics.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_gui_basics.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/JuceLibraryCode/include_juce_gui_extra.cpp )


set(SRCS ${CMAKE_SOURCE_DIR}/AnalysisApp/Source/Main.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/MainComponent.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/MainArea.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/FilterResponse.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/GraphArea.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/PlotArea.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/Graph.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/ResponseGraph.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/PhaseGraph.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/FilteredAudioGraph.cpp
${CMAKE_SOURCE_DIR}/AnalysisApp/Source/WaveFileView.cpp)



add_executable(GuitarMidiAnalysisApp ${SRCS} ${JUCELIBSRC})

target_link_libraries(GuitarMidiAnalysisApp guitarmidi dsp dl rt pthread ${ALSA_LIBRARIES}  ${FT_LIBRARIES} ${CURL_LIBRARIES} ${WEBKITGTK_LIBRARIES})