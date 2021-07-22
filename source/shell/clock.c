/*
 * Copyright (C) 2017-2019 varmd
 * Copyright (C) 2014 Collabora Ltd.
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

#include <alsa/asoundlib.h>


#include "clock.h"

enum {
  VOLUME_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct WaywardClockPrivate {
  GtkWidget *revealer_clock;
  GtkWidget *revealer_system;
  GtkWidget *revealer_volume;

  GtkWidget *label;

  GtkWidget *volume_scale;
  GtkWidget *volume_image;

  snd_mixer_t *mixer_handle;
  snd_mixer_elem_t *mixer;
  glong min_volume, max_volume;
};

static WaywardClock *global_clock = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(WaywardClock, wayward_clock, GTK_TYPE_WINDOW)

static void setup_mixer (WaywardClock *self);
static gboolean volume_idle_cb (gpointer data);

static void
wayward_clock_init (WaywardClock *self)
{
  self->priv = wayward_clock_get_instance_private (self);
}

static gdouble
alsa_volume_to_percentage (WaywardClock *self,
    glong value)
{
  glong range;

  /* min volume isn't always zero unfortunately */
  range = self->priv->max_volume - self->priv->min_volume;

  value -= self->priv->min_volume;

  return (value / (gdouble) range) * 100;
}

static glong
percentage_to_alsa_volume (WaywardClock *self,
    gdouble value)
{
  glong range;

  /* min volume isn't always zero unfortunately */
  range = self->priv->max_volume - self->priv->min_volume;

  return (range * value / 100) + self->priv->min_volume;
}

static void volume_set (gdouble value, WaywardClock *self) {
  
  const gchar *icon_name;
  GtkWidget *box;
  

  printf("volume is %f \n", value);
  
  if (self->priv->mixer != NULL)
  {
    snd_mixer_selem_set_playback_volume_all (self->priv->mixer, percentage_to_alsa_volume (self, value));
  }

  /* update the icon */
  if (value > 75)
    icon_name = "audio-volume-high-symbolic";
  else if (value > 30)
    icon_name = "audio-volume-medium-symbolic";
  else if (value > 0)
    icon_name = "audio-volume-low-symbolic";
  else
    icon_name = "audio-volume-muted-symbolic";
  
  printf("icon_name is %s \n", icon_name);

  box = gtk_widget_get_parent (self->priv->volume_image);
  gtk_widget_destroy (self->priv->volume_image);
  self->priv->volume_image = gtk_image_new_from_icon_name (
      icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (box), self->priv->volume_image,
      FALSE, FALSE, 0);
  gtk_widget_show (self->priv->volume_image);

  g_signal_emit (self, signals[VOLUME_CHANGED], 0, value, icon_name);  
  volume_idle_cb(self);
}


//Called from wayward.c
void clock_volume_mute () {
  volume_set(0, global_clock);
}

void clock_volume_up () {
  
  WaywardClock *self = global_clock;
  long volume;
  
  snd_mixer_handle_events (self->priv->mixer_handle);
  snd_mixer_selem_get_playback_volume (self->priv->mixer,
  0, &volume);
  
  printf("volume up -volume is %d \n", volume);
  
  
  volume_set( alsa_volume_to_percentage(global_clock, (gdouble)(volume + 10)), global_clock);

}

void clock_volume_down () {
  
  WaywardClock *self = global_clock;
  long volume;
  
  snd_mixer_handle_events (self->priv->mixer_handle);
  snd_mixer_selem_get_playback_volume (self->priv->mixer,
  0, &volume);
  
  printf("volume down - volume is %d \n", volume);
  
  
  volume_set( alsa_volume_to_percentage(global_clock, (gdouble)(volume - 10)), global_clock);

}



static void volume_changed_cb (GtkRange *range,
    WaywardClock *self)
{
  gdouble value;  

  value = gtk_range_get_value (range);
  
  volume_set (value, self);
  
  printf("volume_changed_cb: %f is value \n", value);
  
  
}

static gboolean volume_startup_panel_cb (gpointer data)
{
  WaywardClock *self = WAYWARD_CLOCK (data);
  volume_changed_cb(GTK_RANGE (self->priv->volume_scale), self);
  return G_SOURCE_REMOVE;
}

static gboolean
volume_idle_cb (gpointer data)
{
  WaywardClock *self = WAYWARD_CLOCK (data);
  glong volume;
  
  if (self->priv->mixer != NULL)
    {
      //snd_mixer_close (self->priv->mixer_handle);
      //setup_mixer(self);
      snd_mixer_handle_events (self->priv->mixer_handle);
      snd_mixer_selem_get_playback_volume (self->priv->mixer,
          0, &volume);
      
      gtk_range_set_value (GTK_RANGE (self->priv->volume_scale),
          alsa_volume_to_percentage (self, volume));
      
      printf("%d is volume \n", volume);
      printf("%f is value \n", gtk_range_get_value ( GTK_RANGE (self->priv->volume_scale) ));
    }

  return TRUE;
}




