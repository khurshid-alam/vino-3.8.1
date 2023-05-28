/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *      Mark McLoughlin <mark@skynet.ie>
 */

#include <config.h>
#ifdef VINO_HAVE_DAMAGE
#include <X11/extensions/Xfixes.h>
#include <gdk/gdkx.h>
#endif /* VINO_HAVE_DAMAGE */

#include "vino-cursor.h"

struct _VinoCursorData
{
  GdkScreen  *screen;

  guint       update_timeout;

  int         x;
  int         y;

  guint       cursor_has_changed : 1;
};


#define VINO_CURSOR_WIDTH  19
#define VINO_CURSOR_HEIGHT 19

static const char *vino_cursor_source =
"                   "
" x                 "
" xx                "
" xxx               "
" xxxx              "
" xxxxx             "
" xxxxxx            "
" xxxxxxx           "
" xxxxxxxx          "
" xxxxxxxxx         "
" xxxxxxxxxx        "
" xxxxx             "
" xx xxx            "
" x  xxx            "
"     xxx           "
"     xxx           "
"      xxx          "
"      xxx          "
"                   ";

static const char *vino_cursor_mask =
"xx                 "
"xxx                "
"xxxx               "
"xxxxx              "
"xxxxxx             "
"xxxxxxx            "
"xxxxxxxx           "
"xxxxxxxxx          "
"xxxxxxxxxx         "
"xxxxxxxxxxx        "
"xxxxxxxxxxxx       "
"xxxxxxxxxx         "
"xxxxxxxx           "
"xxxxxxxx           "
"xx  xxxxx          "
"    xxxxx          "
"     xxxxx         "
"     xxxxx         "
"      xxx          ";


static gboolean
vino_cursor_update_timeout (VinoCursorData *data)
{
  GdkScreen *tmp_screen;
  int        tmp_x, tmp_y;
  GdkDeviceManager *dm;

  data->x = 0;
  data->y = 0;

  tmp_screen = NULL;
  dm = gdk_display_get_device_manager (gdk_screen_get_display (data->screen));
  gdk_device_get_position (gdk_device_manager_get_client_pointer (dm),
                           &tmp_screen,
                           &tmp_x,
                           &tmp_y);
  if (data->screen == tmp_screen)
    {
      data->x = tmp_x;
      data->y = tmp_y;
    }

  data->cursor_has_changed = TRUE;
  return TRUE;
}

VinoCursorData *
vino_cursor_init (GdkScreen *screen)
{
  VinoCursorData *data;

  g_return_val_if_fail (screen != NULL, NULL);

  data = g_new0 (VinoCursorData, 1);

  data->screen = screen;

  data->update_timeout = g_timeout_add (50,
					(GSourceFunc) vino_cursor_update_timeout,
					data);

  data->cursor_has_changed = TRUE;

  vino_cursor_update_timeout (data);

  return data;
}

void
vino_cursor_finalize (VinoCursorData *data)
{
  g_return_if_fail (data != NULL);

  if (data->update_timeout)
    g_source_remove (data->update_timeout);
  data->update_timeout = 0;

  g_free (data);
}

void
vino_cursor_get_position (VinoCursorData *data,
			  int            *x,
			  int            *y)
{
  g_return_if_fail (data != NULL);

  if (x)
    *x = data->x;
  if (y)
    *y = data->y;
}

#ifdef VINO_HAVE_DAMAGE
static void
argbdata_to_cursor_source (long *argb_data, int len, char **cursor_source)
{
  int i = 0;
  char *p = g_new(char, len);

  *cursor_source = p;
  while (i < len) {
    if (((argb_data[i]>>24 & 0xff) == 0xff) &&
        (argb_data[i] & 0x80) == 0)
      *p++ = 'x';
    else
      *p++ = ' ';
    i++;
  }
}
#endif /* VINO_HAVE_DAMAGE */

gboolean
vino_cursor_get_x_source (VinoCursorData  *data,
			  int             *width,
			  int             *height,
			  const char     **cursor_source,
			  const char     **cursor_mask)
{
#ifdef VINO_HAVE_DAMAGE
  int event, error;
  Display *display = GDK_SCREEN_XDISPLAY(data->screen);
#endif /* VINO_HAVE_DAMAGE */

  g_return_val_if_fail (data != NULL, FALSE);

  if (!data->cursor_has_changed)
    return FALSE;

#ifdef VINO_HAVE_DAMAGE
  if (XFixesQueryExtension (display, &event, &error))
  {
    static char *cursor_source_from_xfixes = NULL;

    XFixesCursorImage *cur = XFixesGetCursorImage (display);
    if (cursor_source_from_xfixes)
      g_free (cursor_source_from_xfixes);
    argbdata_to_cursor_source ((long *)cur->pixels, cur->width*cur->height, &cursor_source_from_xfixes);
    if (width)
      *width = cur->width;
    if (height)
      *height = cur->height;
    if (cursor_source)
      *cursor_source = cursor_source_from_xfixes;
    if (cursor_mask)
      *cursor_mask = NULL;
    data->x = cur->x - cur->xhot;
    data->y = cur->y - cur->yhot;
    XFree (cur);
  } else {
#else /* !VINO_HAVE_DAMAGE */
  {
#endif /* VINO_HAVE_DAMAGE */
    if (width)
      *width = VINO_CURSOR_WIDTH;
    if (height)
      *height = VINO_CURSOR_HEIGHT;
    if (cursor_source)
      *cursor_source = vino_cursor_source;
    if (cursor_mask)
      *cursor_mask = vino_cursor_mask;
  }

  data->cursor_has_changed = FALSE;

  return TRUE;
}
		       
