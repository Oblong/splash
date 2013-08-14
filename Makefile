

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
endif
CXXFLAGS = -Wempty-body \
  -fdiagnostics-show-option \
  -Wall \
  -Wpointer-arith \
  -Wno-shadow \
  -Wwrite-strings \
  -Wno-overloaded-virtual \
  -Wno-strict-overflow \
  -Wno-error=unused-variable \
  -Wno-error \
  -g -O3 -std=gnu++0x

LEAP_LDFLAGS=-L$(LEAPSDK_HOME)/lib/x64
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
    LEAP_LDFLAGS=-L$(LEAPSDK_HOME)/lib
endif


splash: splash.C
	${CXX} ${CXXFLAGS} $^ `PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):/opt/oblong/greenhouse/lib/pkgconfig /opt/oblong/deps-64-8/bin/pkg-config --libs --cflags --static libGreenhouse` ${LEAP_LDFLAGS} -I${LEAPSDK_HOME}/include -I.. -I../.. -L${LEAPSDK_HOME}/lib -lGreenhouse -lLeap -lboost_system -lboost_thread  -o $@

clean:
	rm -f splash
