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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################

set(ICONS_INSTALL_DIR ${PACKAGE_DATA_DIR}/icons/hicolor)

file(GLOB PIXMAPS "*.png" "linphone.icns")

install(FILES ${PIXMAPS}
	DESTINATION ${PACKAGE_DATA_DIR}/pixmaps/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES svg/linphone-micro-muted.svg
	svg/linphone-speaker-muted.svg
	svg/linphone-micro-enabled.svg
	svg/linphone-speaker-enabled.svg
	DESTINATION ${ICONS_INSTALL_DIR}/scalable/status
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES linphone-micro-muted.png
	linphone-speaker-muted.png
	linphone-micro-enabled.png
	linphone-speaker-enabled.png
	DESTINATION ${ICONS_INSTALL_DIR}/48x48/status
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES linphone.png
	DESTINATION ${ICONS_INSTALL_DIR}/48x48/apps
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(WIN32)
	install(FILES index.theme
		DESTINATION ${ICONS_INSTALL_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
endif()
