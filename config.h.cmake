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
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
****************************************************************************/

#define LINPHONE_MAJOR_VERSION ${LINPHONE_MAJOR_VERSION}
#define LINPHONE_MINOR_VERSION ${LINPHONE_MINOR_VERSION}
#define LINPHONE_MICRO_VERSION ${LINPHONE_MICRO_VERSION}
#define LINPHONE_VERSION "${LINPHONE_VERSION}"
#define LIBLINPHONE_VERSION "${LINPHONE_VERSION}"

#define LINPHONE_ALL_LANGS "${LINPHONE_ALL_LANGS}"

#define LINPHONE_PLUGINS_DIR "${LINPHONE_PLUGINS_DIR}"

#define GETTEXT_PACKAGE "${GETTEXT_PACKAGE}"

#define PACKAGE_LOCALE_DIR "${PACKAGE_LOCALE_DIR}"
#define PACKAGE_DATA_DIR "${PACKAGE_DATA_DIR}"
#define PACKAGE_SOUND_DIR "${PACKAGE_SOUND_DIR}"

#cmakedefine HAVE_NOTIFY4
#cmakedefine HAVE_CU_GET_SUITE 1
#cmakedefine HAVE_CU_CURSES 1