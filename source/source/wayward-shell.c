/*
 * Copyright © 2021-2023 varmd - https://github.com/varmd
 * Copyright © 2011 Kristian Høgsberg
 * Copyright © 2011 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <spawn.h>

#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cairo.h>
#include <cairo-svg.h>
#include <glib.h>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "nanosvg/stb_image_write.h"

#define NANOSVG_ALL_COLOR_KEYWORDS	// Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"

//xxtea
#include "xxtea/xxtea.h"
#include "xxtea/base64.h"



//cairo scale

//typedef uint64_t gint64;

typedef gint64 gfixed;
#define GINT_TO_FIXED(x)         ((gfixed) ((x) << 16))
#define GDOUBLE_TO_FIXED(x)      ((gfixed) ((x) * (1 << 16) + 0.5))
#define GFIXED_TO_INT(x)         ((x) >> 16)
#define GFIXED_TO_DOUBLE(x)      (((double) (x)) / (1 << 16))
#define GFIXED_ROUND_TO_INT(x)   (((x) + (1 << (16-1))) >> 16)
#define GFIXED_0                 0L
#define GFIXED_1                 65536L
#define GFIXED_2                 131072L
#define gfixed_mul(x, y)         (((x) * (y)) >> 16)
#define gfixed_div(x, y)         (((x) << 16) / (y))

#include <sys/wait.h>
#include <linux/input.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <wayland-client.h>
#include "window.h"
#include "shared/cairo-util.h"
#include <libweston/config-parser.h>
#include "shared/helpers.h"
#include "shared/xalloc.h"
#include <libweston/zalloc.h>
#include "shared/file-util.h"
#include "shared/timespec-util.h"

#include "weston-desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"

#include <alsa/asoundlib.h>

#define DEFAULT_CLOCK_FORMAT CLOCK_FORMAT_MINUTES
#define DEFAULT_SPACING 10

#define WAYWARD_HIDE_X 43371
#define WAYWARD_NO_MOVE_X -13371
#define WAYWARD_INITIAL_HEIGHT 52
#define WAYWARD_PANEL_LEAVE_BUG_Y 47
#define WAYWARD_ICON_SIZE 32
#define WAYWARD_AUDIO_STEP 3
#define WAYWARD_BATTERY_X 345
#define WAYWARD_BATTERY_Y 30

//https://github.com/rev22/svgViewer/blob/master/svgViewer.c

extern char **environ; /* defined by libc */
struct panel;

enum clock_format {
	CLOCK_FORMAT_MINUTES,
	CLOCK_FORMAT_SECONDS,
	CLOCK_FORMAT_MINUTES_24H,
	CLOCK_FORMAT_SECONDS_24H,
	CLOCK_FORMAT_NONE
};

enum clock_state {
	CLOCK_SHOWN,
	SYSTEM_SHOWN,
	VOLUME_SHOWN,
	CLOCK_NONE
};

struct desktop {
	struct display *display;
	struct weston_desktop_shell *shell;
	struct unlock_dialog *unlock_dialog;
	struct task unlock_task;
	struct wl_list outputs;

	int want_panel;
	enum weston_desktop_shell_panel_position panel_position;
	enum clock_format clock_format;

	struct window *grab_window;
	struct widget *grab_widget;

  struct shell_helper *helper;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_keyboard *keyboard;

  snd_mixer_t *mixer_handle;
  snd_mixer_elem_t *mixer;
  long min_volume, max_volume;

  long current_volume;
  double current_volume_percentage;

  struct toytimer shm_timer;

  struct panel *panel;

	struct weston_config *config;
	bool locking;

  char *pincode;
	enum cursor_type grab_cursor;

	int painted;

};

struct surface {
	void (*configure)(void *data,
			  struct weston_desktop_shell *desktop_shell,
			  uint32_t edges, struct window *window,
			  int32_t width, int32_t height);
};

struct output;

struct panel_label {
	struct widget *widget;
	struct panel *panel;

  int force_x;
};

struct panel {
	struct surface base;

	struct output *owner;

	struct window *window;
	struct widget *widget;
	struct wl_list launcher_list;

	struct wl_surface *wl_surface;

	struct panel_clock *clock;
	struct panel_clock *battery;

  struct rectangle clock_allocation;

  struct panel_launcher *reboot_launcher;
  struct panel_launcher *shutdown_launcher;

  struct panel_launcher *volumedown_launcher;
  struct panel_launcher *volumeup_launcher;
  struct panel_label *volume_label;


	int painted;
	int initial;
	int allocation_set;
	enum weston_desktop_shell_panel_position panel_position;
	enum clock_format clock_format;

  enum clock_state clock_state;

	uint32_t color;
};

struct background {
	struct surface base;
	struct output *owner;

	struct window *window;
	struct widget *widget;
	int painted;

	char *image;
	int type;
	uint32_t color;
};

struct output {
	struct wl_output *output;
	uint32_t server_output_id;
	struct wl_list link;

	int x;
	int y;
	struct panel *panel;
	struct background *background;
};

struct panel_launcher {
	struct widget *widget;
	struct panel *panel;
	cairo_surface_t *icon;
	int focused, pressed, toggled;
  int force_x;
  int initial_x;
  int offset_right;

  char *path;
	struct wl_list link;
	struct wl_array envp;
	struct wl_array argv;
  void (*function)();
};

struct panel_clock {
	struct widget *widget;
	struct panel *panel;
	struct toytimer timer;
	char *format_string;
	time_t refresh_timer;

  int force_x;
};

struct unlock_dialog {
	struct window *window;
	struct widget *widget;
	struct widget *button;
	int button_focused;
	int closing;
	int keys_entered;

	char pincode[7];
	struct desktop *desktop;
};


//wayward shell
static struct desktop *global_desktop = NULL;
int global_desktop_width = 2;


int global_battery_exists = 0;
char global_battery_path[PATH_MAX] = {'\0'};
int global_desktop_height = 0;
int global_grid_width = 0;
int global_grid_height = 0;
int global_monitor_count = 0;
int global_panel_monitor = 1;
int global_events_configured = 0;
int global_keyboard_configured = 0;

int global_power_inhibit = 0;
int global_clock_hide = 0;
int global_panel_in = 0;
int global_panel_in_y = 0;

static const char keycode_to_vkey[] =
{
    0,                   /* reserved */
    0,                   /* KEY_ESC			1 */
    '1',                   /* KEY_1			2 */
    '2',                   /* KEY_2			3 */
    '3',                   /* KEY_3			4 */
    '4',                   /* KEY_4			5 */
    '5',                   /* KEY_5			6 */
    '6',                 /*  KEY_6			7 */
    '7',                 /*  KEY_7 8*/
    '8',                 /* KEY_8			9 */
    '9',                 /* KEY_9			10 */
    '0',                 /* KEY_0			11 */
    '-',                 /* KEY_MINUS		12 */
    '=',                 /* KEY_EQUAL		13 */
    0,                 /* KEY_BACKSPACE		14 */
    0,                 /* KEY_TAB			15 */
    'Q',                 /* KEY_Q			16 */
    'W',                   /* KEY_W			17 */
    'E',                   /* KEY_E			18 */
    'R',               /* KEY_R			19 */
    'T',             /* KEY_T			20 */
    'Y',             /* KEY_Y			21 */
    'U',            /* KEY_U			22 */
    'I',                   /* KEY_I			23 */
    'O',                   /* KEY_O			24 */
    'P',                   /* KEY_P			25 */
    0,                   /* KEY_LEFTBRACE		26 */
    0,                   /* KEY_RIGHTBRACE		27 */
    0,                   /* KEY_ENTER		28 */
    0,                 /* KEY_LEFTCTRL		29 */
    'A',                 /* KEY_A			30 */
    'S',                 /* KEY_S			31 */
    'D',                 /* KEY_D			32 */
    'F',                 /* KEY_F			33 */
    'G',                 /* KEY_G			34 */
    'H',                 /* KEY_H			35 */
    'J',                 /* KEY_J			36 */
    'K',                 /* KEY_K			37 */
    'L',                 /* KEY_L			38 */
    ';',                 /* KEY_SEMICOLON		39 */
    0,                 /* KEY_APOSTROPHE		40 */
    0,                 /* KEY_GRAVE		41 */
    0,                 /* KEY_LEFTSHIFT		42 */

    0,                 /* KEY_BACKSLASH		43 */
    'Z',                 /* KEY_Z			44 */
    'X',                 /* KEY_X			45 */
    'C',                 /* KEY_C			46 */
    'V',                 /* KEY_V			47 */
    'B',                 /* KEY_B			48 */
    'N',                 /* KEY_N			49 */
    'M',                 /* KEY_M			50 */
    ',',                 /* KEY_COMMA		51 */
    '.',                 /* KEY_DOT			52 */
    '/',                 /* KEY_SLASH		53 */
    0,                 /* KEY_RIGHTSHIFT		54 */
    0,        /* KEY_KPASTERISK		55 */ //??
    0,       /* KEY_LEFTALT		56 */
    ' ',            /* KEY_SPACE		57 */
    0,            /* KEY_CAPSLOCK		58 */
    0,   //#define KEY_F1			59
    0, // #define KEY_F2			60
    0, //#define KEY_F3			61
    0,           /* #define KEY_F4			62 */
    0,           /* #define KEY_F5			63 */
    0,              /* #define KEY_F6			64 */
    0,            /* #define KEY_F7			65 */
    0,                   /* #define KEY_F8			66 */
    0,                   /* #define KEY_F9			67 */
    0                   /* #define KEY_F10			68 */
};

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
  SHM_BRIGHTNESS_UP,
  SHM_BRIGHTNESS_DOWN,
};
static void panel_add_battery(struct panel *panel);

static void launch_volume(struct panel_launcher *launcher);
static void launch_system(struct panel_launcher *launcher);
void wayland_pointer_leave_cb(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);

static void
panel_add_launchers(struct panel *panel, struct desktop *desktop);

static int xxtea_pincode_valid(char *pin_keyb64, char *pincode) {

  const char *text = "Login password here super secret";
  size_t len;


  unsigned char *encrypt_data = xxtea_encrypt(text, strlen(text), pincode, &len);
  char *base64_data_encrypted = base64_encode(encrypt_data, len);
  free(encrypt_data);

  if (strncmp(pin_keyb64, base64_data_encrypted, len) == 0) {
      printf("success!\n");
      printf("B64 pincode %s \n", base64_data_encrypted);
      free(base64_data_encrypted);
      return 1;
  } else {
      printf("wrong!\n");
      free(base64_data_encrypted);
      return 0;
  }

  return 0;
}

static void
sigchild_handler(int s)
{
	int status;
	pid_t pid;

	while (pid = waitpid(-1, &status, WNOHANG), pid > 0)
		fprintf(stderr, "child %d exited\n", pid);
}

static int
is_desktop_painted(struct desktop *desktop)
{
	struct output *output;

	wl_list_for_each(output, &desktop->outputs, link) {
		if (output->panel && !output->panel->painted)
			return 0;
		if (output->background && !output->background->painted)
			return 0;
	}

	return 1;
}

static void
check_desktop_ready(struct window *window)
{
	struct display *display;
	struct desktop *desktop;

	display = window_get_display(window);
	desktop = display_get_user_data(display);

	if (!desktop->painted && is_desktop_painted(desktop)) {
		desktop->painted = 1;

		weston_desktop_shell_desktop_ready(desktop->shell);
	}
}

