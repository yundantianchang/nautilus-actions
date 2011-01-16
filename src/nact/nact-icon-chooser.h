/*
 * Nautilus Actions
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

#ifndef __NACT_ICON_CHOOSER_H__
#define __NACT_ICON_CHOOSER_H__

/**
 * SECTION: nact_icon_chooser
 * @short_description: NactIconChooser dialog box
 * @include: nact/nact-icon-chooser.h
 *
 * This class is derived from BaseDialog.
 */

#include "base-dialog.h"

G_BEGIN_DECLS

#define NACT_ICON_CHOOSER_TYPE                ( nact_icon_chooser_get_type())
#define NACT_ICON_CHOOSER( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, NACT_ICON_CHOOSER_TYPE, NactIconChooser ))
#define NACT_ICON_CHOOSER_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, NACT_ICON_CHOOSER_TYPE, NactIconChooserClass ))
#define NACT_IS_ICON_CHOOSER( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, NACT_ICON_CHOOSER_TYPE ))
#define NACT_IS_ICON_CHOOSER_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), NACT_ICON_CHOOSER_TYPE ))
#define NACT_ICON_CHOOSER_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), NACT_ICON_CHOOSER_TYPE, NactIconChooserClass ))

typedef struct _NactIconChooserPrivate        NactIconChooserPrivate;
typedef struct _NactIconChooserClassPrivate   NactIconChooserClassPrivate;

typedef struct {
	BaseDialog              parent;
	NactIconChooserPrivate *private;
}
	NactIconChooser;

typedef struct {
	BaseDialogClass              parent;
	NactIconChooserClassPrivate *private;
}
	NactIconChooserClass;

GType  nact_icon_chooser_get_type( void );

gchar *nact_icon_chooser_choose_icon( BaseWindow *main_window, const gchar *icon_name );

G_END_DECLS

#endif /* __NACT_ICON_CHOOSER_H__ */