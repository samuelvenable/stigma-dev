cd "${0%/*}";

# build dynamic library:
if [ `uname -o` = "Msys" ]; then
  g++ "libdlgmod/libdlgmod.cpp" "libdlgmod/win32/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" -o "libdlgmod-gm.dll" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ -static -lntdll -lgdiplus -lcomctl32 -lshlwapi -lcomdlg32 -lole32 -loleaut32 -luuid -fPIC;
elif [ `uname` = "Darwin" ]; then
  clang++ "libdlgmod/libdlgmod.cpp" "libdlgmod/macos/libdlgmod.mm" -o "libdlgmod-gm.dylib" -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -ObjC++ -framework AppKit -framework UniformTypeIdentifiers -mmacos-version-min=10.13 -arch arm64 -arch x86_64 -fPIC;
elif [ `uname -o` = "GNU/Linux" ]; then
  g++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -lpthread -fPIC;
elif [ `uname` = "FreeBSD" ]; then
  clang++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "DragonFly" ]; then
  g++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "NetBSD" ]; then
  g++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc -static-libstdc++ `pkg-config --cflags --libs x11` -I/usr/X11R7/include -Wl,-rpath,/usr/X11R7/lib -L/usr/X11R7/lib -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "OpenBSD" ]; then
  clang++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared `pkg-config --cflags --libs x11` -lkvm -lc -lpthread -fPIC;
elif [ `uname` = "SunOS" ]; then
  export PKG_CONFIG_PATH=/usr/lib/64/pkgconfig;
  g++ "libdlgmod/libdlgmod.cpp" "libdlgmod/xlib/libdlgmod.cpp" "libdlgmod/general/apiprocess/process.cpp" "libdlgmod/general/xprocess.cpp" "libdlgmod/general/lodepng.cpp" -o "libdlgmod-gm.so" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -shared -static-libgcc `pkg-config --cflags --libs x11` -lkvm -lc -lproc -lpthread -fPIC;
fi;

# build static library:
if [ `uname -o` = "Msys" ]; then
  g++ -c "libdlgmod/win32/libdlgmod.cpp" -o "libdlgmod/win32/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/win32/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/win32/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o";
elif [ `uname` = "Darwin" ]; then
  clang++ -c "libdlgmod/macos/libdlgmod.mm" -o "libdlgmod/macos/libdlgmod.o" -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -ObjC++ -mmacos-version-min=10.13 -arch arm64 -arch x86_64 -fPIC;
  clang++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -ObjC++ -mmacos-version-min=10.13 -arch arm64 -arch x86_64 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/macos/libdlgmod.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/macos/libdlgmod.o";
elif [ `uname -o` = "GNU/Linux" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  g++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "FreeBSD" ]; then
  clang++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "DragonFly" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "NetBSD" ]; then
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  g++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -I/usr/X11R7/include -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "OpenBSD" ]; then
  clang++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  clang++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. -I/usr/local/include -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
elif [ `uname` = "SunOS" ]; then
  export PKG_CONFIG_PATH=/usr/lib/64/pkgconfig;
  g++ -c "libdlgmod/xlib/libdlgmod.cpp" -o "libdlgmod/xlib/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/apiprocess/process.cpp" -o "libdlgmod/general/apiprocess/process.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/xprocess.cpp" -o "libdlgmod/general/xprocess.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/general/lodepng.cpp" -o "libdlgmod/general/lodepng.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  g++ -c "libdlgmod/libdlgmod.cpp" -o "libdlgmod/libdlgmod.o" -DPROCESS_GUIWINDOW_IMPL -DNULLIFY_STDERR -DDIALOG_MODULE_GAME_MAKER_BUILD -Ilibdlgmod/general -I. `pkg-config --cflags x11` -std=c++17 -fPIC;
  ar rc "libdlgmod-gm.a" "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
  rm -rf "libdlgmod/libdlgmod.o" "libdlgmod/xlib/libdlgmod.o" "libdlgmod/general/apiprocess/process.o" "libdlgmod/general/xprocess.o" "libdlgmod/general/lodepng.o";
fi;
