/*
 * * Copyright (C) 2017-2019 varmd
 * Copyright (C) 2013-2014 Collabora Ltd.
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "config.h"

#include "launcher.h"

#include "clock.h"
#include "panel.h"
#include "shell-app-system.h"

enum {
  PROP_0,
  PROP_BACKGROUND,
};

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

extern int global_desktop_width; //defined in wayward.c

GtkWidget *first_button = NULL;
GtkWidget *first_child = NULL;

struct WaywardLauncherPrivate {
  /* background widget so we know the output size */
  GtkWidget *background;
  ShellAppSystem *app_system;
  GtkWidget *scrolled_window;
  GtkWidget *grid;
};

G_DEFINE_TYPE_WITH_PRIVATE(WaywardLauncher, wayward_launcher, GTK_TYPE_WINDOW)

/* each grid item is 114x114 */
#define GRID_ITEM_WIDTH 134
#define GRID_ITEM_HEIGHT 134

static void
wayward_launcher_init (WaywardLauncher *self)
{
  self->priv = wayward_launcher_get_instance_private (self);
  
}

static gint
sort_apps (gconstpointer a,
    gconstpointer b)
{
  GAppInfo *info1 = G_APP_INFO (a);
  GAppInfo *info2 = G_APP_INFO (b);
  gchar *s1, *s2;
  gint ret;

  s1 = g_utf8_casefold (g_app_info_get_display_name (info1), -1);
  s2 = g_utf8_casefold (g_app_info_get_display_name (info2), -1);

  ret = g_strcmp0 (s1, s2);

  g_free (s1);
  g_free (s2);

  return ret;
}

static gboolean
get_child_position_cb (GtkOverlay *overlay,
    GtkWidget *widget,
    GdkRectangle *allocation,
    gpointer user_data)
{
  GtkOverlayClass *klass = GTK_OVERLAY_GET_CLASS (overlay);

  klass->get_child_position (overlay, widget, allocation);

  /* use the same valign and halign properties, but given we have a
   * border of 1px, respect it and don't draw the overlay widget over
   * the border. */
  allocation->x += 1;
  allocation->y -= 1;
  allocation->width -= 2;

  return TRUE;
}

static gboolean
app_launched_idle_cb (gpointer data)
{
  WaywardLauncher *self = data;
  GtkAdjustment *adjustment;

  /* make the scrolled window go back to the top */

  adjustment = gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (self->priv->scrolled_window));

  gtk_adjustment_set_value (adjustment, 0.0);

  return G_SOURCE_REMOVE;
}


static void
clicked_cb (GtkWidget *widget,
    GDesktopAppInfo *info)
{
  WaywardLauncher *self;

  g_app_info_launch (G_APP_INFO (info), NULL, NULL, NULL);


  self = g_object_get_data (G_OBJECT (widget), "launcher");
  g_assert (self);
  g_signal_emit (self, signals[APP_LAUNCHED], 0);

  
  // do this in an idle so it's not done so obviously onscreen 
  g_idle_add (app_launched_idle_cb, self);
}






static void
ebox_clicked_cb (GtkWidget *widget,
    GdkEventButton *event,
    GDesktopAppInfo *info)
{

  WaywardLauncher *self;

  g_app_info_launch (G_APP_INFO (info), NULL, NULL, NULL);


  self = g_object_get_data (G_OBJECT (widget), "launcher");
  g_assert (self);
  g_signal_emit (self, signals[APP_LAUNCHED], 0);

  
  /* do this in an idle so it's not done so obviously onscreen */
  g_idle_add (app_launched_idle_cb, self);
}


static void
ebox_key_released_cb (GtkWidget *widget,
    GdkEventKey *event,
    GDesktopAppInfo *info)
{
 


  /*
  FILE *f;
  f = fopen("/tmp/x.log", "a+");
  fprintf(f, "Key released is %x \n ", event->keyval & 0xff);
  fclose(f);
  */

  if (event->keyval && event->keyval == GDK_KEY_ISO_Enter)
  {
    ebox_clicked_cb(widget, NULL, info);
  }
}

