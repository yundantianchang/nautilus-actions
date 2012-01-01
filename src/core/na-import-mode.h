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

#ifndef __CORE_NA_IMPORT_MODE_H__
#define __CORE_NA_IMPORT_MODE_H__

/* @title: NAImportMode
 * @short_description: The #NAImportMode Class Definition
 * @include: core/na-import-mode.h
 *
 * This class gathers and manages the different import modes we are able
 * to deal with.
 */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

G_BEGIN_DECLS

#define NA_IMPORT_MODE_TYPE           ( na_import_mode_get_type())
#define NA_IMPORT_MODE( o )           ( G_TYPE_CHECK_INSTANCE_CAST( o, NA_IMPORT_MODE_TYPE, NAImportMode ))
#define NA_IMPORT_MODE_CLASS( c )     ( G_TYPE_CHECK_CLASS_CAST( c, NA_IMPORT_MODE_TYPE, NAImportModeClass ))
#define NA_IS_IMPORT_MODE( o )        ( G_TYPE_CHECK_INSTANCE_TYPE( o, NA_IMPORT_MODE_TYPE ))
#define NA_IS_IMPORT_MODE_CLASS( c )  ( G_TYPE_CHECK_CLASS_TYPE(( c ), NA_IMPORT_MODE_TYPE ))
#define NA_IMPORT_MODE_GET_CLASS( o ) ( G_TYPE_INSTANCE_GET_CLASS(( o ), NA_IMPORT_MODE_TYPE, NAImportModeClass ))

typedef struct _NAImportModePrivate      NAImportModePrivate;

typedef struct {
	GObject              parent;
	NAImportModePrivate *private;
}
	NAImportMode;

typedef struct _NAImportModeClassPrivate NAImportModeClassPrivate;

typedef struct {
	GObjectClass              parent;
	NAImportModeClassPrivate *private;
}
	NAImportModeClass;

GType         na_import_mode_get_type( void );

/* NAImportMode properties
 *
 */
#define NA_IMPORT_PROP_MODE				"na-import-mode-prop-mode"
#define NA_IMPORT_PROP_LABEL			"na-import-mode-prop-label"
#define NA_IMPORT_PROP_DESCRIPTION		"na-import-mode-prop-description"
#define NA_IMPORT_PROP_IMAGE			"na-import-mode-prop-image"

NAImportMode *na_import_mode_new     ( guint mode_id );

guint         na_import_mode_get_id  ( const NAImportMode *mode );

G_END_DECLS

#endif /* __CORE_NA_IMPORT_MODE_H__ */
