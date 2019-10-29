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

#include "app-icon.h"

G_DEFINE_TYPE(WaywardAppIcon, wayward_app_icon, GTK_TYPE_BUTTON)

static void
wayward_app_icon_init (WaywardAppIcon *self)
{


  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wayward-apps");

  gtk_style_context_remove_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "button");


  gtk_style_context_remove_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "image-button");

  gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);

}

static void
wayward_app_icon_class_init (WaywardAppIconClass *klass)
{
}

GtkWidget *
wayward_app_icon_new (const gchar *icon_name)
{
  GtkWidget *widget;
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
  widget = g_object_new (WAYWARD_APP_ICON_TYPE,
      "image", image,
      NULL);

  return widget;
}

GtkWidget *
wayward_app_icon_new_from_gicon (GIcon *icon)
{
  GtkWidget *widget, *image;

  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);

  widget = g_object_new (WAYWARD_APP_ICON_TYPE,
      "image", image,
      NULL);

  return widget;
}
