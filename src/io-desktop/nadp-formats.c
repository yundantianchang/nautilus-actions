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

#include <api/na-iexporter.h>

#include "nadp-formats.h"

NAIExporterFormat nadp_formats[] = {

	/* DESKTOP_V1: the initial desktop format as described in
	 * http://www.nautilus-actions.org/?q=node/377
	 */
	{ NADP_FORMAT_DESKTOP_V1,
			N_( "Export as a ._desktop file" ),
			N_( "This format let you easily share your actions with others, including other desktop environments.\n" \
				"The exported .desktop file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- drag-n-drop into the Nautilus Actions Configuration Tool,\n" \
				"- or by copying it into a XDG_DATA_DIRS/file-manager/actions directory." ) },

	{ NULL }
};