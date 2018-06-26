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
import hashlib
import logging
import os
import pystache
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'tools'))
import abstractapi
import genapixml as capi
import metaname
import metadoc


def md5sum(file):
	hasher = hashlib.md5()
	with open(file, mode='rb') as f:
		hasher.update(f.read())
	return hasher.hexdigest()


class RstTools:
	@staticmethod
	def make_part(text):
		return RstTools.make_section(text, char='#', overline=True)

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

		@property
		def rows(self):
			return self._rows

	class CSVTable(Table):
		def addrow(self, row):
			self._rows.append(', '.join([self._format_cell(cell) for cell in row]))

		def __str__(self):
			return '.. csv-table::\n\t\n\t' + '\n\t'.join(self._rows)

		def _format_cell(self, cell):
			return '"{0}"'.format(cell.replace('"', '""'))


	class GridTable(Table):
		def __init__(self):
			RstTools.Table.__init__(self)
			self._widths = []
			self._heights = []
		
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
		if langCode not in LangInfo._displayNames:
			raise ValueError("Invalid language code '{0}'".format(langCode))
		self.langCode = langCode
		self.nameTranslator = metaname.Translator.get(langCode)
		self.langTranslator = abstractapi.Translator.get(langCode)
		self.docTranslator = metadoc.SphinxTranslator(langCode)

	@property
	def displayName(self):
		return LangInfo._displayNames[self.langCode]

	@property
	def directory(self):
		return self.langCode.lower()

	_displayNames = {
		'C'     : 'C',
		'Cpp'   : 'C++',
		'Java'  : 'Java',
		'CSharp': 'C#'
	}


class SphinxPart(object):
	def __init__(self, lang, langs):
		object.__init__(self)
		self.lang = lang
		self.langs = langs

	@property
	def language(self):
		return self.lang.displayName

	@property
	def docTranslator(self):
		return self.lang.docTranslator

	@property
	def hasNamespaceDeclarator(self):
		return ('namespaceDeclarator' in dir(self.docTranslator))

	@property
	def isJava(self):
		return self.lang.langCode == 'Java'

	@property
	def isNotJava(self):
		return not self.isJava

	def make_part(self):
		return lambda text: RstTools.make_part(pystache.render(text, self))

	def make_chapter(self):
		return lambda text: RstTools.make_chapter(pystache.render(text, self))

	def make_section(self):
		return lambda text: RstTools.make_section(pystache.render(text, self))

	def make_subsection(self):
		return lambda text: RstTools.make_subsection(pystache.render(text, self))

	def make_subsection(self):
		return lambda text: RstTools.make_subsubsection(pystache.render(text, self))

	def write_declarator(self):
		return lambda text: self.docTranslator.get_declarator(text)

	def _make_selector(self, obj):
		links = []
		for lang in self.langs:
			if lang is self.lang:
				link = lang.displayName
			else:
				if lang.langCode == 'Java' and type(obj) is abstractapi.Enumerator:
					ref = metadoc.Reference.make_ref_from_object(None, obj.parent)
				else:
					ref = metadoc.Reference.make_ref_from_object(None, obj)
				link = ref.translate(lang.docTranslator, label=lang.displayName)
			links.append(link)
		return ' '.join(links)


class EnumPart(SphinxPart):
	def __init__(self, enum, lang, langs, namespace=None):
		SphinxPart.__init__(self, lang, langs)
		self.name = enum.name.translate(self.lang.nameTranslator)
		self.fullName = enum.name.translate(self.lang.nameTranslator, recursive=True)
		self.briefDesc = enum.briefDescription.translate(self.docTranslator)
		self.enumerators = [self._translate_enumerator(enumerator) for enumerator in enum.enumerators]
		self.selector = self._make_selector(enum)
		self.sectionName = RstTools.make_section(self.name)
		self.declaration = 'public enum {0}'.format(self.name) if self.lang.langCode == 'Java' else self.name

		ref = metadoc.ClassReference(None)
		ref.relatedObject = enum
		self.link = ref.translate(lang.docTranslator)

	def _translate_enumerator(self, enumerator):
			return {
				'name'      : enumerator.name.translate(self.lang.nameTranslator),
				'briefDesc' : enumerator.briefDescription.translate(self.docTranslator),
				'value'     : enumerator.translate_value(self.lang.langTranslator),
				'selector' : self._make_selector(enumerator)
			}


