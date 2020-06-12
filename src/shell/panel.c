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

#include "panel.h"

#include "app-icon.h"
#include "favorites.h"
#include "vertical-clock.h"

enum {
  APP_MENU_TOGGLED,
  SYSTEM_TOGGLED,
  VOLUME_TOGGLED,
  FAVORITE_LAUNCHED,
  EXPOSAY_TOGGLED,
  INHIBIT_TOGGLED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct WaywardPanelPrivate {
  gboolean hidden;

  GtkWidget *revealer_buttons; /* for the top buttons */
  GtkWidget *revealer_clock; /* for the vertical clock */

  GtkWidget *system_button;

  gboolean volume_showing;
  GtkWidget *volume_button;
  gchar *volume_icon_name;
};

G_DEFINE_TYPE_WITH_PRIVATE(WaywardPanel, wayward_panel, GTK_TYPE_WINDOW)

static void
wayward_panel_init (WaywardPanel *self)
{
  //self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, WAYWARD_PANEL_TYPE,      WaywardPanelPrivate);
  self->priv = wayward_panel_get_instance_private(self);

  self->priv->volume_icon_name = g_strdup ("audio-volume-high-symbolic");
}

static gboolean
widget_enter_notify_event_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    WaywardPanel *self)
{
  gboolean handled;
  g_signal_emit_by_name (self, "enter-notify-event", event, &handled);
  return handled;
}

static void
widget_connect_enter_signal (WaywardPanel *self,
    GtkWidget *widget)
{
  g_signal_connect (widget, "enter-notify-event",
      G_CALLBACK (widget_enter_notify_event_cb), self);
}

static void
app_menu_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  g_signal_emit (self, signals[APP_MENU_TOGGLED], 0);
}


static void
system_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  g_signal_emit (self, signals[SYSTEM_TOGGLED], 0);
}

static void
volume_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  g_signal_emit (self, signals[VOLUME_TOGGLED], 0);
}

static void
favorite_launched_cb (WaywardFavorites *favorites,
    WaywardPanel *self)
{
  g_signal_emit (self, signals[FAVORITE_LAUNCHED], 0);
}

static void
exposay_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  g_signal_emit (self, signals[EXPOSAY_TOGGLED], 0);
}


int inhibit_state = 0;

static void
inhibit_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  
  
  g_signal_emit (self, signals[INHIBIT_TOGGLED], 0);
  
  
  
  if(!inhibit_state) {
    
    inhibit_state = 1;
    gtk_style_context_remove_class (
      gtk_widget_get_style_context (GTK_WIDGET(button)), "wayward-idle"
    );
    
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)),
      "wayward-idle-active");
  } else {
    inhibit_state = 0;
    
    gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), "wayward-idle");
    
    gtk_style_context_remove_class (gtk_widget_get_style_context (GTK_WIDGET(button)),
      "wayward-idle-active");
  }
}

static void
wayward_panel_constructed (GObject *object)
{
  WaywardPanel *self = WAYWARD_PANEL (object);
  GtkWidget *main_box, *menu_box, *buttons_box;
  GtkWidget *ebox;
  GtkWidget *image;
  GtkWidget *button;
  GtkWidget *favorites;

  G_OBJECT_CLASS (wayward_panel_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "wayward");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  /* make it black and slightly alpha */
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wayward-panel");

  /* main vbox */
  main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (self), main_box);

  /* for the top buttons and vertical clock we have a few more
   * boxes. the hbox has two cells. in each cell there is a
   * GtkRevealer for hiding and showing the content. only one revealer
   * is ever visibile at one point and transitions happen at the same
   * time so the width stays constant (the animation duration is the
   * same). the first revealer contains another box which has the two
   * wifi and sound buttons. the second revealer has the vertical
   * clock widget.
   */

  /* GtkBoxes seem to eat up enter/leave events, so let's use an event
   * box for the entire thing. */
  ebox = gtk_event_box_new ();
  //gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  widget_connect_enter_signal (self, ebox);

  menu_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (ebox), menu_box);

  /* revealer for the top buttons */
  self->priv->revealer_buttons = gtk_revealer_new ();
  gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->revealer_buttons),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_buttons),
      TRUE);
  gtk_box_pack_start (GTK_BOX (menu_box),
      self->priv->revealer_buttons, FALSE, FALSE, 0);

  /* the box for the top buttons */
  buttons_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_buttons), buttons_box);
  
  
  /* app menu button */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (buttons_box), ebox, FALSE, FALSE, 0);
  
  button = wayward_app_icon_new ("view-grid-symbolic");
  g_signal_connect (button, "clicked",
      G_CALLBACK (app_menu_button_clicked_cb), self);
      
      
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);  

  //g_signal_connect (GTK_WINDOW(self), "key_press_event", G_CALLBACK (app_menu_key_pressed_cb), NULL);

  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);
  
  
  /* vertical clock */
  /* revealer for the vertical clock */
  //self->priv->revealer_clock = gtk_revealer_new ();
  self->priv->revealer_clock = gtk_event_box_new ();
  //gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->revealer_clock),
  //    GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  //gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_clock),
  //    FALSE);
  gtk_box_pack_end (GTK_BOX (main_box),
      self->priv->revealer_clock, FALSE, FALSE, 0);
  
