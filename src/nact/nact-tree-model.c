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

#include <string.h>

#include <common/na-object-api.h>
#include <common/na-object-action.h>
#include <common/na-object-menu.h>
#include <common/na-object-profile.h>
#include <common/na-iprefs.h>
#include <common/na-utils.h>

#include "egg-tree-multi-dnd.h"
#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-tree-model.h"

/*
 * call once egg_tree_multi_drag_add_drag_support( treeview ) at init time (before gtk_main)
 *
 * when we start with drag
 * 	 call once egg_tree_multi_dnd_on_button_press_event( treeview, event, drag_source )
 *   call many egg_tree_multi_dnd_on_motion_event( treeview, event, drag_source )
 *     until mouse quits the selected area
 *
 * as soon as mouse has quitted the selected area
 *   call once egg_tree_multi_dnd_stop_drag_check( treeview )
 *   call once nact_tree_model_imulti_drag_source_row_draggable: drag_source=0x92a0d70, path_list=0x9373c90
 *   call once nact_clipboard_on_drag_begin( treeview, context, main_window )
 *
 * when we drop (e.g. in Nautilus)
 *   call once egg_tree_multi_drag_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once nact_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndDirectSave0 )
 *   call once nact_tree_model_idrag_dest_drag_data_received
 *   call once nact_clipboard_on_drag_end( treeview, context, main_window )
 */

/* private class data
 */
struct NactTreeModelClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactTreeModelPrivate {
	gboolean     dispose_has_run;
	BaseWindow  *window;
	GtkTreeView *treeview;
	gboolean     have_dnd;
	gboolean     only_actions;
	gchar       *drag_dest_uri;
	GList       *drag_items;
};

#define MAX_XDS_ATOM_VAL_LEN			4096
#define TEXT_ATOM						gdk_atom_intern( "text/plain", FALSE )
#define XDS_ATOM						gdk_atom_intern( "XdndDirectSave0", FALSE )
#define XDS_FILENAME					"xds.txt"

#define TREE_MODEL_ORDER_MODE			"nact-tree-model-order-mode"

enum {
	NACT_XCHANGE_FORMAT_NACT = 0,
	NACT_XCHANGE_FORMAT_XDS,
	NACT_XCHANGE_FORMAT_APPLICATION_XML,
	NACT_XCHANGE_FORMAT_TEXT_PLAIN,
	NACT_XCHANGE_FORMAT_URI_LIST
};

/* as a dnd source, we provide
 * - a special XdndNautilusAction format for internal move/copy
 * - a XdndDirectSave, suitable for exporting to a file manager
 *   (note that Nautilus recognized the "XdndDirectSave0" format as XDS
 *   protocol)
 * - a text (xml) format, to go to clipboard or a text editor
 */
static GtkTargetEntry dnd_source_formats[] = {
	{ "XdndNautilusActions", GTK_TARGET_SAME_WIDGET, NACT_XCHANGE_FORMAT_NACT },
	{ "XdndDirectSave0",     GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_XDS },
	{ "application/xml",     GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",          GTK_TARGET_OTHER_APP,   NACT_XCHANGE_FORMAT_TEXT_PLAIN },
};

#define NACT_ATOM						gdk_atom_intern( "XdndNautilusActions", FALSE )

static GtkTargetEntry dnd_dest_targets[] = {
	{ "XdndNautilusActions", 0, NACT_XCHANGE_FORMAT_NACT },
	{ "text/uri-list",       0, NACT_XCHANGE_FORMAT_URI_LIST },
	{ "application/xml",     0, NACT_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",          0, NACT_XCHANGE_FORMAT_TEXT_PLAIN },
};

typedef struct {
	gchar *fname;
	gchar *prefix;
}
	ntmDumpStruct;

typedef struct {
	GtkTreeModel   *store;
	const NAObject *object;
	gboolean        found;
	GtkTreeIter    *iter;
}
	ntmSearchStruct;

typedef struct {
	GtkTreeModel *store;
	gchar        *id;
	gboolean      found;
	GtkTreeIter  *iter;
}
	ntmSearchIdStruct;

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactTreeModelClass *klass );
static void           imulti_drag_source_init( EggTreeMultiDragSourceIface *iface );
static void           idrag_dest_init( GtkTreeDragDestIface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *application );
static void           instance_finalize( GObject *application );

static NactTreeModel *tree_model_new( BaseWindow *window, GtkTreeView *treeview );

static void           fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, GList *items, gboolean only_actions, GtkTreeIter *parent );

static void           insert_get_iters_action( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           insert_get_iters_profile( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           insert_get_iters_menu( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           insert_before_get_iters( GtkTreeModel *model,  GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           insert_before_parent_get_iters( GtkTreeModel *model,  GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           insert_as_last_child_get_iters( GtkTreeModel *model,  GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object );
static void           remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object );

static GList         *add_parent( GList *parents, GtkTreeModel *store, GtkTreeIter *obj_iter );
static void           append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object );
static void           display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object );
static gboolean       dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm );
static void           iter_on_store( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data );
static gboolean       iter_on_store_item( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data );
static gboolean       remove_items( GtkTreeStore *store, GtkTreeIter *iter );
static gboolean       search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_objet_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm );
static gboolean       search_for_object_id( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_object_id_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchIdStruct *ntm );

