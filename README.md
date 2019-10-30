# wayward - wayland compositor for weston

Fast GTK shell and compositor for wayland and weston.

----
Screenshot

![screenshot] (/screenshot.png)

----
## installation

Download repository, extract and makepkg, then pacman -U
Add weston to ~/.bash_profile or /etc/profile.d/. Relogin
For shutdown and reboot icons to work install sudo and add to /etc/sudoers

  yourusername ALL = NOPASSWD: /usr/bin/systemctl poweroff
  yourusername ALL = NOPASSWD: /usr/bin/systemctl reboot

**Requirements**

* weston
* wayland
* ttf-droid
* sudo

----
## Keyboard shortcuts

* Super+a - See list of open applications. Use cursor or mouse to select
* Super+f - Open app launcher

----
## Known issues and limitations

* No multimonitor support
* Need to restart program after new app installation




