/*
 * Copyright (c) 2013 Tiago Vignatti
 * Copyright (c) 2013-2014 Collabora Ltd.
 * Copyright (C) 2017-2021 varmd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <xkbcommon/xkbcommon.h>

#include <unistd.h>
#include <spawn.h>

#include <linux/input.h>

#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>


#include "weston-desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"

#include "wayward-resources.h"

#include "app-icon.h"
#include "clock.h"
#include "favorites.h"
#include "launcher.h"
#include "panel.h"
#include "vertical-clock.h"


#include <libweston/libweston.h>

#include <wayland-client.h>
#include <wayland-cursor.h>

#define WAYWARD_WARM_HOUR_START 21
#define WAYWARD_WARM_HOUR_END 6

#define WAYWARD_NO_MOVE_X -13371


//keyboard
/*
static struct xkb_context *xkb_context;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;
*/
static char running = 1;
//end keyboard

int global_desktop_width = 2;
int global_wallpaper_size = 0;
int global_wallpaper_exists = 0;
int global_battery_exists = 0;
int global_desktop_height = 0;
int global_grid_width = 0;
int global_grid_height = 0;
int global_monitor_count = 0;
int global_panel_monitor = 1;


char global_battery_path[PATH_MAX] = {'\0'};
char *global_home_dir = NULL;
extern char **environ; /* defined by libc */

int gamma_to_warm = 0;
int gamma_to_warm_reset = 0;


enum shm_command {
  SHM_START,
  SHM_MUTE,
  SHM_VOLUMEUP,
  SHM_VOLUMEDOWN,
  SHM_SHUTDOWN,
  SHM_RESTART,
  SHM_LAUNCH_BROWSER,
  SHM_LAUNCH_TERMINAL,
  SHM_LAUNCH_CALC,
};


struct wl_cursor  *wayland_default_cursor;
struct wl_surface *wayland_cursor_surface;

struct wl_shm *wayland_cursor_shm;
struct wl_compositor *wayland_compositor = NULL;

struct element {
  GtkWidget *window;
  GdkPixbuf *pixbuf;
  struct wl_surface *surface;
};


struct output {
	struct wl_output *output;
	uint32_t server_output_id;
	struct wl_list link;

	int x;
	int y;
	struct panel *panel;
	struct element *background;
};

struct desktop {
  struct wl_display *display;
  struct wl_registry *registry;
  struct desktop_shell *shell;
  struct weston_desktop_shell *wshell;

  struct wl_output *output;

  struct wl_list outputs;

  struct shell_helper *helper;

  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_keyboard *keyboard;

  GdkDisplay *gdk_display;

  struct element *background;
  struct element *panel;
  struct element *curtain;
  struct element *launcher_grid;
  struct element *clock;

  guint initial_panel_timeout_id;
  guint hide_panel_idle_id;

  gboolean grid_visible;
  gboolean system_visible;
  gboolean volume_visible;
  gboolean pointer_on_clock;
  gboolean pointer_out_of_panel;
  gboolean panel_event_entered;
  gboolean panel_event_leaved;
};

static gboolean panel_window_enter_cb (GtkWidget *widget,
    GdkEventCrossing *event, struct desktop *desktop);
static gboolean panel_window_leave_cb (GtkWidget *widget,
    GdkEventCrossing *event, struct desktop *desktop);
static void launcher_grid_app_launched (GtkWidget *widget, struct desktop *desktop);


void logfile(char *fmt, ...) {
	va_list args;

  va_start(args, fmt);

  FILE *file = fopen("/tmp/x1.log", "a+");
  vfprintf(file, fmt, args);
  vprintf(fmt, args);
	fclose(file);

	va_end(args);
}


static void
clock_window_enter_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    struct desktop *desktop)
{
  desktop->pointer_on_clock=1;
  panel_window_enter_cb(widget, event, desktop);
}

static void
clock_window_leave_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    struct desktop *desktop)
{
  desktop->pointer_on_clock=0;
  panel_window_leave_cb(widget, event, desktop);
}


//static void
static gboolean
connect_enter_leave_signals (gpointer data)
{
  struct desktop *desktop = data;
  GList *l;

  g_signal_connect (desktop->panel->window, "enter-notify-event",
      G_CALLBACK (panel_window_enter_cb), desktop);

  g_signal_connect (desktop->panel->window, "leave-notify-event",
      G_CALLBACK (panel_window_leave_cb), desktop);

  /*
  g_signal_connect (desktop->clock->window, "enter-notify-event",
      G_CALLBACK (clock_window_enter_cb), desktop);
  g_signal_connect (desktop->clock->window, "leave-notify-event",
      G_CALLBACK (clock_window_leave_cb), desktop);
  */

  g_signal_connect (desktop->clock->window, "enter-notify-event",
      G_CALLBACK (clock_window_enter_cb), desktop);
  g_signal_connect (desktop->clock->window, "leave-notify-event",
      G_CALLBACK (clock_window_leave_cb), desktop);

  g_signal_connect (desktop->launcher_grid->window, "enter-notify-event",
      G_CALLBACK (panel_window_enter_cb), desktop);
  g_signal_connect (desktop->launcher_grid->window, "leave-notify-event",
      G_CALLBACK (panel_window_leave_cb), desktop);

  return G_SOURCE_REMOVE;
  //return TRUE;
}



