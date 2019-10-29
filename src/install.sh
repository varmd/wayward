#!/bin/bash

#cd `dirname $0`

echo $1
echo $0

/usr/bin/install -Dm644 shell/wayward "$1/usr/lib/weston/wayward"
/usr/bin/install -Dm644 wayward-lsdesktopf "$1/usr/lib/weston/wayward-lsdesktopf"
/usr/bin/install -Dm644 shell/shell_helper.so "$1/usr/lib/weston/shell_helper.so"
/usr/bin/install -Dm644 warm.dat "$1/usr/lib/weston/warm.dat"
/usr/bin/install -Dm644 data/org.weston.wayward.gschema.xml "$1//usr/share/glib-2.0/schemas/org.weston.wayward.gschema.xml"
/usr/bin/install -Dm644 weston.ini "$1/etc/xdg/weston/weston.ini"

chmod +x "$1/usr/lib/weston/wayward"
chmod +x "$1/usr/lib/weston/wayward-lsdesktopf"

#clean up
rm -rf shell/wayward
rm -rf shell/shell_helper.so
rm -rf gen

#make
echo OOOK!