static gboolean
app_enter_cb (GtkWidget *widget,
    GdkEvent *event,
    GtkWidget *revealer)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
  return FALSE;
}

static gboolean
app_leave_cb (GtkWidget *widget,
    GdkEvent *event,
    GtkWidget *revealer)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
  return FALSE;
}




static GtkWidget *
app_launcher_new_from_desktop_info (WaywardLauncher *self,
    GDesktopAppInfo *info)
{
  GIcon *icon;
  GtkWidget *alignment;
  GtkWidget *image;
  GtkWidget *button;
  GtkWidget *overlay;
  GtkWidget *revealer;
  GtkWidget *label;
  GtkWidget *ebox;
  GtkStyle *style;

  /* we need an ebox to catch enter and leave events */
  ebox = gtk_event_box_new ();
  gtk_event_box_set_visible_window( GTK_EVENT_BOX(ebox) , FALSE);

//TODO check if no keyboard
//  gtk_window_set_focus_visible(GTK_WINDOW(ebox), FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (ebox),
      "wayward-grid-item");

  /* we use an overlay so we can have the app icon showing but use a
   * GtkRevealer to show a label of the app's name. */
  overlay = gtk_overlay_new ();
  
    gtk_style_context_add_class (gtk_widget_get_style_context (overlay),
      "wayward-grid-overlay");
  
  gtk_container_add (GTK_CONTAINER (ebox), overlay);

  /* ...but each item has a border of 1px and we don't want the
   * revealer to paint into the border, so overload this function to
   * know where to put it. */
//  g_signal_connect (overlay, "get-child-position",
//      G_CALLBACK (get_child_position_cb), NULL);

  revealer = gtk_revealer_new ();
  g_object_set (revealer,
      "halign", GTK_ALIGN_FILL, /* all the width */
      "valign", GTK_ALIGN_END, /* only at the bottom */
      NULL);
  gtk_revealer_set_transition_type (GTK_REVEALER (revealer),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), revealer);

  /* app name */
  /*
  label = gtk_label_new (g_app_info_get_display_name (G_APP_INFO (info)));
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_style_context_add_class (gtk_widget_get_style_context (label), "wayward-grid-label");
  gtk_container_add (GTK_CONTAINER (revealer), label);
  */
  /* icon button to load the app */
  //alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
  alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_container_add (GTK_CONTAINER (overlay), alignment);

  icon = g_app_info_get_icon (G_APP_INFO (info));
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
//  gtk_image_set_pixel_size(image, 64);
  gtk_style_context_add_class (gtk_widget_get_style_context (image),
      "wayward-grid-item-image");
  
  button = gtk_button_new ();

  //TODO check if keyboard works after commenting
  //gtk_window_set_focus_visible(GTK_WINDOW(button), FALSE);
  //gtk_widget_set_can_focus(button, TRUE);
  //if(!first_button) {
  //  first_button = button;
  //}


  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

  
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
      "wayward-grid-item-button");
  

  gtk_style_context_remove_class (
      gtk_widget_get_style_context (button),
      "button");
  
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (button),
      "image-button");

  gtk_button_set_image (GTK_BUTTON (button), image);
  
  
  g_object_set (image,
      "margin", 50,
      NULL);



//  gtk_container_add (GTK_CONTAINER (alignment), image);
  gtk_container_add (GTK_CONTAINER (alignment), button);

  /* TODO: a bit ugly */
  g_object_set_data (G_OBJECT (button), "launcher", self);
  g_signal_connect (button, "clicked", G_CALLBACK (clicked_cb), info);
  g_signal_connect (button, "key-release-event", G_CALLBACK (ebox_key_released_cb), info);

  g_object_set_data (G_OBJECT (ebox), "launcher", self);
  //g_signal_connect (ebox, "button-press-event", G_CALLBACK (ebox_clicked_cb), info);
    //g_signal_connect (ebox, "key-release-event", G_CALLBACK (ebox_key_released_cb), info);


  /* now we have set everything up, we can refernce the ebox and the
   * revealer. enter will show the label and leave will hide the label. */
  g_signal_connect (ebox, "enter-notify-event", G_CALLBACK (app_enter_cb), revealer);
  g_signal_connect (ebox, "leave-notify-event", G_CALLBACK (app_leave_cb), revealer);

  return ebox;
}


