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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

def strip_leading_linphone(s):
	if s.lower().startswith('linphone'):
		return s[8:]
	else:
		return s

class MethodDefinition:
	def __init__(self, method_node, class_, linphone_module):
		self.body = ''
		self.arg_names = []
		self.parse_tuple_format = ''
		self.build_value_format = ''
		self.return_type = 'void'
		self.method_node = method_node
		self.class_ = class_
		self.linphone_module = linphone_module
		self.self_arg = None
		self.xml_method_return = self.method_node.find('./return')
		self.xml_method_args = self.method_node.findall('./arguments/argument')
		self.method_type = self.method_node.tag
		if self.method_type != 'classmethod' and len(self.xml_method_args) > 0:
			self.self_arg = self.xml_method_args[0]
			self.xml_method_args = self.xml_method_args[1:]

	def format_local_variables_definition(self):
		self.return_type = self.xml_method_return.get('type')
		self.return_complete_type = self.xml_method_return.get('completetype')
		if self.return_complete_type != 'void':
			self.body += "\t" + self.return_complete_type + " cresult;\n"
			self.build_value_format = self.__ctype_to_python_format(self.return_type, self.return_complete_type)
			if self.build_value_format == 'O':
				self.body += "\tPyObject * pyresult;\n"
				self.body += "\tPyObject * pyret;\n"
		if self.self_arg is not None:
			self.body += "\t" + self.self_arg.get('completetype') + "native_ptr;\n"
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			fmt = self.__ctype_to_python_format(arg_type, arg_complete_type)
			self.parse_tuple_format += fmt
			if fmt == 'O':
				self.body += "\tPyObject * " + arg_name + ";\n"
				self.body += "\t" + arg_complete_type + " " + arg_name + "_native_ptr;\n"
			elif strip_leading_linphone(arg_complete_type) in self.linphone_module.enum_names:
				self.body += "\tint " + arg_name + ";\n"
			else:
				self.body += "\t" + arg_complete_type + " " + arg_name + ";\n"
			self.arg_names.append(arg_name)

	def format_native_pointer_get(self):
		self.body += \
"""	native_ptr = pylinphone_{class_name}_get_native_ptr(self);
""".format(class_name=self.class_['class_name'])

	def format_native_pointer_checking(self, return_int):
		self.format_native_pointer_get()
		return_value = "NULL"
		if return_int:
			return_value = "-1"
		self.body += \
"""	if (native_ptr == NULL) {{
		PyErr_SetString(PyExc_TypeError, "Invalid linphone.{class_name} instance");
		return {return_value};
	}}
""".format(class_name=self.class_['class_name'], return_value=return_value)

	def format_object_args_native_pointers_checking(self):
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			fmt = self.__ctype_to_python_format(arg_type, arg_complete_type)
			if fmt == 'O':
				self.body += \
"""	if (({arg_name}_native_ptr = pylinphone_{arg_type}_get_native_ptr({arg_name})) == NULL) {{
		return NULL;
	}}
""".format(arg_name=arg_name, arg_type=strip_leading_linphone(arg_type))

	def format_arguments_parsing(self):
		if self.self_arg is not None:
			self.format_native_pointer_checking(False)
		if len(self.arg_names) > 0:
			self.body += \
"""	if (!PyArg_ParseTuple(args, "{fmt}", {args})) {{
		return NULL;
	}}
""".format(fmt=self.parse_tuple_format, args=', '.join(map(lambda a: '&' + a, self.arg_names)))
		self.format_object_args_native_pointers_checking()

	def format_setter_value_checking_and_c_function_call(self):
		attribute_name = self.method_node.get('property_name')
		first_arg_type = self.xml_method_args[0].get('type')
		first_arg_complete_type = self.xml_method_args[0].get('completetype')
		first_arg_name = self.xml_method_args[0].get('name')
		self.format_native_pointer_checking(True)
		# Check that the value exists
		self.body += \
"""	if (value == NULL) {{
		PyErr_SetString(PyExc_TypeError, "Cannot delete the {attribute_name} attribute");
		return -1;
	}}
""".format(attribute_name=attribute_name)
		# Check the value
		type_str, checkfunc, convertfunc = self.__ctype_to_python_type(first_arg_type, first_arg_complete_type)
		first_arg_class = strip_leading_linphone(first_arg_type)
		if checkfunc is None:
			self.body += \
