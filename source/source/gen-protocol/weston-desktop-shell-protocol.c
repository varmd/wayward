/* Generated by wayland-scanner 1.20.0 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *weston_desktop_types[] = {
	NULL,
	&wl_output_interface,
	&wl_surface_interface,
	&wl_output_interface,
	&wl_surface_interface,
	&wl_surface_interface,
	&wl_surface_interface,
	NULL,
	&wl_surface_interface,
	NULL,
	NULL,
	&wl_surface_interface,
	&wl_output_interface,
};

static const struct wl_message weston_desktop_shell_requests[] = {
	{ "set_background", "oo", weston_desktop_types + 1 },
	{ "set_panel", "oo", weston_desktop_types + 3 },
	{ "set_lock_surface", "o", weston_desktop_types + 5 },
	{ "unlock", "", weston_desktop_types + 0 },
	{ "set_grab_surface", "o", weston_desktop_types + 6 },
	{ "desktop_ready", "", weston_desktop_types + 0 },
	{ "set_panel_position", "u", weston_desktop_types + 0 },
};

static const struct wl_message weston_desktop_shell_events[] = {
	{ "configure", "uoii", weston_desktop_types + 7 },
	{ "prepare_lock_surface", "", weston_desktop_types + 0 },
	{ "grab_cursor", "u", weston_desktop_types + 0 },
};

WL_EXPORT const struct wl_interface weston_desktop_shell_interface = {
	"weston_desktop_shell", 1,
	7, weston_desktop_shell_requests,
	3, weston_desktop_shell_events,
};

static const struct wl_message weston_screensaver_requests[] = {
	{ "set_surface", "oo", weston_desktop_types + 11 },
};

WL_EXPORT const struct wl_interface weston_screensaver_interface = {
	"weston_screensaver", 1,
	1, weston_screensaver_requests,
	0, NULL,
};