static int global_menu_done = 0;

//Populate grid entries using entries from cached file
//Removed dependency on gnome-desktop
static void
installed_changed_cb (void *app_system, WaywardLauncher *self)
{
  
  if(global_menu_done) {
    return;  
  }
  
  //GHashTable *entries = shell_app_system_get_entries (app_system);
  GList *l, *values;
  GHashTable *entries;
  
  GList *menu_values = NULL;

  gint output_width, output_height;
  guint cols;
  guint left, top;
  
  
  /*
  char buffer[100];
FILE *fp = fopen("filename", "r");                 // do not use "rb"
while (fgets(buffer, sizeof(buffer), fp)) {
... do something
}
fclose(fp);
*/
  
  
  /* remove all children first */
  gtk_container_foreach (GTK_CONTAINER (self->priv->grid),
      (GtkCallback) gtk_widget_destroy, NULL);

  //values = g_hash_table_get_values (entries);
  //values = g_list_sort (values, sort_apps);

  //wayward_launcher_calculate (self, NULL, NULL, &cols);
  //cols--; /* because we start from zero here */

  

  FILE* f = fopen("/tmp/x.log", "a+");
  
  //Run script to get cached entries
  //TODO use C instead
  if(system ("/usr/bin/sh /usr/lib/weston/wayward-lsdesktopf" )) {
  //if(execl ("/usr/bin/echo", "/usr/bin/echo", " ddd ",  (char*) NULL ) ) {
    fprintf(f, "error occured %s", strerror(errno)); 
    fclose(f);
    return;      
  }
  
  
  char path_buf[1256];
  snprintf(path_buf, sizeof path_buf, "%s/.cache/wayward-menus", getenv("HOME"));
  
  fprintf(f, "%s", path_buf); 
  
  
  
  
   FILE* file = fopen(path_buf, "a+");
   
  
      
    //FILE *f;
    if(file) {
      char * line = NULL;
      //char path_line[256];
      size_t len = 0;
      ssize_t read;
      int array_size = 1;
      //while (fgets(path_line, sizeof(path_line), file)) {
      while ((read = getline(&line, &len, file)) != -1) {
        
        
        
        
        if(line) {
          line[strcspn(line, "\n")] = 0;          
        }
        
        //GDesktopAppInfo *info = G_DESKTOP_APP_INFO (l->data);
        GDesktopAppInfo *info = g_desktop_app_info_new_from_filename (line);
        
          if(info && !g_desktop_app_info_get_is_hidden(info) && !g_desktop_app_info_get_nodisplay(info) ) {
          
            menu_values = g_list_append(menu_values, info);
            array_size++;
          }
        } //end while
        
        printf("%d \n", array_size); 
        wayward_launcher_calculate (self, NULL, NULL, &cols, array_size);
        cols--; /* because we start from zero here */
        left = top = 0;
        
        for (l = menu_values; l; l = l->next)
        {
          GDesktopAppInfo *info = G_DESKTOP_APP_INFO (l->data);
          GtkWidget *app = app_launcher_new_from_desktop_info (self, info);
          

          gtk_grid_attach (GTK_GRID (self->priv->grid), app, left++, top, 1, 1);

          if (left > cols)
            {
              left = 0;
              top++;
            }
        }

          g_list_free (values);

          gtk_widget_show_all (self->priv->grid);
    }
        
        
    
    

    fclose(file);
    fclose(f);
      
    gtk_widget_show_all (self->priv->grid);
    
    global_menu_done = 1;
    
    
    g_list_free (values);
    
    return;
    
    
  
  //./lsdesktopf | sort -t ' ' -k 1,1 -u | sed -n -r -e 's|.*(/usr/share/applications/.*desktop).*$|\1|p'
  
  for (l = values; l; l = l->next)
    {
      GDesktopAppInfo *info = G_DESKTOP_APP_INFO (l->data);
      
      GtkWidget *app = app_launcher_new_from_desktop_info (self, info);
      

      gtk_grid_attach (GTK_GRID (self->priv->grid), app, left++, top, 1, 1);

      if (left > cols)
        {
          left = 0;
          top++;
        }
    }

  g_list_free (values);

  gtk_widget_show_all (self->priv->grid);
}

