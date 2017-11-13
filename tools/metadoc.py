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

import metaname
import abstractapi
import re


class ParsingError(RuntimeError):
	pass


class UnreleasedNodeError(ValueError):
	pass


class ChildrenList(list):
	def __init__(self, node):
		list.__init__(self)
		self.node = node
	
	def __setitem__(self, key, child):
		if child.parent is not None:
			raise UnreleasedNodeError()
		self[key].parent = None
		list.__setitem__(self, key, child)
		child.parent = self.node
	
	def __delitem__(self, key):
		self[key].parent = None
		list.__delitem__(self, key)
	
	def __iadd__(self, other):
		list.__iadd__(self, other)
		for child in other:
			child.parent = self.node
		return self
	
	def append(self, child):
		list.append(self, child)
		child.parent = self.node
	
	def removeall(self):
		children = []
		while len(self) > 0:
			children.append(self[0])
			del self[0]
		return children


class TreeNode(object):
	def __init__(self):
		self.parent = None
	
	def find_ancestor(self, ancestorType):
		ancestor = self.parent
		while ancestor is not None and type(ancestor) is not ancestorType:
			ancestor = ancestor.parent
		return ancestor
	
	def find_root(self):
		node = self
		while node.parent is not None:
			node = node.parent
		return node


class SingleChildTreeNode(TreeNode):
	def __init__(self):
		TreeNode.__init__(self)
		self._child = None
	
	def _setchild(self, child):
		if child is not None and child.parent is not None:
			raise UnreleasedNodeError()
		if self._child is not None:
			self._child.parent = None
		self._child = child
		if child is not None:
			child.parent = self
	
	def _getchild(self):
		return self._child
	
	def _delchild(self):
		if self._child is not None:
			self._child.parent = None
		del self._child
	
	child = property(fset=_setchild, fget=_getchild, fdel=_delchild)


class MultiChildTreeNode(TreeNode):
	def __init__(self):
		TreeNode.__init__(self)
		self.children = ChildrenList(self)


class ParagraphPart(TreeNode):
	pass


class TextPart(ParagraphPart):
	def __init__(self, text):
		ParagraphPart.__init__(self)
		self.text = text
	
	def translate(self, docTranslator, **kargs):
		return docTranslator.translate_text(self)


class LanguageKeyword(ParagraphPart):
	def __init__(self, keyword):
		ParagraphPart.__init__(self)
		self.keyword = keyword
	
	def translate(self, docTranslator, **kargs):
		return docTranslator.translate_keyword(self)


class Reference(ParagraphPart):
	def __init__(self, cname):
		ParagraphPart.__init__(self)
		self.cname = cname
		self.relatedObject = None
	
	def translate(self, docTranslator, **kargs):
		return docTranslator.translate_reference(self, **kargs)


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


class Paragraph(MultiChildTreeNode):
	@property
	def parts(self):
		return self.children
	
	@parts.setter
	def parts(self, parts):
		self.children = parts
	
	def resolve_all_references(self, api):
		for part in self.parts:
			if isinstance(part, Reference):
				part.resolve(api)
			elif isinstance(part, (Section, ParameterList)):
				part.resolve_all_references(api)
	
	def translate(self, docTranslator, **kargs):
		return docTranslator._translate_paragraph(self, **kargs)


class Section(SingleChildTreeNode):
	def __init__(self, kind):
		SingleChildTreeNode.__init__(self)
		self.kind = kind
	
	@property
	def paragraph(self):
		return self.child
	
	@paragraph.setter
	def paragraph(self, paragraph):
		self.child = paragraph
	
	def resolve_all_references(self, api):
		if self.paragraph is not None:
			self.paragraph.resolve_all_references(api)
	
	def translate(self, docTranslator, **kargs):
		return docTranslator._translate_section(self, **kargs)


class ParameterDescription(SingleChildTreeNode):
	def __init__(self, name, desc):
		SingleChildTreeNode.__init__(self)
		self.name = name
		self.child = desc
	
	@property
	def desc(self):
		return self.child
	
	@desc.setter
	def desc(self, desc):
		self.child = desc
	
	def is_self_parameter(self):
		method = self.find_ancestor(Description).relatedObject
		return method.type == abstractapi.Method.Type.Instance and self.name not in [arg.name for arg in method.args]


