# Copyright (C) 2014 Belledonne Communications SARL
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
from sets import Set
import sys


def strip_leading_linphone(s):
	if s.lower().startswith('linphone'):
		return s[8:]
	else:
		return s

def remove_useless_enum_prefix(senum, svalue):
	lenum = re.findall('[A-Z][^A-Z]*', senum)
	lvalue = re.findall('[A-Z][^A-Z]*', svalue)
	if len(lenum) == 0 or len(lvalue) == 0:
		return svalue
	if lenum[0] == lvalue[0]:
		i = 0
		while i < len(lenum) and lenum[i] == lvalue[i]:
			i += 1
		return ''.join(lvalue[i:])
	return svalue

def is_callback(s):
	return s.startswith('Linphone') and s.endswith('Cb')

def compute_event_name(s, className):
	s = strip_leading_linphone(s)
	s = s[len(className):-2] # Remove leading class name and tailing 'Cb'
	event_name = ''
	first = True
	for l in s:
		if l.isupper() and not first:
			event_name += '_'
		event_name += l.lower()
		first = False
	return event_name

def is_const_from_complete_type(complete_type):
	splitted_type = complete_type.split(' ')
	return 'const' in splitted_type


class HandWrittenCode:
	def __init__(self, _class, name, func_list, doc = ''):
		self._class = _class
		self.name = name
		self.func_list = func_list
		self.doc = doc

class HandWrittenInstanceMethod(HandWrittenCode):
	def __init__(self, _class, name, cfunction, doc = ''):
		HandWrittenCode.__init__(self, _class, name, [cfunction], doc)

class HandWrittenClassMethod(HandWrittenCode):
	def __init__(self, _class, name, cfunction, doc = ''):
		HandWrittenCode.__init__(self, _class, name, [cfunction], doc)

class HandWrittenDeallocMethod(HandWrittenCode):
	def __init__(self, _class, cfunction):
		HandWrittenCode.__init__(self, _class, 'dealloc', [cfunction], '')

class HandWrittenProperty(HandWrittenCode):
	def __init__(self, _class, name, getter_cfunction = None, setter_cfunction = None, doc = ''):
		func_list = []
		if getter_cfunction is not None:
			func_list.append(getter_cfunction)
		if setter_cfunction is not None:
			func_list.append(setter_cfunction)
		HandWrittenCode.__init__(self, _class, name, func_list, doc)
		self.getter_cfunction = getter_cfunction
		self.setter_cfunction = setter_cfunction


class ArgumentType:
	def __init__(self, basic_type, complete_type, contained_type, linphone_module):
		self.basic_type = basic_type
		self.complete_type = complete_type
		self.contained_type = contained_type
		self.linphone_module = linphone_module
		self.type_str = None
		self.check_condition = None
		self.convert_code = None
		self.convert_from_func = None
		self.free_convert_result_func = None
		self.fmt_str = 'O'
		self.cfmt_str = '%p'
		self.cnativefmt_str = '%p'
		self.use_native_pointer = False
		self.cast_convert_func_result = True
		self.is_linphone_object = False
		self.__compute()
		if (self.basic_type == 'MSList' or self.basic_type == 'bctbx_list_t') and self.contained_type is not None and self.contained_type != 'const char *':
			self.linphone_module.bctbxlist_types.add(self.contained_type)

	def __compute(self):
		splitted_type = self.complete_type.split(' ')
		if self.basic_type == 'char':
			if '*' in splitted_type:
				self.type_str = 'string'
				self.check_condition = "!PyString_Check({arg_name})"
				self.convert_code = "{result_name}{result_suffix} = {cast}PyString_AsString({arg_name});\n"
				self.fmt_str = 'z'
				self.cfmt_str = '\\"%s\\"'
			else:
				self.type_str = 'int'
				self.check_condition = "!PyInt_Check({arg_name}) && !PyLong_Check({arg_name})"
				self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AsLong({arg_name});\n"
				self.fmt_str = 'b'
				self.cfmt_str = '%08x'
		elif self.basic_type == 'int':
			if 'unsigned' in splitted_type:
				self.type_str = 'unsigned int'
				self.check_condition = "!PyInt_Check({arg_name})"
				self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AsUnsignedLongMask({arg_name});\n"
				self.fmt_str = 'I'
				self.cfmt_str = '%u'
			else:
				self.type_str = 'int'
				self.check_condition = "!PyInt_Check({arg_name})"
				self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AS_LONG({arg_name});\n"
				self.fmt_str = 'i'
				self.cfmt_str = '%d'
		elif self.basic_type in ['int8_t', 'int16_t' 'int32_t']:
			self.type_str = 'int'
			self.check_condition = "!PyInt_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AS_LONG({arg_name});\n"
			if self.basic_type == 'int8_t':
					self.fmt_str = 'c'
			elif self.basic_type == 'int16_t':
					self.fmt_str = 'h'
			elif self.basic_type == 'int32_t':
					self.fmt_str = 'l'
			self.cfmt_str = '%d'
		elif self.basic_type in ['uint8_t', 'uint16_t', 'uint32_t']:
			self.type_str = 'unsigned int'
			self.check_condition = "!PyInt_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AsUnsignedLongMask({arg_name});\n"
			if self.basic_type == 'uint8_t':
				self.fmt_str = 'b'
			elif self.basic_type == 'uint16_t':
				self.fmt_str = 'H'
			elif self.basic_type == 'uint32_t':
				self.fmt_str = 'k'
			self.cfmt_str = '%u'
		elif self.basic_type == 'int64_t':
			self.type_str = '64bits int'
			self.check_condition = "!PyInt_Check({arg_name}) && !PyLong_Check({arg_name})"
			self.convert_code = \
"""if (PyInt_Check({arg_name})) {result_name}{result_suffix} = {cast}(PY_LONG_LONG)PyInt_AsLong({arg_name});
	else if (PyLong_Check({arg_name})) {result_name}{result_suffix} = {cast}PyLong_AsLongLong({arg_name});
"""
			self.fmt_str = 'L'
			self.cfmt_str = '%ld'
		elif self.basic_type == 'uint64_t':
			self.type_str = '64bits unsigned int'
			self.check_condition = "!PyLong_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyLong_AsUnsignedLongLong({arg_name});\n"
			self.fmt_str = 'K'
			self.cfmt_str = '%lu'
		elif self.basic_type == 'size_t':
			self.type_str = 'int'
			self.check_condition = "!PyInt_Check({arg_name}) && !PyLong_Check({arg_name})"
			self.convert_code = \
"""if (PyInt_Check({arg_name})) {result_name}{result_suffix} = {cast}(size_t)PyInt_AsSsize_t({arg_name});
	else if (PyLong_Check({arg_name})) {result_name}{result_suffix} = {cast}(size_t)PyLong_AsSsize_t({arg_name});
"""
			self.fmt_str = 'n'
			self.cfmt_str = '%lu'
		elif self.basic_type == 'float':
			self.type_str = 'float'
			self.check_condition = "!PyFloat_Check({arg_name})"
			self.convert_code = \
"""if (PyInt_Check({arg_name})) {result_name}{result_suffix} = {cast}(float)PyInt_AsLong({arg_name});
	else if (PyLong_Check({arg_name})) {result_name}{result_suffix} = {cast}(float)PyLong_AsLong({arg_name});
	else if (PyFloat_Check({arg_name})) {result_name}{result_suffix} = {cast}(float)PyFloat_AsDouble({arg_name});
"""
			self.fmt_str = 'f'
			self.cfmt_str = '%f'
		elif self.basic_type == 'double':
			self.type_str = 'float'
			self.check_condition = "!PyFloat_Check({arg_name})"
			self.convert_code = \
