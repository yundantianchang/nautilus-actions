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

#ifndef __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__
#define __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__

/**
 * SECTION: na_iexporter
 * @short_description: #NAIExporter interface definition.
 * @include: nautilus-actions/na-iexporter.h
 *
 * The #NAIExporter interface exports items to the outside world.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include "na-object-item.h"

G_BEGIN_DECLS

#define NA_IEXPORTER_TYPE						( na_iexporter_get_type())
#define NA_IEXPORTER( instance )				( G_TYPE_CHECK_INSTANCE_CAST( instance, NA_IEXPORTER_TYPE, NAIExporter ))
#define NA_IS_IEXPORTER( instance )				( G_TYPE_CHECK_INSTANCE_TYPE( instance, NA_IEXPORTER_TYPE ))
#define NA_IEXPORTER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IEXPORTER_TYPE, NAIExporterInterface ))

typedef struct NAIExporter                 NAIExporter;

typedef struct NAIExporterInterfacePrivate NAIExporterInterfacePrivate;

/* When listing available export formats, the instance returns a GList
 * of these structures
 */
typedef struct {
	gchar *format;					/* format identifier (ascii) */
	gchar *dlg_label;				/* label to be displayed in the NactExportAsk dialog (UTF-8 locale) */
	gchar *wnd_label;				/* short label to be displayed in the UI (UTF-8 locale) */
	gchar *description;				/* full description of the format (UTF-8 locale) */
}
	NAExporterStr;

typedef struct {
	GTypeInterface             parent;
	NAIExporterInterfacePrivate *private;

	/**
	 * get_version:
	 * @instance: this #NAIExporter instance.
	 *
	 * Returns: the version of this interface supported by the I/O provider.
	 *
	 * Defaults to 1.
	 */
	guint                 ( *get_version )( const NAIExporter *instance );

	/**
	 * get_formats:
	 * @instance: this #NAIExporter instance.
	 *
	 * Returns: a list of #NAExporterStr structures which describe the
	 * formats supported by @instance.
	 *
	 * Defaults to %NULL (no format at all).
	 *
	 * The returned list is owned by the @instance. It must not be
	 * released by the caller.
	 *
	 * To avoid any collision, the format id is allocated by the
	 * Nautilus-Actions maintainer team. If you wish develop a new
	 * export format, and so need a new format id, please contact the
	 * maintainers (see #nautilus-actions.doap).
	 */
	const NAExporterStr * ( *get_formats )( const NAIExporter *instance );

	/**
	 * to_file:
	 * @instance: this #NAIExporter instance.
	 * @item: a #NAObjectItem-derived object.
	 * @uri: the target directory URI.
	 * @format: the target format.
	 * @fname: the place where allocate a new string to store the output
	 * filename URI.
	 *
	 * Exports the specified @item to the target @uri in the required
	 * @format.
	 *
	 * Returns: the status of the operation.
	 */
	guint                 ( *to_file )    ( const NAIExporter *instance, const NAObjectItem *item, const gchar *uri, const gchar *format, gchar **fname );

	/**
	 * to_buffer:
	 * @instance: this #NAIExporter instance.
	 * @item: a #NAObjectItem-derived object.
	 * @format: the target format.
	 * @buffer: the place where allocate a new buffer to store the output.
	 *
	 * Exports the specified @item to the target @buffer in the required
	 * @format.
	 *
	 * Returns: the status of the operation.
	 */
	guint                 ( *to_buffer )  ( const NAIExporter *instance, const NAObjectItem *item, const gchar *format, gchar **buffer );
}
	NAIExporterInterface;

GType na_iexporter_get_type( void );

/* The reasons for which an item may not have been exported
 */
enum {
	NA_IEXPORTER_CODE_OK = 0,
	NA_IEXPORTER_CODE_INVALID_ITEM,
	NA_IEXPORTER_CODE_INVALID_TARGET,
	NA_IEXPORTER_CODE_INVALID_FORMAT,
	NA_IEXPORTER_CODE_UNABLE_TO_WRITE,
	NA_IEXPORTER_CODE_ERROR,
};

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_IEXPORTER_H__ */