static void
weston_desktop_shell_prepare_lock_surface (void *data,
    struct weston_desktop_shell *weston_desktop_shell)
{
  weston_desktop_shell_unlock (weston_desktop_shell);
}



static void
weston_desktop_shell_grab_cursor (void *data,
    struct weston_desktop_shell *weston_desktop_shell,
    uint32_t cursor)
{

}








static void
launcher_grid_toggle (GtkWidget *widget,
    struct desktop *desktop)
{
  //TODO change to global
  int window_height = 50;

  if (desktop->grid_visible)
    {

      shell_helper_move_surface (
        desktop->helper,
        desktop->launcher_grid->surface,
        WAYWARD_NO_MOVE_X,
        -10 * (global_desktop_height - window_height - 3 * WAYWARD_CLOCK_HEIGHT - 114 )
      );


      //shell_helper_curtain (desktop->helper, desktop->curtain->surface, 0);
    }
  else
    {

      shell_helper_move_surface (desktop->helper, desktop->launcher_grid->surface, WAYWARD_NO_MOVE_X, global_desktop_height - window_height - global_grid_height + 5 );

      //shell_helper_slide_surface (desktop->helper, desktop->launcher_grid->surface, width, 0);

      //shell_helper_move_surface (desktop->helper, desktop->launcher_grid->surface,
      //width - WAYWARD_CLOCK_WIDTH, (height - window_height - WAYWARD_CLOCK_HEIGHT));

      //shell_helper_curtain (desktop->helper, desktop->curtain->surface, 1);
    }

  desktop->grid_visible = !desktop->grid_visible;
}

