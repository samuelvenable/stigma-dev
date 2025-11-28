#!/bin/sh
cd "${0%/*}";
"../msys64/msys2_shell.cmd" -defterm -mingw32 -no-start -here -lc "./STIGMA.sh";
