set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(/usr/include)
include_directories(include)
set(SRC guitarmidi.cpp  noteclassifier.cpp midioutput.cpp fretboard.cpp harmonicgroup.cpp)

add_library(guitarmidi SHARED ${SRC})
target_link_libraries(guitarmidi dsp aubio)

install(TARGETS guitarmidi
        DESTINATION guitarmidi.lv2)

install(FILES ../manifest.ttl ../guitarmidi.ttl
        DESTINATION guitarmidi.lv2)