static void
panel_launcher_activate(struct panel_launcher *widget)
{
	char **argv;
	pid_t pid;

  if(widget->function && widget->focused) {
    widget->function(widget);
    return;
  }

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		return;
	}

	if (pid)
		return;

	argv = widget->argv.data;

	if (setsid() == -1)
		exit(EXIT_FAILURE);

	if (execve(argv[0], argv, widget->envp.data) < 0) {
		fprintf(stderr, "execl '%s' failed: %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
}

static void
panel_launcher_redraw_handler(struct widget *widget, void *data)
{
	struct panel_launcher *launcher = data;
	struct rectangle allocation;
	cairo_t *cr;

	cr = widget_cairo_create(launcher->panel->widget);

	widget_get_allocation(widget, &allocation);

  allocation.x += allocation.width / 2 -
		cairo_image_surface_get_width(launcher->icon) / 2;

	if (allocation.width > allocation.height)
		allocation.x += allocation.width / 2 - allocation.height / 2;
	allocation.y += allocation.height / 2 -
		cairo_image_surface_get_height(launcher->icon) / 2;
	if (allocation.height > allocation.width)
		allocation.y += allocation.height / 2 - allocation.width / 2;
	if (launcher->pressed) {
		allocation.x++;
		allocation.y++;
	}

  //special case to hide launchers for clock section
  if(launcher->force_x) {
    allocation.x = launcher->force_x;
  }

  //printf("Launcher allocation x y %d %d \n", allocation.x, allocation.y);
  //cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_source_surface(cr, launcher->icon,
				 allocation.x, allocation.y);

  cairo_paint(cr);

	if (launcher->focused || launcher->toggled) {
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
		cairo_mask_surface(cr, launcher->icon,
				   allocation.x, allocation.y);
	}

	cairo_destroy(cr);
}

static int
panel_launcher_motion_handler(struct widget *widget, struct input *input,
			      uint32_t time, float x, float y, void *data)
{
	struct panel_launcher *launcher = data;

  //TODO
  //fix subsurface leaving causing panel to hide
  //printf("Y is %d \n", (int)y);
	//widget_set_tooltip(widget, basename((char *)launcher->path), x, 10);

	return CURSOR_LEFT_PTR;
}

static void
set_hex_color(cairo_t *cr, uint32_t color)
{
	cairo_set_source_rgba(cr,
			      ((color >> 16) & 0xff) / 255.0,
			      ((color >>  8) & 0xff) / 255.0,
			      ((color >>  0) & 0xff) / 255.0,
			      ((color >> 24) & 0xff) / 255.0);
}

static void
panel_key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym, enum wl_keyboard_key_state state,
	    void *data)
{
	struct panel_launcher *launcher;
	struct panel_launcher *prev_launcher = NULL;
  int flag = 0;
	int count = 0;

	if (state != WL_KEYBOARD_KEY_STATE_RELEASED) {
    return;
  }

  switch (key) {

    case KEY_ENTER:
    case KEY_SPACE:
      wl_list_for_each(launcher, &global_desktop->panel->launcher_list, link) {
      if(launcher->focused) {
        launcher->focused = 0;
        widget_schedule_redraw(launcher->widget);

        panel_launcher_activate(launcher);

        //shell_helper_keyboard_focus_surface(global_desktop->helper, global_desktop->background->wl_surface);
        if(!launcher->function)
          wayland_pointer_leave_cb(NULL, NULL, 0, global_desktop->panel->wl_surface);
        return;
      }
    }
    break;

    case KEY_TAB:
    case KEY_RIGHT:
      wl_list_for_each(launcher, &global_desktop->panel->launcher_list, link) {
        count++;
        if(flag) {
          launcher->focused = 1;
          widget_schedule_redraw(launcher->widget);
          return;
        }
        if(launcher->focused) {
          launcher->focused = 0;
          widget_schedule_redraw(launcher->widget);
          flag = 1;
        }
      }
      //loop after end
      if(flag) {
        wl_list_for_each(launcher, &global_desktop->panel->launcher_list, link) {
          launcher->focused = 1;
          widget_schedule_redraw(launcher->widget);
          return;
        }
      }
    break;
    case KEY_LEFT:
    wl_list_for_each(launcher, &global_desktop->panel->launcher_list, link) {

      if(launcher->focused) {
        launcher->focused = 0;
        widget_schedule_redraw(launcher->widget);

        if(prev_launcher) {
          prev_launcher->focused = 1;
          widget_schedule_redraw(prev_launcher->widget);
          return;
        }
      }
      prev_launcher = launcher;
    }
    break;

    default:
    break;
  }



}


static void panel_focus_handler(struct window *window,
    struct input *device, void *data) {

  struct panel_launcher *launcher;
  struct panel *panel = data;
  int flag = 0;

  if(!device) {
    return;
  }



  shell_helper_move_surface (global_desktop->helper,
    panel->wl_surface,
    WAYWARD_NO_MOVE_X, global_desktop_height - WAYWARD_INITIAL_HEIGHT
  );


  wl_list_for_each(launcher, &global_desktop->panel->launcher_list, link) {
    if(!flag) {
      launcher->focused = 1;
      widget_schedule_redraw(launcher->widget);
      return;
    }
  }

}

static int
panel_enter_handler(struct widget *widget, struct input *input,
			     float x, float y, void *data)
{
	struct panel *panel = data;



}

static void
panel_leave_handler(struct widget *widget,
			     struct input *input, void *data)
{
	struct panel *panel = data;
}

static void
panel_redraw_handler(struct widget *widget, void *data)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	struct panel *panel = data;

  if(panel->painted) {
    printf("Already painted \n");
    return;
  }


  if(!panel->initial) {
    panel->initial = 1;

  }

	cr = widget_cairo_create(panel->widget);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	set_hex_color(cr, panel->color);
	cairo_paint(cr);

	cairo_destroy(cr);
	surface = window_get_surface(panel->window);
	cairo_surface_destroy(surface);
	panel->painted = 1;
	check_desktop_ready(panel->window);
}

static int
panel_launcher_enter_handler(struct widget *widget, struct input *input,
			     float x, float y, void *data)
{
	struct panel_launcher *launcher = data;

	launcher->focused = 1;
	widget_schedule_redraw(widget);

	return CURSOR_LEFT_PTR;
}

static void
panel_launcher_leave_handler(struct widget *widget,
			     struct input *input, void *data)
{
	struct panel_launcher *launcher = data;

	launcher->focused = 0;
	widget_destroy_tooltip(widget);
	widget_schedule_redraw(widget);
}

static void
panel_launcher_button_handler(struct widget *widget,
			      struct input *input, uint32_t time,
			      uint32_t button,
			      enum wl_pointer_button_state state, void *data)
{
	struct panel_launcher *launcher;

	launcher = widget_get_user_data(widget);
	widget_schedule_redraw(widget);
	if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		panel_launcher_activate(launcher);

}

static void
panel_launcher_touch_down_handler(struct widget *widget, struct input *input,
				  uint32_t serial, uint32_t time, int32_t id,
				  float x, float y, void *data)
{
	struct panel_launcher *launcher;

	launcher = widget_get_user_data(widget);
	launcher->focused = 1;
	widget_schedule_redraw(widget);
}

static void
panel_launcher_touch_up_handler(struct widget *widget, struct input *input,
				uint32_t serial, uint32_t time, int32_t id,
				void *data)
{
	struct panel_launcher *launcher;

	launcher = widget_get_user_data(widget);
	launcher->focused = 0;
	widget_schedule_redraw(widget);
	panel_launcher_activate(launcher);
}



static void
clock_func(struct toytimer *tt)
{
	struct panel_clock *clock = container_of(tt, struct panel_clock, timer);

	widget_schedule_redraw(clock->widget);
}

static void
panel_clock_redraw_handler(struct widget *widget, void *data)
{
	struct panel_clock *clock = data;
	cairo_t *cr;
	struct rectangle allocation;
	cairo_text_extents_t extents;
	time_t rawtime;
	struct tm * timeinfo;
	char string[128];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(string, sizeof string, clock->format_string, timeinfo);

	widget_get_allocation(widget, &allocation);
	if (allocation.width == 0)
		return;


  clock->panel->painted = 0;
	cr = widget_cairo_create(clock->panel->widget);
	cairo_set_font_size(cr, 22);

  cairo_select_font_face (cr, "Droid Sans",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);

	cairo_text_extents(cr, string, &extents);
	if (allocation.x > 0)
		allocation.x +=
			allocation.width - DEFAULT_SPACING * 1.5 - extents.width;
	else
		allocation.x +=
			allocation.width / 2 - extents.width / 2;

  if(clock->force_x)
    allocation.x = clock->force_x;

	allocation.y += allocation.height / 2 - 1 + extents.height / 2;
	//cairo_move_to(cr, allocation.x + 1, allocation.y + 1);
	//cairo_set_source_rgba(cr, 0, 0, 0, 0.85);
	cairo_show_text(cr, string);
	cairo_move_to(cr, allocation.x, allocation.y);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
	cairo_show_text(cr, string);
	cairo_destroy(cr);
}

static int
clock_timer_reset(struct panel_clock *clock)
{
	struct itimerspec its;
	struct timespec ts;
	struct tm *tm;

	clock_gettime(CLOCK_REALTIME, &ts);
	tm = localtime(&ts.tv_sec);

	its.it_interval.tv_sec = clock->refresh_timer;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = clock->refresh_timer - tm->tm_sec % clock->refresh_timer;
	its.it_value.tv_nsec = 10000000; /* 10 ms late to ensure the clock digit has actually changed */
	timespec_add_nsec(&its.it_value, &its.it_value, -ts.tv_nsec);

	toytimer_arm(&clock->timer, &its);
	return 0;
}

static void
panel_destroy_clock(struct panel_clock *clock)
{
	widget_destroy(clock->widget);
	toytimer_fini(&clock->timer);
	free(clock);
}

static void
panel_add_clock(struct panel *panel)
{
	struct panel_clock *clock;

	clock = xzalloc(sizeof *clock);
	clock->force_x = 0;

	clock->panel = panel;
	panel->clock = clock;

	switch (panel->clock_format) {
	case CLOCK_FORMAT_MINUTES:
		clock->format_string = "%a %b %d, %I:%M %p";
		clock->refresh_timer = 60;
		break;
	case CLOCK_FORMAT_SECONDS:
		clock->format_string = "%a %b %d, %I:%M:%S %p";
		clock->refresh_timer = 1;
		break;
	case CLOCK_FORMAT_MINUTES_24H:
		clock->format_string = "%a %b %d, %H:%M";
		clock->refresh_timer = 60;
		break;
	case CLOCK_FORMAT_SECONDS_24H:
		clock->format_string = "%a %b %d, %H:%M:%S";
		clock->refresh_timer = 1;
		break;
	case CLOCK_FORMAT_NONE:
		assert(!"not reached");
	}

	toytimer_init(&clock->timer, CLOCK_MONOTONIC,
		      window_get_display(panel->window), clock_func);
	clock_timer_reset(clock);

	clock->widget = widget_add_widget(panel->widget, clock);
	widget_set_redraw_handler(clock->widget, panel_clock_redraw_handler);
}

static void
panel_resize_handler(struct widget *widget,
		     int32_t width, int32_t height, void *data)
{
	struct panel_launcher *launcher;
	struct panel *panel = data;

  if(panel->painted) {
    printf("Panel is already painted, not resizing \n");
    return;
  }  else {
    printf("Resizing panel \n");
  }

	int x = 0;
	int y = 0;
	int w = height > width ? width : height;
	int h = w;
	int horizontal = panel->panel_position == WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP || panel->panel_position == WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM;
	int first_pad_h = horizontal ? 0 : DEFAULT_SPACING / 2;
	int first_pad_w = horizontal ? DEFAULT_SPACING / 2 : 0;
  int count = 0;
  int _count = 0;
  int _count2 = 5;
  struct rectangle allocation;

  if(!panel->allocation_set) {
    wl_list_for_each(launcher, &panel->launcher_list, link) {
      //Special case for shutdown/volume icons
      if(!launcher->initial_x) {
        widget_set_allocation(launcher->widget, x, y,
          w + first_pad_w + 1, h + first_pad_h + 1);
      } else {
        widget_set_allocation(launcher->widget, launcher->initial_x, y,
          w + first_pad_w + 1, h + first_pad_h + 1);
      }

      if (horizontal)
        x += w + first_pad_w;
      else
        y += h + first_pad_h;
      first_pad_h = first_pad_w = 0;
      count++;
    }
  }

	if (panel->clock_format == CLOCK_FORMAT_SECONDS)
		w = 170;
	else /* CLOCK_FORMAT_MINUTES and 24H versions */
		w = 150;

	if (horizontal)
		x = width - w;
	else
		y = height - (h = DEFAULT_SPACING * 3);


	if (panel->clock) {
    if(!panel->clock->force_x)
		  widget_set_allocation(panel->clock->widget,
				      x, y, w + 1, h + 1);
    else
      widget_set_allocation(panel->clock->widget,
				      panel->clock->force_x, y, w + 1, h + 1);

    if(panel->clock_allocation.width == 0)
      widget_get_allocation(panel->clock->widget, &panel->clock_allocation);

    widget_get_allocation(panel->clock->widget, &allocation);

    //Special allocation x for volume/system launchers
    if(!panel->allocation_set) {
      wl_list_for_each(launcher, &panel->launcher_list, link) {
        //if( (_count >= count - 8) && (_count < count - 4) ) {
        if(launcher->offset_right) {
          widget_get_allocation(launcher->widget, &allocation);
          launcher->force_x = panel->clock_allocation.x - 20 - allocation.width * _count2;
          widget_set_allocation(launcher->widget, launcher->force_x, allocation.y,
          allocation.width, allocation.height);

          _count2--;
        }
        _count++;
      }
    }
  }
  panel->allocation_set = 1;
}

