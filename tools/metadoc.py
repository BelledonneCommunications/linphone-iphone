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


class Nil:
	pass


class Reference:
	def __init__(self, name):
		self.cObjectName = name


class Paragraph:
	def __init__(self, node=None):
		self.parts = []
		if node is not None:
			self.parse_doxygen_node(node)
	
	def parse_doxygen_node(self, node):
		for partNode in node.iter():
			text = partNode.text
			if text is not None:
				self.parts.append(text)
			if partNode is not node:
				tail = partNode.tail
				if tail is not None:
					self.parts.append(tail)


class Description:
	def __init__(self, node=None):
		self.paragraphs = []
		if node is not None:
			self.parse_doxygen_node(node)
	
	def parse_doxygen_node(self, node):
		for paraNode in node.findall('./para'):
			self.paragraphs.append(Paragraph(paraNode))


class Translator:
	def translate(self, description):
		if description is None:
			return None
		
		lines = []
		for para in description.paragraphs:
			if para is not description.paragraphs[0]:
				lines.append('')
			lines.append(''.join(para.parts))
		
		self._tag_as_brief(lines)
		
		translatedDoc = {'lines': []}
		for line in lines:
			translatedDoc['lines'].append({'line': line})
			
		return translatedDoc


class DoxygenCppTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines[0] = '@brief ' + lines[0]


class SandcastleCSharpTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines.insert(0, '<summary>')
			lines.append('</summary>')
