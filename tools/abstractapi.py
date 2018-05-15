
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
import logging


logger = logging.getLogger(__name__)


class Error(Exception):
	@property
	def reason(self):
		return self.args[0]
	
	def __str__(self):
		return str(self.reason)
	
	@staticmethod
	def _name_get_type_as_string(name):
		if type(name) is metaname.ClassName:
			return 'class'
		elif type(name) is metaname.EnumName:
			return 'enum'
		elif type(name) is metaname.EnumeratorName:
			return 'enumerator'
		elif type(name) is metaname.MethodName:
			return 'function'
		else:
			raise TypeError('{0} not handled'.format(type(name)))


class ParsingError(Error):
	@property
	def context(self):
		return self.args[1] if len(self.args) >= 2 else None
	
	def __str__(self):
		if self.context is None:
			return Error.__str__(self)
		else:
			params = {
				'reason': self.reason,
				'name': self.context.to_c(addBrackets=True),
				'type_': Error._name_get_type_as_string(self.context)
			}
			return "error while parsing {name} {type_}: {reason}".format(**params)


class BlacklistedSymbolError(Error):
	@property
	def name(self):
		return self.args[0]
	
	def __str__(self):
		params = {
			'name': self.name.to_c(addBrackets=True),
			'type_': Error._name_get_type_as_string(self.name)
		}
		return "{name} {type_} has been blacklisted".format(**params)


class TranslationError(Error):
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
	
	def find_first_ancestor_by_type(self, *types, **kargs):
		try:
			priorAncestor = kargs['priorAncestor']
		except KeyError:
			priorAncestor = False

		current = self
		ancestor = self.parent
		while ancestor is not None and type(ancestor) not in types:
			current = ancestor
			ancestor = ancestor.parent
		return ancestor if not priorAncestor else current


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
	
	@property
	def containedTypeDesc(self):
		return self._containedTypeDesc
	
	@containedTypeDesc.setter
	def containedTypeDesc(self, desc):
		self._containedTypeDesc = desc
		desc.parent = self
	
	def translate(self, translator, **params):
		return translator.translate_list_type(self, **params)


class DocumentableObject(Object):
	def __init__(self, name):
		Object.__init__(self, name)
		self._briefDescription = None
		self._detailedDescription = None
	
	@property
	def briefDescription(self):
		return self._briefDescription
	
	@briefDescription.setter
	def briefDescription(self, description):
		self._briefDescription = description
		description.relatedObject = self
	
	@property
	def detailedDescription(self):
		return self._detailedDescription
	
	@detailedDescription.setter
	def detailedDescription(self, description):
		self._detailedDescription = description
		description.relatedObject = self
	
	def set_from_c(self, cObject, namespace=None):
		self.briefDescription = cObject.briefDescription
		self.detailedDescription = cObject.detailedDescription
		self.deprecated = cObject.deprecated
		self.parent = namespace
	
	def get_namespace_object(self):
		if isinstance(self, (Namespace,Enum,Class)):
			return self
		elif self.parent is None:
			return None
		else:
			return self.parent.get_namespace_object()


