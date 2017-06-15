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
import genapixml as CApi


class Error(RuntimeError):
	pass


class BlacklistedException(Error):
	pass


class Name(object):
	camelCaseParsingRegex = re.compile('[A-Z][a-z0-9]*')
	lowerCamelCaseSplitingRegex = re.compile('([a-z][a-z0-9]*)([A-Z][a-z0-9]*)')
	
	def __init__(self):
		self.words = []
		self.prev = None
	
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
		return Name.to_camel_case(self, fullName=True)


class InterfaceName(ClassName):
	def to_c(self):
		return ClassName.to_c(self)[:-8] + 'Cbs'


class EnumName(ClassName):
	pass


class EnumValueName(ClassName):
	pass


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


class ArgName(Name):
	def to_c(self):
		return self.to_snake_case()


class PropertyName(ArgName):
	pass


class NamespaceName(Name):
	def __init__(self, *params):
		Name.__init__(self)
		if len(params) > 0:
			self.words = params[0]


class Object(object):
	def __init__(self, name):
		self.name = name
		self.parent = None
		self.deprecated = False
	
	def find_first_ancestor_by_type(self, _type):
		ancestor = self.parent
		while ancestor is not None and type(ancestor) is not _type:
			ancestor = ancestor.parent
		return ancestor


class Type(Object):
	def __init__(self, name, isconst=False, isref=False):
		Object.__init__(self, name)
		self.isconst = isconst
		self.isref = isref
		self.cname = None


class BaseType(Type):
	def __init__(self, name, isconst=False, isref=False, size=None, isUnsigned=False):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.size = size
		self.isUnsigned = isUnsigned


class EnumType(Type):
	def __init__(self, name, isconst=False, isref=False, enumDesc=None):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.desc = enumDesc


class ClassType(Type):
	def __init__(self, name, isconst=False, isref=False, classDesc=None):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.desc = classDesc


class ListType(Type):
	def __init__(self, containedTypeName, isconst=False, isref=False):
		Type.__init__(self, 'list', isconst=isconst, isref=isref)
		self.containedTypeName = containedTypeName
		self._containedTypeDesc = None
	
	def set_contained_type_desc(self, desc):
		self._containedTypeDesc = desc
		desc.parent = self
	
	def get_contained_type_desc(self):
		return self._containedTypeDesc
	
	containedTypeDesc = property(fset=set_contained_type_desc, fget=get_contained_type_desc)


class DocumentableObject(Object):
	def __init__(self, name):
		Object.__init__(self, name)
		self.briefDescription = None
		self.detailedDescription = None
		self.deprecated = None
	
	def set_from_c(self, cObject, namespace=None):
		self.briefDescription = cObject.briefDescription
		self.detailedDescription = cObject.detailedDescription
		self.deprecated = cObject.deprecated
		self.parent = namespace
	
	def get_namespace_object(self):
		if isinstance(self, (Namespace,Enum,Class)):
			return self
		elif self.parent is None:
			raise Error('{0} is not attached to a namespace object'.format(self))
		else:
			return self.parent.get_namespace_object()


