/*
 * Copyright (C) 2013-2014 Collabora Ltd.
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
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __WAYWARD_LAUNCHER_H__
#define __WAYWARD_LAUNCHER_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#define WAYWARD_LAUNCHER_TYPE                 (wayward_launcher_get_type ())
#define WAYWARD_LAUNCHER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_LAUNCHER_TYPE, WaywardLauncher))
#define WAYWARD_LAUNCHER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_LAUNCHER_TYPE, WaywardLauncherClass))
#define WAYWARD_IS_LAUNCHER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_LAUNCHER_TYPE))
#define WAYWARD_IS_LAUNCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_LAUNCHER_TYPE))
#define WAYWARD_LAUNCHER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_LAUNCHER_TYPE, WaywardLauncherClass))

typedef struct WaywardLauncher WaywardLauncher;
typedef struct WaywardLauncherClass WaywardLauncherClass;
typedef struct WaywardLauncherPrivate WaywardLauncherPrivate;



struct WaywardLauncher
{
  GtkWindow parent;

  WaywardLauncherPrivate *priv;
};

struct WaywardLauncherClass
{
  GtkWindowClass parent_class;
};

GType wayward_launcher_get_type (void) G_GNUC_CONST;

GtkWidget * wayward_launcher_new (GtkWidget *background_widget);

void wayward_launcher_calculate (WaywardLauncher *self,
    gint *grid_window_width, gint *grid_window_height,
    gint *grid_cols,
    int array_size
    );

#endif /* __WAYWARD_LAUNCHER_H__ */