static gboolean       imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list );
static gboolean       imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source, GdkDragContext *context, GtkSelectionData *selection_data, GList *path_list, guint info );
static gboolean       imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list );
static GtkTargetList *imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source );
static GdkDragAction  imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source );

static gboolean       idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data );
static gboolean       idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data );

static gboolean       on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );
static void           on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window );

/*static gboolean       on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window );
static void           on_drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, BaseWindow *window );*/

static gint           sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data );
static gboolean       filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model );
static char          *get_xds_atom_value( GdkDragContext *context );

GType
nact_tree_model_get_type( void )
{
	static GType model_type = 0;

	if( !model_type ){
		model_type = register_type();
	}

	return( model_type );
}

static GType
register_type (void)
{
	static const gchar *thisfn = "nact_tree_model_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactTreeModelClass ),
		NULL,							/* base_init */
		NULL,							/* base_finalize */
		( GClassInitFunc ) class_init,
		NULL,							/* class_finalize */
		NULL,							/* class_data */
		sizeof( NactTreeModel ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo imulti_drag_source_info = {
		( GInterfaceInitFunc ) imulti_drag_source_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo idrag_dest_info = {
		( GInterfaceInitFunc ) idrag_dest_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_TREE_MODEL_FILTER, "NactTreeModel", &info, 0 );

	g_type_add_interface_static( type, EGG_TYPE_TREE_MULTI_DRAG_SOURCE, &imulti_drag_source_info );

	g_type_add_interface_static( type, GTK_TYPE_TREE_DRAG_DEST, &idrag_dest_info );

	return( type );
}

static void
class_init( NactTreeModelClass *klass )
{
	static const gchar *thisfn = "nact_tree_model_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactTreeModelClassPrivate, 1 );
}

static void
imulti_drag_source_init( EggTreeMultiDragSourceIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->row_draggable = imulti_drag_source_row_draggable;
	iface->drag_data_get = imulti_drag_source_drag_data_get;
	iface->drag_data_delete = imulti_drag_source_drag_data_delete;
	iface->get_target_list = imulti_drag_source_get_target_list;
	iface->free_target_list = NULL;
	iface->get_drag_actions = imulti_drag_source_get_drag_actions;
}

static void
idrag_dest_init( GtkTreeDragDestIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->drag_data_received = idrag_dest_drag_data_received;
	iface->row_drop_possible = idrag_dest_row_drop_possible;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_tree_model_instance_init";
	NactTreeModel *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_TREE_MODEL( instance ));
	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_dispose";
	NactTreeModel *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_finalize";
	NactTreeModel *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NACT_IS_TREE_MODEL( object ));
	self = NACT_TREE_MODEL( object );

	g_free( self->private->drag_dest_uri );
	g_list_free( self->private->drag_items );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * tree_model_new:
 * @window: a #BaseWindow window which must implement #NactIActionsList
 * interface.
 * @treeview: the #GtkTreeView widget.
 *
 * Creates a new #NactTreeModel model.
 *
 * This function should be called at widget initial load time. Is is so
 * too soon to make any assumption about sorting in the tree view.
 */
static NactTreeModel *
tree_model_new( BaseWindow *window, GtkTreeView *treeview )
{
	static const gchar *thisfn = "nact_tree_model_new";
	GtkTreeStore *ts_model;
	NactTreeModel *model;
	NactApplication *application;
	NAPivot *pivot;
	gint order_mode;

	g_debug( "%s: window=%p, treeview=%p", thisfn, ( void * ) window, ( void * ) treeview );
	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( window ), NULL );
	g_return_val_if_fail( GTK_IS_TREE_VIEW( treeview ), NULL );

	ts_model = gtk_tree_store_new(
			IACTIONS_LIST_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	/* create the filter model
	 */
	model = g_object_new( NACT_TREE_MODEL_TYPE, "child-model", ts_model, NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, model, NULL );

	/* initialize the sortable interface
	 */
	application = NACT_APPLICATION( base_window_get_application( window ));
	pivot = nact_application_get_pivot( application );
	order_mode = na_iprefs_get_order_mode( NA_IPREFS( pivot ));
	nact_tree_model_display_order_change( model, order_mode );

	model->private->window = window;
	model->private->treeview = treeview;

	return( model );
}

/**
 * nact_tree_model_initial_load:
 * @window: the #BaseWindow window.
 * @widget: the #GtkTreeView which will implement the #NactTreeModel.
 *
 * Creates a #NactTreeModel, and attaches it to the treeview.
 *
 * Please note that we cannot make any assumption here whether the
 * treeview, and so the tree model, must or not implement the drag and
 * drop interfaces.
 * This is because #NactIActionsList::on_initial_load() initializes these
 * properties to %FALSE. The actual values will be set by the main
 * program between #NactIActionsList::on_initial_load() returns and call
 * to #NactIActionsList::on_runtime_init().
 */
void
nact_tree_model_initial_load( BaseWindow *window, GtkTreeView *treeview )
{
	static const gchar *thisfn = "nact_tree_model_initial_load";
	NactTreeModel *model;

	g_debug( "%s: window=%p, treeview=%p", thisfn, ( void * ) window, ( void * ) treeview );
	g_return_if_fail( BASE_IS_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( window ));
	g_return_if_fail( GTK_IS_TREE_VIEW( treeview ));

	model = tree_model_new( window, treeview );

	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));

	g_object_unref( model );
}

