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

import argparse
import os
import sys
import pystache

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
print sys.path
import genapixml as CApi
import abstractapi as AbsApi
import metadoc

class CsharpTranslator(object):
	def __init__(self):
		self.ignore = []
		self.docTranslator = metadoc.SandcastleCSharpTranslator()

	def init_method_dict(self):
		methodDict = {}
		methodDict['has_impl'] = True
		methodDict['impl'] = {}
		methodDict['is_string_list'] = False
		methodDict['is_class_list'] = False
		methodDict['list_type'] = None
		methodDict['is_string'] = False
		methodDict['is_bool'] = False
		methodDict['is_class'] = False
		methodDict['is_enum'] = False
		methodDict['is_generic'] = False
		methodDict['takeRef'] = 'true'
		return methodDict

	def get_class_array_type(self, name):
		length = len(name)
		if length > 11 and name[:11] == 'IEnumerable':
			return name[12:length-1]
		return None

	def is_generic(self, methodDict):
		return not methodDict['is_string'] and not methodDict['is_bool'] and not methodDict['is_class'] and not methodDict['is_enum'] and methodDict['list_type'] == None

	def translate_method_name(self, name, recursive=False, topAncestor=None):
		translatedName = name.to_camel_case(lower=True)
			
		if name.prev is None or not recursive or name.prev is topAncestor:
			return translatedName

	def translate_argument_name(self, name):
		argname = name.to_camel_case(lower=True)
		arg = argname
		if argname == "params":
			arg = "parameters"
		elif argname == "event":
			arg = "ev"
		elif argname == "ref":
			arg = "reference"
		elif argname == "value":
			arg = "val"
		return arg

	def translate_base_type(self, _type, isArg, dllImport=True):
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
				res = 'int' # In C the bool_t is an integer
			else:
				res = 'bool'
		elif _type.name == 'integer':
			if _type.isUnsigned:
				res = 'uint'
			else:
				res = 'int'
		elif _type.name == 'string':
			if dllImport:
				if isArg:
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
			if dllImport or isArg:
				return 'IntPtr'
			else:
				return 'IEnumerable<string>'
		else:
			raise AbsApi.Error('\'{0}\' is not a base abstract type'.format(_type.name))

		return res

	def is_linphone_type(self, _type, isArg, dllImport=True):
		if type(_type) is AbsApi.ClassType:
			return False if dllImport else True
		elif type(_type) is AbsApi.EnumType:
			return False if dllImport else True
	
	def translate_type(self, _type, isArg, dllImport=True):
		if type(_type) is AbsApi.EnumType:
			if dllImport and isArg:
				return 'int'
			return _type.desc.name.to_camel_case()
		elif type(_type) is AbsApi.ClassType:
			return "IntPtr" if dllImport else _type.desc.name.to_camel_case()
		elif type(_type) is AbsApi.BaseType:
			return self.translate_base_type(_type, isArg, dllImport)
		elif type(_type) is AbsApi.ListType:
			if dllImport:
				return 'IntPtr'
			else:
				if type(_type.containedTypeDesc) is AbsApi.BaseType:
					if _type.containedTypeDesc.name == 'string':
						return 'IEnumerable<string>'
					else:
						raise AbsApi.Error('translation of bctbx_list_t of basic C types is not supported')
				elif type(_type.containedTypeDesc) is AbsApi.ClassType:
					ptrType = _type.containedTypeDesc.desc.name.to_camel_case()
					return 'IEnumerable<' + ptrType + '>'
				else:
					if _type.containedTypeDesc:
						raise AbsApi.Error('translation of bctbx_list_t of enums')
					else:
						raise AbsApi.Error('translation of bctbx_list_t of unknow type !')
	
	def translate_argument(self, arg, dllImport=True):
		return '{0} {1}'.format(self.translate_type(arg.type, True, dllImport), self.translate_argument_name(arg.name))
	
	def throws_exception(self, return_type):
		if type(return_type) is AbsApi.BaseType:
			if return_type.name == 'status':
				return True
		return False

	def translate_method(self, method, static=False, genImpl=True):
		if method.name.to_snake_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(method.name.to_snake_case(fullName=True)))

		methodElems = {}
		methodElems['return'] = self.translate_type(method.returnType, False)
		methodElems['name'] = method.name.to_c()
		methodElems['params'] = '' if static else 'IntPtr thiz'
		for arg in method.args:
			if arg is not method.args[0] or not static:
				methodElems['params'] += ', '
			methodElems['params'] += self.translate_argument(arg)
		
		methodDict = {}
		methodDict['prototype'] = "static extern {return} {name}({params});".format(**methodElems)

		methodDict['doc'] = self.docTranslator.translate(method.briefDescription)

		methodDict['has_impl'] = genImpl
		if genImpl:
			methodDict['impl'] = {}
			methodDict['impl']['static'] = 'static ' if static else ''
			methodDict['impl']['exception'] = self.throws_exception(method.returnType)
			methodDict['impl']['type'] = self.translate_type(method.returnType, False, False)
			methodDict['impl']['name'] = method.name.to_camel_case()
			methodDict['impl']['override'] = 'override ' if method.name.to_camel_case() == 'ToString' else ''
			methodDict['impl']['return'] = '' if methodDict['impl']['type'] == "void" else 'return '
			methodDict['impl']['c_name'] = method.name.to_c()
			methodDict['impl']['nativePtr'] = '' if static else ('nativePtr, ' if len(method.args) > 0 else 'nativePtr')

			methodDict['list_type'] = self.get_class_array_type(methodDict['impl']['type'])
			methodDict['is_string_list'] = methodDict['list_type'] == 'string'
			methodDict['is_class_list'] = not methodDict['list_type'] == None and not methodDict['list_type'] == 'string'

			methodDict['is_string'] = methodDict['impl']['type'] == "string"
			methodDict['is_bool'] = methodDict['impl']['type'] == "bool"
			methodDict['is_class'] = self.is_linphone_type(method.returnType, False, False) and type(method.returnType) is AbsApi.ClassType
			methodDict['is_enum'] = self.is_linphone_type(method.returnType, False, False) and type(method.returnType) is AbsApi.EnumType
			methodDict['is_generic'] = self.is_generic(methodDict)
			methodDict['takeRef'] = 'true'
			if type(method.returnType.parent) is AbsApi.Method and len(method.returnType.parent.name.words) >=1:
				if method.returnType.parent.name.words == ['new'] or method.returnType.parent.name.words[0] == 'create':
					methodDict['takeRef'] = 'false'

			methodDict['impl']['args'] = ''
			methodDict['impl']['c_args'] = ''
			for arg in method.args:
				if arg is not method.args[0]:
					methodDict['impl']['args'] += ', '
					methodDict['impl']['c_args'] += ', '
				if self.is_linphone_type(arg.type, False, False):
					if type(arg.type) is AbsApi.ClassType:
						argname = self.translate_argument_name(arg.name)
						methodDict['impl']['c_args'] += argname + " != null ? " + argname + ".nativePtr : IntPtr.Zero"
					else:
						methodDict['impl']['c_args'] += '(int)' + self.translate_argument_name(arg.name)
				elif self.translate_type(arg.type, False, False) == "bool":
					methodDict['impl']['c_args'] += self.translate_argument_name(arg.name) + " ? 1 : 0"
				elif self.get_class_array_type(self.translate_type(arg.type, False, False)) is not None:
					listtype = self.get_class_array_type(self.translate_type(arg.type, False, False))
					if listtype == 'string':
						methodDict['impl']['c_args'] += "StringArrayToBctbxList(" + self.translate_argument_name(arg.name) + ")"
					else:
						methodDict['impl']['c_args'] += "ObjectArrayToBctbxList<" + listtype + ">(" + self.translate_argument_name(arg.name) + ")"
				else:
					methodDict['impl']['c_args'] += self.translate_argument_name(arg.name)
				methodDict['impl']['args'] += self.translate_argument(arg, False)

		return methodDict
	
