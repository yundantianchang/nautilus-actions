/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/na-iprefs.h>
#include <core/na-updater.h>

#include "nact-application.h"
#include "nact-sort-buttons.h"

typedef struct {
	gchar    *btn_name;
	/*GCallback on_btn_toggled;*/
	guint     order_mode;
}
	ToggleGroup;

static gboolean st_set_sort_order = FALSE;
static gboolean st_in_toggle      = FALSE;
static gboolean st_enable_buttons = FALSE;
static gint     st_last_active    = -1;

static void enable_buttons( NactMainWindow *window );
static void on_toggle_button_toggled( GtkToggleButton *button, NactMainWindow *window );
static void set_new_sort_order( NactMainWindow *window, guint order_mode );
static void display_sort_order( NactMainWindow *window, guint order_mode );
static gint toggle_group_get_active( ToggleGroup *group, BaseWindow *window );
static gint toggle_group_get_for_mode( ToggleGroup *group, guint mode );
static void toggle_group_set_active( ToggleGroup *group, BaseWindow *window, gint idx );
static gint toggle_group_get_from_button( ToggleGroup *group, BaseWindow *window, GtkToggleButton *toggled_button );

static ToggleGroup st_toggle_group [] = {
		{ "SortManualButton", IPREFS_ORDER_MANUAL },
		{ "SortUpButton",     IPREFS_ORDER_ALPHA_ASCENDING },
		{ "SortDownButton",   IPREFS_ORDER_ALPHA_DESCENDING },
		{ NULL }
};

/**
 * nact_sort_buttons_runtime_init:
 * @window: the #NactMainWindow.
 *
 * Initialization of the UI each time it is displayed.
 *
 * At end, buttons are all :
 * - off,
 * - connected,
 * - enabled (sensitive) if level zero is writable
 */
void
nact_sort_buttons_runtime_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_runtime_init";
	GtkToggleButton *button;
	gint i;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	i = 0;
	while( st_toggle_group[i].btn_name ){

		button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( window ), st_toggle_group[i].btn_name ));
		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( button ),
				"toggled",
				G_CALLBACK( on_toggle_button_toggled ));

		i += 1;
	}

	enable_buttons( window );
}

/**
 * nact_sort_buttons_all_widgets_showed:
 * @window: the #NactMainWindow.
 *
 * Called when all the widgets are showed after end of all runtime
 * initializations.
 */
void
nact_sort_buttons_all_widgets_showed( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_all_widgets_showed";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	st_set_sort_order = TRUE;
}

/**
 * nact_sort_buttons_enable_buttons:
 * @window: the #NactMainWindow.
 * @enable: whether we wish enable or disable these sort buttons.
 *
 * Enable or disable the sort buttons, while keeping relevant with
 * writability status of the various stages.
 *
 * This function may be called when application wishes enable or disable
 * the sort buttons, typically when there is no record to sort.
 */
void
nact_sort_buttons_enable_buttons( NactMainWindow *window, gboolean enable )
{
	static const gchar *thisfn = "nact_sort_buttons_enable_buttons";

	g_debug( "%s: window=%p, enable=%s", thisfn, ( void * ) window, enable ? "True":"False" );

	st_enable_buttons = enable;
	enable_buttons( window );
}

/**
 * nact_sort_buttons_dispose:
 * @window: the #NactMainWindow.
 *
 * The main window is disposed.
 */
void
nact_sort_buttons_dispose( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_dispose";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
}

/**
 * nact_sort_buttons_display_order_change:
 * @window: the #NactMainWindow.
 * @order_mode: the new order mode.
 *
 * Relayed via NactMainWindow, this is a NAIPivotConsumer notification.
 */
void
nact_sort_buttons_display_order_change( NactMainWindow *window, guint order_mode )
{
	static const gchar *thisfn = "nact_sort_buttons_display_order_change";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	display_sort_order( window, order_mode );
}

/**
 * nact_sort_buttons_level_zero_writability_change:
 * @window: the #NactMainWindow.
 *
 * Relayed via NactMainWindow, this is a NAIPivotConsumer notification.
 */
void
nact_sort_buttons_level_zero_writability_change( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_level_zero_writability_change";

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	enable_buttons( window );
}

