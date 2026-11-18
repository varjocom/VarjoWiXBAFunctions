# Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
# LICENSE.txt has the license text.

# The BAFunctions DLL of the Varjo setup and its unit tests. A build that uses this project passes
# these variables:
#
#   WIX_INCLUDE_DIRS    The header directories of the WiX libraries
#   WIX_LIBRARIES       The WiX static libraries, in link order
#   GTEST_INCLUDE_DIRS  The header directories of gtest
#   GTEST_LIBRARIES     gtest_main and gtest
#   VERSION             The version of the DLL, as four numbers separated by dots

cmake_minimum_required(VERSION 3.15)
project(VarjoBAFunctions CXX RC)

string(REPLACE "." "," _version_numbers "${VERSION}")
configure_file(VarjoBAFunctions.rc.in "${CMAKE_CURRENT_BINARY_DIR}/VarjoBAFunctions.rc" @ONLY)

add_library(VarjoBAFunctions SHARED
    ChoiceDialog.cpp
    Controls.cpp
    DialogHooks.cpp
    Dimmer.cpp
    Drawing.cpp
    ErrorPage.cpp
    Images.cpp
    LicensePrinting.cpp
    Module.cpp
    RunOutcome.cpp
    Subclass.cpp
    VarjoBAFunctions.cpp
    WindowUtil.cpp
    VarjoBAFunctions.def
    "${CMAKE_CURRENT_BINARY_DIR}/VarjoBAFunctions.rc"
)
target_compile_features(VarjoBAFunctions PRIVATE cxx_std_17)
target_compile_definitions(VarjoBAFunctions PRIVATE _UNICODE UNICODE NOMINMAX)
target_include_directories(VarjoBAFunctions PRIVATE ${WIX_INCLUDE_DIRS})
target_link_libraries(VarjoBAFunctions PRIVATE
    ${WIX_LIBRARIES}
    comctl32 comdlg32 dwmapi gdiplus uxtheme
    # The WiX libraries need these.
    advapi32 crypt32 gdi32 msi ole32 oleaut32 shell32 shlwapi user32 version wininet
)
target_link_options(VarjoBAFunctions PRIVATE /INCREMENTAL:NO /OPT:REF)
# The WiX libraries use the static C runtime.
set_property(TARGET VarjoBAFunctions PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)

enable_testing()
add_executable(VarjoBAFunctions_TestRunner
    tests/test_actiontext.cpp
    tests/test_controlstate.cpp
    tests/test_errorprompt.cpp
    tests/test_geometry.cpp
    tests/test_shutdownaction.cpp
)
target_compile_features(VarjoBAFunctions_TestRunner PRIVATE cxx_std_17)
target_compile_definitions(VarjoBAFunctions_TestRunner PRIVATE _UNICODE UNICODE NOMINMAX)
target_include_directories(VarjoBAFunctions_TestRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${WIX_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS})
target_link_libraries(VarjoBAFunctions_TestRunner PRIVATE ${GTEST_LIBRARIES})
set_property(TARGET VarjoBAFunctions_TestRunner PROPERTY FOLDER "Tests")
add_test(NAME VarjoBAFunctions_TestRunner COMMAND VarjoBAFunctions_TestRunner)
