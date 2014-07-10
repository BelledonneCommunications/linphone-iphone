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

class MethodDefinition:
	def __init__(self, method_node, class_):
		self.body = ''
		self.arg_names = []
		self.parse_tuple_format = ''
		self.build_value_format = ''
		self.return_type = 'void'
		self.method_node = method_node
		self.class_ = class_
		self.self_arg = None
		self.xml_method_return = self.method_node.find('./return')
		self.xml_method_args = self.method_node.findall('./arguments/argument')
		self.method_type = self.method_node.tag
		if self.method_type != 'classmethod' and len(self.xml_method_args) > 0:
			self.self_arg = self.xml_method_args[0]
			self.xml_method_args = self.xml_method_args[1:]

	def format_local_variables_definition(self):
		self.return_type = self.xml_method_return.get('type')
		if self.return_type != 'void':
			self.body += "\t" + self.return_type + " cresult;\n"
			self.build_value_format = self.__ctype_to_python_format(self.return_type)
			if self.build_value_format == 'O':
				self.body += "\tPyObject * pyresult;\n"
				self.body += "\tPyObject * pyret;\n"
		if self.self_arg is not None:
			self.body += "\t" + self.self_arg.get('type') + "native_ptr;\n"
		for xml_method_arg in self.xml_method_args:
			self.parse_tuple_format += self.__ctype_to_python_format(xml_method_arg.get('type'))
			self.body += "\t" + xml_method_arg.get('type') + " " + xml_method_arg.get('name') + ";\n"
			self.arg_names.append(xml_method_arg.get('name'))

	def format_native_pointer_get(self):
		self.body += "\tnative_ptr = pylinphone_" + self.class_['class_name'] + "_get_native_ptr(self);\n"

	def format_native_pointer_checking(self, return_int):
		self.format_native_pointer_get()
		self.body += "\tif(native_ptr == NULL) {\n"
		self.body += "\t\tPyErr_SetString(PyExc_TypeError, \"Invalid " + self.class_['class_name'] + " instance\");\n"
		if return_int:
			self.body += "\t\treturn -1;\n"
		else:
			self.body += "\t\treturn NULL;\n"
		self.body += "\t}\n"

	def format_arguments_parsing(self):
		if self.self_arg is not None:
			self.format_native_pointer_checking(False)
		if len(self.arg_names) > 0:
			self.body += "\tif (!PyArg_ParseTuple(args, \"" + self.parse_tuple_format + "\""
			self.body += ', ' + ', '.join(map(lambda a: '&' + a, self.arg_names))
			self.body += ")) {\n\t\treturn NULL;\n\t}\n"

	def format_setter_value_checking_and_c_function_call(self):
		# Check the native pointer
		self.format_native_pointer_checking(True)
		self.body += "\tnative_ptr = pylinphone_" + self.class_['class_name'] + "_get_native_ptr(self);\n"
		self.body += "\tif(native_ptr == NULL) {\n"
		self.body += "\t\tPyErr_SetString(PyExc_TypeError, \"Invalid " + self.class_['class_name'] + " instance\");\n"
		self.body += "\t\treturn -1;\n"
		self.body += "\t}\n"
		# Check that the value exists
		self.body += "\tif (value == NULL) {\n"
		self.body += "\t\tPyErr_SetString(PyExc_TypeError, \"Cannot delete this attribute\");\n"
		self.body += "\t\treturn -1;\n"
		self.body += "\t}\n"
		# Check the value
		basic_type, checkfunc, convertfunc = self.__ctype_to_python_type(self.xml_method_args[0].get('type'))
		self.body += "\tif (!" + checkfunc + "(value)) {\n"
		self.body += "\t\tPyErr_SetString(PyExc_TypeError, \"This attribute value must be a " + basic_type + "\");\n"
		self.body += "\t\treturn -1;\n"
		self.body += "\t}\n"
		# Call the C function
		if convertfunc is None:
			pass # TODO
		else:
			self.body += "\t" + self.arg_names[0] + " = (" + self.xml_method_args[0].get('type') + ")" + convertfunc + "(value);\n"
		self.body += "\t" + self.method_node.get('name') + "(native_ptr, " + self.arg_names[0] + ");"

	def format_tracing(self):
		self.body += "\tpylinphone_trace(__FUNCTION__);\n"

	def format_c_function_call(self):
		self.body += "\t"
		if self.return_type != 'void':
			self.body += "cresult = "
		self.body += self.method_node.get('name') + "("
		if self.self_arg is not None:
			self.body += "native_ptr"
			if len(self.arg_names) > 0:
				self.body += ', '
		self.body += ', '.join(self.arg_names) + ");\n"

	def format_method_result(self):
		if self.return_type != 'void':
			if self.build_value_format == 'O':
				self.body += "\tpyresult = pylinphone_" + self.class_['class_name'] + "_new_from_native_ptr(&pylinphone_" + self.class_['class_name'] + "Type, cresult);\n"
				self.body += "\tpyret = Py_BuildValue(\"" + self.build_value_format + "\", pyresult);\n"
				self.body += "\tPy_DECREF(pyresult);\n"
				self.body += "\treturn pyret;"
			else:
				self.body += "\treturn Py_BuildValue(\"" + self.build_value_format + "\", cresult);"
		else:
			self.body += "\tPy_RETURN_NONE;"

	def format_dealloc_c_function_call(self):
		if self.class_['class_refcountable']:
			self.body += "\tif (native_ptr != NULL) {\n"
			self.body += "\t\t" + self.class_['class_c_function_prefix'] + "unref(native_ptr);\n"
			self.body += "\t}\n"
		elif self.class_['class_destroyable']:
			self.body += "\tif (native_ptr != NULL) {\n"
			self.body += "\t\t" + self.class_['class_c_function_prefix'] + "destroy(native_ptr);\n"
			self.body += "\t}\n"
		self.body += "\tself->ob_type->tp_free(self);"

	def __get_basic_type_from_c_type(self, ctype):
		basic_type = 'int'
		keywords = ['const', 'struct', 'enum', 'signed', 'unsigned', 'short', 'long', '*']
		splitted_type = ctype.split(' ')
		for s in splitted_type:
			if s not in keywords:
				basic_type = s
				break
		return (basic_type, splitted_type)

	def __ctype_to_python_format(self, ctype):
		basic_type, splitted_type = self.__get_basic_type_from_c_type(ctype)
		if basic_type == 'char':
			if '*' in splitted_type:
				return 's'
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
			return 'O'

	def __ctype_to_python_type(self, ctype):
		basic_type, splitted_type = self.__get_basic_type_from_c_type(ctype)
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
			return ('class instance', 'PyInstance_Check', None)