"""	if (!PyObject_IsInstance(value, (PyObject *)&pylinphone_{class_name}Type)) {{
		PyErr_SetString(PyExc_TypeError, "The {attribute_name} attribute value must be a linphone.{class_name} instance");
		return -1;
	}}
""".format(class_name=first_arg_class, attribute_name=attribute_name)
		else:
			self.body += \
"""	if (!{checkfunc}(value)) {{
		PyErr_SetString(PyExc_TypeError, "The {attribute_name} attribute value must be a {type_str}");
		return -1;
	}}
""".format(checkfunc=checkfunc, attribute_name=attribute_name, type_str=type_str)
		# Call the C function
		if convertfunc is None:
			self.body += \
"""	{arg_name} = value;
""".format(arg_name=first_arg_name)
		else:
			self.body += \
"""	{arg_name} = ({arg_type}){convertfunc}(value);
""".format(arg_name=first_arg_name, arg_type=first_arg_complete_type, convertfunc=convertfunc)
		if self.__ctype_to_python_format(first_arg_type, first_arg_complete_type) == 'O':
			self.body += \
"""	{arg_name}_native_ptr = pylinphone_{arg_class}_get_native_ptr({arg_name});
	if ({arg_name}_native_ptr == NULL) {{
		PyErr_SetString(PyExc_TypeError, "Invalid linphone.{arg_class} instance");
		return -1;
	}}
	{method_name}(native_ptr, {arg_name}_native_ptr);
""".format(arg_name=first_arg_name, arg_class=first_arg_class, method_name=self.method_node.get('name'))
		else:
			self.body += \
"""	{method_name}(native_ptr, {arg_name});
""".format(method_name=self.method_node.get('name'), arg_name=first_arg_name)
		self.body += \
"""	return 0;"""

	def format_tracing(self):
		self.body += \
"""	pylinphone_trace(__FUNCTION__);
"""

	def format_c_function_call(self):
		arg_names = []
		for xml_method_arg in self.xml_method_args:
			arg_name = xml_method_arg.get('name')
			arg_type = xml_method_arg.get('type')
			arg_complete_type = xml_method_arg.get('completetype')
			type_str, checkfunc, convertfunc = self.__ctype_to_python_type(arg_type, arg_complete_type)
			if convertfunc is None:
				arg_names.append(arg_name + "_native_ptr")
			else:
				arg_names.append(arg_name)
		self.body += "\t"
		if self.return_type != 'void':
			self.body += "cresult = "
		self.body += self.method_node.get('name') + "("
		if self.self_arg is not None:
			self.body += "native_ptr"
			if len(arg_names) > 0:
				self.body += ', '
		self.body += ', '.join(arg_names) + ");\n"

	def format_method_result(self):
		if self.return_complete_type != 'void':
			if self.build_value_format == 'O':
				stripped_return_type = strip_leading_linphone(self.return_type)
				return_type_class = self.__find_class_definition(self.return_type)
				if return_type_class['class_has_user_data']:
					get_user_data_function = return_type_class['class_c_function_prefix'] + "get_user_data"
					self.body += \
"""	if ((cresult != NULL) && ({func}(cresult) != NULL)) {{
		return (PyObject *){func}(cresult);
	}}
""".format(func=get_user_data_function)
				self.body += \
"""	pyresult = pylinphone_{return_type}_new_from_native_ptr(&pylinphone_{return_type}Type, cresult);
	pyret = Py_BuildValue("{fmt}", pyresult);
	Py_DECREF(pyresult);
	return pyret;
""".format(return_type=stripped_return_type, fmt=self.build_value_format)
			else:
				self.body += \
"""	return Py_BuildValue("{fmt}", cresult);""".format(fmt=self.build_value_format)
		else:
			self.body += \
"""	Py_RETURN_NONE;"""

	def format_new_from_native_pointer_body(self):
		self.body += \
"""	pylinphone_{class_name}Object *self;
""".format(class_name=self.class_['class_name'])
		self.format_tracing()
		self.body += \
