cd "${0%/*}";
if [ `uname` = "Linux" ]; then
  arm-linux-gnueabihf-g++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" "libdlgmod/xlib/nfd/src/nfd_portal.cpp" -o "libdlgmod-cc-arm32.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. -std=c++17 -shared -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` `pkg-config --cflags --libs dbus-1` -lpthread -fPIC;
  arm-linux-gnueabihf-g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. `pkg-config --cflags dbus-1` -std=c++17 -fPIC;
  arm-linux-gnueabihf-g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. `pkg-config --cflags dbus-1` -std=c++17 -fPIC;
  arm-linux-gnueabihf-g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. `pkg-config --cflags dbus-1` -std=c++17 -fPIC;
  arm-linux-gnueabihf-g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. `pkg-config --cflags dbus-1` -std=c++17 -fPIC;
  arm-linux-gnueabihf-g++ -c "libdlgmod/xlib/nfd/src/nfd_portal.cpp" -o "libdlgmod/xlib/nfd/src/nfd_portal.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DUSE_XDG_DESKTOP_PORTAL -Ilibdlgmod/general -Ilibdlgmod/xlib/nfd/src/include -I. `pkg-config --cflags dbus-1` -std=c++17 -fPIC;
  ar rc "libdlgmod-cc-arm32.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o" "libdlgmod/xlib/nfd/src/nfd_portal.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o" "libdlgmod/xlib/nfd/src/nfd_portal.o";
fi;