class ParameterList(MultiChildTreeNode):
	@property
	def parameters(self):
		return self.children
	
	@parameters.setter
	def parameters(self, parameters):
		self.children = parameters
	
	def resolve_all_references(self, api):
		for parameter in self.parameters:
			if parameter.desc is not None:
				parameter.desc.resolve_all_references(api)
	
	def translate(self, docTranslator, **kargs):
		return docTranslator._translate_parameter_list(self, **kargs)


class Description(MultiChildTreeNode):
	def __init__(self):
		MultiChildTreeNode.__init__(self)
		self.relatedObject = None
	
	@property
	def paragraphs(self):
		return self.children
	
	@paragraphs.setter
	def paragraphs(self, paragraphs):
		self.children = paragraphs
	
	def resolve_all_references(self, api):
		for paragraph in self.paragraphs:
			paragraph.resolve_all_references(api)
	
	def translate(self, translator, **kargs):
		return translator.translate_description(self, **kargs)


class Parser:
	def __init__(self):
		self.constants_regex = re.compile('(?:^|\W)(TRUE|FALSE|NULL)(?:\W|$)')
	
	def parse_description(self, node):
		if node is None:
			return None
		
		desc = Description()
		for paraNode in node.findall('./para'):
			desc.paragraphs += self._parse_paragraph(paraNode)
		return desc
	
	def _parse_paragraph(self, node):
		paragraphs = []
		paragraph = Paragraph()
		
		text = node.text
		if text is not None:
			paragraph.parts += self._parse_text(text)
		
		for partNode in node.findall('*'):
			if partNode.tag == 'ref':
				ref = self._parse_reference(partNode)
				if ref is not None:
					paragraph.parts.append(ref)
			elif partNode.tag == 'simplesect':
				paragraphs.append(paragraph)
				paragraph.parts.append(self._parse_simple_section(partNode))
				paragraph = Paragraph()
			elif partNode.tag == 'xrefsect':
				paragraphs.append(paragraph)
				paragraph.parts.append(self._parse_xref_section(partNode))
				paragraph = Paragraph()
			elif partNode.tag == 'parameterlist' and partNode.get('kind') == 'param':
				paragraphs.append(paragraph)
				paragraphs.append(self._parse_parameter_list(partNode))
				paragraph = Paragraph()
			else:
				text = partNode.text
				if text is not None:
					paragraph.parts += self._parse_text(text)
			
			text = partNode.tail
			if text is not None:
				text = text.strip('\n')
				if len(text) > 0:
					paragraph.parts += self._parse_text(text)
		
		paragraphs.append(paragraph)
		return [x for x in paragraphs if type(x) is not Paragraph or len(x.parts) > 0]
	
	def _parse_text(self, text):
		parts = []
		lastIndex = 0
		
		match = self.constants_regex.search(text)
		while match is not None:
			if match.start(1)-lastIndex > 0:
				parts.append(TextPart(text[lastIndex:match.start(1)]))
				parts.append(self._parse_constant(text[match.start(1):match.end(1)]))
			lastIndex = match.end(1)
			match = self.constants_regex.search(text, lastIndex)
		
		if lastIndex < len(text):
			parts.append(TextPart(text[lastIndex:]))
		
		return parts
	
	def _parse_constant(self, token):
		if token == 'TRUE':
			return LanguageKeyword(abstractapi.Boolean(True))
		elif token == 'FALSE':
			return LanguageKeyword(abstractapi.Boolean(False))
		elif token == 'NULL':
			return LanguageKeyword(abstractapi.Nil())
		else:
			raise ValueError("invalid C constant token '{0}'".format(token))
	
	def _parse_simple_section(self, sectionNode):
		section = Section(sectionNode.get('kind'))
		para = sectionNode.find('./para')
		paragraphs = self._parse_paragraph(para)
		section.paragraph = paragraphs[0] if len(paragraphs) > 0 else None
		return section
	
	def _parse_parameter_list(self, paramListNode):
		paramList = ParameterList()
		for paramItemNode in paramListNode.findall('./parameteritem'):
			name = metaname.ArgName()
			name.from_snake_case(paramItemNode.find('./parameternamelist/parametername').text)
			desc = self.parse_description(paramItemNode.find('parameterdescription'))
			paramList.parameters.append(ParameterDescription(name, desc))
		return paramList
	
	def _parse_xref_section(self, sectionNode):
		sectionId = sectionNode.get('id')
		if sectionId.startswith('deprecated_'):
			section = Section('deprecated')
			description = self.parse_description(sectionNode.find('./xrefdescription'))
			paras = description.paragraphs.removeall()
			section.paragraph = paras[0] if len(paras) > 0 else None
			return section
		else:
			raise ParsingError('unknown xrefsect type ({0})'.format(sectionId))
	
	def _parse_reference(self, node):
		if node.text.endswith('()'):
			return FunctionReference(node.text[0:-2])
		else:
			return ClassReference(node.text)


