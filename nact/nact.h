/*
 * Nautilus Actions
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   and many others (see AUTHORS)
 *
 * pwi 2009-05-17 make the source ansi-compliant
 */

#ifndef __NACT_H__
#define __NACT_H__

#include <gtk/gtk.h>

enum {
	MENU_ICON_COLUMN = 0,
	MENU_LABEL_COLUMN,
	UUID_COLUMN,
	N_COLUMN
};

void nact_fill_actions_list (GtkWidget *list);

void dialog_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data);
void add_button_clicked_cb (GtkButton *button, gpointer user_data);
void delete_button_clicked_cb (GtkButton *button, gpointer user_data);
void duplicate_button_clicked_cb (GtkButton *button, gpointer user_data);
void edit_button_clicked_cb (GtkButton *button, gpointer user_data);
void im_export_button_clicked_cb (GtkButton *button, gpointer user_data);

#endif /* __NACT_H__ */
