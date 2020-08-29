# wayward - fast desktop shell for wayland and weston

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

Wayward installs its own configuration for weston in /etc/profile.d/weston.ini so rename or remove any existing weston configuration - e.g at ~/.config/weston.ini.  Relogin.


For shutdown and reboot icons to work install sudo and add to /etc/sudoers

    yourusername ALL = NOPASSWD: /usr/bin/systemctl poweroff
    yourusername ALL = NOPASSWD: /usr/bin/systemctl reboot

**Requirements**

* weston and wayland
* ttf-droid and adwaita-icon-theme
* gtk3
* sudo

----
## Keyboard shortcuts

* Super+a - See list of open applications. Use cursor or mouse to select. Press q
  to close an open app, right click to close an open app
* Super+f - Open app launcher

## Changing apps in quick launch bars

Use gsettings to modify and restart the program. E.g.

    gsettings set org.weston.wayward favorites "['mpv.desktop', 'firefox.desktop']"

## Changing wallpaper

Copy wallpaper file to ~/.config/wayward-wallpaper.jpg. Wallpaper file is checked every 30 seconds for changes. Removing ~/.config/wayward-wallpaper.jpg will remove the wallpaper.

----
## Known issues and limitations

* No multimonitor support
* Need to restart wayward after a new app installation to see the app in the app launcher.




