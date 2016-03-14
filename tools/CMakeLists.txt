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

if(MSVC)
	find_library(LIBGCC NAMES gcc)
	find_library(LIBMINGWEX NAMES mingwex)
endif()

set(LP_GEN_WRAPPERS_SOURCE_FILES
	generator.cc
	generator.hh
	genwrappers.cc
	software-desc.cc
	software-desc.hh
)

add_definitions(
	-DIN_LINPHONE
)

set(LP_GEN_WRAPPERS_LIBS
	${LIBGCC}
	${LIBMINGWEX}
	${XML2_LIBRARIES}
)

apply_compile_flags(LP_GEN_WRAPPERS_SOURCE_FILES "CPP" "CXX")
add_executable(lp-gen-wrappers ${LP_GEN_WRAPPERS_SOURCE_FILES})
target_link_libraries(lp-gen-wrappers ${LP_GEN_WRAPPERS_LIBS})


install(TARGETS lp-gen-wrappers
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

set(LP_AUTO_ANSWER_SOURCE_FILES
	auto_answer.c
)

add_definitions(
	-DIN_LINPHONE
)

apply_compile_flags(LP_AUTO_ANSWER_SOURCE_FILES "CPP" "C")
add_executable(lp-auto-answer ${LP_AUTO_ANSWER_SOURCE_FILES})
target_link_libraries(lp-auto-answer linphone)


install(TARGETS lp-auto-answer
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

set(LP_SENDMSG_SOURCE_FILES
        lpsendmsg.c
)

add_definitions(
        -DIN_LINPHONE
)

apply_compile_flags(LP_SENDMSG_SOURCE_FILES "CPP" "C")
add_executable(lp-sendmsg ${LP_SENDMSG_SOURCE_FILES})
target_link_libraries(lp-sendmsg linphone)


install(TARGETS lp-sendmsg
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
