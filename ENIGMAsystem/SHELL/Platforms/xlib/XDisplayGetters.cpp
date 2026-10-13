/*

 MIT License
 
 Copyright © 2026 Samuel Venable
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
*/

#include "Platforms/General/PFwindow.h"

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

namespace {

typedef struct {
  int x;
  int y;
  unsigned int width;
  unsigned int height;
} XRRCrtcInfoRectangle;

XRRCrtcInfoRectangle XRRCrtcInfoGetPrimaryMoniorRectangle(Display *display) {
  XRRCrtcInfoRectangle rectangle = { 0, 0, 0, 0 };
  XRRScreenResources *screenRes = nullptr;
  RROutput primaryOutput = 0;
  XRROutputInfo *outputInfo = nullptr;
  XRRCrtcInfo *crtcInfo = nullptr;
  Window root = DefaultRootWindow(display);
  int major = 0, minor = 0;
  if (!XRRQueryVersion(display, &major, &minor)) {
    goto fallback;
  }
  screenRes = XRRGetScreenResources(display, root);
  if (!screenRes) {
    goto fallback;
  }
  primaryOutput = XRRGetOutputPrimary(display, root);
  if (primaryOutput == None) {
    XRRFreeScreenResources(screenRes);
    goto fallback;
  }
  outputInfo = XRRGetOutputInfo(display, screenRes, primaryOutput);
  if (!outputInfo) {
    XRRFreeScreenResources(screenRes);
    goto fallback;
  }
  if (outputInfo->crtc == None) {
    XRRFreeOutputInfo(outputInfo);
    XRRFreeScreenResources(screenRes);
    goto fallback;
  }
  crtcInfo = XRRGetCrtcInfo(display, screenRes, outputInfo->crtc);
  if (!crtcInfo) {
    XRRFreeOutputInfo(outputInfo);
    XRRFreeScreenResources(screenRes);
    goto fallback;
  }
  rectangle.x = crtcInfo->x;
  rectangle.y = crtcInfo->y;
  rectangle.width = crtcInfo->width;
  rectangle.height = crtcInfo->height;
  XRRFreeCrtcInfo(crtcInfo);
  XRRFreeOutputInfo(outputInfo);
  XRRFreeScreenResources(screenRes);
  return rectangle;
  fallback:
  int screen = DefaultScreen(display);
  rectangle.x = 0, rectangle.y = 0;
  rectangle.width = DisplayWidth(display, screen);
  rectangle.height = DisplayHeight(display, screen);
  return rectangle;
}

} // anonymous namespace

namespace enigma_user {

int display_get_x() {
  Display *display = XOpenDisplay(nullptr);
  if (display) {
    XRRCrtcInfoRectangle rectangle = XRRCrtcInfoGetPrimaryMoniorRectangle(display);
    XCloseDisplay(display);
    return rectangle.x;
  }
  return -1;
}

int display_get_y() { 
  Display *display = XOpenDisplay(nullptr);
  if (display) {
    XRRCrtcInfoRectangle rectangle = XRRCrtcInfoGetPrimaryMoniorRectangle(display);
    XCloseDisplay(display);
    return rectangle.y;
  }
  return -1;
}

int display_get_width() {
  Display *display = XOpenDisplay(nullptr);
  if (display) {
    XRRCrtcInfoRectangle rectangle = XRRCrtcInfoGetPrimaryMoniorRectangle(display);
    XCloseDisplay(display);
    return (int)rectangle.width;
  }
  return -1;
}

int display_get_height() {
  Display *display = XOpenDisplay(nullptr);
  if (display) {
    XRRCrtcInfoRectangle rectangle = XRRCrtcInfoGetPrimaryMoniorRectangle(display);
    XCloseDisplay(display);
    return (int)rectangle.height;
  }
  return -1;
}

} // namespace enigma_user
