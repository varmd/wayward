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

#include "config.h"

#include "vertical-clock.h"

//#define GNOME_DESKTOP_USE_UNSTABLE_API
//#include <libgnome-desktop/gnome-wall-clock.h>

#include "panel.h"

struct WaywardVerticalClockPrivate {
  GtkWidget *label;

  //GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE(WaywardVerticalClock, wayward_vertical_clock, GTK_TYPE_BOX)

/* this widget takes up the entire width of the panel and displays
 * padding for the first (panel width - vertical clock width) pixels,
 * then shows the vertical clock itself. the idea is to put this into
 * a GtkRevealer and only show it when appropriate. */

static void
wayward_vertical_clock_init (WaywardVerticalClock *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      WAYWARD_VERTICAL_CLOCK_TYPE,
      WaywardVerticalClockPrivate);
}

static void
wall_clock_notify_cb (
    WaywardVerticalClock *self)
{
  GDateTime *datetime;
  gchar *str;

  datetime = g_date_time_new_now_local ();

  /*
  str = g_date_time_format (datetime,
      "<span font=\"Droid Sans 12\">%H\n"
      ":\n"
      "%M</span>");
  */
  
  str = g_date_time_format (datetime,
      "<span font=\"Droid Sans 12\">%H:%M</span>");
  gtk_label_set_markup (GTK_LABEL (self->priv->label), str);

  g_free (str);
  g_date_time_unref (datetime);
}

static void
wayward_vertical_clock_constructed (GObject *object)
{
  WaywardVerticalClock *self = WAYWARD_VERTICAL_CLOCK (object);
  GtkWidget *padding;
  gint width;

  G_OBJECT_CLASS (wayward_vertical_clock_parent_class)->constructed (object);

  //self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  //g_timeout_add (self->priv->wall_clock, "notify::clock", G_CALLBACK (wall_clock_notify_cb), self);
  g_timeout_add (5000, G_SOURCE_FUNC(wall_clock_notify_cb), self);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

  /* a label just to pad things out to the correct width */
  width = WAYWARD_PANEL_WIDTH - WAYWARD_VERTICAL_CLOCK_WIDTH;

  padding = gtk_label_new ("");
  gtk_style_context_add_class (gtk_widget_get_style_context (padding),
      "wayward-clock");
  gtk_widget_set_size_request (padding, width, -1);
  gtk_box_pack_end (GTK_BOX (self), padding, FALSE, FALSE, 0);
  
  padding = gtk_label_new ("");
  gtk_style_context_add_class (gtk_widget_get_style_context (padding),
      "wayward-clock");
  gtk_widget_set_size_request (padding, width, -1);
  gtk_box_pack_start (GTK_BOX (self), padding, FALSE, FALSE, 0);

  /* the actual clock label */
  self->priv->label = gtk_label_new ("");
  gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->label),
      "wayward-clock");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request (self->priv->label,
      WAYWARD_VERTICAL_CLOCK_WIDTH, -1);
  gtk_box_pack_start (GTK_BOX (self), self->priv->label, FALSE, FALSE, 0);

  wall_clock_notify_cb (self);
}

static void
wayward_vertical_clock_dispose (GObject *object)
{
  WaywardVerticalClock *self = WAYWARD_VERTICAL_CLOCK (object);

  G_OBJECT_CLASS (wayward_vertical_clock_parent_class)->dispose (object);
}

static void
wayward_vertical_clock_class_init (WaywardVerticalClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = wayward_vertical_clock_constructed;
  object_class->dispose = wayward_vertical_clock_dispose;

  g_type_class_add_private (object_class, sizeof (WaywardVerticalClockPrivate));
}

GtkWidget *
wayward_vertical_clock_new (void)
{
  return g_object_new (WAYWARD_VERTICAL_CLOCK_TYPE,
      NULL);
}
