############################################################################
# CMakeLists.txt
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

if(GETTEXT_FOUND)
	foreach(language ${LINPHONE_ALL_LANGS_LIST})
		GETTEXT_PROCESS_PO_FILES(${language} ALL PO_FILES ${language}.po)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${language}.gmo
			DESTINATION ${PACKAGE_LOCALE_DIR}/${language}/LC_MESSAGES
			RENAME linphone.mo
			PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
	endforeach()
endif()