class LinphoneModule(object):
	def __init__(self, tree, blacklisted_functions):
		self.internal_instance_method_names = ['destroy', 'ref', 'unref']
		self.enums = []
		xml_enums = tree.findall("./enums/enum")
		for xml_enum in xml_enums:
			if xml_enum.get('deprecated') == 'true':
				continue
			e = {}
			e['enum_name'] = self.__strip_leading_linphone(xml_enum.get('name'))
			e['enum_doc'] = self.__format_doc(xml_enum.find('briefdescription'), xml_enum.find('detaileddescription'))
			e['enum_values'] = []
			xml_enum_values = xml_enum.findall("./values/value")
			for xml_enum_value in xml_enum_values:
				if xml_enum_value.get('deprecated') == 'true':
					continue
				v = {}
				v['enum_value_cname'] = xml_enum_value.get('name')
				v['enum_value_name'] = self.__strip_leading_linphone(v['enum_value_cname'])
				e['enum_values'].append(v)
			self.enums.append(e)
		self.classes = []
		xml_classes = tree.findall("./classes/class")
		for xml_class in xml_classes:
			if xml_class.get('deprecated') == 'true':
				continue
			c = {}
			c['class_cname'] = xml_class.get('name')
			c['class_name'] = self.__strip_leading_linphone(c['class_cname'])
			c['class_c_function_prefix'] = xml_class.get('cfunctionprefix')
			c['class_doc'] = self.__format_doc(xml_class.find('briefdescription'), xml_class.find('detaileddescription'))
			c['class_refcountable'] = (xml_class.get('refcountable') == 'true')
			c['class_destroyable'] = (xml_class.get('destroyable') == 'true')
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
				m['method_body'] = self.__format_method_body(xml_type_method, c)
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
				m['method_body'] = self.__format_method_body(xml_instance_method, c)
				c['class_instance_methods'].append(m)
			c['class_properties'] = []
			xml_properties = xml_class.findall("./properties/property")
			for xml_property in xml_properties:
				p = {}
				p['property_name'] = xml_property.get('name')
				xml_property_getter = xml_property.find("./getter")
				xml_property_setter = xml_property.find("./setter")
				if xml_property_getter is not None:
					p['getter_name'] = xml_property_getter.get('name').replace(c['class_c_function_prefix'], '')
					p['getter_body'] = self.__format_getter_body(xml_property_getter, c)
				if xml_property_setter is not None:
					p['setter_name'] = xml_property_setter.get('name').replace(c['class_c_function_prefix'], '')
					p['setter_body'] = self.__format_setter_body(xml_property_setter, c)
				c['class_properties'].append(p)
			if c['class_refcountable']:
				xml_instance_method = xml_class.find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "unref']")
				c['dealloc_body'] = self.__format_dealloc_body(xml_instance_method, c)
			elif c['class_destroyable']:
				xml_instance_method = xml_class.find("./instancemethods/instancemethod[@name='" + c['class_c_function_prefix'] + "destroy']")
				c['dealloc_body'] = self.__format_dealloc_body(xml_instance_method, c)
			self.classes.append(c)

	def __strip_leading_linphone(self, s):
		if s.lower().startswith('linphone'):
			return s[8:]
		else:
			return s

	def __format_method_body(self, method_node, class_):
		method = MethodDefinition(method_node, class_)
		method.format_local_variables_definition()
		method.format_arguments_parsing()
		method.format_tracing()
		method.format_c_function_call()
		method.format_method_result()
		return method.body

	def __format_getter_body(self, getter_node, class_):
		method = MethodDefinition(getter_node, class_)
		method.format_local_variables_definition()
		method.format_arguments_parsing()
		method.format_tracing()
		method.format_c_function_call()
		method.format_method_result()
		return method.body

	def __format_setter_body(self, setter_node, class_):
		method = MethodDefinition(setter_node, class_)
		# Force return value type of dealloc function to prevent declaring useless local variables
		# TODO: Investigate. Maybe we should decide that setters must always return an int value.
		method.xml_method_return.set('type', 'void')
		method.format_local_variables_definition()
		method.format_tracing()
		method.format_setter_value_checking_and_c_function_call()
		return method.body

	def __format_dealloc_body(self, method_node, class_):
		method = MethodDefinition(method_node, class_)
		# Force return value type of dealloc function to prevent declaring useless local variables
		method.xml_method_return.set('type', 'void')
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