//  gtk_container_add (GTK_CONTAINER (self->priv->revealer_clock),
//      wayward_vertical_clock_new ());
  

  /* sound button */
  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  button = gtk_button_new_from_icon_name (self->priv->volume_icon_name,
      GTK_ICON_SIZE_LARGE_TOOLBAR);
      
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);    
      
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
      "wayward-audio");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "image-button");
  g_signal_connect (button, "clicked",
      G_CALLBACK (volume_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);
  self->priv->volume_button = button;


  /* system button */
  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  button = gtk_button_new_from_icon_name ("emblem-system-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
      
  gtk_button_set_relief( GTK_BUTTON(button), GTK_RELIEF_NONE);    
      
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
      "wayward-system");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "image-button");
  g_signal_connect (button, "clicked",
      G_CALLBACK (system_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);
  self->priv->system_button = button;


  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  button = gtk_button_new_from_icon_name ("open-menu-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
      
  gtk_button_set_relief( GTK_BUTTON(button), GTK_RELIEF_NONE);    
      
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
      "wayward-system");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "image-button");
  g_signal_connect (button, "clicked",
      G_CALLBACK (exposay_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);


  /* idle inhibit button */

  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  button = gtk_button_new_from_icon_name ("tv-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
      
  gtk_button_set_relief( GTK_BUTTON(button), GTK_RELIEF_NONE);    
      
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
      "wayward-idle");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "button");
  gtk_style_context_remove_class (gtk_widget_get_style_context (button),
      "image-button");
  g_signal_connect (button, "clicked",
      G_CALLBACK (inhibit_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);

  /* idle inhibit button */

  

  

  /* end of the menu buttons and vertical clock */

  /* favorites */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  favorites = wayward_favorites_new ();
  gtk_container_add (GTK_CONTAINER (ebox), favorites);
  widget_connect_enter_signal (self, ebox);

  g_signal_connect (favorites, "app-launched",
      G_CALLBACK (favorite_launched_cb), self);

  

  /* done */
  self->priv->hidden = FALSE;
  self->priv->volume_showing = FALSE;
}

static void
wayward_panel_dispose (GObject *object)
{
  WaywardPanel *self = WAYWARD_PANEL (object);

  g_free (self->priv->volume_icon_name);
  self->priv->volume_icon_name = NULL;

  G_OBJECT_CLASS (wayward_panel_parent_class)->dispose (object);
}

static void
wayward_panel_class_init (WaywardPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GParamSpec *param_spec;

  object_class->constructed = wayward_panel_constructed;
  object_class->dispose = wayward_panel_dispose;

  signals[APP_MENU_TOGGLED] = g_signal_new ("app-menu-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[SYSTEM_TOGGLED] = g_signal_new ("system-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[VOLUME_TOGGLED] = g_signal_new ("volume-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[FAVORITE_LAUNCHED] = g_signal_new ("favorite-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[EXPOSAY_TOGGLED] = g_signal_new ("exposay-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  signals[INHIBIT_TOGGLED] = g_signal_new ("inhibit-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (WaywardPanelPrivate));
}

GtkWidget *
wayward_panel_new (void)
{
  return g_object_new (WAYWARD_PANEL_TYPE,
      NULL);
}

void
wayward_panel_set_expand (WaywardPanel *self,
    gboolean expand)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_buttons), expand);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_clock), !expand);
}

static void
set_icon (GtkWidget *button,
    const gchar *icon_name)
{
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name,
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_image (GTK_BUTTON (button),
      image);
}

void
wayward_panel_show_previous (WaywardPanel *self,
    WaywardPanelButton button)
{
  switch (button)
    {
    case WAYWARD_PANEL_BUTTON_SYSTEM:
      set_icon (self->priv->system_button, "go-previous-symbolic");
      set_icon (self->priv->volume_button, self->priv->volume_icon_name);
      self->priv->volume_showing = FALSE;
      break;
    case WAYWARD_PANEL_BUTTON_VOLUME:
      set_icon (self->priv->system_button, "emblem-system-symbolic");
      set_icon (self->priv->volume_button, "go-previous-symbolic");
      self->priv->volume_showing = TRUE;
      break;
    case WAYWARD_PANEL_BUTTON_NONE:
    default:
      set_icon (self->priv->system_button, "emblem-system-symbolic");
      set_icon (self->priv->volume_button, self->priv->volume_icon_name);
      self->priv->volume_showing = FALSE;
    }
}

void
wayward_panel_set_volume_icon_name (WaywardPanel *self,
    const gchar *icon_name)
{
  g_free (self->priv->volume_icon_name);
  self->priv->volume_icon_name = g_strdup (icon_name);

  if (!self->priv->volume_showing)
    set_icon (self->priv->volume_button, icon_name);
}