class Translator:
	def __init__(self, langCode):
		self.textWidth = 80
		self.nameTranslator = metaname.Translator.get(langCode)
		self.langTranslator = abstractapi.Translator.get(langCode)
		self.displaySelfParam = True if langCode == 'C' else False
	
	def translate_description(self, description, tagAsBrief=False):
		if description is None:
			return None
		
		paras = self._translate_description(description)
		
		lines = self._paragraphs_to_lines(paras)
		
		if tagAsBrief:
			self._tag_as_brief(lines)
		
		lines = self._crop_text(lines, self.textWidth)
		
		translatedDoc = {'lines': []}
		for line in lines:
			translatedDoc['lines'].append({'line': line})
		
		return translatedDoc
	
	def translate_reference(self, ref, absName=False, namespace=None):
		if ref.relatedObject is None:
			raise ReferenceTranslationError(ref.cname)
		if absName:
			commonName = None
		else:
			if namespace is None:
				description = ref.find_root()
				namespaceObj = description.relatedObject.find_first_ancestor_by_type(abstractapi.Namespace, abstractapi.Class)
				namespace = namespaceObj.name
			if namespace.is_prefix_of(ref.relatedObject.name):
				commonName = namespace
			else:
				commonName = metaname.Name.find_common_parent(ref.relatedObject.name, namespace)
		return ref.relatedObject.name.translate(self.nameTranslator, recursive=True, topAncestor=commonName)
	
	def translate_keyword(self, keyword):
		return keyword.keyword.translate(self.langTranslator)
	
	def translate_text(self, textpart):
		return textpart.text
	
	def _translate_description(self, desc):
		paras = []
		for para in desc.paragraphs:
			paras.append(para.translate(self))
		return [para for para in paras if para != '']
	
	def _translate_paragraph(self, para):
		strPara = ''
		for part in para.parts:
			try:
				if isinstance(part, str):
					strPara += part
				else:
					strPara += part.translate(self)
			except TranslationError as e:
				print('warning: {0}'.format(e.msg()))
		
		return strPara
	
	def _paragraphs_to_lines(self, paragraphs):
		lines = []
		for para in paragraphs:
			if para is not paragraphs[0]:
				lines.append('')
			lines += para.split('\n')
		return lines
	
	def _crop_text(self, inputLines, width):
		outputLines = []
		for line in inputLines:
			outputLines += self._split_line(line, width)
		return outputLines
	
	def _split_line(self, line, width, indent=False):
		firstNonTab = next((c for c in line if c != '\t'), None)
		tabCount = line.index(firstNonTab) if firstNonTab is not None else 0
		linePrefix = ('\t' * tabCount)
		line = line[tabCount:]
		
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
		
		if indent:
			lines = [line if line is lines[0] else '\t' + line for line in lines]
		
		return [linePrefix + line for line in lines]
	
	def _tag_as_brief(self, lines):
		pass


class TranslationError(Exception):
	pass


class ReferenceTranslationError(TranslationError):
	def __init__(self, refName):
		Exception.__init__(self, refName)
	
	def msg(self):
		return '{0} reference could not be translated'.format(self.args[0])


class DoxygenTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines[0] = '@brief ' + lines[0]
	
	def translate_reference(self, ref):
		refStr = Translator.translate_reference(self, ref)
		if isinstance(ref.relatedObject, (abstractapi.Class, abstractapi.Enum)):
			return '#' + refStr
		elif isinstance(ref.relatedObject, abstractapi.Method):
			return refStr + '()'
		else:
			raise ReferenceTranslationError(ref.cname)
	
	def _translate_section(self, section):
		return '@{0} {1}'.format(
			section.kind,
			self._translate_paragraph(section.paragraph)
		)
	
	def _translate_parameter_list(self, parameterList):
		text = ''
		for paramDesc in parameterList.parameters:
			if self.displaySelfParam or not paramDesc.is_self_parameter():
				desc = self._translate_description(paramDesc.desc)
				desc = desc[0] if len(desc) > 0 else ''
				text = ('@param {0} {1}'.format(paramDesc.name.translate(self.nameTranslator), desc))
		return text