static void
panel_destroy(struct panel *panel);

static void
panel_configure(void *data,
		struct weston_desktop_shell *desktop_shell,
		uint32_t edges, struct window *window,
		int32_t width, int32_t height)
{
	struct desktop *desktop = data;
	struct surface *surface = window_get_user_data(window);
	struct panel *panel = container_of(surface, struct panel, base);
	struct output *owner;

  printf("Resizing panel \n");

	if (width < 1 || height < 1) {
		/* Shell plugin configures 0x0 for redundant panel. */
		owner = panel->owner;
		panel_destroy(panel);
		owner->panel = NULL;
		return;
	}

	switch (desktop->panel_position) {
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP:
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM:
    //Panel height
		height = WAYWARD_INITIAL_HEIGHT;
		break;
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT:
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_RIGHT:
		switch (desktop->clock_format) {
		case CLOCK_FORMAT_NONE:
			width = 32;
			break;
		case CLOCK_FORMAT_MINUTES:
		case CLOCK_FORMAT_MINUTES_24H:
		case CLOCK_FORMAT_SECONDS_24H:
			width = 150;
			break;
		case CLOCK_FORMAT_SECONDS:
			width = 170;
			break;
		}
		break;
	}
	window_schedule_resize(panel->window, width, height);
}

static void
panel_destroy_launcher(struct panel_launcher *launcher)
{
	wl_array_release(&launcher->argv);
	wl_array_release(&launcher->envp);

	free(launcher->path);

	cairo_surface_destroy(launcher->icon);

	widget_destroy(launcher->widget);
	wl_list_remove(&launcher->link);

	free(launcher);
}

static void
panel_destroy(struct panel *panel)
{
	struct panel_launcher *tmp;
	struct panel_launcher *launcher;

	if (panel->clock)
		panel_destroy_clock(panel->clock);

	wl_list_for_each_safe(launcher, tmp, &panel->launcher_list, link)
		panel_destroy_launcher(launcher);

	widget_destroy(panel->widget);
	window_destroy(panel->window);

	free(panel);
}

static struct panel *
panel_create(struct desktop *desktop, struct output *output)
{
	struct panel *panel;
	struct weston_config_section *s;
  struct rectangle null_allocation = {0};

	panel = xzalloc(sizeof *panel);

	panel->owner = output;
	panel->base.configure = panel_configure;
	panel->window = window_create_custom(desktop->display);
	panel->widget = window_add_widget(panel->window, panel);
	wl_list_init(&panel->launcher_list);

	window_set_title(panel->window, "panel");
	window_set_user_data(panel->window, panel);

  //TODO
  //disable for now

	window_set_keyboard_focus_handler(panel->window, panel_focus_handler);
	window_set_key_handler(panel->window, panel_key_handler);
	//widget_set_enter_handler(panel->widget, panel_enter_handler);
	//widget_set_leave_handler(panel->widget, panel_leave_handler);

	widget_set_redraw_handler(panel->widget, panel_redraw_handler);
	widget_set_resize_handler(panel->widget, panel_resize_handler);

  panel->allocation_set = 0;

  panel->clock_allocation = null_allocation;

	panel->panel_position = desktop->panel_position;
	panel->clock_format = desktop->clock_format;
  panel->clock_state = CLOCK_NONE;
	if (panel->clock_format != CLOCK_FORMAT_NONE) {
    panel->clock_state = CLOCK_SHOWN;
		panel_add_clock(panel);
    if(global_battery_exists)
		  panel_add_battery(panel);
  }

	s = weston_config_get_section(desktop->config, "shell", NULL, NULL);
	weston_config_section_get_color(s, "panel-color",
					&panel->color, 0xaa000000);

	panel_add_launchers(panel, desktop);



	return panel;
}

static cairo_surface_t *
load_icon_or_fallback(const char *icon)
{
	cairo_surface_t *surface = cairo_image_surface_create_from_png(icon);
	cairo_status_t status;
	cairo_t *cr;

	status = cairo_surface_status(surface);
	if (status == CAIRO_STATUS_SUCCESS)
		return surface;

	cairo_surface_destroy(surface);
	fprintf(stderr, "ERROR loading icon from file '%s', error: '%s'\n",
		icon, cairo_status_to_string(status));

	/* draw fallback icon */
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					     20, 20);
	cr = cairo_create(surface);

	cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
	cairo_paint(cr);

	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_rectangle(cr, 0, 0, 20, 20);
	cairo_move_to(cr, 4, 4);
	cairo_line_to(cr, 16, 16);
	cairo_move_to(cr, 4, 16);
	cairo_line_to(cr, 16, 4);
	cairo_stroke(cr);

	cairo_destroy(cr);

	return surface;
}



cairo_surface_t *
_cairo_image_surface_scale_nearest (cairo_surface_t *image,
				    int              new_width,
				    int              new_height)
{
	cairo_surface_t *scaled;
	int              src_width;
	int              src_height;
	guchar           *p_src;
	guchar           *p_dest;
	int              src_rowstride;
	int              dest_rowstride;
	gfixed           step_x, step_y;
	guchar          *p_src_row;
	guchar          *p_src_col;
	guchar          *p_dest_row;
	guchar          *p_dest_col;
	gfixed           max_row, max_col;
	gfixed           x_src, y_src;
	int              x, y;

	src_width = cairo_image_surface_get_width  (image);

	src_height = cairo_image_surface_get_height (image);
  if(src_width == new_width || src_height == new_height)
    return image;

  scaled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					     new_width,
					     new_height);

	p_src = cairo_image_surface_get_data (image);
	p_dest = cairo_image_surface_get_data (scaled);
	src_rowstride = cairo_image_surface_get_stride (image);
	dest_rowstride = cairo_image_surface_get_stride (scaled);

	cairo_surface_flush (scaled);

	step_x = GDOUBLE_TO_FIXED ((double) src_width / new_width);
	step_y = GDOUBLE_TO_FIXED ((double) src_height / new_height);

	p_dest_row = p_dest;
	p_src_row = p_src;
	max_row = GINT_TO_FIXED (src_height - 1);
	max_col= GINT_TO_FIXED (src_width - 1);
	/* pick the pixel in the middle to avoid the shift effect. */
	y_src = gfixed_div (step_y, GFIXED_2);
	for (y = 0; y < new_height; y++) {
		p_dest_col = p_dest_row;
		p_src_col = p_src_row;

		x_src = gfixed_div (step_x, GFIXED_2);
		for (x = 0; x < new_width; x++) {
			p_src_col = p_src_row + (GFIXED_TO_INT (MIN (x_src, max_col)) << 2); /* p_src_col = p_src_row + x_src * 4 */
			memcpy (p_dest_col, p_src_col, 4);

			p_dest_col += 4;
			x_src += step_x;
		}

		p_dest_row += dest_rowstride;
		y_src += step_y;
		p_src_row = p_src + (GFIXED_TO_INT (MIN (y_src, max_row)) * src_rowstride);
	}

	cairo_surface_mark_dirty (scaled);

	return scaled;
}



static cairo_surface_t* cairo_image_surface_create_from_svg ( const char* filename, int height )
{

    cairo_surface_t *surface = NULL;
    cairo_status_t status;
    int red, green, blue, alpha = 0;
    NSVGimage *image = NULL;
  	NSVGrasterizer *rast = NULL;
  	unsigned char* img_pixels = NULL;
  	unsigned char* rgba_img_pixels = NULL;
  	int w, h;
 //   char path_buf[1256];
    int skip_next = 0;
    static int count = 0;

  	image = nsvgParseFromFile(filename, "px", 96.0f);
  	if (image == NULL) {
  		printf("Could not open SVG image.\n");
  		return NULL;
  	}
  	w = (int)image->width;
  	h = (int)image->height;
  	w = 32;
  	h = 32;
  	double scale = (double) height / image->height;

  	rast = nsvgCreateRasterizer();
  	if (rast == NULL) {
  		printf("Could not init rasterizer.\n");
  		return NULL;
  	}

  	img_pixels = malloc(w*h*4);
  	rgba_img_pixels = malloc(w*h*4);
  	if (img_pixels == NULL) {
  		printf("Could not alloc image buffer.\n");
  		return NULL;
  	}

  	int pixel;
    int size = w * h;

  	//scaleX = width_of_canvas / (float)image->width; scaleY = height_of_canvas / (float)image->height;
  	//nsvgRasterizeXY(rast, image, 0, 0, scaleX, scaleY, img_data.data(), w, h, w * 4);

    //snprintf(path_buf, sizeof path_buf, "%s/pngs/%d-svg.png", getenv("HOME"), count);
    //count++;

//  	printf("rasterizing image %f x %f -> to 32 32 %f \n",
//  	  image->width, image->height, scale);
  	nsvgRasterize(rast, image, 0, 0, scale, img_pixels, w, h, w*4);

    //Alphas need to be pre-multiplied
    //https://gitlab.freedesktop.org/cairo/cairo/-/blob/8f1190dc825ad9ca805c39025dcbcb9aed8b496d/src/cairo.h#L410-411
    #if 0
  	printf("writing svg.png\n");
 	 stbi_write_png(path_buf, w, h, 4, img_pixels, w*4);

  	surface = cairo_image_surface_create_from_png(path_buf);



  	status = cairo_surface_status(surface);
  	if (status == CAIRO_STATUS_SUCCESS) {
  	  free(img_pixels);
  		return surface;
  	}
    #endif

    int rows = w * h;

    for (unsigned int *rgba_ptr = (unsigned int *)img_pixels,
      *argb_ptr = (unsigned int *)rgba_img_pixels;
      rows > 0; rows--, argb_ptr++, rgba_ptr++)
    {
        red   = (*rgba_ptr >> 0) & 0xff;
        green = (*rgba_ptr >> 8) & 0xff;
        blue  = (*rgba_ptr >> 16) & 0xff;
        alpha  = (*rgba_ptr >> 24);

        *argb_ptr = blue | green << 8 | red << 16 | alpha << 24;
    }

    surface = cairo_image_surface_create_for_data ( rgba_img_pixels,
                                               CAIRO_FORMAT_ARGB32,
                                               w, h, w*4 );
    free(img_pixels);
    return surface;


               /*
cairo_t *cr = cairo_create ( surface );
cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1);
		cairo_mask_surface(cr, surface,
				   32, 32);
cairo_destroy ( cr );

*/

    status = cairo_surface_status(surface);
    if (status == CAIRO_STATUS_SUCCESS) {
      return surface;
    }
    return NULL;

    #if 0
    RsvgHandle      * handle;
    handle = rsvg_handle_new_from_file ( file, NULL );
    if ( handle != NULL ) {
        RsvgDimensionData dimensions;
        // Update DPI.
        rsvg_handle_set_dpi ( handle, 88 );
        // Get size.
        rsvg_handle_get_dimensions ( handle, &dimensions );
        // Create cairo surface in the right size.
        double scale = (double) height / dimensions.height;

        #if 0
        printf("%s %f %f %f scale dimensions width height \n", file, scale, dimensions.width, dimensions.height);
        printf("%s %f %f %f scale dimensionsxscale width height \n", file, scale, (double) dimensions.width * scale, (double) dimensions.height * scale);
        #endif

        surface = cairo_image_surface_create ( CAIRO_FORMAT_ARGB32,
                                               (double) dimensions.width * scale,
                                               (double) dimensions.height * scale );
        cairo_status_t status;
        status = cairo_surface_status(surface);

        if (status == CAIRO_STATUS_SUCCESS) {
            cairo_t *cr = cairo_create ( surface );
            cairo_scale ( cr, scale, scale );
            rsvg_handle_render_cairo ( handle, cr );
            cairo_destroy ( cr );
        }

        rsvg_handle_close ( handle, NULL );


        #if 0
        if ( G_UNLIKELY ( failed ) ) {
            g_warning ( "Failed to render file: '%s'", file );
            cairo_surface_destroy ( surface );
            surface = NULL;
        }
        #endif
    }


    return surface;
    #endif
}


