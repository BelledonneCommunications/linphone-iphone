#!/usr/bin/python
# -*- coding: utf-8 -*-

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


import argparse
import logging
import os
import pystache
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'tools'))
import abstractapi
import genapixml as capi
import metaname
import metadoc


class RstTools:
	@staticmethod
	def make_chapter(text):
		return RstTools.make_section(text, char='*', overline=True)
	
	@staticmethod
	def make_section(text, char='=', overline=False):
		size = len(text)
		underline = (char*size)
		lines = [text, underline]
		if overline:
			lines.insert(0, underline)
		return '\n'.join(lines)
	
	@staticmethod
	def make_subsection(text):
		return RstTools.make_section(text, char='-')
	
	@staticmethod
	def make_subsubsection(text):
		return RstTools.make_section(text, char='^')
	
	class Table:
		def __init__(self):
			self._rows = []
			self._widths = []
			self._heights = []
		
		@property
		def rows(self):
			return self._rows
		
		def addrow(self, row):
			if len(self._widths) == 0:
				self._widths.append(0)
				self._widths *= len(row)
			elif len(row) != len(self._widths):
				raise ValueError('row width mismatch table width')
			
			height = 0
			row2 = []
			
			i = 0
			while i<len(row):
				lines = str(row[i]).split(sep='\n')
				row2.append(lines)
				width = len(max(lines, key=len))
				self._widths[i] = max(self._widths[i], width)
				height = max(height, len(lines))
				i += 1
			
			self._rows.append(row2)
			self._heights.append(height)
		
		def _make_hline(self):
			res = '+'
			for width in self._widths:
				res += ('-' * (width+2))
				res += '+'
			res += '\n'
			return res
		
		def _make_row(self, idx):
			res = ''
			row = self._rows[idx]
			j = 0
			while j < self._heights[idx]:
				res += '|'
				i = 0
				while i < len(row):
					line = row[i][j] if j < len(row[i]) else ''
					res += ' {0} '.format(line)
					res += (' ' * (self._widths[i]-len(line)))
					res += '|'
					i += 1
				res += '\n'
				j += 1
			return res
		
		def __str__(self):
			if len(self._rows) == 0 or len(self._widths) == 0:
				return ''
			else:
				res = self._make_hline()
				i = 0
				while i<len(self._rows):
					res += self._make_row(i)
					res += self._make_hline()
					i += 1
				return res


class LangInfo:
	def __init__(self, langCode):
		self.langCode = langCode
		self.displayName = LangInfo._lang_code_to_display_name(langCode)
		self.nameTranslator = metaname.Translator.get(langCode)
		self.langTranslator = abstractapi.Translator.get(langCode)
		self.docTranslator = metadoc.SphinxTranslator(langCode)
	
	@staticmethod
	def _lang_code_to_display_name(langCode):
		if langCode == 'C':
			return 'C'
		elif langCode == 'Cpp':
			return 'C++'
		elif langCode == 'CSharp':
			return 'C#'
		else:
			raise ValueError("Invalid language code: '{0}'".format(langCode))


class SphinxPage(object):
	def __init__(self, lang, langs, filename):
		object.__init__(self)
		self.lang = lang
		self.langs = langs
		self.filename = filename
	
	@property
	def hasNamespaceDeclarator(self):
		return ('namespaceDeclarator' in dir(self.docTranslator))
	
	@property
	def language(self):
		return self.lang.displayName
	
	@property
	def docTranslator(self):
		return self.lang.docTranslator
	
	def make_chapter(self):
		return lambda text: RstTools.make_chapter(pystache.render(text, self))
	
	def make_section(self):
		return lambda text: RstTools.make_section(pystache.render(text, self))
	
	def make_subsection(self):
		return lambda text: RstTools.make_subsection(pystache.render(text, self.properties))
	
	def write_declarator(self):
		return lambda text: self.docTranslator.get_declarator(text)
	
	def write(self, directory):
		r = pystache.Renderer()
		filepath = os.path.join(directory, self.filename)
		with open(filepath, mode='w') as f:
			f.write(r.render(self))
	
	def _get_translated_namespace(self, obj):
		namespace = obj.find_first_ancestor_by_type(abstractapi.Namespace)
		return namespace.name.translate(self.lang.nameTranslator, recursive=True)
	
	def _make_selector(self, obj):
		links = []
		ref = metadoc.Reference(None)
		ref.relatedObject = obj
		for lang in self.langs:
			if lang is self.lang:
				link = lang.displayName
			else:
				link = ref.translate(lang.docTranslator, label=lang.displayName)
			
			links.append(link)
		
		return ' '.join(links)
	
	@staticmethod
	def _classname_to_filename(classname):
		return classname.to_snake_case(fullName=True) + '.rst'


