/*
 * Copyright (C) 2014 Collabora Ltd.
 * Copyright (C) 2017-2021 varmd - https://github.com/varmd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libweston/libweston.h>
#include <libweston/desktop.h>
#include <linux/input.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "weston-desktop-shell-client-protocol.h"
#include "shell-helper-server-protocol.h"


#ifndef container_of
#define container_of(ptr, type, member) ({                              \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif


#define DEFAULT_SCALE 1.3


enum animation_type {
	ANIMATION_NONE,
	ANIMATION_ZOOM,
	ANIMATION_FADE,
	ANIMATION_DIM_LAYER,
};

enum fade_type {
	FADE_IN,
	FADE_OUT
};


enum exposay_target_state {
	EXPOSAY_TARGET_OVERVIEW, /* show all windows */
	EXPOSAY_TARGET_CANCEL, /* return to normal, same focus */
	EXPOSAY_TARGET_SWITCH, /* return to normal, switch focus */
	EXPOSAY_TARGET_CLOSE, /* close view */
};

enum exposay_layout_state {
	EXPOSAY_LAYOUT_INACTIVE = 0, /* normal desktop */
	EXPOSAY_LAYOUT_OVERVIEW, /* show all windows */
	EXPOSAY_LAYOUT_ANIMATE_TO_OVERVIEW, /* in transition to all windows */
};


struct exposay_output {
	int num_surfaces;
	int grid_size;
	int surface_size;
	int padding_inner;
};

struct exposay_surface {
	struct desktop_shell *shell;
	struct exposay_output *eoutput;
	struct weston_surface *surface;
	struct weston_view *view;
	struct wl_listener view_destroy_listener;
	struct wl_list link;

  struct weston_buffer_reference *mask_buffer_ref;

  struct weston_surface *mask_surface;
  struct weston_view *mask_view;

	int x;
	int y;
	int width;
	int height;
	double scale;

	int row;
	int column;
  int highlight;

	/* The animations only apply a transformation for their own lifetime,
	 * and don't have an option to indefinitely maintain the
	 * transformation in a steady state - so, we apply our own once the
	 * animation has finished. */
	struct weston_transform transform;
};


struct exposay {
	struct desktop_shell  *shell;
  struct shell_output   *shell_output;
	struct exposay_surface *focus_prev;
	struct exposay_surface *focus_current;
	struct weston_view *clicked;
	struct workspace *workspace;
	struct weston_seat *seat;



	struct wl_list surface_list;


	struct weston_keyboard_grab grab_kbd;
	struct weston_pointer_grab grab_ptr;

	enum exposay_target_state state_target;
	enum exposay_layout_state state_cur;
	int in_flight; /* number of animations still running */

	int row_current;
	int column_current;
	struct exposay_output *cur_output;

	bool mod_pressed;
	bool mod_invalid;
};

struct focus_surface {
	struct weston_surface *surface;
	struct weston_view *view;
	struct weston_transform workspace_transform;
};

struct workspace {
	struct weston_layer layer;

	struct wl_list focus_list;
	struct wl_listener seat_destroyed_listener;

	struct focus_surface *fsurf_front;
	struct focus_surface *fsurf_back;
	struct weston_view_animation *focus_animation;
};






static void exposay_set_state(struct desktop_shell *shell,
                              enum exposay_target_state state,
			      struct weston_seat *seat);
static void exposay_check_state(struct desktop_shell *shell, struct weston_seat *seat);


struct shell_output {
	struct desktop_shell  *shell;
	struct weston_output  *output;
	struct wl_listener    destroy_listener;
	struct wl_list        link;

	struct weston_surface *panel_surface;
	struct wl_listener panel_surface_listener;

	struct weston_surface *background_surface;
	struct wl_listener background_surface_listener;

	struct {
		struct weston_view *view;
		struct weston_view_animation *animation;
		enum fade_type type;
		struct wl_event_source *startup_timer;
	} fade;
};


struct weston_desktop;

struct desktop_shell {
	struct weston_compositor *compositor;
	struct weston_desktop *desktop;
	const struct weston_xwayland_surface_api *xwayland_surface_api;

	struct wl_listener idle_listener;
	struct wl_listener wake_listener;
	struct wl_listener transform_listener;
	struct wl_listener resized_listener;
	struct wl_listener destroy_listener;
	struct wl_listener show_input_panel_listener;
	struct wl_listener hide_input_panel_listener;
	struct wl_listener update_input_panel_listener;

	struct weston_layer fullscreen_layer;
	struct weston_layer panel_layer;
	struct weston_layer background_layer;
	struct weston_layer lock_layer;
	struct weston_layer input_panel_layer;

	struct wl_listener pointer_focus_listener;
	struct weston_surface *grab_surface;

	struct {
		struct wl_client *client;
		struct wl_resource *desktop_shell;
		struct wl_listener client_destroy_listener;

		unsigned deathcount;
		struct timespec deathstamp;
	} child;

	bool locked;
	bool showing_input_panels;
	bool prepare_event_sent;

	struct text_backend *text_backend;

	struct {
		struct weston_surface *surface;
		pixman_box32_t cursor_rectangle;
	} text_input;

	struct weston_surface *lock_surface;
	struct wl_listener lock_surface_listener;

	struct workspace workspace;

	struct {
		struct wl_resource *binding;
		struct wl_list surfaces;
	} input_panel;

	bool allow_zap;
	uint32_t binding_modifier;
	enum animation_type win_animation_type;
	enum animation_type win_close_animation_type;
	enum animation_type startup_animation_type;
	enum animation_type focus_animation_type;

	struct weston_layer minimized_layer;

	struct wl_listener seat_create_listener;
	struct wl_listener output_create_listener;
	struct wl_listener output_move_listener;
	struct wl_list output_list;
	struct wl_list seat_list;

	enum weston_desktop_shell_panel_position panel_position;

	char *client;

	struct timespec startup_time;
};

//struct weston_output *
//get_default_output(struct weston_compositor *compositor);

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

static void setup_shm(int shm_command);

struct weston_output *
get_default_output(struct weston_compositor *compositor)
{
	if (wl_list_empty(&compositor->output_list))
		return NULL;

	return container_of(compositor->output_list.next,
			    struct weston_output, link);
}

struct weston_view *
get_default_view(struct weston_surface *surface);

//struct shell_surface *
//get_shell_surface(struct weston_surface *surface);

struct workspace *
get_current_workspace(struct desktop_shell *shell);

void
lower_fullscreen_layer(struct desktop_shell *shell,
		       struct weston_output *lowering_output);


typedef void (*shell_for_each_layer_func_t)(struct desktop_shell *,
					    struct weston_layer *, void *);

void
shell_for_each_layer(struct desktop_shell *shell,
		     shell_for_each_layer_func_t func,
void *data);


struct wl_seat *panel_seat;
struct weston_seat *global_weston_seat;
struct weston_keyboard *panel_keyboard;


struct desktop_shell *global_desktop_shell = NULL;
struct wl_array global_minimized_array;

struct shell_helper {
	struct weston_compositor *compositor;
  struct desktop_shell *shell;
	struct wl_listener destroy_listener;
  struct exposay_output eoutput;


	struct weston_layer *panel_layer;

	struct weston_layer curtain_layer;
	struct weston_view *curtain_view;
	struct weston_view_animation *curtain_animation;
	uint32_t curtain_show;

	struct wl_list slide_list;
};



//end copied



static void
shell_helper_move_surface(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *surface_resource,
			  int32_t x,
			  int32_t y)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view;

	view = container_of(surface->views.next, struct weston_view, surface_link);

	if (!view)
		return;

  if(x == -13371)
    x = view->geometry.pos_offset.x;
	weston_view_set_position(view, x, y);
	weston_view_update_transform(view);
}

static void
configure_surface(struct weston_surface *es, struct weston_coord_surface new_origin)
{
	struct weston_view *existing_view = es->committed_private;
	struct weston_view *new_view;

	new_view = container_of(es->views.next, struct weston_view, surface_link);

	if (wl_list_empty(&new_view->layer_link.link)) {
		/* be sure to append to the list, not insert */
		weston_layer_entry_insert(&existing_view->layer_link, &new_view->layer_link);
		weston_compositor_schedule_repaint(es->compositor);
	}
}

