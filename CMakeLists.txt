cmake_minimum_required(VERSION 3.25)
project(AutoClickV VERSION 0.2.0 LANGUAGES CXX RC)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "AutoClickV requer Windows x64")
endif()
find_package(JNI REQUIRED)
set(SAKURA_UI src/sakura/autoclick_menu.cpp src/sakura/tilt.cpp src/sakura/state.cpp src/sakura/theme.cpp src/sakura/widgets.cpp src/sakura/fx.cpp)
add_library(AutoClickVRuntime SHARED src/runtime.cpp ${SAKURA_UI}
  third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
  third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-template-$<CONFIG>.rc"
  CONTENT "101 RCDATA \"$<TARGET_FILE:AutoClickVRuntime>\"\n1 VERSIONINFO\n FILEVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0\n PRODUCTVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0\n BEGIN\n END\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/boull-icon.rc" CONTENT "2 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/src/assets/boull.ico\"\n")
target_sources(AutoClickVRuntime PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/boull-icon.rc")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/boull-test-version.rc"
  CONTENT "1 VERSIONINFO\n FILEVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0\n PRODUCTVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0\n BEGIN\n END\n")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-$<CONFIG>.rc"
  COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-template-$<CONFIG>.rc"
    "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-$<CONFIG>.rc"
  DEPENDS AutoClickVRuntime "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-template-$<CONFIG>.rc"
  VERBATIM)
add_executable(AutoClickV WIN32 src/launcher.cpp src/embedded_runtime.cpp src/updater.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/boull-runtime-$<CONFIG>.rc" "${CMAKE_CURRENT_BINARY_DIR}/boull-icon.rc")
add_dependencies(AutoClickV AutoClickVRuntime)
target_compile_definitions(AutoClickV PRIVATE BOULL_VERSION="${PROJECT_VERSION}")
target_link_libraries(AutoClickV PRIVATE winhttp bcrypt shell32 version)
target_include_directories(AutoClickVRuntime PRIVATE ${JNI_INCLUDE_DIRS} third_party/imgui third_party/imgui/backends)
target_link_libraries(AutoClickVRuntime PRIVATE d3d11 dxgi d3dcompiler dcomp)
foreach(target AutoClickV AutoClickVRuntime)
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc /utf-8)
  endif()
  target_link_libraries(${target} PRIVATE user32)
endforeach()
set_target_properties(AutoClickV PROPERTIES OUTPUT_NAME "Boull")
set_target_properties(AutoClickVRuntime PROPERTIES PREFIX "" OUTPUT_NAME "BoullRuntime")

option(AUTOCLICKV_BUILD_TESTS "Build the isolated JVM mapping test" OFF)
if(AUTOCLICKV_BUILD_TESTS)
  add_executable(BoullUpdateTest tests/update_policy_test.cpp "${CMAKE_CURRENT_BINARY_DIR}/boull-test-version.rc")
  target_compile_features(BoullUpdateTest PRIVATE cxx_std_20)
  target_compile_definitions(BoullUpdateTest PRIVATE BOULL_VERSION="${PROJECT_VERSION}" UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(BoullUpdateTest PRIVATE winhttp bcrypt user32 version)
  if(MSVC)
    target_compile_options(BoullUpdateTest PRIVATE /W4 /permissive- /EHsc /utf-8)
  endif()
  add_executable(AutoClickVUiHost WIN32 tests/ui_host.cpp)
  target_compile_features(AutoClickVUiHost PRIVATE cxx_std_20)
  add_executable(BoullHudTest tests/hud_composition_test.cpp ${SAKURA_UI}
    third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
  target_compile_features(BoullHudTest PRIVATE cxx_std_20)
  target_compile_definitions(BoullHudTest PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(BoullHudTest PRIVATE ${JNI_INCLUDE_DIRS} third_party/imgui third_party/imgui/backends)
  target_link_libraries(BoullHudTest PRIVATE user32 d3d11 dxgi d3dcompiler dcomp)
  add_executable(AutoClickVMappingTest tests/runtime_mapping_test.cpp ${SAKURA_UI}
    third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
  target_compile_features(AutoClickVMappingTest PRIVATE cxx_std_20)
  target_compile_definitions(AutoClickVMappingTest PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(AutoClickVMappingTest PRIVATE ${JNI_INCLUDE_DIRS} third_party/imgui third_party/imgui/backends)
  target_link_libraries(AutoClickVMappingTest PRIVATE user32 d3d11 dxgi d3dcompiler dcomp)
  if(MSVC)
    target_compile_options(AutoClickVMappingTest PRIVATE /W4 /permissive- /EHsc /utf-8)
  endif()
  add_executable(AutoClickVClickTest tests/click_message_test.cpp ${SAKURA_UI}
    third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp third_party/imgui/backends/imgui_impl_dx11.cpp)
  target_compile_features(AutoClickVClickTest PRIVATE cxx_std_20)
  target_compile_definitions(AutoClickVClickTest PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_include_directories(AutoClickVClickTest PRIVATE ${JNI_INCLUDE_DIRS} third_party/imgui third_party/imgui/backends)
  target_link_libraries(AutoClickVClickTest PRIVATE user32 d3d11 dxgi d3dcompiler dcomp)
  if(MSVC)
    target_compile_options(AutoClickVClickTest PRIVATE /W4 /permissive- /EHsc /utf-8)
  endif()
endif()