class Namespace(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self.enums = []
		self.classes = []
		self.interfaces = []

	def addenum(self, enum):
		Namespace._insert_element(self.enums, enum)
		enum.parent = self

	def delenum(self, enum):
		i = self.enums.index(enum)
		del self.enums[i]
		enum.parent = None

	def addclass(self, class_):
		Namespace._insert_element(self.classes, class_)
		class_.parent = self

	def delclass(self, class_):
		i = self.classes.index(class_)
		del self.classes[i]
		class_.parent = None
	
	def addinterface(self, interface):
		Namespace._insert_element(self.interfaces, interface)
		interface.parent = self

	def delinterface(self, interface):
		i = self.interfaces.index(interface)
		del self.interfaces[i]
		interface.parent = None

	@staticmethod
	def _insert_element(l, e):
		try:
			inspoint = next(x for x in l if e.name < x.name)
			index = l.index(inspoint)
			l.insert(index, e)
		except StopIteration:
			l.append(e)


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
	
	@property
	def type(self):
		return self._type
	
	@type.setter
	def type(self, _type):
		self._type = _type
		_type.parent = self
	
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
	
	@property
	def returnType(self):
		return self._returnType
	
	@returnType.setter
	def returnType(self, returnType):
		self._returnType = returnType
		returnType.parent = self
	
	def translate_as_prototype(self, translator, **params):
		return translator.translate_method_as_prototype(self, **params)


class Property(DocumentableObject):
	def __init__(self, name):
		DocumentableObject.__init__(self, name)
		self._setter = None
		self._getter = None
		self._type = None
	
	@property
	def setter(self):
		return self._setter

	@setter.setter
	def setter(self, setter):
		self._setter = setter
		setter.parent = self
	
	@property
	def getter(self):
		return self._getter
	
	@getter.setter
	def getter(self, getter):
		self._getter = getter
		if self._type is None:
			self._type = getter.returnType
		getter.parent = self


class Class(Namespace):
	def __init__(self, name):
		Namespace.__init__(self, name)
		self.properties = []
		self.instanceMethods = []
		self.classMethods = []
		self._listenerInterface = None
		self.multilistener = False
		self.refcountable = False

	def add_property(self, property):
		Namespace._insert_element(self.properties, property)
		property.parent = self
	
	def add_instance_method(self, method):
		Namespace._insert_element(self.instanceMethods, method)
		method.parent = self
	
	def add_class_method(self, method):
		Namespace._insert_element(self.classMethods, method)
		method.parent = self
	
	@property
	def listenerInterface(self):
		return self._listenerInterface
	
	@listenerInterface.setter
	def listenerInterface(self, interface):
		self._listenerInterface = interface
		interface._listenedClass = self
	
	def sort(self):
		self.properties.sort()
		self.instanceMethods.sort()
		self.classMethods.sort()


class Interface(Namespace):
	def __init__(self, name):
		Namespace.__init__(self, name)
		self.instanceMethods = []
		self.classMethods = []
		self._listenedClass = None

	def add_instance_methods(self, method):
		self.instanceMethods.append(method)
		method.parent = self

	def add_class_methods(self, method):
		self.classMethods.append(method)
		method.parent = self

	@property
	def listenedClass(self):
		return self._listenedClass

	@listenedClass.setter
	def listenedClass(self, method):
		self.instanceMethods.append(method)
		method.parent = self

	def sort(self):
		self.instanceMethods.sort()


class CParser(object):
	def __init__(self, cProject, classBlAppend=[]):
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
		for bl in classBlAppend:
			self.classBl.append(bl)
		
		# list of classes that must be concidered as refcountable even if
		# they are no ref()/unref() methods
		self.forcedRefcountableClasses = ['LinphoneFactory']
		
		self.enum_relocations = {
			'LinphoneAccountCreatorActivationCodeStatus' : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorDomainStatus'         : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorEmailStatus'          : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorLanguageStatus'       : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorPasswordStatus'       : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorPhoneNumberStatus'    : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorStatus'               : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorTransportStatus'      : 'LinphoneAccountCreator',
			'LinphoneAccountCreatorUsernameStatus'       : 'LinphoneAccountCreator',
			'LinphoneCallDir'                            : 'LinphoneCall',
			'LinphoneCallState'                          : 'LinphoneCall',
			'LinphoneCallStatus'                         : 'LinphoneCall',
			'LinphoneChatRoomState'                      : 'LinphoneChatRoom',
			'LinphoneChatMessageDirection'               : 'LinphoneChatMessage',
			'LinphoneChatMessageState'                   : 'LinphoneChatMessage',
			'LinphoneCoreLogCollectionUploadState'       : 'LinphoneCore',
			'LinphoneEventLogType'                       : 'LinphoneEventLog',
			'LinphoneFriendListStatus'                   : 'LinphoneFriendList',
			'LinphoneFriendListSyncStatus'               : 'LinphoneFriendList',
			'LinphonePlayerState'                        : 'LinphonePlayer',
			'LinphonePresenceActivityType'               : 'LinphonePresenceActivity',
			'LinphoneTunnelMode'                         : 'LinphoneTunnel'
		}

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
		
		name = metaname.NamespaceName()
		name.from_snake_case('linphone')
		
		self.namespace = Namespace(name)
		self._pending_enums = []
	
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
			except BlacklistedSymbolError as e:
				logger.debug(e)
		
		for class_ in self.cProject.classes:
			try:
				self.parse_class(class_)
			except BlacklistedSymbolError as e:
				logger.debug(e)
		
		self._treat_pending_enums()
		self._clean_all_indexes()
		self._fix_all_types()
		self._fix_all_docs()
	
	def _treat_pending_enums(self):
		for enum in self._pending_enums:
			try:
				enum_cname = enum.name.to_c()
				parent_cname = self.enum_relocations[enum_cname]
				newparent = self.classesIndex[parent_cname]
				enum.parent.delenum(enum)
				newparent.addenum(enum)
				enum.name.from_c(enum_cname, namespace=newparent.name)
			except KeyError:
				reason = "cannot move the enum inside {0} enum because it doesn't exsist".format(parent_cname)
				raise ParsingError(reason, context=enum.name)
		self._pending_enums = []

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
		for method in interface.instanceMethods:
			self._fix_all_types_in_method(method)
	
	def _fix_all_types_in_method(self, method):
		try:
			self._fix_type(method.returnType)
			for arg in method.args:
				self._fix_type(arg.type)
		except ParsingError as e:
			raise ParsingError(e, method.name)
		
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
					raise ParsingError('bctbx_list_t type without specified contained type')
	
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
		name = metaname.EnumName()
		name.from_camel_case(cenum.publicName, namespace=self.namespace.name)
		enum = Enum(name)
		enum.briefDescription = cenum.briefDoc
		enum.detailedDescription = cenum.detailedDoc
		self.namespace.addenum(enum)
		
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
					reason = '{0} enum value has an invalid definition ({1})'.format(cEnumValue.name, cEnumValue.value)
					context = metaname.EnumeratorName()
					context.from_camel_case(cEnumValue.name)
					raise ParsingError(reason, context)
			enum.add_enumerator(aEnumValue)
		
		self.enumsIndex[cenum.publicName] = enum
		if cenum.publicName in self.enum_relocations:
			self._pending_enums.append(enum)
		return enum
	
	def parse_class(self, cclass):
		if cclass.name in self.classBl:
			name = metaname.ClassName()
			name.from_snake_case(cclass.name)
			raise BlacklistedSymbolError(name)
		
		if cclass.name.endswith('Cbs'):
			_class = self._parse_listener(cclass)
			self.interfacesIndex[cclass.name] = _class
			self.namespace.addinterface(_class)
		else:
			_class = self._parse_class(cclass)
			self.classesIndex[cclass.name] = _class
			self.namespace.addclass(_class)
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
			except BlacklistedSymbolError as e:
				logger.debug(e)
		
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
			except BlacklistedSymbolError as e:
				logger.debug(e)
		
		for cMethod in cclass.classMethods.values():
			try:
				method = self.parse_method(cMethod, type=Method.Type.Class, namespace=name)
				_class.add_class_method(method)
			except BlacklistedSymbolError as e:
				logger.debug(e)
		
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
		try:
			name = metaname.InterfaceName()
			name.from_camel_case(cclass.name, namespace=self.namespace.name)
			name.words[-1] = 'listener'
			
			listener = Interface(name)
			listener.briefDescription = cclass.briefDoc
			listener.detailedDescription = cclass.detailedDoc
			
			for property in cclass.properties.values():
				if property.name != 'user_data':
					try:
						method = self._parse_listener_property(property, listener, cclass.events)
						listener.add_instance_methods(method)
					except BlacklistedSymbolError as e:
						logger.debug(e)
			
			return listener
		except ParsingError as e:
			context = metaname.ClassName()
			context.from_camel_case(cclass.name)
			raise ParsingError(e, context)
	
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
			raise ParsingError('event name for {0} property of {1} listener not found'.format(property.name, listener.name.to_c()))
		
		try:
			event = events[eventName]
		except KeyError:
			raise ParsingError('invalid event name \'{0}\''.format(eventName))
		
		method = Method(methodName)
		method.returnType = self.parse_type(event.returnArgument)
		for arg in event.arguments:
			argName = metaname.ArgName()
			argName.from_snake_case(arg.name)
			argument = Argument(argName, self.parse_type(arg))
			method.add_arguments(argument)
		method.briefDescription = event.briefDoc
		method.detailedDescription = event.detailedDoc
		
		return method
	
	def parse_method(self, cfunction, namespace, type=Method.Type.Instance):
		name = metaname.MethodName()
		name.from_snake_case(cfunction.name, namespace=namespace)
		
		try:
			if self._is_blacklisted(name):
				raise BlacklistedSymbolError(name)
			
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
		except ParsingError as e:
			raise ParsingError(e, name)
	
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
			raise ParsingError('Unknown C type \'{0}\''.format(cType.ctype))
		
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
						raise ParsingError('Unhandled double-pointer')
			else:
				matchCtx = re.match(self.regexFixedSizeInteger, elem)
				if matchCtx:
					name = 'integer'
					if matchCtx.group(1) == 'u':
						param['isUnsigned'] = True
					
					param['size'] = int(matchCtx.group(2))
					if param['size'] not in [8, 16, 32, 64]:
						raise ParsingError('{0} C basic type has an invalid size ({1})'.format(cDecl, param['size']))
		
		
		if name is not None:
			return BaseType(name, **param)
		else:
			raise ParsingError('could not find type in \'{0}\''.format(cDecl))



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

	@staticmethod
	def _namespace_to_name_translator_params(namespace):
		return {
			'recursive': True,
			'topAncestor': namespace.name if namespace is not None else None
		}


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
	
	def translate_argument(self, argument, hideArgName=False, namespace=None):
		ret = argument.type.translate(self, namespace=namespace)
		if not hideArgName:
			ret += (' ' + argument.name.translate(self.nameTranslator))
		return ret


class CLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('C')
		self.nilToken = 'NULL'
		self.falseConstantToken = 'FALSE'
		self.trueConstantToken = 'TRUE'
	
	def translate_base_type(self, _type, **kargs):
		return _type.cDecl
	
	def translate_enum_type(self, _type, **kargs):
		return _type.cDecl
	
	def translate_class_type(self, _type, **kargs):
		return _type.cDecl
	
	def translate_list_type(self, _type, **kargs):
		return _type.cDecl
	
	def translate_enumerator_value(self, value, **kargs):
		if value is None:
			return None
		elif isinstance(value, int):
			return str(value)
		elif isinstance(value, Flag):
			return '1<<{0}'.format(value.position)
		else:
			raise TypeError('invalid enumerator value type: {0}'.format(value))
	
	def translate_method_as_prototype(self, method, hideArguments=False, hideArgNames=False, hideReturnType=False, stripDeclarators=False, namespace=None):
		_class = method.find_first_ancestor_by_type(Class,Interface)
		params = []
		if not hideArguments:
			params.append('{const}{className} *obj'.format(
				className=_class.name.to_c(),
				const='const ' if method.isconst and not stripDeclarators else ''
			))
			for arg in method.args:
				params.append(arg.translate(self, hideArgName=hideArgNames))
		return '{returnType}{name}({params})'.format(
			returnType=(method.returnType.translate(self) + ' ') if not hideReturnType else '',
			name=method.name.translate(self.nameTranslator),
			params=', '.join(params)
		)


class CppLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('Cpp')
		self.nilToken = 'nullptr'
		self.falseConstantToken = 'false'
		self.trueConstantToken = 'true'
		self.ambigousTypes = []
	
	def translate_base_type(self, _type, namespace=None):
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
			res = 'std::string'
			if type(_type.parent) is Argument:
				res += ' &'
		elif _type.name == 'string_array':
			res = 'std::list<std::string>'
			if type(_type.parent) is Argument:
				res += ' &'
		else:
			raise TranslationError('\'{0}\' is not a base abstract type'.format(_type.name))
		
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
	
	def translate_enum_type(self, type_, namespace=None):
		if type_.desc is None:
			raise TranslationError('{0} has not been fixed'.format(type_.name))
		return type_.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))

	def translate_class_type(self, type_, namespace=None):
		if type_.desc is None:
			raise TranslationError('{0} has not been fixed'.format(type_.name))
		res = type_.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))
		
		if type_.desc.refcountable:
			if type_.isconst:
				res = 'const ' + res
			if type(type_.parent) is Argument:
				return 'const std::shared_ptr<{0}> &'.format(res)
			else:
				return 'std::shared_ptr<{0}>'.format(res)
		else:
			if type(type_.parent) is Argument:
				return 'const {0} &'.format(res)
			else:
				return '{0}'.format(res)
	
	def translate_list_type(self, _type, namespace=None):
		if _type.containedTypeDesc is None:
			raise TranslationError('{0} has not been fixed'.format(_type.containedTypeName))
		elif isinstance(_type.containedTypeDesc, BaseType):
			res = _type.containedTypeDesc.translate(self)
		else:
			res = _type.containedTypeDesc.translate(self, namespace=namespace)
			
		if type(_type.parent) is Argument:
			return 'const std::list<{0}> &'.format(res)
		else:
			return 'std::list<{0}>'.format(res)
	
	def translate_method_as_prototype(self, method, hideArguments=False, hideArgNames=False, hideReturnType=False, stripDeclarators=False, namespace=None):
		argsAsString = ', '.join([arg.translate(self, hideArgName=hideArgNames, namespace=namespace) for arg in method.args]) if not hideArguments else ''
		return '{return_}{name}({args}){const}'.format(
			return_=(method.returnType.translate(self, namespace=namespace) + ' ') if not hideReturnType else '',
			name=method.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace)),
			args=argsAsString,
			const=' const' if method.isconst and not stripDeclarators else ''
		)


class JavaLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('Java')
		self.nilToken = 'null'
		self.falseConstantToken = 'false'
		self.trueConstantToken = 'true'

	def translate_base_type(self, type_, native=False, jni=False, isReturn=False, namespace=None):
		if type_.name == 'string':
			if jni:
				return 'jstring'
			return 'String'
		elif type_.name == 'integer':
			if type_.size is not None and type_.isref:
				if jni:
					return 'jbyteArray'
				return 'byte[]'
			if jni:
				return 'jint'
			return 'int'
		elif type_.name == 'boolean':
			if jni:
				return 'jboolean'
			return 'boolean'
		elif type_.name == 'floatant':
			if jni:
				return 'jfloat'
			return 'float'
		elif type_.name == 'size':
			if jni:
				return 'jint'
			return 'int'
		elif type_.name == 'time':
			if jni:
				return 'jlong'
			return 'long'
		elif type_.name == 'status':
			if jni:
				return 'jint'
			if native:
				return 'int'
			return 'void'
		elif type_.name == 'string_array':
			if jni:
				return 'jobjectArray'
			return 'String[]'
		elif type_.name == 'character':
			if jni:
				return 'jchar'
			return 'char'
		elif type_.name == 'void':
			if isReturn:
				return 'void'
			if jni:
				return 'jobject'
			return 'Object'
		return type_.name

	def translate_enum_type(self, _type, native=False, jni=False, isReturn=False, namespace=None):
		if native:
			return 'int'
		elif jni:
			return 'jint'
		else:
			return _type.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))

	def translate_class_type(self, _type, native=False, jni=False, isReturn=False, namespace=None):
		if jni:
			return 'jobject'
		return _type.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))

	def translate_list_type(self, _type, native=False, jni=False, isReturn=False, namespace=None):
		if jni:
			if type(_type.containedTypeDesc) is ClassType:
				return 'jobjectArray'
			elif type(_type.containedTypeDesc) is BaseType:
				if _type.containedTypeDesc.name == 'string':
					return 'jobjectArray'
				return _type.containedTypeDesc.translate(self, jni=True) + 'Array'
			elif type(_type.containedTypeDesc) is EnumType:
				ptrtype = _type.containedTypeDesc.translate(self, native=native)
		ptrtype = ''
		if type(_type.containedTypeDesc) is ClassType:
			ptrtype = _type.containedTypeDesc.translate(self, native=native, namespace=namespace)
		elif type(_type.containedTypeDesc) is BaseType:
			ptrtype = _type.containedTypeDesc.translate(self, native=native, namespace=namespace)
		elif type(_type.containedTypeDesc) is EnumType:
			ptrtype = _type.containedTypeDesc.translate(self, native=native, namespace=namespace)
		else:
			if _type.containedTypeDesc:
				raise Error('translation of bctbx_list_t of ' + _type.containedTypeDesc.name)
			else:
				raise Error('translation of bctbx_list_t of unknow type !')
		return ptrtype + '[]'

	def translate_argument(self, arg, native=False, jni=False, hideArgName=False, namespace=None):
		res = arg.type.translate(self, native=native, jni=jni, namespace=namespace)
		if not hideArgName:
			res += (' ' + arg.name.translate(self.nameTranslator))
		return res

	def translate_method_as_prototype(self, method, hideArguments=False, hideArgNames=False, hideReturnType=False, stripDeclarators=False, namespace=None):
		return '{public}{returnType}{methodName}({arguments})'.format(
			public='public ' if not stripDeclarators else '',
			returnType=(method.returnType.translate(self, isReturn=True, namespace=namespace) + ' ') if not hideReturnType else '',
			methodName=method.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace)),
			arguments=', '.join([arg.translate(self, hideArgName=hideArgNames, namespace=namespace) for arg in method.args]) if not hideArguments else ''
		)


