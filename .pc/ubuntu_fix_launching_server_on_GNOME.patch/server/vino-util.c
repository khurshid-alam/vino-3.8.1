/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2006-2010 Jonh Wendell <wendell@bani.com.br>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *      Jonh Wendell <wendell@bani.com.br>
 *      Mark McLoughlin <mark@skynet.ie>
 */

#include "config.h"

#include "vino-util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <gtk/gtk.h>

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#else
#include "libvncserver/ifaddr/ifaddrs.h"
#endif

#ifdef RFC2553
#define ADDR_FAMILY_MEMBER ss_family
#else
#define ADDR_FAMILY_MEMBER sa_family
#endif

#ifdef GNOME_ENABLE_DEBUG
VinoDebugFlags _vino_debug_flags = VINO_DEBUG_NONE;

void
vino_setup_debug_flags (void)
{
  const char       *env_str;
  static GDebugKey  debug_keys [] =
    {
      { "polling", VINO_DEBUG_POLLING },
      { "rfb",     VINO_DEBUG_RFB },
      { "input",   VINO_DEBUG_INPUT },
      { "prefs",   VINO_DEBUG_PREFS },
      { "tls",     VINO_DEBUG_TLS },
      { "mdns",    VINO_DEBUG_MDNS },
      { "prompt",  VINO_DEBUG_PROMPT },
      { "dbus",    VINO_DEBUG_DBUS },
      { "upnp",    VINO_DEBUG_UPNP },
      { "tube",    VINO_DEBUG_TUBE }
    };
  
  env_str = g_getenv ("VINO_SERVER_DEBUG");
  
  if (env_str)
    _vino_debug_flags |= g_parse_debug_string (env_str,
					       debug_keys,
					       G_N_ELEMENTS (debug_keys));
}
#endif /* GNOME_ENABLE_DEBUG */

static struct VinoStockItem
{
  char *stock_id;
  char *stock_icon_id;
  char *label;
} vino_stock_items [] = {
  { VINO_STOCK_ALLOW,  GTK_STOCK_OK,     N_("_Allow") },
  { VINO_STOCK_REFUSE, GTK_STOCK_CANCEL, N_("_Refuse") },
};

void
vino_init_stock_items (void)
{
  static gboolean  initialized = FALSE;
  GtkIconFactory  *factory;
  GtkStockItem    *items;
  int              i;

  if (initialized)
    return;

  factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (factory);

  items = g_new (GtkStockItem, G_N_ELEMENTS (vino_stock_items));

  for (i = 0; i < G_N_ELEMENTS (vino_stock_items); i++)
    {
      GtkIconSet *icon_set;

      items [i].stock_id           = g_strdup (vino_stock_items [i].stock_id);
      items [i].label              = g_strdup (vino_stock_items [i].label);
      items [i].modifier           = 0;
      items [i].keyval             = 0;
      items [i].translation_domain = g_strdup (GETTEXT_PACKAGE);

      /* FIXME: does this take into account the theme? */
      icon_set = gtk_icon_factory_lookup_default (vino_stock_items [i].stock_icon_id);
      gtk_icon_factory_add (factory, vino_stock_items [i].stock_id, icon_set);
    }
  
  gtk_stock_add_static (items, G_N_ELEMENTS (vino_stock_items));

  g_object_unref (factory);

  initialized = TRUE;
}

void
vino_util_show_error (const gchar *title, const gchar *message, GtkWindow *parent)
{
  GtkWidget *d;
  gchar     *t;

  if (title)
    t = g_strdup (title);
  else
    t = g_strdup (_("An error has occurred:"));

  d = gtk_message_dialog_new (parent,
			      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			      GTK_MESSAGE_ERROR,
			      GTK_BUTTONS_CLOSE,
			      "%s",
			      t);
  g_free (t);

  if (message)
    gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (d),
					      "%s",
					      message);

  g_signal_connect_swapped (d,
			    "response", 
			    G_CALLBACK (gtk_widget_destroy),
			    d);
  gtk_widget_show_all (GTK_WIDGET(d));
}

gchar *
vino_util_get_local_hostname (const gchar *server_iface)
{
  char                *retval, buf[INET6_ADDRSTRLEN];
  struct ifaddrs      *myaddrs, *ifa;
  void                *sin;
  GHashTable          *ipv4, *ipv6;
  GHashTableIter      iter;
  gpointer            key, value;

  retval = NULL;
  ipv4 = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  ipv6 = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);

  getifaddrs (&myaddrs);
  for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
      if (ifa->ifa_addr == NULL || ifa->ifa_name == NULL || (ifa->ifa_flags & IFF_UP) == 0)
        continue;

      switch (ifa->ifa_addr->ADDR_FAMILY_MEMBER)
        {
          case AF_INET:
            sin = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop (AF_INET, sin, buf, INET6_ADDRSTRLEN);
            g_hash_table_insert (ipv4,
                                 ifa->ifa_name,
                                 g_strdup (buf));
            break;

          case AF_INET6:
            sin = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            inet_ntop (AF_INET6, sin, buf, INET6_ADDRSTRLEN);
            g_hash_table_insert (ipv6,
                                 ifa->ifa_name,
                                 g_strdup (buf));
            break;
          default: continue;
        }
    }

  if (server_iface && server_iface[0] != '\0')
    {
      if ((retval = g_strdup (g_hash_table_lookup (ipv4, server_iface))))
        goto the_end;
      if ((retval = g_strdup (g_hash_table_lookup (ipv6, server_iface))))
        goto the_end;
    }

  g_hash_table_iter_init (&iter, ipv4);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (strncmp (key, "lo", 2) == 0)
        continue;
      retval = g_strdup (value);
      goto the_end;
    }

  g_hash_table_iter_init (&iter, ipv6);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (strncmp (key, "lo", 2) == 0)
        continue;
      retval = g_strdup (value);
      goto the_end;
    }

  if ((retval = g_strdup (g_hash_table_lookup (ipv4, "lo"))))
    goto the_end;
  if ((retval = g_strdup (g_hash_table_lookup (ipv6, "lo"))))
    goto the_end;

  the_end:
  freeifaddrs (myaddrs);
  g_hash_table_destroy (ipv4);
  g_hash_table_destroy (ipv6);

  return retval;
}
