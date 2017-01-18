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


import pystache
import re
import argparse
import os
import genapixml as CApi
import abstractapi as AbsApi


class CppTranslator(object):
	sharedPtrTypeExtractor = re.compile('^(const )?std::shared_ptr<(.+)>( &)?$')
	
	def __init__(self):
		self.ignore = []
		self.ambigousTypes = ['LinphonePayloadType']
	
	def is_ambigous_type(self, _type):
		return _type.name in self.ambigousTypes or (_type.name == 'list' and CppTranslator.is_ambigous_type(self, _type.containedTypeDesc))
	
	@staticmethod
	def translate_enum(enum):
		enumDict = {}
		enumDict['name'] = enum.name.to_camel_case()
		enumDict['values'] = []
		i = 0
		for enumValue in enum.values:
			enumValDict = CppTranslator.translate_enum_value(enumValue)
			enumValDict['notLast'] = (i != len(enum.values)-1)
			enumDict['values'].append(enumValDict)
			i += 1
		return enumDict
	
	@staticmethod
	def translate_enum_value(enumValue):
		enumValueDict = {}
		enumValueDict['name'] = CppTranslator.translate_enum_value_name(enumValue.name)
		return enumValueDict
	
	def translate_class(self, _class):
		if _class.name.to_camel_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(_class.name.to_camel_case(fullName=True)))
		
		islistenable = _class.listenerInterface is not None
		ismonolistenable = (islistenable and not _class.multilistener)
		ismultilistenable = (islistenable and _class.multilistener)
		
		classDict = {}
		classDict['islistenable'] = islistenable
		classDict['isnotlistenable'] = not islistenable
		classDict['ismonolistenable'] = ismonolistenable
		classDict['ismultilistenable'] = ismultilistenable
		classDict['isNotListener'] = True
		classDict['isfactory'] = (_class.name.to_c() == 'LinphoneFactory')
		classDict['isVcard'] = (_class.name.to_c() == 'LinphoneVcard')
		classDict['parentClassName'] = None
		classDict['className'] = CppTranslator.translate_class_name(_class.name)
		classDict['cClassName'] = '::' + _class.name.to_c()
		classDict['parentClassName'] = 'Object'
		classDict['methods'] = []
		classDict['staticMethods'] = []
		classDict['wrapperCbs'] = []
		classDict['friendClasses'] = []
		
		if _class.name.to_c() == 'LinphoneCore':
			classDict['friendClasses'].append({'name': 'Factory'});
		
		if islistenable:
			classDict['listenerClassName'] = CppTranslator.translate_class_name(_class.listenerInterface.name)
			classDict['cListenerName'] = _class.listenerInterface.name.to_c()
			for method in _class.listenerInterface.methods:
				classDict['wrapperCbs'].append(CppTranslator._generate_wrapper_callback(self, _class, method))
		
		if ismonolistenable:
			classDict['cCbsGetter'] = _class.name.to_snake_case(fullName=True) + '_get_callbacks'
			classDict['cppListenerName'] = CppTranslator.translate_class_name(_class.listenerInterface.name)
			classDict['parentClassName'] = 'ListenableObject'
		elif ismultilistenable:
			classDict['parentClassName'] = 'MultiListenableObject'
			classDict['listenerCreator'] = 'linphone_factory_create_' + _class.listenerInterface.name.to_snake_case()[:-len('_listener')] + '_cbs'
			classDict['callbacksAdder'] = _class.name.to_snake_case(fullName=True)+ '_add_callbacks'
			classDict['callbacksRemover'] = _class.name.to_snake_case(fullName=True)+ '_remove_callbacks'
			classDict['userDataSetter'] = _class.listenerInterface.name.to_snake_case(fullName=True)[:-len('_listener')] + '_cbs_set_user_data'
		
		for property in _class.properties:
			try:
				classDict['methods'] += CppTranslator.translate_property(self, property)
			except AbsApi.Error as e:
				print('error while translating {0} property: {1}'.format(property.name.to_snake_case(), e.args[0]))
		
		for method in _class.instanceMethods:
			try:
				methodDict = CppTranslator.translate_method(self, method)
				classDict['methods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
				
		for method in _class.classMethods:
			try:
				methodDict = CppTranslator.translate_method(self, method)
				classDict['staticMethods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
		
		return classDict
	
	def _generate_wrapper_callback(self, listenedClass, method):
		namespace = method.find_first_ancestor_by_type(AbsApi.Namespace)
		listenedClass = method.find_first_ancestor_by_type(AbsApi.Interface).listenedClass
		
		params = {}
		params['name'] = method.name.to_camel_case(lower=True)[2:] + 'Cb'
		params['name'] = params['name'][0].lower() + params['name'][1:]
		args = []
		wrappedArgs = []
		for arg in method.args:
			args.append(arg.type.cname + ' ' + arg.name.to_c())
			wrappedArgs.append(CppTranslator._wrap_c_expression_to_cpp(self, arg.name.to_c(), arg.type, usedNamespace=namespace))
		params['params'] = ', '.join(args)
		params['returnType'] = method.returnType.cname
		
		wrapperCbDict = {}
		wrapperCbDict['decl'] = 'static {returnType} {name}({params});'.format(**params)
		wrapperCbDict['cbName'] = params['name']
		wrapperCbDict['declArgs'] = params['params']
		#wrapperCbDict['methodName'] = method.name.to_camel_case(lower=True)
		#wrapperCbDict['wrappedArgs'] = ', '.join(wrappedArgs)
		wrapperCbDict['firstArgName'] = method.args[0].name.to_c()
		wrapperCbDict['returnType'] = params['returnType']
		wrapperCbDict['hasReturnValue'] = (params['returnType'] != 'void')
		wrapperCbDict['hasNotReturnValue'] = not wrapperCbDict['hasReturnValue']
		wrapperCbDict['callbackSetter'] = listenedClass.name.to_snake_case(fullName=True) + '_cbs_set_' + method.name.to_snake_case()[3:]
		wrapperCbDict['cppMethodCallingLine'] = 'listener->{methodName}({wrappedArgs})'.format(
			methodName=method.name.to_camel_case(lower=True),
			wrappedArgs=', '.join(wrappedArgs))
		wrapperCbDict['cppMethodCallingLine'] = CppTranslator._wrap_cpp_expression_to_c(self,
																				  wrapperCbDict['cppMethodCallingLine'],
																				  method.returnType)
		return wrapperCbDict
	
	def translate_interface(self, interface):
		if interface.name.to_camel_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(interface.name.to_camel_case(fullName=True)))
		
		intDict = {}
		intDict['inheritFrom'] = {'name': 'Listener'}
		intDict['className'] = CppTranslator.translate_class_name(interface.name)
		intDict['constructor'] = None
		intDict['parentClassName'] = 'Listener'
		intDict['isNotListener'] = False
		intDict['methods'] = []
		for method in interface.methods:
			try:
				methodDict = CppTranslator.translate_method(self, method, genImpl=False)
				intDict['methods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
		
		return intDict
	
	def translate_property(self, property):
		res = []
		if property.getter is not None:
			res.append(CppTranslator.translate_method(self, property.getter))
		if property.setter is not None:
			res.append(CppTranslator.translate_method(self, property.setter))
		return res
	
	def translate_method(self, method, genImpl=True):
		if method.name.to_snake_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(method.name.to_snake_case(fullName=True)))
		
		namespace = method.find_first_ancestor_by_type(AbsApi.Namespace)
		
		methodElems = {}
		methodElems['return'] = CppTranslator.translate_type(self, method.returnType)
		methodElems['name'] = CppTranslator.translate_method_name(method.name)
		
		methodElems['params'] = ''
		for arg in method.args:
			if arg is not method.args[0]:
				methodElems['params'] += ', '
			methodElems['params'] += CppTranslator.translate_argument(self, arg)
		
		methodElems['const'] = ' const' if method.constMethod else ''
		methodElems['semicolon'] = ';'
		if type(method.parent) is AbsApi.Class and method.type == AbsApi.Method.Type.Class:
			methodElems['methodType'] = 'static '
		elif type(method.parent) is AbsApi.Interface:
			methodElems['methodType'] = 'virtual '
			if isinstance(method.returnType, AbsApi.BaseType) and method.returnType.name == 'void':
				methodElems['semicolon'] = ' {}'
			else:
				methodElems['semicolon'] = ' = 0;'
		else:
			methodElems['methodType'] = ''
		
		methodElems['deprecated'] = 'LINPHONE_DEPRECATED ' if method.deprecated else ''
		
		methodDict = {}
		methodDict['prototype'] = 'LINPHONE_PUBLIC {deprecated}{methodType}{return} {name}({params}){const}{semicolon}'.format(**methodElems)
	
		if genImpl:
			if not CppTranslator.is_ambigous_type(self, method.returnType):
				methodElems['implReturn'] = CppTranslator.translate_type(self, method.returnType, namespace=namespace)
			else:
				methodElems['implReturn'] = CppTranslator.translate_type(self, method.returnType, namespace=None)
			
			methodElems['longname'] = CppTranslator.translate_method_name(method.name, recursive=True)
			methodElems['implParams'] = ''
			for arg in method.args:
				if arg is not method.args[0]:
					methodElems['implParams'] += ', '
				methodElems['implParams'] += CppTranslator.translate_argument(self, arg, namespace=namespace)
			
			methodDict['implPrototype'] = '{implReturn} {longname}({implParams}){const}'.format(**methodElems)
			methodDict['sourceCode' ] = CppTranslator._generate_source_code(self, method, usedNamespace=namespace)
		
		return methodDict
	
	def _generate_source_code(self, method, usedNamespace=None):
		nsName = usedNamespace.name if usedNamespace is not None else None
		params = {
			'functionName': method.name.to_c(),
			'args': CppTranslator._generate_wrapped_arguments(self, method, usedNamespace=usedNamespace)
		}
		
		if method.name.to_camel_case(lower=True) != 'setListener':
			cExpr = '{functionName}({args})'.format(**params)
			cppExpr = CppTranslator._wrap_c_expression_to_cpp(self, cExpr, method.returnType, usedNamespace=usedNamespace)
		else:
			cppExpr = 'ListenableObject::setListener(std::static_pointer_cast<Listener>({0}))'.format(method.args[0].name.to_snake_case())
		
		if type(method.returnType) is AbsApi.BaseType and method.returnType.name == 'void' and not method.returnType.isref:
			return cppExpr + ';'
		else:
			return 'return {0};'.format(cppExpr)
	
	def _generate_wrapped_arguments(self, method, usedNamespace=None):
		args = []
		if method.type == AbsApi.Method.Type.Instance:
			_class = method.find_first_ancestor_by_type(AbsApi.Class)
			argStr = '(::{0} *)mPrivPtr'.format(_class.name.to_camel_case(fullName=True))
			args.append(argStr)
		
		for arg in method.args:
			paramName = arg.name.to_camel_case(lower=True)
			args.append(CppTranslator._wrap_cpp_expression_to_c(self, paramName, arg.type, usedNamespace=usedNamespace))
		
		return ', '.join(args)
	
	def _wrap_cpp_expression_to_c(self, cppExpr, exprtype, usedNamespace=None):
		if type(exprtype) is AbsApi.BaseType:
			if exprtype.name == 'string':
				cExpr = 'cppStringToC({0})'.format(cppExpr);
			elif exprtype not in ['void', 'string', 'string_array'] and exprtype.isref:
				cExpr = '&' + cppExpr
			else:
				cExpr = cppExpr
		elif type(exprtype) is AbsApi.EnumType:
			cExpr = '(::{0}){1}'.format(exprtype.desc.name.to_c(), cppExpr)
		elif type(exprtype) is AbsApi.ClassType:
			param = {}
			param['ptrType'] = CppTranslator.translate_class_type(self, exprtype, namespace=usedNamespace)
			param['ptrType'] = CppTranslator.sharedPtrTypeExtractor.match(param['ptrType']).group(2)
			param['cPtrType'] = exprtype.desc.name.to_c()
			param['cppExpr'] = cppExpr
			param['object'] = 'const Object' if exprtype.isconst else 'Object'
			cExpr = '(::{cPtrType} *)sharedPtrToCPtr(std::static_pointer_cast<{object},{ptrType}>({cppExpr}))'.format(**param)
		elif type(exprtype) is AbsApi.ListType:
			if type(exprtype.containedTypeDesc) is AbsApi.BaseType and exprtype.containedTypeDesc.name == 'string':
				cExpr = 'StringBctbxListWrapper({0}).c_list()'.format(cppExpr)
			elif type(exprtype.containedTypeDesc) is AbsApi.Class:
				ptrType = CppTranslator.translate_class_type(exprtype, namespace=usedNamespace)
				ptrType = CppTranslator.sharedPtrTypeExtractor.match(ptrType).group(2)
				cExpr = 'ObjectBctbxListWrapper<{0}>({1}).c_list()'.format(ptrType, cppExpr)
			else:
				raise AbsApi.Error('translation of bctbx_list_t of enums or basic C types is not supported')
		
		return cExpr
	
	def _wrap_c_expression_to_cpp(self, cExpr, exprtype, usedNamespace=None):
		if type(exprtype) is AbsApi.BaseType:
			if exprtype.name == 'void' and not exprtype.isref:
				return cExpr
			elif exprtype.name == 'string':
				return 'cStringToCpp({0})'.format(cExpr)
			elif exprtype.name == 'string_array':
				return 'cStringArrayToCppList({0})'.format(cExpr)
			else:
				return cExpr
		elif type(exprtype) is AbsApi.EnumType:
			cppEnumName = CppTranslator.translate_enum_type(self, exprtype, namespace=usedNamespace)
			return '({0}){1}'.format(cppEnumName, cExpr)
		elif type(exprtype) is AbsApi.ClassType:
			cppReturnType = CppTranslator.translate_class_type(self, exprtype, namespace=usedNamespace)
			cppReturnType = CppTranslator.sharedPtrTypeExtractor.match(cppReturnType).group(2)
			
			if type(exprtype.parent) is AbsApi.Method and len(exprtype.parent.name.words) >=1 and (exprtype.parent.name.words == ['new'] or exprtype.parent.name.words[0] == 'create'):
				return 'cPtrToSharedPtr<{0}>((::belle_sip_object_t *){1}, false)'.format(cppReturnType, cExpr)
			else:
				return 'cPtrToSharedPtr<{0}>((::belle_sip_object_t *){1})'.format(cppReturnType, cExpr)
		elif type(exprtype) is AbsApi.ListType:
			if type(exprtype.containedTypeDesc) is AbsApi.BaseType and exprtype.containedTypeDesc.name == 'string':
				return 'bctbxStringListToCppList({0})'.format(cExpr)
			elif type(exprtype.containedTypeDesc) is AbsApi.ClassType:
				cppReturnType = CppTranslator.translate_class_type(self, exprtype.containedTypeDesc, namespace=usedNamespace)
				cppReturnType = CppTranslator.sharedPtrTypeExtractor.match(cppReturnType).group(2)
				return 'bctbxObjectListToCppList<{0}>({1})'.format(cppReturnType, cExpr)
			else:
				raise AbsApi.Error('translation of bctbx_list_t of enums or basic C types is not supported')
		else:
			return cExpr
	
	def translate_argument(self, arg, **params):
		return '{0} {1}'.format(CppTranslator.translate_type(self, arg.type, **params), CppTranslator.translate_argument_name(arg.name))
	
	def translate_type(self, aType, **params):
		if type(aType) is AbsApi.BaseType:
			return CppTranslator.translate_base_type(self, aType)
		elif type(aType) is AbsApi.EnumType:
			return CppTranslator.translate_enum_type(self, aType, **params)
		elif type(aType) is AbsApi.ClassType:
			return CppTranslator.translate_class_type(self, aType, **params)
		elif type(aType) is AbsApi.ListType:
			return CppTranslator.translate_list_type(self, aType, **params)
		else:
			CppTranslator.fail(aType)
	
	def translate_base_type(self, _type):
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
		elif _type.name == 'string':
			res = 'std::string'
			if type(_type.parent) is AbsApi.Argument:
				res += ' &'
		elif _type.name == 'string_array':
			res = 'std::list<std::string>'
			if type(_type.parent) is AbsApi.Argument:
				res += ' &'
		else:
			raise AbsApi.Error('\'{0}\' is not a base abstract type'.format(_type.name))
		
		if _type.isUnsigned:
			if _type.name == 'integer' and isinstance(_type.size, int):
				res = 'u' + res
			else:
				res = 'unsigned ' + res
		
		if _type.isconst:
			if _type.name not in ['string', 'string_array'] or type(_type.parent) is AbsApi.Argument:
				res = 'const ' + res
		
		if _type.isref:
			res += ' &'
		return res
	
	def translate_enum_type(self, _type, **params):
		if _type.name in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(_type.name))
		
		if _type.desc is None:
			raise AbsApi.Error('{0} has not been fixed'.format(_type.name.to_camel_case(fullName=True)))
		
		if 'namespace' in params:
			nsName = params['namespace'].name if params['namespace'] is not None else None
		else:
			method = _type.find_first_ancestor_by_type(AbsApi.Method)
			nsName = AbsApi.Name.find_common_parent(_type.desc.name, method.name)
		
		return CppTranslator.translate_enum_name(_type.desc.name, recursive=True, topAncestor=nsName)
	
	def translate_class_type(self, _type, **params):
		if _type.name in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(_type.name))
		
		if _type.desc is None:
			raise AbsApi.Error('{0} has not been fixed'.format(_type.name))
		
		if 'namespace' in params:
			nsName = params['namespace'].name if params['namespace'] is not None else None
		else:
			method = _type.find_first_ancestor_by_type(AbsApi.Method)
			nsName = AbsApi.Name.find_common_parent(_type.desc.name, method.name)
		
		res = CppTranslator.translate_class_name(_type.desc.name, recursive=True, topAncestor=nsName)
		
		if _type.isconst:
			res = 'const ' + res
		
		if type(_type.parent) is AbsApi.Argument:
			return 'const std::shared_ptr<{0}> &'.format(res)
		else:
			return 'std::shared_ptr<{0}>'.format(res)
	
	def translate_list_type(self, _type, **params):
		if _type.containedTypeDesc is None:
			raise AbsApi.Error('{0} has not been fixed'.format(_type.containedTypeName))
		elif isinstance(_type.containedTypeDesc, AbsApi.BaseType):
			res = CppTranslator.translate_type(self, _type.containedTypeDesc)
		else:
			res = CppTranslator.translate_type(self, _type.containedTypeDesc, **params)
			
		if type(_type.parent) is AbsApi.Argument:
			return 'const std::list<{0} > &'.format(res)
		else:
			return 'std::list<{0} >'.format(res)
	
	@staticmethod
	def translate_name(aName, **params):
		if type(aName) is AbsApi.ClassName:
			return CppTranslator.translate_class_name(aName, **params)
		elif type(aName) is AbsApi.InterfaceName:
			return CppTranslator.translate_class_name(aName, **params)
		elif type(aName) is AbsApi.EnumName:
			return CppTranslator.translate_enum_name(aName, **params)
		elif type(aName) is AbsApi.EnumValueName:
			return CppTranslator.translate_enum_value_name(aName, **params)
		elif type(aName) is AbsApi.MethodName:
			return CppTranslator.translate_method_name(aName, **params)
		elif type(aName) is AbsApi.ArgName:
			return CppTranslator.translate_argument_name(aName, **params)
		elif type(aName) is AbsApi.NamespaceName:
			return CppTranslator.translate_namespace_name(aName, **params)
		elif type(aName) is AbsApi.PropertyName:
			return CppTranslator.translate_property_name(aName, **params)
		else:
			CppTranslator.fail(aName)
	
	@staticmethod
	def translate_class_name(name, recursive=False, topAncestor=None):
		if name.prev is None or not recursive or name.prev is topAncestor:
			return name.to_camel_case()
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return CppTranslator.translate_name(name.prev, **params) + '::' + name.to_camel_case()
	
	@staticmethod
	def translate_enum_name(name, recursive=False, topAncestor=None):
		params = {'recursive': recursive, 'topAncestor': topAncestor}
		return CppTranslator.translate_class_name(name, **params)
	
	@staticmethod
	def translate_enum_value_name(name, recursive=False, topAncestor=None):
		params = {'recursive': recursive, 'topAncestor': topAncestor}
		return CppTranslator.translate_enum_name(name.prev, **params) + name.to_camel_case()
	
	@staticmethod
	def translate_method_name(name, recursive=False, topAncestor=None):
		translatedName = name.to_camel_case(lower=True)
		if translatedName == 'new':
			translatedName = '_new'
			
		if name.prev is None or not recursive or name.prev is topAncestor:
			return translatedName
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return CppTranslator.translate_name(name.prev, **params) + '::' + translatedName
	
	@staticmethod
	def translate_namespace_name(name, recursive=False, topAncestor=None):
		if name.prev is None or not recursive or name.prev is topAncestor:
			return name.concatenate()
		else:
			params = {'recursive': recursive, 'topAncestor': topAncestor}
			return CppTranslator.translate_namespace_name(name.prev, **params) + '::' + name.concatenate()
	
	@staticmethod
	def translate_argument_name(name):
		return name.to_camel_case(lower=True)
	
	@staticmethod
	def translate_property_name(name):
		CppTranslator.translate_argument_name(name)
	
	@staticmethod
	def fail(obj):
		raise AbsApi.Error('Cannot translate {0} type'.format(type(obj)))


