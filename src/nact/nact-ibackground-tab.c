/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
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
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>

#include <common/na-iprefs.h>
#include <common/na-object-api.h>
#include <common/na-utils.h>

#include <runtime/na-pivot.h>

#include "base-iprefs.h"
#include "base-window.h"
#include "nact-application.h"
#include "nact-main-tab.h"
#include "nact-ibackground-tab.h"

/* private interface data
 */
struct NactIBackgroundTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* column ordering
 */
enum {
	BACKGROUND_URI_COLUMN = 0,
	BACKGROUND_N_COLUMN
};

#define IPREFS_BACKGROUND_DIALOG		"ibackground-chooser"
#define IPREFS_BACKGROUND_URI			"ibackground-uri"

static gboolean st_initialized = FALSE;
static gboolean st_finalized = FALSE;

static GType        register_type( void );
static void         interface_base_init( NactIBackgroundTabInterface *klass );
static void         interface_base_finalize( NactIBackgroundTabInterface *klass );

static void         on_tab_updatable_selection_changed( NactIBackgroundTab *instance, gint count_selected );
static void         on_tab_updatable_enable_tab( NactIBackgroundTab *instance, NAObjectItem *item );
static gboolean     tab_set_sensitive( NactIBackgroundTab *instance );

static void         add_row( NactIBackgroundTab *instance, GtkTreeView *listview, const gchar *uri );
static void         add_uri_to_folders( NactIBackgroundTab *instance, const gchar *uri );
static GtkTreeView *get_folders_treeview( NactIBackgroundTab *instance );
static void         on_folder_uri_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIBackgroundTab *instance );
static void         on_folders_selection_changed( GtkTreeSelection *selection, NactIBackgroundTab *instance );
static void         on_add_folder_clicked( GtkButton *button, NactIBackgroundTab *instance );
static void         on_remove_folder_clicked( GtkButton *button, NactIBackgroundTab *instance );
static void         reset_folders( NactIBackgroundTab *instance );
static void         setup_folders( NactIBackgroundTab *instance );
static void         treeview_cell_edited( NactIBackgroundTab *instance, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text );

static void         on_toolbar_same_label_toggled( GtkToggleButton *button, NactIBackgroundTab *instance );

static void         on_toolbar_label_changed( GtkEntry *entry, NactIBackgroundTab *instance );

GType
nact_ibackground_tab_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_ibackground_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIBackgroundTabInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIBackgroundTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIBackgroundTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibackground_tab_interface_base_init";

	if( !st_initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIBackgroundTabInterfacePrivate, 1 );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIBackgroundTabInterface *klass )
{
	static const gchar *thisfn = "nact_ibackground_tab_interface_base_finalize";

	if( !st_finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		st_finalized = TRUE;
	}
}

void
nact_ibackground_tab_initial_load_toplevel( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_initial_load_toplevel";
	GtkTreeView *listview;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		model = gtk_list_store_new( BACKGROUND_N_COLUMN, G_TYPE_STRING );
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), BACKGROUND_URI_COLUMN, GTK_SORT_ASCENDING );
		listview = get_folders_treeview( instance );
		gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
		g_object_unref( model );

		text_cell = gtk_cell_renderer_text_new();
		g_object_set( G_OBJECT( text_cell ), "editable", TRUE, NULL );
		column = gtk_tree_view_column_new_with_attributes(
				"folder-uri",
				text_cell,
				"text", BACKGROUND_URI_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );

		gtk_tree_view_set_headers_visible( listview, FALSE );
	}
}

