#!/bin/bash

#cd `dirname $0`

echo $1
echo $0

/usr/bin/install -Dm644 wayward "$1/usr/lib/weston/wayward"
/usr/bin/install -Dm644 scripts/wayward-lsdesktopf "$1/usr/lib/weston/wayward-lsdesktopf"
/usr/bin/install -Dm644 scripts/wayward-get-brightness-ddc "$1/usr/lib/weston/wayward-get-brightness-ddc"
/usr/bin/install -Dm644 scripts/wayward-get-brightness-ctl "$1/usr/lib/weston/wayward-get-brightness-ctl"
/usr/bin/install -Dm655 wayward-set-pincode "$1/usr/bin/wayward-set-pincode"
/usr/bin/install -Dm644 shell_helper.so "$1/usr/lib/weston/shell_helper.so"
/usr/bin/install -Dm655 wayward-terminal "$1/usr/bin/wayward-terminal"
/usr/bin/install -Dm655 wayward-pincode "$1/usr/lib/weston/wayward-pincode"

/usr/bin/install -Dm644 weston.ini "$1/etc/xdg/weston/weston.ini"

chmod +x "$1/usr/lib/weston/wayward"
chmod +x "$1/usr/lib/weston/wayward-lsdesktopf"

#clean up
rm -rf wayward
rm -rf shell_helper.so
rm -rf wayward-terminal

mkdir -p $1/usr/share/wayward
cp icons/* $1/usr/share/wayward/

echo OOOK!