"""if (PyInt_Check({arg_name})) {result_name}{result_suffix} = {cast}(double)PyInt_AsLong({arg_name});
	else if (PyLong_Check({arg_name})) {result_name}{result_suffix} = {cast}(double)PyLong_AsLong({arg_name});
	else if (PyFloat_Check({arg_name})) {result_name}{result_suffix} = {cast}(double)PyFloat_AsDouble({arg_name});
"""
			self.fmt_str = 'd'
			self.cfmt_str = '%f'
		elif self.basic_type == 'bool_t':
			self.type_str = 'bool'
			self.check_condition = "!PyBool_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyObject_IsTrue({arg_name});\n"
			self.convert_from_func = 'PyBool_FromLong'
			self.fmt_str = 'O'
			self.cfmt_str = '%p'
			self.cnativefmt_str = '%u'
		elif self.basic_type == 'time_t':
			self.type_str = 'DateTime'
			self.check_condition = "!PyDateTime_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyDateTime_As_time_t({arg_name});\n"
			self.convert_from_func = 'PyDateTime_From_time_t'
			self.fmt_str = 'O'
			self.cfmt_str = '%p'
			self.cnativefmt_str = '%ld'
		elif self.basic_type == 'MSList' or self.basic_type == 'bctbx_list_t':
			if self.contained_type == 'const char *':
				self.type_str = 'list of string'
				self.convert_code = "{result_name}{result_suffix} = {cast}PyList_AsBctbxListOfString({arg_name});\n"
				self.convert_from_func = 'PyList_FromBctbxListOfString'
			else:
				self.type_str = 'list of linphone.' + self.contained_type
				self.convert_code = "{result_name}{result_suffix} = {cast}PyList_AsBctbxListOf" + self.contained_type + "({arg_name});\n"
				self.convert_from_func = 'PyList_FromBctbxListOf' + self.contained_type
			if not is_const_from_complete_type(self.complete_type):
				self.free_convert_result_func = "bctbx_list_free"
			self.check_condition = "!PyList_Check({arg_name})"
			self.fmt_str = 'O'
			self.cfmt_str = '%p'
		elif self.basic_type == 'MSVideoSize':
			self.type_str = 'linphone.VideoSize'
			self.check_condition = "!PyLinphoneVideoSize_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyLinphoneVideoSize_AsMSVideoSize({arg_name});\n"
			self.convert_from_func = 'PyLinphoneVideoSize_FromMSVideoSize'
			self.fmt_str = 'O'
			self.cfmt_str = '%p'
			self.cast_convert_func_result = False
		elif self.basic_type == 'LCSipTransports':
			self.type_str = 'linphone.SipTransports'
			self.check_condition = "!PyLinphoneSipTransports_Check({arg_name})"
			self.convert_code = "{result_name}{result_suffix} = {cast}PyLinphoneSipTransports_AsLCSipTransports({arg_name});\n"
			self.convert_from_func = 'PyLinphoneSipTransports_FromLCSipTransports'
			self.fmt_str = 'O'
			self.cfmt_str = '%p'
			self.cast_convert_func_result = False
		else:
			if strip_leading_linphone(self.basic_type) in self.linphone_module.enum_names:
				self.type_str = 'int'
				self.check_condition = "!PyInt_Check({arg_name})"
				self.convert_code = "{result_name}{result_suffix} = {cast}PyInt_AsLong({arg_name});\n"
				self.fmt_str = 'i'
				self.cfmt_str = '%d'
			elif is_callback(self.complete_type):
				self.type_str = 'callable'
				self.check_condition = "!PyCallable_Check({arg_name})"
				self.cnativefmt_str = None
			elif '*' in splitted_type:
				self.type_str = 'linphone.' + strip_leading_linphone(self.basic_type)
				self.use_native_pointer = True
				self.is_linphone_object = True
			else:
				self.type_str = 'linphone.' + strip_leading_linphone(self.basic_type)
				self.is_linphone_object = True


class MethodDefinition:
	def __init__(self, linphone_module, class_, method_name = "", method_node = None):
		self.body = ''
		self.arg_names = []
		self.parse_tuple_format = ''
		self.build_value_format = ''
		self.return_type = 'void'
		self.return_complete_type = 'void'
		self.return_contained_type = None
		self.method_name = method_name
		self.method_node = method_node
		self.class_ = class_
		self.linphone_module = linphone_module
		self.self_arg = None
		self.xml_method_return = None
		self.xml_method_args = []
		self.method_type = 'instancemethod'

	def format_local_variables_definition(self):
		body = self.format_local_return_variables_definition()
		if self.self_arg is not None:
			body += "\t" + self.self_arg.get('completetype') + "native_ptr;\n"
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			self.parse_tuple_format += argument_type.fmt_str
			if is_callback(arg_complete_type):
				body += "\tPyObject * {arg_name};\n".format(arg_name=arg_name)
			elif argument_type.fmt_str == 'O' and argument_type.use_native_pointer:
				body += "\tPyObject * " + arg_name + ";\n"
				body += "\t" + arg_complete_type + " " + arg_name + "_native_ptr = NULL;\n"
			elif argument_type.fmt_str == 'O' and argument_type.convert_code is not None:
				body += "\tPyObject * " + arg_name + ";\n"
				body += "\t" + arg_complete_type + " " + arg_name + "_native_obj;\n"
			elif strip_leading_linphone(arg_complete_type) in self.linphone_module.enum_names:
				body += "\tint " + arg_name + ";\n"
			else:
				body += "\t" + arg_complete_type + " " + arg_name + ";\n"
			self.arg_names.append(arg_name)
		return body

	def format_deprecation_warning(self):
		if self.method_node is not None and self.method_node.get('deprecated') == 'true':
			print(self.class_['class_name'] + "." + self.method_name + " is deprecated")
			return "\tPyErr_WarnEx(PyExc_DeprecationWarning, \"{msg}\", 1);\n".format(msg="{class_name}.{method_name} is deprecated".format(class_name=self.class_['class_name'], method_name=self.method_name))
		return ""

	def format_arguments_parsing(self):
		class_native_ptr_check_code = ''
		if self.self_arg is not None:
			class_native_ptr_check_code = self.format_class_native_pointer_check(False)
		parse_tuple_code = ''
		if len(self.arg_names) > 0:
			parse_tuple_code = \
"""if (!PyArg_ParseTuple(args, "{fmt}", {args})) {{
		return NULL;
	}}
""".format(fmt=self.parse_tuple_format, args=', '.join(map(lambda a: '&' + a, self.arg_names)))
		args_conversion_code = ''
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O' and argument_type.convert_code is not None:
				args_conversion_code += argument_type.convert_code.format(result_name=arg_name, result_suffix='_native_obj', cast='', arg_name=arg_name)
		return \