"""	if (native_ptr == NULL) Py_RETURN_NONE;
	self = (pylinphone_{class_name}Object *)PyObject_New(pylinphone_{class_name}Object, type);
	if (self == NULL) Py_RETURN_NONE;
	self->native_ptr = ({class_cname} *)native_ptr;
""".format(class_name=self.class_['class_name'], class_cname=self.class_['class_cname'])
		if self.class_['class_has_user_data']:
			self.body += \
"""	{function_prefix}set_user_data(self->native_ptr, self);
""".format(function_prefix=self.class_['class_c_function_prefix'])
		self.body += \
"""	return (PyObject *)self;"""

	def format_dealloc_c_function_call(self):
		if self.class_['class_refcountable']:
			self.body += \
"""	if (native_ptr != NULL) {{
		{function_prefix}unref(native_ptr);
	}}
""".format(function_prefix=self.class_['class_c_function_prefix'])
		elif self.class_['class_destroyable']:
			self.body += \
"""	if (native_ptr != NULL) {{
		{function_prefix}destroy(native_ptr);
	}}
""".format(function_prefix=self.class_['class_c_function_prefix'])
		self.body += \
"""	self->ob_type->tp_free(self);"""

	def __ctype_to_python_format(self, basic_type, complete_type):
		splitted_type = complete_type.split(' ')
		if basic_type == 'char':
			if '*' in splitted_type:
				return 'z'
			elif 'unsigned' in splitted_type:
				return 'b'
		elif basic_type == 'int':
			# TODO:
			return 'i'
		elif basic_type == 'int8_t':
			return 'c'
		elif basic_type == 'uint8_t':
			return 'b'
		elif basic_type == 'int16_t':
			return 'h'
		elif basic_type == 'uint16_t':
			return 'H'
		elif basic_type == 'int32_t':
			return 'l'
		elif basic_type == 'uint32_t':
			return 'k'
		elif basic_type == 'int64_t':
			return 'L'
		elif basic_type == 'uint64_t':
			return 'K'
		elif basic_type == 'size_t':
			return 'n'
		elif basic_type == 'float':
			return 'f'
		elif basic_type == 'double':
			return 'd'
		elif basic_type == 'bool_t':
			return 'i'
		else:
			if strip_leading_linphone(basic_type) in self.linphone_module.enum_names:
				return 'i'
			else:
				return 'O'

	def __ctype_to_python_type(self, basic_type, complete_type):
		splitted_type = complete_type.split(' ')
		if basic_type == 'char':
			if '*' in splitted_type:
				return ('string', 'PyString_Check', 'PyString_AsString')
			else:
				return ('int', 'PyInt_Check', 'PyInt_AsLong')
		elif basic_type == 'int':
			if 'unsigned' in splitted_type:
				return ('unsigned int', 'PyLong_Check', 'PyLong_AsUnsignedLong')
			else:
				return ('int', 'PyLong_Check', 'PyLong_AsLong')
		elif basic_type in ['int8_t', 'int16_t' 'int32_t']:
			return ('int', 'PyLong_Check', 'PyLong_AsLong')
		elif basic_type in ['uint8_t', 'uin16_t', 'uint32_t']:
			return ('unsigned int', 'PyLong_Check', 'PyLong_AsUnsignedLong')
		elif basic_type == 'int64_t':
			return ('64bits int', 'PyLong_Check', 'PyLong_AsLongLong')
		elif basic_type == 'uint64_t':
			return ('64bits unsigned int', 'PyLong_Check', 'PyLong_AsUnsignedLongLong')
		elif basic_type == 'size_t':
			return ('size_t', 'PyLong_Check', 'PyLong_AsSsize_t')
		elif basic_type in ['float', 'double']:
			return ('float', 'PyFloat_Check', 'PyFloat_AsDouble')
		elif basic_type == 'bool_t':
			return ('bool', 'PyBool_Check', 'PyInt_AsLong')
		else:
			if strip_leading_linphone(basic_type) in self.linphone_module.enum_names:
				return ('int', 'PyInt_Check', 'PyInt_AsLong')
			else:
				return (None, None, None)

	def __find_class_definition(self, basic_type):
		basic_type = strip_leading_linphone(basic_type)
		for c in self.linphone_module.classes:
			if c['class_name'] == basic_type:
				return c
		return None