/**
 * nact_tree_model_runtime_init_dnd:
 * @model: this #NactTreeModel instance.
 * @have_dnd: whether the tree model must implement drag and drop
 * interfaces.
 *
 * Initializes the tree model.
 *
 * We use drag and drop:
 * - inside of treeview, for duplicating items, or moving items between
 *   menus
 * - from treeview to the outside world (e.g. Nautilus) to export actions
 * - from outside world (e.g. Nautilus) to import actions
 */
void
nact_tree_model_runtime_init( NactTreeModel *model, gboolean have_dnd )
{
	static const gchar *thisfn = "nact_tree_model_runtime_init";

	g_debug( "%s: model=%p, have_dnd=%s", thisfn, ( void * ) model, have_dnd ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		if( have_dnd ){
			egg_tree_multi_drag_add_drag_support( EGG_TREE_MULTI_DRAG_SOURCE( model ), model->private->treeview );

			gtk_tree_view_enable_model_drag_dest(
				model->private->treeview,
				dnd_dest_targets, G_N_ELEMENTS( dnd_dest_targets ), GDK_ACTION_COPY | GDK_ACTION_MOVE );

			base_window_signal_connect(
					BASE_WINDOW( model->private->window ),
					G_OBJECT( model->private->treeview ),
					"drag-begin",
					G_CALLBACK( on_drag_begin ));

			base_window_signal_connect(
					BASE_WINDOW( model->private->window ),
					G_OBJECT( model->private->treeview ),
					"drag-end",
					G_CALLBACK( on_drag_end ));

			/*nact_window_signal_connect(
					NACT_WINDOW( window ),
					G_OBJECT( treeview ),
					"drag_drop",
					G_CALLBACK( on_drag_drop ));

			nact_window_signal_connect(
					NACT_WINDOW( window ),
					G_OBJECT( treeview ),
					"drag_data-received",
					G_CALLBACK( on_drag_data_received ));*/
		}
	}
}

void
nact_tree_model_dispose( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dispose";
	GtkTreeStore *ts_model;

	g_debug( "%s: model=%p", thisfn, ( void * ) model );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		nact_tree_model_dump( model );

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		gtk_tree_store_clear( ts_model );
	}
}

/**
 * nact_tree_model_display:
 * @model: this #NactTreeModel instance.
 * @object: the object whose display is to be refreshed.
 *
 * Refresh the display of a #NAObject.
 */
void
nact_tree_model_display( NactTreeModel *model, NAObject *object )
{
	/*static const gchar *thisfn = "nact_tree_model_display";*/
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	/*g_debug( "%s: model=%p (%s), object=%p (%s)", thisfn,
			( void * ) model, G_OBJECT_TYPE_NAME( model ),
			( void * ) object, G_OBJECT_TYPE_NAME( object ));*/
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			display_item( store, model->private->treeview, &iter, object );
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			gtk_tree_model_row_changed( GTK_TREE_MODEL( store ), path, &iter );
			gtk_tree_path_free( path );
		}

		/*gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));*/
	}
}

/**
 * Setup the new order mode.
 */
void
nact_tree_model_display_order_change( NactTreeModel *model, gint order_mode )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		g_object_set_data( G_OBJECT( store ), TREE_MODEL_ORDER_MODE, GINT_TO_POINTER( order_mode ));

		switch( order_mode ){

			case PREFS_ORDER_ALPHA_ASCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_ASCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case PREFS_ORDER_ALPHA_DESCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_DESCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case PREFS_ORDER_MANUAL:
			default:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
						0 );
				break;
		}
	}
}

void
nact_tree_model_dump( NactTreeModel *model )
{
	static const gchar *thisfn = "nact_tree_model_dump";
	GtkTreeStore *store;
	ntmDumpStruct *ntm;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		g_debug( "%s: %s at %p, %s at %p", thisfn,
				G_OBJECT_TYPE_NAME( model ), ( void * ) model, G_OBJECT_TYPE_NAME( store ), ( void * ) store );

		ntm = g_new0( ntmDumpStruct, 1 );
		ntm->fname = g_strdup( thisfn );
		ntm->prefix = g_strdup( "" );

		nact_tree_model_iter( model, ( FnIterOnStore ) dump_store, ntm );

		g_free( ntm->prefix );
		g_free( ntm->fname );
		g_free( ntm );
	}
}

/**
 * nact_tree_model_fill:
 * @model: this #NactTreeModel instance.
 * @ŧreeview: the #GtkTreeView widget.
 * @items: this list of items, usually from #NAPivot, which will be used
 * to fill up the tree store.
 * @only_actions: whether to store only actions, or all items.
 *
 * Fill up the tree store with specified items.
 *
 * We enter with the GSList owned by NAPivot which contains the ordered
 * list of level-zero items. We want have a duplicate of this list in
 * tree store, so that we are able to freely edit it.
 */
void
nact_tree_model_fill( NactTreeModel *model, GList *items, gboolean only_actions)
{
	static const gchar *thisfn = "nact_tree_model_fill";
	GtkTreeStore *ts_model;

	g_debug( "%s: model=%p, items=%p (%d items), only_actions=%s",
			thisfn, ( void * ) model, ( void * ) items, g_list_length( items ), only_actions ? "True":"False" );
	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		model->private->only_actions = only_actions;
		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		gtk_tree_store_clear( ts_model );
		fill_tree_store( ts_model, model->private->treeview, items, only_actions, NULL );
	}
}

