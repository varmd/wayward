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

#ifndef __WAYWARD_APP_ICON_H__
#define __WAYWARD_APP_ICON_H__

#include <gtk/gtk.h>

#define WAYWARD_APP_ICON_TYPE                 (wayward_app_icon_get_type ())
#define WAYWARD_APP_ICON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_APP_ICON_TYPE, WaywardAppIcon))
#define WAYWARD_APP_ICON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_APP_ICON_TYPE, WaywardAppIconClass))
#define WAYWARD_IS_APP_ICON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_APP_ICON_TYPE))
#define WAYWARD_IS_APP_ICON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_APP_ICON_TYPE))
#define WAYWARD_APP_ICON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_APP_ICON_TYPE, WaywardAppIconClass))

typedef struct WaywardAppIcon WaywardAppIcon;
typedef struct WaywardAppIconClass WaywardAppIconClass;
typedef struct WaywardAppIconPrivate WaywardAppIconPrivate;

struct WaywardAppIcon
{
  GtkButton parent;

  WaywardAppIconPrivate *priv;
};

struct WaywardAppIconClass
{
  GtkButtonClass parent_class;
};

GType      wayward_app_icon_get_type       (void) G_GNUC_CONST;
GtkWidget *wayward_app_icon_new            (const gchar *icon_name);
GtkWidget *wayward_app_icon_new_from_gicon (GIcon *icon);

#endif /* __WAYWARD_APP_ICON_H__ */
