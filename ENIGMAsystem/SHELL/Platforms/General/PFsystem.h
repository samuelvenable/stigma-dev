/** Copyright (C) 2014 Robert B. Colton
*** Copyright (C) 2013 forthevin
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#ifndef ENIGMA_PLATFORM_SYSTEM_H
#define ENIGMA_PLATFORM_SYSTEM_H

#include <string>
using std::string;

namespace enigma_user {

#if !defined(os_unknown)
#define os_unknown -1
#endif
#if !defined(os_win32)
#define os_win32 0
#endif
#if !defined(os_win64)
#define os_win64 1
#endif
#if !defined(os_macosx)
#define os_macosx 2
#endif
#if !defined(os_linux)
#define os_linux 3
#endif
#if !defined(os_freebsd)
#define os_freebsd 4
#endif
#if !defined(os_dragonfly)
#define os_dragonfly 5
#endif
#if !defined(os_netbsd)
#define os_netbsd 6
#endif
#if !defined(os_openbsd)
#define os_openbsd 7
#endif
#if !defined(os_sunos)
#define os_sunos 8
#endif
#if (defined(_WIN32) && !defined(_WIN64) && !defined(os_windows))
#define os_windows os_win32
#elif (defined(_WIN32) && defined(_WIN64) && !defined(os_windows))
#define os_windows os_win64
#elif !defined(os_windows)
#define os_windows os_unknown
#endif
#if !defined(os_type)
#if (defined(_WIN32) && !defined(_WIN64))
#define os_type os_win32
#elif (defined(_WIN32) && defined(_WIN64))
#define os_type os_win64
#elif (defined(__APPLE__) && defined(__MACH__))
#define os_type os_macosx
#elif defined(__linux__)
#define os_type os_linux
#elif defined(__FreeBSD__)
#define os_type os_freebsd
#elif defined(__DragonFly__)
#define os_type os_dragonfly
#elif defined(__NetBSD__)
#define os_type os_netbsd
#elif defined(__OpenBSD__)
#define os_type os_openbsd
#elif defined(__sun)
#define os_type os_sunos
#else
#define os_type os_unknown
#endif
#endif

string os_get_config();
int os_get_info();
string os_get_language();
string os_get_region();
bool os_is_network_connected();
bool os_is_paused();
void os_lock_orientation(bool enable);
void os_powersave_enable(bool enable);

} // enigma_user

#endif //ENIGMA_PLATFORM_SYSTEM_H