static void
fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview,
					GList *items, gboolean only_actions, GtkTreeIter *parent )
{
	/*static const gchar *thisfn = "nact_tree_model_fill_tree_store";*/
	GList *subitems, *it;
	NAObject *object;
	NAObject *duplicate;
	GtkTreeIter iter;

	for( it = items ; it ; it = it->next ){
		object = NA_OBJECT( it->data );
		/*g_debug( "%s: object=%p(%s)", thisfn
				, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

		if( NA_IS_OBJECT_MENU( object )){
			duplicate = object;
			if( !only_actions ){
				duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
				/*g_debug( "%s: appending duplicate=%p (%s)", thisfn, ( void * ) duplicate, G_OBJECT_TYPE_NAME( duplicate ));*/
				append_item( model, treeview, parent, &iter, duplicate );
				g_object_unref( duplicate );
			}
			subitems = na_object_get_items( duplicate );
			fill_tree_store( model, treeview, subitems, only_actions, only_actions ? NULL : &iter );
			na_object_free_items( subitems );
		}

		if( NA_IS_OBJECT_ACTION( object )){
			duplicate = parent ? g_object_ref( object ) : na_object_duplicate( object );
			/*g_debug( "%s: appending duplicate=%p (%s)", thisfn, ( void * ) duplicate, G_OBJECT_TYPE_NAME( duplicate ));*/
			append_item( model, treeview, parent, &iter, duplicate );
			g_object_unref( duplicate );
			if( !only_actions ){
				subitems = na_object_get_items( duplicate );
				fill_tree_store( model, treeview, subitems, only_actions, &iter );
				na_object_free_items( subitems );
			}
			g_return_if_fail( NA_IS_OBJECT_ACTION( duplicate ));
			g_return_if_fail( na_object_get_items_count( duplicate ) >= 1 );
		}

		if( NA_IS_OBJECT_PROFILE( object )){
			g_assert( !only_actions );
			/*g_debug( "%s: appending object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/
			append_item( model, treeview, parent, &iter, object );
		}
	}
}

/**
 * nact_tree_model_insert:
 * @model: this #NactTreeModel instance.
 * @object: a #NAObject-derived object to be inserted.
 * @path: the #GtkTreePath of the beginning of the current selection,
 * or NULL.
 * @obj_parent: set to the parent or the object itself.
 *
 * Insert a new row at the given position.
 *
 * Returns: the path string of the inserted row as a newly allocated
 * string. The returned path should be g_free() by the caller.
 *
 * +--------------------+----------------------+----------------------+----------------------+
 * | inserted object -> |        action        |        profile       |         menu         |
 * +--------------------+----------------------+----------------------+----------------------+
 * | currently selected |                      |                      |                      |
 * |      |             |                      |                      |                      |
 * |      v             |                      |                      |                      |
 * |    (nil)           |    insert_before     |          n/a         |    insert_before     |
 * |                    |                      |                      |                      |
 * |   action           |    insert_before     | insert_as_last_child |    insert_before     |
 * |                    |                      |                      |                      |
 * |   profile          | insert_before_parent |    insert_before     | insert_before_parent |
 * |                    |                      |                      |                      |
 * |    menu            | insert_as_last_child |          n/a         | insert_as_last_child |
 * +-----------------------------------------------------------------------------------------+
 *
 * insert_before       : parent=NULL     , sibling_from_path (or null if path was null)
 * insert_before_parent: parent=NULL     , sibling_from_parent_path
 * insert_as_last_child: parent_from_path, sibling=NULL
 *
 * Gtk API uses to returns iter ; but at least when inserting a new profile in an action, we
 * may have store_iter_path="0:1" (good), but iter_path="0:0" (bad) - so we return rather a
 * string path
 */
gchar *
nact_tree_model_insert( NactTreeModel *model, const NAObject *object, GtkTreePath *path, NAObject **obj_parent )
{
	static const gchar *thisfn = "nact_tree_model_insert";
	gchar *path_str = NULL;
	GtkTreeModel *store;
	GtkTreeIter select_iter;
	NAObject *select_object;
	GtkTreeIter parent_iter;
	GtkTreeIter sibling_iter;
	GtkTreeIter store_iter;
	gboolean has_parent_iter;
	gboolean has_sibling_iter;

	path_str = path ? gtk_tree_path_to_string( path ) : NULL;
	g_debug( "%s: model=%p, object=%p (%s), path=%p (%s)",
			thisfn, ( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ),
			( void * ) path, path_str );
	g_free( path_str );

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		has_parent_iter = FALSE;
		has_sibling_iter = FALSE;
		*obj_parent = NA_OBJECT( object );

		remove_if_exists( model, store, object );

		if( path ){
			gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &select_iter, path );
			gtk_tree_model_get( GTK_TREE_MODEL( model ), &select_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &select_object, -1 );

			g_return_val_if_fail( select_object, NULL );
			g_return_val_if_fail( NA_IS_OBJECT( select_object ), NULL );

			if( NA_IS_OBJECT_ACTION( object )){
				insert_get_iters_action( store, select_object, path, object, &parent_iter, &has_parent_iter, &sibling_iter, &has_sibling_iter, obj_parent );
			}

			if( NA_IS_OBJECT_PROFILE( object )){
				insert_get_iters_profile( store, select_object, path, object, &parent_iter, &has_parent_iter, &sibling_iter, &has_sibling_iter, obj_parent );
			}

			if( NA_IS_OBJECT_MENU( object )){
				insert_get_iters_menu( store, select_object, path, object, &parent_iter, &has_parent_iter, &sibling_iter, &has_sibling_iter, obj_parent );
			}

			g_object_unref( select_object );

		} else {
			g_return_val_if_fail( NA_IS_OBJECT_ITEM( object ), NULL );
		}

		gtk_tree_store_insert_before( GTK_TREE_STORE( store ), &store_iter, has_parent_iter ? &parent_iter : NULL, has_sibling_iter ? &sibling_iter : NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &store_iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &store_iter, object );

		path_str = gtk_tree_model_get_string_from_iter( store, &store_iter );
	}

	return( path_str );
}

