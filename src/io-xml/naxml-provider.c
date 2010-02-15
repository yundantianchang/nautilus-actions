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

#include <api/na-iio-factory.h>
#include <api/na-iio-provider.h>

#include "naxml-provider.h"

/* private class data
 */
struct NaxmlProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NaxmlProviderPrivate {
	gboolean dispose_has_run;
};

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;

static void     class_init( NaxmlProviderClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static void     iio_provider_iface_init( NAIIOProviderInterface *iface );
static gchar   *iio_provider_get_id( const NAIIOProvider *provider );
static gchar   *iio_provider_get_name( const NAIIOProvider *provider );
static guint    iio_provider_get_version( const NAIIOProvider *provider );
static gboolean iio_provider_is_willing_to_write( const NAIIOProvider *instance );
static gboolean iio_provider_is_able_to_write( const NAIIOProvider *instance );
static guint    iio_provider_write_item( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );

static void     iio_factory_iface_init( NAIIOFactoryInterface *iface );
static guint    iio_factory_get_version( const NAIIOFactory *provider );

GType
naxml_provider_get_type( void )
{
	return( st_module_type );
}

void
naxml_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "naxml_provider_register_type";

	static GTypeInfo info = {
		sizeof( NaxmlProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NaxmlProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iio_factory_iface_info = {
		( GInterfaceInitFunc ) iio_factory_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NaxmlProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );

	g_type_module_add_interface( module, st_module_type, NA_IIO_FACTORY_TYPE, &iio_factory_iface_info );
}

static void
class_init( NaxmlProviderClass *klass )
{
	static const gchar *thisfn = "naxml_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NaxmlProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "naxml_provider_instance_init";
	NaxmlProvider *self;

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( instance ));
	self = NAXML_PROVIDER( instance );

	self->private = g_new0( NaxmlProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "naxml_provider_instance_dispose";
	NaxmlProvider *self;

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAXML_PROVIDER( object );

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
	NaxmlProvider *self;

	g_assert( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAXML_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "naxml_provider_iio_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_id = iio_provider_get_id;
	iface->get_name = iio_provider_get_name;
	iface->get_version = iio_provider_get_version;
	iface->read_items = NULL;
	iface->is_willing_to_write = iio_provider_is_willing_to_write;
	iface->is_able_to_write = iio_provider_is_able_to_write;
	iface->write_item = iio_provider_write_item;
	iface->delete_item = NULL;
}

static gchar *
iio_provider_get_id( const NAIIOProvider *provider )
{
	return( g_strdup( "na-xml" ));
}

static gchar *
iio_provider_get_name( const NAIIOProvider *provider )
{
	return( g_strdup( _( "Nautilus-Actions XML I/O Provider" )));
}

static guint
iio_provider_get_version( const NAIIOProvider *provider )
{
	return( 1 );
}

static gboolean
iio_provider_is_willing_to_write( const NAIIOProvider *instance )
{
	return( TRUE );
}

static gboolean
iio_provider_is_able_to_write( const NAIIOProvider *instance )
{
	return( TRUE );
}

static guint
iio_provider_write_item( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages )
{
	return( NA_IIO_PROVIDER_CODE_OK );
}

static void
iio_factory_iface_init( NAIIOFactoryInterface *iface )
{
	static const gchar *thisfn = "naxml_provider_iio_factory_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_version = iio_factory_get_version;
}

static guint
iio_factory_get_version( const NAIIOFactory *provider )
{
	return( 1 );
}
