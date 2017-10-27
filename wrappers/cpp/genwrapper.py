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
import sys
import errno

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
import genapixml as CApi
import abstractapi as AbsApi
import metadoc
import metaname


class CppTranslator(object):
	sharedPtrTypeExtractor = re.compile('^(const )?std::shared_ptr<(.+)>( &)?$')
	
	def __init__(self):
		self.ambigousTypes = ['LinphonePayloadType']
		self.nameTranslator = metaname.Translator.get('Cpp')
		self.langTranslator = AbsApi.Translator.get('Cpp')
		self.langTranslator.ambigousTypes.append('LinphonePayloadType')
		self.docTranslator = metadoc.DoxygenTranslator('Cpp')
	
	def is_ambigous_type(self, _type):
		return _type.name in self.ambigousTypes or (_type.name == 'list' and self.is_ambigous_type(_type.containedTypeDesc))
	
	def translate_enum(self, enum):
		enumDict = {}
		enumDict['name'] = enum.name.to_camel_case()
		enumDict['doc'] = enum.briefDescription.translate(self.docTranslator)
		enumDict['enumerators'] = []
		for enumerator in enum.enumerators:
			enumeratorDict = self.translate_enumerator(enumerator)
			enumeratorDict['notLast'] = (enumerator is not enum.enumerators[-1])
			enumDict['enumerators'].append(enumeratorDict)
		return enumDict
	
	def translate_enumerator(self, enumerator):
		enumeratorDict = {
			'name'  : enumerator.name.translate(self.nameTranslator),
			'doc'   : enumerator.briefDescription.translate(self.docTranslator),
			'value' : enumerator.translate_value(self.langTranslator)
		}
		return enumeratorDict
	
	def translate_class(self, _class):
		islistenable = _class.listenerInterface is not None
		ismonolistenable = (islistenable and not _class.multilistener)
		ismultilistenable = (islistenable and _class.multilistener)
		
		classDict = {
			'islistenable'        : islistenable,
			'isnotlistenable'     : not islistenable,
			'ismonolistenable'    : ismonolistenable,
			'ismultilistenable'   : ismultilistenable,
			'isrefcountable'      : _class.refcountable,
			'isnotrefcountable'   : not _class.refcountable,
			'isNotListener'       : True,
			'isListener'          : False,
			'isfactory'           : (_class.name.to_c() == 'LinphoneFactory'),
			'isVcard'             : (_class.name.to_c() == 'LinphoneVcard'),
			'className'           : _class.name.translate(self.nameTranslator),
			'cClassName'          : '::' + _class.name.to_c(),
			'privCClassName'      : '_' + _class.name.to_c(),
			'parentClassName'     : 'Object' if _class.refcountable else None,
			'methods'             : [],
			'staticMethods'       : [],
			'wrapperCbs'          : [],
			'friendClasses'       : []
		}
		
		if _class.name.to_c() == 'LinphoneCore':
			classDict['friendClasses'].append({'name': 'Factory'});
		
		classDict['briefDoc'] = _class.briefDescription.translate(self.docTranslator, tagAsBrief=True)
		classDict['detailedDoc'] = _class.detailedDescription.translate(self.docTranslator)
		
		if islistenable:
			classDict['listenerClassName'] = _class.listenerInterface.name.translate(self.nameTranslator)
			classDict['cListenerName'] = _class.listenerInterface.name.to_c()
			classDict['cppListenerName'] = _class.listenerInterface.name.translate(self.nameTranslator)
			for method in _class.listenerInterface.methods:
				classDict['wrapperCbs'].append(self._generate_wrapper_callback(_class, method))
		
		if ismonolistenable:
			classDict['cCbsGetter'] = _class.name.to_snake_case(fullName=True) + '_get_callbacks'
			classDict['parentClassName'] = 'ListenableObject'
		elif ismultilistenable:
			classDict['parentClassName'] = 'MultiListenableObject'
			classDict['listenerCreator'] = 'linphone_factory_create_' + _class.listenerInterface.name.to_snake_case()[:-len('_listener')] + '_cbs'
			classDict['callbacksAdder'] = _class.name.to_snake_case(fullName=True)+ '_add_callbacks'
			classDict['callbacksRemover'] = _class.name.to_snake_case(fullName=True)+ '_remove_callbacks'
			classDict['userDataSetter'] = _class.listenerInterface.name.to_snake_case(fullName=True)[:-len('_listener')] + '_cbs_set_user_data'
			classDict['userDataGetter'] = _class.listenerInterface.name.to_snake_case(fullName=True)[:-len('_listener')] + '_cbs_get_user_data'
			classDict['currentCallbacksGetter'] = _class.name.to_snake_case(fullName=True) + '_get_current_callbacks'
		
		for _property in _class.properties:
			try:
				classDict['methods'] += self.translate_property(_property)
			except AbsApi.Error as e:
				print('error while translating {0} property: {1}'.format(_property.name.to_snake_case(), e.args[0]))
		
		for method in _class.instanceMethods:
			try:
				methodDict = self.translate_method(method)
				classDict['methods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
				
		for method in _class.classMethods:
			try:
				methodDict = self.translate_method(method)
				classDict['staticMethods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
		
		return classDict
	
	def _generate_wrapper_callback(self, listenedClass, method):
		namespace = method.find_first_ancestor_by_type(AbsApi.Namespace)
		listenedClass = method.find_first_ancestor_by_type(AbsApi.Interface).listenedClass
		
		params = {}
		params['name'] = method.name.to_snake_case(fullName=True) + '_cb'
		args = []
		wrappedArgs = []
		for arg in method.args:
			args.append(arg.type.cDecl + ' ' + arg.name.to_c())
			wrappedArgs.append(self._wrap_c_expression_to_cpp(arg.name.to_c(), arg.type, usedNamespace=namespace))
		params['params'] = ', '.join(args)
		params['returnType'] = method.returnType.cDecl
		
		wrapperCbDict = {}
		wrapperCbDict['cbName'] = params['name']
		wrapperCbDict['declArgs'] = params['params']
		wrapperCbDict['firstArgName'] = method.args[0].name.to_c()
		wrapperCbDict['returnType'] = params['returnType']
		wrapperCbDict['hasReturnValue'] = (params['returnType'] != 'void')
		wrapperCbDict['hasNotReturnValue'] = not wrapperCbDict['hasReturnValue']
		wrapperCbDict['callbackSetter'] = listenedClass.name.to_snake_case(fullName=True) + '_cbs_set_' + method.name.to_snake_case()[3:]
		wrapperCbDict['cppMethodCallingLine'] = 'listener->{methodName}({wrappedArgs})'.format(
			methodName=method.name.to_camel_case(lower=True),
			wrappedArgs=', '.join(wrappedArgs))
		wrapperCbDict['cppMethodCallingLine'] = self._wrap_cpp_expression_to_c(wrapperCbDict['cppMethodCallingLine'], method.returnType)
		return wrapperCbDict
	
	def translate_interface(self, interface):
		intDict = {
			'inheritFrom'     : {'name': 'Listener'},
			'className'       : interface.name.translate(self.nameTranslator),
			'constructor'     : None,
			'parentClassName' : 'Listener',
			'isNotListener'   : False,
			'isListener'      : True,
			'methods'         : []
		}
		for method in interface.methods:
			try:
				methodDict = self.translate_method(method, genImpl=False)
				intDict['methods'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))
		
		return intDict
	
	def translate_property(self, _property):
		res = []
		if _property.getter is not None:
			res.append(self.translate_method(_property.getter))
		if _property.setter is not None:
			res.append(self.translate_method(_property.setter))
		return res
	
	def translate_method(self, method, genImpl=True):
		namespace = method.find_first_ancestor_by_type(AbsApi.Namespace)
		
		methodDict = {
			'declPrototype': method.translate_as_prototype(self.langTranslator),
			'implPrototype': method.translate_as_prototype(self.langTranslator, namespace=namespace),
			'deprecated': method.deprecated,
			'suffix': '',
			'briefDoc': method.briefDescription.translate(self.docTranslator, tagAsBrief=True) if method.briefDescription is not None else None,
			'detailedDoc': method.detailedDescription.translate(self.docTranslator) if method.detailedDescription is not None else None
		}
		
		if type(method.parent) is AbsApi.Interface:
			if isinstance(method.returnType, AbsApi.BaseType) and method.returnType.name == 'void':
				methodDict['suffix'] = ' {}'
			else:
				methodDict['suffix'] = ' = 0'
	
		if genImpl:
			methodDict['sourceCode' ] = self._generate_source_code(method, usedNamespace=namespace)
		
		return methodDict
	
	def _generate_source_code(self, method, usedNamespace=None):
		nsName = usedNamespace.name if usedNamespace is not None else None
		params = {
			'functionName': method.name.to_c(),
			'args': self._generate_wrapped_arguments(method, usedNamespace=usedNamespace)
		}
		
		if method.name.to_camel_case(lower=True) != 'setListener':
			cExpr = '{functionName}({args})'.format(**params)
			cppExpr = self._wrap_c_expression_to_cpp(cExpr, method.returnType, usedNamespace=usedNamespace)
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
			args.append(self._wrap_cpp_expression_to_c(paramName, arg.type, usedNamespace=usedNamespace))
		
		return ', '.join(args)
	
	def _wrap_cpp_expression_to_c(self, cppExpr, exprtype, usedNamespace=None):
		if type(exprtype) is AbsApi.BaseType:
			if exprtype.name == 'string':
				cExpr = 'StringUtilities::cppStringToC({0})'.format(cppExpr);
			else:
				cExpr = cppExpr
		elif type(exprtype) is AbsApi.EnumType:
			cExpr = '(::{0}){1}'.format(exprtype.desc.name.to_c(), cppExpr)
		elif type(exprtype) is AbsApi.ClassType:
			cPtrType = exprtype.desc.name.to_c()
			if exprtype.desc.refcountable:
				ptrType = exprtype.translate(self.langTranslator, namespace=usedNamespace)
				ptrType = CppTranslator.sharedPtrTypeExtractor.match(ptrType).group(2)
				param = {
					'ptrType' : ptrType,
					'cPtrType': cPtrType,
					'cppExpr' : cppExpr,
					'object'  : 'const Object' if exprtype.isconst else 'Object'
				}
				cExpr = '(::{cPtrType} *)Object::sharedPtrToCPtr(std::static_pointer_cast<{object},{ptrType}>({cppExpr}))'.format(**param)
			else:
				if exprtype.isref:
					cExpr = '(const ::{_type} *)({expr}).c_struct()'.format(_type=cPtrType, expr=cppExpr)
				else:
					cExpr = '*(const ::{_type} *)({expr}).c_struct()'.format(_type=cPtrType, expr=cppExpr)
		elif type(exprtype) is AbsApi.ListType:
			if type(exprtype.containedTypeDesc) is AbsApi.BaseType and exprtype.containedTypeDesc.name == 'string':
				cExpr = 'StringBctbxListWrapper({0}).c_list()'.format(cppExpr)
			elif type(exprtype.containedTypeDesc) is AbsApi.ClassType:
				ptrType = exprtype.containedTypeDesc.translate(self.langTranslator, namespace=usedNamespace)
				if exprtype.containedTypeDesc.desc.refcountable:
					ptrType = CppTranslator.sharedPtrTypeExtractor.match(ptrType).group(2)
					cExpr = 'ObjectBctbxListWrapper<{0}>({1}).c_list()'.format(ptrType, cppExpr)
				else:
					cType = exprtype.containedTypeDesc.desc.name.to_c()
					if exprtype.isconst:
						cExpr = 'StructBctbxListWrapper<{0},{1}>({2}).c_list()'.format(ptrType, cType, cppExpr)
					else:
						cExpr = 'StructBctbxListWrapper<{0},{1}>::cppListToBctbxList({2})'.format(ptrType, cType, cppExpr)
			else:
				raise AbsApi.Error('translation of bctbx_list_t of enums or basic C types is not supported')
		
		return cExpr
	
	def _wrap_c_expression_to_cpp(self, cExpr, exprtype, usedNamespace=None):
		if type(exprtype) is AbsApi.BaseType:
			if exprtype.name == 'string':
				return 'StringUtilities::cStringToCpp({0})'.format(cExpr)
			elif exprtype.name == 'string_array':
				return 'StringUtilities::cStringArrayToCppList({0})'.format(cExpr)
			elif exprtype.name == 'boolean':
				return '({0} != FALSE)'.format(cExpr)
			else:
				return cExpr
		elif type(exprtype) is AbsApi.EnumType:
			cppEnumName = exprtype.translate(self.langTranslator, namespace=usedNamespace)
			return '({0}){1}'.format(cppEnumName, cExpr)
		elif type(exprtype) is AbsApi.ClassType:
			cppReturnType = exprtype.translate(self.langTranslator, namespace=usedNamespace)
			if exprtype.desc.refcountable:
				cppReturnType = CppTranslator.sharedPtrTypeExtractor.match(cppReturnType).group(2)
				
				if type(exprtype.parent) is AbsApi.Method and len(exprtype.parent.name.words) >=1 and (exprtype.parent.name.words == ['new'] or exprtype.parent.name.words[0] == 'create'):
					return 'Object::cPtrToSharedPtr<{0}>({1}, false)'.format(cppReturnType, cExpr)
				else:
					return 'Object::cPtrToSharedPtr<{0}>({1})'.format(cppReturnType, cExpr)
			else:
				if exprtype.isref:
					return '{0}({1})'.format(exprtype.desc.name.to_camel_case(), cExpr)
				else:
					return '{0}(StructWrapper<::{1}>({2}).ptr())'.format(
						exprtype.desc.name.to_camel_case(),
						exprtype.desc.name.to_c(),
						cExpr)
		elif type(exprtype) is AbsApi.ListType:
			if type(exprtype.containedTypeDesc) is AbsApi.BaseType and exprtype.containedTypeDesc.name == 'string':
				return 'StringBctbxListWrapper::bctbxListToCppList({0})'.format(cExpr)
			elif type(exprtype.containedTypeDesc) is AbsApi.ClassType:
				cppReturnType = exprtype.containedTypeDesc.translate(self.langTranslator, namespace=usedNamespace)
				if exprtype.containedTypeDesc.desc.refcountable:
					cppReturnType = CppTranslator.sharedPtrTypeExtractor.match(cppReturnType).group(2)
					return 'ObjectBctbxListWrapper<{0}>::bctbxListToCppList({1})'.format(cppReturnType, cExpr)
				else:
					cType = exprtype.containedTypeDesc.desc.name.to_c()
					return 'StructBctbxListWrapper<{0},{1}>::bctbxListToCppList({2})'.format(cppReturnType, cType, cExpr)
			else:
				raise AbsApi.Error('translation of bctbx_list_t of enums or basic C types is not supported')
		else:
			return cExpr
	
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
	def __init__(self, _class, translator):
		if type(_class) is AbsApi.Class:
			self._class = translator.translate_class(_class)
		else:
			self._class = translator.translate_interface(_class)
		
		self.define = '_{0}_HH'.format(_class.name.to_snake_case(upper=True, fullName=True))
		self.filename = '{0}.hh'.format(_class.name.to_snake_case())
		self.priorDeclarations = []
		self.private_type = _class.name.to_camel_case(fullName=True)
		
		self.includes = {'internal': [], 'external': []}
		includes = self.needed_includes(_class)
		for include in includes['internal']:
			if _class.name.to_camel_case(fullName=True) == 'LinphoneCore' or (isinstance(_class, AbsApi.Interface) and _class.listenedClass is not None and include == _class.listenedClass.name.to_snake_case()):
				if include == 'enums':
					self.includes['internal'].append({'name': include})
				else:
					className = metaname.ClassName()
					className.from_snake_case(include)
					self.priorDeclarations.append({'name': className.to_camel_case()})
			else:
				self.includes['internal'].append({'name': include})
		
		for include in includes['external']:
			self.includes['external'].append({'name': include})
	
	def needed_includes(self, _class):
		includes = {'internal': [], 'external': []}
		
		if type(_class) is AbsApi.Class:
			for _property in _class.properties:
				if _property.setter is not None:
					self._needed_includes_from_method(_property.setter, includes)
				if _property.getter is not None:
					self._needed_includes_from_method(_property.getter, includes)
		
		if type(_class) is AbsApi.Class:
			methods = _class.classMethods + _class.instanceMethods
		else:
			methods = _class.methods
		
		for method in methods:
			self._needed_includes_from_type(method.returnType, includes)
			for arg in method.args:
				self._needed_includes_from_type(arg.type, includes)
		
		if isinstance(_class, AbsApi.Class) and _class.listenerInterface is not None:
			self._add_include(includes, 'internal', _class.listenerInterface.name.to_snake_case())
		
		currentClassInclude = _class.name.to_snake_case()
		if currentClassInclude in includes['internal']:
			includes['internal'].remove(currentClassInclude)
			
		return includes
	
	def _needed_includes_from_method(self, method, includes):
		self._needed_includes_from_type(method.returnType, includes)
		for arg in method.args:
			self._needed_includes_from_type(arg.type, includes)
	
	def _needed_includes_from_type(self, _type, includes):
		if isinstance(_type, AbsApi.ClassType):
			self._add_include(includes, 'external', 'memory')
			if _type.desc is not None:
				self._add_include(includes, 'internal', _type.desc.name.to_snake_case())
		elif isinstance(_type, AbsApi.EnumType):
			self._add_include(includes, 'internal', 'enums')
		elif isinstance(_type, AbsApi.BaseType):
			if _type.name == 'integer' and isinstance(_type.size, int):
				self._add_include(includes, 'external', 'cstdint')
			elif _type.name == 'string':
				self._add_include(includes, 'external', 'string')
		elif isinstance(_type, AbsApi.ListType):
			self._add_include(includes, 'external', 'list')
			self._needed_includes_from_type(_type.containedTypeDesc, includes)
	
	def _add_include(self, includes, location, name):
		if not name in includes[location]:
			includes[location].append(name)


class MainHeader(object):
	def __init__(self):
		self.includes = []
		self.define = '_LINPHONE_HH'
	
	def add_include(self, include):
		self.includes.append({'name': include})


class ClassImpl(object):
	def __init__(self):
		self.classes = []
		self.namespace = 'linphone'

class GenWrapper(object):
	def __init__(self, includedir, srcdir, xmldir):
		self.includedir = includedir
		self.srcdir = srcdir

		project = CApi.Project()
		project.initFromDir(xmldir)
		project.check()
		
		self.parser = AbsApi.CParser(project)
		self.parser.parse_all()
		self.translator = CppTranslator()
		self.renderer = pystache.Renderer()
		self.mainHeader = MainHeader()
		self.impl = ClassImpl()

	def render_all(self):
		header = EnumsHeader(self.translator)
		for item in self.parser.enumsIndex.items():
			if item[1] is not None:
				header.add_enum(item[1])
			else:
				print('warning: {0} enum won\'t be translated because of parsing errors'.format(item[0]))
		
		self.render(header, self.includedir + '/enums.hh')
		self.mainHeader.add_include('enums.hh')
		
		for _class in self.parser.interfacesIndex.values():
			self.render_header(_class)
		for _class in self.parser.classesIndex.values():
			self.render_header(_class)
		
		self.render(self.mainHeader, self.includedir + '/linphone.hh')
		self.render(self.impl, self.srcdir + '/linphone++.cc')

	def render(self, item, path):
		tmppath = path + '.tmp'
		content = ''
		with open(tmppath, mode='w') as f:
			f.write(self.renderer.render(item))
		with open(tmppath, mode='rU') as f:
			content = f.read()
		with open(path, mode='w') as f:
			f.write(content)
		os.unlink(tmppath)

	def render_header(self, _class):
		if _class is not None:
			try:
				header = ClassHeader(_class, self.translator)
				headerName = _class.name.to_snake_case() + '.hh'
				self.mainHeader.add_include(headerName)
				self.render(header, self.includedir + '/' + header.filename)
				
				if type(_class) is not AbsApi.Interface:
					self.impl.classes.append(header._class)
				
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))

def main():
	argparser = argparse.ArgumentParser(description='Generate source files for the C++ wrapper')
	argparser.add_argument('xmldir', type=str, help='Directory where the XML documentation of the Linphone\'s API generated by Doxygen is placed')
	argparser.add_argument('-o --output', type=str, help='the directory where to generate the source files', dest='outputdir', default='.')
	args = argparser.parse_args()
	
	includedir = args.outputdir + '/include/linphone++'
	srcdir = args.outputdir + '/src'
	
	try:
		os.makedirs(includedir)
	except OSError as e:
		if e.errno != errno.EEXIST:
			print("Cannot create '{0}' directory: {1}".format(includedir, e.strerror))
			sys.exit(1)
	
	try:
		os.makedirs(srcdir)
	except OSError as e:
		if e.errno != errno.EEXIST:
			print("Cannot create '{0}' directory: {1}".format(srcdir, e.strerror))
			sys.exit(1)
	
	genwrapper = GenWrapper(includedir, srcdir, args.xmldir)
	genwrapper.render_all()


if __name__ == '__main__':
	main()