static cairo_surface_t *
load_icon_svg_or_fallback(const char *icon)
{
	cairo_status_t status;
	cairo_t *cr;
	cairo_surface_t *surface = cairo_image_surface_create_from_svg(icon, WAYWARD_ICON_SIZE);

  if(surface) {
  	status = cairo_surface_status(surface);
	  if (status == CAIRO_STATUS_SUCCESS)
		  return surface;
		else
    	cairo_surface_destroy(surface);
	}


	fprintf(stderr, "ERROR loading icon from file '%s', error: '%s'\n",
		icon, cairo_status_to_string(status));

	/* draw fallback icon */
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					     20, 20);
	cr = cairo_create(surface);

	cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1);
	cairo_paint(cr);

	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_rectangle(cr, 0, 0, 20, 20);
	cairo_move_to(cr, 4, 4);
	cairo_line_to(cr, 16, 16);
	cairo_move_to(cr, 4, 16);
	cairo_line_to(cr, 16, 4);
	cairo_stroke(cr);

	cairo_destroy(cr);

	return surface;
}

static struct panel_launcher *panel_add_launcher(struct panel *panel, const char *icon,
   const char *path, int initial_x, void (*function)(struct panel_launcher *widget)
    )
{
	struct panel_launcher *launcher;
	char *start, *p, *eq, **ps;
	int i, j, k;

	launcher = xzalloc(sizeof *launcher);

  if (strstr(icon, ".png") != NULL) {
	  launcher->icon = load_icon_or_fallback(icon);
	  launcher->icon = _cairo_image_surface_scale_nearest(launcher->icon, WAYWARD_ICON_SIZE, WAYWARD_ICON_SIZE);
  } else if( strstr(icon, ".svg") != NULL ) {
    launcher->icon = load_icon_svg_or_fallback(icon);
  }
	launcher->path = xstrdup(path);

  if(initial_x) {
    printf("Launcher initial X is %d \n", initial_x);
    launcher->initial_x = initial_x;
  } else {
    launcher->initial_x = 0;
  }

  launcher->toggled = 0;
  launcher->offset_right = 0;

  launcher->function = NULL;
  if(function) {
    launcher->function = function;
    //move special launchers to the right
    if(!launcher->initial_x)
      launcher->offset_right = 1;
  }


	wl_array_init(&launcher->envp);
	wl_array_init(&launcher->argv);
	for (i = 0; environ[i]; i++) {
		ps = wl_array_add(&launcher->envp, sizeof *ps);
		*ps = environ[i];
	}
	j = 0;

	start = launcher->path;
	while (*start) {
		for (p = start, eq = NULL; *p && !isspace(*p); p++)
			if (*p == '=')
				eq = p;

		if (eq && j == 0) {
			ps = launcher->envp.data;
			for (k = 0; k < i; k++)
				if (strncmp(ps[k], start, eq - start) == 0) {
					ps[k] = start;
					break;
				}
			if (k == i) {
				ps = wl_array_add(&launcher->envp, sizeof *ps);
				*ps = start;
				i++;
			}
		} else {
			ps = wl_array_add(&launcher->argv, sizeof *ps);
			*ps = start;
			j++;
		}

		while (*p && isspace(*p))
			*p++ = '\0';

		start = p;
	}

	ps = wl_array_add(&launcher->envp, sizeof *ps);
	*ps = NULL;
	ps = wl_array_add(&launcher->argv, sizeof *ps);
	*ps = NULL;

	launcher->panel = panel;
	wl_list_insert(panel->launcher_list.prev, &launcher->link);

	launcher->widget = widget_add_widget(panel->widget, launcher);
	widget_set_enter_handler(launcher->widget,
				 panel_launcher_enter_handler);
	widget_set_leave_handler(launcher->widget,
				   panel_launcher_leave_handler);
	widget_set_button_handler(launcher->widget,
				    panel_launcher_button_handler);
	widget_set_touch_down_handler(launcher->widget,
				      panel_launcher_touch_down_handler);
	widget_set_touch_up_handler(launcher->widget,
				    panel_launcher_touch_up_handler);
	widget_set_redraw_handler(launcher->widget,
				  panel_launcher_redraw_handler);
	widget_set_motion_handler(launcher->widget,
				  panel_launcher_motion_handler);

  return launcher;
}

enum {
	BACKGROUND_SCALE,
	BACKGROUND_SCALE_CROP,
	BACKGROUND_TILE,
	BACKGROUND_CENTERED
};

static void
background_draw(struct widget *widget, void *data)
{
	struct background *background = data;
	cairo_surface_t *surface, *image;
	cairo_pattern_t *pattern;
	cairo_matrix_t matrix;
	cairo_t *cr;
	double im_w, im_h;
	double sx, sy, s;
	double tx, ty;
	struct rectangle allocation;

	surface = window_get_surface(background->window);

	cr = widget_cairo_create(background->widget);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	if (background->color == 0)
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.2, 1.0);
	else
		set_hex_color(cr, background->color);
	cairo_paint(cr);

	widget_get_allocation(widget, &allocation);
	image = NULL;
	if (background->image)
		image = load_cairo_surface(background->image);
	else if (background->color == 0) {
		char *name = file_name_with_datadir("pattern.png");

		image = load_cairo_surface(name);
		free(name);
	}

	if (image && background->type != -1) {
		im_w = cairo_image_surface_get_width(image);
		im_h = cairo_image_surface_get_height(image);
		sx = im_w / allocation.width;
		sy = im_h / allocation.height;

		pattern = cairo_pattern_create_for_surface(image);

		switch (background->type) {
		case BACKGROUND_SCALE:
			cairo_matrix_init_scale(&matrix, sx, sy);
			cairo_pattern_set_matrix(pattern, &matrix);
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
			break;
		case BACKGROUND_SCALE_CROP:
			s = (sx < sy) ? sx : sy;
			/* align center */
			tx = (im_w - s * allocation.width) * 0.5;
			ty = (im_h - s * allocation.height) * 0.5;
			cairo_matrix_init_translate(&matrix, tx, ty);
			cairo_matrix_scale(&matrix, s, s);
			cairo_pattern_set_matrix(pattern, &matrix);
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
			break;
		case BACKGROUND_TILE:
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
			break;
		case BACKGROUND_CENTERED:
			s = (sx < sy) ? sx : sy;
			if (s < 1.0)
				s = 1.0;

			/* align center */
			tx = (im_w - s * allocation.width) * 0.5;
			ty = (im_h - s * allocation.height) * 0.5;

			cairo_matrix_init_translate(&matrix, tx, ty);
			cairo_matrix_scale(&matrix, s, s);
			cairo_pattern_set_matrix(pattern, &matrix);
			break;
		}

		cairo_set_source(cr, pattern);
		cairo_pattern_destroy (pattern);
		cairo_surface_destroy(image);
		cairo_mask(cr, pattern);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	background->painted = 1;
	check_desktop_ready(background->window);
}

static void
background_destroy(struct background *background);

static void
background_configure(void *data,
		     struct weston_desktop_shell *desktop_shell,
		     uint32_t edges, struct window *window,
		     int32_t width, int32_t height)
{
	struct output *owner;
	struct background *background =
		(struct background *) window_get_user_data(window);

	if (width < 1 || height < 1) {
		/* Shell plugin configures 0x0 for redundant background. */
		owner = background->owner;
		background_destroy(background);
		owner->background = NULL;
		return;
	}

	if (!background->image && background->color) {
		widget_set_viewport_destination(background->widget, width, height);
		width = 1;
		height = 1;
	}

	widget_schedule_resize(background->widget, width, height);
}

static void
unlock_dialog_redraw_handler(struct widget *widget, void *data)
{
	struct unlock_dialog *dialog = data;
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_pattern_t *pat;
	double cx, cy, r, f;
	struct wl_surface *surface2;

	cr = widget_cairo_create(widget);

	widget_get_allocation(dialog->widget, &allocation);
	cairo_rectangle(cr, allocation.x, allocation.y,
			allocation.width, allocation.height);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
	cairo_fill(cr);

	cairo_translate(cr, allocation.x, allocation.y);
	if (dialog->button_focused)
		f = 1.0;
	else
		f = 0.7;

	cx = allocation.width / 2.0;
	cy = allocation.height / 2.0;
	r = (cx < cy ? cx : cy) * 0.4;
	pat = cairo_pattern_create_radial(cx, cy, r * 0.7, cx, cy, r);
	cairo_pattern_add_color_stop_rgb(pat, 0.0, 0, 0.86 * f, 0);
	cairo_pattern_add_color_stop_rgb(pat, 0.85, 0.2 * f, f, 0.2 * f);
	cairo_pattern_add_color_stop_rgb(pat, 1.0, 0, 0.86 * f, 0);
	cairo_set_source(cr, pat);
	cairo_pattern_destroy(pat);
	cairo_arc(cr, cx, cy, r, 0.0, 2.0 * M_PI);
	cairo_fill(cr);

	widget_set_allocation(dialog->button,
			      allocation.x + cx - r,
			      allocation.y + cy - r, 2 * r, 2 * r);

  for (int i = 0; i < dialog->keys_entered && i < 6; i++) {
    	cx = 25;
    	cy = 20;
    	r = (cx < cy ? cx : cy) * 0.4;
//    	pat = cairo_pattern_create_radial(cx*i, cy, r * 0.7, cx*i, cy, r);
    	//cairo_pattern_add_color_stop_rgb(pat, 1.0, 0.2, 0.86, 0);
    	//cairo_set_source(cr, pat);
      set_hex_color(cr, 0xff0040ff);
  		//cairo_set_source_rgba(cr, 0.3, 0.4, 0.2, 1.0);
//    	cairo_pattern_destroy(pat);
    	cairo_arc(cr, cx * ( i + 1), cy, r, 0.0, 2.0 * M_PI);
    	cairo_fill(cr);
  }

	cairo_destroy(cr);

	surface = window_get_surface(dialog->window);
	cairo_surface_destroy(surface);

  //focus keyboard for pin
	surface2 = window_get_wl_surface(dialog->window);

	shell_helper_keyboard_focus_surface(global_desktop->helper,
	  surface2);
}



static void
unlock_dialog_button_handler(struct widget *widget,
			     struct input *input, uint32_t time,
			     uint32_t button,
			     enum wl_pointer_button_state state, void *data)
{
	struct unlock_dialog *dialog = data;
	struct desktop *desktop = dialog->desktop;

	if (desktop->pincode != NULL) {
	  if(
	    !dialog->keys_entered ||
  	  !xxtea_pincode_valid(desktop->pincode, dialog->pincode) != 0
  	) {
      return;
    }
	}


	if (button == BTN_LEFT) {
		if (state == WL_POINTER_BUTTON_STATE_RELEASED &&
		    !dialog->closing) {
			display_defer(desktop->display, &desktop->unlock_task);
			dialog->closing = 1;
		}
	}
}


