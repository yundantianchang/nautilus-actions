/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-core-utils.h>

#include "nact-add-capability-dialog.h"

/* private class data
 */
struct NactAddCapabilityDialogClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAddCapabilityDialogPrivate {
	gboolean dispose_has_run;
	GSList  *already_used;
	gchar   *capability;
};

/* column ordering in the model
 */
enum {
	CAPABILITY_KEYWORD_COLUMN = 0,
	CAPABILITY_DESC_COLUMN,
	CAPABILITY_ALREADY_USED_COLUMN,
	CAPABILITY_N_COLUMN
};

typedef struct {
	gchar *keyword;
	gchar *desc;
}
	CapabilityTextStruct;

static CapabilityTextStruct st_caps[] = {
		{ "Owner",      N_( "User is the owner of the item" ) },
		{ "Readable",   N_( "Item is readable by the user" ) },
		{ "Writable",   N_( "Item is writable by the user" ) },
		{ "Executable", N_( "Item is executable by the user" ) },
		{ "Local",      N_( "Item is local" ) },
		{ NULL },
};

static GObjectClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAddCapabilityDialogClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactAddCapabilityDialog *add_capability_dialog_new( BaseWindow *parent );

static gchar   *base_get_iprefs_window_id( const BaseWindow *window );
static gchar   *base_get_dialog_name( const BaseWindow *window );
static gchar   *base_get_ui_filename( const BaseWindow *dialog );
static void     on_base_initial_load_dialog( NactAddCapabilityDialog *editor, gpointer user_data );
static void     on_base_runtime_init_dialog( NactAddCapabilityDialog *editor, gpointer user_data );
static void     on_base_all_widgets_showed( NactAddCapabilityDialog *editor, gpointer user_data );
static void     on_cancel_clicked( GtkButton *button, NactAddCapabilityDialog *editor );
static void     on_ok_clicked( GtkButton *button, NactAddCapabilityDialog *editor );
static void     on_selection_changed( GtkTreeSelection *selection, BaseWindow *window );
static void     display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window );
static void     display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window );
static void     display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window, guint column_id );
static gboolean setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *capabilities );
static void     validate_dialog( NactAddCapabilityDialog *editor );
static gboolean base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_add_capability_dialog_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_add_capability_dialog_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAddCapabilityDialogClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAddCapabilityDialog ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactAddCapabilityDialog", &info, 0 );

	return( type );
}

static void
class_init( NactAddCapabilityDialogClass *klass )
{
	static const gchar *thisfn = "nact_add_capability_dialog_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAddCapabilityDialogClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->get_ui_filename = base_get_ui_filename;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_add_capability_dialog_instance_init";
	NactAddCapabilityDialog *self;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( instance ));
	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	self = NACT_ADD_CAPABILITY_DIALOG( instance );

	self->private = g_new0( NactAddCapabilityDialogPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
	self->private->capability = NULL;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_add_capability_dialog_instance_dispose";
	NactAddCapabilityDialog *self;
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));
	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	self = NACT_ADD_CAPABILITY_DIALOG( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
		model = gtk_tree_view_get_model( listview );
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_unselect_all( selection );
		gtk_list_store_clear( GTK_LIST_STORE( model ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_add_capability_dialog_instance_finalize";
	NactAddCapabilityDialog *self;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));
	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	self = NACT_ADD_CAPABILITY_DIALOG( dialog );

	na_core_utils_slist_free( self->private->already_used );
	g_free( self->private->capability );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactAddCapabilityDialog object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactAddCapabilityDialog *
add_capability_dialog_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ADD_CAPABILITY_DIALOG_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
}

/**
 * nact_add_capability_dialog_run:
 * @parent: the BaseWindow parent of this dialog
 *  (usually the NactMainWindow).
 * @capabilities: list of already used capabilities.
 *
 * Initializes and runs the dialog.
 *
 * Returns: the selected capability, as a newly allocated string which should
 * be g_free() by the caller, or NULL.
 */
