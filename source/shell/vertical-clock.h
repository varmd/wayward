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

#ifndef __WAYWARD_VERTICAL_CLOCK_H__
#define __WAYWARD_VERTICAL_CLOCK_H__

#include <gtk/gtk.h>

#define WAYWARD_VERTICAL_CLOCK_TYPE                 (wayward_vertical_clock_get_type ())
#define WAYWARD_VERTICAL_CLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_VERTICAL_CLOCK_TYPE, WaywardVerticalClock))
#define WAYWARD_VERTICAL_CLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_VERTICAL_CLOCK_TYPE, WaywardVerticalClockClass))
#define WAYWARD_IS_VERTICAL_CLOCK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_VERTICAL_CLOCK_TYPE))
#define WAYWARD_IS_VERTICAL_CLOCK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_VERTICAL_CLOCK_TYPE))
#define WAYWARD_VERTICAL_CLOCK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_VERTICAL_CLOCK_TYPE, WaywardVerticalClockClass))

typedef struct WaywardVerticalClock WaywardVerticalClock;
typedef struct WaywardVerticalClockClass WaywardVerticalClockClass;
typedef struct WaywardVerticalClockPrivate WaywardVerticalClockPrivate;

struct WaywardVerticalClock
{
  GtkBox parent;

  WaywardVerticalClockPrivate *priv;
};

struct WaywardVerticalClockClass
{
  GtkBoxClass parent_class;
};

#define WAYWARD_VERTICAL_CLOCK_WIDTH 25

GType wayward_vertical_clock_get_type (void) G_GNUC_CONST;

GtkWidget * wayward_vertical_clock_new (void);

#endif /* __WAYWARD_VERTICAL_CLOCK_H__ */
