cd "${0%/*}";

# build dynamic library:
if [ "$OS" = "Windows_NT" ]; then
  g++ "libdlgmod/win32/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" -o "libdlgmod-cc.dll" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ -static -lntdll -lgdiplus -lcomctl32 -lshlwapi -lcomdlg32 -lole32 -loleaut32 -luuid -fPIC;
elif [ `uname` = "Darwin" ]; then
  clang++ "libdlgmod/macos/libdlgmod.mm" -o "libdlgmod-cc.dylib" -Ilibdlgmod/general -I. -std=c++17 -shared -ObjC++ -framework AppKit -framework UniformTypeIdentifiers -mmacos-version-min=10.13 -arch arm64 -arch x86_64 -fPIC;
elif [ `uname` = "Linux" ]; then
  g++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -lpthread -fPIC;
elif [ `uname` = "FreeBSD" ]; then
  clang++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "DragonFly" ]; then
  g++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "NetBSD" ]; then
  g++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc `pkg-config --cflags --libs x11` -I/usr/X11R7/include -Wl,-rpath,/usr/X11R7/lib -L/usr/X11R7/lib -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "OpenBSD" ]; then
  clang++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "SunOS" ]; then
  export PKG_CONFIG_PATH=/usr/lib/64/pkgconfig;
  g++ "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-cc.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc `pkg-config --cflags --libs x11` -lkvm -lc -lproc -lpthread -fPIC;
fi;

# build static library:
if [ "$OS" = "Windows_NT" ]; then
  g++ -c "libdlgmod/win32/libdlgmod.cpp" -o "libdlgmod/win32/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/win32/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o";
  rm -rf "libdlgmod/win32/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o";
elif [ `uname` = "Darwin" ]; then
  clang++ -c "libdlgmod/macos/libdlgmod.mm" -o "libdlgmod/macos/libdlgmod.o" -Ilibdlgmod/general -I. -std=c++17 -ObjC++ -mmacos-version-min=10.13 -arch arm64 -arch x86_64 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/macos/libdlgmod.o";
  rm -rf "libdlgmod/macos/libdlgmod.o";
elif [ `uname` = "Linux" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "FreeBSD" ]; then
  clang++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "DragonFly" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "NetBSD" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "OpenBSD" ]; then
  clang++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "SunOS" ]; then
  export PKG_CONFIG_PATH=/usr/lib/64/pkgconfig;
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  ar rc "libdlgmod-cc.a" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
fi;