class LinphoneModule(object):
	def __init__(self, tree, blacklisted_functions):
		self.internal_instance_method_names = ['destroy', 'ref', 'unref']
		self.internal_property_names = ['user_data']
		self.enums = []
		self.enum_names = []
		xml_enums = tree.findall("./enums/enum")
		for xml_enum in xml_enums:
			if xml_enum.get('deprecated') == 'true':
				continue
			e = {}
			e['enum_name'] = strip_leading_linphone(xml_enum.get('name'))
			e['enum_doc'] = self.__format_doc(xml_enum.find('briefdescription'), xml_enum.find('detaileddescription'))
			e['enum_values'] = []
			xml_enum_values = xml_enum.findall("./values/value")
			for xml_enum_value in xml_enum_values:
				if xml_enum_value.get('deprecated') == 'true':
					continue
				v = {}
				v['enum_value_cname'] = xml_enum_value.get('name')
				v['enum_value_name'] = strip_leading_linphone(v['enum_value_cname'])
				e['enum_values'].append(v)
			self.enums.append(e)
			self.enum_names.append(e['enum_name'])
		self.classes = []
		xml_classes = tree.findall("./classes/class")
		for xml_class in xml_classes:
			if xml_class.get('deprecated') == 'true':
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
			xml_type_methods = xml_class.findall("./classmethods/classmethod")
			for xml_type_method in xml_type_methods:
				if xml_type_method.get('deprecated') == 'true':
					continue
				method_name = xml_type_method.get('name')
				if method_name in blacklisted_functions:
					continue
				m = {}
				m['method_name'] = method_name.replace(c['class_c_function_prefix'], '')
				m['method_xml_node'] = xml_type_method
				c['class_type_methods'].append(m)
			c['class_instance_methods'] = []
			xml_instance_methods = xml_class.findall("./instancemethods/instancemethod")
			for xml_instance_method in xml_instance_methods:
				if xml_instance_method.get('deprecated') == 'true':
					continue
				method_name = xml_instance_method.get('name')
				if method_name in blacklisted_functions:
					continue
				method_name = method_name.replace(c['class_c_function_prefix'], '')
				if method_name in self.internal_instance_method_names:
					continue
				m = {}
				m['method_name'] = method_name
				m['method_xml_node'] = xml_instance_method
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
				if xml_property_getter is not None and (
					xml_property_getter.get('name') in blacklisted_functions or xml_property_getter.get('deprecated') == 'true'):
					continue
				if xml_property_setter is not None and (
					xml_property_setter.get('name') in blacklisted_functions or xml_property_setter.get('deprecated') == 'true'):
					continue
				if xml_property_getter is not None:
					xml_property_getter.set('property_name', property_name)
					p['getter_name'] = xml_property_getter.get('name').replace(c['class_c_function_prefix'], '')
					p['getter_xml_node'] = xml_property_getter
					p['getter_reference'] = "(getter)pylinphone_" + c['class_name'] + "_" + p['getter_name']
					p['getter_definition_begin'] = "static PyObject * pylinphone_" + c['class_name'] + "_" + p['getter_name'] + "(PyObject *self, void *closure) {"
					p['getter_definition_end'] = "}"
				else:
					p['getter_reference'] = "NULL"
				if xml_property_setter is not None:
					xml_property_setter.set('property_name', property_name)
					p['setter_name'] = xml_property_setter.get('name').replace(c['class_c_function_prefix'], '')
					p['setter_xml_node'] = xml_property_setter
					p['setter_reference'] = "(setter)pylinphone_" + c['class_name'] + "_" + p['setter_name']
					p['setter_definition_begin'] = "static int pylinphone_" + c['class_name'] + "_" + p['setter_name'] + "(PyObject *self, PyObject *value, void *closure) {"
					p['setter_definition_end'] = "}"
				else:
					p['setter_reference'] = "NULL"
				c['class_properties'].append(p)
			self.classes.append(c)
		# Format methods' bodies
		for c in self.classes:
			for m in c['class_type_methods']:
				m['method_body'] = self.__format_method_body(m['method_xml_node'], c)
			for m in c['class_instance_methods']:
				m['method_body'] = self.__format_method_body(m['method_xml_node'], c)
			for p in c['class_properties']:
				if p.has_key('getter_xml_node'):
					p['getter_body'] = self.__format_getter_body(p['getter_xml_node'], c)
				if p.has_key('setter_xml_node'):
					p['setter_body'] = self.__format_setter_body(p['setter_xml_node'], c)
			if c['class_refcountable']:
				xml_instance_method = c['class_xml_node'].find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "unref']")
				c['new_from_native_pointer_body'] = self.__format_new_from_native_pointer_body(xml_instance_method, c)
				c['dealloc_body'] = self.__format_dealloc_body(xml_instance_method, c)
			elif c['class_destroyable']:
				xml_instance_method = c['class_xml_node'].find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "destroy']")
				c['new_from_native_pointer_body'] = self.__format_new_from_native_pointer_body(xml_instance_method, c)
				c['dealloc_body'] = self.__format_dealloc_body(xml_instance_method, c)

	def __format_method_body(self, method_node, class_):
		method = MethodDefinition(method_node, class_, self)
		method.format_local_variables_definition()
		method.format_arguments_parsing()
		method.format_tracing()
		method.format_c_function_call()
		method.format_method_result()
		return method.body

	def __format_getter_body(self, getter_node, class_):
		method = MethodDefinition(getter_node, class_, self)
		method.format_local_variables_definition()
		method.format_arguments_parsing()
		method.format_tracing()
		method.format_c_function_call()
		method.format_method_result()
		return method.body

	def __format_setter_body(self, setter_node, class_):
		method = MethodDefinition(setter_node, class_, self)
		# Force return value type of dealloc function to prevent declaring useless local variables
		# TODO: Investigate. Maybe we should decide that setters must always return an int value.
		method.xml_method_return.set('type', 'void')
		method.xml_method_return.set('completetype', 'void')
		method.format_local_variables_definition()
		method.format_tracing()
		method.format_setter_value_checking_and_c_function_call()
		return method.body

	def __format_new_from_native_pointer_body(self, method_node, class_):
		method = MethodDefinition(method_node, class_, self)
		# Force return value type of dealloc function to prevent declaring useless local variables
		method.xml_method_return.set('type', 'void')
		method.xml_method_return.set('completetype', 'void')
		method.format_new_from_native_pointer_body()
		return method.body

	def __format_dealloc_body(self, method_node, class_):
		method = MethodDefinition(method_node, class_, self)
		# Force return value type of dealloc function to prevent declaring useless local variables
		method.xml_method_return.set('type', 'void')
		method.xml_method_return.set('completetype', 'void')
		method.format_local_variables_definition()
		method.format_native_pointer_get()
		method.format_tracing()
		method.format_dealloc_c_function_call()
		return method.body

	def __format_doc_node(self, node):
		desc = ''
		if node.tag == 'para':
			desc += node.text.strip()
			for n in list(node):
				desc += self.__format_doc_node(n)
		elif node.tag == 'note':
			desc += node.text.strip()
			for n in list(node):
				desc += self.__format_doc_node(n)
		elif node.tag == 'ref':
			desc += ' ' + node.text.strip() + ' '
		tail = node.tail.strip()
		if tail != '':
			if node.tag != 'ref':
				desc += '\n'
			desc += tail
		if node.tag == 'para':
			desc += '\n'
		return desc

	def __format_doc(self, brief_description, detailed_description):
		doc = ''
		if brief_description is None:
			brief_description = ''
		if detailed_description is None:
			detailed_description = ''
		else:
			desc = ''
			for node in list(detailed_description):
				desc += self.__format_doc_node(node) + '\n'
			detailed_description = desc.strip().replace('\n', '\\n')
		brief_description = brief_description.strip()
		doc += brief_description
		if detailed_description != '':
			if doc != '':
				doc += '\\n\\n'
			doc+= detailed_description
		doc = '\"' + doc + '\"'
		return doc