class IndexPage(SphinxPage):
	def __init__(self, lang, langs):
		SphinxPage.__init__(self, lang, langs, 'index.rst')
		self.tocEntries = []
	
	def add_class_entry(self, _class):
		self.tocEntries.append({'entryName': SphinxPage._classname_to_filename(_class.name)})


class EnumsPage(SphinxPage):
	def __init__(self, lang, langs, enums):
		SphinxPage.__init__(self, lang, langs, 'enums.rst')
		self._translate_enums(enums)
	
	def _translate_enums(self, enums):
		self.enums = []
		for enum in enums:
			translatedEnum = {
				'name'         : enum.name.translate(self.lang.nameTranslator),
				'fullName'     : enum.name.translate(self.lang.nameTranslator, recursive=True),
				'briefDesc'    : enum.briefDescription.translate(self.docTranslator),
				'enumerators'  : self._translate_enum_values(enum),
				'selector'    : self._make_selector(enum)
			}
			translatedEnum['namespace'] = self._get_translated_namespace(enum) if self.lang.langCode == 'Cpp' else translatedEnum['fullName']
			translatedEnum['sectionName'] = RstTools.make_section(translatedEnum['name'])
			self.enums.append(translatedEnum)
	
	def _translate_enum_values(self, enum):
		translatedEnumerators = []
		for enumerator in enum.enumerators:
			translatedValue = {
				'name'      : enumerator.name.translate(self.lang.nameTranslator),
				'briefDesc' : enumerator.briefDescription.translate(self.docTranslator),
				'value'     : enumerator.translate_value(self.lang.langTranslator),
				'selector' : self._make_selector(enumerator)
			}
			translatedEnumerators.append(translatedValue)
		
		return translatedEnumerators