static void
launcher_grid_create (struct desktop *desktop)
{
  struct element *launcher_grid;
  GdkWindow *gdk_window;

  launcher_grid = malloc (sizeof *launcher_grid);
  memset (launcher_grid, 0, sizeof *launcher_grid);

  launcher_grid->window = wayward_launcher_new (desktop->background->window);
  gdk_window = gtk_widget_get_window (launcher_grid->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  launcher_grid->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  shell_helper_add_surface_to_layer (desktop->helper,
      launcher_grid->surface,
      desktop->panel->surface);

  g_signal_connect (launcher_grid->window, "app-launched",
      G_CALLBACK (launcher_grid_app_launched), desktop);

  gtk_widget_show_all (launcher_grid->window);

  desktop->launcher_grid = launcher_grid;
}


int global_events_configured = 0;


static void
shell_configure (struct desktop *desktop,
    uint32_t edges,
    struct wl_surface *surface,
    int32_t width, int32_t height)
{

  if(global_events_configured) {
    return;
  }

  global_events_configured = 1;

  int window_height;
  int grid_width, grid_height;

  logfile("shell configure %d %d \n", (int)width, (int)height);

  //launcher needs access to desktop width so needs to be
  // called from shell_configure
  launcher_grid_create (desktop);


  if(desktop->background)
    gtk_widget_set_size_request (desktop->background->window, width, height);

  /* TODO: make this height a little nicer */
  window_height = height * WAYWARD_PANEL_HEIGHT_RATIO;
  window_height = 50;


  shell_helper_move_surface (desktop->helper, desktop->panel->surface,
      WAYWARD_NO_MOVE_X, (global_desktop_height - window_height) );


  gtk_window_resize (GTK_WINDOW (desktop->panel->window),
      width + 1, 50);

  //wayward_launcher_calculate (WAYWARD_LAUNCHER (desktop->launcher_grid->window),       &global_grid_width, &global_grid_height, NULL);


  if(global_desktop_width > 1920) {
    global_grid_width = 650;
  } else if(global_desktop_width > 1600)  {
    global_grid_width = 550;
  } else {
    global_grid_width = 450;
  }
  global_grid_height = 450;

  //wayward_launcher_calculate (WAYWARD_LAUNCHER (desktop->launcher_grid->window),       &global_grid_width, &global_grid_height, NULL, 40);

  gtk_widget_set_size_request (desktop->launcher_grid->window,
      global_grid_width + 100, global_grid_height);



  shell_helper_move_surface ( desktop->helper, desktop->clock->surface,
      global_panel_monitor * width - WAYWARD_CLOCK_WIDTH,
     (height - window_height - WAYWARD_CLOCK_HEIGHT+ 5)
  );


  gtk_window_resize (GTK_WINDOW (desktop->clock->window),
      WAYWARD_CLOCK_WIDTH, WAYWARD_CLOCK_HEIGHT);


  shell_helper_move_surface (desktop->helper, desktop->launcher_grid->surface,
      ( global_panel_monitor - 1 ) * width,
      -10 * (global_desktop_height - window_height - 3 * WAYWARD_CLOCK_HEIGHT - 114 ) );

   weston_desktop_shell_desktop_ready (desktop->wshell);

  /* TODO: why does the panel signal leave on drawing for
   * startup? we don't want to have to have this silly
   * timeout. */
  //connect_enter_leave_signals(desktop);
  g_timeout_add_seconds (3, connect_enter_leave_signals, desktop);
}

static void
weston_desktop_shell_configure (void *data,
    struct weston_desktop_shell *weston_desktop_shell,
    uint32_t edges,
    struct wl_surface *surface,
    int32_t width, int32_t height)
{
  shell_configure(data, edges, surface, width, height);
}

static const struct weston_desktop_shell_listener wshell_listener = {
  weston_desktop_shell_configure,
  weston_desktop_shell_prepare_lock_surface,
  weston_desktop_shell_grab_cursor
};


static void
volume_changed_cb (WaywardClock *clock,
    gdouble value,
    const gchar *icon_name,
    struct desktop *desktop)
{
  wayward_panel_set_volume_icon_name (
      WAYWARD_PANEL (desktop->panel->window), icon_name);
}

static GtkWidget *
clock_create (struct desktop *desktop)
{
  struct element *clock;
  GdkWindow *gdk_window;

  clock = malloc (sizeof *clock);
  memset (clock, 0, sizeof *clock);

  clock->window = wayward_clock_new ();

  g_signal_connect (clock->window, "volume-changed",
      G_CALLBACK (volume_changed_cb), desktop);

  //panel_gdk_window = gtk_widget_get_window (desktop->panel->window);
  //gtk_widget_set_parent_window(clock->window, panel_gdk_window);
  gdk_window = gtk_widget_get_window (clock->window);
  clock->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  gdk_wayland_window_set_use_custom_surface (gdk_window);
  shell_helper_add_surface_to_layer (desktop->helper, clock->surface,
      desktop->panel->surface);

  gtk_widget_show_all (clock->window);

  desktop->clock = clock;
}

static void
button_toggled_cb (struct desktop *desktop,
    gboolean *visible,
    gboolean *not_visible)
{
  *visible = !*visible;
  *not_visible = FALSE;

  if (desktop->system_visible)
    {
      wayward_clock_show_section (WAYWARD_CLOCK (desktop->clock->window),
          WAYWARD_CLOCK_SECTION_SYSTEM);
      wayward_panel_show_previous (WAYWARD_PANEL (desktop->panel->window),
          WAYWARD_PANEL_BUTTON_SYSTEM);
    }
  else if (desktop->volume_visible)
    {
      wayward_clock_show_section (WAYWARD_CLOCK (desktop->clock->window),
          WAYWARD_CLOCK_SECTION_VOLUME);
      wayward_panel_show_previous (WAYWARD_PANEL (desktop->panel->window),
          WAYWARD_PANEL_BUTTON_VOLUME);
    }
  else
    {
      wayward_clock_show_section (WAYWARD_CLOCK (desktop->clock->window),
          WAYWARD_CLOCK_SECTION_CLOCK);
      wayward_panel_show_previous (WAYWARD_PANEL (desktop->panel->window),
          WAYWARD_PANEL_BUTTON_NONE);
    }
}

static void
system_toggled_cb (GtkWidget *widget,
    struct desktop *desktop)
{
  button_toggled_cb (desktop,
      &desktop->system_visible,
      &desktop->volume_visible);
}

static void
volume_toggled_cb (GtkWidget *widget,
    struct desktop *desktop)
{
  button_toggled_cb (desktop,
      &desktop->volume_visible,
      &desktop->system_visible);
}


static gboolean
panel_window_enter_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    struct desktop *desktop)
{




  if(widget) {
    if(!desktop->panel_event_entered) {
      desktop->panel_event_leaved = 0;
      desktop->panel_event_entered = 1;
    } else { //invalid event
      return FALSE;
    }



  //if (desktop->initial_panel_timeout_id > 0)
    //{
      //g_source_remove (desktop->initial_panel_timeout_id);
      //desktop->initial_panel_timeout_id = 0;
    //}

  if (desktop->hide_panel_idle_id > 0 && g_main_context_find_source_by_id (NULL, desktop->hide_panel_idle_id))
    {
      g_source_remove (desktop->hide_panel_idle_id);
      desktop->hide_panel_idle_id = 0;
      return FALSE;
    }

  if (desktop->pointer_out_of_panel)
    {
      desktop->pointer_out_of_panel = FALSE;
      return FALSE;
    }
  }

  gint window_height = 50;

  shell_helper_move_surface (desktop->helper, desktop->panel->surface,
      WAYWARD_NO_MOVE_X, (global_desktop_height - window_height ) );


  shell_helper_move_surface (desktop->helper, desktop->clock->surface,
    WAYWARD_NO_MOVE_X, (global_desktop_height - window_height - WAYWARD_CLOCK_HEIGHT + 5)
  );

  return FALSE;
}

static gboolean
leave_panel_idle_cb (gpointer data)
{
  struct desktop *desktop = data;

  if( (desktop->system_visible || desktop->volume_visible) && !desktop->hide_panel_idle_id) {
    //desktop->hide_panel_idle_id = g_idle_add(leave_panel_idle_cb, desktop);
    desktop->hide_panel_idle_id = g_timeout_add(100, leave_panel_idle_cb, desktop);
    return FALSE;
  }

  if(desktop->pointer_on_clock) {
    return FALSE;
  }
  //if(desktop->hide_panel_idle_id && g_main_context_find_source_by_id (NULL, desktop->hide_panel_idle_id)) {
    //g_source_remove (desktop->hide_panel_idle_id);
    desktop->hide_panel_idle_id = 0;
  //}


  //window_height = height * WAYWARD_PANEL_HEIGHT_RATIO;
  gint window_height = 50;

  shell_helper_move_surface (desktop->helper, desktop->panel->surface, WAYWARD_NO_MOVE_X, global_desktop_height - 3 );


  shell_helper_move_surface (desktop->helper, desktop->clock->surface,
    WAYWARD_NO_MOVE_X,
   global_desktop_height +WAYWARD_CLOCK_HEIGHT
  );

  //}
  /*
  shell_helper_slide_surface (desktop->helper,
      desktop->panel->surface,
      0, 44);
  shell_helper_slide_surface (desktop->helper,
      desktop->clock->surface,
      width, 44);
  */

  //wayward_panel_set_expand (WAYWARD_PANEL (desktop->panel->window), FALSE);

  wayward_clock_show_section (WAYWARD_CLOCK (desktop->clock->window),
      WAYWARD_CLOCK_SECTION_CLOCK);
  wayward_panel_show_previous (WAYWARD_PANEL (desktop->panel->window),
      WAYWARD_PANEL_BUTTON_NONE);
  desktop->system_visible = FALSE;
  desktop->volume_visible = FALSE;
  desktop->pointer_out_of_panel = FALSE;

  //return G_SOURCE_REMOVE;
  return FALSE;
}