static struct shell_output *
find_shell_output_from_weston_output(struct desktop_shell *shell,
				     struct weston_output *output)
{
	struct shell_output *shell_output;

	wl_list_for_each(shell_output, &shell->output_list, link) {
		if (shell_output->output == output)
			return shell_output;
	}

	return NULL;
}

static void
shell_helper_add_surface_to_layer(struct wl_client *client,
				  struct wl_resource *resource,
				  struct wl_resource *new_surface_resource,
				  struct wl_resource *existing_surface_resource)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *new_surface =
		wl_resource_get_user_data(new_surface_resource);
  struct weston_surface *existing_surface = wl_resource_get_user_data(existing_surface_resource);
  struct weston_view *new_view, *existing_view, *next;
	struct wl_layer *layer;
  struct shell_output *sh_output;



	if (new_surface->committed) {
		wl_resource_post_error(new_surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface role already assigned");
		return;
	}

	existing_view = container_of(existing_surface->views.next,
				     struct weston_view,
				     surface_link);

  if(!existing_view) {
    printf("No existing surface to add to \n");
    exit(1);
  }
  if(!new_surface) {
    printf("No new surface to move \n");
    exit(1);
  }

	wl_list_for_each_safe(new_view, next, &new_surface->views, surface_link)
		weston_view_destroy(new_view);
	new_view = weston_view_create(new_surface);

	new_surface->committed = configure_surface;
	new_surface->committed_private = existing_view;
	new_surface->output = existing_view->output;
}

//TODO
//Is this needed?
static void
configure_panel(struct weston_surface *es, struct weston_coord_surface new_origin)
{
	struct shell_helper *helper = es->committed_private;
	struct weston_view *view;

	view = container_of(es->views.next, struct weston_view, surface_link);

	if (wl_list_empty(&view->layer_link.link)) {
		weston_layer_entry_insert(&helper->panel_layer->view_list, &view->layer_link);
		weston_compositor_schedule_repaint(es->compositor);
	}
}

static void
shell_helper_set_panel(struct wl_client *client,
		       struct wl_resource *resource,
		       struct wl_resource *surface_resource)
{


	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view = container_of(surface->views.next,
						struct weston_view,
						surface_link);
  struct weston_coord_surface tmp;

	/* we need to save the panel's layer so we can use it later on, but
	 * it hasn't yet been defined because the original surface configure
	 * function hasn't yet been called. if we call it here we will have
	 * access to the layer. */
	tmp = weston_coord_surface(0,
				      0,
				      surface);
	surface->committed(surface, tmp);

	helper->panel_layer = container_of(view->layer_link.link.next,
					   struct weston_layer,
					   view_list.link);

	/* set new configure functions that only ensure the surface is in the
	 * correct layer. */
	surface->committed = configure_panel;
	surface->committed_private = helper;
}

enum SlideState {
	SLIDE_STATE_NONE,
	SLIDE_STATE_SLIDING_OUT,
	SLIDE_STATE_OUT,
	SLIDE_STATE_SLIDING_BACK,
	SLIDE_STATE_BACK
};

enum SlideRequest {
	SLIDE_REQUEST_NONE,
	SLIDE_REQUEST_OUT,
	SLIDE_REQUEST_BACK
};

struct slide {
	struct weston_surface *surface;
	struct weston_view *view;
	int x;
	int y;

	enum SlideState state;
	enum SlideRequest request;

	struct weston_transform transform;

	struct wl_list link;
};

static void slide_back(struct slide *slide);

static void
slide_done_cb(struct weston_view_animation *animation, void *data)
{
	struct slide *slide = data;

	slide->state = SLIDE_STATE_OUT;

	wl_list_insert(&slide->view->transform.position.link,
		       &slide->transform.link);
	weston_matrix_init(&slide->transform.matrix);
	weston_matrix_translate(&slide->transform.matrix,
				slide->x,
				slide->y,
				0);

	weston_view_geometry_dirty(slide->view);
	weston_compositor_schedule_repaint(slide->surface->compositor);

	if (slide->request == SLIDE_REQUEST_BACK) {
		slide->request = SLIDE_REQUEST_NONE;
		slide_back(slide);
	}
}

static void
slide_out(struct slide *slide)
{
	assert(slide->state == SLIDE_STATE_NONE || slide->state == SLIDE_STATE_BACK);

	slide->state = SLIDE_STATE_SLIDING_OUT;

	weston_move_scale_run(slide->view, slide->x, slide->y,
			      1.0, 1.0, 0,
			      slide_done_cb, slide);
}

static void
slide_back_done_cb(struct weston_view_animation *animation, void *data)
{
	struct slide *slide = data;

	slide->state = SLIDE_STATE_BACK;

	wl_list_remove(&slide->transform.link);
	weston_view_geometry_dirty(slide->view);

	if (slide->request == SLIDE_REQUEST_OUT) {
		slide->request = SLIDE_REQUEST_NONE;
		slide_out(slide);
	} else {
		wl_list_remove(&slide->link);
		free(slide);
	}
}

static void
slide_back(struct slide *slide)
{
	assert(slide->state == SLIDE_STATE_OUT);

	slide->state = SLIDE_STATE_SLIDING_BACK;

	weston_move_scale_run(slide->view, -slide->x, -slide->y,
			      1.0, 1.0, 0,
			      slide_back_done_cb, slide);
}


struct crtc_data {
  uint16_t *r, *g, *b; // gammas
  //int fd;
  //uint32_t crtc_id;
  //drmModeCrtc *crtc;
};

/*
void iterate_fds(struct weston_surface *surface)
{
  int ret;
  int fds[DRM_MAX_MINOR];
  drmModeRes *fd_res[DRM_MAX_MINOR];
  int fd_ct = 0;

  weston_log("DRM_MAX_MINOR %15d\n", DRM_MAX_MINOR); // 16
  for (int fd_idx = 0; fd_idx < DRM_MAX_MINOR; fd_idx++) {
    fds[fd_idx] = 0;
    char * filename = NULL;
    ret = asprintf(&filename, "/dev/dri/card%d", fd_idx);
    assert(ret != -1);
    assert(filename);

    int fd = open(filename, "rw");
    free(filename);

    if (fd < 0) continue;
    fd_res[fd_ct] = drmModeGetResources(fd);
    if (!fd_res[fd_ct]) {
      close(fd);
      continue;
    }

    fds[fd_ct] = fd;
    fd_ct ++;

    //iterate_crtcs(fd, &surface);
    close(fd);
  }
}
*/


void load_gamma(struct weston_surface *surface)
{
  int load_fd;
  //drmModeResPtr res = drmModeGetResources(fd);
  //weston_log("crtc resource count %15d\n", res->count_crtcs);

  // allocate/init crtc_data
  struct crtc_data *dat = (struct crtc_data *) malloc(sizeof(struct crtc_data));
  //dat->fd = fd;

  // open files as necessary


	load_fd = -2;
	load_fd = open("/usr/lib/weston/warm.dat", O_RDONLY);
	if (load_fd < 0) {
    weston_log("22222222    %d", load_fd);
	  return;
	}


  weston_log("gamma size is 15%d  \n", surface->output->gamma_size);



  dat->r = calloc(surface->output->gamma_size, sizeof(uint16_t));
  dat->g = calloc(surface->output->gamma_size, sizeof(uint16_t));
  dat->b = calloc(surface->output->gamma_size, sizeof(uint16_t));

  size_t gamma_size = surface->output->gamma_size;
  read(load_fd, dat->r, surface->output->gamma_size*sizeof(uint16_t));
  read(load_fd, dat->g, surface->output->gamma_size*sizeof(uint16_t));
  read(load_fd, dat->b, surface->output->gamma_size*sizeof(uint16_t));
  weston_log("read one table\n");
  for (int gidx = 0; gidx < surface->output->gamma_size; gidx++) {
    //weston_log("\t\tgamma(%3d) %5d %5d %5d\n", gidx, dat->r[gidx], dat->g[gidx], dat->b[gidx]);
  }
  //SET_GAMMA(dat);
  //surface->output->set_gamma(surface->output, surface->output->gamma_size, dat->r, dat->g, dat->b);


  // Iterate through crtcs for this fd/
  /*
  for (int crtc_idx = 0; crtc_idx < res->count_crtcs; crtc_idx++) {
    dat->crtc_id = res->crtcs[crtc_idx];
    dat->crtc = drmModeGetCrtc(fd, dat->crtc_id);
    dat->r = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->g = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->b = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    weston_log("\tcrtc id %15d \t size %3d  \n", dat->crtc_id, dat->crtc->gamma_size);

    // Process gamma as desired


    weston_log("crtc gotten\n");
    free(dat->r); free(dat->g); free(dat->b);
  }
  */
  // close files as necessary
  close(load_fd);
  // cleanup
  free(dat);
}