/*
 * inserts an action
 */
static void
insert_get_iters_action( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	g_return_if_fail( NA_IS_OBJECT_ACTION( object ));

	/* (insert_before)
	 * insert action before selected action
	 * if selected action has a parent :
	 * - set parent_object to parent of selected action
	 * - add object to subitems of parent of selected action
	 */
	if( NA_IS_OBJECT_ACTION( select_object )){
		insert_before_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	/* (insert_before_parent)
	 * insert action before parent of selected profile
	 * if parent of selected profile has itself a parent :
	 * - set parent_object to parent of parent of selected profile
	 * - add object to subitems of parent of parent of selected profile
	 */
	if( NA_IS_OBJECT_PROFILE( select_object )){
		insert_before_parent_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	/* (insert_as_last_child)
	 */
	if( NA_IS_OBJECT_MENU( select_object )){
		insert_as_last_child_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}
}

/*
 * insert a profile
 */
static void
insert_get_iters_profile( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));

	if( NA_IS_OBJECT_ACTION( select_object )){
		insert_as_last_child_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	if( NA_IS_OBJECT_PROFILE( select_object )){
		insert_before_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	if( NA_IS_OBJECT_MENU( select_object )){
		g_return_if_reached();
	}
}

/*
 * insert a menu
 */
static void
insert_get_iters_menu( GtkTreeModel *model, const NAObject *select_object, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	g_return_if_fail( NA_IS_OBJECT_MENU( object ));

	if( NA_IS_OBJECT_ACTION( select_object )){
		insert_before_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	if( NA_IS_OBJECT_PROFILE( select_object )){
		insert_before_parent_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}

	if( NA_IS_OBJECT_MENU( select_object )){
		insert_as_last_child_get_iters( model, select_path, object, parent_iter, has_parent_iter, sibling_iter, has_sibling_iter, parent_object );
	}
}

/*
 * insert an action or a menu when there is no current selection
 * insert an action or a menu when the selection is an action
 * insert a profile before a profile
 */
static void
insert_before_get_iters( GtkTreeModel *model,  GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *sibling_obj;

	g_debug( "nact_tree_model_insert_before_get_iters" );

	gtk_tree_model_get_iter( model, sibling_iter, select_path );
	gtk_tree_model_get( model, sibling_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &sibling_obj, -1 );
	*has_sibling_iter = TRUE;

	if( gtk_tree_path_get_depth( select_path ) > 1 ){
		path = gtk_tree_path_copy( select_path );
		gtk_tree_path_up( path );
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, parent_object, -1 );
		g_return_if_fail( NA_IS_OBJECT_ITEM( *parent_object ));
		na_object_insert_item( *parent_object, object, sibling_obj );
		g_object_unref( *parent_object );
		gtk_tree_path_free( path );
	}

	g_object_unref( sibling_obj );
}

/*
 * insert an action or a menu when the selection is a profile
 */
static void
insert_before_parent_get_iters( GtkTreeModel *model, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *sibling_obj;

	g_debug( "nact_tree_model_insert_before_parent_get_iters" );

	path = gtk_tree_path_copy( select_path );
	gtk_tree_path_up( path );
	gtk_tree_model_get_iter( model, sibling_iter, path );
	gtk_tree_model_get( model, sibling_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &sibling_obj, -1 );
	*has_sibling_iter = TRUE;

	if( gtk_tree_path_get_depth( path ) > 1 ){
		gtk_tree_path_up( path );
		gtk_tree_model_get_iter( model, &iter, path );
		gtk_tree_model_get( model, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, parent_object, -1 );
		g_return_if_fail( NA_IS_OBJECT_ITEM( *parent_object ));
		na_object_insert_item( *parent_object, object, sibling_obj );
		g_object_unref( *parent_object );
	}

	g_object_unref( sibling_obj );
	gtk_tree_path_free( path );
}

/*
 * insert an action or a menu when the selection is a menu
 * insert a profile when the selection is an action
 */
static void
insert_as_last_child_get_iters( GtkTreeModel *model, GtkTreePath *select_path, const NAObject *object, GtkTreeIter *parent_iter, gboolean *has_parent_iter, GtkTreeIter *sibling_iter, gboolean *has_sibling_iter, NAObject **parent_object )
{
	g_debug( "nact_tree_model_insert_as_last_child_get_iters" );

	gtk_tree_model_get_iter( model, parent_iter, select_path );
	*has_parent_iter = TRUE;

	gtk_tree_model_get( model, parent_iter, IACTIONS_LIST_NAOBJECT_COLUMN, parent_object, -1 );
	g_return_if_fail( NA_IS_OBJECT_ITEM( *parent_object ));
	na_object_append_item( *parent_object, object );
	g_object_unref( *parent_object );
}

/*
 * if the object, identified by its uuid, already exists, then remove it first
 */
static void
remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object )
{
	GtkTreeIter iter;

	if( NA_IS_OBJECT_ITEM( object )){
		if( search_for_object_id( model, store, object, &iter )){
			gtk_tree_store_remove( GTK_TREE_STORE( store ), &iter );
		}
	}
}

void
nact_tree_model_iter( NactTreeModel *model, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, fn, user_data );
	}
}