static void
background_size_allocate_cb (GtkWidget *widget,
    GdkRectangle *allocation,
    WaywardLauncher *self)
{
  //installed_changed_cb (self->priv->app_system, self);
}

int focus_set = 0;
static gboolean launcher_grid_key_pressed_cb (GtkWidget *widget, GdkEventKey *event, gpointer data) {
  
  WaywardLauncher *self = data;
  
  FILE *f;
  f = fopen("/tmp/x.log", "a+");

    fprintf ( f, " Key was pressed \n");
	

  fclose(f);
  

  if(first_button && !focus_set) {

    //gtk_container_set_focus_child(GTK_CONTAINER(self->priv->grid), first_button);
    
    //gtk_window_set_focus_visible(GTK_WINDOW(self->priv->grid), TRUE);
//    gtk_widget_grab_focus (GTK_WIDGET(first_button));
//    gtk_window_activate_focus (GTK_WINDOW(self->priv->grid));
//    gtk_widget_grab_focus (GTK_WIDGET(first_button));
    //gtk_widget_grab_focus (first_button);
    focus_set = 1;
  }
  return FALSE;
}


static gboolean launcher_key_pressed_cb (GtkWidget *widget, GdkEventKey *event, gpointer data) {

  WaywardLauncher *self = data;
  if(first_button && !focus_set) {
    
    gtk_window_set_focus_visible(GTK_WINDOW(self), FALSE);
    gtk_window_set_focus_visible(GTK_WINDOW(self->priv->grid), FALSE);
    gtk_window_set_focus_visible(GTK_WINDOW(self->priv->scrolled_window), FALSE);
//    gtk_widget_grab_focus (GTK_WIDGET(first_button));
//    gtk_window_activate_focus (GTK_WINDOW(self->priv->grid));
//    gtk_widget_grab_focus (GTK_WIDGET(first_button));
    gtk_widget_grab_focus (GTK_WIDGET(self->priv->grid));
    gtk_window_activate_focus (GTK_WINDOW(self->priv->grid));
    focus_set = 1;
      
  }
  return FALSE;
}

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
wayward_launcher_constructed (GObject *object)
{
  WaywardLauncher *self = WAYWARD_LAUNCHER (object);

  G_OBJECT_CLASS (wayward_launcher_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "wayward");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_window_set_focus_visible(GTK_WINDOW(self), FALSE);

  gtk_widget_realize (GTK_WIDGET (self));

  /* make it black and slightly alpha */
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wayward-grid");

  /* scroll it */
  self->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (self), self->priv->scrolled_window);

//  gtk_window_set_focus_visible(GTK_WINDOW(self->priv->scrolled_window), TRUE);
  self->priv->grid = gtk_grid_new ();
  //gtk_container_add (GTK_CONTAINER (self), self->priv->grid);
   gtk_container_add (GTK_CONTAINER (self->priv->scrolled_window), self->priv->grid);
/*
  

  GtkWidget *button;




  button = gtk_button_new_with_label ("Button 1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);


  gtk_grid_attach (GTK_GRID (self->priv->grid), button, 0, 0, 1, 1);

  button = gtk_button_new_with_label ("Button 2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);


  gtk_grid_attach (GTK_GRID (self->priv->grid), button, 1, 0, 1, 1);

  button = gtk_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (print_hello), NULL);

  gtk_grid_attach (GTK_GRID (self->priv->grid), button, 0, 1, 2, 1);
  

//  gtk_widget_set_can_focus(GTK_WIDGET(self->priv->scrolled_window), TRUE);
//  gtk_widget_set_can_focus(self->priv->grid, TRUE);
*/

  

    //gtk_container_add (GTK_CONTAINER (self), self->priv->grid);


 

  g_signal_connect (GTK_WIDGET (self->priv->scrolled_window), "key_press_event",
      G_CALLBACK (launcher_key_pressed_cb), self);

