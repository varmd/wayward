# wayward - fast desktop shell for wayland and weston

## Features

* Lightweight, only 12-15MB memory usage for one FHD monitor
* Bottom app launch panel with autohide, time, date, volume control, battery indicator, and restart/shutdown buttons.
* Button to prevent monitor standby when playing videos
* No X11 dependencies, Xwayland is not required
* No GTK or QT required

----
## Screenshot

![screenshot](https://raw.githubusercontent.com/varmd/wayward/master/screenshot.png "Screenshot")

----
## Download

You can download a precompiled package from https://github.com/varmd/wayward/releases. This version is automatically built via Github Actions. cd to download folder and install

    pacman -U wayward*pkg*

## Installation

Download repository, extract and makepkg, then

    pacman -U wayward*pkg*

## Configuration

Add weston to ~/.bash_profile or /etc/profile.d/weston.sh

    echo "weston" >> ~/.bash_profile

Or

    echo "weston" > /etc/profile.d/weston.sh

Wayward installs its own configuration for weston in /etc/xdg/weston/weston.ini so rename or remove any existing weston configuration - e.g at ~/.config/weston.ini.  Relogin.

For shutdown and reboot icons to work install sudo and add to /etc/sudoers

    yourusername ALL = NOPASSWD: /usr/bin/systemctl poweroff
    yourusername ALL = NOPASSWD: /usr/bin/systemctl reboot

**Requirements**

* weston and wayland
* ttf-droid
* cairo
* sudo

----
## Keyboard shortcuts

* `Super + a` - See list of open applications. Use cursor or mouse to select. Press q to close an open app, right click to close an open app
* `Super + e` - Open app panel. Use mouse or Tab, cursor keys <- -> to navigate
* `Volume mute` - Mute volume
* `Volume up` - Volume up
* `Volume down` - Volume down
* `Super + Shift + T` - Launch wayward-terminal
* `Browser key` - Launch browser
* `Super + Ctrl + Alt + s` - Shutdown
* `Super + Ctrl + Alt + r` - Restart

## Hiding apps in the app panel

Edit weston.ini and add hide-apps to the shell section. For example

    hide-apps=mpv,zathura,file-roller


## Changing wallpaper

Wallpapers are changed from weston.ini. See weston.ini documentation - https://www.mankier.com/5/weston.ini#Shell_Section-background-image

## Changelog

1.2.4 - Fix icons broken if there is no audio. Fix app icons missing if hide-apps is empty. Other minor fixes.

1.2.3 - Update to Weston 12.0

1.2.2 - Remove adwaita-icon-theme dependency due to upstream path changes.

1.2.0 - Remove librsvg dependency. Reduces memory usage by upto 10MB. May cause issues with some icons.

1.1.0 - Update to Weston 11.0. Remove workspaces as Weston 11 removed them.

1.0.3 - Update to Weston 10.0

1.0   - Remove GTK due to bugs and incompatible API of GTK4 with weston

0.9   - Add keyboard shortcuts for volume and shutdown

0.8.3 - Add initial multi-monitor support

0.8   - Add battery indicator

----
## Known issues and limitations

* If there are large number of applications in /usr/share/applications, app icons will overlap with system icons. This can be fixed by hiding apps in weston.ini.
* Misconfigured or broken audio such as AMD HDMI audio can cause crash on startup. As a workaround audio setup should be fixed. For example for AMD HDMI audio built-in soundcard can be used as a workaround - e.g by setting snd_hda_intel.index=1,0 if built in soundcard comes as second when running aplay -l
* Multimonitor support - not tested after removal of GTK
* Need to restart weston after a new app installation to see the new app icon in the panel.


Icons are taken from Adwaita icon theme - https://github.com/GNOME/adwaita-icon-theme