static void
shell_helper_keyboard_focus_surface(struct wl_client *client,
			   struct wl_resource *resource,
                           struct wl_resource *surface_resource)
{

//  struct weston_keyboard *keyboard = weston_seat_get_keyboard(panel_seat);
  if(!panel_keyboard) {
    return;
  }
  if(!panel_keyboard || wl_resource_get_user_data(surface_resource)) {
    printf("Error focus \n");
  } else {
    weston_keyboard_set_focus(panel_keyboard, wl_resource_get_user_data(surface_resource));
  }
}


void
mute_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_MUTE);
}

void volumeup_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_VOLUMEUP);
}

void volumedown_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_VOLUMEDOWN);
}


void shutdown_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_SHUTDOWN);
}

void restart_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_RESTART);
}

void terminal_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_LAUNCH_TERMINAL);
}

void browser_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data) {
  setup_shm(SHM_LAUNCH_BROWSER);
}



void fullscreen_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data)
{
  struct weston_surface *surface = keyboard->focus;
  struct desktop_shell *shell = data;

  struct weston_transform transform;
  struct weston_view *view;
  struct exposay_surface *esurface;



	if (surface == NULL)
		return;

  //view = container_of(surface->views.next, struct weston_view, surface_link);
  view = get_default_view(surface);

	if (!view)
		return;

  struct workspace *workspace;
	workspace = get_current_workspace(shell);


  //wl_list_for_each(view, &workspace->layer.view_list.link, layer_link.link) {


    printf("output width %d \n", view->output->width);
		printf("surface width %d \n", view->surface->width);
		printf("output height %d \n", view->output->height);
    printf("surface height %d \n", view->surface->height);

    printf(" /width %f \n", (float)((float)view->output->width/(float)view->surface->width));
		printf(" /height %f \n", (float)((float)view->output->height / (float)view->surface->height));

		//if (!get_shell_surface(view->surface))
		//	continue;
		//if (view->output != output)
	  //		continue;

		esurface = malloc(sizeof(*esurface));



		esurface->shell = shell;

		esurface->view = view;



    wl_list_insert(&view->geometry.transformation_list, &esurface->transform.link);
	weston_matrix_init(&esurface->transform.matrix);

  //weston_matrix_scale(&esurface->transform.matrix, view->output->width / surface->width, view->output->height / surface->height, 1.0f);
  weston_matrix_scale(&esurface->transform.matrix, (float)((float)view->output->width/(float)view->surface->width),
    (float)((float)view->output->height / (float)view->surface->height),
    1.0f);
	//weston_matrix_scale(&esurface->transform.matrix, 2, 2, 1.0f);
	//weston_matrix_scale(&esurface->transform.matrix, 2, 2, 1.0f);

	weston_matrix_translate(&esurface->transform.matrix, 0, 0, 0);


	  weston_view_geometry_dirty(esurface->view);
	  weston_compositor_schedule_repaint(esurface->view->surface->compositor);
    weston_view_set_position(view, 0, 0);

    return;
	//}


  //TODO figure out why below is not working???
  wl_list_for_each(view, &workspace->layer.view_list.link, layer_link.link) {

    wl_list_init(&transform.link);
    weston_matrix_init(&transform.matrix);
    wl_list_insert(&view->geometry.transformation_list, &transform.link);

  //wl_list_remove(&transform.link);

	//weston_matrix_scale(&transform.matrix, surface->output->width / surface->width, 				    surface->output->height / surface->height, 1);
	//weston_matrix_scale(matrix, 0.5, 0.5, 1.0);

  weston_matrix_scale(&transform.matrix, 1.0, 1.0, 1.0);


		//x = output->x + (output->width - width) / 2 - surf_x;
		//y = output->y + (output->height - height) / 2 - surf_y;
  //weston_matrix_translate(&transform.matrix, 10 - view->geometry.x, 10 -view->geometry.y, 0);

    weston_view_update_transform(view);
    weston_view_geometry_dirty(view);
	  weston_compositor_schedule_repaint(view->surface->compositor);
    return;
  }

  //weston_view_set_position(view, 0, 0);


}


void
exposay_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key,
		void *data)
{
	struct desktop_shell *shell = data;

	exposay_set_state(shell, EXPOSAY_TARGET_OVERVIEW, keyboard->seat);
}

static void
debug_binding(struct weston_keyboard *keyboard, const struct timespec *time, uint32_t key, void *data)
{
  //struct weston_keyboard *keyboard = weston_seat_get_keyboard(panel_seat);
  panel_keyboard = keyboard;
  printf("panel_keyboard set \n");
  weston_keyboard_set_focus(keyboard, data);
}


static void
shell_helper_bind_key_panel(struct wl_client *client,
			   struct wl_resource *resource,
                           struct wl_resource *surface_resource,
                           struct wl_resource *seat_resource,
                           struct wl_resource *shell_resource
                           )
{

  //struct weston_keyboard *keyboard = weston_seat_get_keyboard(state->seat);
  panel_seat = wl_resource_get_user_data(seat_resource);
  struct weston_surface *panel_weston_surface = wl_resource_get_user_data(surface_resource);
  struct desktop_shell *shell = wl_resource_get_user_data(shell_resource);

  global_desktop_shell = shell;

  struct shell_helper *helper = wl_resource_get_user_data(resource);
  weston_compositor_add_key_binding(helper->compositor, KEY_E, MODIFIER_SUPER, debug_binding, panel_weston_surface);

  weston_compositor_add_key_binding(helper->compositor, KEY_A, MODIFIER_SUPER, exposay_binding, shell);
  weston_compositor_add_key_binding(helper->compositor, KEY_L, MODIFIER_SUPER, exposay_binding, shell);
  weston_compositor_add_key_binding(helper->compositor, KEY_F12, MODIFIER_SUPER, fullscreen_binding, shell);
  weston_compositor_add_key_binding(helper->compositor, KEY_MUTE, 0, mute_binding, shell);
  weston_compositor_add_key_binding(helper->compositor, KEY_VOLUMEDOWN , 0, volumedown_binding, shell);
  weston_compositor_add_key_binding(helper->compositor, KEY_VOLUMEUP , 0, volumeup_binding, shell);



  weston_compositor_add_key_binding(helper->compositor, KEY_S , MODIFIER_SUPER | MODIFIER_CTRL | MODIFIER_ALT, shutdown_binding, shell);

  weston_compositor_add_key_binding(helper->compositor, KEY_R , MODIFIER_SUPER | MODIFIER_CTRL | MODIFIER_ALT, restart_binding, shell);

  weston_compositor_add_key_binding(helper->compositor, KEY_T , MODIFIER_SUPER | MODIFIER_SHIFT, terminal_binding, shell);

  weston_compositor_add_key_binding(helper->compositor, KEY_HOMEPAGE, 0, browser_binding, shell);

  printf("Added key bindings \n");

}