static void
enable_buttons( NactMainWindow *window )
{
	NactApplication *application;
	NAUpdater *updater;
	gboolean writable;
	GtkWidget *button;
	gint i;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	writable = na_iprefs_is_level_zero_writable( NA_PIVOT( updater ));

	i = 0;
	while( st_toggle_group[i].btn_name ){
		button = base_window_get_widget( BASE_WINDOW( window ), st_toggle_group[i].btn_name );
		gtk_widget_set_sensitive( button, writable && st_enable_buttons );
		i += 1;
	}
}

static void
on_toggle_button_toggled( GtkToggleButton *toggled_button, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_sort_buttons_on_toggle_button_toggled";
	gint ibtn, iprev;

	if( !st_in_toggle ){

		ibtn = toggle_group_get_from_button( st_toggle_group, BASE_WINDOW( window ), toggled_button );
		g_return_if_fail( ibtn >= 0 );

		iprev = toggle_group_get_active( st_toggle_group, BASE_WINDOW( window ));

		g_debug( "%s: iprev=%d, ibtn=%d", thisfn, iprev, ibtn );

		if( iprev == ibtn ){
			gtk_toggle_button_set_active( toggled_button, TRUE );

		} else {
			toggle_group_set_active( st_toggle_group, BASE_WINDOW( window ), ibtn );
			set_new_sort_order( window, st_toggle_group[ibtn].order_mode );
		}
	}
}

static void
set_new_sort_order( NactMainWindow *window, guint order_mode )
{
	static const gchar *thisfn = "nact_sort_buttons_set_new_sort_order";
	NactApplication *application;
	NAUpdater *updater;

	if( st_set_sort_order ){
		g_debug( "%s: order_mode=%d", thisfn, order_mode );

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );

		na_iprefs_set_order_mode( NA_PIVOT( updater ), order_mode );
	}
}

/*
 * activate the button corresponding to the new sort order
 * desactivate the previous button
 * do nothing if new button and previous button are the sames
 */
static void
display_sort_order( NactMainWindow *window, guint order_mode )
{
	static const gchar *thisfn = "nact_sort_buttons_display_sort_order";
	gint iprev, inew;

	iprev = toggle_group_get_active( st_toggle_group, BASE_WINDOW( window ));

	inew = toggle_group_get_for_mode( st_toggle_group, order_mode );
	g_return_if_fail( inew >= 0 );

	g_debug( "%s: iprev=%d, inew=%d", thisfn, iprev, inew );

	if( iprev == -1 || inew != iprev ){
		toggle_group_set_active( st_toggle_group, BASE_WINDOW( window ), inew );
	}
}

/*
 * returns the index of the button currently active
 * or -1 if not found
 */
static gint
toggle_group_get_active( ToggleGroup *group, BaseWindow *window )
{
	GtkToggleButton *button;
	gint i = 0;

	if( st_last_active != -1 ){
		return( st_last_active );
	}

	while( group[i].btn_name ){
		button = GTK_TOGGLE_BUTTON( base_window_get_widget( window, group[i].btn_name ));
		if( gtk_toggle_button_get_active( button )){
			return( i );
		}
		i += 1;
	}

	return( -1 );
}

/*
 * returns the index of the button for the given order mode
 * or -1 if not found
 */
static gint
toggle_group_get_for_mode( ToggleGroup *group, guint mode )
{
	gint i = 0;

	while( group[i].btn_name ){
		if( group[i].order_mode == mode ){
			return( i );
		}
		i += 1;
	}

	return( -1 );
}

static void
toggle_group_set_active( ToggleGroup *group, BaseWindow *window, gint idx )
{
	static const gchar *thisfn = "nact_sort_buttons_toggle_group_set_active";
	GtkToggleButton *button;
	gint i;

	g_debug( "%s: idx=%d", thisfn, idx );

	i = 0;
	st_in_toggle = TRUE;

	while( group[i].btn_name ){
		button = GTK_TOGGLE_BUTTON( base_window_get_widget( window, group[i].btn_name ));
		gtk_toggle_button_set_active( button, i==idx );
		i += 1;
	}

	st_in_toggle = FALSE;
	st_last_active = idx;
}

static gint
toggle_group_get_from_button( ToggleGroup *group, BaseWindow *window, GtkToggleButton *toggled_button )
{
	GtkToggleButton *button;
	gint i = 0;

	while( group[i].btn_name ){
		button = GTK_TOGGLE_BUTTON( base_window_get_widget( window, group[i].btn_name ));
		if( button == toggled_button ){
			return( i );
		}
		i += 1;
	}

	return( -1 );
}