static void unlock_dialog_key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym, enum wl_keyboard_key_state state,
	    void *data)
{
  struct unlock_dialog *dialog = data;
  struct desktop *desktop = dialog->desktop;
  int add = 0;

  if (state != WL_KEYBOARD_KEY_STATE_RELEASED) {
    return;
  }

 switch (key) {

    case KEY_ENTER:
      unlock_dialog_button_handler(NULL,
			     NULL, 0,
			     BTN_LEFT,
			     WL_POINTER_BUTTON_STATE_RELEASED, data);
    break;
    case KEY_BACKSPACE:
      dialog->keys_entered = 0;
      dialog->pincode[0] = 0;
    break;
    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:

      if(!desktop->pincode) {
        return;
      }

      printf("Entered key %d %c \n", (unsigned int)key,
        keycode_to_vkey[(unsigned int)key]);


      if(dialog->keys_entered < 6) {

        if(dialog->keys_entered > 0)
          add = strlen(dialog->pincode);

        sprintf(dialog->pincode + add,
          "%c",
          keycode_to_vkey[(unsigned int)key]);
        printf("Entered %s \n", dialog->pincode);

        dialog->keys_entered++;

      }

    break;

    default:
    break;
  }


  widget_schedule_redraw(dialog->widget);

  /*


  //struct widget *widget = dialog->widget;
  printf("Key entered \n");

  struct rectangle allocation;
  cairo_surface_t *surface;
	cairo_t *cr;
	cairo_pattern_t *pat;
	double cx, cy, r, f;

	cr = widget_cairo_create(dialog->widget);

	widget_get_allocation(dialog->widget, &allocation);
	cairo_rectangle(cr, allocation.x, allocation.y,
			allocation.width, allocation.height);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
	cairo_fill(cr);

	cairo_translate(cr, allocation.x, allocation.y);
	if (dialog->button_focused)
		f = 1.0;
	else
		f = 0.7;

	cx = allocation.width / 12.0;
	cy = allocation.height / 12.0;
	r = (cx < cy ? cx : cy) * 0.4;
	pat = cairo_pattern_create_radial(cx, cy, r * 0.7, cx, cy, r);
	cairo_pattern_add_color_stop_rgb(pat, 0.0, 0, 0.86 * f, 0);
	cairo_pattern_add_color_stop_rgb(pat, 0.85, 0.2 * f, f, 0.2 * f);
	cairo_pattern_add_color_stop_rgb(pat, 1.0, 0, 0.86 * f, 0);
	cairo_set_source(cr, pat);
	cairo_pattern_destroy(pat);
	cairo_arc(cr, cx, cy, r, 0.0, 2.0 * M_PI);
	cairo_fill(cr);


 cairo_destroy(cr);

	surface = window_get_surface(dialog->window);
	cairo_surface_destroy(surface);

*/
}



static void
unlock_dialog_touch_down_handler(struct widget *widget, struct input *input,
		   uint32_t serial, uint32_t time, int32_t id,
		   float x, float y, void *data)
{
	struct unlock_dialog *dialog = data;

	dialog->button_focused = 1;
	widget_schedule_redraw(widget);
}

static void
unlock_dialog_touch_up_handler(struct widget *widget, struct input *input,
				uint32_t serial, uint32_t time, int32_t id,
				void *data)
{
	struct unlock_dialog *dialog = data;
	struct desktop *desktop = dialog->desktop;

	dialog->button_focused = 0;
	widget_schedule_redraw(widget);
	display_defer(desktop->display, &desktop->unlock_task);
	dialog->closing = 1;
}

static void
unlock_dialog_keyboard_focus_handler(struct window *window,
				     struct input *device, void *data)
{
	window_schedule_redraw(window);
}

static int
unlock_dialog_widget_enter_handler(struct widget *widget,
				   struct input *input,
				   float x, float y, void *data)
{
	struct unlock_dialog *dialog = data;

	dialog->button_focused = 1;
	widget_schedule_redraw(widget);

	return CURSOR_LEFT_PTR;
}

static void
unlock_dialog_widget_leave_handler(struct widget *widget,
				   struct input *input, void *data)
{
	struct unlock_dialog *dialog = data;

	dialog->button_focused = 0;
	widget_schedule_redraw(widget);
}

static struct unlock_dialog *
unlock_dialog_create(struct desktop *desktop)
{
	struct display *display = desktop->display;
	struct unlock_dialog *dialog;
	struct wl_surface *surface;

	dialog = xzalloc(sizeof *dialog);

	dialog->window = window_create_custom(display);
	dialog->widget = window_frame_create(dialog->window, dialog);
	window_set_title(dialog->window, "Unlock desktop");

	window_set_user_data(dialog->window, dialog);
	window_set_keyboard_focus_handler(dialog->window,
					  unlock_dialog_keyboard_focus_handler);
  window_set_key_handler(dialog->window, unlock_dialog_key_handler);
	dialog->button = widget_add_widget(dialog->widget, dialog);
	widget_set_redraw_handler(dialog->widget,
				  unlock_dialog_redraw_handler);
	widget_set_enter_handler(dialog->button,
				 unlock_dialog_widget_enter_handler);
	widget_set_leave_handler(dialog->button,
				 unlock_dialog_widget_leave_handler);
	widget_set_button_handler(dialog->button,
				  unlock_dialog_button_handler);
	widget_set_touch_down_handler(dialog->button,
				      unlock_dialog_touch_down_handler);
	widget_set_touch_up_handler(dialog->button,
				      unlock_dialog_touch_up_handler);

	surface = window_get_wl_surface(dialog->window);
	weston_desktop_shell_set_lock_surface(desktop->shell, surface);

	window_schedule_resize(dialog->window, 400, 400);

	return dialog;
}

static void
unlock_dialog_destroy(struct unlock_dialog *dialog)
{
	window_destroy(dialog->window);
	free(dialog);
}

static void
unlock_dialog_finish(struct task *task, uint32_t events)
{
	struct desktop *desktop =
		container_of(task, struct desktop, unlock_task);

	weston_desktop_shell_unlock(desktop->shell);
	unlock_dialog_destroy(desktop->unlock_dialog);
	desktop->unlock_dialog = NULL;
}

static void
desktop_shell_configure(void *data,
			struct weston_desktop_shell *desktop_shell,
			uint32_t edges,
			struct wl_surface *surface,
			int32_t width, int32_t height)
{
	struct window *window = wl_surface_get_user_data(surface);
	struct surface *s = window_get_user_data(window);



	s->configure(data, desktop_shell, edges, window, width, height);
}

static void
desktop_shell_prepare_lock_surface(void *data,
				   struct weston_desktop_shell *desktop_shell)
{
	struct desktop *desktop = data;

	if (!desktop->locking) {
		weston_desktop_shell_unlock(desktop->shell);
		return;
	}

	if (!desktop->unlock_dialog) {
		desktop->unlock_dialog = unlock_dialog_create(desktop);
		desktop->unlock_dialog->desktop = desktop;
	}
}

static void
desktop_shell_grab_cursor(void *data,
			  struct weston_desktop_shell *desktop_shell,
			  uint32_t cursor)
{
	struct desktop *desktop = data;

	switch (cursor) {
	case WESTON_DESKTOP_SHELL_CURSOR_NONE:
		desktop->grab_cursor = CURSOR_BLANK;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_BUSY:
		desktop->grab_cursor = CURSOR_WATCH;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_MOVE:
		desktop->grab_cursor = CURSOR_DRAGGING;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_TOP:
		desktop->grab_cursor = CURSOR_TOP;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM:
		desktop->grab_cursor = CURSOR_BOTTOM;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_LEFT:
		desktop->grab_cursor = CURSOR_LEFT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_RIGHT:
		desktop->grab_cursor = CURSOR_RIGHT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_TOP_LEFT:
		desktop->grab_cursor = CURSOR_TOP_LEFT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_TOP_RIGHT:
		desktop->grab_cursor = CURSOR_TOP_RIGHT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_LEFT:
		desktop->grab_cursor = CURSOR_BOTTOM_LEFT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_RESIZE_BOTTOM_RIGHT:
		desktop->grab_cursor = CURSOR_BOTTOM_RIGHT;
		break;
	case WESTON_DESKTOP_SHELL_CURSOR_ARROW:
	default:
		desktop->grab_cursor = CURSOR_LEFT_PTR;
	}
}

static const struct weston_desktop_shell_listener listener = {
	desktop_shell_configure,
	desktop_shell_prepare_lock_surface,
	desktop_shell_grab_cursor
};

static void
background_destroy(struct background *background)
{
	widget_destroy(background->widget);
	window_destroy(background->window);

	free(background->image);
	free(background);
}

static struct background *
background_create(struct desktop *desktop, struct output *output)
{
	struct background *background;
	struct weston_config_section *s;
	char *type;

	background = xzalloc(sizeof *background);
	background->owner = output;
	background->base.configure = background_configure;
	background->window = window_create_custom(desktop->display);

	background->widget = window_add_widget(background->window, background);
	window_set_user_data(background->window, background);
	widget_set_redraw_handler(background->widget, background_draw);
	widget_set_transparent(background->widget, 0);

	s = weston_config_get_section(desktop->config, "shell", NULL, NULL);
	weston_config_section_get_string(s, "background-image",
					 &background->image, NULL);
	weston_config_section_get_color(s, "background-color",
					&background->color, 0x00000000);

	weston_config_section_get_string(s, "background-type",
					 &type, "tile");
	if (type == NULL) {
		fprintf(stderr, "%s: out of memory\n", program_invocation_short_name);
		exit(EXIT_FAILURE);
	}

	if (strcmp(type, "scale") == 0) {
		background->type = BACKGROUND_SCALE;
	} else if (strcmp(type, "scale-crop") == 0) {
		background->type = BACKGROUND_SCALE_CROP;
	} else if (strcmp(type, "tile") == 0) {
		background->type = BACKGROUND_TILE;
	} else if (strcmp(type, "centered") == 0) {
		background->type = BACKGROUND_CENTERED;
	} else {
		background->type = -1;
		fprintf(stderr, "invalid background-type: %s\n",
			type);
	}

	free(type);

	return background;
}

static int
grab_surface_enter_handler(struct widget *widget, struct input *input,
			   float x, float y, void *data)
{
	struct desktop *desktop = data;

	return desktop->grab_cursor;
}

static void
grab_surface_destroy(struct desktop *desktop)
{
	widget_destroy(desktop->grab_widget);
	window_destroy(desktop->grab_window);
}

static void
grab_surface_create(struct desktop *desktop)
{
	struct wl_surface *s;

	desktop->grab_window = window_create_custom(desktop->display);
	window_set_user_data(desktop->grab_window, desktop);

	s = window_get_wl_surface(desktop->grab_window);
	weston_desktop_shell_set_grab_surface(desktop->shell, s);

	desktop->grab_widget =
		window_add_widget(desktop->grab_window, desktop);
	/* We set the allocation to 1x1 at 0,0 so the fake enter event
	 * at 0,0 will go to this widget. */
	widget_set_allocation(desktop->grab_widget, 0, 0, 1, 1);

	widget_set_enter_handler(desktop->grab_widget,
				 grab_surface_enter_handler);
}

static void
output_destroy(struct output *output)
{
	if (output->background)
		background_destroy(output->background);
	if (output->panel)
		panel_destroy(output->panel);
	wl_output_destroy(output->output);
	wl_list_remove(&output->link);

	free(output);
}

static void
desktop_destroy_outputs(struct desktop *desktop)
{
	struct output *tmp;
	struct output *output;

	wl_list_for_each_safe(output, tmp, &desktop->outputs, link)
		output_destroy(output);
}

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
	struct output *output = data;

	output->x = x;
	output->y = y;

	if (output->panel)
		window_set_buffer_transform(output->panel->window, transform);
	if (output->background)
		window_set_buffer_transform(output->background->window, transform);
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
  }
  if(height > global_desktop_height) {
    global_desktop_height = height;
  }

  printf("Found output with WxH %d %d \n", global_desktop_width, global_desktop_height);

}

static void
output_handle_done(void *data,
                   struct wl_output *wl_output)
{
}

static void
output_handle_scale(void *data,
                    struct wl_output *wl_output,
                    int32_t scale)
{
	struct output *output = data;

	if (output->panel)
		window_set_buffer_scale(output->panel->window, scale);
	if (output->background)
		window_set_buffer_scale(output->background->window, scale);
}

static const struct wl_output_listener output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale
};

static void
output_init(struct output *output, struct desktop *desktop)
{
	struct wl_surface *surface;

	if (desktop->want_panel) {
		output->panel = panel_create(desktop, output);
		surface = window_get_wl_surface(output->panel->window);
    output->panel->wl_surface = surface;

		weston_desktop_shell_set_panel(desktop->shell,
		  output->output, surface);

    shell_helper_set_panel (desktop->helper, surface);

    printf("2 \n");
    //Set panel initial position
    if(!output->panel->wl_surface)
      exit(1);
    printf("3 \n");



    printf("4 \n");

    if(!desktop->panel)
      desktop->panel = output->panel;

    shell_helper_bind_key_panel(desktop->helper, surface, desktop->seat, desktop->shell);


	}

	output->background = background_create(desktop, output);
	surface = window_get_wl_surface(output->background->window);
	weston_desktop_shell_set_background(desktop->shell,
					    output->output, surface);
}