static void
system_shutdown_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  //char *args[] = {"/usr/bin/sudo", , "/usr/bin/systemctl", "poweroff", (char *)0};
  execl ("/usr/bin/sudo", "/usr/bin/sudo", "/usr/bin/systemctl", "poweroff", (char *)0);
}

static void
system_restart_button_clicked_cb (GtkButton *button,
    WaywardPanel *self)
{
  
  //char *args[] = {"/usr/bin/systemctl", "reboot", (char *)0};
  execl ("/usr/bin/sudo", "/usr/bin/sudo", "/usr/bin/systemctl", "reboot", (char *)0);
//  char *args[] = {"systemctl", "reboot", (char *)0};
//  execve(args[0], args, NULL);
//  execve("/usr/bin/systemctl reboot", NULL, NULL);
   //system("/usr/bin/systemctl reboot");
                                             
                                                  
}


void clock_shutdown () {
  system_shutdown_button_clicked_cb(NULL, NULL);
}

void clock_restart () {
  system_restart_button_clicked_cb(NULL, NULL);
}


static GtkWidget *
create_system_box (WaywardClock *self)
{
  GtkWidget *button;
  GtkWidget *buttons_box;
  GtkWidget *ebox;
  
  buttons_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (buttons_box), ebox, FALSE, FALSE, 0);
  button = wayward_app_icon_new ("view-refresh-symbolic");
  /*
  button = gtk_button_new_from_icon_name ("view-refresh-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  */
  g_signal_connect (button, "clicked",
      G_CALLBACK (system_restart_button_clicked_cb), self);
  
  gtk_container_add (GTK_CONTAINER (ebox), button);
  
  
  //Shutdown
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (buttons_box), ebox, FALSE, FALSE, 0);
  /*
  button = gtk_button_new_from_icon_name ("system-shutdown-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  */
  button = wayward_app_icon_new ("system-shutdown-symbolic");  
  g_signal_connect (button, "clicked",
      G_CALLBACK (system_shutdown_button_clicked_cb), self);
  
  gtk_container_add (GTK_CONTAINER (ebox), button);
  
  

//  widget_connect_enter_signal (self, button);

  
  return buttons_box;
}

static GtkWidget *
create_volume_box (WaywardClock *self)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  self->priv->volume_image = gtk_image_new_from_icon_name (
      "audio-volume-muted-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (box), self->priv->volume_image,
      FALSE, FALSE, 0);

  self->priv->volume_scale = gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (self->priv->volume_scale), FALSE);
  gtk_widget_set_size_request (self->priv->volume_scale, 100, -1);
  gtk_box_pack_end (GTK_BOX (box), self->priv->volume_scale, TRUE, TRUE, 0);

  g_signal_connect (self->priv->volume_scale, "value-changed",
      G_CALLBACK (volume_changed_cb), self);

  /* set the initial value in an idle so ::volume-changed is emitted
   * when other widgets are connected to the signal and can react
   * accordingly. */
  //g_idle_add (volume_idle_cb, self);
  
  
  g_timeout_add_seconds (30, volume_idle_cb, self);

  return box;
}
/*GnomeWallClock *wall_clock,
    GParamSpec *pspec,*/
static int
wall_clock_notify_cb ( WaywardClock *self)
{
  GDateTime *datetime;
  gchar *str;

  datetime = g_date_time_new_now_local ();

  str = g_date_time_format (datetime,
      "<span font=\"Droid Sans 32\">%H:%M</span>\n"
      "<span font=\"Droid Sans 12\">%d/%m/%Y</span>");
  gtk_label_set_markup (GTK_LABEL (self->priv->label), str);

  g_free (str);
  g_date_time_unref (datetime);
  
  return 1;
}