int global_counter = 0;

static gboolean
panel_window_leave_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    struct desktop *desktop)
{

  global_counter++;

  if(!desktop->panel_event_leaved) {
    desktop->panel_event_leaved = 1;
    desktop->panel_event_entered = 0;
  } else { //invalid event
    return FALSE;
  }

  //if (desktop->initial_panel_timeout_id > 0)
    //{
      //g_source_remove (desktop->initial_panel_timeout_id);
      //desktop->initial_panel_timeout_id = 0;
    //}

  //if (desktop->hide_panel_idle_id > 0)
  //  return FALSE;

  if (desktop->grid_visible)
    {
      desktop->pointer_out_of_panel = TRUE;
      return FALSE;
    }

  //if(!desktop->hide_panel_idle_id) {
    //desktop->hide_panel_idle_id = g_timeout_add (300, leave_panel_idle_cb, desktop);
    //desktop->hide_panel_idle_id = g_idle_add (leave_panel_idle_cb, desktop);
  //}
  leave_panel_idle_cb(desktop);
  return FALSE;
}


static void
launcher_grid_app_launched (GtkWidget *widget,
    struct desktop *desktop)
{

  launcher_grid_toggle(NULL, desktop);
  leave_panel_idle_cb(desktop);
}


/*
static gboolean
panel_hide_timeout_cb (gpointer data)
{
  struct desktop *desktop = data;

  panel_window_leave_cb (NULL, NULL, desktop);

  return G_SOURCE_REMOVE;
}
*/

static void
favorite_launched_cb (WaywardPanel *panel,
    struct desktop *desktop)
{
  if (desktop->grid_visible)
    launcher_grid_toggle (desktop->launcher_grid->window, desktop);

  panel_window_leave_cb (NULL, NULL, desktop);
}

static void
exposay_toggled_cb (GtkWidget *widget,
    struct desktop *desktop)
{


//  desktop->helper->shell = desktop->wshell;
  shell_helper_launch_exposay (desktop->helper, desktop->wshell, desktop->seat);
  /*
  button_toggled_cb (desktop,
      &desktop->system_visible,
      &desktop->volume_visible);
  */
}



static void
inhibit_toggled_cb (GtkWidget *widget,
    struct desktop *desktop)
{
  shell_helper_toggle_inhibit (desktop->helper, desktop->wshell, desktop->seat);
}

/*
static void
panel_key_pressed_cb (GtkWidget *widget, GdkEventKey *event, gpointer data) {



   struct desktop *desktop = data;




  //shell_helper_slide_surface_back (desktop->helper, desktop->panel->surface);

  //launcher_grid_toggle (desktop->launcher_grid->window, desktop);




fprintf(f, "Panel is being moved", event->keyval);

  fclose(f);


  switch (event->keyval)
  {



    case GDK_KEY_f:
    case GDK_KEY_F:

      if (event->state & GDK_SUPER_MASK)
      {

          exit(0);

//        g_signal_emit (self, signals[APP_MENU_TOGGLED], 0);
      }

     default:
     break;
  }


}
*/