class Namespace(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.children = []
	
	def add_child(self, child):
		self.children.append(child)
		child.parent = self


class Flag:
	def __init__(self, position):
		self.position = position


class EnumValue(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.value = None
	
	def value_from_string(self, stringValue):
		m = re.match('^1\s*<<\s*([0-9]+)$', stringValue)
		if m is not None:
			self.value = Flag(int(m.group(1)))
		else:
			self.value = int(stringValue, base=0)


class Enum(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.values = []
	
	def add_value(self, value):
		self.values.append(value)
		value.parent = self
	
	def set_from_c(self, cEnum, namespace=None):
		Object.set_from_c(self, cEnum, namespace=namespace)
		
		if 'associatedTypedef' in dir(cEnum):
			name = cEnum.associatedTypedef.name
		else:
			name = cEnum.name
		
		self.name = EnumName()
		self.name.prev = None if namespace is None else namespace.name
		self.name.set_from_c(name)
		
		for cEnumValue in cEnum.values:
			aEnumValue = EnumValue()
			aEnumValue.set_from_c(cEnumValue, namespace=self)
			self.add_value(aEnumValue)


class Argument(DocumentableObject):
	def __init__(self, name, argType, optional=False, default=None):
		DocumentableObject.__init__(self, name)
		self._type = argType
		argType.parent = self
		self.optional = optional
		self.default = default
	
	def _set_type(self, _type):
		self._type = _type
		_type.parent = self
	
	def _get_type(self):
		return self._type
	
	type = property(fset=_set_type, fget=_get_type)


class Method(DocumentableObject):
	class Type:
		Instance = 0,
		Class = 1
	
	def __init__(self, name, type=Type.Instance):
		DocumentableObject.__init__(self, name)
		self.type = type
		self.constMethod = False
		self.args = []
		self._returnType = None
		
	def _set_return_type(self, returnType):
		self._returnType = returnType
		returnType.parent = self
	
	def _get_return_type(self):
		return self._returnType
	
	def add_arguments(self, arg):
		self.args.append(arg)
		arg.parent = self
	
	returnType = property(fset=_set_return_type, fget=_get_return_type)


class Property(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self._setter = None
		self._getter = None
		self._type = None
	
	def set_setter(self, setter):
		self._setter = setter
		setter.parent = self
	
	def get_setter(self):
		return self._setter
	
	def set_getter(self, getter):
		self._getter = getter
		if self._type is None:
			self._type = getter.returnType
		getter.parent = self
	
	def get_getter(self):
		return self._getter
	
	setter = property(fset=set_setter, fget=get_setter)
	getter = property(fset=set_getter, fget=get_getter)


class Class(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.properties = []
		self.instanceMethods = []
		self.classMethods = []
		self._listenerInterface = None
		self.multilistener = False
		self.refcountable = False
	
	def add_property(self, property):
		self.properties.append(property)
		property.parent = self
	
	def add_instance_method(self, method):
		self.instanceMethods.append(method)
		method.parent = self
	
	def add_class_method(self, method):
		self.classMethods.append(method)
		method.parent = self
	
	def set_listener_interface(self, interface):
		self._listenerInterface = interface
		interface._listenedClass = self
	
	def get_listener_interface(self):
		return self._listenerInterface
	
	listenerInterface = property(fget=get_listener_interface, fset=set_listener_interface)


class Interface(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.methods = []
		self._listenedClass = None
	
	def add_method(self, method):
		self.methods.append(method)
		method.parent = self
	
	def get_listened_class(self):
		return self._listenedClass
	
	listenedClass = property(fget=get_listened_class)


class CParser(object):
	def __init__(self, cProject):
		self.cBaseType = ['void', 'bool_t', 'char', 'short', 'int', 'long', 'size_t', 'time_t', 'float', 'double', 'LinphoneStatus']
		self.cListType = 'bctbx_list_t'
		self.regexFixedSizeInteger = '^(u?)int(\d?\d)_t$'
		self.methodBl = ['ref', 'unref', 'new', 'destroy', 'getCurrentCallbacks', 'setUserData', 'getUserData']
		self.functionBl = [
					   'linphone_factory_create_core', # manualy wrapped
					   'linphone_factory_create_core_with_config', # manualy wrapped
					   'linphone_vcard_get_belcard'] # manualy wrapped

		self.classBl = ['LpConfig']  # temporarly blacklisted
		
		# list of classes that must be concidered as refcountable even if
		# they are no ref()/unref() methods
		self.forcedRefcountableClasses = ['LinphoneFactory']
		
		self.cProject = cProject
		
		self.enumsIndex = {}
		for enum in self.cProject.enums:
			if enum.associatedTypedef is None:
				self.enumsIndex[enum.name] = None
			else:
				self.enumsIndex[enum.associatedTypedef.name] = None
		
		self.classesIndex = {}
		self.interfacesIndex = {}
		for _class in self.cProject.classes:
			if _class.name not in self.classBl:
				if _class.name.endswith('Cbs'):
					self.interfacesIndex[_class.name] = None
				else:
					self.classesIndex[_class.name] = None
		
		self.methodsIndex = {}
		for _class in self.cProject.classes:
			for funcname in _class.classMethods:
				self.methodsIndex[funcname] = None
			for funcname in _class.instanceMethods:
				self.methodsIndex[funcname] = None
			for _property in _class.properties.values():
				if _property.setter is not None:
					self.methodsIndex[_property.setter.name] = None
				if _property.getter is not None:
					self.methodsIndex[_property.getter.name] = None
		
		name = NamespaceName()
		name.from_snake_case('linphone')
		
		self.namespace = Namespace(name)
	
	def _is_blacklisted(self, name):
		if type(name) is MethodName:
			return name.to_camel_case(lower=True) in self.methodBl or name.to_c() in self.functionBl
		elif type(name) is ClassName:
			return name.to_c() in self.classBl
		else:
			return False
		
	def parse_all(self):
		for enum in self.cProject.enums:
			try:
				self.parse_enum(enum)
			except Error as e:
				print('Could not parse \'{0}\' enum: {1}'.format(enum.name, e.args[0]))
		
		for _class in self.cProject.classes:
			try:
				self.parse_class(_class)
			except BlacklistedException:
				pass
			except Error as e:
				print('Could not parse \'{0}\' class: {1}'.format(_class.name, e.args[0]))
		
		
		self._clean_all_indexes()
		self._fix_all_types()
		self._fix_all_docs()
	
	def _clean_all_indexes(self):
		for index in [self.classesIndex, self.interfacesIndex, self.methodsIndex]:
			self._clean_index(index)
	
	def _clean_index(self, index):
		keysToRemove = []
		for key in index.keys():
			if index[key] is None:
				keysToRemove.append(key)
		
		for key in keysToRemove:
			del index[key]
	
	def _class_is_refcountable(self, _class):
		if _class.name in self.forcedRefcountableClasses:
			return True
		
		for method in _class.instanceMethods:
			if method.startswith(_class.cFunctionPrefix) and method[len(_class.cFunctionPrefix):] == 'ref':
				return True
		return False
	
	def _fix_all_types_in_class_or_interface(self, _class):
		if _class is not None:
			if type(_class) is Class:
				self._fix_all_types_in_class(_class)
			else:
				self._fix_all_types_in_interface(_class)
	
	def _fix_all_types(self):
		for _class in self.interfacesIndex.values():
			self._fix_all_types_in_class_or_interface(_class)
		for _class in self.classesIndex.values():
			self._fix_all_types_in_class_or_interface(_class)
	
	def _fix_all_types_in_class(self, _class):
		for property in _class.properties:
			if property.setter is not None:
				self._fix_all_types_in_method(property.setter)
			if property.getter is not None:
				self._fix_all_types_in_method(property.getter)
		
		for method in (_class.instanceMethods + _class.classMethods):
			self._fix_all_types_in_method(method)
	
	def _fix_all_types_in_interface(self, interface):
		for method in interface.methods:
			self._fix_all_types_in_method(method)
	
	def _fix_all_types_in_method(self, method):
		try:
			self._fix_type(method.returnType)
			for arg in method.args:
				self._fix_type(arg.type)
		except Error as e:
			print('warning: some types could not be fixed in {0}() function: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
		
	def _fix_type(self, _type):
		if isinstance(_type, EnumType) and _type.desc is None:
			_type.desc = self.enumsIndex[_type.name]
		elif isinstance(_type, ClassType) and _type.desc is None:
			if _type.name in self.classesIndex:
				_type.desc = self.classesIndex[_type.name]
			else:
				_type.desc = self.interfacesIndex[_type.name]
		elif isinstance(_type, ListType) and _type.containedTypeDesc is None:
			if _type.containedTypeName in self.classesIndex:
				_type.containedTypeDesc = ClassType(_type.containedTypeName, classDesc=self.classesIndex[_type.containedTypeName])
			elif _type.containedTypeName in self.interfacesIndex:
				_type.containedTypeDesc = ClassType(_type.containedTypeName, classDesc=self.interfacesIndex[_type.containedTypeName])
			elif _type.containedTypeName in self.enumsIndex:
				_type.containedTypeDesc = EnumType(_type.containedTypeName, enumDesc=self.enumsIndex[_type.containedTypeName])
			else:
				if _type.containedTypeName is not None:
					_type.containedTypeDesc = self.parse_c_base_type(_type.containedTypeName)
				else:
					raise Error('bctbx_list_t type without specified contained type')
	
	def _fix_all_docs(self):
		for _class in self.classesIndex.values():
			if _class.briefDescription is not None:
				_class.briefDescription.resolve_all_references(self)
		for method in self.methodsIndex.values():
			if method.briefDescription is not None:
				method.briefDescription.resolve_all_references(self)
	
	def parse_enum(self, cenum):
		if 'associatedTypedef' in dir(cenum):
			nameStr = cenum.associatedTypedef.name
		else:
			nameStr = cenum.name
		
		name = EnumName()
		name.from_camel_case(nameStr, namespace=self.namespace.name)
		enum = Enum(name)
		enum.briefDescription = cenum.briefDoc
		self.namespace.add_child(enum)
		
		for cEnumValue in cenum.values:
			valueName = EnumValueName()
			valueName.from_camel_case(cEnumValue.name, namespace=name)
			aEnumValue = EnumValue(valueName)
			aEnumValue.briefDescription = cEnumValue.briefDoc
			if cEnumValue.value is not None:
				try:
					aEnumValue.value_from_string(cEnumValue.value)
				except ValueError:
					raise Error('{0} enum value has an invalid definition ({1})'.format(cEnumValue.name, cEnumValue.value))
			enum.add_value(aEnumValue)
		
		self.enumsIndex[nameStr] = enum
		return enum
	
	def parse_class(self, cclass):
		if cclass.name in self.classBl:
			raise BlacklistedException('{0} is blacklisted'.format(cclass.name));
		
		if cclass.name.endswith('Cbs'):
			_class = self._parse_listener(cclass)
			self.interfacesIndex[cclass.name] = _class
		else:
			_class = self._parse_class(cclass)
			self.classesIndex[cclass.name] = _class
		self.namespace.add_child(_class)
		return _class
	
	def _parse_class(self, cclass):
		name = ClassName()
		name.from_camel_case(cclass.name, namespace=self.namespace.name)
		_class = Class(name)
		_class.briefDescription = cclass.briefDoc
		_class.refcountable = self._class_is_refcountable(cclass)
		
		for cproperty in cclass.properties.values():
			try:
				if cproperty.name != 'callbacks':
					absProperty = self._parse_property(cproperty, namespace=name)
					_class.add_property(absProperty)
				else:
					_class.listenerInterface = self.interfacesIndex[cproperty.getter.returnArgument.ctype]
			except Error as e:
				print('Could not parse {0} property in {1}: {2}'.format(cproperty.name, cclass.name, e.args[0]))
		
		for cMethod in cclass.instanceMethods.values():
			try:
				method = self.parse_method(cMethod, namespace=name)
				if method.name.to_snake_case() == 'add_callbacks' or method.name.to_snake_case() == 'remove_callbacks':
					if _class.listenerInterface is None or not _class.multilistener:
						_class.multilistener = True
						_class.listenerInterface = self.interfacesIndex[_class.name.to_camel_case(fullName=True) + 'Cbs']
				elif isinstance(method.returnType, ClassType) and method.returnType.name.endswith('Cbs'):
					pass
				else:
					_class.add_instance_method(method)
					
			except BlacklistedException:
				pass
			except Error as e:
				print('Could not parse {0} function: {1}'.format(cMethod.name, e.args[0]))
				
		for cMethod in cclass.classMethods.values():
			try:
				method = self.parse_method(cMethod, type=Method.Type.Class, namespace=name)
				_class.add_class_method(method)
			except BlacklistedException:
				pass
			except Error as e:
				print('Could not parse {0} function: {1}'.format(cMethod.name, e.args[0]))
		
		return _class
	
	def _parse_property(self, cproperty, namespace=None):
		name = PropertyName()
		name.from_snake_case(cproperty.name)
		if (cproperty.setter is not None and len(cproperty.setter.arguments) == 1) or (cproperty.getter is not None and len(cproperty.getter.arguments) == 0):
			methodType = Method.Type.Class
		else:
			methodType = Method.Type.Instance
		aproperty = Property(name)
		if cproperty.setter is not None:
			method = self.parse_method(cproperty.setter, namespace=namespace, type=methodType)
			aproperty.setter = method
		if cproperty.getter is not None:
			method = self.parse_method(cproperty.getter, namespace=namespace, type=methodType)
			aproperty.getter = method
		return aproperty
	
	
	def _parse_listener(self, cclass):
		name = InterfaceName()
		name.from_camel_case(cclass.name, namespace=self.namespace.name)
		
		if name.words[len(name.words)-1] == 'cbs':
			name.words[len(name.words)-1] = 'listener'
		else:
			raise Error('{0} is not a listener'.format(cclass.name))
		
		listener = Interface(name)
		listener.briefDescription = cclass.briefDoc
		
		for property in cclass.properties.values():
			if property.name != 'user_data':
				try:
					method = self._parse_listener_property(property, listener, cclass.events)
					listener.add_method(method)
				except Error as e:
					print('Could not parse property \'{0}\' of listener \'{1}\': {2}'.format(property.name, cclass.name, e.args[0]))
		
		return listener
	
	def _parse_listener_property(self, property, listener, events):
		methodName = MethodName()
		methodName.from_snake_case(property.name)
		methodName.words.insert(0, 'on')
		methodName.prev = listener.name
		
		if property.getter is not None:
			eventName = property.getter.returnArgument.ctype
		elif property.setter is not None and len(property.setter.arguments) == 2:
			eventName = property.setter.arguments[1].ctype
		else:
			raise Error('event name for {0} property of {1} listener not found'.format(property.name, listener.name.to_snake_case(fullName=True)))
		
		try:
			event = events[eventName]
		except KeyError:
			raise Error('invalid event name \'{0}\''.format(eventName))
		
		method = Method(methodName)
		method.returnType = self.parse_type(event.returnArgument)
		for arg in event.arguments:
			argName = ArgName()
			argName.from_snake_case(arg.name)
			argument = Argument(argName, self.parse_type(arg))
			method.add_arguments(argument)
		
		return method
	
	def parse_method(self, cfunction, namespace, type=Method.Type.Instance):
		name = MethodName()
		name.from_snake_case(cfunction.name, namespace=namespace)
		
		if self._is_blacklisted(name):
			raise BlacklistedException('{0} is blacklisted'.format(name.to_c()));
		
		method = Method(name, type=type)
		method.briefDescription = cfunction.briefDoc
		method.deprecated = cfunction.deprecated
		method.returnType = self.parse_type(cfunction.returnArgument)
		
		for arg in cfunction.arguments:
			if type == Method.Type.Instance and arg is cfunction.arguments[0]:
				method.constMethod = ('const' in arg.completeType.split(' '))
			else:
				aType = self.parse_type(arg)
				argName = ArgName()
				argName.from_snake_case(arg.name)
				absArg = Argument(argName, aType)
				method.add_arguments(absArg)
		
		self.methodsIndex[cfunction.name] = method
		return method
	
	def parse_type(self, cType):
		if cType.ctype in self.cBaseType or re.match(self.regexFixedSizeInteger, cType.ctype):
			absType = self.parse_c_base_type(cType.completeType)
		elif cType.ctype in self.enumsIndex:
			absType = EnumType(cType.ctype, enumDesc=self.enumsIndex[cType.ctype])
		elif cType.ctype in self.classesIndex or cType.ctype in self.interfacesIndex:
			absType = ClassType(cType.ctype)
			absType.isconst = cType.completeType.startswith('const ')
			absType.isref = cType.completeType.endswith('*')
		elif cType.ctype == self.cListType:
			absType = ListType(cType.containedType)
		elif cType.ctype.endswith('Mask'):
			absType = BaseType('integer', isUnsigned=True)
		else:
			raise Error('Unknown C type \'{0}\''.format(cType.ctype))
		
		absType.cname = cType.completeType
		return absType
	
	def parse_c_base_type(self, cDecl):
		declElems = cDecl.split(' ')
		param = {}
		name = None
		for elem in declElems:
			if elem == 'const':
				if name is None:
					param['isconst'] = True
			elif elem == 'unsigned':
				param['isUnsigned'] = True
			elif elem == 'char':
				name = 'character'
			elif elem == 'void':
				name = 'void'
			elif elem == 'bool_t':
				name = 'boolean'
			elif elem in ['short', 'long']:
				param['size'] = elem
			elif elem == 'int':
				name = 'integer'
			elif elem == 'float':
				name = 'floatant'
				param['size'] = 'float'
			elif elem == 'size_t':
				name = 'size'
			elif elem == 'time_t':
				name = 'time'
			elif elem == 'double':
				name = 'floatant'
				if 'size' in param and param['size'] == 'long':
					param['size'] = 'long double'
				else:
					param['size'] = 'double'
			elif elem == 'LinphoneStatus':
				name = 'status'
			elif elem == '*':
				if name is not None:
					if name == 'character':
						name = 'string'
					elif name == 'string':
						name = 'string_array'
					elif 'isref' not in param or param['isref'] is False:
						param['isref'] = True
					else:
						raise Error('Unhandled double-pointer')
			else:
				matchCtx = re.match(self.regexFixedSizeInteger, elem)
				if matchCtx:
					name = 'integer'
					if matchCtx.group(1) == 'u':
						param['isUnsigned'] = True
					
					param['size'] = int(matchCtx.group(2))
					if param['size'] not in [8, 16, 32, 64]:
						raise Error('{0} C basic type has an invalid size ({1})'.format(cDecl, param['size']))
		
		
		if name is not None:
			return BaseType(name, **param)
		else:
			raise Error('could not find type in \'{0}\''.format(cDecl))
