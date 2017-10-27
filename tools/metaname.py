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


import re


class Name(object):
	camelCaseParsingRegex = re.compile('[A-Z][a-z0-9]*')
	lowerCamelCaseSplitingRegex = re.compile('([a-z][a-z0-9]*)([A-Z][a-z0-9]*)')
	
	def __init__(self):
		self.words = []
		self.prev = None
	
	def __eq__(self, other):
		return (other is not None and self.words == other.words) and (self.prev == other.prev)
	
	def __lt__(self, other):
		return self.to_camel_case() < other.to_camel_case()
	
	def copy(self):
		nameType = type(self)
		name = nameType()
		name.words = list(self.words)
		name.prev = None if self.prev is None else self.prev.copy()
		return name
	
	def delete_prefix(self, prefix):
		it = self
		_next = None
		while it is not None and it.words != prefix.words:
			_next = it
			it = it.prev
		
		if it is None or it != prefix:
			raise Error('no common prefix')
		elif _next is not None:
			_next.prev = None
	
	def _set_namespace(self, namespace):
		self.prev = namespace
		if self.prev is not None:
			prefix = namespace.to_word_list()
			i = 0
			while i<len(self.words) and i<len(prefix) and self.words[i] == prefix[i]:
				i += 1
			if i == len(self.words):
				raise Error('name equal to namespace \'{0}\'', self.words)
			else:
				self.words = self.words[i:]
	
	def _lower_all_words(self):
		i = 0
		while i<len(self.words):
			self.words[i] = self.words[i].lower()
			i += 1
	
	def from_snake_case(self, name, namespace=None):
		self.words = name.split('_')
		Name._set_namespace(self, namespace)
	
	def from_camel_case(self, name, islowercased=False, namespace=None):
		if not islowercased:
			self.words = Name.camelCaseParsingRegex.findall(name)
		else:
			match = Name.lowerCamelCaseSplitingRegex.match(name)
			self.words = [match.group(1)]
			self.words += Name.camelCaseParsingRegex.findall(match.group(2))
		
		Name._lower_all_words(self)
		Name._set_namespace(self, namespace)
	
	def to_snake_case(self, fullName=False, upper=False):
		if self.prev is None or not fullName:
			res = '_'.join(self.words)
			if upper:
				res = res.upper()
			return res
		else:
			return Name.to_snake_case(self.prev, fullName=True, upper=upper) + '_' + Name.to_snake_case(self, upper=upper)
	
	def to_camel_case(self, lower=False, fullName=False):
		if self.prev is None or not fullName:
			res = ''
			for elem in self.words:
				if elem is self.words[0] and lower:
					res += elem
				else:
					res += elem.title()
			return res
		else:
			return Name.to_camel_case(self.prev, fullName=True, lower=lower) + Name.to_camel_case(self)
	
	def concatenate(self, upper=False, fullName=False):
		if self.prev is None or not fullName:
			res = ''
			for elem in self.words:
				if upper:
					res += elem.upper()
				else:
					res += elem
			return res
		else:
			return Name.concatenate(self.prev, upper=upper, fullName=True) + Name.concatenate(self, upper=upper)
	
	def to_word_list(self):
		if self.prev is None:
			return list(self.words)
		else:
			return Name.to_word_list(self.prev) + self.words
	
	def is_prefix_of(self, other):
		node = other
		while node is not None and node != self:
			node = node.prev
		return (node is not None)
	
	@staticmethod
	def find_common_parent(name1, name2):
		if name1.prev is None or name2.prev is None:
			return None
		elif name1.prev is name2.prev:
			return name1.prev
		else:
			commonParent = Name.find_common_parent(name1.prev, name2)
			if commonParent is not None:
				return commonParent
			else:
				return Name.find_common_parent(name1, name2.prev)


class ClassName(Name):
	def to_c(self):
		return self.to_camel_case(fullName=True)
	
	def translate(self, translator, **params):
		return translator.translate_class_name(self, **params)


class InterfaceName(ClassName):
	def to_c(self):
		return ClassName.to_c(self)[:-8] + 'Cbs'
	
	def translate(self, translator, **params):
		return translator.translate_interface_name(self, **params)


class EnumName(ClassName):
	def translate(self, translator, **params):
		return translator.translate_enum_name(self, **params)


class EnumeratorName(ClassName):
	def translate(self, translator, **params):
		return translator.translate_enumerator_name(self, **params)


