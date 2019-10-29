/*
 * Copyright (C) 2013 Collabora Ltd.
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
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __WAYWARD_FAVORITES_H__
#define __WAYWARD_FAVORITES_H__

#include <gtk/gtk.h>

#define WAYWARD_TYPE_FAVORITES                 (wayward_favorites_get_type ())
#define WAYWARD_FAVORITES(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_TYPE_FAVORITES, WaywardFavorites))
#define WAYWARD_FAVORITES_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_TYPE_FAVORITES, WaywardFavoritesClass))
#define WAYWARD_IS_FAVORITES(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_TYPE_FAVORITES))
#define WAYWARD_IS_FAVORITES_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_TYPE_FAVORITES))
#define WAYWARD_FAVORITES_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_TYPE_FAVORITES, WaywardFavoritesClass))

typedef struct WaywardFavorites WaywardFavorites;
typedef struct WaywardFavoritesClass WaywardFavoritesClass;
typedef struct WaywardFavoritesPrivate WaywardFavoritesPrivate;

struct WaywardFavorites
{
  GtkBox parent;

  WaywardFavoritesPrivate *priv;
};

struct WaywardFavoritesClass
{
  GtkBoxClass parent_class;
};

GType      wayward_favorites_get_type         (void) G_GNUC_CONST;
GtkWidget *wayward_favorites_new              (void);

#endif /* __WAYWARD_FAVORITES_H__ */
