include_directories(${PROJECT_SOURCE_DIR}/core/include)
include_directories(${PROJECT_BINARY_DIR}/core/include)
include_directories(${PROJECT_SOURCE_DIR}/ext/zita-resampler-1.11.2/source)

add_executable(test_ringbuffer test_ringbuffer.cpp)
add_test(NAME ringbuffer COMMAND test_ringbuffer)

# The frame-assembly test needs no TensorFlow: compile the DSP sources directly.
add_executable(test_filterbank_frame
    test_filterbank_frame.cpp
    ${PROJECT_SOURCE_DIR}/core/src/filterbank.cpp
    ${PROJECT_SOURCE_DIR}/core/src/fretboardrepresentation.cpp
    ${PROJECT_SOURCE_DIR}/core/src/guitarnote.cpp
    ${PROJECT_SOURCE_DIR}/core/src/guitarstring.cpp
    ${PROJECT_SOURCE_DIR}/core/src/logging.cpp)
add_test(NAME filterbank_frame COMMAND test_filterbank_frame)

if(EXISTS ${PROJECT_SOURCE_DIR}/python/neuralnetmodelling/guitarmidi.tflite)
    add_executable(test_pipeline_timing test_pipeline_timing.cpp)
    target_link_libraries(test_pipeline_timing PRIVATE guitarmidi_core)
    target_compile_definitions(test_pipeline_timing PRIVATE
        GUITARMIDI_TEST_MODEL_DIR="${PROJECT_SOURCE_DIR}/python/neuralnetmodelling")
    add_test(NAME pipeline_timing COMMAND test_pipeline_timing)
    set_tests_properties(pipeline_timing PROPERTIES TIMEOUT 240)

    # A minimal LV2 host around the plugin's own shared library.
    add_executable(test_lv2_wrapper test_lv2_wrapper.cpp)
    target_link_libraries(test_lv2_wrapper PRIVATE guitarmidi)
    target_compile_definitions(test_lv2_wrapper PRIVATE
        GUITARMIDI_TEST_MODEL_DIR="${PROJECT_SOURCE_DIR}/python/neuralnetmodelling")
    add_test(NAME lv2_wrapper COMMAND test_lv2_wrapper)
    set_tests_properties(lv2_wrapper PROPERTIES TIMEOUT 120)
else()
    message(STATUS "guitarmidi.tflite not found; skipping the pipeline timing test")
endif()
