############################################################################
# FindCpuFeatures.cmake
# Copyright (C) 2017  Belledonne Communications, Grenoble France
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
#
# - Find the Android cpufeatures include file and library
#
#  CPUFEATURES_FOUND - system has libcpufeatures
#  CPUFEATURES_LIBRARIES - The libraries needed to use libcpufeatures

find_library(CPUFEATURES_LIBRARIES
	NAMES cpufeatures
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CpuFeatures
	DEFAULT_MSG
	CPUFEATURES_LIBRARIES
)

mark_as_advanced(CPUFEATURES_LIBRARIES)
