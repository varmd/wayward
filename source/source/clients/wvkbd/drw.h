#ifndef __DRW_H
#define __DRW_H


#define DEFAULT_FONT "Sans 16"
#define DEFAULT_FONT_SIZE 16
#define DEFAULT_ROUNDING 0

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include <cairo.h>
#include <stdbool.h>

struct drw {
	struct wl_shm *shm;
};
struct drwsurf {
	uint32_t width, height, size;
	double scale;

	struct drw *ctx;
	struct wl_surface *surf;
	struct wl_buffer *buf;
	struct wl_shm *shm;
	unsigned char *pool_data;

  struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	cairo_t *cairo;
	cairo_surface_t *cairo_surface;

};
struct kbd;

void drwsurf_resize(struct drwsurf *ds, uint32_t w, uint32_t h, double s);
void drwsurf_flip(struct drwsurf *ds);

typedef union {
	uint8_t bgra[4];
	uint32_t color;
} Color;

void drw_do_clear(struct drwsurf *d, uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h);
void drw_do_rectangle(struct drwsurf *d, Color color, uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h, bool fill, int rounding);
void drw_fill_rectangle(struct drwsurf *d, Color color, uint32_t x, uint32_t y,
                        uint32_t w, uint32_t h, int rounding);
void drw_over_rectangle(struct drwsurf *d, Color color, uint32_t x, uint32_t y,
                        uint32_t w, uint32_t h, int rounding);
#ifdef USE_PANGO
void drw_draw_text(struct drwsurf *d, Color color, uint32_t x, uint32_t y,
                   uint32_t w, uint32_t h, uint32_t b, const char *label,
                   PangoFontDescription *font_description);
#else
void drw_draw_text(struct drwsurf *d, Color color, uint32_t x, uint32_t y,
                   uint32_t w, uint32_t h, uint32_t b, const char *label);
#endif

uint32_t setup_buffer(struct drwsurf *drwsurf);

#endif
