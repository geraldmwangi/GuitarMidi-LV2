include_directories(${PROJECT_SOURCE_DIR}/core/include)
include_directories(${PROJECT_BINARY_DIR}/core/include)

add_executable(latency_report latency_report.cpp)
target_link_libraries(latency_report PRIVATE guitarmidi_core)
