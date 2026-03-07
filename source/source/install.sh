#!/bin/bash

#cd `dirname $0`

echo $1
echo $0

/usr/bin/install -Dm644 o/wayward "$1/usr/lib/weston/wayward"
/usr/bin/install -Dm644 scripts/wayward-lsdesktopf "$1/usr/lib/weston/wayward-lsdesktopf"
/usr/bin/install -Dm644 scripts/wayward-get-brightness-ddc "$1/usr/lib/weston/wayward-get-brightness-ddc"
/usr/bin/install -Dm644 scripts/wayward-get-brightness-ctl "$1/usr/lib/weston/wayward-get-brightness-ctl"
/usr/bin/install -Dm644 scripts/wayward-set-sound-card "$1/usr/lib/weston/wayward-set-sound-card"
/usr/bin/install -Dm655 wayward-set-pincode "$1/usr/bin/wayward-set-pincode"
/usr/bin/install -Dm644 o/shell_helper.so "$1/usr/lib/weston/shell_helper.so"
/usr/bin/install -Dm655 o/wayward-terminal "$1/usr/bin/wayward-terminal"
/usr/bin/install -Dm655 o/wayward-pincode "$1/usr/lib/weston/wayward-pincode"

/usr/bin/install -Dm644 weston.ini "$1/etc/xdg/weston/weston.ini"

chmod +x "$1/usr/lib/weston/wayward"
chmod +x "$1/usr/lib/weston/wayward-lsdesktopf"

#clean up
rm -rf o/*

mkdir -p $1/usr/share/wayward
cp icons/* $1/usr/share/wayward/

echo OOOK!