###########################################################################################################################################

	def translate_property_getter(self, prop, name, static=False):
		methodDict = self.translate_method(prop, static, False)

		methodDict['property_static'] = 'static ' if static else ''
		methodDict['property_return'] = self.translate_type(prop.returnType, False, False)
		methodDict['property_name'] = (name[3:] if len(name) > 3 else 'Instance') if name[:3] == "Get" else name

		methodDict['has_property'] = True
		methodDict['has_getter'] = True
		methodDict['has_setter'] = False
		methodDict['return'] = methodDict['property_return']
		methodDict['exception'] = self.throws_exception(prop.returnType)
		methodDict['getter_nativePtr'] = '' if static else 'nativePtr'
		methodDict['getter_c_name'] = prop.name.to_c()

		methodDict['list_type'] = self.get_class_array_type(methodDict['return'])
		methodDict['is_string_list'] = methodDict['list_type'] == 'string'
		methodDict['is_class_list'] = not methodDict['list_type'] == None and not methodDict['list_type'] == 'string'

		methodDict['is_string'] = methodDict['return'] == "string"
		methodDict['is_bool'] = methodDict['return'] == "bool"
		methodDict['is_class'] = self.is_linphone_type(prop.returnType, False, False) and type(prop.returnType) is AbsApi.ClassType
		methodDict['is_enum'] = self.is_linphone_type(prop.returnType, False, False) and type(prop.returnType) is AbsApi.EnumType
		methodDict['is_generic'] = self.is_generic(methodDict)
		methodDict['takeRef'] = 'true'
		if type(prop.returnType.parent) is AbsApi.Method and len(prop.returnType.parent.name.words) >=1:
			if prop.returnType.parent.name.words == ['new'] or prop.returnType.parent.name.words[0] == 'create':
				methodDict['takeRef'] = 'false'

		return methodDict

	def translate_property_setter(self, prop, name, static=False):
		methodDict = self.translate_method(prop, static, False)

		methodDict['property_static'] = 'static ' if static else ''
		methodDict['property_return'] = self.translate_type(prop.args[0].type, True, False)
		methodDict['property_name'] = (name[3:] if len(name) > 3 else 'Instance') if name[:3] == "Set" else name

		methodDict['has_property'] = True
		methodDict['has_getter'] = False
		methodDict['has_setter'] = True
		methodDict['return'] = methodDict['property_return']
		methodDict['exception'] = self.throws_exception(prop.returnType)
		methodDict['setter_nativePtr'] = '' if static else 'nativePtr, '
		methodDict['setter_c_name'] = prop.name.to_c()

		methodDict['list_type'] = self.get_class_array_type(methodDict['return'])
		methodDict['is_string_list'] = methodDict['list_type'] == 'string'
		methodDict['is_class_list'] = not methodDict['list_type'] == None and not methodDict['list_type'] == 'string'

		methodDict['is_string'] = methodDict['return'] == "string"
		methodDict['is_bool'] = methodDict['return'] == "bool"
		methodDict['is_class'] = self.is_linphone_type(prop.args[0].type, True, False) and type(prop.args[0].type) is AbsApi.ClassType
		methodDict['is_enum'] = self.is_linphone_type(prop.args[0].type, True, False) and type(prop.args[0].type) is AbsApi.EnumType
		methodDict['is_generic'] = self.is_generic(methodDict)

		return methodDict

	def translate_property_getter_setter(self, getter, setter, name, static=False):
		methodDict = self.translate_property_getter(getter, name, static)
		methodDictSet = self.translate_property_setter(setter, name, static)

		protoElems = {}
		methodDict["prototype"] = methodDict['prototype']
		methodDict["has_second_prototype"] = True
		methodDict["second_prototype"] = methodDictSet['prototype']

		methodDict['has_setter'] = True
		methodDict['exception'] = methodDictSet['exception']
		methodDict['setter_nativePtr'] = methodDictSet['setter_nativePtr']
		methodDict['setter_c_name'] = methodDictSet['setter_c_name']

		return methodDict
	
	def translate_property(self, prop):
		res = []
		name = prop.name.to_camel_case()
		if prop.getter is not None:
			if prop.setter is not None:
				res.append(self.translate_property_getter_setter(prop.getter, prop.setter, name))
			else:
				res.append(self.translate_property_getter(prop.getter, name))
		elif prop.setter is not None:
			res.append(self.translate_property_setter(prop.setter, name))
		return res
	
