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

#ifndef __NAUTILUS_ACTIONS_API_NA_OBJECT_PROFILE_H__
#define __NAUTILUS_ACTIONS_API_NA_OBJECT_PROFILE_H__

/**
 * SECTION: na_object_profile
 * @short_description: #NAObjectProfile class definition.
 * @include: nautilus-actions/na-object-item.h
 *
 * This is a pure virtual class, i.e. not an instantiatable one, but
 * serves as the base class for #NAObjectAction and #NAObjectMenu.
 */

#include "na-object-id.h"

G_BEGIN_DECLS

#define NA_OBJECT_PROFILE_TYPE					( na_object_profile_get_type())
#define NA_OBJECT_PROFILE( object )				( G_TYPE_CHECK_INSTANCE_CAST( object, NA_OBJECT_PROFILE_TYPE, NAObjectProfile ))
#define NA_OBJECT_PROFILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST( klass, NA_OBJECT_PROFILE_TYPE, NAObjectProfileClass ))
#define NA_IS_OBJECT_PROFILE( object )			( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_OBJECT_PROFILE_TYPE ))
#define NA_IS_OBJECT_PROFILE_CLASS( klass )		( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_OBJECT_PROFILE_TYPE ))
#define NA_OBJECT_PROFILE_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_OBJECT_PROFILE_TYPE, NAObjectProfileClass ))

typedef struct NAObjectProfilePrivate      NAObjectProfilePrivate;

typedef struct {
	NAObjectId              parent;
	NAObjectProfilePrivate *private;
}
	NAObjectProfile;

typedef struct NAObjectProfileClassPrivate NAObjectProfileClassPrivate;

typedef struct {
	NAObjectIdClass              parent;
	NAObjectProfileClassPrivate *private;
}
	NAObjectProfileClass;

/* default prefix for profile name
 */
#define NA_PROFILE_DEFAULT_PREFIX		"profile-"

GType            na_object_profile_get_type( void );

NAObjectProfile *na_object_profile_new( void );

void             na_object_profile_set_scheme( NAObjectProfile *profile, const gchar *scheme, gboolean selected );
void             na_object_profile_replace_folder( NAObjectProfile *profile, const gchar *old, const gchar *new );

gboolean         na_object_profile_is_candidate                ( const NAObjectProfile *profile, gint target, GList *files );
gboolean         na_object_profile_is_candidate_for_tracked    ( const NAObjectProfile *profile, GList *tracked );

gchar           *na_object_profile_parse_parameters            ( const NAObjectProfile *profile, gint target, GList *files );
gchar           *na_object_profile_parse_parameters_for_tracked( const NAObjectProfile *profile, GList *tracked );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_OBJECT_PROFILE_H__ */
