#!/usr/bin/python

# Copyright (C) 2014 Belledonne Communications SARL
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


from distutils.spawn import find_executable
import os
import sys
from subprocess import Popen, PIPE


def find_xsdcxx():
	xsdcxx = find_executable("xsdcxx")
	if xsdcxx is not None:
		return xsdcxx
	xsdcxx = find_executable("xsd")
	return xsdcxx

def generate(name):
	xsdcxx = find_xsdcxx()
	if xsdcxx is None:
		print("Cannot find xsdcxx (or xsd) program in the PATH")
		return -1
	print("Using " + xsdcxx)
	cwd = os.getcwd()
	script_dir = os.path.dirname(os.path.realpath(__file__))
	source_file = name + ".xsd"
	print("Generating code from " + source_file)
	source_file = os.path.join("xml", source_file)
	work_dir = os.path.join(script_dir, "..")
	os.chdir(work_dir)
	p = Popen([xsdcxx,
		"cxx-tree",
		"--generate-wildcard",
		"--generate-serialization",
		"--generate-ostream",
		"--generate-detach",
		"--std", "c++11",
		"--type-naming", "java",
		"--function-naming", "java",
		"--location-regex-trace",
		"--show-sloc",
		"--hxx-suffix", ".h",
		"--ixx-suffix", ".h",
		"--cxx-suffix", ".cpp",
		"--location-regex", "%http://.+/(.+)%$1%",
		"--output-dir", "xml",
		source_file
		], shell=False)
	p.communicate()
	os.chdir(cwd)
	return 0

def main(argv = None):
	generate("xml")
	generate("conference-info")
	generate("resource-lists")

if __name__ == "__main__":
	sys.exit(main())
