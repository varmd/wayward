#!/bin/bash

WESTON_VER=12

cd `dirname $0`

#mkdir gen
#cd protocol
#sh make.sh
#cd ..
#cd protocol
#make


GTK_CFLAGS="-std=c11 -pthread \
-I/usr/include/libweston-${WESTON_VER}/ \
-I. \
-I.. \
-I../shared/ \
-I/usr/include \
-I/usr/include/cairo \
-I/usr/include/pixman-1 \
-I/usr/include/glib-2.0 \
-I/usr/lib/glib-2.0/include \
-I/usr/include/gdk-pixbuf-2.0 \
-I/usr/include/libjpeg \
-I/usr/include/
-Igen-protocol
"

GTK_LIBS=" -lwayland-client -lpng -lutil \
   -lwayland-cursor -lpixman-1  -lcairo  -lxkbcommon -lasound -ljpeg -lm -lrt "


cd `dirname $0`


  CLIENT_SOURCES="\
    terminal.c \
  "

  WINDOW_SOURCES="\
	window.c				\
  ../shared/file-util.c \
  ../shared/image-loader.c \
  ../shared/cairo-util.c \
  ../shared/xalloc.c \
  ../shared/option-parser.c \
  ../shared/frame.c \
  ../shared/os-compatibility.c \
  ../shared/config-parser.c \
  gen-protocol/xdg-shell-protocol.c \
  gen-protocol/viewporter-protocol.c \
  gen-protocol/pointer-constraints-unstable-v1-protocol.c \
  gen-protocol/relative-pointer-unstable-v1-protocol.c \
  gen-protocol/text-cursor-position-protocol.c \
  gen-protocol/weston-desktop-shell-code.c \
  gen-protocol/shell-helper-protocol.c
	"

  SIMLE_EGL_SOURCES="\
    simple-egl.c \
  "

  SIMLE_SHM_SOURCES="\
    simple-shm.c \
  "

  WAYWARD_SOURCES="\
    wayward-shell.c \
  "

  SHELL_HELPER_SOURCES="\
    shell-helper.c \
  "

gcc -Wno-deprecated-declarations  ${GTK_CFLAGS} ${CLIENT_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o wayward-terminal

gcc -Wno-deprecated-declarations  ${GTK_CFLAGS} ${WAYWARD_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm -o wayward

gcc -shared  ${GTK_CFLAGS} ${GTK_LIBS} -I/usr/include/libdrm/ -lm -lweston-$WESTON_VER -o shell_helper.so -fPIC gen-protocol/weston-desktop-shell-code.c gen-protocol/shell-helper-protocol.c shell-helper.c


echo OK!
