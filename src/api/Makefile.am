# Nautilus Actions
# A Nautilus extension which offers configurable context menu actions.
#
# Copyright (C) 2005 The GNOME Foundation
# Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
# Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
#
# This Program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This Program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this Library; see the file COPYING.  If not,
# write to the Free Software Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA 02111-1307, USA.
#
# Authors:
#   Frederic Ruaudel <grumz@grumz.net>
#   Rodrigo Moya <rodrigo@gnome-db.org>
#   Pierre Wieser <pwieser@trychlos.org>
#   ... and many others (see AUTHORS)

pkglib_LTLIBRARIES=libna-api.la

AM_CPPFLAGS += \
	-I $(top_srcdir)									\
	$(NAUTILUS_ACTIONS_CFLAGS)							\
	-DG_LOG_DOMAIN=\"${NA_LOGDOMAIN_API}\"				\
	$(NULL)

libna_api_includedir = $(pkgincludedir)/api

libna_api_include_HEADERS = \
	na-api.h											\
	na-dbus.h											\
	na-gconf-keys.h										\
	na-gconf-monitor.h									\
	na-iio-provider.h									\
	na-object-api.h										\
	$(NULL)

libna_api_la_SOURCES = \
	na-gconf-monitor.c									\
	na-iio-provider.c									\
	$(NULL)

libna_api_la_LIBADD = \
	$(NULL)

#VERSION_INFO = `echo @VERSION@ | $(SED) -e 's/\./:/g' -e 's/[a-aA-Z]//g'`
VERSION_INFO = 1:0:0
libna_api_la_LDFLAGS = \
	-module 											\
	-no-undefined										\
	-version-number $(VERSION_INFO)						\
	$(NULL)