#!/usr/bin/env bash
# Produced with obi!
# brew tap Oblong/homebrew-tools
# brew install obi
set -e
set -v

mkdir -p build
cmake -H"/Users/mattieruth/src/platform/splash" -B"build" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build "build" -- -j8 -l8
pkill -KILL -f '[a-z/]+splash .*' || true
OB_SHARE_PATH=share/splash:$OB_SHARE_PATH build/splash   2>&1 | tee -a splash.log
