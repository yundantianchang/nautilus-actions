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

#ifndef __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__
#define __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__

/**
 * SECTION: na_iio_provider
 * @short_description: #NAIIOProvider interface definition.
 * @include: nautilus-actions/api/na-iio-provider.h
 *
 * This is the API all I/O Providers should implement in order to
 * provide I/O storage resources to Nautilus-Actions.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include <nautilus-actions/private/na-object-item-class.h>

G_BEGIN_DECLS

#define NA_IIO_PROVIDER_TYPE						( na_iio_provider_get_type())
#define NA_IIO_PROVIDER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IIO_PROVIDER_TYPE, NAIIOProvider ))
#define NA_IS_IIO_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IIO_PROVIDER_TYPE ))
#define NA_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_PROVIDER_TYPE, NAIIOProviderInterface ))

typedef struct NAIIOProvider                 NAIIOProvider;

typedef struct NAIIOProviderInterfacePrivate NAIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIIOProviderInterfacePrivate *private;

	/*
	 * This is the API the I/O providers have to implement.
	 */

	/**
	 * get_id:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the id of the IO provider as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * To avoid any collision, the IO provider id is allocated by the
	 * Nautilus-Actions maintainer team. If you wish develop a new IO
	 * provider, and so need a new provider id, please contact the
	 * maintainers (see nautilus-actions.doap).
	 *
	 * The I/O provider must implement this function.
	 */
	gchar *  ( *get_id )             ( const NAIIOProvider *instance );

	/**
	 * get_name:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the name to be displayed for this I/O provider as a
	 * newly allocated string which should be g_free() by the caller.
	 *
	 * Defaults to an empty string.
	 */
	gchar *  ( *get_name )           ( const NAIIOProvider *instance );

	/**
	 * get_version:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the version of this API supported by the IO provider.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )        ( const NAIIOProvider *instance );

	/**
	 * read_items:
	 * @instance: the #NAIIOProvider provider.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Reads the whole items list from the specified I/O provider.
	 *
	 * Returns: a unordered flat #GList of #NAObjectItem-derived objects
	 * (menus or actions) ; the actions embed their own profiles.
	 */
	GList *  ( *read_items )         ( const NAIIOProvider *instance, GSList **messages );

	/**
	 * is_willing_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: %TRUE if this I/O provider is willing to write,
	 *  %FALSE else.
	 *
	 * The 'willing_to_write' property is intrinsic to the I/O provider.
	 * It shouldn't do any supposition against the actual runtime
	 * environment.
	 * It just says that the developer/maintainer has released the needed
	 * code in order to update/create/delete #NAObjectItem-derived objects.
	 *
	 * Note that even if this property is %TRUE, there is yet several
	 * reasons for not being able to update/delete existing items or
	 * create new ones (see e.g. #is_able_to_write() below).
	 */
	gboolean ( *is_willing_to_write )( const NAIIOProvider *instance );

	/**
	 * is_able_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: %TRUE if this I/O provider is able to do write
	 * operations at runtime, %FALSE else.
	 *
	 * The 'able_to_write' property is a runtime one.
	 * When returning %TRUE, the I/O provider insures that it has
	 * sucessfully checked that it was able to write some things
	 * down to its storage subsystems.
	 *
	 * The 'able_to_write' property is independant of the
	 * 'willing_to_write' above.
	 *
	 * This condition is only relevant when trying to define new items,
	 * to see if a willing_to provider is actually able to do write
	 * operations. It it not relevant for updating/deleting already
	 * existings items as they have already checked their own runtime
	 * writability status when readen from the storage subsystems.
	 *
	 * Note that even if this property is %TRUE, there is yet several
	 * reasons for not being able to update/delete existing items or
	 * create new ones (see e.g. #is_willing_to_write() above).
	 */
	gboolean ( *is_able_to_write )   ( const NAIIOProvider *instance );

	/**
	 * write_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived menu or action.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Writes a new @item.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the write operation
	 * was successfull, or another code depending of the detected error.
	 *
	 * Note: there is no update_item function ; it is the responsability
	 * of the provider to delete the previous version of an item before
	 * writing the new version.
	 */
	guint    ( *write_item )         ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );

	/**
	 * delete_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived menu or action.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Deletes an existing @item from the I/O subsystem.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the delete operation was
	 * successfull, or another code depending of the detected error.
	 */
	guint    ( *delete_item )        ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );
}
	NAIIOProviderInterface;

GType na_iio_provider_get_type       ( void );

/* This function is to be called by the I/O provider when it detects
 * that the specified object has been modified in its underlying storage
 * subsystem.
 */
void  na_iio_provider_config_changed ( const NAIIOProvider *instance, const gchar *id );

/* the reasons for which an item may not be writable
 * adding a new status here should imply also adding a new tooltip
 * in nact_main_statusbar_set_locked().
 */
enum {
	NA_IIO_PROVIDER_STATUS_UNDETERMINED = 0,
	NA_IIO_PROVIDER_STATUS_WRITABLE,
	NA_IIO_PROVIDER_STATUS_ITEM_READONLY,
	NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO,
	NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND,
	NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN,
	NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER,
	NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN,
	NA_IIO_PROVIDER_STATUS_NO_API,
	NA_IIO_PROVIDER_STATUS_LAST,
};

/* return code of operations
 */
enum {
	NA_IIO_PROVIDER_CODE_OK = 0,
	NA_IIO_PROVIDER_CODE_PROGRAM_ERROR = 1 + NA_IIO_PROVIDER_STATUS_LAST,
	NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN,
	NA_IIO_PROVIDER_CODE_WRITE_ERROR,
	NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR,
	NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR,
};

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__ */