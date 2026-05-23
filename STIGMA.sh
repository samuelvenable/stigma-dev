#!/bin/sh
cd "${0%/*}"
windres "Resources.rc" "Resources.o"
g++ -c "STIGMA.cpp" "shared/apifilesystem/filesystem.cpp" "shared/apiprocess/process.cpp" -std=c++17 -I"shared"
g++ "STIGMA.o" "filesystem.o" "process.o" "Resources.o" -o "STIGMA.exe" -std=c++17 -static -lshell32 -lole32 -luuid -lntdll -Wl,--subsystem,windows