gchar *
nact_add_capability_dialog_run( BaseWindow *parent, GSList *capabilities )
{
	static const gchar *thisfn = "nact_add_capability_dialog_run";
	NactAddCapabilityDialog *dialog;
	gchar *capability;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );

	g_return_val_if_fail( BASE_IS_WINDOW( parent ), NULL );

	dialog = add_capability_dialog_new( parent );
	dialog->private->already_used = na_core_utils_slist_duplicate( capabilities );

	base_window_run( BASE_WINDOW( dialog ));

	capability = g_strdup( dialog->private->capability );

	g_object_unref( dialog );

	return( capability );
}

static gchar *
base_get_iprefs_window_id( const BaseWindow *window )
{
	return( g_strdup( "add-capability-dialog" ));
}

static gchar *
base_get_dialog_name( const BaseWindow *window )
{
	return( g_strdup( "AddCapabilityDialog" ));
}

static gchar *
base_get_ui_filename( const BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-add-capability.ui" ));
}

static void
on_base_initial_load_dialog( NactAddCapabilityDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_initial_load_dialog";
	GtkTreeView *listview;
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_cell;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));

		model = GTK_TREE_MODEL( gtk_list_store_new( CAPABILITY_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN ));
		gtk_tree_view_set_model( listview, GTK_TREE_MODEL( model ));
		g_object_unref( model );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"capability-keyword",
				text_cell,
				"text", CAPABILITY_KEYWORD_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( model ), CAPABILITY_KEYWORD_COLUMN, GTK_SORT_ASCENDING );
		gtk_tree_view_column_set_cell_data_func(
				column, text_cell, ( GtkTreeCellDataFunc ) display_keyword, dialog, NULL );

		text_cell = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(
				"capability-description",
				text_cell,
				"text", CAPABILITY_DESC_COLUMN,
				NULL );
		gtk_tree_view_append_column( listview, column );
		gtk_tree_view_column_set_cell_data_func(
				column, text_cell, ( GtkTreeCellDataFunc ) display_description, dialog, NULL );

		gtk_tree_view_set_headers_visible( listview, FALSE );

		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
	}
}

static void
on_base_runtime_init_dialog( NactAddCapabilityDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_runtime_init_dialog";
	GtkTreeView *listview;
	GtkListStore *model;
	GtkTreeIter row;
	guint i;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
		model = GTK_LIST_STORE( gtk_tree_view_get_model( listview ));

		for( i = 0 ; st_caps[i].keyword ; i = i+1 ){
			gtk_list_store_append( model, &row );
			gtk_list_store_set( model, &row,
					CAPABILITY_KEYWORD_COLUMN, st_caps[i].keyword,
					CAPABILITY_DESC_COLUMN, st_caps[i].desc,
					CAPABILITY_ALREADY_USED_COLUMN, FALSE,
					-1 );
		}

		gtk_tree_model_foreach( GTK_TREE_MODEL( model ), ( GtkTreeModelForeachFunc ) setup_values_iter, dialog->private->already_used );

		base_window_signal_connect(
				BASE_WINDOW( dialog ),
				G_OBJECT( gtk_tree_view_get_selection( listview )),
				"changed",
				G_CALLBACK( on_selection_changed ));

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"CancelButton",
				"clicked",
				G_CALLBACK( on_cancel_clicked ));

		base_window_signal_connect_by_name(
				BASE_WINDOW( dialog ),
				"OKButton",
				"clicked",
				G_CALLBACK( on_ok_clicked ));
	}
}

static void
on_base_all_widgets_showed( NactAddCapabilityDialog *dialog, gpointer user_data )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_all_widgets_showed";
	GtkTreeView *listview;
	GtkTreePath *path;
	GtkTreeSelection *selection;

	g_return_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( dialog ));

	if( !dialog->private->dispose_has_run ){

		g_debug( "%s: dialog=%p, user_data=%p", thisfn, ( void * ) dialog, ( void * ) user_data );

		listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
		path = gtk_tree_path_new_first();
		selection = gtk_tree_view_get_selection( listview );
		gtk_tree_selection_select_path( selection, path );
		gtk_tree_path_free( path );
	}
}