class MethodName(Name):
	regex = re.compile('^\d+$')
	
	def __init__(self):
		self.overloadRef = 0
	
	def from_snake_case(self, name, namespace=None):
		Name.from_snake_case(self, name, namespace=namespace)
		if len(self.words) > 0:
			suffix = self.words[-1]
			if MethodName.regex.match(suffix) is not None:
				self.overloadRef = int(suffix)
				del self.words[-1]
	
	def to_c(self):
		suffix = ('_' + str(self.overloadRef)) if self.overloadRef > 0 else ''
		return self.to_snake_case(fullName=True) + suffix
	
	def translate(self, translator, **params):
		return translator.translate_method_name(self, **params)


class ArgName(Name):
	def to_c(self):
		return self.to_snake_case()
	
	def translate(self, translator, **params):
		return translator.translate_argument_name(self, **params)


class PropertyName(ArgName):
	def translate(self, translator, **params):
		return translator.translate_property_name(self, **params)


class NamespaceName(Name):
	def __init__(self, *params):
		Name.__init__(self)
		if len(params) > 0:
			self.words = params[0]
	
	def translate(self, translator, **params):
		return translator.translate_namespace_name(self, **params)


class Translator:
	instances = {}
	
	@staticmethod
	def get(langCode):
		try:
			if langCode == '':
				raise ValueError('Empty language code')
			if langCode not in Translator.instances:
				className = langCode + 'Translator'
				_class = globals()[className]
				Translator.instances[langCode] = _class()
			
			return Translator.instances[langCode]
		except KeyError:
			raise ValueError("Invalid language code: '{0}'".format(langCode))


class CTranslator(Translator):
	def translate_class_name(self, name, **params):
		return name.to_c()
	
	def translate_interface_name(self, name, **params):
		return name.to_c()
	
	def translate_enum_name(self, name, **params):
		return name.to_c()
	
	def translate_enumerator_name(self, name, **params):
		return name.to_c()
	
	def translate_method_name(self, name, **params):
		return name.to_c()
	
	def translate_namespace_name(self, name, **params):
		return None
	
	def translate_argument_name(self, name, **params):
		return name.to_c()
	
	def translate_property_name(self, name, **params):
		return name.to_c()


class JavaTranslator(Translator):
	def __init__(self):
		self.nsSep = '.'
		self.keyWordEscapes = {}
		self.lowerMethodNames = True
		self.lowerNamespaceNames = True
	
	def translate_class_name(self, name, recursive=False, topAncestor=None):
		if name.prev is None or not recursive or name.prev is topAncestor:
			return name.to_camel_case()
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return name.prev.translate(self, **params) + self.nsSep + name.to_camel_case()
	
	def translate_interface_name(self, name, **params):
		return self.translate_class_name(name, **params)
	
	def translate_enum_name(self, name, **params):
		return self.translate_class_name(name, **params)
	
	def translate_enumerator_name(self, name, **params):
		return self.translate_class_name(name, **params)
	
	def translate_method_name(self, name, recursive=False, topAncestor=None):
		translatedName = name.to_camel_case(lower=self.lowerMethodNames)
		translatedName = self._escape_keyword(translatedName)
		
		if name.prev is None or not recursive or name.prev is topAncestor:
			return translatedName
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return name.prev.translate(self, **params) + self.nsSep + translatedName
	
	def translate_namespace_name(self, name, recursive=False, topAncestor=None):
		translatedName = name.concatenate() if self.lowerNamespaceNames else name.to_camel_case()
		if name.prev is None or not recursive or name.prev is topAncestor:
			return translatedName
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return name.prev.translate(self, **params) + self.nsSep + translatedName
	
	def translate_argument_name(self, name):
		argname = name.to_camel_case(lower=True)
		return self._escape_keyword(argname)
	
	def translate_property_name(self, name):
		return self.translate_argument_name(name)
	
	def _escape_keyword(self, keyword):
		try:
			return self.keyWordEscapes[keyword]
		except KeyError:
			return keyword


class CppTranslator(JavaTranslator):
	def __init__(self):
		JavaTranslator.__init__(self)
		self.nsSep = '::'
		self.keyWordEscapes = {'new' : '_new'}
	
	def translate_enumerator_name(self, name, **params):
		return self.translate_enum_name(name.prev, **params) + name.to_camel_case()


class CSharpTranslator(JavaTranslator):
	def __init__(self):
		JavaTranslator.__init__(self)
		self.keyWordEscapes = {
			'params' : 'parameters',
			'event'  : 'ev',
			'ref'    : 'reference',
			'value'  : 'val',
			'new'    : '_new'
		}
		self.lowerMethodNames = False
		self.lowerNamespaceNames = False
	
	def translate_property_name(self, name):
		return name.to_camel_case()
