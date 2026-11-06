cmake_minimum_required(VERSION 3.22)
project(Solaris VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# ON = the finished .vst3 is copied into your system VST3 folder after the build
# (Windows: needs an Administrator terminal. macOS: ~/Library/Audio/Plug-Ins/VST3. Linux: ~/.vst3)
option(SOLARIS_INSTALL_AFTER_BUILD "Copy VST3 to system folder after build" ON)
if(SOLARIS_INSTALL_AFTER_BUILD)
  set(SOLARIS_COPY TRUE)
else()
  set(SOLARIS_COPY FALSE)
endif()

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Solaris
  COMPANY_NAME "SolarisAudio"
  BUNDLE_ID com.solarisaudio.saturator
  PLUGIN_MANUFACTURER_CODE Slra
  PLUGIN_CODE Sat1
  FORMATS VST3
  PRODUCT_NAME "Solaris Saturator"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Fx Distortion
  COPY_PLUGIN_AFTER_BUILD ${SOLARIS_COPY})

# Works whether the files sit in Source/ or were uploaded flat into the repo root
file(GLOB SOLARIS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
if(NOT SOLARIS_SOURCES)
  message(FATAL_ERROR "No .cpp files found. Upload PluginProcessor.cpp and PluginEditor.cpp (in Source/ or next to CMakeLists.txt).")
endif()
target_sources(Solaris PRIVATE ${SOLARIS_SOURCES})
target_include_directories(Solaris PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(Solaris PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Solaris
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags
         juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