static void
panel_create (struct desktop *desktop)
{


  struct element *panel;
  GdkWindow *gdk_window;

  panel = malloc (sizeof *panel);
  memset (panel, 0, sizeof *panel);

  panel->window = wayward_panel_new ();


  g_signal_connect (panel->window, "app-menu-toggled",
      G_CALLBACK (launcher_grid_toggle), desktop);

  g_signal_connect (panel->window, "system-toggled",
      G_CALLBACK (system_toggled_cb), desktop);
  g_signal_connect (panel->window, "volume-toggled",
      G_CALLBACK (volume_toggled_cb), desktop);
  g_signal_connect (panel->window, "favorite-launched",
      G_CALLBACK (favorite_launched_cb), desktop);

  g_signal_connect (panel->window, "exposay-toggled",
      G_CALLBACK (exposay_toggled_cb), desktop);

  g_signal_connect (panel->window, "inhibit-toggled",
      G_CALLBACK (inhibit_toggled_cb), desktop);


  //desktop->initial_panel_timeout_id =
  //  g_timeout_add_seconds (8, panel_hide_timeout_cb, desktop);

  /* set it up as the panel */
  gdk_window = gtk_widget_get_window (panel->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  //g_signal_connect (panel->window, "key_press_event", G_CALLBACK (panel_key_pressed_cb), desktop);


  panel->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  weston_desktop_shell_set_user_data (desktop->wshell, desktop);
  weston_desktop_shell_set_panel (desktop->wshell, desktop->output,
    panel->surface);
  weston_desktop_shell_set_panel_position (desktop->wshell,
	  WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM);
    //}
  //
  shell_helper_set_panel (desktop->helper, panel->surface);

  gtk_widget_show_all (panel->window);

  desktop->panel = panel;

}

/* Expose callback for the drawing area */
static gboolean
draw_cb (GtkWidget *widget,
    cairo_t *cr,
    gpointer data)
{
  struct output *output = data;

  if(!output || !output->background)
    logfile("output does not exist \n");

  gdk_cairo_set_source_pixbuf (cr, output->background->pixbuf, 0, 0);
  cairo_paint (cr);

  return TRUE;
}

/* Destroy handler for the window */
static void
destroy_cb (GObject *object,
    gpointer data)
{
  gtk_main_quit ();
}

static GdkPixbuf *
scale_background (GdkPixbuf *original_pixbuf)
{
  /* Scale original_pixbuf so it mostly fits on the screen.
   * If the aspect ratio is different than a bit on the right or on the
   * bottom could be cropped out. */
  GdkScreen *screen = gdk_screen_get_default ();
  gint screen_width, screen_height;
  gint original_width, original_height;
  gint final_width, final_height;
  gdouble ratio_horizontal, ratio_vertical, ratio;

  screen_width = gdk_screen_get_width (screen);
  screen_height = gdk_screen_get_height (screen);


  logfile("Screen width %d / height %d \n", screen_width, screen_height);

  screen_width = global_desktop_width;
  screen_height = global_desktop_height;

  logfile("Screen width %d / height %d \n", screen_width, screen_height);

  original_width = gdk_pixbuf_get_width (original_pixbuf);
  original_height = gdk_pixbuf_get_height (original_pixbuf);

  logfile("Original width %d / height %d \n", original_width, original_height);

  ratio_horizontal = (double) screen_width / original_width;
  ratio_vertical = (double) screen_height / original_height;
  ratio = MAX (ratio_horizontal, ratio_vertical);

  final_width = ceil (ratio * original_width);
  final_height = ceil (ratio * original_height);

  return gdk_pixbuf_scale_simple (original_pixbuf,
      final_width, final_height, GDK_INTERP_BILINEAR);
}

static void
background_create (struct desktop *desktop, const gchar *filename, struct output *output)
{
  GdkWindow *gdk_window;
  struct element *background;

  GdkPixbuf *unscaled_background;
  //const gchar *xpm_data[] = {"1 1 1 1", "_ c SteelBlue", "_"};
  const gchar *xpm_data[] = {"1 1 1 1", "_ c Black", "_"};


  if(!output->background) {
    background = malloc (sizeof *background);
    memset (background, 0, sizeof *background);

    logfile("Creating background \n");

  } else {
    background = output->background;
  }

  if(!filename) {
    filename = g_getenv ("WAYWARD_BACKGROUND");
  }
  if (filename && filename[0] != '\0')
    unscaled_background = gdk_pixbuf_new_from_file (filename, NULL);
  else
    unscaled_background = gdk_pixbuf_new_from_xpm_data (xpm_data);

  if (!unscaled_background)
    {
      g_message ("Could not load background (%s).",
          filename ? filename : "built-in");
      exit (EXIT_FAILURE);
    }

  background->pixbuf = scale_background (unscaled_background);
  g_object_unref (unscaled_background);

  //gboolean handled;

  if(output->background) {
    logfile("Found output background \n");
    //g_signal_emit_by_name (background->window, "draw", desktop, &handled);
    //gtk_window_resize (GTK_WINDOW (desktop->background->window),
    //  1920, 400);
    //gtk_widget_hide (background->window);
    //gtk_widget_show_all (background->window);
    gtk_widget_queue_draw(background->window);
    return;
  }

  background->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (background->window, "destroy",
      G_CALLBACK (destroy_cb), NULL);

  g_signal_connect (background->window, "draw",
      G_CALLBACK (draw_cb), output);

  gtk_window_set_title (GTK_WINDOW (background->window), "wayward");
  gtk_window_set_decorated (GTK_WINDOW (background->window), FALSE);

  gtk_widget_set_size_request (background->window, global_desktop_width, global_desktop_height);

  gtk_widget_realize (background->window);

  gdk_window = gtk_widget_get_window (background->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  background->surface = gdk_wayland_window_get_wl_surface (gdk_window);
  weston_desktop_shell_set_user_data (desktop->wshell, desktop);

  weston_desktop_shell_set_background (desktop->wshell, output->output,
	  background->surface);

  logfile("Setting wallpaper \n");

  if(output->background && output->background->window ) {
    gtk_widget_destroy( output->background->window );
    free(output->background);
    desktop->background = NULL;
  }
  logfile("Setting wallpaper 2 \n");

  output->background = background;
  if(!desktop->background)
    desktop->background = output->background;

  gtk_widget_show_all (background->window);
}

static void
curtain_create (struct desktop *desktop)
{
  GdkWindow *gdk_window;
  struct element *curtain;

  curtain = malloc (sizeof *curtain);
  memset (curtain, 0, sizeof *curtain);

  curtain->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (curtain->window), "wayward");
  gtk_window_set_decorated (GTK_WINDOW (curtain->window), FALSE);
  gtk_widget_set_size_request (curtain->window, 1, 1);
  gtk_widget_realize (curtain->window);

  gdk_window = gtk_widget_get_window (curtain->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  curtain->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  desktop->curtain = curtain;

  gtk_widget_show_all (curtain->window);
}

static void
css_setup (struct desktop *desktop)
{
  GtkCssProvider *provider;
  GFile *file;
  GError *error = NULL;

  provider = gtk_css_provider_new ();

  file = g_file_new_for_uri ("resource:///org/raspberry-pi/wayward/style.css");

  if (!gtk_css_provider_load_from_file (provider, file, &error))
    {
      g_warning ("Failed to load CSS file: %s", error->message);
      g_clear_error (&error);
      g_object_unref (file);
      return;
    }

  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
      GTK_STYLE_PROVIDER (provider), 600);

  g_object_unref (file);
}


/* Keyboard */

static void keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {

}

int is_panel_surface_focused = 0;
int is_focus_on_launcher_grid = 0;

static void keyboard_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {

  if(is_focus_on_launcher_grid) {
    is_focus_on_launcher_grid = 0;
    return;
  }

  struct desktop *desktop = data;
  if(!is_panel_surface_focused) {
    if(!desktop->grid_visible) {
      launcher_grid_toggle (desktop->launcher_grid->window, desktop);

      panel_window_enter_cb(NULL, NULL, desktop);
      shell_helper_keyboard_focus_surface(desktop->helper, desktop->launcher_grid->surface);
      is_focus_on_launcher_grid = 1;
    } else {
      launcher_grid_toggle (desktop->launcher_grid->window, desktop);
      leave_panel_idle_cb(desktop);

    }
    is_panel_surface_focused = 1;
  }

}
static void keyboard_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
  is_panel_surface_focused = 0;

  logfile (" Unfocus keyboard \n");

}
static void keyboard_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {

}
static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	//xkb_state_update_mask (xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
static struct wl_keyboard_listener keyboard_listener = {&keyboard_keymap, &keyboard_enter, &keyboard_leave, &keyboard_key, &keyboard_modifiers};