//  g_signal_connect (GTK_WIDGET (self->priv->grid), "key_press_event",
//      G_CALLBACK (launcher_grid_key_pressed_cb), self);

  /* fill the grid with apps */

  /*
  self->priv->app_system = shell_app_system_get_default ();
  g_signal_connect (self->priv->app_system, "installed-changed",
      G_CALLBACK (installed_changed_cb), self);
  */
  self->priv->app_system = NULL;

  /* refill the grid if the background is changed */

  g_assert (self->priv->background != NULL);
  g_signal_connect (self->priv->background, "size-allocate",
      G_CALLBACK (background_size_allocate_cb), self);

  /* now actually fill the grid */

  installed_changed_cb (self->priv->app_system, self);
}

static void
wayward_launcher_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  WaywardLauncher *self = WAYWARD_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_BACKGROUND:
        g_value_set_object (value, self->priv->background);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
wayward_launcher_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  WaywardLauncher *self = WAYWARD_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_BACKGROUND:
        self->priv->background = g_value_get_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
wayward_launcher_class_init (WaywardLauncherClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = wayward_launcher_constructed;
  object_class->get_property = wayward_launcher_get_property;
  object_class->set_property = wayward_launcher_set_property;

  g_object_class_install_property (object_class, PROP_BACKGROUND,
      g_param_spec_object ("background",
          "background",
          "The #GtkWidget of the background of the output",
          GTK_TYPE_WIDGET,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (WaywardLauncherPrivate));
}

GtkWidget *
wayward_launcher_new (GtkWidget *background_widget)
{
  return g_object_new (WAYWARD_LAUNCHER_TYPE,
      "background", background_widget,
      NULL);
}

void
wayward_launcher_calculate (WaywardLauncher *self,
    gint *grid_window_width,
    gint *grid_window_height,
    gint *grid_cols,
    int apps_size
    )
{
  gint output_width, output_height, panel_height;
  gint usable_width, usable_height;
  guint cols, rows;
  guint num_apps;
  guint scrollbar_width = 13;


  //Below is not working, using global instead
  //gtk_widget_get_size_request (self->priv->background->window, &output_width, &output_height);
  //panel_height = output_height * WAYWARD_PANEL_HEIGHT_RATIO;

  
  /*
  FILE *f;
  f = fopen("/tmp/x.log", "a+");
  fprintf ( f, "Background %d \n", global_desktop_width);
	fclose(f);
  */
  
  if(global_desktop_width > 1920) {
    *grid_cols = 5;
  } else if(global_desktop_width > 1600)  {
    *grid_cols = 4;
  } else {
    *grid_cols = 3;
  }
  return;
  
  

  usable_width = output_width - WAYWARD_PANEL_WIDTH - scrollbar_width;
  /* don't go further down than the panel */
  //usable_height = panel_height - WAYWARD_CLOCK_HEIGHT;
  usable_height = panel_height;

  /* try and fill half the screen, otherwise round down */
  cols = (int) ((usable_width / 2.0) / GRID_ITEM_WIDTH) - 2;
  /* try to fit as many rows as possible in the panel height we have */
  rows = (int) (usable_height / GRID_ITEM_HEIGHT) - 2;

  /* we don't need to include the scrollbar if we already have enough
   * space for all the apps. */
  /* 
  num_apps = g_hash_table_size (
      shell_app_system_get_entries (self->priv->app_system));
  */
  num_apps = apps_size;
      
  if ((cols * rows) >= (num_apps))
    scrollbar_width = 0;

  /* done! */
  
  if(rows < 2) {
    rows = 2;
  }

  if (grid_window_width)
    *grid_window_width = (cols * GRID_ITEM_WIDTH) + scrollbar_width;
  
  
  if(grid_window_width && *grid_window_width < 400) {
      *grid_window_width = 450;
  }

  if (grid_window_height)
    *grid_window_height = (rows * GRID_ITEM_HEIGHT) + 48;
  

  if (grid_cols) {
    *grid_cols = cols;
    if(*grid_cols <= 2) {
      *grid_cols = 3;
    }
    if(output_width > 1599) {
      *grid_cols = 4;
    }
  }  
}
