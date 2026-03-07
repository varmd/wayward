#!/bin/bash

WESTON_VER=15

cd `dirname $0`

#mkdir gen
#cd protocol
#sh make.sh
#cd ..
#cd protocol
#make

GTK_CFLAGS=" -D_POSIX_C_SOURCE=200112L -std=c11 -pthread \
-I/usr/include/libweston-${WESTON_VER}/ \
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





LAYOUT=mobintl
gcc -DWITH_WAYLAND_SHM -DLAYOUT=\"layout.${LAYOUT}.h\" -DKEYMAP=\"keymap.${LAYOUT}.h\" ${GTK_CFLAGS} ${WINDOW_SOURCES} ${GTK_LIBS} -lm  -o wvkbd-weston

#gcc -fzero-init-padding-bits=unions ${GTK_CFLAGS} ${WAYWARD_SOURCES} ${XXTEA_SOURCES}  \
#   ${WINDOW_SOURCES} ${GTK_LIBS} -lm -o wayward


#gcc -fzero-init-padding-bits=unions -fPIC -shared  ${GTK_CFLAGS} ${GTK_LIBS} -I/usr/include/libdrm/ -lm -lweston-$WESTON_VER -o shell_helper.so \
#   gen-protocol/weston-desktop-shell-code.c gen-protocol/shell-helper-protocol.c shell-helper.c


echo OK!