/* End Keyboard */

static void
seat_handle_capabilities (void *data,
    struct wl_seat *seat,
    enum wl_seat_capability caps)
{

  struct desktop *desktop = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !desktop->pointer) {
    desktop->pointer = wl_seat_get_pointer(seat);
    wl_pointer_set_user_data (desktop->pointer, desktop);
    //wl_pointer_add_listener(desktop->pointer, &pointer_listener,           desktop);
    //fprintf ( f, "the mouse was added");
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && desktop->pointer) {
    wl_pointer_destroy(desktop->pointer);
    desktop->pointer = NULL;
  }

   if ( (caps & WL_SEAT_CAPABILITY_KEYBOARD) && !desktop->keyboard) {
     desktop->keyboard = wl_seat_get_keyboard(seat);
     wl_keyboard_add_listener (desktop->keyboard, &keyboard_listener, desktop);
     //fprintf ( f, "the keyboard was added");
   } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && desktop->keyboard) {
    wl_keyboard_destroy(desktop->keyboard);
    desktop->keyboard = NULL;
    //fprintf ( f, "the keyboard was deleted");
  }

  /* TODO: touch */
}

static void
seat_handle_name (void *data,
    struct wl_seat *seat,
    const char *name)
{
}

static const struct wl_seat_listener seat_listener = {
  seat_handle_capabilities,
  seat_handle_name
};


//Output listeners
static void
output_handle_geometry(void *data,
                       struct wl_output *wl_output,
                       int x, int y,
                       int physical_width,
                       int physical_height,
                       int subpixel,
                       const char *make,
                       const char *model,
                       int transform)
{
	#if 0

  struct output *output = data;

	output->x = x;
	output->y = y;

	if (output->panel)
		window_set_buffer_transform(output->panel->window, transform);
	if (output->background)
		window_set_buffer_transform(output->background->window, transform);
  #endif
}

static void
output_handle_mode(void *data,
		   struct wl_output *wl_output,
		   uint32_t flags,
		   int width,
		   int height,
		   int refresh)
{

  if(width > global_desktop_width) {
    global_desktop_width = width;
    logfile("Found output with WxH %d %d \n", global_desktop_width, global_desktop_height);
  }
  if(height > global_desktop_height) {
    global_desktop_height = height;
    printf("Found output with WxH %d %d \n", global_desktop_width, global_desktop_height);
  }



}

static void
output_handle_done(void *data,
                   struct wl_output *wl_output)
{

  logfile("output done \n");

}

static void
output_handle_scale(void *data,
                    struct wl_output *wl_output,
                    int32_t scale)
{
  #if 0
	struct output *output = data;

	if (output->panel)
		window_set_buffer_scale(output->panel->window, scale);
	if (output->background)
		window_set_buffer_scale(output->background->window, scale);
  #endif
}

static const struct wl_output_listener output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale
};