void
nact_ibackground_tab_runtime_init_toplevel( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_runtime_init_toplevel";
	GtkTreeView *listview;
	GtkTreeViewColumn *column;
	GList *renderers;
	GtkWidget *add_button, *remove_button;
	GtkWidget *label_widget;
	GtkWidget *toolbar_same_label_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		g_signal_connect(
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_tab_updatable_selection_changed ),
				instance );

		g_signal_connect(
				G_OBJECT( instance ),
				TAB_UPDATABLE_SIGNAL_ENABLE_TAB,
				G_CALLBACK( on_tab_updatable_enable_tab ),
				instance );

		listview = get_folders_treeview( instance );
		column = gtk_tree_view_get_column( listview, BACKGROUND_URI_COLUMN );
		renderers = gtk_tree_view_column_get_cell_renderers( column );
		g_signal_connect(
				G_OBJECT( renderers->data ),
				"edited",
				G_CALLBACK( on_folder_uri_edited ),
				instance );

		add_button = base_window_get_widget( BASE_WINDOW( instance ), "AddFolderButton");
		g_signal_connect(
				G_OBJECT( add_button ),
				"clicked",
				G_CALLBACK( on_add_folder_clicked ),
				instance );

		remove_button = base_window_get_widget( BASE_WINDOW( instance ), "RemoveFolderButton");
		g_signal_connect(
				G_OBJECT( remove_button ),
				"clicked",
				G_CALLBACK( on_remove_folder_clicked ),
				instance );

		g_signal_connect(
				G_OBJECT( gtk_tree_view_get_selection( listview )),
				"changed",
				G_CALLBACK( on_folders_selection_changed ),
				instance );

		toolbar_same_label_button = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" );
		g_signal_connect(
				G_OBJECT( toolbar_same_label_button ),
				"toggled",
				G_CALLBACK( on_toolbar_same_label_toggled ),
				instance );

		label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
		g_signal_connect(
				G_OBJECT( label_widget ),
				"changed",
				G_CALLBACK( on_toolbar_label_changed ),
				instance );
	}
}

void
nact_ibackground_tab_all_widgets_showed( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){
	}
}

void
nact_ibackground_tab_dispose( NactIBackgroundTab *instance )
{
	static const gchar *thisfn = "nact_ibackground_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		reset_folders( instance );
	}
}

static void
on_tab_updatable_selection_changed( NactIBackgroundTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_ibackground_tab_on_tab_updatable_selection_changed";
	NAObjectItem *item;
	gboolean enable_tab;
	GtkToggleButton *same_label_button;
	gboolean same_label;
	GtkWidget *short_label_widget;
	gchar *short_label;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );
	g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

	if( st_initialized && !st_finalized ){

		reset_folders( instance );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
				NULL );

		g_return_if_fail( !item || NA_IS_OBJECT_ITEM( item ));

		enable_tab = tab_set_sensitive( instance );

		if( enable_tab ){
			setup_folders( instance );
		}

		/* only actions go to toolbar */
		same_label_button = GTK_TOGGLE_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ToolbarSameLabelButton" ));
		same_label = enable_tab && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_use_same_label( NA_OBJECT_ACTION( item )) : FALSE;
		gtk_toggle_button_set_active( same_label_button, same_label );

		short_label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
		short_label = enable_tab && NA_IS_OBJECT_ACTION( item ) ? na_object_action_toolbar_get_label( NA_OBJECT_ACTION( item )) : g_strdup( "" );
		gtk_entry_set_text( GTK_ENTRY( short_label_widget ), short_label );
		g_free( short_label );
	}
}

static void
on_tab_updatable_enable_tab( NactIBackgroundTab *instance, NAObjectItem *item )
{
	static const gchar *thisfn = "nact_ibackground_tab_on_tab_updatable_enable_tab";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: instance=%p, item=%p", thisfn, ( void * ) instance, ( void * ) item );
		g_return_if_fail( NACT_IS_IBACKGROUND_TAB( instance ));

		tab_set_sensitive( instance );
	}
}

static gboolean
tab_set_sensitive( NactIBackgroundTab *instance )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	gboolean is_action;
	gboolean enable_tab, enable_folders_frame, enable_toolbar_frame;
	GtkWidget *toolbar_frame_widget;
	gboolean enable_toolbar_label_entry;
	GtkWidget *toolbar_label_entry;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	is_action = ( action && NA_IS_OBJECT_ACTION( action ));
	enable_toolbar_frame = ( is_action && na_object_action_is_target_toolbar( action ));
	enable_folders_frame = ( profile != NULL &&
			( na_object_action_is_target_background( action ) || enable_toolbar_frame ));
	enable_tab = enable_folders_frame || enable_toolbar_frame;

	nact_main_tab_enable_page( NACT_MAIN_WINDOW( instance ), TAB_BACKGROUND, enable_tab );

	toolbar_frame_widget = base_window_get_widget( BASE_WINDOW( instance ), "BackgroundToolbarFrame" );
	gtk_widget_set_sensitive( toolbar_frame_widget, enable_toolbar_frame );

	enable_toolbar_label_entry = ( enable_toolbar_frame && !na_object_action_toolbar_use_same_label( action ));
	toolbar_label_entry = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
	gtk_widget_set_sensitive( toolbar_label_entry, enable_toolbar_label_entry );

	return( enable_tab );
}

