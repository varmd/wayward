/*
 * Copyright (C) 2014 Collabora Ltd.
 * Copyright (C) 2017-2019 varmd
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

#ifndef __WAYWARD_CLOCK_H__
#define __WAYWARD_CLOCK_H__

#include <gtk/gtk.h>

#include "panel.h"

#define WAYWARD_CLOCK_TYPE                 (wayward_clock_get_type ())
#define WAYWARD_CLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_CLOCK_TYPE, WaywardClock))
#define WAYWARD_CLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_CLOCK_TYPE, WaywardClockClass))
#define WAYWARD_IS_CLOCK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_CLOCK_TYPE))
#define WAYWARD_IS_CLOCK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_CLOCK_TYPE))
#define WAYWARD_CLOCK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_CLOCK_TYPE, WaywardClockClass))

typedef struct WaywardClock WaywardClock;
typedef struct WaywardClockClass WaywardClockClass;
typedef struct WaywardClockPrivate WaywardClockPrivate;

struct WaywardClock
{
  GtkWindow parent;

  WaywardClockPrivate *priv;
};

struct WaywardClockClass
{
  GtkWindowClass parent_class;
};

#define WAYWARD_CLOCK_WIDTH (WAYWARD_PANEL_WIDTH * 2.6)
#define WAYWARD_CLOCK_HEIGHT (WAYWARD_PANEL_WIDTH * 2)

typedef enum {
  WAYWARD_CLOCK_SECTION_CLOCK,
  WAYWARD_CLOCK_SECTION_SYSTEM,
  WAYWARD_CLOCK_SECTION_VOLUME
} WaywardClockSection;

GType wayward_clock_get_type (void) G_GNUC_CONST;

GtkWidget * wayward_clock_new (void);

void wayward_clock_show_section (WaywardClock *self, WaywardClockSection section);

void clock_volume_mute ();
void clock_volume_up ();
void clock_volume_down ();
void clock_shutdown ();
void clock_restart ();

#endif /* __WAYWARD_CLOCK_H__ */