static void
create_output(struct desktop *desktop, uint32_t id)
{
	struct output *output;

	output = zalloc(sizeof *output);
	if (!output)
		return;

	output->output =
		display_bind(desktop->display, id, &wl_output_interface, 2);
	output->server_output_id = id;

	wl_output_add_listener(output->output, &output_listener, output);

	wl_list_insert(&desktop->outputs, &output->link);

	/* On start up we may process an output global before the shell global
	 * in which case we can't create the panel and background just yet */
	if (desktop->shell)
		output_init(output, desktop);
}

static void
output_remove(struct desktop *desktop, struct output *output)
{
	struct output *cur;
	struct output *rep = NULL;

	if (!output->background) {
		output_destroy(output);
		return;
	}

	/* Find a wl_output that is a clone of the removed wl_output.
	 * We don't want to leave the clone without a background or panel. */
	wl_list_for_each(cur, &desktop->outputs, link) {
		if (cur == output)
			continue;

		/* XXX: Assumes size matches. */
		if (cur->x == output->x && cur->y == output->y) {
			rep = cur;
			break;
		}
	}

	if (rep) {
		/* If found and it does not already have a background or panel,
		 * hand over the background and panel so they don't get
		 * destroyed.
		 *
		 * We never create multiple backgrounds or panels for clones,
		 * but if the compositor moves outputs, a pair of wl_outputs
		 * might become "clones". This may happen temporarily when
		 * an output is about to be removed and the rest are reflowed.
		 * In this case it is correct to let the background/panel be
		 * destroyed.
		 */

		if (!rep->background) {
			rep->background = output->background;
			output->background = NULL;
			rep->background->owner = rep;
		}

		if (!rep->panel) {
			rep->panel = output->panel;
			output->panel = NULL;
			if (rep->panel)
				rep->panel->owner = rep;
		}
	}

	output_destroy(output);
}

void wayland_pointer_enter_cb(void *data,
		struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface,
		wl_fixed_t sx, wl_fixed_t sy)
{

  if(!global_desktop || !global_desktop->panel)
    return;


  if(surface == global_desktop->panel->wl_surface) {
    global_panel_in = 1;
    shell_helper_move_surface (global_desktop->helper,
        global_desktop->panel->wl_surface,
        WAYWARD_NO_MOVE_X, global_desktop_height - WAYWARD_INITIAL_HEIGHT
    );
  }
}
//TODO fix without global_desktop
void wayland_pointer_leave_cb(void *data,
		struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
  if(!global_desktop || !global_desktop->panel || global_panel_in_y > WAYWARD_PANEL_LEAVE_BUG_Y)
    return;

  if(surface == global_desktop->panel->wl_surface) {
    global_panel_in = 0;
    printf("Hiding panel surface \n");
    shell_helper_move_surface (global_desktop->helper,
        global_desktop->panel->wl_surface,
        WAYWARD_NO_MOVE_X, global_desktop_height - 5
    );

    if(global_desktop->panel->clock_state == VOLUME_SHOWN) {
      launch_volume(global_desktop->panel->volumeup_launcher);
    } else if(global_desktop->panel->clock_state == SYSTEM_SHOWN) {
      launch_system(global_desktop->panel->volumeup_launcher);
    }

    window_schedule_resize(global_desktop->panel->window, global_desktop_width, 50);
  }
}


void wayland_pointer_motion_cb(void *data,
		struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{

  //Fix for Weston bug when mouse at the edge of the screen bottom
  if(global_panel_in) {
    global_panel_in_y = wl_fixed_to_int(sy);
  }

}






void wayland_pointer_button_cb(void *data,
		struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button,
		uint32_t state)
{}

static void wayland_pointer_frame_cb(void *data, struct wl_pointer *wl_pointer) {
  //do nothing

}
static void wayland_pointer_axis_source_cb(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)	{

  //do nothing
}
static void wayland_pointer_axis_stop_cb(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)	{

  //do nothing
}


static void wayland_pointer_axis_discrete_cb(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
}

//Mouse wheel
void wayland_pointer_axis_cb(void *data,
		struct wl_pointer *pointer, uint32_t time, uint32_t axis,
		wl_fixed_t value)
{

}

static const struct wl_pointer_listener pointer_listener_wayland =
      {   wayland_pointer_enter_cb,
          wayland_pointer_leave_cb,
          wayland_pointer_motion_cb,
          wayland_pointer_button_cb,
          wayland_pointer_axis_cb,

          /*wayland_pointer_frame_cb,
          wayland_pointer_axis_source_cb,
          wayland_pointer_axis_stop_cb,
          wayland_pointer_axis_discrete_cb,
          */
      };


/* Keyboard */

static void keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {

}



static void keyboard_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {



}
static void keyboard_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
  printf (" Unfocus keyboard \n");

}
static void keyboard_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {

}
static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {

}
static struct wl_keyboard_listener keyboard_listener = {
  keyboard_keymap,
  keyboard_enter,
  keyboard_leave,
  keyboard_key,
  keyboard_modifiers
};

static void
seat_handle_capabilities (void *data,
    struct wl_seat *seat,
    enum wl_seat_capability caps)
{


  struct desktop *desktop = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !desktop->pointer) {
    desktop->pointer = wl_seat_get_pointer(seat);
    wl_pointer_set_user_data (desktop->pointer, desktop);
    wl_pointer_add_listener(desktop->pointer, &pointer_listener_wayland, desktop);
    //fprintf ( f, "the mouse was added");
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && desktop->pointer) {
    wl_pointer_destroy(desktop->pointer);
    desktop->pointer = NULL;
  }
  #if 0
   if ( (caps & WL_SEAT_CAPABILITY_KEYBOARD) && !desktop->keyboard) {
     desktop->keyboard = wl_seat_get_keyboard(seat);
     wl_keyboard_add_listener (desktop->keyboard, &keyboard_listener, desktop);
     //fprintf ( f, "the keyboard was added");
   } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && desktop->keyboard) {
    wl_keyboard_destroy(desktop->keyboard);
    desktop->keyboard = NULL;

  }
  #endif
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

static void
global_handler(struct display *display, uint32_t id,
	       const char *interface, uint32_t version, void *data)
{
	struct desktop *desktop = data;

	if (!strcmp(interface, "weston_desktop_shell")) {
		desktop->shell = display_bind(desktop->display,
					      id,
					      &weston_desktop_shell_interface,
					      1);
		weston_desktop_shell_add_listener(desktop->shell,
						  &listener,
						  desktop);
	} else if (!strcmp(interface, "wl_output")) {
		create_output(desktop, id);
	} else if (!strcmp (interface, "shell_helper")) {
      desktop->helper = display_bind (desktop->display,
					id,
      &shell_helper_interface, 1);
  } else if (!strcmp (interface, "wl_seat"))
    {
      desktop->seat = display_bind (desktop->display,
        id,
        &wl_seat_interface, 1);
      wl_seat_add_listener (desktop->seat, &seat_listener, desktop);
    }
}

static void
global_handler_remove(struct display *display, uint32_t id,
	       const char *interface, uint32_t version, void *data)
{
	struct desktop *desktop = data;
	struct output *output;

	if (!strcmp(interface, "wl_output")) {
		wl_list_for_each(output, &desktop->outputs, link) {
			if (output->server_output_id == id) {
				output_remove(desktop, output);
				break;
			}
		}
	}
}

//trim utility
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

//Alsa setup
static void setup_mixer (struct desktop *desktop)
{
  snd_mixer_selem_id_t *sid;
  int ret;


  if ((ret = snd_mixer_open (&desktop->mixer_handle, 0)) < 0)
    goto error;

  if ((ret = snd_mixer_attach (desktop->mixer_handle, "default")) < 0)
    goto error;

  if ((ret = snd_mixer_selem_register (desktop->mixer_handle, NULL, NULL)) < 0)
    goto error;

  if ((ret = snd_mixer_load (desktop->mixer_handle)) < 0)
    goto error;

  snd_mixer_selem_id_alloca (&sid);
  snd_mixer_selem_id_set_index (sid, 0);
  snd_mixer_selem_id_set_name (sid, "Master");
  desktop->mixer = snd_mixer_find_selem (desktop->mixer_handle, sid);

  /* fallback to mixer "Master" */
  if (desktop->mixer == NULL)
    {
      snd_mixer_selem_id_set_name (sid, "PCM");
      desktop->mixer = snd_mixer_find_selem (desktop->mixer_handle, sid);
      if (desktop->mixer == NULL)
        goto error;
    }

  if ((ret = snd_mixer_selem_get_playback_volume_range (desktop->mixer,
              &desktop->min_volume, &desktop->max_volume)) < 0)
    goto error;

  printf("Mixer volumes are max/min %d/%d \n", desktop->max_volume,
    desktop->min_volume);

  return;

error:
  printf ("failed to setup mixer: %s", snd_strerror (ret));

  if (desktop->mixer_handle != NULL)
    snd_mixer_close (desktop->mixer_handle);
  desktop->mixer_handle = NULL;
  desktop->mixer = NULL;
}
//Alsa

//Battery
void check_battery_exists() {

  #if 0
  //TESTING
  global_battery_exists = 1;
  sprintf(global_battery_path, "/sys/class/power_supply/BAT0/capacity");
  sprintf(global_battery_path, "/sys/class/graphics/fb0/state");
  return;
  #endif

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
//Battery

//Shm commands

//shm audio commands
static double
alsa_volume_to_percentage (long value)
{
  long range;

  /* min volume isn't always zero unfortunately */
  range = global_desktop->max_volume - global_desktop->min_volume;

  value -= global_desktop->min_volume;

  return (value / (double) range) * 100;
}

static long
percentage_to_alsa_volume (double value)
{
  long range;

  /* min volume isn't always zero unfortunately */
  range = global_desktop->max_volume - global_desktop->min_volume;

  return (range * value / 100) + global_desktop->min_volume;
}

static void volume_set (double value) {

  const char *icon_name = NULL;


  printf("volume value is %f \n", value);

  #if 0
  if(value > global_desktop->max_volume ) {
    value = global_desktop->max_volume - 1;
  } else if ( value < global_desktop->min_volume ) {
    value = global_desktop->min_volume;
  }
  #endif

  if (global_desktop->mixer != NULL)
  {
    snd_mixer_selem_set_playback_volume_all (
      global_desktop->mixer, percentage_to_alsa_volume (value)
    );
  }

  /* update the icon */
  if (value > 75)
    icon_name = "audio-volume-high-symbolic";
  else if (value > 30)
    icon_name = "audio-volume-medium-symbolic";
  else if (value > 0)
    icon_name = "audio-volume-low-symbolic";
  else
    icon_name = "audio-volume-muted-symbolic";

  printf("icon_name is %s \n", icon_name);


}


//Called from wayward.c

 //battery

static void
panel_battery_redraw_handler(struct widget *widget, void *data)
{
	struct panel_clock *battery = data;
	cairo_t *cr;
	struct rectangle allocation;
	cairo_text_extents_t extents;
	time_t rawtime;
	struct tm * timeinfo;
  char string[7];
  FILE *fp;
  char buff[10] = {'\0'};
  int padding_right = 0;
	int battery_level = 0;


  if(!battery->panel->allocation_set) {
    return;
  }

  fp = fopen(global_battery_path, "r");
  if(fp != NULL) {
    fgets(buff, 10, (FILE*)fp);
    fclose(fp);

    buff[strlen(buff) - 1] = '\0';
    battery_level = buff[0] - '0';
    sprintf(string, "%s%%", buff);
  }

	widget_get_allocation(widget, &allocation);

  battery->panel->painted = 0;
	cr = widget_cairo_create(battery->panel->widget);
	cairo_set_font_size(cr, 22);

  cairo_select_font_face (cr, "Droid Sans",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);

	cairo_text_extents(cr, string, &extents);

  if(battery_level == 100) {
    padding_right = 15;
  }

  allocation.x = battery->panel->clock_allocation.x - WAYWARD_BATTERY_X - padding_right;


	allocation.y = WAYWARD_BATTERY_Y;

	cairo_show_text(cr, string);
	cairo_move_to(cr, allocation.x, allocation.y);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.85);
	cairo_show_text(cr, string);
	cairo_destroy(cr);

  printf("Redrawing battery %d \n", battery_level);

}