static void
on_cancel_clicked( GtkButton *button, NactAddCapabilityDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactAddCapabilityDialog *dialog )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( dialog ));

	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
on_selection_changed( GtkTreeSelection *selection, BaseWindow *window )
{
	GList *rows;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean used;
	GtkWidget *button;

	rows = gtk_tree_selection_get_selected_rows( selection, &model );
	used = FALSE;

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, CAPABILITY_ALREADY_USED_COLUMN, &used, -1 );
	}

	button = base_window_get_widget( window, "OKButton" );
	gtk_widget_set_sensitive( button, !used );
}

static void
display_keyword( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window )
{
	display_label( column, cell, model, iter, window, CAPABILITY_KEYWORD_COLUMN );
}

static void
display_description( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window )
{
	display_label( column, cell, model, iter, window, CAPABILITY_DESC_COLUMN );
}

static void
display_label( GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, BaseWindow *window, guint column_id )
{
	gboolean used;

	gtk_tree_model_get( model, iter, CAPABILITY_ALREADY_USED_COLUMN, &used, -1 );
	g_object_set( cell, "style-set", FALSE, NULL );

	if( used ){
		g_object_set( cell, "style", PANGO_STYLE_ITALIC, "style-set", TRUE, NULL );
	}
}

static gboolean
setup_values_iter( GtkTreeModel *model, GtkTreePath *path, GtkTreeIter* iter, GSList *capabilities )
{
	gchar *keyword;
	gchar *description, *new_description;

	gtk_tree_model_get( model, iter, CAPABILITY_KEYWORD_COLUMN, &keyword, CAPABILITY_DESC_COLUMN, &description, -1 );

	if( na_core_utils_slist_find_negated( capabilities, keyword )){
		/* i18n: add a comment when a capability is already used by current item */
		new_description = g_strdup_printf( _( "%s (already used)"), description );
		gtk_list_store_set( GTK_LIST_STORE( model ), iter, CAPABILITY_DESC_COLUMN, new_description, CAPABILITY_ALREADY_USED_COLUMN, TRUE, -1 );
		g_free( new_description );
	}

	g_free( description );
	g_free( keyword );

	return( FALSE ); /* don't stop looping */
}

static void
validate_dialog( NactAddCapabilityDialog *dialog )
{
	GtkTreeView *listview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *rows;
	GtkTreePath *path;
	GtkTreeIter iter;

	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( dialog ), "CapabilitiesTreeView" ));
	selection = gtk_tree_view_get_selection( listview );
	rows = gtk_tree_selection_get_selected_rows( selection, &model );

	if( g_list_length( rows ) == 1 ){
		path = ( GtkTreePath * ) rows->data;
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, CAPABILITY_KEYWORD_COLUMN, &dialog->private->capability, -1 );
	}
}

static gboolean
base_dialog_response( GtkDialog *dialog_box, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_add_capability_dialog_on_dialog_response";
	NactAddCapabilityDialog *dialog;

	g_return_val_if_fail( NACT_IS_ADD_CAPABILITY_DIALOG( window ), FALSE );

	dialog = NACT_ADD_CAPABILITY_DIALOG( window );

	if( !dialog->private->dispose_has_run ){
		g_debug( "%s: dialog_box=%p, code=%d, window=%p", thisfn, ( void * ) dialog_box, code, ( void * ) window );

		switch( code ){
			case GTK_RESPONSE_OK:
				validate_dialog( dialog );

			case GTK_RESPONSE_NONE:
			case GTK_RESPONSE_DELETE_EVENT:
			case GTK_RESPONSE_CLOSE:
			case GTK_RESPONSE_CANCEL:
				return( TRUE );
				break;
		}
	}

	return( FALSE );
}