"""	{class_native_ptr_check_code}
	{parse_tuple_code}
	{args_type_check_code}
	{args_native_ptr_check_code}
	{args_conversion_code}
""".format(class_native_ptr_check_code=class_native_ptr_check_code,
		parse_tuple_code=parse_tuple_code,
		args_type_check_code=self.format_args_type_check(),
		args_native_ptr_check_code=self.format_args_native_pointer_check(),
		args_conversion_code=args_conversion_code)

	def format_enter_trace(self):
		fmt = ''
		args = []
		if self.self_arg is not None:
			fmt += "%p [%p]"
			args += ["self", "native_ptr"]
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			if fmt != '':
				fmt += ', '
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			fmt += argument_type.cfmt_str
			args.append(arg_name)
			if argument_type.fmt_str == 'O' and argument_type.cnativefmt_str is not None:
				fmt += ' [' + argument_type.cnativefmt_str + ']'
				if argument_type.use_native_pointer:
					args.append(arg_name + '_native_ptr')
				else:
					args.append(arg_name + '_native_obj')
		args = ', '.join(args)
		if args != '':
			args = ', ' + args
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s({fmt})\", __FUNCTION__{args});\n".format(fmt=fmt, args=args)

	def format_c_function_call(self):
		arg_names = []
		c_function_call_code = ''
		cfree_argument_code = ''
		python_ref_code = ''
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O' and argument_type.use_native_pointer:
				arg_names.append(arg_name + "_native_ptr")
			elif argument_type.fmt_str == 'O' and argument_type.convert_code is not None:
				arg_names.append(arg_name + "_native_obj")
				if argument_type.free_convert_result_func is not None and not is_const_from_complete_type(arg_complete_type):
					cfree_argument_code = \
"""{free_func}({arg_name}_native_obj);
""".format(free_func=argument_type.free_convert_result_func, arg_name=arg_name)
			else:
				arg_names.append(arg_name)
		if is_callback(self.return_complete_type):
			c_function_call_code = "pyresult = ((pylinphone_{class_name}Object *)self)->{callback_name};".format(class_name=self.class_['class_name'], callback_name=compute_event_name(self.return_complete_type, self.class_['class_name']))
		else:
			if self.return_complete_type != 'void':
				c_function_call_code += "cresult = "
			c_function_call_code += self.method_node.get('name') + "("
			if self.self_arg is not None:
				c_function_call_code += "native_ptr"
				if len(arg_names) > 0:
					c_function_call_code += ', '
			c_function_call_code += ', '.join(arg_names) + ");"
		if self.method_name == 'add_callbacks':
			python_ref_code = "Py_INCREF(_cbs);"
		elif self.method_name == 'remove_callbacks':
			python_ref_code = "Py_XDECREF(_cbs);"
		from_native_pointer_code = ''
		convert_from_code = ''
		build_value_code = ''
		cfree_code = ''
		result_variable = ''
		take_native_ref = 'TRUE'
		if self.return_complete_type != 'void':
			if self.build_value_format == 'O':
				stripped_return_type = strip_leading_linphone(self.return_type)
				return_type_class = self.find_class_definition(self.return_type)
				if return_type_class is not None:
					if self.method_name.startswith('create'):
						take_native_ref = 'FALSE'
					from_native_pointer_code = "pyresult = pylinphone_{return_type}_from_native_ptr(&pylinphone_{return_type}Type, cresult, {take_native_ref});\n".format(return_type=stripped_return_type, take_native_ref=take_native_ref)
				else:
					return_argument_type = ArgumentType(self.return_type, self.return_complete_type, self.return_contained_type, self.linphone_module)
					if return_argument_type.convert_from_func is not None:
						convert_from_code = \
"""pyresult = {convert_func}(cresult);
""".format(convert_func=return_argument_type.convert_from_func)
					if return_argument_type.free_convert_result_func is not None:
						cfree_code = \
"""{free_func}(cresult);
""".format(free_func=return_argument_type.free_convert_result_func)
				result_variable = 'pyresult'
			else:
				result_variable = 'cresult'
		if result_variable != '':
			build_value_code = "pyret = Py_BuildValue(\"{fmt}\", {result_variable});".format(fmt=self.build_value_format, result_variable=result_variable)
			if take_native_ref == 'FALSE':
				build_value_code += """
	Py_XDECREF(pyresult);"""
		if self.return_complete_type == 'char *':
			cfree_code = 'ms_free(cresult);';
		body = \
"""	{c_function_call_code}
	{cfree_argument_code}
	{python_ref_code}
	pylinphone_dispatch_messages();
	{from_native_pointer_code}
	{convert_from_code}
	{build_value_code}
	{cfree_code}
""".format(c_function_call_code=c_function_call_code,
		cfree_argument_code=cfree_argument_code,
		python_ref_code=python_ref_code,
		from_native_pointer_code=from_native_pointer_code,
		convert_from_code=convert_from_code,
		build_value_code=build_value_code,
		cfree_code=cfree_code)
		return body

	def format_return_trace(self):
		if self.return_complete_type != 'void':
			return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> %p\", __FUNCTION__, pyret);\n"
		else:
			return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> None\", __FUNCTION__);\n"

	def format_return_result(self):
		if self.return_complete_type != 'void':
			return "\treturn pyret;"
		return "\tPy_RETURN_NONE;"

	def format_return_none_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> None\", __FUNCTION__);\n"

	def format_class_native_pointer_check(self, return_int):
		return_value = "NULL"
		if return_int:
			return_value = "-1"
		return \
"""native_ptr = pylinphone_{class_name}_get_native_ptr(self);
	if (native_ptr == NULL) {{
		PyErr_SetString(PyExc_TypeError, "Invalid linphone.{class_name} instance");
		return {return_value};
	}}
""".format(class_name=self.class_['class_name'], return_value=return_value)

	def format_args_type_check(self):
		body = ''
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O':
				if argument_type.use_native_pointer:
					body += \
"""	if (({arg_name} != Py_None) && !PyObject_IsInstance({arg_name}, (PyObject *)&pylinphone_{arg_type}Type)) {{
		PyErr_SetString(PyExc_TypeError, "The '{arg_name}' argument must be a {type_str} instance.");
		return NULL;
	}}
""".format(arg_name=arg_name, arg_type=strip_leading_linphone(arg_type), type_str=argument_type.type_str)
				else:
					body += \
"""	if ({check_condition}) {{
		PyErr_SetString(PyExc_TypeError, "The '{arg_name}' argument must be a {type_str} instance.");
		return NULL;
	}}
""".format(arg_name=arg_name, check_condition=argument_type.check_condition.format(arg_name=arg_name), type_str=argument_type.type_str)
		if body != '':
			body = body[1:] # Remove leading '\t'
		return body

	def format_args_native_pointer_check(self):
		body = ''
		for xml_method_arg in self.xml_method_args:
			arg_name = "_" + xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O' and argument_type.use_native_pointer:
				body += \
