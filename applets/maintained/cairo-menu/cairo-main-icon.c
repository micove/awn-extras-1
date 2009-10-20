/*
 * Copyright (C) 2007, 2008, 2009 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
*/

/* cairo-menu-main-icon.c */

#include <gtk/gtk.h>
#include <libawn/awn-themed-icon.h>
#include "cairo-main-icon.h"
#include "cairo-menu.h"
#include "cairo-menu-applet.h"
#include "gnome-menu-builder.h"
#include "misc.h"
#include "config.h"

extern MenuBuildFunc  menu_build;

G_DEFINE_TYPE (CairoMainIcon, cairo_main_icon, AWN_TYPE_THEMED_ICON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_CAIRO_MAIN_ICON, CairoMainIconPrivate))

typedef struct _CairoMainIconPrivate CairoMainIconPrivate;

struct _CairoMainIconPrivate {
  DEMenuType   menu_type;
  GtkWidget   *menu;
  AwnApplet   * applet;
  MenuInstance * menu_instance;
  GtkWidget   * context_menu;
};


enum
{
  PROP_0,
  PROP_APPLET
};

static gboolean _button_clicked_event (CairoMainIcon *applet, GdkEventButton *event, gpointer null);

static const GtkTargetEntry drop_types[] =
{
  { (gchar*)"STRING", 0, 0 },
  { (gchar*)"text/plain", 0,  },
  { (gchar*)"text/uri-list", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS(drop_types);

static void
cairo_main_icon_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (object);
  
  switch (property_id) {
  case PROP_APPLET:
    g_value_set_pointer (value,priv->applet);
    break;                    
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
cairo_main_icon_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (object);
  
  switch (property_id) {
  case PROP_APPLET:
      priv->applet = g_value_get_pointer (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
cairo_main_icon_dispose (GObject *object)
{
  G_OBJECT_CLASS (cairo_main_icon_parent_class)->dispose (object);
}

static void
cairo_main_icon_finalize (GObject *object)
{
  G_OBJECT_CLASS (cairo_main_icon_parent_class)->finalize (object);
}

static void
size_changed_cb (CairoMainIcon * icon,gint size)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (icon);

  awn_themed_icon_set_size (AWN_THEMED_ICON (icon),awn_applet_get_size (priv->applet));
}

static void
cairo_main_icon_constructed (GObject *object)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (object);
  GdkPixbuf * pbuf;
  gint size = awn_applet_get_size (priv->applet);
  G_OBJECT_CLASS (cairo_main_icon_parent_class)->constructed (object);  

  awn_themed_icon_set_info_simple (AWN_THEMED_ICON(object),"cairo-menu",awn_applet_get_uid (priv->applet),"gnome-main-menu");
  awn_themed_icon_set_size (AWN_THEMED_ICON (object),size);  

  gtk_drag_dest_set (GTK_WIDGET (object),
                       GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                       drop_types, n_drop_types,
                       GDK_ACTION_COPY | GDK_ACTION_ASK);
  /* call our function in the module */

  priv->menu_instance = get_menu_instance (priv->applet,
                                          (GetRunCmdFunc)cairo_menu_applet_get_run_cmd,
                                          (GetSearchCmdFunc)cairo_menu_applet_get_search_cmd,
                                          (AddIconFunc) cairo_menu_applet_add_icon,
                                          (CheckMenuHiddenFunc) cairo_menu_applet_check_hidden_menu,
                                          NULL,
                                          0);
  priv->menu = menu_build (priv->menu_instance);
  gtk_widget_show_all (priv->menu);
  g_signal_connect(object, "button-press-event", G_CALLBACK(_button_clicked_event), NULL);
  g_signal_connect_swapped(priv->applet,"size-changed",G_CALLBACK(size_changed_cb),object);
}

static void 
cairo_main_icon_drag_data_received (GtkWidget        *widget, 
                                    GdkDragContext   *context,
                                    gint              x, 
                                    gint              y, 
                                    GtkSelectionData *sdata,
                                    guint             info,
                                    guint             evt_time)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (widget);  
  gchar           *sdata_data;
  GStrv           i;
  GStrv           tokens = NULL;  
  sdata_data = (gchar*)gtk_selection_data_get_data (sdata);


  if (strstr (sdata_data, "cairo_menu_item_dir:///"))
  {
    /*TODO move into a separate function */
    tokens = g_strsplit  (sdata_data, "\n",-1);    
    for (i=tokens; *i;i++)
    {
      GStrv           sub_tokens = NULL;
      gchar * filename = g_filename_from_uri ((gchar*) *i,NULL,NULL);
      if (!filename && ((gchar *)*i))
      {
        filename = g_strdup ((gchar*)*i);
      }
      if (filename)
      {
        g_strstrip(filename);
        sub_tokens = g_strsplit (filename,"@@@",-1);
        if (sub_tokens && g_strv_length (sub_tokens)==4)
        {
          cairo_menu_applet_add_icon (AWN_CAIRO_MENU_APPLET(priv->applet),sub_tokens[1],sub_tokens[2],sub_tokens[3]);
          gtk_drag_finish (context, TRUE, FALSE, evt_time);
          g_strfreev (sub_tokens); 
          g_strfreev (tokens);
          return;
        }
        g_strfreev (sub_tokens); 
      }
    }
    g_strfreev (tokens);
  }
  awn_themed_icon_drag_data_received (widget,context,x,y,sdata,info,evt_time);
  //gtk_drag_finish (context,TRUE,FALSE,evt_time);
}

static void
cairo_main_icon_class_init (CairoMainIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);  
  GParamSpec   *pspec;  

  object_class->get_property = cairo_main_icon_get_property;
  object_class->set_property = cairo_main_icon_set_property;
  object_class->dispose = cairo_main_icon_dispose;
  object_class->finalize = cairo_main_icon_finalize;
  object_class->constructed = cairo_main_icon_constructed;

  wid_class->drag_data_received = cairo_main_icon_drag_data_received;  

  pspec = g_param_spec_pointer ("applet",
                               "applet",
                               "AwnApplet",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_APPLET, pspec);
  
  g_type_class_add_private (klass, sizeof (CairoMainIconPrivate));
}

static void
cairo_main_icon_init (CairoMainIcon *self)
{
  CairoMainIconPrivate * priv = GET_PRIVATE (self);

  priv->menu_type = MENU_TYPE_GUESS;
  priv->context_menu = NULL;
}

GtkWidget*
cairo_main_icon_new (AwnApplet * applet)
{
  return g_object_new (AWN_TYPE_CAIRO_MAIN_ICON, 
                        "applet",applet,
                        "drag_and_drop",FALSE,
                        NULL);
}

static gboolean 
_button_clicked_event (CairoMainIcon *icon, GdkEventButton *event, gpointer null)
{
  g_return_if_fail (AWN_IS_CAIRO_MAIN_ICON(icon));
  CairoMainIconPrivate * priv = GET_PRIVATE (icon);
  
  if (event->button == 1)
  {
    gtk_menu_popup(GTK_MENU(priv->menu), NULL, NULL, NULL, NULL,
                          event->button, event->time);   
    g_object_set(awn_overlayable_get_effects (AWN_OVERLAYABLE(icon)), "depressed", FALSE,NULL);
  }
  else if (event->button == 3)
  {
    GtkWidget * item;

    if (!priv->context_menu)
    {
      priv->context_menu = awn_applet_create_default_menu (AWN_APPLET(priv->applet));
      item = gtk_menu_item_new_with_label("Preferences");
      
      gtk_widget_show(item);
      gtk_menu_set_screen(GTK_MENU(priv->context_menu), NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->context_menu), item);
//      g_signal_connect(G_OBJECT(item), "button-press-event", G_CALLBACK(_show_prefs), NULL);
      item=awn_applet_create_about_item_simple(AWN_APPLET(priv->applet),
                                               "Copyright 2007,2008, 2009 Rodney Cryderman <rcryderman@gmail.com>",
                                               AWN_APPLET_LICENSE_GPLV2,
                                               NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->context_menu), item);      
      
    }
    gtk_menu_popup(GTK_MENU(priv->context_menu), NULL, NULL, NULL, NULL,event->button, event->time);
    g_object_set(awn_overlayable_get_effects (AWN_OVERLAYABLE(icon)), "depressed", FALSE,NULL);    
  }
  return TRUE;
}

void
cairo_main_icon_refresh_menu (CairoMainIcon * icon)
{
  g_return_if_fail (AWN_IS_CAIRO_MAIN_ICON(icon));
  CairoMainIconPrivate * priv = GET_PRIVATE (icon);

  if (priv->menu && (AWN_IS_CAIRO_MENU(priv->menu)))
  {
    g_debug ("Destroying menu");
    gtk_widget_destroy (priv->menu);
  }
  priv->menu_instance->menu=NULL;
  priv->menu_instance->places=NULL;
  priv->menu_instance->recent=NULL;  
  priv->menu = menu_build (priv->menu_instance);
  gtk_widget_show_all (priv->menu);
}