static void
shell_helper_change_gamma(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *surface_resource,
                           int32_t reset)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface = wl_resource_get_user_data(surface_resource);

         int i;
	uint16_t *red,*green,*blue;
        uint16_t rgb;


	if (!surface->output->set_gamma) {
    weston_log ("No set gamma \r\n ");
		return;
  }

   if(!reset) {
      weston_log ("Yes set gamma \r\n");
      weston_log("gamma size is %d  \n", surface->output->gamma_size);


    int load_fd;
  //drmModeResPtr res = drmModeGetResources(fd);
  //weston_log("crtc resource count %15d\n", res->count_crtcs);

  // allocate/init crtc_data
  struct crtc_data *dat = (struct crtc_data *) malloc(sizeof(struct crtc_data));
  //dat->fd = fd;

  // open files as necessary

  weston_log("ewfewf efewf\n");

	load_fd = -2;
	load_fd = open("/usr/lib/weston/warm.dat", O_RDONLY);
	if (load_fd < 0) {
    weston_log("22222222    %d", load_fd);
	  return;
	}


  weston_log("gamma size is 15%d  \n", surface->output->gamma_size);



  dat->r = calloc(surface->output->gamma_size, sizeof(uint16_t));
  dat->g = calloc(surface->output->gamma_size, sizeof(uint16_t));
  dat->b = calloc(surface->output->gamma_size, sizeof(uint16_t));

  size_t gamma_size = surface->output->gamma_size;
  read(load_fd, dat->r, surface->output->gamma_size*sizeof(uint16_t));
  read(load_fd, dat->g, surface->output->gamma_size*sizeof(uint16_t));
  read(load_fd, dat->b, surface->output->gamma_size*sizeof(uint16_t));
  weston_log("read one table\n");
  for (int gidx = 0; gidx < surface->output->gamma_size; gidx++) {
    weston_log("\t\tgamma(%3d) %5d %5d %5d\n", gidx, dat->r[gidx], dat->g[gidx], dat->b[gidx]);
  }
  //SET_GAMMA(dat);
  surface->output->set_gamma(surface->output, surface->output->gamma_size, dat->r, dat->g, dat->b);


  // Iterate through crtcs for this fd/
  /*
  for (int crtc_idx = 0; crtc_idx < res->count_crtcs; crtc_idx++) {
    dat->crtc_id = res->crtcs[crtc_idx];
    dat->crtc = drmModeGetCrtc(fd, dat->crtc_id);
    dat->r = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->g = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    dat->b = calloc(dat->crtc->gamma_size, sizeof(uint16_t));
    weston_log("\tcrtc id %15d \t size %3d  \n", dat->crtc_id, dat->crtc->gamma_size);

    // Process gamma as desired


    weston_log("crtc gotten\n");
    free(dat->r); free(dat->g); free(dat->b);
  }
  */
  // close files as necessary
  close(load_fd);
  // cleanup
  free(dat);
  } else {


		//257   173    96
		//int _red = 257;
		//int _green = 173;
		//int _blue = 96;

		//TODO further research
		red = calloc(surface->output->gamma_size, sizeof(uint16_t));
		//green = calloc(surface->output->gamma_size, sizeof(uint16_t));
		//blue = calloc(surface->output->gamma_size, sizeof(uint16_t));
		for (i = 0; i < surface->output->gamma_size; i++) {
		  red[i] = (uint32_t) 0xffff * (uint32_t) i / (uint32_t) (surface->output->gamma_size - 1);
    }
    surface->output->set_gamma(surface->output, surface->output->gamma_size, red, red, red);
          /*
          red[i] = surface->output->gamma_size * _red;
		  green[i] = surface->output->gamma_size * _green;
		  blue[i] = surface->output->gamma_size * _blue;
          */

		//rgb = 65536 * 246 + 256 * 227 + 183;


		free(red);


  }


}



static void
shell_helper_slide_surface(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *surface_resource,
			   int32_t x,
			   int32_t y)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
        wl_resource_get_user_data(surface_resource);


  //weston_view_set_position(esurface->mask_view, esurface->x, esurface->y + esurface->height - esurface->mask_surface->height);


	struct weston_view *view;
	struct slide *slide;

	wl_list_for_each(slide, &helper->slide_list, link) {
		if (slide->surface == surface) {
			if (slide->state == SLIDE_STATE_SLIDING_BACK)
				slide->request = SLIDE_REQUEST_OUT;
			return;
		}
	}

	view = container_of(surface->views.next, struct weston_view, surface_link);

	if (!view)
		return;

	slide = malloc(sizeof *slide);
	if (!slide)
		return;

	slide->surface = surface;
	slide->view = view;
	slide->x = x;
	slide->y = y;

	slide->state = SLIDE_STATE_NONE;
	slide->request = SLIDE_REQUEST_NONE;

	wl_list_insert(&helper->slide_list,
		       &slide->link);

	slide_out(slide);
}

static void
shell_helper_slide_surface_back(struct wl_client *client,
				struct wl_resource *resource,
				struct wl_resource *surface_resource)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view;
	int found = 0;
	struct slide *slide;

	wl_list_for_each(slide, &helper->slide_list, link) {
		if (slide->surface == surface) {
			found = 1;
			break;
		}
	}

	if (!found || slide->state == SLIDE_STATE_SLIDING_BACK)
		return;

	if (slide->state == SLIDE_STATE_SLIDING_OUT)
		slide->request = SLIDE_REQUEST_BACK;
	else
		slide_back(slide);
}

/* mostly copied from weston's desktop-shell/shell.c */
static struct weston_view *
shell_curtain_create_view(struct shell_helper *helper,
			  struct weston_surface *surface)
{
	struct weston_view *view;

	if (!surface)
		return NULL;

	view = weston_view_create(surface);
	if (!view) {
		return NULL;
	}

	weston_view_set_position(view, 0, 0);
//	weston_surface_set_color(surface, 0.0, 0.0, 0.0, 0.7);
	weston_layer_entry_insert(&helper->curtain_layer.view_list,
				  &view->layer_link);
	pixman_region32_init_rect(&surface->input, 0, 0,
	                          surface->width,
	                          surface->height);

	return view;
}

static void
curtain_done_hide(struct weston_view_animation *animation,
		  void *data);

static void
curtain_fade_done(struct weston_view_animation *animation,
		  void *data)
{
	struct shell_helper *helper = data;

	if (!helper->curtain_show)
		wl_list_remove(&helper->curtain_layer.link);

	helper->curtain_animation = NULL;
}

static void
shell_helper_curtain(struct wl_client *client,
		     struct wl_resource *resource,
		     struct wl_resource *surface_resource,
		     int32_t show)
{

  //TODO what is?
  /*
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);




	helper->curtain_show = show;

	if (show) {
		if (helper->curtain_animation) {
			weston_fade_update(helper->curtain_animation, 0.7);
			return;
		}

		if (!helper->curtain_view) {
			weston_layer_init(&helper->curtain_layer,
					  &helper->panel_layer->link);

			helper->curtain_view = shell_curtain_create_view(helper, surface);

			// we need to assign an output to the view before we can
			// fade it in
			weston_view_geometry_dirty(helper->curtain_view);
			weston_view_update_transform(helper->curtain_view);
		} else {
			wl_list_insert(&helper->panel_layer->link, &helper->curtain_layer.link);
		}

		helper->curtain_animation = weston_fade_run(
			helper->curtain_view,
			0.0, 0.9, 400,
			curtain_fade_done, helper);

	} else {
		if (helper->curtain_animation) {
			weston_fade_update(helper->curtain_animation, 0.0);
			return;
		}

		// should never happen in theory
		if (!helper->curtain_view)
			return;

		helper->curtain_animation = weston_fade_run(
			helper->curtain_view,
			0.7, 0.0, 400,
			curtain_fade_done, helper);
	}
  */
}


static void
shell_helper_launch_exposay(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *shell_resource,
                           struct wl_resource *seat_resource)
{

  //struct weston_keyboard *keyboard = weston_seat_get_keyboard(state->seat);
  struct wl_seat *seat = wl_resource_get_user_data(seat_resource);

  struct shell_helper *helper = wl_resource_get_user_data(resource);
  struct desktop_shell *shell = wl_resource_get_user_data(shell_resource);
  //exposay_set_state(shell, EXPOSAY_TARGET_OVERVIEW, seat);
  exposay_set_state(shell, EXPOSAY_TARGET_OVERVIEW, NULL);

}


static void
shell_helper_toggle_inhibit(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *shell_resource,
                           struct wl_resource *seat_resource)
{

  //struct weston_keyboard *keyboard = weston_seat_get_keyboard(state->seat);
  struct shell_helper *helper = wl_resource_get_user_data(resource);
  if(!helper->compositor->idle_inhibit) {
    helper->compositor->idle_inhibit = 1;
  } else {
    helper->compositor->idle_inhibit = 0;
  }
  //struct wl_seat *seat = wl_resource_get_user_data(seat_resource);