###########################################################################################################################################

	def translate_listener(self, _class, method):
		listenedClass = method.find_first_ancestor_by_type(AbsApi.Interface).listenedClass

		listenerDict = {}
		c_name_setter = listenedClass.name.to_snake_case(fullName=True) + '_cbs_set_' + method.name.to_snake_case()[3:]
		delegate_name_public = method.name.to_camel_case() + "Delegate"
		delegate_name_private = delegate_name_public + "Private"
		listenerDict['cb_setter'] = {}
		listenerDict['cb_setter']['name'] = c_name_setter
		listenerDict['name_private'] = delegate_name_private

		listenerDict['delegate'] = {}
		listenerDict['delegate']['name_public'] = delegate_name_public
		listenerDict['delegate']['name_private'] = delegate_name_private
		var_name_public = method.name.to_snake_case() + '_public'
		var_name_private = method.name.to_snake_case() + '_private'
		listenerDict['delegate']['var_public'] = var_name_public
		listenerDict['delegate']['var_private'] = var_name_private
		listenerDict['delegate']['cb_name'] = method.name.to_snake_case()
		listenerDict['delegate']['name'] = method.name.to_camel_case()

		listenerDict['delegate']['interfaceClassName'] = listenedClass.name.to_camel_case()
		listenerDict['delegate']['isSimpleListener'] = not listenedClass.multilistener
		listenerDict['delegate']['isMultiListener'] = listenedClass.multilistener
		
		listenerDict['delegate']['params_public'] = ""
		listenerDict['delegate']['params_private'] = ""
		listenerDict['delegate']['params'] = ""
		for arg in method.args:
			dllImportType = self.translate_type(arg.type, True, True)
			normalType = self.translate_type(arg.type, True, False)

			argName = self.translate_argument_name(arg.name)
			if arg != method.args[0]:
				listenerDict['delegate']['params_public'] += ', '
				listenerDict['delegate']['params_private'] += ', '
				listenerDict['delegate']['params'] += ', '

				if normalType == dllImportType:
					listenerDict['delegate']['params'] += argName
				else:
					if normalType == "bool":
						listenerDict['delegate']['params'] += argName + " == 0"
					elif self.is_linphone_type(arg.type, True, False) and type(arg.type) is AbsApi.ClassType:
						listenerDict['delegate']['params'] += "fromNativePtr<" + normalType + ">(" + argName + ")"
					elif self.is_linphone_type(arg.type, True, False) and type(arg.type) is AbsApi.EnumType:
						listenerDict['delegate']['params'] += "(" + normalType + ")" + argName + ""
					else:
						raise("Error")
			else:
				listenerDict['delegate']['first_param'] = argName
				listenerDict['delegate']['params'] = 'thiz'
				
			listenerDict['delegate']['params_public'] += normalType + " " + argName
			listenerDict['delegate']['params_private'] += dllImportType + " " + argName

		listenerDict['delegate']["c_name_setter"] = c_name_setter
		return listenerDict

	def generate_getter_for_listener_callbacks(self, _class, classname):
		methodDict = self.init_method_dict()
		c_name = _class.name.to_snake_case(fullName=True) + '_get_callbacks'
		methodDict['prototype'] = "static extern IntPtr {c_name}(IntPtr thiz);".format(classname = classname, c_name = c_name)

		methodDict['has_impl'] = False
		methodDict['has_property'] = True
		methodDict['property_static'] = ''
		methodDict['property_return'] = classname
		methodDict['property_name'] = 'Listener'
		methodDict['has_getter'] = True
		methodDict['return'] = classname
		methodDict['getter_nativePtr'] = 'nativePtr'
		methodDict['getter_c_name'] = c_name
		methodDict['is_class'] = True

		return methodDict

	def generate_add_for_listener_callbacks(self, _class, classname):
		methodDict = self.init_method_dict()
		c_name = _class.name.to_snake_case(fullName=True) + '_add_callbacks'
		methodDict['prototype'] = "static extern void {c_name}(IntPtr thiz, IntPtr cbs);".format(classname = classname, c_name = c_name)
		methodDict['impl']['static'] = ''
		methodDict['impl']['type'] = 'void'
		methodDict['impl']['name'] = 'AddListener'
		methodDict['impl']['return'] = ''
		methodDict['impl']['c_name'] = c_name
		methodDict['impl']['nativePtr'] = 'nativePtr, '
		methodDict['impl']['args'] = classname + ' cbs'
		methodDict['impl']['c_args'] = 'cbs != null ? cbs.nativePtr : IntPtr.Zero'
		methodDict['is_generic'] = True

		return methodDict

	def generate_remove_for_listener_callbacks(self, _class, classname):
		methodDict = self.init_method_dict()
		c_name = _class.name.to_snake_case(fullName=True) + '_remove_callbacks'
		methodDict['prototype'] = "static extern void {c_name}(IntPtr thiz, IntPtr cbs);".format(classname = classname, c_name = c_name)
		methodDict['impl']['static'] = ''
		methodDict['impl']['type'] = 'void'
		methodDict['impl']['name'] = 'RemoveListener'
		methodDict['impl']['return'] = ''
		methodDict['impl']['c_name'] = c_name
		methodDict['impl']['nativePtr'] = 'nativePtr, '
		methodDict['impl']['args'] = classname + ' cbs'
		methodDict['impl']['c_args'] = 'cbs != null ? cbs.nativePtr : IntPtr.Zero'
		methodDict['is_generic'] = True

		return methodDict
	
