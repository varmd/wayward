#!/bin/bash

WESTON_VER=15

cd `dirname $0`

#mkdir gen
#cd protocol
#sh make.sh
#cd ..
#cd protocol
#make

mkdir -p o

WAYWARD_CFLAGS="-std=c11 -pthread \
-I../../src/weston-${WESTON_VER}.0.0/include/ \
-I. \
-I.. \
-I../shared/ \
-Ixxtea/ \
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

  WVKBD_SOURCES="\
  ./clients/wvkbd/wvkbd.c \
  ./clients/wvkbd/keyboard.c \
  ./clients/wvkbd/drw.c \
  \
	"

  FLOWER_SOURCES="\
    clients/flower.c \
  "
  KEYBOARD_SOURCES="\
    clients/keyboard.c \
  "


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
  gen-protocol/single-pixel-buffer-v1-protocol.c \
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
    wayward-shell.c
  "

  XXTEA_SOURCES="\
    xxtea/xxtea/xxtea.c \
    xxtea/xxtea/base64.c \
  "

  SHELL_HELPER_SOURCES="\
    shell-helper.c \
  "

#gcc ${WAYWARD_CFLAGS} ./clients/wvkbd/main.c ${WVKBD_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o o/wayward-wvkbd

#gcc ${WAYWARD_CFLAGS} ${FLOWER_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o o/wayward-flower
#gcc ${WAYWARD_CFLAGS} ${KEYBOARD_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o o/wayward-keyboard

gcc ${WAYWARD_CFLAGS} ${CLIENT_SOURCES} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o o/wayward-terminal

gcc -fzero-init-padding-bits=unions ${WAYWARD_CFLAGS} ${WAYWARD_SOURCES} ${XXTEA_SOURCES}  \
   ${WINDOW_SOURCES} ${WVKBD_SOURCES} ${GTK_LIBS} -lm -o o/wayward

gcc ${WAYWARD_CFLAGS} xxtea/wayward-pincode.c \
  ${XXTEA_SOURCES} -o o/wayward-pincode

WESTON_LIB="-lweston-$WESTON_VER"
WESTON_LIB="-L../../src/weston-${WESTON_VER}.0.0/build/libweston/ -l:libweston-${WESTON_VER}.so"


gcc -fzero-init-padding-bits=unions -fPIC -shared  ${WAYWARD_CFLAGS} ${GTK_LIBS} -I/usr/include/libdrm/  -lm ${WESTON_LIB} -o shell_helper.so \
   gen-protocol/weston-desktop-shell-code.c gen-protocol/shell-helper-protocol.c \
   ../shared/os-compatibility.c \
   shell-helper.c -o o/shell_helper.so


echo OK!