  //struct shell_helper *helper = wl_resource_get_user_data(resource);
  //struct desktop_shell *shell = wl_resource_get_user_data(shell_resource);


}



static void
shell_helper_move_background_surface(
  struct wl_client *client,
  struct wl_resource *resource,
  struct wl_resource *shell_resource,
  struct wl_resource *background_surface_resource
)
{

  struct shell_helper *helper = wl_resource_get_user_data(resource);
  struct desktop_shell *shell = wl_resource_get_user_data(shell_resource);

  struct weston_surface *background_surface = wl_resource_get_user_data(background_surface_resource);
  struct weston_view *new_view, *existing_view, *next;
	struct wl_layer *layer;
  struct shell_output *sh_output;


  //Background case for GTK4
  printf("No existing surface \n");
  #if 0
  sh_output = find_shell_output_from_weston_output(shell, background_surface->output);
  if (!sh_output->background_surface) {
    return;
  }
  #endif

  printf("Adding to background surface \n");
  existing_view = container_of(background_surface->views.next,
				     struct weston_view,
				     surface_link);

  //weston_layer_entry_remove(&existing_view->layer_link);
  weston_layer_entry_insert(&shell->background_layer.view_list, &existing_view->layer_link);
	weston_compositor_schedule_repaint(shell->compositor);
  return;

  existing_view = container_of(sh_output->background_surface->views.next,
				     struct weston_view,
				     surface_link);

  if(!existing_view) {
    printf("No existing surface to add to \n");
    exit(1);
  }

	wl_list_for_each_safe(new_view, next, &background_surface->views, surface_link)
		weston_view_destroy(new_view);
	new_view = weston_view_create(background_surface);

	background_surface->committed = configure_surface;
	background_surface->committed_private = existing_view;
	background_surface->output = existing_view->output;


}


static const struct shell_helper_interface helper_implementation = {
	shell_helper_move_surface,
	shell_helper_add_surface_to_layer,
	shell_helper_set_panel,
	shell_helper_slide_surface,
  shell_helper_change_gamma,
  shell_helper_bind_key_panel,
  shell_helper_keyboard_focus_surface,
	shell_helper_slide_surface_back,
	shell_helper_curtain,
  shell_helper_launch_exposay,
  shell_helper_toggle_inhibit,
  shell_helper_move_background_surface
};

static void
bind_helper(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct shell_helper *helper = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &shell_helper_interface, 1, id);
	if (resource)
		wl_resource_set_implementation(resource, &helper_implementation,
					       helper, NULL);
}

static void
helper_destroy(struct wl_listener *listener, void *data)
{
	struct shell_helper *helper =
		container_of(listener, struct shell_helper, destroy_listener);

	free(helper);
}


static void setup_shm(int shm_command) {
  //shm create

  const char *name = "/wayward-shared_mem";
  static int shm_fd = 0;
  static int shm_init = 0;
  static void *ptr;
  if(shm_init < 1) {
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int) );
    ptr = mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
    shm_init = 1;
  }


  if(ptr < 0)
  {
      perror("Mapping failed");
      exit(0);
  }
  int test = shm_command;
  memcpy((int*)ptr, &test, sizeof(int));

  memcpy(&test, (int *)ptr, sizeof(int));

  printf("SHM is %d \n", test);
}

//TODO adapt to multiple monitors
//Weston 11 removed exposays so need to set global for now
struct exposay_output global_eoutput;
struct exposay global_exposay;


WL_EXPORT int
wet_module_init(struct weston_compositor *ec,
	    int *argc, char *argv[])
{
	struct shell_helper *helper;

	helper = zalloc(sizeof *helper);
	if (helper == NULL)
		return -1;

	helper->compositor = ec;
	helper->panel_layer = NULL;
	helper->curtain_view = NULL;
	helper->curtain_show = 0;


  //exposay
  //global_eoutput = zalloc(sizeof *global_eoutput);
  //global_exposay = zalloc(sizeof *global_exposay);

  struct weston_seat *seat;

  wl_list_for_each(seat, &ec->seat_list, link) {
			struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);

			if (keyboard) {
        global_weston_seat = seat;
				printf("Found keyboard and seat \n");
      }  else {
        printf("Not found keyboard and seat \n");
      }
		}

	wl_list_init(&helper->slide_list);

	helper->destroy_listener.notify = helper_destroy;
	wl_signal_add(&ec->destroy_signal, &helper->destroy_listener);

	if (wl_global_create(ec->wl_display, &shell_helper_interface, 1,
			     helper, bind_helper) == NULL)
		return -1;


  setup_shm(SHM_START);

	return 0;
}



/** Begin exposay */
#ifndef container_of
#define container_of(ptr, type, member) ({				\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#ifdef  __cplusplus
}
#endif


int
is_shell_surface(struct weston_surface *surface)
{
	if (weston_surface_is_desktop_surface(surface)) {
		struct weston_desktop_surface *desktop_surface =
			weston_surface_get_desktop_surface(surface);
    void *user_data = weston_desktop_surface_get_user_data(desktop_surface);
		if(user_data != NULL) {
      return 1;
    } else {
      return 0;
    }
	}
	return 0;
}



struct weston_view *
get_default_view(struct weston_surface *surface)
{
	struct weston_view *view;

	if (!surface || wl_list_empty(&surface->views))
		return NULL;

/*

	shsurf = get_shell_surface(surface);
	if (shsurf)
		return shsurf->view;
*/

	wl_list_for_each(view, &surface->views, surface_link)
		if (weston_view_is_mapped(view))
			return view;

	return container_of(surface->views.next, struct weston_view, surface_link);
}


static void
exposay_surface_destroy(struct exposay_surface *esurface)
{
	if(&esurface->link) {
    wl_list_remove(&esurface->link);
  }
	//wl_list_remove(&esurface->view_destroy_listener.link);

	if (global_exposay.focus_current == esurface)
		global_exposay.focus_current = NULL;
	if (global_exposay.focus_prev == esurface)
		global_exposay.focus_prev = NULL;

	free(esurface);
}

static void
exposay_activate(struct desktop_shell *shell)
{

  struct workspace *ws;
  struct weston_layer_entry *new_layer_link;
  struct weston_seat *seat = global_exposay.seat;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);


  if(!global_exposay.focus_current) {
    return;
  }

  struct exposay_surface *esurface = global_exposay.focus_current;
  weston_view_activate_input(esurface->view, global_exposay.seat,
    WESTON_ACTIVATE_FLAG_CONFIGURE);

  new_layer_link = &global_exposay.workspace->layer.view_list;

	if (new_layer_link == NULL)
		return;


	weston_view_geometry_dirty(esurface->view);
	weston_layer_entry_remove(&esurface->view->layer_link);
	weston_layer_entry_insert(new_layer_link, &esurface->view->layer_link);
	weston_view_geometry_dirty(esurface->view);
  weston_compositor_schedule_repaint(esurface->view->surface->compositor);


}


static void
exposay_scale(struct exposay_surface *esurface)
{
  wl_list_insert(&esurface->view->geometry.transformation_list, 	               &esurface->transform.link);
	weston_matrix_init(&esurface->transform.matrix);

	//weston_matrix_scale(&esurface->transform.matrix, esurface->scale, esurface->scale, 1.0f);
	weston_matrix_scale(&esurface->transform.matrix, esurface->scale, esurface->scale, 1.0f);
	//weston_matrix_translate(&esurface->transform.matrix, esurface->x - esurface->view->geometry.x, esurface->y - esurface->view->geometry.y, 0);
	weston_matrix_translate(&esurface->transform.matrix,
	  esurface->x - esurface->view->geometry.pos_offset.x,
	  esurface->y - esurface->view->geometry.pos_offset.y, 0);


	weston_view_geometry_dirty(esurface->view);
	weston_compositor_schedule_repaint(esurface->view->surface->compositor);

}

static void
exposay_descale(struct exposay_surface *esurface)
{

	/* Remove the static transformation set up by
	 * exposay_transform_in_done(). */
	wl_list_remove(&esurface->transform.link);


  if(esurface->mask_surface) {

    if(esurface->highlight) {
      weston_layer_entry_remove(&esurface->mask_view->layer_link);
      esurface->highlight = 0;
    }

    weston_view_destroy(esurface->mask_view);
    weston_surface_unref(esurface->mask_surface);
    weston_buffer_destroy_solid(esurface->mask_buffer_ref);
    esurface->mask_surface = NULL;
    esurface->mask_view = NULL;

  }

	weston_view_geometry_dirty(esurface->view);
  weston_compositor_schedule_repaint(esurface->shell->compositor);

}

