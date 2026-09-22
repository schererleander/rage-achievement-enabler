cmake_minimum_required(VERSION 3.20)

project(RageAchievements VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "RageAchievements builds a Windows ASI. Configure with the MinGW toolchain.")
endif()

add_library(Achievements SHARED
  src/dllmain.cpp
  src/Achievement.cpp
  src/Achievement.h
  src/GameReady.cpp
  src/GameReady.h
  src/Hook.cpp
  src/Hook.h
  src/Logger.cpp
  src/Logger.h
)

target_compile_features(Achievements PRIVATE cxx_std_17)
target_compile_definitions(Achievements PRIVATE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  _WIN32_WINNT=0x0A00
)

if(MINGW)
  target_link_options(Achievements PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
  )
endif()

set_target_properties(Achievements PROPERTIES
  PREFIX ""
  OUTPUT_NAME "Achievements"
  SUFFIX ".asi"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set(ACHIEVEMENTS_DEPLOY_DIR "" CACHE PATH
  "Optional RAGE bin directory to receive Achievements.asi after build")

if(ACHIEVEMENTS_DEPLOY_DIR)
  add_custom_command(TARGET Achievements POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ACHIEVEMENTS_DEPLOY_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "$<TARGET_FILE:Achievements>" "${ACHIEVEMENTS_DEPLOY_DIR}/Achievements.asi"
    COMMENT "Deploying Achievements.asi to ${ACHIEVEMENTS_DEPLOY_DIR}"
  )
endif()