/**
 * nact_tree_model_remove:
 * @model: this #NactTreeModel instance.
 * @object: the #NAObject to be deleted.
 *
 * Recursively deletes the specified object.
 *
 * Returns: a path which may be suitable for the next selection.
 */
GtkTreePath *
nact_tree_model_remove( NactTreeModel *model, NAObject *object )
{
	GtkTreeIter iter;
	GtkTreeStore *store;
	GList *parents = NULL;
	GList *it;
	GtkTreePath *path = NULL;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			parents = add_parent( parents, GTK_TREE_MODEL( store ), &iter );
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			remove_items( store, &iter );
		}

		for( it = parents ; it ; it = it->next ){
			na_object_check_edition_status( it->data );
		}
	}

	return( path );
}

/*
 * iter is positionned on the row which is going to be deleted
 * remove the object from the subitems list of parent (if any)
 * add parent to the list to check its status after remove will be done
 */
static GList *
add_parent( GList *parents, GtkTreeModel *store, GtkTreeIter *obj_iter )
{
	NAObject *object;
	GtkTreePath *path;
	GtkTreeIter iter;
	NAObject *parent;

	gtk_tree_model_get( store, obj_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	path = gtk_tree_model_get_path( store, obj_iter );

	if( gtk_tree_path_get_depth( path ) > 1 ){
		gtk_tree_path_up( path );
		gtk_tree_model_get_iter( store, &iter, path );
		gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &parent, -1 );

		if( !g_list_find( parents, parent )){
			parents = g_list_prepend( parents, parent );
			na_object_remove_item( parent, object );
		}

		g_object_unref( parent );
	}

	gtk_tree_path_free( path );
	g_object_unref( object );

	return( parents );
}

static void
append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object )
{
	gtk_tree_store_append( model, iter, parent );

	gtk_tree_store_set( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );

	display_item( model, treeview, iter, object );
}

static void
display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object )
{
	gchar *label = na_object_get_label( object );
	gtk_tree_store_set( model, iter, IACTIONS_LIST_LABEL_COLUMN, label, -1 );
	g_free( label );

	if( NA_IS_OBJECT_ITEM( object )){
		GdkPixbuf *icon = na_object_item_get_pixbuf( NA_OBJECT_ITEM( object ), GTK_WIDGET( treeview ));
		gtk_tree_store_set( model, iter, IACTIONS_LIST_ICON_COLUMN, icon, -1 );
	}
}

static gboolean
dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm )
{
	gint depth;
	gint i;
	GString *prefix;
	gchar *id, *label;

	depth = gtk_tree_path_get_depth( path );
	prefix = g_string_new( ntm->prefix );
	for( i=1 ; i<depth ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	id = na_object_get_id( object );
	label = na_object_get_label( object );
	g_debug( "%s: %s%s at %p \"[%s] %s\"",
			ntm->fname, prefix->str, G_OBJECT_TYPE_NAME( object ), ( void * ) object, id, label );
	g_free( label );
	g_free( id );

	g_string_free( prefix, TRUE );

	/* don't stop iteration */
	return( FALSE );
}

static void
iter_on_store( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeIter iter;
	gboolean stop;

	if( gtk_tree_model_iter_children( store, &iter, parent )){
		stop = iter_on_store_item( model, store, &iter, fn, user_data );
		while( !stop && gtk_tree_model_iter_next( store, &iter )){
			stop = iter_on_store_item( model, store, &iter, fn, user_data );
		}
	}
}

static gboolean
iter_on_store_item( NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data )
{
	NAObject *object;
	GtkTreePath *path;
	gboolean stop;

	gtk_tree_model_get( store, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	path = gtk_tree_model_get_path( store, iter );

	stop = ( *fn )( model, path, object, user_data );

	gtk_tree_path_free( path );
	g_object_unref( object );

	if( !stop ){
		iter_on_store( model, store, iter, fn, user_data );
	}

	return( stop );
}

/*
 * recursively remove child items starting with iter
 * returns TRUE if iter is always valid after the remove
 */
static gboolean
remove_items( GtkTreeStore *store, GtkTreeIter *iter )
{
	GtkTreeIter child;
	gboolean valid;

	while( gtk_tree_model_iter_children( GTK_TREE_MODEL( store ), &child, iter )){
		remove_items( store, &child );
	}
	valid = gtk_tree_store_remove( store, iter );

	return( valid );
}

static gboolean
search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *result_iter )
{
	gboolean found = FALSE;
	ntmSearchStruct *ntm;
	GtkTreeIter iter;

	ntm = g_new0( ntmSearchStruct, 1 );
	ntm->store = store;
	ntm->object = object;
	ntm->found = FALSE;
	ntm->iter = &iter;

	iter_on_store( model, store, NULL, ( FnIterOnStore ) search_for_objet_iter, ntm );

	if( ntm->found ){
		found = TRUE;
		memcpy( result_iter, ntm->iter, sizeof( GtkTreeIter ));
	}

	g_free( ntm );
	return( found );
}

static gboolean
search_for_objet_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm )
{
	if( object == ntm->object ){
		if( gtk_tree_model_get_iter( ntm->store, ntm->iter, path )){
			ntm->found = TRUE;
		}
	}

	/* stop iteration when found */
	return( ntm->found );
}

