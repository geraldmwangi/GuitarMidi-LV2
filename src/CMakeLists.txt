set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_BINARY_DIR}/core/include)
include_directories(/usr/include)
include_directories(include)
include_directories(${PROJECT_SOURCE_DIR}/core/include)
find_package(PkgConfig REQUIRED)
find_library(ZITA_RESAMPLER_LIB zita-resampler)
message("Found zita-resampler library: ${ZITA_RESAMPLER_LIB}")
#find_package(OpenMP REQUIRED)
# pkg_check_modules(Pango REQUIRED pango)

# add_subdirectory(tflite_custom_op_example)

# Read TF_VERSION file to get TensorFlow version






set(SRC guitarmidi.cpp )

add_library(guitarmidi SHARED ${SRC})


target_link_libraries(guitarmidi PUBLIC guitarmidi_core )






install(TARGETS guitarmidi
        DESTINATION guitarmidi.lv2)

if(WITH_AUDIO_OUTPUT)
#rename ../guitarmidi-audiooutput.ttl to /tmp/guitarmidi.ttl then copy to the install directory, this is a workaround to avoid having to maintain two separate ttl files with only minor differences 
file(REMOVE  /tmp/guitarmidi.ttl)
file(COPY_FILE ../guitarmidi-audiooutput.ttl /tmp/guitarmidi.ttl)

install(FILES ../manifest.ttl /tmp/guitarmidi.ttl ../python/neuralnetmodelling/guitarmidi.tflite
        DESTINATION guitarmidi.lv2)
else()
install(FILES ../manifest.ttl ../guitarmidi.ttl ../python/neuralnetmodelling/guitarmidi.tflite
        DESTINATION guitarmidi.lv2)
endif()