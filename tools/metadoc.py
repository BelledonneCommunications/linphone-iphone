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

import abstractapi


class Nil:
	pass


class Reference:
	def __init__(self, cname):
		self.cname = cname
		self.relatedObject = None


class ClassReference(Reference):
	def resolve(self, api):
		try:
			self.relatedObject = api.classesIndex[self.cname]
		except KeyError:
			print('doc reference pointing on an unknown object ({0})'.format(self.cname))


class FunctionReference(Reference):
	def resolve(self, api):
		try:
			self.relatedObject = api.methodsIndex[self.cname]
		except KeyError:
			print('doc reference pointing on an unknown object ({0})'.format(self.cname))


class Paragraph:
	def __init__(self):
		self.parts = []
	
	def resolve_all_references(self, api):
		for part in self.parts:
			if isinstance(part, Reference):
				part.resolve(api)


class Description:
	def __init__(self):
		self.paragraphs = []
	
	def resolve_all_references(self, api):
		for paragraph in self.paragraphs:
			paragraph.resolve_all_references(api)


class Parser:
	def parse_description(self, node):
		desc = Description()
		for paraNode in node.findall('./para'):
			desc.paragraphs.append(self._parse_paragraph(paraNode))
		return desc
	
	def _parse_paragraph(self, node):
		paragraph = Paragraph()
		for partNode in node.iter():
			if partNode is node:
				text = partNode.text
				if text is not None:
					paragraph.parts.append(text)
			else:
				if partNode.tag == 'ref':
					ref = self._parse_reference(partNode)
					if ref is not None:
						paragraph.parts.append(ref)
				else:
					text = partNode.text
					if text is not None:
						paragraph.parts.append(text)
				
				tail = partNode.tail
				if tail is not None:
					paragraph.parts.append(tail)
		
		return paragraph
	
	def _parse_reference(self, node):
		if node.text.endswith('()'):
			return FunctionReference(node.text[0:-2])
		else:
			return ClassReference(node.text)


class Translator:
	def __init__(self):
		self.textWidth = 80
	
	def translate(self, description):
		if description is None:
			return None
		
		lines = []
		for para in description.paragraphs:
			if para is not description.paragraphs[0]:
				lines.append('')
			lines.append(self._translate_paragraph(para))
		
		self._tag_as_brief(lines)
		lines = self._crop_text(lines, self.textWidth)
		
		translatedDoc = {'lines': []}
		for line in lines:
			translatedDoc['lines'].append({'line': line})
			
		return translatedDoc
	
	def _translate_paragraph(self, para):
		strPara = ''
		for part in para.parts:
			if isinstance(part, str):
				strPara += part
			elif isinstance(part, Reference):
				try:
					strPara += self._translate_reference(part)
				except ReferenceTranslationError as e:
					print('could not translate one reference in docstrings ({0})'.format(e.args[0]))
					strPara += Translator._translate_reference(self, part)
			else:
				raise TypeError('untranslatable paragraph element ({0})'.format(part))
		
		return strPara
	
	def _translate_reference(self, ref):
		if isinstance(ref, FunctionReference):
			return ref.cname + '()'
		else:
			return ref.cname
	
	def _crop_text(self, inputLines, width):
		outputLines = []
		for line in inputLines:
			outputLines += self._split_line(line, width)
		return outputLines
	
	def _split_line(self, line, width):
		lines = []
		while len(line) > width:
			cutIndex = line.rfind(' ', 0, width)
			if cutIndex != -1:
				lines.append(line[0:cutIndex])
				line = line[cutIndex+1:]
			else:
				cutIndex = width
				lines.append(line[0:cutIndex])
				line = line[cutIndex:]
		
		lines.append(line)
		return lines


class ReferenceTranslationError(RuntimeError):
	pass


class DoxygenCppTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines[0] = '@brief ' + lines[0]
	
	def _translate_reference(self, ref):
		if isinstance(ref.relatedObject, (abstractapi.Class, abstractapi.Enum)):
			return '#' + ref.relatedObject.name.to_c()
		elif isinstance(ref.relatedObject, abstractapi.Method):
			return ref.relatedObject.name.to_c() + '()'
		else:
			raise ReferenceTranslationError(ref.cname)


class SandcastleCSharpTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines.insert(0, '<summary>')
			lines.append('</summary>')
