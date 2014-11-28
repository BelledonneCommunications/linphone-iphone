############################################################################
# FindNotify.cmake
# Copyright (C) 2014  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################
#
# - Find the notify include file and library
#
#  NOTIFY_FOUND - system has linphone
#  NOTIFY_INCLUDE_DIRS - the linphone include directory
#  NOTIFY_LIBRARIES - The libraries needed to use linphone

set(_NOTIFY_ROOT_PATHS
	${WITH_NOTIFY}
	${CMAKE_INSTALL_PREFIX}
)

find_path(NOTIFY_INCLUDE_DIRS
	NAMES libnotify/notify.h
	HINTS _NOTIFY_ROOT_PATHS
	PATH_SUFFIXES include
)

if(NOTIFY_INCLUDE_DIRS)
	set(HAVE_LIBNOTIFY_NOTIFY_H 1)
endif()

find_library(NOTIFY_LIBRARIES
	NAMES notify
	HINTS ${_NOTIFY_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Notify
	DEFAULT_MSG
	NOTIFY_INCLUDE_DIRS NOTIFY_LIBRARIES HAVE_LIBNOTIFY_NOTIFY_H
)

mark_as_advanced(NOTIFY_INCLUDE_DIRS NOTIFY_LIBRARIES HAVE_LIBNOTIFY_NOTIFY_H)
