#!/bin/sh
cd "${0%/*}";
"C:/Windows/System32/cmd.exe" /c "../msys64/msys2_shell.cmd" -defterm -mingw32 -no-start -here -lc "./STIGMA.sh"