static void
setup_mixer (WaywardClock *self)
{
  snd_mixer_selem_id_t *sid;
  gint ret;

  /* this is all pretty specific to the rpi */

  if ((ret = snd_mixer_open (&self->priv->mixer_handle, 0)) < 0)
    goto error;

  if ((ret = snd_mixer_attach (self->priv->mixer_handle, "default")) < 0)
    goto error;

  if ((ret = snd_mixer_selem_register (self->priv->mixer_handle, NULL, NULL)) < 0)
    goto error;

  if ((ret = snd_mixer_load (self->priv->mixer_handle)) < 0)
    goto error;

  snd_mixer_selem_id_alloca (&sid);
  snd_mixer_selem_id_set_index (sid, 0);
  snd_mixer_selem_id_set_name (sid, "Master");
  self->priv->mixer = snd_mixer_find_selem (self->priv->mixer_handle, sid);

  /* fallback to mixer "Master" */
  if (self->priv->mixer == NULL)
    {
      snd_mixer_selem_id_set_name (sid, "PCM");
      self->priv->mixer = snd_mixer_find_selem (self->priv->mixer_handle, sid);
      if (self->priv->mixer == NULL)
        goto error;
    }

  if ((ret = snd_mixer_selem_get_playback_volume_range (self->priv->mixer,
              &self->priv->min_volume, &self->priv->max_volume)) < 0)
    goto error;

  return;

error:
  g_debug ("failed to setup mixer: %s", snd_strerror (ret));

  if (self->priv->mixer_handle != NULL)
    snd_mixer_close (self->priv->mixer_handle);
  self->priv->mixer_handle = NULL;
  self->priv->mixer = NULL;
}

static void
wayward_clock_constructed (GObject *object)
{
  WaywardClock *self = WAYWARD_CLOCK (object);
  GtkWidget *box, *system_box, *volume_box;

  G_OBJECT_CLASS (wayward_clock_parent_class)->constructed (object);

  //self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  //g_signal_connect (self->priv->wall_clock, "notify::clock", G_CALLBACK (wall_clock_notify_cb), self);
  g_timeout_add_seconds (10, G_SOURCE_FUNC(wall_clock_notify_cb), self);

  gtk_window_set_title (GTK_WINDOW (self), "wayward");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wayward-clock");

  /* the box for the revealers */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (self), box);

  /* volume */
  self->priv->revealer_volume = gtk_revealer_new ();
  gtk_revealer_set_transition_type (
      GTK_REVEALER (self->priv->revealer_volume),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_volume), FALSE);
  gtk_box_pack_start (GTK_BOX (box), self->priv->revealer_volume,
      TRUE, TRUE, 0);

  volume_box = create_volume_box (self);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_volume),
      volume_box);

  /* system */
  self->priv->revealer_system = gtk_revealer_new ();
  gtk_revealer_set_transition_type (
      GTK_REVEALER (self->priv->revealer_system),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_system), FALSE);
  gtk_box_pack_start (GTK_BOX (box), self->priv->revealer_system,
      TRUE, TRUE, 0);

  system_box = create_system_box (self);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_system),
      system_box);

  /* clock */
  self->priv->revealer_clock = gtk_revealer_new ();
  gtk_revealer_set_transition_type (
      GTK_REVEALER (self->priv->revealer_clock),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_clock), TRUE);
  gtk_box_pack_start (GTK_BOX (box), self->priv->revealer_clock,
      TRUE, TRUE, 0);

  self->priv->label = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_clock),
      self->priv->label);

  /* TODO: work out how to fix the padding properly. this is added to
   * fix the broken alignment where the clock appears to the right. */
  gtk_box_pack_start (GTK_BOX (box), gtk_revealer_new (), TRUE, TRUE, 0);

  setup_mixer (self);
  
  
  
  volume_idle_cb(self);  
  printf("%f is value \n", gtk_range_get_value ( GTK_RANGE (self->priv->volume_scale) ));
  
  volume_changed_cb(GTK_RANGE (self->priv->volume_scale), self);
  g_idle_add (volume_startup_panel_cb, self);

  wall_clock_notify_cb (self);
  
  global_clock = self;
}

static void
wayward_clock_dispose (GObject *object)
{
  WaywardClock *self = WAYWARD_CLOCK (object);

  //g_clear_object (&self->priv->wall_clock);

  if (self->priv->mixer_handle != NULL)
    snd_mixer_close (self->priv->mixer_handle);
  self->priv->mixer_handle = NULL;
  self->priv->mixer = NULL;

  G_OBJECT_CLASS (wayward_clock_parent_class)->dispose (object);
}

static void
wayward_clock_class_init (WaywardClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = wayward_clock_constructed;
  object_class->dispose = wayward_clock_dispose;

  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_STRING);

  g_type_class_add_private (object_class, sizeof (WaywardClockPrivate));
}

GtkWidget *
wayward_clock_new (void)
{
  return g_object_new (WAYWARD_CLOCK_TYPE,
      NULL);
}

void
wayward_clock_show_section (WaywardClock *self,
    WaywardClockSection section)
{
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_clock),
      section == WAYWARD_CLOCK_SECTION_CLOCK);

  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_system),
      section == WAYWARD_CLOCK_SECTION_SYSTEM);

  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_volume),
      section == WAYWARD_CLOCK_SECTION_VOLUME);
}
