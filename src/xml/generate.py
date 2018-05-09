#!/usr/bin/python

# Copyright (C) 2017 Belledonne Communications SARL
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
	prologue_file = os.path.join("xml", "prologue.txt")
	epilogue_file = os.path.join("xml", "epilogue.txt")
	work_dir = os.path.join(script_dir, "..")
	os.chdir(work_dir)
	p = Popen([xsdcxx,
		"cxx-tree",
		"--generate-wildcard",
		"--generate-serialization",
		"--generate-ostream",
		"--generate-detach",
		"--generate-polymorphic",
		"--std", "c++11",
		"--type-naming", "java",
		"--function-naming", "java",
		"--hxx-suffix", ".h",
		"--ixx-suffix", ".h",
		"--cxx-suffix", ".cpp",
		"--location-regex", "%http://.+/(.+)%$1%",
		"--output-dir", "xml",
		"--show-sloc",
		"--prologue-file", prologue_file,
		"--epilogue-file", epilogue_file,
		"--root-element-first",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-([^,-]+)-?([^,-]*)%\\u$1\\u$2\\u$3\\u$4%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-?([^,-]*)%\\u$1\\u$2\\u$3%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-?([^,-]*)%\\u$1\\u$2%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-([^,-]+)-?([^,-]*),([^,]+)%\\u$1\\u$2\\u$3\\u$4\\l\\u$5%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-?([^,-]*),([^,]+)%\\u$1\\u$2\\u$3\\l\\u$4%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-?([^,-]*),([^,]+)%\\u$1\\u$2\\l\\u$3%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+)%\\u$1\\u$2\\u$3\\u$4\\l\\u$5\\u$6%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+)%\\u$1\\u$2\\u$3\\l\\u$4\\u$5%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-?([^,-]*),([^,]+),([^,]+)%\\u$1\\u$2\\l\\u$3\\u$4%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+),([^,]+)%\\u$1\\u$2\\u$3\\u$4\\l\\u$5\\u$6\\u$7%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+),([^,]+)%\\u$1\\u$2\\u$3\\l\\u$4\\u$5\\u$6%",
		"--type-regex", "%(?:[^ ]* )?([^,-]+)-?([^,-]*),([^,]+),([^,]+),([^,]+)%\\u$1\\u$2\\l\\u$3\\u$4\\u$5%",
		"--accessor-regex", "%([^,-]+)-([^,-]+)-?([^,-]*)%get\\u$1\\u$2\\u$3%",
		"--accessor-regex", "%([^,-]+)-?([^,-]*)%get\\u$1\\u$2%",
		"--accessor-regex", "%([^,-]+)-([^,-]+)-?([^,-]*),([^,]+)%get\\u$1\\u$2\\u$3\\l\\u$4%",
		"--accessor-regex", "%([^,-]+)-?([^,-]*),([^,]+)%get\\u$1\\u$2\\l\\u$3%",
		"--accessor-regex", "%([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+)%get\\u$1\\u$2\\u$3\\l\\u$4\\u$5%",
		"--accessor-regex", "%([^,-]+)-?([^,-]*),([^,]+),([^,]+)%get\\u$1\\u$2\\l\\u$3\\u$4%",
		"--modifier-regex", "%([^,-]+)-([^,-]+)-?([^,-]*)%set\\u$1\\u$2\\u$3%",
		"--modifier-regex", "%([^,-]+)-?([^,-]*)%set\\u$1\\u$2%",
		"--modifier-regex", "%([^,-]+)-([^,-]+)-?([^,-]*),([^,]+)%set\\u$1\\u$2\\u$3\\l\\u$4%",
		"--modifier-regex", "%([^,-]+)-?([^,-]*),([^,]+)%set\\u$1\\u$2\\l\\u$3%",
		"--modifier-regex", "%([^,-]+)-([^,-]+)-?([^,-]*),([^,]+),([^,]+)%set\\u$1\\u$2\\u$3\\l\\u$4\\u$5%",
		"--modifier-regex", "%([^,-]+)-?([^,-]*),([^,]+),([^,]+)%set\\u$1\\u$2\\l\\u$3\\u$4%",
		"--parser-regex", "%([^-]+)-?([^-]*)%parse\\u$1\\u$2%",
		"--serializer-regex", "%([^-]+)-?([^-]*)%serialize\\u$1\\u$2%",
		"--namespace-map", "http://www.w3.org/2001/XMLSchema=LinphonePrivate::Xsd::XmlSchema",
		"--namespace-map", "urn:ietf:params:xml:ns:conference-info=LinphonePrivate::Xsd::ConferenceInfo",
		"--namespace-map", "urn:ietf:params:xml:ns:imdn=LinphonePrivate::Xsd::Imdn",
		"--namespace-map", "urn:ietf:params:xml:ns:im-iscomposing=LinphonePrivate::Xsd::IsComposing",
		"--namespace-map", "http://www.linphone.org/xsds/imdn.xsd=LinphonePrivate::Xsd::LinphoneImdn",
		"--namespace-map", "urn:ietf:params:xml:ns:resource-lists=LinphonePrivate::Xsd::ResourceLists",
		"--namespace-map", "urn:ietf:params:xml:ns:rlmi=LinphonePrivate::Xsd::Rlmi",
		source_file
		], shell=False)
	p.communicate()
	os.chdir(cwd)
	return 0

def main(argv = None):
	generate("xml")
	generate("conference-info")
	generate("imdn")
	generate("is-composing")
	generate("linphone-imdn")
	generate("resource-lists")
	generate("rlmi")

if __name__ == "__main__":
	sys.exit(main())
