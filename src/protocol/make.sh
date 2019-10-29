
wayland-scanner client-header weston-desktop-shell.xml ../gen/weston-desktop-shell-client-protocol.h
wayland-scanner server-header weston-desktop-shell.xml ../gen/weston-desktop-shell-server-protocol.h
wayland-scanner code weston-desktop-shell.xml ../gen/weston-desktop-shell-protocol.c

wayland-scanner client-header shell-helper.xml ../gen/shell-helper-client-protocol.h
wayland-scanner server-header shell-helper.xml ../gen/shell-helper-server-protocol.h
wayland-scanner code shell-helper.xml ../gen/shell-helper-protocol.c

echo OK!
