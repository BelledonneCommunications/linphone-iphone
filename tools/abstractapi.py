
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
import metaname


class Error(RuntimeError):
	pass


class BlacklistedException(Error):
	pass


class Constant:
	pass


class Nil(Constant):
	def translate(self, langTranslator):
		return langTranslator.nilToken


class Boolean(Constant):
	def __init__(self, value=False):
		self.value = value
	
	def __bool__(self):
		return self.value
	
	def translate(self, langTranslator):
		return langTranslator.trueConstantToken if self else langTranslator.falseConstantToken


class Object(object):
	def __init__(self, name):
		self.name = name
		self.parent = None
		self.deprecated = False
	
	def __lt__(self, other):
		return self.name < other.name
	
	def find_first_ancestor_by_type(self, *types):
		ancestor = self.parent
		while ancestor is not None and type(ancestor) not in types:
			ancestor = ancestor.parent
		return ancestor


class Type(Object):
	def __init__(self, name, isconst=False, isref=False):
		Object.__init__(self, name)
		self.isconst = isconst
		self.isref = isref
		self.cDecl = None


class BaseType(Type):
	def __init__(self, name, isconst=False, isref=False, size=None, isUnsigned=False):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.size = size
		self.isUnsigned = isUnsigned
	
	def translate(self, translator, **params):
		return translator.translate_base_type(self, **params)


class EnumType(Type):
	def __init__(self, name, isconst=False, isref=False, enumDesc=None):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.desc = enumDesc
	
	def translate(self, translator, **params):
		return translator.translate_enum_type(self, **params)


class ClassType(Type):
	def __init__(self, name, isconst=False, isref=False, classDesc=None):
		Type.__init__(self, name, isconst=isconst, isref=isref)
		self.desc = classDesc
	
	def translate(self, translator, **params):
		return translator.translate_class_type(self, **params)


class ListType(Type):
	def __init__(self, containedTypeName, isconst=False, isref=False):
		Type.__init__(self, 'list', isconst=isconst, isref=isref)
		self.containedTypeName = containedTypeName
		self._containedTypeDesc = None
	
	def _set_contained_type_desc(self, desc):
		self._containedTypeDesc = desc
		desc.parent = self
	
	def _get_contained_type_desc(self):
		return self._containedTypeDesc
	
	containedTypeDesc = property(fset=_set_contained_type_desc, fget=_get_contained_type_desc)
	
	def translate(self, translator, **params):
		return translator.translate_list_type(self, **params)


class DocumentableObject(Object):
	def __init__(self, name):
		Object.__init__(self, name)
		self._briefDescription = None
		self._detailedDescription = None
		self.deprecated = None
	
	def _get_brief_description(self):
		return self._briefDescription
	
	def _set_brief_description(self, description):
		self._briefDescription = description
		description.relatedObject = self
	
	def _get_detailed_description(self):
		return self._detailedDescription
	
	def _set_detailed_description(self, description):
		self._detailedDescription = description
		description.relatedObject = self
	
	briefDescription = property(fget=_get_brief_description, fset=_set_brief_description)
	detailedDescription = property(fget=_get_detailed_description, fset=_set_detailed_description)
	
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


GlobalNs = Namespace('')


class Flag:
	def __init__(self, position):
		self.position = position