static void
registry_handle_global (void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
  struct desktop *d = data;
  struct output *doutput;

  if (!strcmp (interface, "weston_desktop_shell"))
    {
      d->wshell = wl_registry_bind (registry, name,
          &weston_desktop_shell_interface, MIN(version, 1));
      weston_desktop_shell_add_listener (d->wshell, &wshell_listener, d);
      weston_desktop_shell_set_user_data (d->wshell, d);
    }
  else if (!strcmp (interface, "wl_output"))
    {

      char *env_panel_monitor_num = getenv("WAYWARD_PANEL_MONITOR");


      global_monitor_count++;

      if(env_panel_monitor_num)
        logfile("\n registry_global wl_output %d %s %d \n", global_monitor_count, env_panel_monitor_num, atoi(env_panel_monitor_num) );

      doutput = malloc(sizeof *doutput);
      if (!doutput)
        return;

      doutput->background = NULL;

      doutput->output = wl_registry_bind (registry, name,
          &wl_output_interface, 3);



      wl_list_insert(&d->outputs, &doutput->link);

      wl_output_add_listener(doutput->output, &output_listener, d);

      if(!d->output)
        d->output = doutput->output;

      if(env_panel_monitor_num && global_monitor_count == atoi(env_panel_monitor_num) ) {
        d->output = doutput->output;
        global_panel_monitor = global_monitor_count;
      }


    }
  else if (!strcmp (interface, "wl_seat"))
    {
      d->seat = wl_registry_bind (registry, name,
          &wl_seat_interface, 1);
      wl_seat_add_listener (d->seat, &seat_listener, d);
    }
  else if (!strcmp (interface, "shell_helper"))
    {
      d->helper = wl_registry_bind (registry, name,
          &shell_helper_interface, 1);


    }

}

static void
registry_handle_global_remove (void *data,
    struct wl_registry *registry,
    uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
  registry_handle_global,
  registry_handle_global_remove
};


static void
grab_surface_create(struct desktop *desktop)
{
	struct wl_surface *s;

  GdkWindow *gdk_window;
  struct element *curtain;

  curtain = malloc (sizeof *curtain);
  memset (curtain, 0, sizeof *curtain);

  curtain->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title (GTK_WINDOW (curtain->window), "wayward2");
  gtk_window_set_decorated (GTK_WINDOW (curtain->window), FALSE);
  gtk_widget_set_size_request (curtain->window, 10, 10);
  gtk_widget_realize (curtain->window);

  gdk_window = gtk_widget_get_window (curtain->window);
  gdk_wayland_window_set_use_custom_surface (gdk_window);

  curtain->surface = gdk_wayland_window_get_wl_surface (gdk_window);

  //desktop->curtain = curtain;

  //gtk_widget_show_all (curtain->window);

	weston_desktop_shell_set_grab_surface(desktop->wshell, curtain->surface);


}



static gboolean check_gamma(gpointer data) {
  struct desktop *desktop = data;

  time_t now;
  struct tm *now_tm;
  int hour;

  now = time(NULL);
  now_tm = localtime(&now);
  hour = now_tm->tm_hour;






  if(!gamma_to_warm && (hour >= WAYWARD_WARM_HOUR_START || hour < WAYWARD_WARM_HOUR_END ) ) {
    shell_helper_change_gamma (desktop->helper, desktop->panel->surface, 0);
    gamma_to_warm = 1;
    //fprintf(f, "Set gamma to warm %d \n ", hour);
    //gamma_to_warm_reset = 0;
  } else if(gamma_to_warm && ( hour >= WAYWARD_WARM_HOUR_END && hour < WAYWARD_WARM_HOUR_START) ) {
    shell_helper_change_gamma (desktop->helper, desktop->panel->surface, 1);
    gamma_to_warm = 0;
    //gamma_to_warm_reset = 1;
  }

  //fclose(f);
  return TRUE;
}


void check_battery_exists() {

  struct stat st;
  char battery_path[PATH_MAX];

  sprintf(battery_path, "/sys/class/power_supply/BAT0/capacity");

  int file_access = access(battery_path, F_OK);

  if(file_access == -1) {
    goto try1;
  }

  stat(battery_path, &st);
  if(st.st_size < 1) //does not exist
    goto try1;

  global_battery_exists = 1;
  sprintf(global_battery_path, "/sys/class/power_supply/BAT0/capacity");
  //sprintf(global_battery_path, "/sys/class/graphics/fb0/state");


  ///sys/class/hwmon/hwmon0/temp1_input

  try1:

  sprintf(battery_path, "/sys/class/power_supply/BAT1/capacity");
  file_access = access(battery_path, F_OK);

  if(file_access == -1) {
    return;
  }

  stat(battery_path, &st);
  if(st.st_size < 1) //does not exist
    return;

  global_battery_exists = 1;
  sprintf(global_battery_path, "/sys/class/power_supply/BAT1/capacity");

}


void launch_browser() {
  
  extern char **environ;
  
  pid_t pid;
  char *argv[] = {"/usr/bin/xdg-open", "https://start.duckduckgo.com?kae=d", NULL};
  
  int status = posix_spawn(&pid, "/usr/bin/xdg-open", NULL, NULL, argv, environ);
  return;

  
}

void launch_terminal() {
  
  extern char **environ;
  
  pid_t pid;
  char *argv[] = {"/usr/bin/weston-terminal", NULL};
  
  int status = posix_spawn(&pid, "/usr/bin/weston-terminal", NULL, NULL, argv, environ);
  return;

  
}