class SphinxPage(SphinxPart):
	def __init__(self, lang, langs, filename):
		SphinxPart.__init__(self, lang, langs)
		self.filename = filename
	
	def write(self, directory):
		r = pystache.Renderer()
		filepath = os.path.join(directory, self.filename)
		tmpFilepath = filepath + '.tmp'
		with open(tmpFilepath, mode='w') as f:
			f.write(r.render(self))
		if os.path.exists(filepath) and md5sum(filepath) == md5sum(tmpFilepath):
			os.remove(tmpFilepath)
		else:
			os.rename(tmpFilepath, filepath)
		return filepath

	def _get_translated_namespace(self, obj):
		namespace = obj.find_first_ancestor_by_type(abstractapi.Namespace)
		return namespace.name.translate(self.lang.nameTranslator, recursive=True)
	
	@staticmethod
	def _classname_to_filename(classname):
		return classname.to_snake_case(fullName=True) + '.rst'


class IndexPage(SphinxPage):
	def __init__(self, lang, filename):
		SphinxPage.__init__(self, lang, None, filename)
		self._entries = []
		self._sorted = True

	@property
	def title(self):
		return RstTools.make_chapter("{0} API".format(self.lang.displayName))

	@property
	def dir(self):
		return self.lang.directory

	@property
	def entries(self):
		if not self._sorted:
			self._entries.sort(key=lambda x: x['filename'])
			self._sorted = True
		return self._entries

	def add_entry(self, filename):
		self.entries.append({'filename': filename})
		self._sorted = False


class EnumPage(SphinxPage):
	def __init__(self, enum, lang, langs):
		filename = SphinxPage._classname_to_filename(enum.name)
		SphinxPage.__init__(self, lang, langs, filename)
		namespace = enum.find_first_ancestor_by_type(abstractapi.Namespace)
		self.namespace = namespace.name.translate(lang.nameTranslator) if lang.langCode != 'C' else None
		self.enum = EnumPart(enum, lang, langs, namespace=namespace)