static void
exposay_close(struct desktop_shell *shell)
{

  pid_t pid;
  struct wl_client *client;

  struct workspace *ws;
  struct weston_layer_entry *new_layer_link;
  struct exposay_surface *esurface = global_exposay.focus_current;

  if(!global_exposay.focus_current) {
    return;
  }

  client = wl_resource_get_client(esurface->view->surface->resource);
	wl_client_get_credentials(client, &pid, NULL, NULL);




  weston_layer_entry_remove(&esurface->mask_view->layer_link);
  weston_view_damage_below(esurface->mask_view);
  weston_view_destroy(esurface->mask_view);
  weston_surface_unref(esurface->mask_surface);

  esurface->mask_view = NULL;
  esurface->mask_surface = NULL;

  global_exposay.focus_current = NULL;
  global_exposay.focus_prev = NULL;





  wl_list_remove(&esurface->link);
  //weston_compositor_schedule_repaint(shell->compositor);
  //weston_desktop_surface_unlink_view(esurface->view);
  weston_view_unmap(esurface->view);



	kill(pid, SIGKILL);
  free(esurface);
}


static void
exposay_highlight_surface(struct desktop_shell *shell,
                          struct exposay_surface *esurface)
{


  struct exposay_surface *tmp_esurface;


  if (esurface->highlight == 1) {
    return;
  }

	struct weston_view *view = esurface->view;

  if( (!global_exposay.focus_prev && global_exposay.focus_current) || (global_exposay.focus_prev != global_exposay.focus_current) ) {
    if(global_exposay.focus_current != esurface && global_exposay.focus_current != NULL) {
      global_exposay.focus_prev = global_exposay.focus_current;
    }
  }

  global_exposay.focus_current = esurface;
  global_exposay.row_current = esurface->row;
	global_exposay.column_current = esurface->column;
	global_exposay.cur_output = esurface->eoutput;

  float divider = (float)255;

  /*
  						     53/divider,
						     108/divider,
						     163/divider,
  */

  #if 0

  int red = 78;
  int green = 109;
  int blue = 242;

  #endif


  int red = 36;
  int green = 134;
  int blue = 222;

  esurface->mask_buffer_ref = weston_buffer_create_solid_rgba(shell->compositor,
						     red/divider,
						     green/divider,
						     blue/divider,
						     1.0);

        /*
        esurface->mask_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, esurface->width, esurface->height);
        //cairo_set_source_surface(esurface->mask_surface);
        esurface->mask_cr = cairo_create(esurface->mask_surface);
        cairo_move_to(esurface->mask_cr, esurface->x, esurface->y);
        cairo_rectangle (esurface->mask_cr, 0, 0, 0.5, 0.5);
        cairo_set_source_rgba (esurface->mask_cr, 1, 0, 0, 0.80);
        cairo_fill (esurface->mask_cr);
        */

        wl_list_for_each(tmp_esurface, &global_exposay.surface_list, link) {

          if(tmp_esurface->highlight) {
            weston_layer_entry_remove(&tmp_esurface->mask_view->layer_link);

            weston_view_geometry_dirty(tmp_esurface->mask_view);
            weston_view_damage_below(tmp_esurface->mask_view);

//            weston_view_geometry_dirty(tmp_esurface->view);
//            weston_view_damage_below(tmp_esurface->view);

            tmp_esurface->highlight = 0;

          }

	      }



        if(!esurface->mask_surface) {

          esurface->mask_surface = weston_surface_create(shell->compositor);
          esurface->mask_view = weston_view_create(esurface->mask_surface);

          esurface->mask_surface->committed = NULL;
	        esurface->mask_surface->committed_private = NULL;

          esurface->mask_surface->width = esurface->width;
          esurface->mask_surface->height = 15;

          weston_surface_attach_solid(esurface->mask_view->surface, esurface->mask_buffer_ref,
            esurface->mask_surface->width,
				    esurface->mask_surface->height
				  );

				  weston_surface_map(esurface->mask_surface);
        }

        weston_view_set_position(esurface->mask_view, esurface->x, esurface->y + esurface->height - esurface->mask_surface->height + 1);
//        weston_view_set_output(esurface->mask_view, global_exposay.shell_output->output);
      	esurface->mask_view->is_mapped = true;

        weston_layer_entry_insert( &global_exposay.workspace->layer.view_list ,
        &esurface->mask_view->layer_link);

//        weston_view_geometry_dirty(esurface->mask_view);

//        weston_surface_damage(esurface->mask_surface);

        weston_compositor_schedule_repaint(shell->compositor);

        esurface->highlight = 1;

}



static void
exposay_pick(struct desktop_shell *shell, int x, int y)
{
	struct exposay_surface *esurface;
  struct exposay_surface *esurface2 = NULL;

	wl_list_for_each(esurface, &global_exposay.surface_list, link) {
		if (x < esurface->x || x > esurface->x + esurface->width)
			continue;

		if (y < esurface->y || y > esurface->y + esurface->height)
			continue;

		esurface2 = esurface;
	}
  if(esurface2 && !esurface2->highlight) {
    exposay_highlight_surface(shell, esurface2);
  }
}

static void
handle_view_destroy(struct wl_listener *listener, void *data)
{
//	struct exposay_surface *esurface = container_of(listener,
//						 struct exposay_surface,
//						 view_destroy_listener);

//	exposay_surface_destroy(esurface);
}

/* Pretty lame layout for now; just tries to make a square.  Should take
 * aspect ratio into account really.  Also needs to be notified of surface
 * addition and removal and adjust layout/animate accordingly. */