"""	if (({arg_name} != NULL) && ({arg_name} != Py_None)) {{
		if (({arg_name}_native_ptr = pylinphone_{arg_type}_get_native_ptr({arg_name})) == NULL) {{
			return NULL;
		}}
	}}
""".format(arg_name=arg_name, arg_type=strip_leading_linphone(arg_type))
		if body != '':
			body = body[1:] # Remove leading '\t'
		return body

	def format_local_return_variables_definition(self):
		body = ''
		if self.xml_method_return is not None:
			self.return_type = self.xml_method_return.get('type')
			self.return_complete_type = self.xml_method_return.get('completetype')
			self.return_contained_type = self.xml_method_return.get('containedtype')
		if is_callback(self.return_complete_type):
			body += "\tPyObject * pyresult;\n"
			body += "\tPyObject * pyret;\n"
			argument_type = ArgumentType(self.return_type, self.return_complete_type, self.return_contained_type, self.linphone_module)
			self.build_value_format = argument_type.fmt_str
		elif self.return_complete_type != 'void':
			body += "\t" + self.return_complete_type + " cresult;\n"
			argument_type = ArgumentType(self.return_type, self.return_complete_type, self.return_contained_type, self.linphone_module)
			self.build_value_format = argument_type.fmt_str
			if self.build_value_format == 'O':
				body += "\tPyObject * pyresult;\n"
			body += "\tPyObject * pyret;\n"
		return body

	def parse_method_node(self):
		if self.method_node is not None:
			self.xml_method_return = self.method_node.find('./return')
			self.xml_method_args = self.method_node.findall('./arguments/argument')
			self.method_type = self.method_node.tag
		if self.method_type != 'classmethod' and len(self.xml_method_args) > 0:
			self.self_arg = self.xml_method_args[0]
			self.xml_method_args = self.xml_method_args[1:]

	def find_class_definition(self, basic_type):
		basic_type = strip_leading_linphone(basic_type)
		for c in self.linphone_module.classes:
			if c['class_name'] == basic_type:
				return c
		return None

	def find_property_definition(self, basic_type, property_name):
		class_definition = self.find_class_definition(basic_type)
		if class_definition is None:
			return None
		for p in class_definition['class_properties']:
			if p['property_name'] == property_name:
				return p
		return None

	def format(self):
		self.parse_method_node()
		body = self.format_local_variables_definition()
		body += self.format_deprecation_warning()
		body += self.format_arguments_parsing()
		body += self.format_enter_trace()
		body += self.format_c_function_call()
		body += self.format_return_trace()
		body += self.format_return_result()
		return body

class NewMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, "new", method_node)

	def format_local_variables_definition(self):
		return "\tpylinphone_{class_name}Object *self = (pylinphone_{class_name}Object *)type->tp_alloc(type, 0);\n".format(class_name=self.class_['class_name'])

	def format_arguments_parsing(self):
		return ''

	def format_enter_trace(self):
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s()\", __FUNCTION__);\n"

	def format_c_function_call(self):
		return ''

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> %p\", __FUNCTION__, self);\n"

	def format_return_result(self):
		return "\treturn (PyObject *)self;"

class InitMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, "init", method_node)

	def format_local_variables_definition(self):
		return "\tpylinphone_{class_name}Object *self_obj = (pylinphone_{class_name}Object *)self;\n".format(class_name=self.class_['class_name'])

	def format_arguments_parsing(self):
		return ''

	def format_enter_trace(self):
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s()\", __FUNCTION__);\n"

	def format_c_function_call(self):
		specific_member_initialization_code = ''
		for member in self.class_['class_object_members']:
			specific_member_initialization_code += "\tself_obj->{member} = NULL;\n".format(member=member)
		return \
"""	self_obj->native_ptr = NULL;
	self_obj->user_data = NULL;
{specific_member_initialization_code}
""".format(specific_member_initialization_code=specific_member_initialization_code)

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> %p\", __FUNCTION__, self);\n"

	def format_return_result(self):
		return "\treturn 0;"

class FromNativePointerMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_):
		MethodDefinition.__init__(self, linphone_module, class_, "from_native_pointer", None)

	def format_local_variables_definition(self):
		return "\tpylinphone_{class_name}Object *self = NULL;\n".format(class_name=self.class_['class_name'])

	def format_arguments_parsing(self):
		return ''

	def format_enter_trace(self):
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s(%p)\", __FUNCTION__, native_ptr);\n"

	def format_c_function_call(self):
		get_user_data_func_call = ''
		set_user_data_func_call = ''
		if self.class_['class_has_user_data']:
			get_user_data_func_call = "self = (pylinphone_{class_name}Object *){function_prefix}get_user_data(native_ptr);".format(class_name=self.class_['class_name'], function_prefix=self.class_['class_c_function_prefix'])
			set_user_data_func_call = "{function_prefix}set_user_data(self->native_ptr, self);".format(function_prefix=self.class_['class_c_function_prefix'])
		ref_native_pointer_code = ''
		if self.class_['class_refcountable']:
			ref_native_pointer_code = "if (take_native_ref == TRUE) {func}(self->native_ptr);".format(func=self.class_['class_c_function_prefix'] + "ref")
		return \
"""	if (native_ptr == NULL) {{
	{none_trace}
		Py_RETURN_NONE;
	}}
	{get_user_data_func_call}
	if (self == NULL) {{
		self = (pylinphone_{class_name}Object *)PyObject_CallObject((PyObject *)&pylinphone_{class_name}Type, NULL);
		if (self == NULL) {{
		{none_trace}
			Py_RETURN_NONE;
		}}
		self->native_ptr = ({class_cname} *)native_ptr;
		{set_user_data_func_call}
		{ref_native_pointer_code}
	}}
""".format(class_name=self.class_['class_name'], class_cname=self.class_['class_cname'],
		none_trace=self.format_return_none_trace(),
		get_user_data_func_call=get_user_data_func_call, set_user_data_func_call=set_user_data_func_call,
		ref_native_pointer_code=ref_native_pointer_code)

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> %p\", __FUNCTION__, self);\n"

	def format_return_result(self):
		return "\treturn (PyObject *)self;"

class DeallocMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, "dealloc", method_node)

	def format_local_variables_definition(self):
		func = "pylinphone_{class_name}_get_native_ptr".format(class_name=self.class_['class_name'])
		return \
"""	{arg_type} * native_ptr = {func}(self);
""".format(arg_type=self.class_['class_cname'], func=func)

	def format_arguments_parsing(self):
		# Check that the dealloc is not called a second time because of reentrancy
		return "\tif (Py_REFCNT(self) < 0) return;\n"

	def format_enter_trace(self):
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s(%p [%p])\", __FUNCTION__, self, native_ptr);\n"

	def format_c_function_call(self):
		reset_user_data_code = ''
		if self.class_['class_name'] != 'Core' and self.class_['class_has_user_data']:
			reset_user_data_code += \
"""if (native_ptr != NULL) {{
		{function_prefix}set_user_data(native_ptr, NULL);
	}}
""".format(function_prefix=self.class_['class_c_function_prefix'])
		native_ptr_dealloc_code = ''
		specific_member_decref_code = ''
		if self.class_['class_refcountable']:
			native_ptr_dealloc_code += \
"""	if (native_ptr != NULL) {{
		{function_prefix}unref(native_ptr);
	}}
""".format(function_prefix=self.class_['class_c_function_prefix'])
		elif self.class_['class_destroyable']:
			native_ptr_dealloc_code += \
"""	if (native_ptr != NULL) {{
		{function_prefix}destroy(native_ptr);
	}}
""".format(function_prefix=self.class_['class_c_function_prefix'])
		for member in self.class_['class_object_members']:
			specific_member_decref_code += "\tPy_XDECREF(((pylinphone_{class_name}Object *)self)->{member});\n".format(class_name=self.class_['class_name'], member=member)
		return \
"""	{reset_user_data_code}
	{native_ptr_dealloc_code}
	pylinphone_dispatch_messages();
	Py_XDECREF(((pylinphone_{class_name}Object *)self)->user_data);
{specific_member_decref_code}
	self->ob_type->tp_free(self);
""".format(class_name=self.class_['class_name'], reset_user_data_code=reset_user_data_code, native_ptr_dealloc_code=native_ptr_dealloc_code, specific_member_decref_code=specific_member_decref_code)

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s\", __FUNCTION__);"

	def format_return_result(self):
		return ''

	def format(self):
		return \
"""static void pylinphone_{class_name}_dealloc(PyObject *self) {{
{method_body}
}}""".format(class_name=self.class_['class_name'], method_body=MethodDefinition.format(self))

class GetterMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_name = "", method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, method_name, method_node)

class SetterMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_name = "", method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, method_name, method_node)

	def format_arguments_parsing(self):
		if self.first_argument_type.check_condition is None:
			attribute_type_check_code = \
"""if ((value != Py_None) && !PyObject_IsInstance(value, (PyObject *)&pylinphone_{class_name}Type)) {{
		PyErr_SetString(PyExc_TypeError, "The '{attribute_name}' attribute value must be a linphone.{class_name} instance.");
		return -1;
	}}
""".format(class_name=self.first_arg_class, attribute_name=self.attribute_name)
		else:
			checknotnone = ''
			if self.first_argument_type.type_str == 'string':
				checknotnone = "(value != Py_None) && "
			attribute_type_check_code = \
"""if ({checknotnone}{check_condition}) {{
		PyErr_SetString(PyExc_TypeError, "The '{attribute_name}' attribute value must be a {type_str}.");
		return -1;
	}}
""".format(checknotnone=checknotnone, check_condition=self.first_argument_type.check_condition.format(arg_name='value'), attribute_name=self.attribute_name, type_str=self.first_argument_type.type_str)
		attribute_conversion_code = ''
		callback_setting_code = ''
		if is_callback(self.first_argument_type.complete_type):
			callback_setting_code = \
"""Py_XDECREF(((pylinphone_{class_name}Object *)self)->{callback_name});
	Py_INCREF(value);
	((pylinphone_{class_name}Object *)self)->{callback_name} = value;
""".format(class_name=self.class_['class_name'], callback_name=compute_event_name(self.first_arg_complete_type, self.class_['class_name']))
		if (self.first_argument_type.convert_code is None) or \
			(self.first_argument_type.fmt_str == 'O' and self.first_argument_type.convert_code is not None):
			attribute_conversion_code += "{arg_name} = value;\n".format(arg_name="_" + self.first_arg_name)
		if self.first_argument_type.convert_code is not None:
			cast_code = ''
			suffix = ''
			if self.first_argument_type.cast_convert_func_result:
				cast_code = "({arg_type})".format(arg_type=self.first_arg_complete_type)
			if self.first_argument_type.fmt_str == 'O' and self.first_argument_type.convert_code is not None:
				suffix = '_native_obj'
			attribute_conversion_code += self.first_argument_type.convert_code.format(result_name="_" + self.first_arg_name, result_suffix=suffix, cast=cast_code, arg_name='value')
		attribute_native_ptr_check_code = ''
		if self.first_argument_type.use_native_pointer:
			attribute_native_ptr_check_code = \
"""if ({arg_name} != Py_None) {{
		if (({arg_name}_native_ptr = pylinphone_{arg_class}_get_native_ptr({arg_name})) == NULL) {{
			PyErr_SetString(PyExc_TypeError, "Invalid linphone.{arg_class} instance.");
			return -1;
		}}
	}}
""".format(arg_name="_" + self.first_arg_name, arg_class=self.first_arg_class)
		return \
"""	{native_ptr_check_code}
	if (value == NULL) {{
		PyErr_SetString(PyExc_TypeError, "Cannot delete the '{attribute_name}' attribute.");
		return -1;
	}}
	{attribute_type_check_code}
	{attribute_conversion_code}
	{callback_setting_code}
	{attribute_native_ptr_check_code}
""".format(attribute_name=self.attribute_name,
		native_ptr_check_code=self.format_class_native_pointer_check(True),
		attribute_type_check_code=attribute_type_check_code,
		attribute_conversion_code=attribute_conversion_code,
		callback_setting_code=callback_setting_code,
		attribute_native_ptr_check_code=attribute_native_ptr_check_code)

	def format_c_function_call(self):
		if is_callback(self.first_argument_type.complete_type):
			return \
"""	{method_name}(native_ptr, pylinphone_{class_name}_callback_{callback_name});
	pylinphone_dispatch_messages();
""".format(method_name=self.method_node.get('name'), class_name=self.class_['class_name'], callback_name=compute_event_name(self.first_argument_type.complete_type, self.class_['class_name']))
		cfree_argument_code = ''
		suffix = ''
		if self.first_argument_type.fmt_str == 'O' and self.first_argument_type.use_native_pointer:
			suffix = '_native_ptr'
		elif self.first_argument_type.fmt_str == 'O' and self.first_argument_type.convert_code is not None:
			suffix = '_native_obj'
			if self.first_argument_type.free_convert_result_func is not None and not is_const_from_complete_type(self.first_argument_type.complete_type):
					cfree_argument_code = \
"""{free_func}({arg_name}_native_obj);
""".format(free_func=self.first_argument_type.free_convert_result_func, arg_name="_" + self.first_arg_name)
		return \
"""	{method_name}(native_ptr, {arg_name}{suffix});
	{cfree_argument_code}
	pylinphone_dispatch_messages();
""".format(arg_name="_" + self.first_arg_name, method_name=self.method_node.get('name'), suffix=suffix, cfree_argument_code=cfree_argument_code)

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s -> 0\", __FUNCTION__);\n"

	def format_return_result(self):
		return "\treturn 0;"

	def parse_method_node(self):
		MethodDefinition.parse_method_node(self)
		# Force return value type of setter function to prevent declaring useless local variables
		# TODO: Investigate. Maybe we should decide that setters must always return an int value.
		self.xml_method_return = None
		self.attribute_name = self.method_node.get('property_name')
		self.first_arg_type = self.xml_method_args[0].get('type')
		self.first_arg_complete_type = self.xml_method_args[0].get('completetype')
		self.first_arg_contained_type = self.xml_method_args[0].get('containedtype')
		self.first_arg_name = self.xml_method_args[0].get('name')
		self.first_argument_type = ArgumentType(self.first_arg_type, self.first_arg_complete_type, self.first_arg_contained_type, self.linphone_module)
		self.first_arg_class = strip_leading_linphone(self.first_arg_type)

class EventCallbackMethodDefinition(MethodDefinition):
	def __init__(self, linphone_module, class_, method_name = "", method_node = None):
		MethodDefinition.__init__(self, linphone_module, class_, method_name, method_node)

	def format_local_variables_definition(self):
		class_name = self.class_['event_class']
		nocallbacks_class_name = class_name
		if class_name.endswith('Cbs'):
			nocallbacks_class_name = class_name[:-3]
		has_current_callbacks = self.find_property_definition(nocallbacks_class_name, 'current_callbacks')
		if has_current_callbacks is not None:
			get_callbacks_funcname = 'get_current_callbacks'
		else:
			get_callbacks_funcname = 'get_callbacks'
		returnvars = self.format_local_return_variables_definition()
		common = \
"""	pylinphone_{class_name}Object *pyself = (pylinphone_{class_name}Object *){function_prefix}get_user_data(self);
	PyObject *func;
	PyObject *args;
	PyGILState_STATE pygil_state;""".format(class_name=nocallbacks_class_name, function_prefix=self.find_class_definition(nocallbacks_class_name)['class_c_function_prefix'])
		if class_name.endswith('Cbs'):
			common += """
	pylinphone_{class_name}Object *pycbs = (pylinphone_{class_name}Object *){cbs_function_prefix}get_user_data({function_prefix}{get_callbacks_funcname}(self));
""".format(class_name=class_name, cbs_function_prefix=self.find_class_definition(class_name)['class_c_function_prefix'], function_prefix=self.find_class_definition(nocallbacks_class_name)['class_c_function_prefix'], get_callbacks_funcname=get_callbacks_funcname)
		specific = ''
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O':
				specific += "\tPyObject * py" + arg_name + " = NULL;\n"
		return "{returnvars}\n{common}\n{specific}".format(returnvars=returnvars, common=common, specific=specific)

	def format_arguments_parsing(self):
		return_str = ''
		if self.return_complete_type == 'int':
			return_str = '-1'
		elif self.return_complete_type == 'bool_t':
			return_str = 'FALSE'
		elif self.return_complete_type != 'void':
			argument_type = ArgumentType(self.return_type, self.return_complete_type, self.return_contained_type, self.linphone_module)
			if argument_type.fmt_str == 'O':
				return_str = 'NULL'
		return \