static gboolean
search_for_object_id( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *result_iter )
{
	gboolean found = FALSE;
	ntmSearchIdStruct *ntm;
	GtkTreeIter iter;

	ntm = g_new0( ntmSearchIdStruct, 1 );
	ntm->store = store;
	ntm->id = na_object_get_id( object );
	ntm->found = FALSE;
	ntm->iter = &iter;

	iter_on_store( model, store, NULL, ( FnIterOnStore ) search_for_object_id_iter, ntm );

	if( ntm->found ){
		found = TRUE;
		memcpy( result_iter, ntm->iter, sizeof( GtkTreeIter ));
	}

	g_free( ntm->id );
	g_free( ntm );
	return( found );
}

static gboolean
search_for_object_id_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchIdStruct *ntm )
{
	gchar *id;

	id = na_object_get_id( object );

	if( !g_ascii_strcasecmp( id, ntm->id )){
		if( gtk_tree_model_get_iter( ntm->store, ntm->iter, path )){
			ntm->found = TRUE;
		}
	}

	g_free( id );

	/* stop iteration when found */
	return( ntm->found );
}

/*
 * all rows are draggable
 */
static gboolean
imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *path_list )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_row_draggable";
	/*FrWindow     *window;
	GtkTreeModel *model;
	GList        *scan;*/

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) path_list );

	/*window = g_object_get_data (G_OBJECT (drag_source), "FrWindow");
	g_return_val_if_fail (window != NULL, FALSE);

	model = fr_window_get_list_store (window);

	for (scan = path_list; scan; scan = scan->next) {
		GtkTreeRowReference *reference = scan->data;
		GtkTreePath         *path;
		GtkTreeIter          iter;
		FileData            *fdata;

		path = gtk_tree_row_reference_get_path (reference);
		if (path == NULL)
			continue;

		if (! gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &fdata,
				    -1);

		if (fdata != NULL)
			return TRUE;
	}*/

	return( TRUE );
}

/*
 * drag_data_get is called when we release the selected items onto the
 * destination
 *
 * if some rows are selected
 * here, we only provide id. of dragged rows :
 * 		M:uuid
 * 		A:uuid
 * 		P:uuid/name
 * this is suitable and sufficient for the internal clipboard
 *
 * when exporting to the outside, we should prepare to export the items
 */
static gboolean
imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source,
				   GdkDragContext         *context,
				   GtkSelectionData       *selection_data,
				   GList                  *path_list,
				   guint                   info )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_get";
	gchar *atom_name;
	NactTreeModel *model;
	GList *selected_items;
	gchar *data;
	gboolean ret = FALSE;
	gchar *dest_folder, *folder;
	gboolean is_writable;
	gboolean copy_data;

	atom_name = gdk_atom_name( selection_data->target );
	g_debug( "%s: drag_source=%p, context=%p, action=%d, selection_data=%p, path_list=%p, atom=%s",
			thisfn, ( void * ) drag_source, ( void * ) context, ( int ) context->suggested_action, ( void * ) selection_data, ( void * ) path_list,
			atom_name );
	g_free( atom_name );

	model = NACT_TREE_MODEL( drag_source );
	g_assert( model->private->window );

	selected_items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( model->private->window ));
	if( !selected_items ){
		return( FALSE );
	}
	if( !g_list_length( selected_items )){
		g_list_free( selected_items );
		return( FALSE );
	}

	switch( info ){
		case NACT_XCHANGE_FORMAT_NACT:
			copy_data = ( context->suggested_action == GDK_ACTION_COPY );
			nact_clipboard_get_data_for_intern_use( selected_items, copy_data );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) "", 0 );
			ret = TRUE;
			break;

		case NACT_XCHANGE_FORMAT_XDS:
			folder = get_xds_atom_value( context );
			dest_folder = na_utils_remove_last_level_from_path( folder );
			g_free( folder );
			is_writable = na_utils_is_writable_dir( dest_folder );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * )( is_writable ? "S" : "F" ), 1 );
			if( is_writable ){
				model->private->drag_dest_uri = g_strdup( dest_folder );
				model->private->drag_items = g_list_copy( selected_items );
			}
			g_free( dest_folder );
			ret = TRUE;
			break;

		case NACT_XCHANGE_FORMAT_APPLICATION_XML:
		case NACT_XCHANGE_FORMAT_TEXT_PLAIN:
			data = nact_clipboard_get_data_for_extern_use( selected_items );
			gtk_selection_data_set( selection_data, selection_data->target, 8, ( guchar * ) data, strlen( data ));
			g_free( data );
			ret = TRUE;
			break;

		default:
			break;
	}

	na_object_free_items( selected_items );
	return( ret );
}