static void
add_row( NactIBackgroundTab *instance, GtkTreeView *listview, const gchar *uri )
{
	GtkTreeModel *model;
	GtkTreeIter row;

	model = gtk_tree_view_get_model( listview );

	gtk_list_store_append(
			GTK_LIST_STORE( model ),
			&row );

	gtk_list_store_set(
			GTK_LIST_STORE( model ),
			&row,
			BACKGROUND_URI_COLUMN, uri,
			-1 );
}

static void
add_uri_to_folders( NactIBackgroundTab *instance, const gchar *uri )
{
	NAObjectAction *action;
	NAObjectProfile *edited;
	GSList *folders;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	folders = na_object_profile_get_folders( edited );
	folders = g_slist_prepend( folders, ( gpointer ) g_strdup( uri ));
	na_object_profile_set_folders( edited, folders );
	na_utils_free_string_list( folders );

	g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
}

static GtkTreeView *
get_folders_treeview( NactIBackgroundTab *instance )
{
	GtkWidget *treeview;

	treeview = base_window_get_widget( BASE_WINDOW( instance ), "FoldersTreeview" );
	g_assert( GTK_IS_TREE_VIEW( treeview ));

	return( GTK_TREE_VIEW( treeview ));
}

static void
on_add_folder_clicked( GtkButton *button, NactIBackgroundTab *instance )
{
	GtkWidget *dialog;
	GtkWindow *toplevel;
	NactApplication *application;
	NAPivot *pivot;
	gchar *uri;
	GtkTreeView *listview;

	uri = NULL;
	listview = get_folders_treeview( instance );
	toplevel = base_window_get_toplevel( BASE_WINDOW( instance ));

	/* i18n: title of the FileChoose dialog when selecting an URI which
	 * will be comparent to Nautilus 'current_folder'
	 */
	dialog = gtk_file_chooser_dialog_new( _( "Select a folder" ),
			toplevel,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
	pivot = nact_application_get_pivot( application );

	base_iprefs_position_named_window( BASE_WINDOW( instance ), GTK_WINDOW( dialog ), IPREFS_BACKGROUND_DIALOG );

	uri = na_iprefs_read_string( NA_IPREFS( pivot ), IPREFS_BACKGROUND_URI, "x-nautilus-desktop:///" );
	if( uri && strlen( uri )){
		gtk_file_chooser_set_uri( GTK_FILE_CHOOSER( dialog ), uri );
	}
	g_free( uri );

	if( gtk_dialog_run( GTK_DIALOG( dialog )) == GTK_RESPONSE_ACCEPT ){
		uri = gtk_file_chooser_get_uri( GTK_FILE_CHOOSER( dialog ));
		na_iprefs_write_string( NA_IPREFS( pivot ), IPREFS_BACKGROUND_URI, uri );
		add_row( instance, listview, uri );
		add_uri_to_folders( instance, uri );
		g_free( uri );
	}

	base_iprefs_save_named_window_position( BASE_WINDOW( instance ), GTK_WINDOW( dialog ), IPREFS_BACKGROUND_DIALOG );

	gtk_widget_destroy( dialog );
}

static void
on_folder_uri_edited( GtkCellRendererText *renderer, const gchar *path, const gchar *text, NactIBackgroundTab *instance )
{
	treeview_cell_edited( instance, path, text, BACKGROUND_URI_COLUMN, NULL, NULL );
}

static void
on_folders_selection_changed( GtkTreeSelection *selection, NactIBackgroundTab *instance )
{
	GtkWidget *button;

	button = base_window_get_widget( BASE_WINDOW( instance ), "RemoveFolderButton");

	gtk_widget_set_sensitive( button, gtk_tree_selection_count_selected_rows( selection ) > 0 );
}

static void
on_remove_folder_clicked( GtkButton *button, NactIBackgroundTab *instance )
{
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *selected_path, *isp;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *uri;
	NAObjectAction *action;
	NAObjectProfile *edited;
	GSList *folders;

	listview = get_folders_treeview( instance );
	model = gtk_tree_view_get_model( listview );
	selection = gtk_tree_view_get_selection( listview );
	selected_path = gtk_tree_selection_get_selected_rows( selection, NULL );

	for( isp = selected_path ; isp ; isp = isp->next ){
		path = ( GtkTreePath * ) isp->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, BACKGROUND_URI_COLUMN, &uri, -1 );
		gtk_list_store_remove( GTK_LIST_STORE( model ), &iter );

		g_object_get(
				G_OBJECT( instance ),
				TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
				TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
				NULL );

		folders = na_object_profile_get_folders( edited );
		folders = na_utils_remove_from_string_list( folders, uri );
		na_object_profile_set_folders( edited, folders );

		na_utils_free_string_list( folders );
		g_free( uri );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
	}

	g_list_foreach( selected_path, ( GFunc ) gtk_tree_path_free, NULL );
	g_list_free( selected_path );
}

