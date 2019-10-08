# /* (c) oblong industries */

# Want cmake with fix for https://gitlab.kitware.com/cmake/cmake/issues/16293
# yobuild's 3.6.2 and upstream's 3.7.0 are ok.
# Ubuntu 17.04 has cmake 3.7.2, which is great.
# However, ubuntu 16.04's 3.5.1 is otherwise ok, so let developers slide for now
#cmake_minimum_required (VERSION 3.7.0)   # ideal
#cmake_minimum_required (VERSION 3.6.2)   # good if you have yobuild's cmake on PATH
cmake_minimum_required (VERSION 3.5.1)    # expedient, but may test against installed copy

project(splash CXX)

# Select C++11 dialect
set(CMAKE_PREFIX_PATH "/opt/oblong/deps-64-13;/opt/oblong/g-speak5.0")
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get access to g-speak as well as its cmake options, helper functions,
# and variables.  (See $G_SPEAK_HOME/src/FindOblong.cmake for what this does.)
# 
# MAC OS NOTE:
# If cmake aborts on the next line with the cmake error
#   By not providing "FindOblong.cmake" in CMAKE_MODULE_PATH...
# that means $G_SPEAK_HOME/bin is not on your PATH and/or
# 'obs get-gspeak-home' couldn't find it (by e.g. looking in debian/rules).
# Normally, oblong's mac platform installer takes care of this for you
# by adding an entry to /etc/paths.d.
FIND_PACKAGE(Oblong)

#---------- Compiler flags ----------------------------------------------------

# Select desired compiler warning level
if (WIN32)
    set(ERROR_CXXFLAGS "/WX")
    # /wdXXXX disables a particular warning for visual C++
    set(FLAGS
      #---- Turn on ALL THE WARNINGS! by default----
      /W3
      #---- OK, but not the annoying ones ------
      /wd4996   # Ignore warnings about POSIX function names being deprecated.  Last I heard, POSIX didn't deprecate them.
    )
else()
    set(ERROR_CXXFLAGS "-Werror")
    set(FLAGS
      #---- Turn on ALL THE WARNINGS! by default----
      -Wall
      -Wempty-body
      -Wpointer-arith
      -Wshadow
      -Wstrict-prototypes
      -Wsuggest-override
      -Wtype-limits
      -Wvla
      -Wwrite-strings
      -fdiagnostics-show-option
      #---- OK, but not the annoying ones ------
      -Wno-error=overloaded-virtual
      )
endif ()

# Turn the wish list FLAGS into the supported subset ERROR_CXXFLAGS
ObCheckCXXFlags(ERROR_CXXFLAGS ${FLAGS})

# Put it all together
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZE_FLAGS} ${ERROR_CXXFLAGS} ${SANITIZER_FLAGS} ${COVERAGE_FLAGS}")

#------------- Finally, the project per se ----------------------------------

# Set project_CFLAGS and project_LDFLAGS to the flags needed by the
# libraries this project uses.  Any pkg-config library can be named here.
ObFindLibs(project ${OB_PREFER_LIB_TYPE}
  libBasement
  libStagingCore
)

SET(LEAPSDK_HOME $ENV{LEAPSDK_HOME})
IF(APPLE)
  SET(LEAP_LDFLAGS "-L${LEAPSDK_HOME}/lib")
ELSE ()
  SET(LEAP_LDFLAGS "-L${LEAPSDK_HOME}/lib/x64")
ENDIF ()

MESSAGE(STATUS "Setting environment variable LEAP_LDFLAGS=${LEAP_LDFLAGS}")

SET (project_source_files
  src/splash.cpp
)
include_directories("include" "src" "${LEAPSDK_HOME}/include")

# Finally, build the app!
add_executable(splash ${project_source_files})
target_link_libraries(splash ${project_LDFLAGS} ${LEAP_LDFLAGS} -lLeap)
set_target_properties(splash PROPERTIES
   COMPILE_FLAGS "${project_CFLAGS}"
)

# We normally do
INSTALL(TARGETS splash RUNTIME DESTINATION "bin")  # comment to use WebThing
# but apps that use WebThing (i.e. cef) have to do this instead:
#cef_install_blessed_app(splash)   # uncomment to use WebThing


#------------- Tell user what happened ----------------------------------------

MESSAGE("--------- -D Options from commandline -----------------------")
MESSAGE("ASAN:                   ${ASAN}; see https://github.com/google/sanitizers/wiki/AddressSanitizer")
MESSAGE("TSAN:                   ${TSAN}; see https://github.com/google/sanitizers/wiki/ThreadSanitizer")
MESSAGE("BUILD_TESTS:            ${BUILD_TESTS}")
MESSAGE("CMAKE_BUILD_TYPE:       ${CMAKE_BUILD_TYPE} (choices are Debug, Release, or RelWithDebInfo)")
MESSAGE("CMAKE_VERBOSE_MAKEFILE: ${CMAKE_VERBOSE_MAKEFILE}")
MESSAGE("CMAKE_INSTALL_PREFIX:   ${CMAKE_INSTALL_PREFIX}")
MESSAGE("COVERAGE:               ${COVERAGE}")
MESSAGE("G_SPEAK_HOME:           ${G_SPEAK_HOME}")
MESSAGE("USE_STATIC_G_SPEAK:     ${USE_STATIC_G_SPEAK}")
MESSAGE("")
MESSAGE("--------- Intermediate compiler flags -----------------------")
MESSAGE("COVERAGE_FLAGS:         ${COVERAGE_FLAGS}")
MESSAGE("ERROR_CXXFLAGS:         ${ERROR_CXXFLAGS}")
MESSAGE("OPTIMIZE_FLAGS:         ${OPTIMIZE_FLAGS}")
MESSAGE("SANITIZER_FLAGS:        ${SANITIZER_FLAGS}")
MESSAGE("")
MESSAGE("--------- Final global compiler flags -----------------------")
MESSAGE("CMAKE_CXX_FLAGS:        ${CMAKE_CXX_FLAGS}")
MESSAGE("project_CFLAGS:         ${project_CFLAGS}")
MESSAGE("project_LDFLAGS:        ${project_LDFLAGS}")