static gboolean
imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *path_list )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_drag_data_delete";

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) path_list );

	return( TRUE );
}

static GtkTargetList *
imulti_drag_source_get_target_list( EggTreeMultiDragSource *drag_source )
{
	GtkTargetList *target_list;

	target_list = gtk_target_list_new( dnd_source_formats, G_N_ELEMENTS( dnd_source_formats ));

	return( target_list );
}

static GdkDragAction
imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source )
{
	return( GDK_ACTION_COPY | GDK_ACTION_MOVE );
}

/*
 * TODO: empty the internal clipboard at drop time
 */
static gboolean
idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_drag_data_received";

	g_debug( "%s: drag_dest=%p, dest=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest, ( void * ) selection_data );

	return( FALSE );
}

static gboolean
idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_row_drop_possible";

	g_debug( "%s: drag_dest=%p, dest_path=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest_path, ( void * ) selection_data );

	return( TRUE );
}

static gboolean
on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_begin";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_list_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_change(
			context->source_window,
			XDS_ATOM, TEXT_ATOM, 8, GDK_PROP_MODE_REPLACE, ( guchar * ) XDS_FILENAME, strlen( XDS_FILENAME ));

	return( FALSE );
}

static void
on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_end";
	NactTreeModel *model;

	g_debug( "%s: widget=%p, context=%p, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, ( void * ) window );

	model = NACT_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));

	if( model->private->drag_dest_uri && model->private->drag_items && g_list_length( model->private->drag_items )){
		nact_clipboard_export_items( model->private->drag_dest_uri, model->private->drag_items );
	}

	g_free( model->private->drag_dest_uri );
	model->private->drag_dest_uri = NULL;

	g_list_free( model->private->drag_items );
	model->private->drag_items = NULL;

	gdk_property_delete( context->source_window, XDS_ATOM );
}

/*static gboolean
on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	static const gchar *thisfn = "nact_clipboard_on_drag_drop";

	g_debug( "%s: widget=%p, context=%p, x=%d, y=%d, time=%d, window=%p",
			thisfn, ( void * ) widget, ( void * ) context, x, y, time, ( void * ) window );
*/
	/*No!*/
	/*gtk_drag_get_data( widget, context, NACT_ATOM, time );*/

	/* return TRUE is the mouse pointer is on a drop zone, FALSE else */
	/*return( TRUE );
}*/

/*static void
on_drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *data, guint info, guint time, BaseWindow *window )
{
	static const gchar *thisfn = "nact_tree_model_on_drag_data_received";

	g_debug( "%s: widget=%p, drag_context=%p, x=%d, y=%d, selection_data=%p, info=%d, time=%d, window=%p",
			thisfn, ( void * ) widget, ( void * ) drag_context, x, y, ( void * ) data, info, time, ( void * ) window );
}*/

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_tree_model_sort_actions_list";*/
	NAObjectId *obj_a, *obj_b;
	gint ret;

	/*g_debug( "%s: model=%p, a=%p, b=%p, window=%p", thisfn, ( void * ) model, ( void * ) a, ( void * ) b, ( void * ) window );*/

	gtk_tree_model_get( model, a, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_a, -1 );
	gtk_tree_model_get( model, b, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_b, -1 );

	ret = na_pivot_sort_alpha_asc( obj_a, obj_b );

	g_object_unref( obj_b );
	g_object_unref( obj_a );

	/*g_debug( "%s: ret=%d", thisfn, ret );*/
	return( ret );
}

static gboolean
filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model )
{
	/*static const gchar *thisfn = "nact_tree_model_filter_visible";*/
	NAObject *object;
	NAObjectAction *action;
	gboolean only_actions;
	gint count;

	/*g_debug( "%s: model=%p, iter=%p, window=%p", thisfn, ( void * ) model, ( void * ) iter, ( void * ) window );*/
	/*g_debug( "%s at %p", G_OBJECT_TYPE_NAME( model ), ( void * ) model );*/
	/* is a GtkTreeStore */

	gtk_tree_model_get( store, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( object ){
		/*na_object_dump( object );*/

		if( NA_IS_OBJECT_ACTION( object )){
			g_object_unref( object );
			return( TRUE );
		}

		only_actions = NACT_TREE_MODEL( model )->private->only_actions;

		if( !only_actions ){

			if( NA_IS_OBJECT_MENU( object )){
				g_object_unref( object );
				return( TRUE );
			}

			if( NA_IS_OBJECT_PROFILE( object )){
				action = na_object_profile_get_action( NA_OBJECT_PROFILE( object ));
				g_object_unref( object );
				count = na_object_get_items_count( action );
				/*g_debug( "action=%p: count=%d", ( void * ) action, count );*/
				/*return( TRUE );*/
				return( count > 1 );
			}

			g_assert_not_reached();
		}
	}

	return( FALSE );
}

/* The following function taken from bugzilla
 * (http://bugzilla.gnome.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL
 */
static char *
get_xds_atom_value (GdkDragContext *context)
{
	char *ret;

	g_return_val_if_fail (context != NULL, NULL);
	g_return_val_if_fail (context->source_window != NULL, NULL);

	gdk_property_get (context->source_window,
						XDS_ATOM, TEXT_ATOM,
						0, MAX_XDS_ATOM_VAL_LEN,
						FALSE, NULL, NULL, NULL,
						(unsigned char **) &ret);

	return ret;
}
