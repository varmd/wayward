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

#ifndef __WAYWARD_PANEL_H__
#define __WAYWARD_PANEL_H__

#include <gtk/gtk.h>

#define WAYWARD_PANEL_TYPE                 (wayward_panel_get_type ())
#define WAYWARD_PANEL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAYWARD_PANEL_TYPE, WaywardPanel))
#define WAYWARD_PANEL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WAYWARD_PANEL_TYPE, WaywardPanelClass))
#define WAYWARD_IS_PANEL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAYWARD_PANEL_TYPE))
#define WAYWARD_IS_PANEL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WAYWARD_PANEL_TYPE))
#define WAYWARD_PANEL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WAYWARD_PANEL_TYPE, WaywardPanelClass))

typedef struct WaywardPanel WaywardPanel;
typedef struct WaywardPanelClass WaywardPanelClass;
typedef struct WaywardPanelPrivate WaywardPanelPrivate;

struct WaywardPanel
{
  GtkWindow parent;

  WaywardPanelPrivate *priv;
};

struct WaywardPanelClass
{
  GtkWindowClass parent_class;
};

#define WAYWARD_PANEL_WIDTH 56
#define WAYWARD_PANEL_HEIGHT_RATIO 0.73

typedef enum {
  WAYWARD_PANEL_BUTTON_NONE,
  WAYWARD_PANEL_BUTTON_SYSTEM,
  WAYWARD_PANEL_BUTTON_VOLUME
} WaywardPanelButton;

GType wayward_panel_get_type (void) G_GNUC_CONST;

GtkWidget * wayward_panel_new (void);

void wayward_panel_set_expand (WaywardPanel *self, gboolean expand);

void wayward_panel_show_previous (WaywardPanel *self, WaywardPanelButton button);

void wayward_panel_set_volume_icon_name (WaywardPanel *self,
    const gchar *icon_name);

#endif /* __WAYWARD_PANEL_H__ */
