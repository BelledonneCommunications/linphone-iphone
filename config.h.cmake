/***************************************************************************
* config.h.cmake
* Copyright (C) 2014  Belledonne Communications, Grenoble France
*
****************************************************************************
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
****************************************************************************/

#define LINPHONE_MAJOR_VERSION ${LINPHONE_MAJOR_VERSION}
#define LINPHONE_MINOR_VERSION ${LINPHONE_MINOR_VERSION}
#define LINPHONE_MICRO_VERSION ${LINPHONE_MICRO_VERSION}
#define LINPHONE_VERSION "${LINPHONE_VERSION}"
#define LIBLINPHONE_VERSION "${LINPHONE_VERSION}"

#define LINPHONE_ALL_LANGS "${LINPHONE_ALL_LANGS}"

#define LINPHONE_PLUGINS_DIR "${LINPHONE_PLUGINS_DIR}"
#define LINPHONE_CONFIG_DIR "${LINPHONE_CONFIG_DIR}"

#define GETTEXT_PACKAGE "${GETTEXT_PACKAGE}"

#define PACKAGE_LOCALE_DIR "${PACKAGE_LOCALE_DIR}"
#define PACKAGE_DATA_DIR "${PACKAGE_DATA_DIR}"
#define PACKAGE_SOUND_DIR "${PACKAGE_SOUND_DIR}"

#cmakedefine BUILD_WIZARD
#cmakedefine HAVE_GTK_OSX 1
#cmakedefine HAVE_NOTIFY4
#cmakedefine HAVE_ZLIB 1
#cmakedefine HAVE_CU_GET_SUITE 1
#cmakedefine HAVE_CU_CURSES 1
#cmakedefine HAVE_LIBUDEV_H 0
#cmakedefine HAVE_LIME
#cmakedefine ENABLE_NLS 1