class CSharpLangTranslator(CLikeLangTranslator):
	def __init__(self):
		self.nameTranslator = metaname.Translator.get('CSharp')
		self.nilToken = 'null'
		self.falseConstantToken = 'false'
		self.trueConstantToken = 'true'
	
	def translate_base_type(self, _type, dllImport=True, namespace=None):
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
			raise TranslationError('\'{0}\' is not a base abstract type'.format(_type.name))
		
		return res
	
	def translate_enum_type(self, _type, dllImport=True, namespace=None):
		if dllImport and type(_type.parent) is Argument:
			return 'int'
		else:
			return _type.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))
	
	def translate_class_type(self, _type, dllImport=True, namespace=None):
		return "IntPtr" if dllImport else _type.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))
	
	def translate_list_type(self, _type, dllImport=True, namespace=None):
		if dllImport:
			return 'IntPtr'
		else:
			if type(_type.containedTypeDesc) is BaseType:
				if _type.containedTypeDesc.name == 'string':
					return 'IEnumerable<string>'
				else:
					raise TranslationError('translation of bctbx_list_t of basic C types is not supported')
			elif type(_type.containedTypeDesc) is ClassType:
				ptrType = _type.containedTypeDesc.desc.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace))
				return 'IEnumerable<' + ptrType + '>'
			else:
				if _type.containedTypeDesc:
					raise TranslationError('translation of bctbx_list_t of enums')
				else:
					raise TranslationError('translation of bctbx_list_t of unknow type !')
	
	def translate_argument(self, arg, dllImport=True, namespace=None):
		return '{0} {1}'.format(
			arg.type.translate(self, dllImport=dllImport, namespace=None),
			arg.name.translate(self.nameTranslator)
		)
	
	def translate_method_as_prototype(self, method, hideArguments=False, hideArgNames=False, hideReturnType=False, stripDeclarators=False, namespace=None):
		return '{static}{override}{returnType}{name}({args})'.format(
			static     = 'static ' if method.type == Method.Type.Class and not stripDeclarators else '',
			override   = 'override ' if method.name.translate(self.nameTranslator) == 'ToString' and not stripDeclarators else '',
			returnType = (method.returnType.translate(self, dllImport=False, namespace=namespace) + ' ') if not hideReturnType else '',
			name       = method.name.translate(self.nameTranslator, **Translator._namespace_to_name_translator_params(namespace)),
			args       = ', '.join([arg.translate(self, dllImport=False, namespace=namespace) for arg in method.args]) if not hideArguments else ''
		)
