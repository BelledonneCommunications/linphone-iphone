############################################################################
# FindXML2.txt
# Copyright (C) 2015  Belledonne Communications, Grenoble France
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
# - Find the libxml2 include file and library
#
#  XML2_FOUND - system has libxml2
#  XML2_INCLUDE_DIRS - the libxml2 include directory
#  XML2_LIBRARIES - The libraries needed to use libxml2

if(APPLE AND NOT IOS)
	set(XML2_HINTS "/usr")
endif()
if(XML2_HINTS)
	set(XML2_LIBRARIES_HINTS "${XML2_HINTS}/lib")
endif()

find_path(XML2_INCLUDE_DIRS
	NAMES libxml/xmlreader.h
	HINTS "${XML2_HINTS}"
	PATH_SUFFIXES include/libxml2
)

if(XML2_INCLUDE_DIRS)
	set(HAVE_LIBXML_XMLREADER_H 1)
endif()

find_library(XML2_LIBRARIES
	NAMES xml2
	HINTS "${XML2_LIBRARIES_HINTS}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XML2
	DEFAULT_MSG
	XML2_INCLUDE_DIRS XML2_LIBRARIES
)

mark_as_advanced(XML2_INCLUDE_DIRS XML2_LIBRARIES)
