cmake_minimum_required(VERSION 3.22)
project(xmb-native-plasma VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Quick Qml OpenGL DBus)

set(PACKAGE_DIR "${CMAKE_SOURCE_DIR}/package")
set(PLUGIN_DIR "${PACKAGE_DIR}/contents/ui/xmbnative")

add_library(xmbnativeplugin SHARED
    src/xmbnativeplugin.cpp
    src/xmbnativeplugin.h
    src/xmbrendereritem.cpp
    src/xmbrendereritem.h
    src/systemusageitem.cpp
    src/systemusageitem.h
    src/outputvisibilitycontroller.cpp
    src/outputvisibilitycontroller.h
)

target_link_libraries(xmbnativeplugin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::OpenGL
    Qt6::DBus
)

target_include_directories(xmbnativeplugin PRIVATE "${CMAKE_SOURCE_DIR}/src")

enable_testing()
add_executable(xmb_zoomboundaries_test tests/zoom-boundaries.cpp)
target_link_libraries(xmb_zoomboundaries_test PRIVATE Qt6::Core)
target_include_directories(xmb_zoomboundaries_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_zoomboundaries COMMAND xmb_zoomboundaries_test)

add_executable(xmb_meshvisibility_test tests/mesh-visibility.cpp)
target_include_directories(xmb_meshvisibility_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_meshvisibility COMMAND xmb_meshvisibility_test)

add_executable(xmb_meshgeometry_test tests/mesh-geometry.cpp)
target_include_directories(xmb_meshgeometry_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_meshgeometry COMMAND xmb_meshgeometry_test)

add_executable(xmb_splineequivalence_test tests/spline-equivalence.cpp)
target_include_directories(xmb_splineequivalence_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_splineequivalence COMMAND xmb_splineequivalence_test)

add_executable(xmb_splinegpu_test tests/spline-gpu.cpp)
target_link_libraries(xmb_splinegpu_test PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
)
target_include_directories(xmb_splinegpu_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_splinegpu COMMAND xmb_splinegpu_test)
set_tests_properties(xmb_splinegpu PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(xmb_outputvisibility_test
    tests/output-visibility.cpp
    src/outputvisibilitycontroller.cpp
    src/outputvisibilitycontroller.h
)
target_link_libraries(xmb_outputvisibility_test PRIVATE Qt6::Core)
target_include_directories(xmb_outputvisibility_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
add_test(NAME xmb_outputvisibility COMMAND xmb_outputvisibility_test)

foreach(test_target IN ITEMS xmb_zoomboundaries_test xmb_meshvisibility_test xmb_outputvisibility_test)
    if(MSVC)
        target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
endforeach()

set_target_properties(xmbnativeplugin PROPERTIES
    OUTPUT_NAME "xmbnativeplugin"
    LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_DIR}"
)