static void
reset_folders( NactIBackgroundTab *instance )
{
	GtkTreeView *listview;
	GtkTreeModel *model;

	listview = get_folders_treeview( instance );
	model = gtk_tree_view_get_model( listview );
	gtk_list_store_clear( GTK_LIST_STORE( model ));
}

static void
setup_folders( NactIBackgroundTab *instance )
{
	NAObjectProfile *edited;
	GSList *folders, *is;
	GtkTreeView *listview;

	listview = get_folders_treeview( instance );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	folders = na_object_profile_get_folders( edited );
	for( is = folders ; is ; is = is->next ){
		add_row( instance, listview, ( const gchar * ) is->data );
	}
	na_utils_free_string_list( folders );
}

static void
treeview_cell_edited( NactIBackgroundTab *instance, const gchar *path_string, const gchar *text, gint column, gboolean *state, gchar **old_text )
{
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	NAObjectAction *action;
	NAObjectProfile *edited;

	listview = get_folders_treeview( instance );
	model = gtk_tree_view_get_model( listview );
	path = gtk_tree_path_new_from_string( path_string );
	gtk_tree_model_get_iter( model, &iter, path );
	gtk_tree_path_free( path );

	if( state && old_text ){
		gtk_tree_model_get( model, &iter, BACKGROUND_URI_COLUMN, old_text, -1 );
	}

	gtk_list_store_set( GTK_LIST_STORE( model ), &iter, column, text, -1 );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	na_object_profile_replace_folder_uri( edited, *old_text, text );

	g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, action, FALSE );
}

static void
on_toolbar_same_label_toggled( GtkToggleButton *button, NactIBackgroundTab *instance )
{
	NAObjectItem *edited;
	gboolean same_label;
	GtkWidget *label_widget;
	gchar *text;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		g_return_if_fail( NA_IS_OBJECT_ACTION( edited ));

		same_label = gtk_toggle_button_get_active( button );
		na_object_action_toolbar_set_same_label( NA_OBJECT_ACTION( edited ), same_label );

		tab_set_sensitive( instance );

		if( same_label ){
			label_widget = base_window_get_widget( BASE_WINDOW( instance ), "ToolbarLabelEntry" );
			text = na_object_get_label( edited );
			gtk_entry_set_text( GTK_ENTRY( label_widget ), text );
			g_free( text );
		}

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}

static void
on_toolbar_label_changed( GtkEntry *entry, NactIBackgroundTab *instance )
{
	NAObjectItem *edited;
	const gchar *label;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &edited,
			NULL );

	if( edited ){
		g_return_if_fail( NA_IS_OBJECT_ACTION( edited ));

		label = gtk_entry_get_text( entry );
		na_object_action_toolbar_set_label( NA_OBJECT_ACTION( edited ), label );

		g_signal_emit_by_name( G_OBJECT( instance ), TAB_UPDATABLE_SIGNAL_ITEM_UPDATED, edited, FALSE );
	}
}