static enum exposay_layout_state
exposay_layout(struct desktop_shell *shell, struct shell_output *shell_output)
{
  struct workspace *workspace = get_current_workspace(shell);


	struct weston_output *output = shell_output->output;
	struct exposay_output *eoutput = &global_eoutput;
	struct weston_view *view;
	struct exposay_surface *esurface, *highlight = NULL;

	int w, h;
	int i;
	int last_row_removed = 0;

  int hpadding_outer = 0;
  int vpadding_outer = 0;
  int padding_inner = 20;

  struct weston_view *tmp;
	struct weston_view **minimized;

	wl_list_for_each_safe(view, tmp, &shell->minimized_layer.view_list.link, layer_link.link) {

		weston_layer_entry_remove(&view->layer_link);
		weston_layer_entry_insert(&workspace->layer.view_list, &view->layer_link);

		minimized = wl_array_add(&global_minimized_array, sizeof *minimized);
		*minimized = view;
	}



	eoutput->num_surfaces = 0;

	wl_list_for_each(view, &workspace->layer.view_list.link, layer_link.link) {
		if (!is_shell_surface(view->surface))
			continue;

		if (view->output != output)
			continue;
		eoutput->num_surfaces++;
	}

  printf("%d surfaces found \n", eoutput->num_surfaces);

	if (eoutput->num_surfaces == 0) {
		eoutput->grid_size = 0;
		eoutput->surface_size = 0;
		return EXPOSAY_LAYOUT_OVERVIEW;
	}

	/* Lay the grid out as square as possible, losing surfaces from the
	 * bottom row if required.  Start with fixed padding of a 10% margin
	 * around the outside and 80px internal padding between surfaces, and
	 * maximise the area made available to surfaces after this, but only
	 * to a maximum of 1/3rd the total output size.
	 *
	 * If we can't make a square grid, add one extra row at the bottom
	 * which will have a smaller number of columns.
	 *
	 * XXX: Surely there has to be a better way to express this maths,
	 *      right?!
	 */
	eoutput->grid_size = floor(sqrtf(eoutput->num_surfaces));

	if (pow(eoutput->grid_size, 2) != eoutput->num_surfaces)
		eoutput->grid_size++;

  if(eoutput->grid_size < 5) {
    eoutput->grid_size = 5;
  }

	last_row_removed = pow(eoutput->grid_size, 2) - eoutput->num_surfaces;

	hpadding_outer = 20;
	vpadding_outer = 20;

  if(output->width > 1900) {
    hpadding_outer = 12;
  	vpadding_outer = 12;
    padding_inner  = 11;
  }



  //TODO figure out how this works

	w =  output->width - hpadding_outer * 2;
	w = w - padding_inner * (eoutput->grid_size - 1);
	w = w / eoutput->grid_size;

	h = output->height - vpadding_outer * 2;
	h -= padding_inner * (eoutput->grid_size - 1);
	h /= eoutput->grid_size;

	eoutput->surface_size = (w < h) ? w : h;

  if(output->width > 1600)
    eoutput->grid_size = eoutput->grid_size + 2;

	i = 0;
	int acc_x = 0;
	int acc_y = 0;
	int max_y = 0;
	int last_row = 1;
	wl_list_for_each(view, &workspace->layer.view_list.link, layer_link.link) {

		if (!is_shell_surface(view->surface))
			continue;

    if (view->output != output)
			continue;

		esurface = malloc(sizeof(*esurface));

		if (!esurface) {
			exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, global_exposay.seat);
			break;
		}

		wl_list_insert(&global_exposay.surface_list, &esurface->link);
		esurface->shell = shell;
		esurface->eoutput = eoutput;
		esurface->view = view;
    esurface->mask_surface = NULL;
    esurface->mask_view = NULL;

		if (view->surface->width > view->surface->height)
			esurface->scale = (float)eoutput->surface_size / (float) view->surface->width;
		else
			esurface->scale = (float)eoutput->surface_size / (float) view->surface->height;

   //TODO remove default scale
    esurface->scale = esurface->scale * DEFAULT_SCALE;
		esurface->width = floor(view->surface->width * esurface->scale);
		esurface->height = floor(view->surface->height * esurface->scale);

		esurface->row = i / eoutput->grid_size;
		esurface->column = i % eoutput->grid_size;

		if(esurface->row != last_row) {
		  last_row = esurface->row;
		  acc_x = 0;
		  acc_y += max_y + padding_inner;
		  max_y = 0;
		}


		esurface->x = output->x + hpadding_outer;
		esurface->x += acc_x;

    acc_x = acc_x + esurface->width + padding_inner;
    if(esurface->height > max_y) {
      max_y = esurface->height;
    }

		esurface->y = output->y + vpadding_outer;
		esurface->y += acc_y;


		if (!highlight) {
			highlight = esurface;
    }

    esurface->highlight = 0;
    exposay_scale(esurface);
		/* We want our destroy handler to be after the animation
		 * destroy handler in the list, this way when the view is
		 * destroyed, the animation can safely call the animation
		 * completion callback before we free the esurface in our
		 * destroy handler.
		 */
		//esurface->view_destroy_listener.notify = handle_view_destroy;
		//wl_signal_add(&view->destroy_signal, &esurface->view_destroy_listener);

		i++;
	}


	if (highlight) {
		global_exposay.focus_current = NULL;
		exposay_highlight_surface(shell, highlight);
		printf("hightlight esurface->highlight %d \n", highlight->highlight);
	}

	weston_compositor_schedule_repaint(shell->compositor);

	return EXPOSAY_LAYOUT_ANIMATE_TO_OVERVIEW;
}

static void
exposay_focus(struct weston_pointer_grab *grab)
{
}




static void
exposay_motion(struct weston_pointer_grab *grab,
	       const struct timespec *time,
	       struct weston_pointer_motion_event *event)
{

	struct desktop_shell *shell = global_exposay.shell;

	struct weston_coord_global pos;
  pos = weston_pointer_motion_to_abs(grab->pointer, event);
	weston_pointer_move(grab->pointer, event);

  exposay_pick(shell,
	  (int)pos.c.x,
	  (int)pos.c.y
	);
}

static void
exposay_button(struct weston_pointer_grab *grab, const struct timespec *time,
	       uint32_t button, uint32_t state_w)
{
	struct desktop_shell *shell = global_exposay.shell;
	struct weston_seat *seat = grab->pointer->seat;
	enum wl_pointer_button_state state = state_w;



  if (button == BTN_LEFT) {
	  exposay_activate(shell);
    exposay_set_state(shell, EXPOSAY_TARGET_SWITCH, seat);
  } else if ( button == BTN_RIGHT &&  state == WL_POINTER_BUTTON_STATE_RELEASED) {
    exposay_set_state(shell, EXPOSAY_TARGET_CLOSE, seat);
    exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, seat);
    exposay_set_state(shell, EXPOSAY_TARGET_OVERVIEW, seat);
  }

	// Store the surface we clicked on, and don't do anything if we end up
	// releasing on a different surface.

        /*
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		global_exposay.clicked = global_exposay.focus_current;
		return;
	}

	if (global_exposay.focus_current == global_exposay.clicked)
		exposay_set_state(shell, EXPOSAY_TARGET_SWITCH, seat);
	else
		global_exposay.clicked = NULL;

        */
}

static void
exposay_axis(struct weston_pointer_grab *grab,
	     const struct timespec *time,
	     struct weston_pointer_axis_event *event)
{
}

static void
exposay_axis_source(struct weston_pointer_grab *grab, uint32_t source)
{
}

static void
exposay_frame(struct weston_pointer_grab *grab)
{
}

static void
exposay_pointer_grab_cancel(struct weston_pointer_grab *grab)
{
/*

	struct desktop_shell *shell =
		container_of(grab, struct desktop_shell, exposay.grab_ptr);

	exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, global_exposay.seat);

	*/
}

static const struct weston_pointer_grab_interface exposay_ptr_grab = {
	exposay_focus,
	exposay_motion,
	exposay_button,
	exposay_axis,
	exposay_axis_source,
	exposay_frame,
	exposay_pointer_grab_cancel,
};

static int
exposay_maybe_move(struct desktop_shell *shell, int row, int column)
{
	struct exposay_surface *esurface;

	wl_list_for_each(esurface, &global_exposay.surface_list, link) {
		if (esurface->eoutput != global_exposay.cur_output ||
		    esurface->row != row || esurface->column != column)
			continue;

		exposay_highlight_surface(shell, esurface);
		return 1;
	}

	return 0;
}

static void
exposay_key(struct weston_keyboard_grab *grab, const struct timespec *time,
	    uint32_t key, uint32_t state_w)
{
	struct weston_seat *seat = grab->keyboard->seat;
	struct desktop_shell *shell = global_exposay.shell;
	enum wl_keyboard_key_state state = state_w;

	if (state != WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (key) {
	case KEY_ESC:
		exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, seat);
		break;
  case KEY_Q:
		exposay_set_state(shell, EXPOSAY_TARGET_CLOSE, seat);
    exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, seat);
    exposay_set_state(shell, EXPOSAY_TARGET_OVERVIEW, seat);
		break;
	case KEY_ENTER:
	case KEY_SPACE:
		exposay_set_state(shell, EXPOSAY_TARGET_SWITCH, seat);
		break;
	case KEY_UP:
		exposay_maybe_move(shell, global_exposay.row_current - 1,
		                   global_exposay.column_current);
		break;
	case KEY_DOWN:
		/* Special case for trying to move to the bottom row when it
		 * has fewer items than all the others. */
		if (!exposay_maybe_move(shell, global_exposay.row_current + 1,
		                        global_exposay.column_current) &&
		    global_exposay.row_current < (global_exposay.cur_output->grid_size - 1)) {
			exposay_maybe_move(shell, global_exposay.row_current + 1,
					   (global_exposay.cur_output->num_surfaces %
					    global_exposay.cur_output->grid_size) - 1);
		}
		break;
	case KEY_LEFT:
		exposay_maybe_move(shell, global_exposay.row_current,
		                   global_exposay.column_current - 1);
		break;
	case KEY_RIGHT:
		exposay_maybe_move(shell, global_exposay.row_current,
		                   global_exposay.column_current + 1);
		break;
	case KEY_TAB:
		/* Try to move right, then down (and to the leftmost column),
		 * then if all else fails, to the top left. */
		if (!exposay_maybe_move(shell, global_exposay.row_current,
					global_exposay.column_current + 1) &&
		    !exposay_maybe_move(shell, global_exposay.row_current + 1, 0))
			exposay_maybe_move(shell, 0, 0);
		break;
	default:
		break;
	}
}