static void
panel_add_battery(struct panel *panel)
{
	struct panel_clock *battery;

	battery = xzalloc(sizeof *battery);
	battery->force_x = 0;

	battery->panel = panel;
	panel->battery = battery;

  toytimer_init(&battery->timer, CLOCK_MONOTONIC,
		      window_get_display(panel->window), clock_func);


  struct itimerspec its;
  //In seconds - needs to be zero for nsec to work
	its.it_interval.tv_sec = 30;
  //ms * 1000000
  //runs every 500ms
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 1;

  toytimer_arm(&battery->timer, &its);


	battery->widget = widget_add_widget(panel->widget, battery);
	widget_set_redraw_handler(battery->widget, panel_battery_redraw_handler);
}
 //battery


static void toggle_inhibit(struct panel_launcher *launcher) {

  shell_helper_toggle_inhibit (global_desktop->helper, global_desktop->shell, global_desktop->seat);
  global_power_inhibit = !global_power_inhibit;
  launcher->toggled = !launcher->toggled;

  widget_schedule_redraw(launcher->widget);
}

static void launch_exposay(struct panel_launcher *launcher) {
  shell_helper_launch_exposay (global_desktop->helper, global_desktop->shell, global_desktop->seat);
}
//TODO
//Add support for cases when clock is disabled in the weston settings

static void launch_system(struct panel_launcher *launcher) {

  struct rectangle allocation;
  struct rectangle widget_allocation;
	cairo_t *cr;


  if(launcher->panel->clock_state == VOLUME_SHOWN) {
    launch_volume(launcher);
	}

	widget_get_allocation(launcher->panel->reboot_launcher->widget, &widget_allocation);

  if(launcher->panel->clock_state == CLOCK_SHOWN) {
    launcher->panel->clock->force_x = WAYWARD_HIDE_X;
    launcher->panel->reboot_launcher->force_x = launcher->panel->clock_allocation.x;
    launcher->panel->shutdown_launcher->force_x = launcher->panel->clock_allocation.x + 48;
    launcher->panel->reboot_launcher->initial_x = launcher->panel->clock_allocation.x;
    launcher->panel->shutdown_launcher->initial_x = launcher->panel->clock_allocation.x + 48;
    launcher->panel->clock_state = SYSTEM_SHOWN;

	} else {
    launcher->panel->clock->force_x = 0;
    launcher->panel->reboot_launcher->force_x = WAYWARD_HIDE_X;
    launcher->panel->shutdown_launcher->force_x = WAYWARD_HIDE_X;
    launcher->panel->clock_state = CLOCK_SHOWN;

  }



  widget_set_allocation(launcher->panel->clock->widget,
    launcher->panel->clock->force_x,
    launcher->panel->clock_allocation.y,
		launcher->panel->clock_allocation.width,
    launcher->panel->clock_allocation.height
  );


  widget_set_allocation(launcher->panel->reboot_launcher->widget,
    launcher->panel->reboot_launcher->force_x,
    widget_allocation.y,
		widget_allocation.width,
    widget_allocation.height
  );

  widget_set_allocation(launcher->panel->shutdown_launcher->widget,
    launcher->panel->shutdown_launcher->force_x,
    widget_allocation.y,
		widget_allocation.width,
    widget_allocation.height
  );


  widget_schedule_redraw(launcher->panel->reboot_launcher->widget);
  widget_schedule_redraw(launcher->panel->shutdown_launcher->widget);
  widget_schedule_redraw(launcher->panel->clock->widget);
  window_schedule_resize(global_desktop->panel->window, global_desktop_width, 50);
}


static void panel_volume_label_redraw_handler(struct widget *widget, void *data) {




	cairo_t *cr;
	struct rectangle allocation;
	cairo_text_extents_t extents;
	char string[7];
  struct panel_label *label = data;

  if(label->panel->clock_state != VOLUME_SHOWN)
    return;

	sprintf(string, "%d", (int)global_desktop->current_volume_percentage);

  label->panel->painted = 0;
	cr = widget_cairo_create(label->panel->widget);
	cairo_set_font_size(cr, 20);

  cairo_select_font_face (cr, "Droid Sans",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);

	cairo_text_extents(cr, string, &extents);

  allocation.x = label->panel->clock_allocation.x;
  allocation.y = 31;

  if(label->force_x)
    allocation.x = label->force_x;

	printf("Volume label drawn annotation x y %d \n", allocation.x, allocation.y);

	cairo_move_to(cr, allocation.x, 31);
	cairo_set_source_rgba(cr, 1, 1, 1, 1);
	cairo_show_text(cr, string);
	cairo_destroy(cr);

}

static struct panel_label *panel_add_volume_label(struct panel *panel)
{
	struct panel_label *label;

	label = xzalloc(sizeof *label);
	label->force_x = 0;

	label->panel = panel;
	panel->volume_label = label;


	label->widget = widget_add_widget(panel->widget, label);
	widget_set_redraw_handler(label->widget, panel_volume_label_redraw_handler);

  return label;
}

static void launch_volume(struct panel_launcher *launcher) {

  struct rectangle allocation;
  struct rectangle widget_allocation;
	cairo_t *cr;
  int label_padding = 45;

	widget_get_allocation(launcher->panel->reboot_launcher->widget, &widget_allocation);

  if(launcher->panel->clock_state == SYSTEM_SHOWN) {
    launch_system(launcher);
	}

  if(launcher->panel->clock_state == CLOCK_SHOWN) {
    if(global_desktop->current_volume_percentage == 100) {
      label_padding = 41;
    }
    printf("Volume Clock shown \n");
    launcher->panel->clock->force_x = WAYWARD_HIDE_X;
    launcher->panel->volumedown_launcher->force_x = launcher->panel->clock_allocation.x;
    launcher->panel->volumeup_launcher->force_x = launcher->panel->clock_allocation.x + 84;
    launcher->panel->volumedown_launcher->initial_x = launcher->panel->clock_allocation.x;
    launcher->panel->volumeup_launcher->initial_x = launcher->panel->clock_allocation.x + 84;
    //volume label
    launcher->panel->volume_label->force_x = launcher->panel->clock_allocation.x + label_padding;

    launcher->panel->clock_state = VOLUME_SHOWN;

	} else {
    launcher->panel->clock->force_x = 0;
    launcher->panel->volumedown_launcher->force_x = WAYWARD_HIDE_X;
    launcher->panel->volumeup_launcher->force_x = WAYWARD_HIDE_X;
    launcher->panel->volume_label->force_x = WAYWARD_HIDE_X;
    launcher->panel->clock_state = CLOCK_SHOWN;
  }

  widget_set_allocation(launcher->panel->clock->widget,
    launcher->panel->clock->force_x,
    launcher->panel->clock_allocation.y,
		launcher->panel->clock_allocation.width,
    launcher->panel->clock_allocation.height
  );


  widget_set_allocation(launcher->panel->volumedown_launcher->widget,
    launcher->panel->volumedown_launcher->force_x,
    widget_allocation.y,
		widget_allocation.width,
    widget_allocation.height
  );

  widget_set_allocation(launcher->panel->volumeup_launcher->widget,
    launcher->panel->volumeup_launcher->force_x,
    widget_allocation.y,
		widget_allocation.width,
    widget_allocation.height
  );

  widget_set_allocation(launcher->panel->volume_label->widget,
    launcher->panel->volume_label->force_x,
    widget_allocation.y,
		40,
    50
  );


  widget_schedule_redraw(launcher->panel->volumedown_launcher->widget);
  widget_schedule_redraw(launcher->panel->volumeup_launcher->widget);
  widget_schedule_redraw(launcher->panel->volume_label->widget);

  widget_schedule_redraw(launcher->panel->clock->widget);
  window_schedule_resize(global_desktop->panel->window, global_desktop_width, 50);
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
  char *argv[] = {"/usr/bin/wayward-terminal", NULL};

  int status = posix_spawn(&pid, "/usr/bin/wayward-terminal", NULL, NULL, argv, environ);
  return;


}

void clock_volume_mute () {
  volume_set(0);
  global_desktop->current_volume_percentage = 0;
}

void clock_volume_up () {

  printf("Volume pre-up is %d \n", global_desktop->current_volume);

  volume_set( alsa_volume_to_percentage((double)(global_desktop->current_volume + WAYWARD_AUDIO_STEP)));

  if (global_desktop->mixer != NULL)
  {
    snd_mixer_handle_events (global_desktop->mixer_handle);
    snd_mixer_selem_get_playback_volume(global_desktop->mixer,
    0, &global_desktop->current_volume);

    global_desktop->current_volume_percentage = alsa_volume_to_percentage(global_desktop->current_volume);

    printf("Volume post-up is %d \n", global_desktop->current_volume);
  }
}

void clock_volume_down () {



  printf("Volume pre-down is %d \n", global_desktop->current_volume);


  volume_set( alsa_volume_to_percentage((double)(global_desktop->current_volume - WAYWARD_AUDIO_STEP)) );

  if (global_desktop->mixer != NULL)
  {
    snd_mixer_handle_events (global_desktop->mixer_handle);
    snd_mixer_selem_get_playback_volume (global_desktop->mixer,
    0, &global_desktop->current_volume);

    global_desktop->current_volume_percentage = alsa_volume_to_percentage(global_desktop->current_volume);
    printf("Volume post-down is %d \n", global_desktop->current_volume);
  }

  if(global_desktop->current_volume_percentage < 7 )
    volume_set(0);



}

static void clock_shutdown ()
{
  printf("Shutdown \n");
  execl ("/usr/bin/sudo", "/usr/bin/sudo", "/usr/bin/systemctl", "poweroff", (char *)0);
}

static void
clock_restart ()
{
  printf("Restart  \n");
  execl ("/usr/bin/sudo", "/usr/bin/sudo", "/usr/bin/systemctl", "reboot", (char *)0);
}

static void brightnessctl (int dir) {
  printf("Brightness up %d  \n", dir);
  pid_t pid;
  pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		return;
	}

	if (pid)
		return;


	if (setsid() == -1)
		exit(EXIT_FAILURE);



//	char *argv[] = {"/usr/bin/brightnessctl", "s", "5+", " > /tmp/d1", NULL};
//    posix_spawn(&pid, "/usr/bin/brightnessctl", NULL, NULL, argv, environ);

//  return;

  if(dir > 0) {
    char *newargv[] = { "/usr/bin/brightnessctl", "s", "5%+", "", NULL };
    execve(newargv[0], newargv, environ);
  } else {
    char *newargv[] = { "/usr/bin/brightnessctl", "s", "5%-", "", NULL };
    execve(newargv[0], newargv, environ);
  }
}



static void check_shm_commands(struct toytimer *tt) {
  char name[70];


  static int shm_init = 0;
  static void *ptr = NULL;
  static int shm_fd = 0;
  uid_t uid = 0;
  if(!shm_init) {
    uid = geteuid();
    printf("/wayward-shared_mem%d \n", uid);
    sprintf(name, "/wayward-shared_mem%d", uid);

    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int) );
    ptr = mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);\
    shm_init = 1;

    if(ptr < 0)
    {
      printf("SHM Mapping failed. Keyboard shortcuts disabled \n");
      return;
    }

  }


  int test = 0;
  int null = 0;
  memcpy(&test, (int *)ptr, sizeof(int));
  if(test != SHM_START) {
    memcpy((int*)ptr, &null, sizeof(int));
  } else {
    return;
  }
  //printf("SHM Command is %d \n", test);
  if(test == SHM_MUTE) {
    clock_volume_mute();
  } else if(test == SHM_VOLUMEUP) {
    clock_volume_up();
  } else if(test == SHM_VOLUMEDOWN) {
    clock_volume_down();
  } else if(test == SHM_BRIGHTNESS_UP) {
    brightnessctl(1);
  } else if(test == SHM_BRIGHTNESS_DOWN) {
    brightnessctl(0);
  } else if(test == SHM_SHUTDOWN) {
    clock_shutdown();
  } else if(test == SHM_RESTART) {
    clock_restart();
  } else if(test == SHM_LAUNCH_TERMINAL) {
    launch_terminal();
  } else if(test == SHM_LAUNCH_BROWSER) {
    launch_browser();
  }

  printf("SHM on \n");

}



