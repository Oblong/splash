cmake_minimum_required (VERSION 2.8.7)
project(splash CXX)

# This is the amazingly complicated idiom for providing a default value for cache variables set from the commandline with -D
# (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT makes this easier in just one special case, but general case is harder.)
IF(DEFINED CMAKE_BUILD_TYPE AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "None"))
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo")
ELSE()
   SET(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
ENDIF()


# The list of g-speak libraries this app needs (careful, this is case sensitive).
# It will pull in any other g-speak libraries needed by the ones you specify.
# Actually, any pkgconfig library from g-speak, yobuild, or the system may be listed here.
set(PLATFORM_LIBS libAfferent libTwillig libGreenhouse libWebThing)

# NOTE: To change which g-speak this project uses, edit g-speak.dat to say e.g.
#   G_SPEAK_HOME=/opt/oblong/g-speak3.22
# Be careful to not add spaces.
# You can also use -DG_SPEAK_HOME=/opt/oblong/g-speak3.22 on cmake commandline.
# TIP: If you're building debian packages, run
#   sh set-gspeak.sh 3.22
# instead.
IF(DEFINED G_SPEAK_HOME AND (NOT ${G_SPEAK_HOME} STREQUAL "None"))
   SET(G_SPEAK_HOME ${G_SPEAK_HOME} CACHE STRING "Choose where g-speak was installed")
ELSE()
   # But default to value specified in debian/rules, set by set-gspeak.sh
   execute_process(COMMAND awk "-F=" "/G_SPEAK_HOME=/ { print $2 }"
		   INPUT_FILE "${PROJECT_SOURCE_DIR}/g-speak.dat"
		   OUTPUT_VARIABLE G_SPEAK_HOME
		   OUTPUT_STRIP_TRAILING_WHITESPACE
		   )
   SET(G_SPEAK_HOME "${G_SPEAK_HOME}" CACHE STRING "Choose where g-speak was installed" FORCE)
ENDIF()

# Calling FindGSpeak with a set of pkgconfig library names
# adds the needed directories to the include path, and sets the variables
#   G_SPEAK_LDFLAGS
#   G_SPEAK_CFLAGS
#   G_SPEAK_YOBUILD_HOME
INCLUDE("${G_SPEAK_HOME}/lib/cmake/FindGSpeak.cmake")
FindGSpeak(${PLATFORM_LIBS})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${G_SPEAK_CFLAGS}")

#
# Debugging Options
#
set (CMAKE_VERBOSE_MAKEFILE 0) # Use 1 for debugging, 0 for release

#
# Compiler flags
#
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1y")
elseif(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Common compiler flags for Windows, OSX and Linux
if (WIN32)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
  # Developers will probably prefer to use #pragma's in a .h file to
  # suppress most warnings... but some warnings don't suppress that way,
  # so do them here if needed.
  #set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")
else ()
  # Flags commonly used on Linux and Mac.  Adjust to taste.
  FOREACH(FLAG
        -fdiagnostics-show-option
        -Wall
        -Wempty-body
        -Werror
        -Wno-error=unused-variable
        -Wno-overloaded-virtual
        -Wno-parentheses
        -Wno-strict-overflow
        -Wno-unused-but-set-variable
        -Wno-unused-command-line-argument
        -Wpointer-arith
        -Wshadow
        -Wtype-limits
        -Wvla
        -Wwrite-strings
    )
    CHECK_CXX_COMPILER_FLAG(${FLAG} COMPILER_SUPPORTS_${FLAG})
    IF(COMPILER_SUPPORTS_${FLAG})
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
    ENDIF()
  ENDFOREACH()
endif ()

SET(LEAPSDK_HOME $ENV{LEAPSDK_HOME})
IF(APPLE)
  SET(LEAP_LDFLAGS "-L${LEAPSDK_HOME}/lib")
ELSE ()
  SET(LEAP_LDFLAGS "-L${LEAPSDK_HOME}/lib/x64")
ENDIF ()

MESSAGE(STATUS "Setting environment variable LEAP_LDFLAGS=${LEAP_LDFLAGS}")
#
# Project Search Paths
#
SET (project_source_files
  src/main.cpp
)
include_directories("include" "src" "${LEAPSDK_HOME}/include")

# Finally, build the app!
add_executable(splash ${project_source_files})
target_link_libraries(splash ${G_SPEAK_LDFLAGS} ${LEAP_LDFLAGS} -lLeap)