"""	if (Py_REFCNT(pyself) <= 0) return {return_str};
	func = pycbs->{event_name};
	pygil_state = PyGILState_Ensure();
""".format(event_name=self.class_['event_name'], return_str=return_str)

	def format_enter_trace(self):
		fmt = '%p'
		args = ['self']
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			if fmt != '':
				fmt += ', '
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			fmt += argument_type.cfmt_str
			args.append(arg_name)
		args=', '.join(args)
		if args != '':
			args = ', ' + args
		return "\tpylinphone_trace(1, \"[PYLINPHONE] >>> %s({fmt})\", __FUNCTION__{args});\n".format(fmt=fmt, args=args)

	def format_c_function_call(self):
		create_python_objects_code = ''
		convert_python_result_code = ''
		fmt = 'O'
		args = ['pyself']
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arg_contained_type = xml_method_arg.get('containedtype')
			argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self.linphone_module)
			fmt += argument_type.fmt_str
			if argument_type.fmt_str == 'O':
				args.append('py' + arg_name)
			else:
				args.append(arg_name)
			if argument_type.fmt_str == 'O':
				if argument_type.type_str == "bool":
					create_python_objects_code += "\t\tpy{name} = {convert_from_func}({name});\n".format(name=arg_name, convert_from_func=argument_type.convert_from_func)
				else:
					type_class = self.find_class_definition(arg_type)
					create_python_objects_code += "\t\tpy{name} = pylinphone_{arg_type}_from_native_ptr(&pylinphone_{arg_type}Type, {name}, TRUE);\n".format(name=arg_name, arg_type=strip_leading_linphone(arg_type))
		args=', '.join(args)
		if self.return_complete_type != 'void':
			argument_type = ArgumentType(self.return_type, self.return_complete_type, self.return_contained_type, self.linphone_module)
			if argument_type.is_linphone_object:
				convert_python_result_code = \
"""		if ((pyresult != Py_None) && !PyObject_IsInstance(pyresult, (PyObject *)&pylinphone_{class_name}Type)) {{
			PyErr_SetString(PyExc_TypeError, "The return value must be a linphone.{class_name} instance.");
			return NULL;
		}}
		if ((cresult = pylinphone_{class_name}_get_native_ptr(pyresult)) == NULL) {{
			return NULL;
		}}
""".format(class_name=strip_leading_linphone(self.return_type))

			else:
				convert_python_result_code = '\t\t' + argument_type.convert_code.format(result_name='cresult', result_suffix='', cast='', arg_name='pyresult')
		return \
"""	if ((func != NULL) && PyCallable_Check(func)) {{
{create_python_objects_code}
		args = Py_BuildValue("{fmt}", {args});
		pyresult = PyEval_CallObject(func, args);
		if (pyresult == NULL) {{
			PyErr_Print();
		}}
		Py_DECREF(args);
{convert_python_result_code}
	}}
""".format(fmt=fmt, args=args, create_python_objects_code=create_python_objects_code, convert_python_result_code=convert_python_result_code)

	def format_return_trace(self):
		return "\tpylinphone_trace(-1, \"[PYLINPHONE] <<< %s\", __FUNCTION__);\n"

	def format_return_result(self):
		s = '\tPyGILState_Release(pygil_state);'
		if self.return_complete_type != 'void':
			s += '\n\treturn cresult;'
		return s

	def format_local_return_variables_definition(self):
		body = "\tPyObject * pyresult;"
		if self.xml_method_return is not None:
			self.return_type = self.xml_method_return.get('type')
			self.return_complete_type = self.xml_method_return.get('completetype')
			self.return_contained_type = self.xml_method_return.get('containedtype')
		if self.return_complete_type != 'void':
			body += "\n\t" + self.return_complete_type + " cresult;"
		return body

	def format(self):
		body = MethodDefinition.format(self)
		class_name = self.class_['event_class']
		nocallbacks_class_name = class_name
		if class_name.endswith('Cbs'):
			nocallbacks_class_name = class_name[:-3]
		arguments = ['Linphone' + nocallbacks_class_name + ' * self']
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			arguments.append(arg_complete_type + ' ' + arg_name)
		definition = \
"""static {returntype} pylinphone_{class_name}_callback_{event_name}({arguments}) {{
{body}
}}
""".format(returntype=self.return_complete_type, class_name=class_name, event_name=self.class_['event_name'], arguments=', '.join(arguments), body=body)
		return definition