class SphinxTranslator(Translator):
	def __init__(self, langCode):
		Translator.__init__(self, langCode)
		if langCode == 'C':
			self.domain = 'c'
			self.classDeclarator = 'type'
			self.methodDeclarator = 'function'
			self.enumDeclarator = 'type'
			self.enumeratorDeclarator = 'var'
			self.enumeratorReferencer = 'data'
			self.methodReferencer = 'func'
		elif langCode == 'Cpp':
			self.domain = 'cpp'
			self.classDeclarator = 'class'
			self.methodDeclarator = 'function'
			self.enumDeclarator = 'enum'
			self.enumeratorDeclarator = 'enumerator'
			self.namespaceDeclarator = 'namespace'
			self.methodReferencer = 'func'
		elif langCode == 'CSharp':
			self.domain = 'csharp'
			self.classDeclarator = 'class'
			self.methodDeclarator = 'method'
			self.enumDeclarator = 'enum'
			self.enumeratorDeclarator = 'value'
			self.namespaceDeclarator = 'namespace'
			self.classReferencer = 'type'
			self.enumReferencer = 'type'
			self.enumeratorReferencer = 'enum'
			self.methodReferencer = 'meth'
		else:
			raise ValueError('invalid language code: {0}'.format(langCode))
	
	def get_declarator(self, typeName):
		try:
			attrName = typeName + 'Declarator'
			declarator = getattr(self, attrName)
			return '{0}:{1}'.format(self.domain, declarator)
		except AttributeError:
			raise ValueError("'{0}' declarator type not supported".format(typeName))
	
	def get_referencer(self, typeName):
		try:
			attrName = typeName + 'Referencer'
			if attrName in dir(self):
				referencer = getattr(self, attrName)
				return '{0}:{1}'.format(self.domain, referencer)
			else:
				return self.get_declarator(typeName)
		except AttributeError:
			raise ValueError("'{0}' referencer type not supported".format(typeName))
	
	def translate_reference(self, ref, label=None, namespace=None):
		strRef = Translator.translate_reference(self, ref, absName=True)
		kargs = {
			'tag'   : self._sphinx_ref_tag(ref),
			'ref'   : strRef,
		}
		kargs['label'] = label if label is not None else Translator.translate_reference(self, ref, namespace=namespace)
		if isinstance(ref, FunctionReference):
			kargs['label'] += '()'
		
		return ':{tag}:`{label} <{ref}>`'.format(**kargs)
	
	def translate_keyword(self, keyword):
		translatedKeyword = Translator.translate_keyword(self, keyword)
		return '``{0}``'.format(translatedKeyword)
	
	def _translate_section(self, section):
		strPara = self._translate_paragraph(section.paragraph)
		if section.kind == 'deprecated':
			return '**Deprecated:** {0}\n'.format(strPara)
		else:
			if section.kind == 'see':
				kind = 'seealso'
			else:
				kind = section.kind
			
			if section.kind == 'return':
				return ':return: {0}'.format(strPara)
			else:
				return '.. {0}::\n\t\n\t{1}\n\n'.format(kind, strPara)
	
	def _translate_parameter_list(self, parameterList):
		text = ''
		for paramDesc in parameterList.parameters:
			if self.displaySelfParam or not paramDesc.is_self_parameter():
				desc = self._translate_description(paramDesc.desc)
				desc = desc[0] if len(desc) > 0 else ''
				text += (':param {0}: {1}\n'.format(paramDesc.name.translate(self.nameTranslator), desc))
		text += '\n'
		return text
	
	def _sphinx_ref_tag(self, ref):
		typeName = type(ref.relatedObject).__name__.lower()
		return self.get_referencer(typeName)
	
	isParamDescRegex = re.compile('\t*:(?:param\s+\w+|return):')
	
	def _split_line(self, line, width):
		if SphinxTranslator.isParamDescRegex.match(line) is not None:
			lines = Translator._split_line(self, line, width, indent=True)
			return lines
		else:
			return Translator._split_line(self, line, width)


class SandCastleTranslator(Translator):
	def _tag_as_brief(self, lines):
		if len(lines) > 0:
			lines.insert(0, '<summary>')
			lines.append('</summary>')
	
	def translate_reference(self, ref):
		refStr = Translator.translate_reference(self, ref, absName=True)
		if isinstance(ref, FunctionReference):
			refStr += '()'
		return '<see cref="{0}" />'.format(refStr)
