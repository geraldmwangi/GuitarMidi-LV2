set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
configure_file(include/config.hpp.in include/config.hpp)
include_directories(${PROJECT_BINARY_DIR}/src/include)
include_directories(/usr/include)
include_directories(include)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Pango REQUIRED pango)

set(SRC guitarmidi.cpp  noteclassifier.cpp midioutput.cpp fretboard.cpp harmonicgroup.cpp logging.cpp)

add_library(guitarmidi SHARED ${SRC})
if(WITH_AUBIO=On)
message("Building with onset support")
target_link_libraries(guitarmidi dsp aubio ${Pango_LIBRARIES})
else()
message("Building without onset support")
target_link_libraries(guitarmidi dsp  ${Pango_LIBRARIES})
endif()

install(TARGETS guitarmidi
        DESTINATION guitarmidi.lv2)

install(FILES ../manifest.ttl ../guitarmidi.ttl
        DESTINATION guitarmidi.lv2)