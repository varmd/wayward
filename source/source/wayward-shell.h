#ifndef __WAYWARD_H
#define __WAYWARD_H



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

#include "single-pixel-buffer-v1-client-protocol.h"
#include "weston-desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"

#include <alsa/asoundlib.h>

#define DEFAULT_CLOCK_FORMAT CLOCK_FORMAT_MINUTES
#define DEFAULT_SPACING 10

#define WAYWARD_HIDE_X 43371
#define WAYWARD_NO_MOVE_X -13371
#define WAYWARD_INITIAL_HEIGHT 82
#define WAYWARD_PANEL_LEAVE_BUG_Y 47

#define WAYWARD_ICON_SIZE 32
#define WAYWARD_AUDIO_STEP 3


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
	BRIGHTNESS_SHOWN,
	CLOCK_NONE
};

typedef struct {
    char name[256];
    char long_name[384];
    int card;
    int device;
    int init;
} sound_device;

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

  snd_mixer_t *mixer_handle[10];
  snd_mixer_elem_t *mixer[10];


  long current_volume;
  double current_volume_percentage;

	bool enable_autohide;

  int current_brightness;
	bool enable_brightness_ddc;
	int ddc_i2c_number;
	bool enable_brightness_ctl;
	char *brightnessctl_device;

  int panel_height;
  int panel_spacing;
  int icon_size;
  int panel_font_size;
	int right_icons;

	bool virtual_keyboard;
	int virtual_keyboard_width;
	int virtual_keyboard_height;

  struct toytimer shm_timer;

  struct panel *panel;
  struct panel *panels[30];

	struct weston_config *config;
	bool locking;


  char *pincode;
	enum cursor_type grab_cursor;

	int painted;

	struct input_panel *input_panel;
  struct wl_surface *input_panel_surface;
  int input_panel_shown;

  int panel_left_padding;

  char *multiple_sound_cards;
  //char sound_card_names[10][50];
  int sound_cards[10];
  int sound_cards_is_blue[10];
  char *sound_card_devices[10];
  char *sound_mixers[10];
  double card_volume_per[10];
  long card_volumes[10];
  long min_volumes[10];
  long max_volumes[10];
  int  current_sound_card;
  int  sound_cards_count;

  //struct sound_device sound_cards[10];

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
  struct panel_label *sound_card_label;

  struct panel_launcher *brightnessdown_launcher;
  struct panel_launcher *brightnessup_launcher;
  struct panel_label *brightness_label;

	int painted;
	int initial;
	int allocation_set;
	enum weston_desktop_shell_panel_position panel_position;
	enum clock_format clock_format;
  enum clock_state clock_state;

	uint32_t color;
	struct desktop *desktop;

	struct output *output;
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

	int width;
	int height;
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

struct input_panel {
	struct window *window;
	struct widget *widget;
	struct widget *button;
	int button_focused;
	int closing;
	int keys_entered;

	char pincode[7];
	struct desktop *desktop;
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

extern struct wl_surface *main_input_panel(struct desktop *desktop, int width, int height);
extern struct wl_surface *redraw_input_panel(struct desktop *desktop, int width, int height);
extern struct wl_surface *destroy_input_panel();


static double alsa_volume_to_percentage (long value, int card_index);

#endif
