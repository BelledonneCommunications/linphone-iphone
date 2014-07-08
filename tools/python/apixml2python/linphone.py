class LinphoneModule(object):
	def __init__(self, tree):
		self.enums = []
		xml_enums = tree.findall("./enums/enum")
		for xml_enum in xml_enums:
			e = {}
			e['enum_name'] = xml_enum.get('name')
			e['enum_doc'] = self.__format_doc(xml_enum.find('briefdescription'), xml_enum.find('detaileddescription'))
			e['enum_values'] = []
			xml_enum_values = xml_enum.findall("./values/value")
			for xml_enum_value in xml_enum_values:
				v = {}
				v['enum_value_name'] = xml_enum_value.get('name')
				e['enum_values'].append(v)
			self.enums.append(e)
		self.classes = []
		xml_classes = tree.findall("./classes/class")
		for xml_class in xml_classes:
			c = {}
			c['class_name'] = xml_class.get('name')
			c['class_c_function_prefix'] = xml_class.get('cfunctionprefix')
			c['class_doc'] = self.__format_doc(xml_class.find('briefdescription'), xml_class.find('detaileddescription'))
			c['class_type_methods'] = []
			xml_type_methods = xml_class.findall("./classmethods/classmethod")
			for xml_type_method in xml_type_methods:
				m = {}
				m['method_name'] = xml_type_method.get('name').replace(c['class_c_function_prefix'], '')
				m['method_body'] = self.__format_method_body(xml_type_method)
				c['class_type_methods'].append(m)
			c['class_instance_methods'] = []
			xml_instance_methods = xml_class.findall("./instancemethods/instancemethod")
			for xml_instance_method in xml_instance_methods:
				m = {}
				m['method_name'] = xml_instance_method.get('name').replace(c['class_c_function_prefix'], '')
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
				if xml_property_setter is not None:
					p['setter_name'] = xml_property_setter.get('name').replace(c['class_c_function_prefix'], '')
				c['class_properties'].append(p)
			self.classes.append(c)

	def __ctype_to_parse_tuple_format(self, ctype):
		keywords = ['const', 'struct', 'enum', 'signed', 'unsigned', 'short', 'long', '*']
		splitted_type = ctype.split(' ')
		for s in splitted_type:
			if s not in keywords:
				basic_type = s
				break
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
		else:
			return 'O'

	def __format_method_body(self, method_node):
		body = ''
		parse_tuple_format = ''
		xml_method_args = method_node.findall('./arguments/argument')
		arg_names = []
		for xml_method_arg in xml_method_args:
			parse_tuple_format += self.__ctype_to_parse_tuple_format(xml_method_arg.get('type'))
			body += "\t" + xml_method_arg.get('type') + " " + xml_method_arg.get('name') + ";\n"
			arg_names.append(xml_method_arg.get('name'))
		body += "\tpylinphone_trace(__FUNCTION__);\n"
		if len(xml_method_args) > 0:
			body += "\n\tif (!PyArg_ParseTuple(args, \"" + parse_tuple_format + "\""
			body += ', ' + ', '.join(map(lambda a: '&' + a, arg_names))
			body += ")) {\n\t\treturn NULL;\n\t}\n\n"
		body += "\tPy_RETURN_NONE;"
		return body

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
