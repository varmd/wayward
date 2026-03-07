#include "window.h"
#include "shared/xalloc.h"

#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wchar.h>


extern struct wl_surface *main_input_panel();

int
main(int argc, char **argv) {
  main_input_panel( 1 , 1600, 400);
  return 0;
}