static gboolean check_shm_commands(gpointer data) {
  const char *name = "/wayward-shared_mem";
  
  static int shm_init = 0;
  static void *ptr = NULL;
  static int shm_fd = 0;
  if(!shm_init) {
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int) ); 
    ptr = mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);\
    shm_init = 1;
    
    if(ptr < 0)
    {
      printf("SHM Mapping failed. Keyboard shortcuts disabled \n");
      return FALSE;
    }
  }
  
  
  int test = 0;
  int null = 0;
  memcpy(&test, (int *)ptr, sizeof(int));
  if(test != SHM_START) {
    memcpy((int*)ptr, &null, sizeof(int));
  } else {
    return TRUE;  
  }
  //printf("SHM Command is %d \n", test);
  if(test == SHM_MUTE) {
    clock_volume_mute();
  } else if(test == SHM_VOLUMEUP) {
    clock_volume_up();
  } else if(test == SHM_VOLUMEDOWN) {
    clock_volume_down();
  } else if(test == SHM_SHUTDOWN) {
    clock_shutdown();
  } else if(test == SHM_RESTART) {
    clock_restart();
  } else if(test == SHM_LAUNCH_TERMINAL) {
    launch_terminal();
  } else if(test == SHM_LAUNCH_BROWSER) {
    launch_browser();
  }
  return TRUE;
}


static gboolean check_wallpaper(gpointer data) {

  struct desktop *desktop = data;

  if(!global_home_dir)
    return FALSE;

  struct stat st;
  char wallpaper_path[PATH_MAX];
  struct output *cur_output;

  sprintf(wallpaper_path, "%s/.config/wayward-wallpaper.jpg", global_home_dir);
  int file_access = access(wallpaper_path, F_OK);

  logfile("Wallpaper filename is %s %d \n", wallpaper_path, file_access);

  if(file_access == -1 && !global_wallpaper_size) {
    return TRUE;
  }
  else if( file_access == -1 && global_wallpaper_size) {
    wl_list_for_each(cur_output, &desktop->outputs, link) {
      background_create(desktop, NULL, cur_output);
    }
    global_wallpaper_size = 0;
    return TRUE;
  }


  stat(wallpaper_path, &st);

  if(st.st_size < 1 && !global_wallpaper_size) //does not exist
    return TRUE;
  else if(st.st_size > 0 && (!global_wallpaper_size || st.st_size !=global_wallpaper_size) ) { //set wallpaper
    //delete wallpaper
    global_wallpaper_size = st.st_size;
    logfile("global_wallpaper_size is %d \n", global_wallpaper_size);
    wl_list_for_each(cur_output, &desktop->outputs, link) {
      background_create(desktop, wallpaper_path, cur_output);
    }
  }

  return TRUE;

}



int main (int argc,
    char *argv[])
{
  struct desktop *desktop;



  FILE *f;

  global_home_dir = getenv("HOME");

  gdk_set_allowed_backends ("wayland");

  gtk_init (&argc, &argv);

  g_resources_register (wayward_get_resource ());

  desktop = malloc (sizeof *desktop);
  if(!desktop)
    return 0;



  wl_list_init(&desktop->outputs);



  desktop->output = NULL;
  desktop->shell = NULL;
  desktop->helper = NULL;
  desktop->seat = NULL;
  desktop->pointer = NULL;
  desktop->keyboard = NULL;
  desktop->background = NULL;

  f = fopen("/tmp/wayward.log", "w");
  fprintf(f, "Start log \n ");
  fclose(f);

  desktop->gdk_display = gdk_display_get_default ();
  desktop->display = gdk_wayland_display_get_wl_display (desktop->gdk_display);


  if (desktop->display == NULL)
    {
      fprintf (stderr, "failed to get display: %m\n");
      return -1;
    }

  desktop->registry = wl_display_get_registry (desktop->display);
  wl_registry_add_listener (desktop->registry,
      &registry_listener, desktop);



  wl_display_roundtrip (desktop->display);
  wl_display_roundtrip (desktop->display);

  /* Wait until we have been notified about the compositor,
   * shell, and shell helper objects */
  if (!desktop->output || (!desktop->wshell) ||
      !desktop->helper)
    wl_display_roundtrip (desktop->display);

  if ((!desktop->shell && !desktop->wshell) ||
      !desktop->helper)
    {
      fprintf (stderr, "could not find output, shell or helper modules\n");
      return -1;
    }



  desktop->grid_visible = FALSE;
  desktop->system_visible = FALSE;
  desktop->volume_visible = FALSE;
  desktop->pointer_out_of_panel = FALSE;
  desktop->hide_panel_idle_id = 0;
  desktop->panel_event_entered = 0;
  desktop->panel_event_leaved = 0;
  desktop->pointer_on_clock = 0;


  //because shell configure is called later on this is the only way to
  // get display width
  //global_desktop_width = desktop->output.width;

  //check if battery exists
  check_battery_exists();

  css_setup (desktop);

  curtain_create (desktop);




  /* panel needs to be first so the clock and launcher grid can
   * be added to its layer */
  panel_create (desktop);
  clock_create (desktop);


  logfile("panel \n ");



  grab_surface_create(desktop);

  global_wallpaper_size = 2;
  g_timeout_add_seconds (30, check_wallpaper, desktop);
  g_timeout_add (200, check_shm_commands, desktop);
  //g_idle_add (check_shm_commands, desktop);



  logfile("wallpaper \n ");


  check_wallpaper(desktop);



  logfile("wallpaper 2 \n ");


  //Add gamma check timeout
  //TODO fix warm.dat
  #if 0
  g_timeout_add_seconds (120, check_gamma, desktop);
  check_gamma(desktop);
  #endif

  if(!desktop->panel->surface) {
    exit(1);
  }

  shell_helper_bind_key_panel(desktop->helper, desktop->panel->surface, desktop->seat, desktop->wshell);

  gtk_main ();

  /* TODO cleanup */
  return EXIT_SUCCESS;
}
