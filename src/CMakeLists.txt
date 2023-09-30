set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
configure_file(include/config.hpp.in include/config.hpp)
include_directories(${PROJECT_BINARY_DIR}/src/include)
include_directories(/usr/include)
include_directories(include)

set(SRC guitarmidi.cpp  noteclassifier.cpp midioutput.cpp fretboard.cpp harmonicgroup.cpp logging.cpp)

add_library(guitarmidi SHARED ${SRC})
target_link_libraries(guitarmidi dsp aubio)

install(TARGETS guitarmidi
        DESTINATION guitarmidi.lv2)

install(FILES ../manifest.ttl ../guitarmidi.ttl
        DESTINATION guitarmidi.lv2)