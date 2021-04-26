#!/bin/bash

cd `dirname $0`
mkdir gen
cd protocol
sh make.sh
cd ..
#cd protocol
#make

WESTON_VER=9

GTK_CFLAGS="-std=c11 -pthread -I/usr/include/gio-unix-2.0/ -I/usr/lib/glib-2.0/include  -I. -I../gen/ -I/usr/include -I/usr/include/gtk-3.0 
-I/usr/include/glib-2.0 \
-I/usr/include/pango-1.0 \
-I/usr/include/cairo \
-I/usr/include/gdk-pixbuf-2.0 \
-I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 \
-I/usr/include/atk-1.0 \
-I/usr/include/harfbuzz \
-I/usr/include/gsettings-desktop-schemas \
-I/usr/include/pixman-1 \
-I/usr/include/libweston-$WESTON_VER \
-I/usr/include/
"

#GTK_LIBS="-lwayland-client -lgtk-3 -lgnome-menu-3 -lgio-2.0 -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lasound -lxkbcommon"

GTK_LIBS="-lwayland-client -lgtk-3 -lgio-2.0 -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lasound -lxkbcommon"





cd `dirname $0`

cd shell

	#../gen/desktop-shell-client-protocol.h		\
	#../gen/desktop-shell-protocol.c		\

SOURCES="\
	wayward.c				\
	app-icon.c				\
	app-icon.h				\
	clock.c					\
	clock.h					\
	favorites.c				\
	favorites.h				\
	shell-app-system.c			\
	shell-app-system.h			\
	panel.c					\
	panel.h					\
	vertical-clock.c			\
	vertical-clock.h			\
	launcher.c				\
	launcher.h				\
	wayward-resources.c			\
	wayward-resources.h			\
	../gen/weston-desktop-shell-client-protocol.h	\
	../gen/weston-desktop-shell-protocol.c		\
	../gen/shell-helper-client-protocol.h		\
	../gen/shell-helper-protocol.c"

#gcc

#generate resource
## gresource for css
#resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/wayward.gresource.xml)
#wayward-resources.c: wayward.gresource.xml $(resource_files)
glib-compile-resources wayward.gresource.xml --target=wayward-resources.c --sourcedir=. --generate-source --c-name wayward
glib-compile-resources wayward.gresource.xml --target=wayward-resources.h --sourcedir=. --generate-header --c-name wayward


gcc -O2 -Wno-deprecated-declarations ${GTK_CFLAGS} ${SOURCES} ${GTK_LIBS} -lm  -o wayward



gcc -O2 -shared  ${GTK_CFLAGS} ${GTK_LIBS} -I/usr/include/libdrm/ -lm -lweston-$WESTON_VER -lweston-desktop-$WESTON_VER -o shell_helper.so -fPIC ../gen/weston-desktop-shell-protocol.c ../gen/shell-helper-protocol.c shell-helper.c

#make
echo OK!
