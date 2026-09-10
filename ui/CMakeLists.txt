set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(CAIRO REQUIRED cairo)

add_library(guitarmidi_ui SHARED guitarmidi_ui.cpp)

target_include_directories(guitarmidi_ui PRIVATE
    ${X11_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS})

target_link_libraries(guitarmidi_ui PRIVATE
    ${X11_LIBRARIES}
    ${CAIRO_LIBRARIES})

install(TARGETS guitarmidi_ui
        DESTINATION guitarmidi.lv2)
