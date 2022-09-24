
declare -a arr=(
  "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
  "/usr/share/wayland-protocols/stable/viewporter/viewporter.xml"
  "/usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml"
  "/usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml"
      "../protocol/text-cursor-position.xml"
      "../protocol/weston-desktop-shell.xml"
)

## now loop through the above array
for i in "${arr[@]}"
do
   ii=$(basename $i .xml)
   echo $ii
   wayland-scanner client-header "$i" gen-protocol/"$ii"-client-protocol.h
   wayland-scanner server-header "$i" gen-protocol/"$ii"-server-protocol.h
   wayland-scanner code "$i" gen-protocol/"$ii"-protocol.c
   # or do whatever with individual element of the array
done


