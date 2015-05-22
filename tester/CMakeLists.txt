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

if(WIN32)
	set(GTK2_ADDITIONAL_SUFFIXES "../lib/glib-2.0/include" "../lib/gtk-2.0/include")
endif()
find_package(GTK2 2.18 COMPONENTS gtk)

file (GLOB_RECURSE SOURCE_FILES *.c)

add_definitions(-DBC_CONFIG_FILE="config.h")

add_executable(liblinphone_tester ${SOURCE_FILES})
set_target_properties(liblinphone_tester PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(liblinphone_tester PUBLIC ${CUNIT_INCLUDE_DIRS} PRIVATE common)
target_link_libraries(liblinphone_tester linphone ${CUNIT_LIBRARIES})
if (GTK2_FOUND)
	target_compile_definitions(liblinphone_tester PRIVATE HAVE_GTK)
	target_include_directories(liblinphone_tester PUBLIC ${GTK2_INCLUDE_DIRS})
	target_link_libraries(liblinphone_tester linphone ${GTK2_LIBRARIES})
endif()
