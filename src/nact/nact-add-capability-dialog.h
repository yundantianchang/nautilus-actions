/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NACT_ADD_CAPABILITY_DIALOG_H__
#define __NACT_ADD_CAPABILITY_DIALOG_H__

/**
 * SECTION: nact_add_capability_dialog
 * @short_description: #NactAddCapabilityDialog class definition.
 * @include: nact/nact-add-capability-dialog.h
 *
 * The dialog let the user pick a capability from the default list
 * when adding a new capability to a profile (resp. an action, a menu).
 */

#include "base-dialog.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define NACT_TYPE_ADD_CAPABILITY_DIALOG                ( nact_add_capability_dialog_get_type())
#define NACT_ADD_CAPABILITY_DIALOG( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_TYPE_ADD_CAPABILITY_DIALOG, NactAddCapabilityDialog ))
#define NACT_ADD_CAPABILITY_DIALOG_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_TYPE_ADD_CAPABILITY_DIALOG, NactAddCapabilityDialogClass ))
#define NACT_IS_ADD_CAPABILITY_DIALOG( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_TYPE_ADD_CAPABILITY_DIALOG ))
#define NACT_IS_ADD_CAPABILITY_DIALOG_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_TYPE_ADD_CAPABILITY_DIALOG ))
#define NACT_ADD_CAPABILITY_DIALOG_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_TYPE_ADD_CAPABILITY_DIALOG, NactAddCapabilityDialogClass ))

typedef struct _NactAddCapabilityDialogPrivate         NactAddCapabilityDialogPrivate;

typedef struct {
	/*< private >*/
	BaseDialog                      parent;
	NactAddCapabilityDialogPrivate *private;
}
	NactAddCapabilityDialog;

typedef struct _NactAddCapabilityDialogClassPrivate    NactAddCapabilityDialogClassPrivate;

typedef struct {
	/*< private >*/
	BaseDialogClass                      parent;
	NactAddCapabilityDialogClassPrivate *private;
}
	NactAddCapabilityDialogClass;

GType  nact_add_capability_dialog_get_type( void );

gchar *nact_add_capability_dialog_run     ( NactMainWindow *parent,
													GSList *capabilities );

G_END_DECLS

#endif /* __NACT_ADD_CAPABILITY_DIALOG_H__ */