class Enumerator(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.value = None
	
	def value_from_string(self, stringValue):
		m = re.match('^\s*1\s*<<\s*([0-9]+)$', stringValue)
		if m is not None:
			self.value = Flag(int(m.group(1)))
		else:
			self.value = int(stringValue, base=0)
	
	def translate_value(self, translator):
		return translator.translate_enumerator_value(self.value)


class Enum(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.enumerators = []
	
	def add_enumerator(self, enumerator):
		self.enumerators.append(enumerator)
		enumerator.parent = self
	
	def set_from_c(self, cEnum, namespace=None):
		Object.set_from_c(self, cEnum, namespace=namespace)
		
		if 'associatedTypedef' in dir(cEnum):
			name = cEnum.associatedTypedef.name
		else:
			name = cEnum.name
		
		self.name = metaname.EnumName()
		self.name.prev = None if namespace is None else namespace.name
		self.name.set_from_c(name)
		
		for cEnumValue in cEnum.values:
			aEnumValue = Enumerator()
			aEnumValue.set_from_c(cEnumValue, namespace=self)
			self.add_enumerator(aEnumValue)


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
	
	def translate(self, translator, **params):
		return translator.translate_argument(self, **params)


class Method(DocumentableObject):
	class Type:
		Instance = 0,
		Class = 1
	
	def __init__(self, name, type=Type.Instance):
		DocumentableObject.__init__(self, name)
		self.type = type
		self.isconst = False
		self.args = []
		self._returnType = None
	
	def add_arguments(self, arg):
		self.args.append(arg)
		arg.parent = self
	
	def _set_return_type(self, returnType):
		self._returnType = returnType
		returnType.parent = self
	
	def _get_return_type(self):
		return self._returnType
	
	returnType = property(fset=_set_return_type, fget=_get_return_type)
	
	def translate_as_prototype(self, translator, **params):
		return translator.translate_method_as_prototype(self, **params)


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
	
	def sort(self):
		self.properties.sort()
		self.instanceMethods.sort()
		self.classMethods.sort()


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
	
	def sort(self):
		self.methods.sort()


class CParser(object):
	def __init__(self, cProject):
		self.cBaseType = ['void', 'bool_t', 'char', 'short', 'int', 'long', 'size_t', 'time_t', 'float', 'double', 'LinphoneStatus']
		self.cListType = 'bctbx_list_t'
		self.regexFixedSizeInteger = '^(u?)int(\d?\d)_t$'
		self.methodBl = ['ref', 'unref', 'new', 'destroy', 'getCurrentCallbacks', 'setUserData', 'getUserData']
		self.functionBl = [
					   'linphone_factory_create_core', # manually wrapped
					   'linphone_factory_create_core_2', # manually wrapped
					   'linphone_factory_create_core_with_config', # manually wrapped
					   'linphone_factory_create_core_with_config_2', # manually wrapped
					   'linphone_vcard_get_belcard'] # manually wrapped

		self.classBl = ['LpConfig']  # temporarly blacklisted
		
		# list of classes that must be concidered as refcountable even if
		# they are no ref()/unref() methods
		self.forcedRefcountableClasses = ['LinphoneFactory']
		
		self.cProject = cProject
		
		self.enumsIndex = {}
		self.enums = []
		for enum in self.cProject.enums:
			if enum.associatedTypedef is None:
				self.enumsIndex[enum.name] = None
			else:
				self.enumsIndex[enum.associatedTypedef.name] = None
		
		self.classesIndex = {}
		self.interfacesIndex = {}
		self.classes = []
		self.interfaces = []
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
		
		name = metaname.NamespaceName()
		name.from_snake_case('linphone')
		
		self.namespace = Namespace(name)
	
	def _is_blacklisted(self, name):
		if type(name) is metaname.MethodName:
			return name.to_camel_case(lower=True) in self.methodBl or name.to_c() in self.functionBl
		elif type(name) is metaname.ClassName:
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
		self._sortall()
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
	
	def _sortall(self):
		self.enums.sort()
		self.classes.sort()
		self.interfaces.sort()
		for class_ in self.classes:
			class_.sort()
		for interface in self.interfaces:
			interface.sort()
	
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
			self._fix_doc(_class)
		for enum in self.enumsIndex.values():
			self._fix_doc(enum)
			for enumerator in enum.enumerators:
				self._fix_doc(enumerator)
		for method in self.methodsIndex.values():
			self._fix_doc(method)
	
	def _fix_doc(self, obj):
		if obj.briefDescription is not None:
			obj.briefDescription.resolve_all_references(self)
		if obj.detailedDescription is not None:
			obj.detailedDescription.resolve_all_references(self)
	
	def parse_enum(self, cenum):
		if 'associatedTypedef' in dir(cenum):
			nameStr = cenum.associatedTypedef.name
		else:
			nameStr = cenum.name
		
		name = metaname.EnumName()
		name.from_camel_case(nameStr, namespace=self.namespace.name)
		enum = Enum(name)
		enum.briefDescription = cenum.briefDoc
		enum.detailedDescription = cenum.detailedDoc
		self.namespace.add_child(enum)
		
		for cEnumValue in cenum.values:
			valueName = metaname.EnumeratorName()
			valueName.from_camel_case(cEnumValue.name, namespace=name)
			aEnumValue = Enumerator(valueName)
			aEnumValue.briefDescription = cEnumValue.briefDoc
			aEnumValue.detailedDescription = cEnumValue.detailedDoc
			if cEnumValue.value is not None:
				try:
					aEnumValue.value_from_string(cEnumValue.value)
				except ValueError:
					raise Error('{0} enum value has an invalid definition ({1})'.format(cEnumValue.name, cEnumValue.value))
			enum.add_enumerator(aEnumValue)
		
		self.enumsIndex[nameStr] = enum
		self.enums.append(enum)
		return enum
	
	def parse_class(self, cclass):
		if cclass.name in self.classBl:
			raise BlacklistedException('{0} is blacklisted'.format(cclass.name));
		
		if cclass.name.endswith('Cbs'):
			_class = self._parse_listener(cclass)
			self.interfacesIndex[cclass.name] = _class
			self.interfaces.append(_class)
		else:
			_class = self._parse_class(cclass)
			self.classesIndex[cclass.name] = _class
			self.classes.append(_class)
		self.namespace.add_child(_class)
		return _class
	
	def _parse_class(self, cclass):
		name = metaname.ClassName()
		name.from_camel_case(cclass.name, namespace=self.namespace.name)
		_class = Class(name)
		_class.briefDescription = cclass.briefDoc
		_class.detailedDescription = cclass.detailedDoc
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
		name = metaname.PropertyName()
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
		name = metaname.InterfaceName()
		name.from_camel_case(cclass.name, namespace=self.namespace.name)
		
		if name.words[len(name.words)-1] == 'cbs':
			name.words[len(name.words)-1] = 'listener'
		else:
			raise Error('{0} is not a listener'.format(cclass.name))
		
		listener = Interface(name)
		listener.briefDescription = cclass.briefDoc
		listener.detailedDescription = cclass.detailedDoc
		
		for property in cclass.properties.values():
			if property.name != 'user_data':
				try:
					method = self._parse_listener_property(property, listener, cclass.events)
					listener.add_method(method)
				except Error as e:
					print('Could not parse property \'{0}\' of listener \'{1}\': {2}'.format(property.name, cclass.name, e.args[0]))
		
		return listener
	
	def _parse_listener_property(self, property, listener, events):
		methodName = metaname.MethodName()
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
			argName = metaname.ArgName()
			argName.from_snake_case(arg.name)
			argument = Argument(argName, self.parse_type(arg))
			method.add_arguments(argument)
		
		return method
	
	def parse_method(self, cfunction, namespace, type=Method.Type.Instance):
		name = metaname.MethodName()
		name.from_snake_case(cfunction.name, namespace=namespace)
		
		if self._is_blacklisted(name):
			raise BlacklistedException('{0} is blacklisted'.format(name.to_c()));
		
		method = Method(name, type=type)
		method.briefDescription = cfunction.briefDoc
		method.detailedDescription = cfunction.detailedDoc
		method.deprecated = cfunction.deprecated
		method.returnType = self.parse_type(cfunction.returnArgument)
		
		for arg in cfunction.arguments:
			if type == Method.Type.Instance and arg is cfunction.arguments[0]:
				method.isconst = ('const' in arg.completeType.split(' '))
			else:
				aType = self.parse_type(arg)
				argName = metaname.ArgName()
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
		
		absType.cDecl = cType.completeType
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


class Translator:
	instances = {}
	
	@staticmethod
	def get(langCode):
		try:
			if langCode not in Translator.instances:
				className = langCode + 'LangTranslator'
				_class = globals()[className]
				Translator.instances[langCode] = _class()
			
			return Translator.instances[langCode]
		except KeyError:
			raise ValueError("Invalid language code: '{0}'".format(langCode))


class CLikeLangTranslator(Translator):
	def translate_enumerator_value(self, value):
		if value is None:
			return None
		elif isinstance(value, int):
			return str(value)
		elif isinstance(value, Flag):
			return '1<<{0}'.format(value.position)
		else:
			raise TypeError('invalid enumerator value type: {0}'.format(value))
	
	def translate_argument(self, argument, **kargs):
		return '{0} {1}'.format(argument.type.translate(self, **kargs), argument.name.translate(self.nameTranslator))


class CLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('C')
		self.nilToken = 'NULL'
		self.falseConstantToken = 'FALSE'
		self.trueConstantToken = 'TRUE'
	
	def translate_base_type(self, _type):
		return _type.cDecl
	
	def translate_enum_type(self, _type):
		return _type.cDecl
	
	def translate_class_type(self, _type):
		return _type.cDecl
	
	def translate_list_type(self, _type):
		return _type.cDecl
	
	def translate_enumerator_value(self, value):
		if value is None:
			return None
		elif isinstance(value, int):
			return str(value)
		elif isinstance(value, Flag):
			return '1<<{0}'.format(value.position)
		else:
			raise TypeError('invalid enumerator value type: {0}'.format(value))
	
	def translate_method_as_prototype(self, method, **params):
		_class = method.find_first_ancestor_by_type(Class)
		params = '{const}{className} *obj'.format(
			className=_class.name.to_c(),
			const='const ' if method.isconst else ''
		)
		for arg in method.args:
			params += (', ' + arg.translate(self))
		
		return '{returnType} {name}({params})'.format(
			returnType=method.returnType.translate(self),
			name=method.name.translate(self.nameTranslator),
			params=params
		)


class CppLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('Cpp')
		self.nilToken = 'nullptr'
		self.falseConstantToken = 'false'
		self.trueConstantToken = 'true'
		self.ambigousTypes = []
	
	def translate_base_type(self, _type, showStdNs=True, namespace=None):
		if _type.name == 'void':
			if _type.isref:
				return 'void *'
			else:
				return 'void'
		elif _type.name == 'boolean':
			res = 'bool'
		elif _type.name == 'character':
			res = 'char'
		elif _type.name == 'size':
			res = 'size_t'
		elif _type.name == 'time':
			res = 'time_t'
		elif _type.name == 'integer':
			if _type.size is None:
				res = 'int'
			elif isinstance(_type.size, str):
				res = _type.size
			else:
				res = 'int{0}_t'.format(_type.size)
				
		elif _type.name == 'floatant':
			if _type.size is not None and _type.size == 'double':
				res = 'double'
			else:
				res = 'float'
		elif _type.name == 'status':
			res = 'linphone::Status'
		elif _type.name == 'string':
			res = CppLangTranslator.prepend_std('string', showStdNs)
			if type(_type.parent) is Argument:
				res += ' &'
		elif _type.name == 'string_array':
			res = '{0}<{1}>'.format(
				CppLangTranslator.prepend_std('list', showStdNs),
				CppLangTranslator.prepend_std('string', showStdNs)
			)
			if type(_type.parent) is Argument:
				res += ' &'
		else:
			raise Error('\'{0}\' is not a base abstract type'.format(_type.name))
		
		if _type.isUnsigned:
			if _type.name == 'integer' and isinstance(_type.size, int):
				res = 'u' + res
			else:
				res = 'unsigned ' + res
		
		if _type.isconst:
			if _type.name not in ['string', 'string_array'] or type(_type.parent) is Argument:
				res = 'const ' + res
		
		if _type.isref:
			res += ' *'
		return res
	
	def translate_enum_type(self, _type, showStdNs=True, namespace=None):
		if _type.desc is None:
			raise Error('{0} has not been fixed'.format(_type.name))
		
		if namespace is not None:
			nsName = namespace.name if namespace is not GlobalNs else None
		else:
			method = _type.find_first_ancestor_by_type(Method)
			nsName = metaname.Name.find_common_parent(_type.desc.name, method.name)
		
		return _type.desc.name.translate(self.nameTranslator, recursive=True, topAncestor=nsName)
	
	def translate_class_type(self, _type, showStdNs=True, namespace=None):
		if _type.desc is None:
			raise Error('{0} has not been fixed'.format(_type.name))
		
		if namespace is not None:
			nsName = namespace.name if namespace is not GlobalNs else None
		else:
			method = _type.find_first_ancestor_by_type(Method)
			nsName = metaname.Name.find_common_parent(_type.desc.name, method.name)
		
		if _type.desc.name.to_c() in self.ambigousTypes:
			nsName = None
		
		res = _type.desc.name.translate(self.nameTranslator, recursive=True, topAncestor=nsName)
		
		if _type.desc.refcountable:
			if _type.isconst:
				res = 'const ' + res
			if type(_type.parent) is Argument:
				return 'const {0}<{1}> &'.format(
					CppLangTranslator.prepend_std('shared_ptr', showStdNs),
					res
				)
			else:
				return '{0}<{1}>'.format(
					CppLangTranslator.prepend_std('shared_ptr', showStdNs),
					res
				)
		else:
			if type(_type.parent) is Argument:
				return 'const {0} &'.format(res)
			else:
				return '{0}'.format(res)
	
	def translate_list_type(self, _type, showStdNs=True, namespace=None):
		if _type.containedTypeDesc is None:
			raise Error('{0} has not been fixed'.format(_type.containedTypeName))
		elif isinstance(_type.containedTypeDesc, BaseType):
			res = _type.containedTypeDesc.translate(self)
		else:
			res = _type.containedTypeDesc.translate(self, showStdNs=showStdNs, namespace=namespace)
			
		if type(_type.parent) is Argument:
			return 'const {0}<{1} > &'.format(
				CppLangTranslator.prepend_std('list', showStdNs),
				res
			)
		else:
			return '{0}<{1} >'.format(
				CppLangTranslator.prepend_std('list', showStdNs),
				res
			)
	
	def translate_method_as_prototype(self, method, showStdNs=True, namespace=None):
		_class = method.find_first_ancestor_by_type(Class, Interface)
		if namespace is not None:
			if _class.name.to_c() in self.ambigousTypes:
				nsName = None
			else:
				nsName = namespace.name if namespace is not GlobalNs else None
		else:
			nsName = _class.name
		
		argsString = ''
		argStrings = []
		for arg in method.args:
			argStrings.append(arg.translate(self, showStdNs=showStdNs, namespace=namespace))
		argsString = ', '.join(argStrings)
		
		return '{_return} {name}({args}){const}'.format(
			_return=method.returnType.translate(self, ),
			name=method.name.translate(self.nameTranslator, recursive=True, topAncestor=nsName),
			args=argsString,
			const=' const' if method.isconst else ''
		)
	
	@staticmethod
	def prepend_std(string, prepend):
		return 'std::' + string if prepend else string


class CSharpLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('CSharp')
		self.nilToken = 'null'
		self.falseConstantToken = 'false'
		self.trueConstantToken = 'true'
	
	def translate_base_type(self, _type, dllImport=True):
		if _type.name == 'void':
				if _type.isref:
					return 'IntPtr'
				return 'void'
		elif _type.name == 'status':
			if dllImport:
				return 'int'
			else:
				return 'void'
		elif _type.name == 'boolean':
			if dllImport:
				res = 'char'
			else:
				res = 'bool'
		elif _type.name == 'integer':
			if _type.isUnsigned:
				res = 'uint'
			else:
				res = 'int'
		elif _type.name == 'string':
			if dllImport:
				if type(_type.parent) is Argument:
					return 'string'
				else:
					res = 'IntPtr' # Return as IntPtr and get string with Marshal.PtrToStringAnsi()
			else:
				return 'string'
		elif _type.name == 'character':
			if _type.isUnsigned:
				res = 'byte'
			else:
				res = 'sbyte'
		elif _type.name == 'time':
			res = 'long' #TODO check
		elif _type.name == 'size':
			res = 'long' #TODO check
		elif _type.name == 'floatant':
			return 'float'
		elif _type.name == 'string_array':
			if dllImport or type(_type.parent) is Argument:
				return 'IntPtr'
			else:
				return 'IEnumerable<string>'
		else:
			raise Error('\'{0}\' is not a base abstract type'.format(_type.name))
		
		return res
	
	def translate_enum_type(self, _type, dllImport=True):
		if dllImport and type(_type.parent) is Argument:
			return 'int'
		else:
			return _type.desc.name.translate(self.nameTranslator)
	
	def translate_class_type(self, _type, dllImport=True):
		return "IntPtr" if dllImport else _type.desc.name.translate(self.nameTranslator)
	
	def translate_list_type(self, _type, dllImport=True):
		if dllImport:
			return 'IntPtr'
		else:
			if type(_type.containedTypeDesc) is BaseType:
				if _type.containedTypeDesc.name == 'string':
					return 'IEnumerable<string>'
				else:
					raise Error('translation of bctbx_list_t of basic C types is not supported')
			elif type(_type.containedTypeDesc) is ClassType:
				ptrType = _type.containedTypeDesc.desc.name.translate(self.nameTranslator)
				return 'IEnumerable<' + ptrType + '>'
			else:
				if _type.containedTypeDesc:
					raise Error('translation of bctbx_list_t of enums')
				else:
					raise Error('translation of bctbx_list_t of unknow type !')
	
	def translate_argument(self, arg, dllImport=True):
		return '{0} {1}'.format(
			arg.type.translate(self, dllImport=dllImport),
			arg.name.translate(self.nameTranslator)
		)
	
	def translate_method_as_prototype(self, method):
		kargs = {
			'static'     : 'static ' if method.type == Method.Type.Class else '',
			'override'   : 'override ' if method.name.translate(self.nameTranslator) == 'ToString' else '',
			'returnType' : method.returnType.translate(self, dllImport=False),
			'name'       : method.name.translate(self.nameTranslator),
			'args'       : ''
		}
		for arg in method.args:
			if kargs['args'] != '':
				kargs['args'] += ', '
			kargs['args'] += arg.translate(self, dllImport=False)
		return '{static}{override}{returnType} {name}({args})'.format(**kargs)