static void
exposay_modifier(struct weston_keyboard_grab *grab, uint32_t serial,
                 uint32_t mods_depressed, uint32_t mods_latched,
                 uint32_t mods_locked, uint32_t group)
{
	struct desktop_shell *shell = global_exposay.shell;
	struct weston_seat *seat = (struct weston_seat *) grab->keyboard->seat;

	/* We want to know when mod has been pressed and released.
	 * FIXME: There is a problem here: if mod is pressed, then a key
	 * is pressed and released, then mod is released, we will treat that
	 * as if only mod had been pressed and released. */
	if (seat->modifier_state) {
		if (seat->modifier_state == shell->binding_modifier) {
			global_exposay.mod_pressed = true;
		} else {
			global_exposay.mod_invalid = true;
		}
	} else {
		if (global_exposay.mod_pressed && !global_exposay.mod_invalid)
			exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, seat);

		global_exposay.mod_invalid = false;
		global_exposay.mod_pressed = false;
	}

	return;
}

static void
exposay_cancel(struct weston_keyboard_grab *grab)
{

	struct desktop_shell *shell =  global_exposay.shell;

	exposay_set_state(shell, EXPOSAY_TARGET_CANCEL, global_exposay.seat);

}

static const struct weston_keyboard_grab_interface exposay_kbd_grab = {
	exposay_key,
	exposay_modifier,
	exposay_cancel,
};

/**
 * Called when the transition from overview -> desktop has completed.
 */
static enum exposay_layout_state
exposay_set_inactive(struct desktop_shell *shell)
{

	struct weston_seat *seat = global_exposay.seat;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (pointer)
		weston_pointer_end_grab(pointer);

	if (keyboard) {
		weston_keyboard_end_grab(keyboard);
		if (keyboard->input_method_resource)
			keyboard->grab = &keyboard->input_method_grab;
	}

	return EXPOSAY_LAYOUT_INACTIVE;
}

/**
 * Begins the transition from overview to inactive. */
static enum exposay_layout_state
exposay_transition_inactive(struct desktop_shell *shell, int switch_focus, struct weston_seat *seat)
{
	struct exposay_surface *esurface;
  struct exposay_surface *tmp;
  struct weston_view *current_view = NULL;
  struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);


  printf("Clearing surfaces \n");

	if (switch_focus && global_exposay.focus_current != NULL) {
    current_view = global_exposay.focus_current->view;
		exposay_activate(shell);
  }

	wl_list_for_each_safe(esurface, tmp, &global_exposay.surface_list, link) {
    exposay_descale(esurface);
    exposay_surface_destroy(esurface);
  }

  printf("Clearing surfaces 2 \n");

  if(current_view != NULL) {
    /* re-hide surfaces that were temporary shown during the switch */


    struct weston_view **minimized;
    wl_array_for_each(minimized, &global_minimized_array) {
      /* with the exception of the current selected */
      if ( (*minimized)->surface != current_view->surface) {
        weston_layer_entry_remove(&(*minimized)->layer_link);
        weston_layer_entry_insert(&shell->minimized_layer.view_list, &(*minimized)->layer_link);
        weston_view_damage_below(*minimized);
      }
    }
  }
	wl_array_release(&global_minimized_array);


  printf("Clearing surfaces 3 \n");

  exposay_set_inactive(shell);

  //Focus active surface
  if(current_view) {
    struct weston_desktop_surface *desktop_surface =
  	  weston_surface_get_desktop_surface(current_view->surface);
    weston_desktop_surface_set_activated(desktop_surface, true);
    weston_compositor_schedule_repaint(shell->compositor);
  }

  return EXPOSAY_LAYOUT_INACTIVE;
}

static enum exposay_layout_state
exposay_transition_active(struct desktop_shell *shell)
{
	struct weston_seat *seat = NULL;

  seat = global_weston_seat;

  struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct shell_output *shell_output;

	global_exposay.shell = shell;
	global_exposay.workspace = get_current_workspace(shell);
	//global_exposay.focus_prev = get_default_view(keyboard->focus);
	//global_exposay.focus_current = get_default_view(keyboard->focus);

  global_exposay.focus_prev = NULL;
	global_exposay.focus_current = NULL;

	global_exposay.clicked = NULL;
	wl_list_init(&global_exposay.surface_list);
  wl_array_init(&global_minimized_array);


  lower_fullscreen_layer(shell, NULL);
  weston_compositor_schedule_repaint(shell->compositor);

	global_exposay.grab_kbd.interface = &exposay_kbd_grab;



	weston_keyboard_start_grab(keyboard,
	                           &global_exposay.grab_kbd);
	weston_keyboard_set_focus(keyboard, NULL);


	global_exposay.grab_ptr.interface = &exposay_ptr_grab;


	if (pointer) {
		weston_pointer_start_grab(pointer,
		                          &global_exposay.grab_ptr);
		weston_pointer_clear_focus(pointer);
	}



	wl_list_for_each(shell_output, &shell->output_list, link) {
		enum exposay_layout_state state;

    //printf("output name %s \n", shell_output->name);
		//state =
  	global_exposay.shell_output = shell_output;
    exposay_layout(shell, shell_output);

	}

  return EXPOSAY_LAYOUT_OVERVIEW;

}

static void
exposay_check_state(struct desktop_shell *shell, struct weston_seat *seat)
{
	enum exposay_layout_state state_new = global_exposay.state_cur;


	switch (global_exposay.state_target) {

	case EXPOSAY_TARGET_OVERVIEW:

		switch (global_exposay.state_cur) {
      case EXPOSAY_LAYOUT_OVERVIEW:
        return;
      default:
        state_new = exposay_transition_active(shell);
        global_exposay.state_cur = state_new;
        break;
		}
		break;

	case EXPOSAY_TARGET_CLOSE:
    exposay_close(shell);
    break;

	case EXPOSAY_TARGET_SWITCH:
	case EXPOSAY_TARGET_CANCEL:
    exposay_transition_inactive(shell, 1, seat);
    global_exposay.state_cur = EXPOSAY_LAYOUT_INACTIVE;
    break;
	}

}


static void
exposay_set_state(struct desktop_shell *shell, enum exposay_target_state state,
                  struct weston_seat *seat)
{
	global_exposay.state_target = state;
  if(!seat) {
	  global_exposay.seat = global_weston_seat;
  } else {
    global_exposay.seat = seat;
  }
	exposay_check_state(shell, seat);
}


/*

static struct workspace *
get_workspace(struct desktop_shell *shell, unsigned int index)
{
	struct workspace **pws = shell->workspaces.array.data;
	assert(index < shell->workspaces.num);
	pws += index;
	return *pws;
}

*/

struct workspace *
get_current_workspace(struct desktop_shell *shell)
{
  //Weston 11 removed workspaces
  return &shell->workspace;

//	return get_workspace(shell, shell->workspaces.current);
}



/* Move all fullscreen layers down to the current workspace and hide their
 * black views. The surfaces' state is set to both fullscreen and lowered,
 * and this is reversed when such a surface is re-configured, see
 * shell_configure_fullscreen() and shell_ensure_fullscreen_black_view().
 *
 * lowering_output = NULL - Lower on all outputs, else only lower on the
 *                   specified output.
 *
 * This should be used when implementing shell-wide overlays, such as
 * the alt-tab switcher, which need to de-promote fullscreen layers. */
void
lower_fullscreen_layer(struct desktop_shell *shell,
		       struct weston_output *lowering_output)
{
	struct workspace *ws;
	struct weston_view *view, *prev;

	ws = get_current_workspace(shell);

	wl_list_for_each_reverse_safe(view, prev,
				      &shell->fullscreen_layer.view_list.link,
				      layer_link.link) {

		/* Lower the view to the workspace layer */
		weston_layer_entry_remove(&view->layer_link);

    //only insert the actual surface
    //not the black view
    if (is_shell_surface(view->surface))
		  weston_layer_entry_insert(&ws->layer.view_list, &view->layer_link);

		weston_view_damage_below(view);
		weston_surface_damage(view->surface);
    weston_view_geometry_dirty(view);
    weston_compositor_schedule_repaint(shell->compositor);

	}
}



/** End exposay */
