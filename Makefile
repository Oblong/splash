

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
    PATH:=/opt/oblong/deps-64-8/bin:$(PATH)
else
    PATH:=/opt/oblong/deps/bin:$(PATH)
endif

GH_PATH=/opt/oblong/greenhouse
GH_PKG_CONFIG=$(PKG_CONFIG_PATH):$(GH_PATH)/lib/pkgconfig pkg-config

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
  `PKG_CONFIG_PATH=$(GH_PKG_CONFIG) --cflags --static libGreenhouse` \
  -g -O3 -std=gnu++0x

LDFLAGS=`PKG_CONFIG_PATH=$(GH_PKG_CONFIG) --libs --static libGreenhouse`
LEAP_LDFLAGS=-L$(LEAPSDK_HOME)/lib/x64
ifeq ($(UNAME_S),Darwin)
    LEAP_LDFLAGS=-L$(LEAPSDK_HOME)/lib
endif


splash: splash.C
	${CXX} $^ ${LDFLAGS} ${LEAP_LDFLAGS} -I${LEAPSDK_HOME}/include -I.. -I../.. -L${LEAPSDK_HOME}/lib -lGreenhouse -lLeap -lboost_system -lboost_thread ${CXXFLAGS} -o $@

clean:
	rm -f splash