class ClassPage(SphinxPage):
	def __init__(self, _class, lang, langs):
		filename = SphinxPage._classname_to_filename(_class.name)
		SphinxPage.__init__(self, lang, langs, filename)
		namespace = _class.find_first_ancestor_by_type(abstractapi.Namespace)
		self.namespace = namespace.name.translate(self.lang.nameTranslator, recursive=True)
		self.className = _class.name.translate(self.lang.nameTranslator)
		self.fullClassName = _class.name.translate(self.lang.nameTranslator, recursive=True)
		self.briefDoc = _class.briefDescription.translate(self.docTranslator)
		self.detailedDoc = _class.detailedDescription.translate(self.docTranslator) if _class.detailedDescription is not None else None
		self.enums = [EnumPart(enum, lang, langs) for enum in _class.enums]
		self.properties = self._translate_properties(_class.properties) if isinstance(_class, abstractapi.Class) else []
		self.methods = self._translate_methods(_class.instanceMethods)
		self.classMethods = self._translate_methods(_class.classMethods)
		self.selector = self._make_selector(_class)

	@property
	def classDeclaration(self):
		return 'public interface {0}'.format(self.className) if self.lang.langCode == 'Java' else self.className
	
	@property
	def hasMethods(self):
		return len(self.methods) > 0
	
	@property
	def hasClassMethods(self):
		return len(self.classMethods) > 0
	
	@property
	def hasProperties(self):
		return len(self.properties) > 0

	@property
	def hasEnums(self):
		return len(self.enums) > 0
	
	def _translate_properties(self, properties):
		translatedProperties = []
		for property_ in properties:
			propertyAttr = {
				'name'         : property_.name.translate(self.lang.nameTranslator),
				'getter'       : self._translate_method(property_.getter) if property_.getter is not None else None,
				'setter'       : self._translate_method(property_.setter) if property_.setter is not None else None
			}
			propertyAttr['title'] = RstTools.make_subsubsection(propertyAttr['name'])
			propertyAttr['ref_label'] = (self.lang.langCode + '_')
			propertyAttr['ref_label'] += (property_.getter.name.to_snake_case(fullName=True) if property_.getter is not None else property_.setter.name.to_snake_case(fullName=True))
			translatedProperties.append(propertyAttr)
		return translatedProperties
	
	def _translate_methods(self, methods):
		translatedMethods = []
		for method in methods:
			translatedMethods.append(self._translate_method(method))
		return translatedMethods
	
	def _translate_method(self, method):
		namespace = method.find_first_ancestor_by_type(abstractapi.Class,abstractapi.Interface)
		methAttr = {
			'prototype'    : method.translate_as_prototype(self.lang.langTranslator, namespace=namespace),
			'briefDoc'     : method.briefDescription.translate(self.docTranslator),
			'detailedDoc'  : method.detailedDescription.translate(self.docTranslator),
			'selector'     : self._make_selector(method)
		}
		reference = metadoc.FunctionReference(None)
		reference.relatedObject = method
		methAttr['link'] = reference.translate(self.lang.docTranslator, namespace=method.find_first_ancestor_by_type(abstractapi.Class, abstractapi.Interface))
		return methAttr

	@property
	def enumsSummary(self):
		table = RstTools.CSVTable()
		for enum in self.enums:
			briefDoc = ' '.join([line['line'] for line in enum.briefDesc['lines']])
			table.addrow((enum.link, briefDoc))
		return table
	
	@property
	def propertiesSummary(self):
		table = RstTools.CSVTable()
		for property_ in self.properties:
			reference = ':ref:`{0}`'.format(property_['ref_label'])
			briefDoc = property_['getter']['briefDoc'] if property_['getter'] is not None else property_['setter']['briefDoc']
			briefDoc = ' '.join([line['line'] for line in briefDoc['lines']])
			table.addrow([reference, briefDoc])
		return table
	
	@property
	def instanceMethodsSummary(self):
		table = RstTools.CSVTable()
		for method in self.methods:
			briefDoc = ' '.join([line['line'] for line in method['briefDoc']['lines']])
			table.addrow([method['link'], briefDoc])
		return table
	
	@property
	def classMethodsSummary(self):
		table = RstTools.CSVTable()
		for method in self.classMethods:
			briefDoc = ' '.join([line['line'] for line in method['briefDoc']['lines']])
			table.addrow([method['link'], briefDoc])
		return table


class OldFilesCleaner:
	def __init__(self, rootDirectory):
		self._filesToKeep = set()
		self.root = rootDirectory

	def protect_file(self, directory):
		self._filesToKeep.add(directory)

	def clean(self):
		self._clean(self.root)

	def _clean(self, dir_):
		if os.path.isdir(dir_):
			for filename in os.listdir(dir_):
				self._clean(os.path.join(dir_, filename))
		elif dir_ not in self._filesToKeep:
			os.remove(dir_)


class DocGenerator:
	def __init__(self, api):
		self.api = api
		self.languages = [
			LangInfo('C'),
			LangInfo('Cpp'),
			LangInfo('Java'),
			LangInfo('CSharp')
		]
	
	def generate(self, outputdir):
		for lang in self.languages:
			directory = os.path.join(args.outputdir, lang.directory)
			cleaner = OldFilesCleaner(directory)
			indexPage = IndexPage(lang, 'index.rst')
			if not os.path.exists(directory):
				os.mkdir(directory)
			for enum in self.api.namespace.enums:
				page = EnumPage(enum, lang, self.languages)
				filepath = page.write(directory)
				indexPage.add_entry(page.filename)
				cleaner.protect_file(filepath)
			for class_ in (self.api.namespace.classes + self.api.namespace.interfaces):
				page = ClassPage(class_, lang, self.languages)
				filepath = page.write(directory)
				indexPage.add_entry(page.filename)
				cleaner.protect_file(filepath)
			filepath = indexPage.write(directory)
			cleaner.protect_file(filepath)
			cleaner.clean()


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