class LinphoneModule(object):
	def __init__(self, tree, blacklisted_classes, blacklisted_events, blacklisted_functions, hand_written_codes):
		self.internal_instance_method_names = ['destroy', 'ref', 'unref']
		self.internal_property_names = ['user_data']
		self.bctbxlist_types = Set([])
		self.enums = []
		self.enum_names = []
		self.cfunction2methodmap = {}
		hand_written_functions = []
		for hand_written_code in hand_written_codes:
			hand_written_functions += hand_written_code.func_list
		xml_enums = tree.findall("./enums/enum")
		for xml_enum in xml_enums:
			e = {}
			e['enum_name'] = strip_leading_linphone(xml_enum.get('name'))
			e['enum_doc'] = self.__format_doc_content(xml_enum.find('briefdescription'), xml_enum.find('detaileddescription'))
			e['enum_doc'] = self.__replace_doc_special_chars(e['enum_doc'])
			e['enum_doc'] += """

.. csv-table::
   :delim: |
   :widths: 30, 70
   :header: Value,Description

"""
			e['enum_values'] = []
			e['enum_deprecated_values'] = []
			xml_enum_values = xml_enum.findall("./values/value")
			for xml_enum_value in xml_enum_values:
				v = {}
				v['enum_value_cname'] = xml_enum_value.get('name')
				valname = strip_leading_linphone(v['enum_value_cname'])
				v['enum_value_name'] = remove_useless_enum_prefix(e['enum_name'], valname)
				v['enum_value_doc'] = self.__format_doc(xml_enum_value.find('briefdescription'), xml_enum_value.find('detaileddescription'))
				e['enum_doc'] += '   ' + v['enum_value_name'] + '|' + v['enum_value_doc'] + '\n'
				e['enum_values'].append(v)
				if v['enum_value_name'] != valname:
					# TODO: To remove. Add deprecated value name.
					v = {}
					v['enum_value_cname'] = xml_enum_value.get('name')
					v['enum_value_name'] = strip_leading_linphone(v['enum_value_cname'])
					v['enum_value_doc'] = self.__format_doc(xml_enum_value.find('briefdescription'), xml_enum_value.find('detaileddescription'))
					e['enum_deprecated_values'].append(v)
			e['enum_doc'] = self.__replace_doc_special_chars(e['enum_doc'])
			self.enums.append(e)
			self.enum_names.append(e['enum_name'])
		self.core_events = []
		self.classes = []
		xml_classes = tree.findall("./classes/class")
		for xml_class in xml_classes:
			if xml_class.get('name') in blacklisted_classes:
				continue
			c = {}
			c['class_xml_node'] = xml_class
			c['class_cname'] = xml_class.get('name')
			c['class_name'] = strip_leading_linphone(c['class_cname'])
			c['class_c_function_prefix'] = xml_class.get('cfunctionprefix')
			c['class_doc'] = self.__format_doc(xml_class.find('briefdescription'), xml_class.find('detaileddescription'))
			c['class_refcountable'] = (xml_class.get('refcountable') == 'true')
			c['class_destroyable'] = (xml_class.get('destroyable') == 'true')
			c['class_has_user_data'] = False
			c['class_type_methods'] = []
			c['class_type_hand_written_methods'] = []
			c['class_instance_hand_written_methods'] = []
			c['class_hand_written_properties'] = []
			c['class_object_members'] = []
			c['class_object_members_code'] = ''
			c['class_events'] = []
			xml_events = xml_class.findall("./events/event")
			for xml_event in xml_events:
				if xml_event.get('name') in blacklisted_events:
						continue
				ev = {}
				ev['event_class'] = c['class_name']
				ev['event_xml_node'] = xml_event
				ev['event_cname'] = xml_event.get('name')
				ev['event_name'] = compute_event_name(ev['event_cname'], c['class_name'])
				ev['event_doc'] = self.__format_doc(xml_event.find('briefdescription'), xml_event.find('detaileddescription'))
				c['class_events'].append(ev)
				c['class_object_members'].append(ev['event_name'])
				c['class_object_members_code'] += "\tPyObject *" + ev['event_name'] + ";\n"
			for hand_written_code in hand_written_codes:
				if hand_written_code._class == c['class_name']:
					if isinstance(hand_written_code, HandWrittenClassMethod):
						m = {}
						m['method_name'] = hand_written_code.name
						m['method_doc'] = self.__replace_doc_special_chars(hand_written_code.doc)
						c['class_type_hand_written_methods'].append(m)
					elif isinstance(hand_written_code, HandWrittenInstanceMethod):
						m = {}
						m['method_name'] = hand_written_code.name
						m['method_doc'] = self.__replace_doc_special_chars(hand_written_code.doc)
						c['class_instance_hand_written_methods'].append(m)
					elif isinstance(hand_written_code, HandWrittenDeallocMethod):
						c['class_has_hand_written_dealloc'] = True
					elif isinstance(hand_written_code, HandWrittenProperty):
						p = {}
						p['property_name'] = hand_written_code.name
						if hand_written_code.getter_cfunction is None:
							p['getter_reference'] = 'NULL'
						else:
							p['getter_reference'] = '(getter)pylinphone_' + c['class_name'] + '_get_' + p['property_name']
						if hand_written_code.setter_cfunction is None:
							p['setter_reference'] = 'NULL'
						else:
							p['setter_reference'] = '(setter)pylinphone_' + c['class_name'] + '_set_' + p['property_name']
						p['property_doc'] = self.__replace_doc_special_chars(hand_written_code.doc)
						c['class_hand_written_properties'].append(p)
			xml_type_methods = xml_class.findall("./classmethods/classmethod")
			for xml_type_method in xml_type_methods:
				method_name = xml_type_method.get('name')
				if method_name in blacklisted_functions:
					continue
				m = {}
				m['method_name'] = method_name.replace(c['class_c_function_prefix'], '')
				if method_name not in hand_written_functions:
					m['method_xml_node'] = xml_type_method
					self.cfunction2methodmap[method_name] = ':py:meth:`linphone.' + c['class_name'] + '.' + m['method_name'] + '`'
					c['class_type_methods'].append(m)
			c['class_instance_methods'] = []
			xml_instance_methods = xml_class.findall("./instancemethods/instancemethod")
			for xml_instance_method in xml_instance_methods:
				method_name = xml_instance_method.get('name')
				if method_name in blacklisted_functions:
					continue
				if method_name.replace(c['class_c_function_prefix'], '') in self.internal_instance_method_names:
					continue
				m = {}
				m['method_name'] = method_name.replace(c['class_c_function_prefix'], '')
				if method_name not in hand_written_functions:
					m['method_xml_node'] = xml_instance_method
					self.cfunction2methodmap[method_name] = ':py:meth:`linphone.' + c['class_name'] + '.' + m['method_name'] + '`'
					c['class_instance_methods'].append(m)
			c['class_properties'] = []
			xml_properties = xml_class.findall("./properties/property")
			for xml_property in xml_properties:
				property_name = xml_property.get('name')
				if property_name == 'user_data':
					c['class_has_user_data'] = True
				if property_name in self.internal_property_names:
					continue
				p = {}
				p['property_name'] = property_name
				xml_property_getter = xml_property.find("./getter")
				xml_property_setter = xml_property.find("./setter")
				if xml_property_getter is not None:
					if xml_property_getter.get('name') in blacklisted_functions or xml_property_getter.get('name') in hand_written_functions:
						continue
				if xml_property_setter is not None:
					if xml_property_setter.get('name') in blacklisted_functions or xml_property_setter.get('name') in hand_written_functions:
						continue
				if xml_property_getter is not None:
					xml_property_getter.set('property_name', property_name)
					p['getter_name'] = xml_property_getter.get('name').replace(c['class_c_function_prefix'], '')
					p['getter_xml_node'] = xml_property_getter
					p['getter_reference'] = "(getter)pylinphone_" + c['class_name'] + "_" + p['getter_name']
					p['getter_definition_begin'] = "static PyObject * pylinphone_" + c['class_name'] + "_" + p['getter_name'] + "(PyObject *self, void *closure) {"
					p['getter_definition_end'] = "}"
					self.cfunction2methodmap[xml_property_getter.get('name')] = ':py:attr:`linphone.' + c['class_name'] + '.' + property_name + '`'
				else:
					p['getter_reference'] = "NULL"
				if xml_property_setter is not None:
					xml_property_setter.set('property_name', property_name)
					p['setter_name'] = xml_property_setter.get('name').replace(c['class_c_function_prefix'], '')
					p['setter_xml_node'] = xml_property_setter
					p['setter_reference'] = "(setter)pylinphone_" + c['class_name'] + "_" + p['setter_name']
					p['setter_definition_begin'] = "static int pylinphone_" + c['class_name'] + "_" + p['setter_name'] + "(PyObject *self, PyObject *value, void *closure) {"
					p['setter_definition_end'] = "}"
					self.cfunction2methodmap[xml_property_setter.get('name')] = ':py:attr:`linphone.' + c['class_name'] + '.' + property_name + '`'
				else:
					p['setter_reference'] = "NULL"
				c['class_properties'].append(p)
			self.classes.append(c)
		# Format events definitions
		for c in self.classes:
			for ev in c['class_events']:
				ev['event_callback_definition'] = EventCallbackMethodDefinition(self, ev, ev['event_name'], ev['event_xml_node']).format()
		# Format methods' bodies
		for c in self.classes:
			xml_new_method = c['class_xml_node'].find("./classmethods/classmethod[@name='" + c['class_c_function_prefix'] + "new']")
			try:
				c['new_body'] = NewMethodDefinition(self, c, xml_new_method).format()
			except Exception, e:
				e.args += (c['class_name'], 'new_body')
				raise
			try:
				c['init_body'] = InitMethodDefinition(self, c, xml_new_method).format()
			except Exception, e:
				e.args += (c['class_name'], 'init_body')
				raise
			try:
				c['from_native_pointer_body'] = FromNativePointerMethodDefinition(self, c).format()
			except Exception, e:
				e.args += (c['class_name'], 'from_native_pointer_body')
				raise
			try:
				for m in c['class_type_methods']:
					m['method_body'] = MethodDefinition(self, c, m['method_name'], m['method_xml_node']).format()
					m['method_doc'] = self.__format_method_doc(m['method_xml_node'])
				for m in c['class_instance_methods']:
					m['method_body'] = MethodDefinition(self, c, m['method_name'], m['method_xml_node']).format()
					m['method_doc'] = self.__format_method_doc(m['method_xml_node'])
			except Exception, e:
				e.args += (c['class_name'], m['method_name'])
				raise
			try:
				for p in c['class_properties']:
					p['property_doc'] = ''
					if p.has_key('setter_xml_node'):
						p['setter_body'] = SetterMethodDefinition(self, c, p['property_name'], p['setter_xml_node']).format()
						p['property_doc'] = self.__format_setter_doc(p['setter_xml_node'])
					if p.has_key('getter_xml_node'):
						p['getter_body'] = GetterMethodDefinition(self, c, p['property_name'], p['getter_xml_node']).format()
						if p['property_doc'] == '':
							p['property_doc'] = self.__format_getter_doc(p['getter_xml_node'])
			except Exception, e:
				e.args += (c['class_name'], p['property_name'])
				raise
			if not 'class_has_hand_written_dealloc' in c:
				try:
					if c['class_refcountable']:
						xml_instance_method = c['class_xml_node'].find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "unref']")
						c['dealloc_definition'] = DeallocMethodDefinition(self, c, xml_instance_method).format()
					elif c['class_destroyable']:
						xml_instance_method = c['class_xml_node'].find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "destroy']")
						c['dealloc_definition'] = DeallocMethodDefinition(self, c, xml_instance_method).format()
					else:
						c['dealloc_definition'] = DeallocMethodDefinition(self, c).format()
				except Exception, e:
					e.args += (c['class_name'], 'dealloc_body')
					raise
		# Convert bctbxlist_types to a list of dictionaries for the template
		d = []
		for bctbxlist_type in self.bctbxlist_types:
			t = {}
			t['c_contained_type'] = bctbxlist_type
			t['python_contained_type'] = strip_leading_linphone(bctbxlist_type)
			d.append(t)
		self.bctbxlist_types = d

	def __format_doc_node(self, node):
		desc = ''
		if node.tag == 'para':
			if node.text is not None:
				desc += node.text.strip()
			for n in list(node):
				desc += self.__format_doc_node(n)
		elif node.tag == 'note':
			if node.text is not None:
				desc += node.text.strip()
			for n in list(node):
				desc += self.__format_doc_node(n)
		elif node.tag == 'ref':
			if node.text is not None:
				desc += ' ' + node.text.strip() + ' '
		tail = node.tail.strip()
		if tail != '':
			if node.tag != 'ref':
				desc += '\n'
			desc += tail
		if node.tag == 'para':
			desc += '\n'
		return desc

	def __format_doc_content(self, brief_description, detailed_description):
		doc = ''
		if brief_description is None:
			brief_description = ''
		if detailed_description is None:
			detailed_description = ''
		else:
			desc = ''
			for node in list(detailed_description):
				desc += self.__format_doc_node(node) + '\n'
			detailed_description = desc.strip()
		brief_description = brief_description.strip()
		doc += brief_description
		if detailed_description != '':
			if doc != '':
				doc += '\n\n'
			doc += detailed_description
		return doc

	def __replace_doc_special_chars(self, doc):
		return doc.replace('"', '').encode('string-escape')

	def __replace_doc_cfunction_by_method(self, doc):
		for cfunction, method in self.cfunction2methodmap.iteritems():
			doc = doc.replace(cfunction + '()', method)
		for cfunction, method in self.cfunction2methodmap.iteritems():
			doc = doc.replace(cfunction, method)
		return doc

	def __replace_doc_keywords(self, doc):
		return doc.replace('NULL', 'None')

	def __format_doc(self, brief_description, detailed_description):
		doc = self.__format_doc_content(brief_description, detailed_description)
		doc = self.__replace_doc_cfunction_by_method(doc)
		doc = self.__replace_doc_keywords(doc)
		doc = self.__replace_doc_special_chars(doc)
		return doc

	def __format_method_doc(self, xml_node):
		doc = self.__format_doc_content(xml_node.find('briefdescription'), xml_node.find('detaileddescription'))
		xml_method_return = xml_node.find('./return')
		xml_method_args = xml_node.findall('./arguments/argument')
		method_type = xml_node.tag
		if method_type != 'classmethod' and len(xml_method_args) > 0:
			xml_method_args = xml_method_args[1:]
		doc += '\n'
		if len(xml_method_args) > 0:
			for xml_method_arg in xml_method_args:
				arg_name = xml_method_arg.get('name')
				arg_type = xml_method_arg.get('type')
				arg_complete_type = xml_method_arg.get('completetype')
				arg_contained_type = xml_method_arg.get('containedtype')
				argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self)
				arg_doc = self.__format_doc_content(None, xml_method_arg.find('description'))
				doc += '\n:param ' + arg_name + ':'
				if arg_doc != '':
					doc += ' ' + arg_doc
				doc += '\n:type ' + arg_name + ': ' + argument_type.type_str
		if xml_method_return is not None:
			return_type = xml_method_return.get('type')
			return_complete_type = xml_method_return.get('completetype')
			return_contained_type = xml_method_return.get('containedtype')
			if return_complete_type != 'void':
				return_doc = self.__format_doc_content(None, xml_method_return.find('description'))
				return_argument_type = ArgumentType(return_type, return_complete_type, return_contained_type, self)
				doc += '\n:returns: ' + return_doc
				doc += '\n:rtype: ' + return_argument_type.type_str
		doc = self.__replace_doc_cfunction_by_method(doc)
		doc = self.__replace_doc_keywords(doc)
		doc = self.__replace_doc_special_chars(doc)
		return doc

	def __format_setter_doc(self, xml_node):
		xml_method_arg = xml_node.findall('./arguments/argument')[1]
		arg_type = xml_method_arg.get('type')
		arg_complete_type = xml_method_arg.get('completetype')
		arg_contained_type = xml_method_arg.get('containedtype')
		argument_type = ArgumentType(arg_type, arg_complete_type, arg_contained_type, self)
		doc = self.__format_doc_content(xml_node.find('briefdescription'), xml_node.find('detaileddescription'))
		doc = '[' + argument_type.type_str + '] ' + doc
		doc = self.__replace_doc_cfunction_by_method(doc)
		doc = self.__replace_doc_keywords(doc)
		doc = self.__replace_doc_special_chars(doc)
		return doc

	def __format_getter_doc(self, xml_node):
		xml_method_return = xml_node.find('./return')
		return_type = xml_method_return.get('type')
		return_complete_type = xml_method_return.get('completetype')
		return_contained_type = xml_method_return.get('containedtype')
		return_argument_type = ArgumentType(return_type, return_complete_type, return_contained_type, self)
		doc = self.__format_doc_content(xml_node.find('briefdescription'), xml_node.find('detaileddescription'))
		doc = '[' + return_argument_type.type_str + '] ' + doc
		doc = self.__replace_doc_cfunction_by_method(doc)
		doc = self.__replace_doc_keywords(doc)
		doc = self.__replace_doc_special_chars(doc)
		return doc