class EnumsHeader(object):
	def __init__(self, translator):
		self.translator = translator
		self.enums = []
	
	def add_enum(self, enum):
		self.enums.append(self.translator.translate_enum(enum))


class ClassHeader(object):
	def __init__(self, _class, translator, ignore=[]):
		if type(_class) is AbsApi.Class:
			self._class = translator.translate_class(_class)
		else:
			self._class = translator.translate_interface(_class)
		
		self.define = '_{0}_HH'.format(_class.name.to_snake_case(upper=True, fullName=True))
		self.filename = '{0}.hh'.format(_class.name.to_snake_case())
		self.priorDeclarations = []
		self.private_type = _class.name.to_camel_case(fullName=True)
		self.ignore = ignore
		
		self.includes = {'internal': [], 'external': []}
		includes = ClassHeader.needed_includes(self, _class)
		for include in includes['internal']:
			if _class.name.to_camel_case(fullName=True) == 'LinphoneCore' or (isinstance(_class, AbsApi.Interface) and _class.listenedClass is not None and include == _class.listenedClass.name.to_snake_case()):
				if include == 'enums':
					self.includes['internal'].append({'name': include})
				else:
					className = AbsApi.ClassName()
					className.from_snake_case(include)
					self.priorDeclarations.append({'name': className.to_camel_case()})
			else:
				self.includes['internal'].append({'name': include})
		
		for include in includes['external']:
			self.includes['external'].append({'name': include})
	
	def needed_includes(self, _class):
		includes = {'internal': set(), 'external': set()}
		
		if type(_class) is AbsApi.Class:
			includes['internal'].add('object')
			
			for property in _class.properties:
				if property.setter is not None:
					ClassHeader._needed_includes_from_method(self, property.setter, includes)
				if property.getter is not None:
					ClassHeader._needed_includes_from_method(self, property.getter, includes)
		
		if type(_class) is AbsApi.Class:
			methods = _class.classMethods + _class.instanceMethods
		else:
			methods = _class.methods
		
		for method in methods:
			ClassHeader._needed_includes_from_type(self, method.returnType, includes)
			for arg in method.args:
				ClassHeader._needed_includes_from_type(self, arg.type, includes)
		
		if isinstance(_class, AbsApi.Class) and _class.listenerInterface is not None:
			includes['internal'].add(_class.listenerInterface.name.to_snake_case())
		
		currentClassInclude = _class.name.to_snake_case()
		if currentClassInclude in includes['internal']:
			includes['internal'].remove(currentClassInclude)
			
		return includes
	
	def _needed_includes_from_method(self, method, includes):
		ClassHeader._needed_includes_from_type(self, method.returnType, includes)
		for arg in method.args:
			ClassHeader._needed_includes_from_type(self, arg.type, includes)
	
	def _needed_includes_from_type(self, _type, includes):
		if isinstance(_type, AbsApi.ClassType):
			includes['external'].add('memory')
			if _type.desc is not None and _type.name not in self.ignore:
				includes['internal'].add(_type.desc.name.to_snake_case())
		elif isinstance(_type, AbsApi.EnumType):
			includes['internal'].add('enums')
		elif isinstance(_type, AbsApi.BaseType):
			if _type.name == 'integer' and isinstance(_type.size, int):
				includes['external'].add('cstdint')
			elif _type.name == 'string':
				includes['external'].add('string')
		elif isinstance(_type, AbsApi.ListType):
			includes['external'].add('list')
			ClassHeader._needed_includes_from_type(self, _type.containedTypeDesc, includes)