class ClassPage(SphinxPage):
	def __init__(self, _class, lang, langs):
		filename = SphinxPage._classname_to_filename(_class.name)
		SphinxPage.__init__(self, lang, langs, filename)
		self.namespace = self._get_translated_namespace(_class)
		self.className = _class.name.translate(self.lang.nameTranslator)
		self.fullClassName = _class.name.translate(self.lang.nameTranslator, recursive=True)
		self.briefDoc = _class.briefDescription.translate(self.docTranslator)
		self.detailedDoc = _class.detailedDescription.translate(self.docTranslator) if _class.detailedDescription is not None else None
		self.properties = self._translate_properties(_class.properties)
		self.methods = self._translate_methods(_class.instanceMethods)
		self.classMethods = self._translate_methods(_class.classMethods)
		self.selector = self._make_selector(_class)
	
	@property
	def hasMethods(self):
		return len(self.methods) > 0
	
	@property
	def hasClassMethods(self):
		return len(self.classMethods) > 0
	
	@property
	def hasProperties(self):
		return len(self.properties) > 0
	
	def _translate_properties(self, properties):
		translatedProperties = []
		for property_ in properties:
			propertyAttr = {
				'name'         : property_.name.translate(self.lang.nameTranslator),
				'ref_label'    : '{0}_{1}'.format(self.lang.langCode, property_.name.to_snake_case(fullName=True)),
				'getter'       : self._translate_method(property_.getter) if property_.getter is not None else None,
				'setter'       : self._translate_method(property_.setter) if property_.setter is not None else None
			}
			propertyAttr['title'] = RstTools.make_subsubsection(propertyAttr['name'])
			translatedProperties.append(propertyAttr)
		return translatedProperties
	
	def _translate_methods(self, methods):
		translatedMethods = []
		for method in methods:
			translatedMethods.append(self._translate_method(method))
		return translatedMethods
	
	def _translate_method(self, method):
		prototypeParams = {}
		if self.lang.langCode == 'Cpp':
			prototypeParams['showStdNs'] = True
		methAttr = {
			'prototype'    : method.translate_as_prototype(self.lang.langTranslator, **prototypeParams),
			'briefDoc'     : method.briefDescription.translate(self.docTranslator),
			'detailedDoc'  : method.detailedDescription.translate(self.docTranslator),
			'selector'     : self._make_selector(method)
		}
		reference = metadoc.FunctionReference(None)
		reference.relatedObject = method
		methAttr['link'] = reference.translate(self.lang.docTranslator)
		return methAttr
	
	@property
	def propertiesSummary(self):
		table = RstTools.Table()
		for property_ in self.properties:
			reference = ':ref:`{0}`'.format(property_['ref_label'])
			briefDoc = property_['getter']['briefDoc'] if property_['getter'] is not None else property_['setter']['briefDoc']
			briefDoc = '\n'.join([line['line'] for line in briefDoc['lines']])
			table.addrow([reference, briefDoc])
		return table
	
	@property
	def instanceMethodsSummary(self):
		table = RstTools.Table()
		for method in self.methods:
			briefDoc = '\n'.join([line['line'] for line in method['briefDoc']['lines']])
			table.addrow([method['link'], briefDoc])
		return table
	
	@property
	def classMethodsSummary(self):
		table = RstTools.Table()
		for method in self.classMethods:
			briefDoc = '\n'.join([line['line'] for line in method['briefDoc']['lines']])
			table.addrow([method['link'], briefDoc])
		return table


class DocGenerator:
	def __init__(self, api):
		self.api = api
		self.languages = [
			LangInfo('C'),
			LangInfo('Cpp'),
			LangInfo('CSharp')
		]
	
	def generate(self, outputdir):
		for lang in self.languages:
			subdirectory = lang.langCode.lower()
			directory = os.path.join(args.outputdir, subdirectory)
			if not os.path.exists(directory):
				os.mkdir(directory)
			
			enumsPage = EnumsPage(lang, self.languages, absApiParser.enums)
			enumsPage.write(directory)
			
			indexPage = IndexPage(lang, self.languages)
			for _class in absApiParser.classes:
				page = ClassPage(_class, lang, self.languages)
				page.write(directory)
				indexPage.add_class_entry(_class)
			
			indexPage.write(directory)


if __name__ == '__main__':
	argparser = argparse.ArgumentParser(description='Generate a sphinx project to generate the documentation of Linphone Core API.')
	argparser.add_argument('xmldir', type=str, help='directory holding the XML documentation of the C API generated by Doxygen')
	argparser.add_argument('-o --output', type=str, help='directory into where Sphinx source files will be written', dest='outputdir', default='.')
	argparser.add_argument('-v --verbose', action='store_true', default=False, dest='verbose_mode', help='Show warning and info messages')
	args = argparser.parse_args()
	
	loglevel = logging.INFO if args.verbose_mode else logging.ERROR
	logging.basicConfig(format='%(levelname)s[%(name)s]: %(message)s', level=loglevel)

	cProject = capi.Project()
	cProject.initFromDir(args.xmldir)
	cProject.check()

	absApiParser = abstractapi.CParser(cProject)
	absApiParser.parse_all()
	
	docGenerator = DocGenerator(absApiParser)
	docGenerator.generate(args.outputdir)