//TODO add launchers with icons
//TODO add svg
static int wayward_add_launchers(struct panel *panel, struct desktop *desktop)
{

  int found = 0;
  char *icon = NULL;
  char *path = NULL;
  char *line = NULL;
  char *hide_apps = NULL;
  char *hide_apps_token, *hide_apps_str, *hide_apps_tofree;

  size_t len = 0;
  ssize_t read;

  struct weston_config_section *s;
  s = weston_config_get_section(desktop->config, "shell", NULL, NULL);
  weston_config_section_get_string(s, "hide-apps", &hide_apps, NULL);

  printf("Hide apps %s \n", hide_apps);

  //Run script to get cached entries
  //TODO use C instead
  if(system ("/usr/bin/bash /usr/lib/weston/wayward-lsdesktopf" )) {
    printf("error occured %s", strerror(errno));

    return found;
  }


  char path_buf[1256];
  int skip_next = 0;

  snprintf(path_buf, sizeof path_buf, "%s/.cache/wayward-menus", getenv("HOME"));



  printf("Adding applications \n");


  FILE* file = fopen(path_buf, "r");
  if(!file) {
    return found;
  }


  while ((read = getline(&line, &len, file)) != -1) {



    if(line) {

      if(skip_next){
        skip_next = 0;
        continue;
      }

      line[strcspn(line, "\n")] = 0;


      if(!path) {

/*
        if(strstr(hide_apps, trimwhitespace(line) ) != NULL) {
          skip_next = 1;
          continue;
        }
        if(strstr(hide_apps, basename(trimwhitespace(line)) ) != NULL) {
          skip_next = 1;
          continue;
        }
*/

        //hide_apps_token, *hide_apps_str, *hide_apps_tofree;
        if(hide_apps) {
          hide_apps_tofree = hide_apps_str = strdup(hide_apps);  // We own str's memory now.
          while ((hide_apps_token = strsep(&hide_apps_str, ","))) {

            if(strstr(line, trimwhitespace(hide_apps_token) ) != NULL) {
              printf("Hide app %s line %s \n", hide_apps_token, line);
              skip_next = 1;
              break;
            }
          };
          free(hide_apps_tofree);
          if(skip_next)
            continue;

        }

        path = (char *) malloc(len);
        strncpy(path, line, len);

      } else if(path && !icon) {
        icon = (char *) malloc(len);
        strncpy(icon, line, len);

      }

      if(path && icon) {
        //printf("path and icon %s %s %s \n", path, icon, basename(path) );
        panel_add_launcher(panel, icon, path, 0, NULL);

        free(icon);
        free(path);
        icon = NULL;
        path = NULL;
        found = 1;
      }


    }



  }


  free(hide_apps);
  free(line);
  line = NULL;
  hide_apps = NULL;
  fclose(file);

  printf("File closed \n");
  return found;
}


static void
panel_add_launchers(struct panel *panel, struct desktop *desktop)
{
	struct weston_config_section *s;
	char *icon, *path;
	const char *name;
	int count;
	int shared_launchers = 0;
  struct rectangle clock_allocation;
  struct rectangle launcher_allocation;






	count = 0;
  s = NULL;
	while (weston_config_next_section(desktop->config, &s, &name)) {
		if (strcmp(name, "launcher") != 0)
			continue;

		weston_config_section_get_string(s, "icon", &icon, NULL);
		weston_config_section_get_string(s, "path", &path, NULL);

		if (icon != NULL && path != NULL) {
			panel_add_launcher(panel, icon, path, 0, NULL);
			count++;
		} else {
			fprintf(stderr, "invalid launcher section\n");
		}

		free(icon);
		free(path);
	}

  //Add launchers from /usr/share/applications
  shared_launchers = wayward_add_launchers(panel, desktop);


	if (shared_launchers == 0 && count == 0) {
		/* add default launcher */
		panel_add_launcher(panel,
				   "/usr/share/wayward/utilities-terminal-symbolic.svg",
				   BINDIR "/wayward-terminal",
           0,
           NULL
    );

	}

  widget_get_allocation(panel->clock->widget, &clock_allocation);
  printf("Clock allocation is %d %d \n", clock_allocation.x, clock_allocation.y);

  //Action launchers
  panel_add_launcher(panel,
		"/usr/share/wayward/tv-symbolic.svg",
    BINDIR "/weston-terminal",
    0,
    toggle_inhibit
  );

  panel_add_launcher(panel,
		"/usr/share/wayward/open-menu-symbolic.svg",
    BINDIR "/weston-terminal",
    0,
    launch_exposay
  );

  panel_add_launcher(panel,
		"/usr/share/wayward/emblem-system-symbolic.svg",
    BINDIR "/weston-terminal",
    0,
    launch_system
  );


  if(global_desktop->mixer_handle != NULL) {
    panel_add_launcher(panel,
     "/usr/share/wayward/multimedia-volume-control-symbolic.svg",
      BINDIR "/weston-terminal",
      0,
      launch_volume
    );
  }

  //Add Restart button for system section

  panel->reboot_launcher = panel_add_launcher(panel,
		"/usr/share/wayward/system-reboot-symbolic.svg",
    BINDIR "/weston-terminal",
    WAYWARD_HIDE_X,
    clock_restart
  );


  //Add Shutdown button
  panel->shutdown_launcher = panel_add_launcher(panel,
		"/usr/share/wayward/system-shutdown-symbolic.svg",
    BINDIR "/weston-terminal",
    WAYWARD_HIDE_X,
    clock_shutdown
  );


  panel->reboot_launcher->force_x = WAYWARD_HIDE_X;
  panel->shutdown_launcher->force_x = WAYWARD_HIDE_X;
  widget_schedule_redraw(panel->reboot_launcher->widget);
  widget_schedule_redraw(panel->shutdown_launcher->widget);

  if(!global_desktop->mixer_handle)
    return;

  //Add plus/minus button
  panel->volumedown_launcher = panel_add_launcher(panel,
		"/usr/share/wayward/list-remove-symbolic.svg",
    BINDIR "/weston-terminal",
    WAYWARD_HIDE_X,
    clock_volume_down
  );
  panel->volumeup_launcher = panel_add_launcher(panel,
		"/usr/share/wayward/list-add-symbolic.svg",
    BINDIR "/weston-terminal",
    WAYWARD_HIDE_X,
    clock_volume_up
  );

  snd_mixer_handle_events (global_desktop->mixer_handle);
  snd_mixer_selem_get_playback_volume(global_desktop->mixer,
  0, &global_desktop->current_volume);
  global_desktop->current_volume_percentage = alsa_volume_to_percentage(global_desktop->current_volume);
  printf("Volume is %d \n", desktop->current_volume);
  panel->volume_label = panel_add_volume_label(panel);

  panel->volumedown_launcher->force_x = WAYWARD_HIDE_X;
  panel->volumeup_launcher->force_x = WAYWARD_HIDE_X;
  panel->volume_label->force_x = WAYWARD_HIDE_X;
  widget_schedule_redraw(panel->volumedown_launcher->widget);
  widget_schedule_redraw(panel->volumeup_launcher->widget);



}

static void
parse_panel_position(struct desktop *desktop, struct weston_config_section *s)
{
	char *position;

	desktop->want_panel = 1;

	weston_config_section_get_string(s, "panel-position", &position, "top");
	if (strcmp(position, "top") == 0) {
		desktop->panel_position = WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP;
	} else if (strcmp(position, "bottom") == 0) {
		desktop->panel_position = WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM;
	} else if (strcmp(position, "left") == 0) {
		desktop->panel_position = WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT;
	} else if (strcmp(position, "right") == 0) {
		desktop->panel_position = WESTON_DESKTOP_SHELL_PANEL_POSITION_RIGHT;
	} else {
		/* 'none' is valid here */
		if (strcmp(position, "none") != 0)
			fprintf(stderr, "Wrong panel position: %s\n", position);
		desktop->want_panel = 0;
	}
	desktop->want_panel = 1;
	free(position);
}

static void
parse_clock_format(struct desktop *desktop, struct weston_config_section *s)
{
	char *clock_format;

	weston_config_section_get_string(s, "clock-format", &clock_format, "");
	if (strcmp(clock_format, "minutes") == 0)
		desktop->clock_format = CLOCK_FORMAT_MINUTES;
	else if (strcmp(clock_format, "seconds") == 0)
		desktop->clock_format = CLOCK_FORMAT_SECONDS;
	else if (strcmp(clock_format, "minutes-24h") == 0)
		desktop->clock_format = CLOCK_FORMAT_MINUTES_24H;
	else if (strcmp(clock_format, "seconds-24h") == 0)
		desktop->clock_format = CLOCK_FORMAT_SECONDS_24H;
	else if (strcmp(clock_format, "none") == 0)
		desktop->clock_format = CLOCK_FORMAT_NONE;
	else
		desktop->clock_format = DEFAULT_CLOCK_FORMAT;
	free(clock_format);
}

int main(int argc, char *argv[])
{
	struct desktop desktop = { 0 };
	struct output *output;
	struct weston_config_section *s;
	const char *config_file;

	desktop.unlock_task.run = unlock_dialog_finish;
	wl_list_init(&desktop.outputs);

	config_file = weston_config_get_name_from_env();
	desktop.config = weston_config_parse(config_file);
	s = weston_config_get_section(desktop.config, "shell", NULL, NULL);
	weston_config_section_get_bool(s, "locking", &desktop.locking, true);

  weston_config_section_get_string(s, "pin-code", &desktop.pincode, NULL);
//  desktop.pincode = "wyuQRH90gc4arvr9naCBMtuluE/eLxGKg/Bt9WNkRNfk+NS2";

	parse_panel_position(&desktop, s);
	parse_clock_format(&desktop, s);

  desktop.display = display_create(&argc, argv);
	if (desktop.display == NULL) {
		fprintf(stderr, "failed to create display: %s\n",
			strerror(errno));
		weston_config_destroy(desktop.config);
		return -1;
	}

  global_desktop = &desktop;

	display_set_user_data(desktop.display, &desktop);
	display_set_global_handler(desktop.display, global_handler);
	display_set_global_handler_remove(desktop.display, global_handler_remove);

	/* Create panel and background for outputs processed before the shell
	 * global interface was processed */
	if (desktop.want_panel)
		weston_desktop_shell_set_panel_position(desktop.shell, desktop.panel_position);

  setup_mixer(&desktop);

  //check if battery exists
  check_battery_exists();

  wl_list_for_each(output, &desktop.outputs, link)
		if (!output->panel)
			output_init(output, &desktop);


  //Setup shm timer
  toytimer_init(&desktop.shm_timer, CLOCK_MONOTONIC,
		      desktop.display, check_shm_commands);

  struct itimerspec its;
	struct timespec ts;
	struct tm *tm;

	clock_gettime(CLOCK_REALTIME, &ts);

  printf("Seconds %d \n",  ts.tv_sec);

	its.it_interval.tv_sec = 0;
  //ms * 1000000
  //runs every 500ms
	its.it_interval.tv_nsec = 500 * 1000000;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 1;
	//timespec_add_nsec(&its.it_value, &its.it_value, -ts.tv_nsec);

	toytimer_arm(&desktop.shm_timer, &its);
  //end Setup shm timer


	grab_surface_create(&desktop);



	signal(SIGCHLD, sigchild_handler);

	display_run(desktop.display);

	/* Cleanup */
	grab_surface_destroy(&desktop);
	desktop_destroy_outputs(&desktop);
	if (desktop.unlock_dialog)
		unlock_dialog_destroy(desktop.unlock_dialog);
	weston_desktop_shell_destroy(desktop.shell);
	display_destroy(desktop.display);
	weston_config_destroy(desktop.config);

	return 0;
}