###########################################################################################################################################

	def translate_enum(self, enum):
		enumDict = {}
		enumDict['enumName'] = enum.name.to_camel_case()
		enumDict['doc'] = self.docTranslator.translate(enum.briefDescription)
		enumDict['values'] = []
		i = 0
		lastValue = None
		for enumValue in enum.values:
			enumValDict = {}
			enumValDict['name'] = enumValue.name.to_camel_case()
			enumValDict['doc'] = self.docTranslator.translate(enumValue.briefDescription)
			if type(enumValue.value) is int:
				lastValue = enumValue.value
				enumValDict['value'] = str(enumValue.value)
			elif type(enumValue.value) is AbsApi.Flag:
				enumValDict['value'] = '1<<' + str(enumValue.value.position)
			else:
				if lastValue is not None:
					enumValDict['value'] = lastValue + 1
					lastValue += 1
				else:
					enumValDict['value'] = i
			enumDict['values'].append(enumValDict)
			i += 1
		return enumDict

	def translate_class(self, _class):
		if _class.name.to_camel_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(_class.name.to_camel_case(fullName=True)))

		classDict = {}
		classDict['className'] = _class.name.to_camel_case()
		classDict['isLinphoneFactory'] = _class.name.to_camel_case() == "Factory"
		classDict['doc'] = self.docTranslator.translate(_class.briefDescription)
		classDict['dllImports'] = []

		islistenable = _class.listenerInterface is not None
		ismonolistenable = (islistenable and not _class.multilistener)
		if islistenable:
			listenerName = _class.listenerInterface.name.to_camel_case()
			if ismonolistenable:
				classDict['dllImports'].append(self.generate_getter_for_listener_callbacks(_class, listenerName))
			else:
				classDict['dllImports'].append(self.generate_add_for_listener_callbacks(_class, listenerName))
				classDict['dllImports'].append(self.generate_remove_for_listener_callbacks(_class, listenerName))
		
		for method in _class.classMethods:
			try:
				if 'get' in method.name.to_word_list():
					methodDict = self.translate_property_getter(method, method.name.to_camel_case(), True)
				#The following doesn't work because there a at least one method that has both getter and setter, 
				#and because it doesn't do both of them at once, property is declared twice
				#elif 'set' in method.name.to_word_list():
				#	methodDict = self.translate_property_setter(method, method.name.to_camel_case(), True)
				else:
					methodDict = self.translate_method(method, static=True, genImpl=True)
				classDict['dllImports'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

		for prop in _class.properties:
			try:
				classDict['dllImports'] += self.translate_property(prop)
			except AbsApi.Error as e:
				print('error while translating {0} property: {1}'.format(prop.name.to_snake_case(), e.args[0]))

		for method in _class.instanceMethods:
			try:
				methodDict = self.translate_method(method, static=False, genImpl=True)
				classDict['dllImports'].append(methodDict)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

		return classDict

	def translate_interface(self, interface):
		if interface.name.to_camel_case(fullName=True) in self.ignore:
			raise AbsApi.Error('{0} has been escaped'.format(interface.name.to_camel_case(fullName=True)))

		interfaceDict = {}
		interfaceDict['interfaceName'] = interface.name.to_camel_case()
		interfaceDict['methods'] = []
		for method in interface.methods:
			interfaceDict['methods'].append(self.translate_listener(interface, method))
		
		return interfaceDict

###########################################################################################################################################

class EnumImpl(object):
	def __init__(self, enum, translator):
		namespace = enum.find_first_ancestor_by_type(AbsApi.Namespace)
		self.namespace = namespace.name.concatenate(fullName=True) if namespace is not None else None
		self.enum = translator.translate_enum(enum)

class ClassImpl(object):
	def __init__(self, _class, translator):
		namespace = _class.find_first_ancestor_by_type(AbsApi.Namespace)
		self.namespace = namespace.name.concatenate(fullName=True) if namespace is not None else None
		self._class = translator.translate_class(_class)

class InterfaceImpl(object):
	def __init__(self, interface, translator):
		namespace = interface.find_first_ancestor_by_type(AbsApi.Namespace)
		self.namespace = namespace.name.concatenate(fullName=True) if namespace is not None else None
		self.interface = translator.translate_interface(interface)

class WrapperImpl(object):
	def __init__(self, enums, interfaces, classes):
		self.enums = enums
		self.interfaces = interfaces
		self.classes = classes
	
###########################################################################################################################################

def render(renderer, item, path):
	tmppath = path + '.tmp'
	content = ''
	with open(tmppath, mode='w') as f:
		f.write(renderer.render(item))
	with open(tmppath, mode='rU') as f:
		content = f.read()
	with open(path, mode='w') as f:
		f.write(content)
	os.unlink(tmppath)

def main():
	argparser = argparse.ArgumentParser(description='Generate source files for the C++ wrapper')
	argparser.add_argument('xmldir', type=str, help='Directory where the XML documentation of the Linphone\'s API generated by Doxygen is placed')
	argparser.add_argument('-o --output', type=str, help='the directory where to generate the source files', dest='outputdir', default='.')
	argparser.add_argument('-n --name', type=str, help='the name of the genarated source file', dest='outputfile', default='LinphoneWrapper.cs')
	args = argparser.parse_args()
	
	entries = os.listdir(args.outputdir)
	
	project = CApi.Project()
	project.initFromDir(args.xmldir)
	project.check()
	
	parser = AbsApi.CParser(project)
	parser.functionBl = ['linphone_vcard_get_belcard', 'linphone_core_get_current_vtable']
	parser.classBl += 'LinphoneCoreVTable'
	parser.methodBl.remove('getCurrentCallbacks')
	parser.parse_all()
	translator = CsharpTranslator()
	renderer = pystache.Renderer()
	
	enums = []
	for item in parser.enumsIndex.items():
		if item[1] is not None:
			impl = EnumImpl(item[1], translator)
			enums.append(impl)
		else:
			print('warning: {0} enum won\'t be translated because of parsing errors'.format(item[0]))

	interfaces = []
	classes = []
	for _class in parser.classesIndex.values() + parser.interfacesIndex.values():
		if _class is not None:
			try:
				if type(_class) is AbsApi.Class:
					impl = ClassImpl(_class, translator)
					classes.append(impl)
				else:
					impl = InterfaceImpl(_class, translator)
					interfaces.append(impl)
			except AbsApi.Error as e:
				print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))

	wrapper = WrapperImpl(enums, interfaces, classes)
	render(renderer, wrapper, args.outputdir + "/" + args.outputfile)

if __name__ == '__main__':
	main()