class MainHeader(object):
	def __init__(self):
		self.includes = []
		self.define = '_LINPHONE_HH'
	
	def add_include(self, include):
		self.includes.append({'name': include})


class ClassImpl(object):
	def __init__(self, parsedClass, translatedClass):
		self._class = translatedClass
		self.filename = parsedClass.name.to_snake_case() + '.cc'
		self.internalIncludes = []
		self.internalIncludes.append({'name': parsedClass.name.to_snake_case() + '.hh'})
		self.internalIncludes.append({'name': 'coreapi/linphonecore.h'})
		
		namespace = parsedClass.find_first_ancestor_by_type(AbsApi.Namespace)
		self.namespace = namespace.name.concatenate(fullName=True) if namespace is not None else None

class CMakeLists(object):
	def __init__(self):
		self.classes = []
		self.interfaces = []


def main():
	argparser = argparse.ArgumentParser(description='Generate source files for the C++ wrapper')
	argparser.add_argument('xmldir', type=str, help='Directory where the XML documentation of the Linphone\'s API generated by Doxygen is placed')
	argparser.add_argument('-o --output', type=str, help='the directory where to generate the source files', dest='outputdir', default='.')
	args = argparser.parse_args()
	
	entries = os.listdir(args.outputdir)
	if 'include' not in entries:
		os.mkdir(args.outputdir + '/include')
	if 'src' not in entries:
		os.mkdir(args.outputdir + '/src')
	
	project = CApi.Project()
	project.initFromDir(args.xmldir)
	project.check()
	
	parser = AbsApi.CParser(project)
	parser.parse_all()
	translator = CppTranslator()
	renderer = pystache.Renderer()	
	
	header = EnumsHeader(translator)
	for item in parser.enumsIndex.items():
		if item[1] is not None:
			header.add_enum(item[1])
		else:
			print('warning: {0} enum won\'t be translated because of parsing errors'.format(item[0]))
	
	with open(args.outputdir + '/include/enums.hh', mode='w') as f:
		f.write(renderer.render(header))
	
	mainHeader = MainHeader()
	cmakelists = CMakeLists()
	
	for _class in parser.classesIndex.values() + parser.interfacesIndex.values():
		if _class is not None:
			try:
				header = ClassHeader(_class, translator, ignore=['LinphoneBuffer'])
				impl = ClassImpl(_class, header._class)
				
				headerName = _class.name.to_snake_case() + '.hh'
				sourceName = _class.name.to_snake_case() + '.cc'
				mainHeader.add_include(headerName)
				
				if type(_class) is AbsApi.Class:
					cmakelists.classes.append({'header': headerName, 'source': sourceName})
				else:
					cmakelists.interfaces.append({'header': headerName})
				
				with open(args.outputdir + '/include/' + header.filename, mode='w') as f:
					f.write(renderer.render(header))
				
				if type(_class) is AbsApi.Class:
					with open(args.outputdir + '/src/' + impl.filename, mode='w') as f:
						f.write(renderer.render(impl))
				
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
	
	with open(args.outputdir + '/include/linphone.hh', mode='w') as f:
		f.write(renderer.render(mainHeader))
	
	with open(args.outputdir + '/CMakeLists.txt', mode='w') as f:
		f.write(renderer.render(cmakelists))


if __name__ == '__main__':
	main()
