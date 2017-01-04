#!/usr/bin/python

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

import argparse
import os
import string
import sys
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom


class CObject:
	def __init__(self, name):
		self.name = name.strip()
		self.briefDescription = ''
		self.detailedDescription = None
		self.deprecated = False


class CEnumValue(CObject):
	pass


class CEnum(CObject):
	def __init__(self, name):
		CObject.__init__(self, name)
		self.values = []
		self.associatedTypedef = None

	def addValue(self, value):
		self.values.append(value)


class CStructMember(CObject):
	def __init__(self, name, t):
		CObject.__init__(self, name)
		self.ctype = t.strip()


class CStruct(CObject):
	def __init__(self, name):
		CObject.__init__(self, name)
		self.members = []
		self.associatedTypedef = None

	def addMember(self, member):
		self.members.append(member)


class CTypedef(CObject):
	def __init__(self, name, definition):
		CObject.__init__(self, name)
		self.definition = definition.strip()


class CArgument(CObject):
	def __init__(self, t, name = '', enums = [], structs = []):
		CObject.__init__(self, name)
		self.description = None
		self.containedType = None
		keywords = [ 'const', 'struct', 'enum', 'signed', 'unsigned', 'short', 'long', '*' ]
		fullySplittedType = []
		splittedType = t.strip().split(' ')
		for s in splittedType:
			if s.startswith('*'):
				fullySplittedType.append('*')
				if len(s) > 1:
					fullySplittedType.append(s[1:])
			elif s.endswith('*'):
				fullySplittedType.append(s[:-1])
				fullySplittedType.append('*')
			else:
				fullySplittedType.append(s)
		if 'MS2_DEPRECATED' in fullySplittedType:
			fullySplittedType.remove('MS2_DEPRECATED')
		elif 'LINPHONE_DEPRECATED' in fullySplittedType:
			fullySplittedType.remove('LINPHONE_DEPRECATED')
		isStruct = False
		isEnum = False
		self.ctype = 'int' # Default to int so that the result is correct eg. for 'unsigned short'
		for s in fullySplittedType:
			if not s in keywords:
				self.ctype = s
			if s == 'struct':
				isStruct = True
			if s == 'enum':
				isEnum = True
		if isStruct:
			for st in structs:
				if st.associatedTypedef is not None:
					self.ctype = st.associatedTypedef.name
		elif isEnum:
			for e in enums:
				if e.associatedTypedef is not None:
					self.ctype = e.associatedTypedef.name
		if self.ctype == 'int' and 'int' not in fullySplittedType:
			if fullySplittedType[-1] == '*':
				fullySplittedType.insert(-1, 'int')
			else:
				fullySplittedType.append('int')
		self.completeType = ' '.join(fullySplittedType)

	def __str__(self):
		return self.completeType + " " + self.name


class CArgumentsList:
	def __init__(self):
		self.arguments = []

	def addArgument(self, arg):
		self.arguments.append(arg)

	def __len__(self):
		return len(self.arguments)

	def __getitem__(self, key):
		return self.arguments[key]

	def __str__(self):
		argstr = []
		for arg in self.arguments:
			argstr.append(str(arg))
		return ', '.join(argstr)


class CFunction(CObject):
	def __init__(self, name, returnarg, argslist):
		CObject.__init__(self, name)
		self.returnArgument = returnarg
		self.arguments = argslist
		self.location = None


class CEvent(CFunction):
	pass


class CProperty:
	def __init__(self, name):
		self.name = name
		self.getter = None
		self.setter = None


class CClass(CObject):
	def __init__(self, st):
		CObject.__init__(self, st.associatedTypedef.name)
		if st.deprecated or st.associatedTypedef.deprecated:
			self.deprecated = True
		if len(st.associatedTypedef.briefDescription) > 0:
			self.briefDescription = st.associatedTypedef.briefDescription
		elif len(st.briefDescription) > 0:
			self.briefDescription = st.briefDescription
		if st.associatedTypedef.detailedDescription is not None:
			self.detailedDescription = st.associatedTypedef.detailedDescription
		elif st.detailedDescription is not None:
			self.detailedDescription = st.detailedDescription
		self.__struct = st
		self.events = {}
		self.classMethods = {}
		self.instanceMethods = {}
		self.properties = {}
		self.__computeCFunctionPrefix()

	def __computeCFunctionPrefix(self):
		self.cFunctionPrefix = ''
		first = True
		for l in self.name:
			if l.isupper() and not first:
				self.cFunctionPrefix += '_'
			self.cFunctionPrefix += l.lower()
			first = False
		self.cFunctionPrefix += '_'

	def __addPropertyGetter(self, name, f):
		if not name in self.properties:
			prop = CProperty(name)
			self.properties[name] = prop
		self.properties[name].getter = f

	def __addPropertySetter(self, name, f):
		if not name in self.properties:
			prop = CProperty(name)
			self.properties[name] = prop
		self.properties[name].setter = f

	def __addClassMethod(self, f):
		name = f.name[len(self.cFunctionPrefix):]
		if name.startswith('get_') and len(f.arguments) == 0:
			self.__addPropertyGetter(name[4:], f)
		elif name.startswith('is_') and len(f.arguments) == 0 and f.returnArgument.ctype == 'bool_t':
			self.__addPropertyGetter(name[3:], f)
		elif name.endswith('_enabled') and len(f.arguments) == 0 and f.returnArgument.ctype == 'bool_t':
			self.__addPropertyGetter(name, f)
		elif name.startswith('set_') and len(f.arguments) == 1:
			self.__addPropertySetter(name[4:], f)
		elif name.startswith('enable_') and len(f.arguments) == 1 and f.arguments[0].ctype == 'bool_t':
			self.__addPropertySetter(name[7:] + '_enabled', f)
		else:
			if not f.name in self.classMethods:
				self.classMethods[f.name] = f

	def __addInstanceMethod(self, f):
		name = f.name[len(self.cFunctionPrefix):]
		if name.startswith('get_') and len(f.arguments) == 1:
			self.__addPropertyGetter(name[4:], f)
		elif name.startswith('is_') and len(f.arguments) == 1 and f.returnArgument.ctype == 'bool_t':
			self.__addPropertyGetter(name[3:], f)
		elif name.endswith('_enabled') and len(f.arguments) == 1 and f.returnArgument.ctype == 'bool_t':
			self.__addPropertyGetter(name, f)
		elif name.startswith('set_') and len(f.arguments) == 2:
			self.__addPropertySetter(name[4:], f)
		elif name.startswith('enable_') and len(f.arguments) == 2 and f.arguments[1].ctype == 'bool_t':
			self.__addPropertySetter(name[7:] + '_enabled', f)
		else:
			if not f.name in self.instanceMethods:
				self.instanceMethods[f.name] = f

	def addEvent(self, ev):
		if not ev.name in self.events:
			self.events[ev.name] = ev

	def addMethod(self, f):
		if len(f.arguments) > 0 and f.arguments[0].ctype == self.name:
			self.__addInstanceMethod(f)
		else:
			self.__addClassMethod(f)


class Project:
	def __init__(self):
		self.verbose = False
		self.prettyPrint = False
		self.enums = []
		self.__structs = []
		self.__typedefs = []
		self.__events = []
		self.__functions = []
		self.classes = []

	def add(self, elem):
		if isinstance(elem, CClass):
			if self.verbose:
				print("Adding class " + elem.name)
			self.classes.append(elem)
		elif isinstance(elem, CEnum):
			if self.verbose:
				print("Adding enum " + elem.name)
				for ev in elem.values:
					print("\t" + ev.name)
			self.enums.append(elem)
		elif isinstance(elem, CStruct):
			if self.verbose:
				print("Adding struct " + elem.name)
				for sm in elem.members:
					print("\t" + sm.ctype + " " + sm.name)
			self.__structs.append(elem)
		elif isinstance(elem, CTypedef):
			if self.verbose:
				print("Adding typedef " + elem.name)
				print("\t" + elem.definition)
			self.__typedefs.append(elem)
		elif isinstance(elem, CEvent):
			if self.verbose:
				print("Adding event " + elem.name)
				print("\tReturns: " + elem.returnArgument.ctype)
				print("\tArguments: " + str(elem.arguments))
			self.__events.append(elem)
		elif isinstance(elem, CFunction):
			if self.verbose:
				print("Adding function " + elem.name)
				print("\tReturns: " + elem.returnArgument.ctype)
				print("\tArguments: " + str(elem.arguments))
			self.__functions.append(elem)

	def __cleanDescription(self, descriptionNode):
		for para in descriptionNode.findall('./para'):
			for n in para.findall('./parameterlist'):
				para.remove(n)
			for n in para.findall("./simplesect[@kind='return']"):
				para.remove(n)
			for n in para.findall("./simplesect[@kind='see']"):
				t = ''.join(n.itertext())
				n.clear()
				n.tag = 'see'
				n.text = t
			for n in para.findall("./simplesect[@kind='note']"):
				n.tag = 'note'
				n.attrib = {}
			for n in para.findall(".//xrefsect"):
				para.remove(n)
			for n in para.findall('.//ref'):
				n.attrib = {}
			for n in para.findall(".//bctbx_list"):
				para.remove(n)
		if descriptionNode.tag == 'parameterdescription':
			descriptionNode.tag = 'description'
		if descriptionNode.tag == 'simplesect':
			descriptionNode.tag = 'description'
			descriptionNode.attrib = {}
		return descriptionNode

	def __discoverClasses(self):
		for td in self.__typedefs:
			if td.definition.startswith('enum '):
				for e in self.enums:
					if (e.associatedTypedef is None) and td.definition[5:] == e.name:
						e.associatedTypedef = td
						break
			elif td.definition.startswith('struct '):
				structFound = False
				for st in self.__structs:
					if (st.associatedTypedef is None) and td.definition[7:] == st.name:
						st.associatedTypedef = td
						structFound = True
						break
				if not structFound:
					name = td.definition[7:]
					print("Structure with no associated typedef: " + name)
					st = CStruct(name)
					st.associatedTypedef = td
					self.add(st)
		for td in self.__typedefs:
			if td.definition.startswith('struct '):
				for st in self.__structs:
					if st.associatedTypedef == td:
						self.add(CClass(st))
						break
			elif ('Linphone' + td.definition) == td.name:
				st = CStruct(td.name)
				st.associatedTypedef = td
				self.add(st)
				self.add(CClass(st))
		# Sort classes by length of name (longest first), so that methods are put in the right class
		self.classes.sort(key = lambda c: len(c.name), reverse = True)
		for e in self.__events:
			eventAdded = False
			for c in self.classes:
				if c.name.endswith('Cbs') and e.name.startswith(c.name):
					c.addEvent(e)
					eventAdded = True
					break
			if not eventAdded:
				for c in self.classes:
					if e.name.startswith(c.name):
						c.addEvent(e)
						eventAdded = True
						break
		for f in self.__functions:
			for c in self.classes:
				if c.cFunctionPrefix == f.name[0 : len(c.cFunctionPrefix)]:
					c.addMethod(f)
					break

	def __parseCEnumValue(self, node):
		ev = CEnumValue(node.find('./name').text)
		deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
		if deprecatedNode is not None:
			ev.deprecated = True
		ev.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
		ev.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
		return ev

	def __parseCEnumMemberdef(self, node):
		e = CEnum(node.find('./name').text)
		deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
		if deprecatedNode is not None:
			e.deprecated = True
		e.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
		e.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
		enumvalues = node.findall("enumvalue[@prot='public']")
		for enumvalue in enumvalues:
			ev = self.__parseCEnumValue(enumvalue)
			e.addValue(ev)
		return e

	def __findCEnum(self, tree):
		memberdefs = tree.findall("./compounddef[@kind='group']/sectiondef[@kind='enum']/memberdef[@kind='enum'][@prot='public']")
		for m in memberdefs:
			e = self.__parseCEnumMemberdef(m)
			self.add(e)

	def __parseCStructMember(self, node, structname):
		name = node.find('./name').text
		definition = node.find('./definition').text
		t = definition[0:string.find(definition, structname + "::" + name)]
		sm = CStructMember(name, t)
		deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
		if deprecatedNode is not None:
			sm.deprecated = True
		sm.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
		sm.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
		return sm

	def __parseCStructCompounddef(self, node):
		s = CStruct(node.find('./compoundname').text)
		deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
		if deprecatedNode is not None:
			s.deprecated = True
		s.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
		s.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
		structmembers = node.findall("sectiondef/memberdef[@kind='variable'][@prot='public']")
		for structmember in structmembers:
			sm = self.__parseCStructMember(structmember, s.name)
			s.addMember(sm)
		return s

	def __findCStruct(self, tree):
		compounddefs = tree.findall("./compounddef[@kind='struct'][@prot='public']")
		for c in compounddefs:
			s = self.__parseCStructCompounddef(c)
			self.add(s)

	def __parseCTypedefMemberdef(self, node):
		name = node.find('./name').text
		definition = node.find('./definition').text
		if definition.startswith('typedef '):
			definition = definition[8 :]
		if name.endswith('Cb'):
			pos = string.find(definition, "(*")
			if pos == -1:
				return None
			returntype = definition[0:pos].strip()
			returnarg = CArgument(returntype, enums = self.enums, structs = self.__structs)
			returndesc = node.find("./detaileddescription/para/simplesect[@kind='return']")
			if returndesc is not None:
				if returnarg.ctype == 'MSList' or returnarg.ctype == 'bctbx_list_t':
					n = returndesc.find('.//bctbxlist')
					if n is not None:
						returnarg.containedType = n.text
				returnarg.description = self.__cleanDescription(returndesc)
			elif returnarg.completeType != 'void':
				missingDocWarning += "\tReturn value is not documented\n"
			definition = definition[pos + 2 :]
			pos = string.find(definition, "(")
			definition = definition[pos + 1 : -1]
			argslist = CArgumentsList()
			for argdef in definition.split(', '):
				argType = ''
				starPos = string.rfind(argdef, '*')
				spacePos = string.rfind(argdef, ' ')
				if starPos != -1:
					argType = argdef[0 : starPos + 1]
					argName = argdef[starPos + 1 :]
				elif spacePos != -1:
					argType = argdef[0 : spacePos]
					argName = argdef[spacePos + 1 :]
				argslist.addArgument(CArgument(argType, argName, self.enums, self.__structs))
			if len(argslist) > 0:
				paramdescs = node.findall("detaileddescription/para/parameterlist[@kind='param']/parameteritem")
				if paramdescs:
					for arg in argslist.arguments:
						for paramdesc in paramdescs:
							if arg.name == paramdesc.find('./parameternamelist').find('./parametername').text:
								arg.description = self.__cleanDescription(paramdesc.find('./parameterdescription'))
					missingDocWarning = ''
					for arg in argslist.arguments:
						if arg.description == None:
							missingDocWarning += "\t'" + arg.name + "' parameter not documented\n";
					if missingDocWarning != '':
						print(name + ":\n" + missingDocWarning)
			f = CEvent(name, returnarg, argslist)
			deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
			if deprecatedNode is not None:
				f.deprecated = True
			f.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
			f.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
			return f
		else:
			pos = string.rfind(definition, " " + name)
			if pos != -1:
				definition = definition[0 : pos]
			td = CTypedef(name, definition)
			deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
			if deprecatedNode is not None:
				td.deprecated = True
			td.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
			td.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
			return td
		return None

	def __findCTypedef(self, tree):
		memberdefs = tree.findall("./compounddef[@kind='group']/sectiondef[@kind='typedef']/memberdef[@kind='typedef'][@prot='public']")
		for m in memberdefs:
			td = self.__parseCTypedefMemberdef(m)
			self.add(td)

	def __parseCFunctionMemberdef(self, node):
		internal = node.find("./detaileddescription/internal")
		if internal is not None:
			return None
		missingDocWarning = ''
		name = node.find('./name').text
		t = ''.join(node.find('./type').itertext())
		returnarg = CArgument(t, enums = self.enums, structs = self.__structs)
		returndesc = node.find("./detaileddescription/para/simplesect[@kind='return']")
		if returndesc is not None:
			if returnarg.ctype == 'MSList' or returnarg.ctype == 'bctbx_list_t':
				n = returndesc.find('.//bctbxlist')
				if n is not None:
					returnarg.containedType = n.text
			returnarg.description = self.__cleanDescription(returndesc)
		elif returnarg.completeType != 'void':
			missingDocWarning += "\tReturn value is not documented\n"
		argslist = CArgumentsList()
		argslistNode = node.findall('./param')
		for argNode in argslistNode:
			argType = ''.join(argNode.find('./type').itertext())
			argName = ''
			argNameNode = argNode.find('./declname')
			if argNameNode is not None:
				argName = ''.join(argNameNode.itertext())
			if argType != 'void':
				argslist.addArgument(CArgument(argType, argName, self.enums, self.__structs))
		if len(argslist) > 0:
			paramdescs = node.findall("./detaileddescription/para/parameterlist[@kind='param']/parameteritem")
			if paramdescs:
				for arg in argslist.arguments:
					for paramdesc in paramdescs:
						if arg.name == paramdesc.find('./parameternamelist').find('./parametername').text:
							if arg.ctype == 'MSList' or arg.ctype == 'bctbx_list_t':
								n = paramdesc.find('.//bctbxlist')
								if n is not None:
									arg.containedType = n.text
							arg.description = self.__cleanDescription(paramdesc.find('./parameterdescription'))
				missingDocWarning = ''
				for arg in argslist.arguments:
					if arg.description == None:
						missingDocWarning += "\t'" + arg.name + "' parameter not documented\n";
		f = CFunction(name, returnarg, argslist)
		deprecatedNode = node.find(".//xrefsect[xreftitle='Deprecated']")
		if deprecatedNode is not None:
			f.deprecated = True
		f.briefDescription = ''.join(node.find('./briefdescription').itertext()).strip()
		f.detailedDescription = self.__cleanDescription(node.find('./detaileddescription'))
		if f.briefDescription == '' and ''.join(f.detailedDescription.itertext()).strip() == '':
			return None
		locationNode = node.find('./location')
		if locationNode is not None:
			f.location = locationNode.get('file')
			if not f.location.endswith('.h'):
				missingDocWarning += "\tNot documented in a header file ('" + f.location + "')\n";
		if missingDocWarning != '':
			print(name + ":\n" + missingDocWarning)
		return f

	def __findCFunction(self, tree):
		memberdefs = tree.findall("./compounddef[@kind='group']/sectiondef[@kind='func']/memberdef[@kind='function'][@prot='public'][@static='no']")
		for m in memberdefs:
			f = self.__parseCFunctionMemberdef(m)
			if f is not None:
				self.add(f)

	def initFromFiles(self, xmlfiles):
		trees = []
		for f in xmlfiles:
			tree = None
			try:
				if self.verbose:
					print("Parsing XML file: " + f.name)
				tree = ET.parse(f)
			except ET.ParseError as e:
				print(e)
			if tree is not None:
				trees.append(tree)
		for tree in trees:
			self.__findCEnum(tree)
		for tree in trees:
			self.__findCStruct(tree)
		for tree in trees:
			self.__findCTypedef(tree)
		for tree in trees:
			self.__findCFunction(tree)
		self.__discoverClasses()

	def initFromDir(self, xmldir):
		files = [ os.path.join(xmldir, f) for f in os.listdir(xmldir) if (os.path.isfile(os.path.join(xmldir, f)) and f.endswith('.xml')) ]
		self.initFromFiles(files)

	def check(self):
		for c in self.classes:
			for name, p in c.properties.iteritems():
				if p.getter is None and p.setter is not None:
					print("Property '" + name + "' of class '" + c.name + "' has a setter but no getter")


class Generator:
	def __init__(self, outputfile):
		self.__outputfile = outputfile

	def __generateEnum(self, cenum, enumsNode):
		enumNodeAttributes = { 'name' : cenum.name, 'deprecated' : str(cenum.deprecated).lower() }
		if cenum.associatedTypedef is not None:
			enumNodeAttributes['name'] = cenum.associatedTypedef.name
		enumNode = ET.SubElement(enumsNode, 'enum', enumNodeAttributes)
		if cenum.briefDescription != '':
			enumBriefDescriptionNode = ET.SubElement(enumNode, 'briefdescription')
			enumBriefDescriptionNode.text = cenum.briefDescription
		enumNode.append(cenum.detailedDescription)
		if len(cenum.values) > 0:
			enumValuesNode = ET.SubElement(enumNode, 'values')
			for value in cenum.values:
				enumValuesNodeAttributes = { 'name' : value.name, 'deprecated' : str(value.deprecated).lower() }
				valueNode = ET.SubElement(enumValuesNode, 'value', enumValuesNodeAttributes)
				if value.briefDescription != '':
					valueBriefDescriptionNode = ET.SubElement(valueNode, 'briefdescription')
					valueBriefDescriptionNode.text = value.briefDescription
				valueNode.append(value.detailedDescription)

	def __generateFunction(self, parentNode, nodeName, f):
		functionAttributes = { 'name' : f.name, 'deprecated' : str(f.deprecated).lower() }
		if f.location is not None:
			functionAttributes['location'] = f.location
		functionNode = ET.SubElement(parentNode, nodeName, functionAttributes)
		returnValueAttributes = { 'type' : f.returnArgument.ctype, 'completetype' : f.returnArgument.completeType }
		if f.returnArgument.containedType is not None:
			returnValueAttributes['containedtype'] = f.returnArgument.containedType
		returnValueNode = ET.SubElement(functionNode, 'return', returnValueAttributes)
		if f.returnArgument.description is not None:
			returnValueNode.append(f.returnArgument.description)
		argumentsNode = ET.SubElement(functionNode, 'arguments')
		for arg in f.arguments:
			argumentNodeAttributes = { 'name' : arg.name, 'type' : arg.ctype, 'completetype' : arg.completeType }
			if arg.containedType is not None:
				argumentNodeAttributes['containedtype'] = arg.containedType
			argumentNode = ET.SubElement(argumentsNode, 'argument', argumentNodeAttributes)
			if arg.description is not None:
				argumentNode.append(arg.description)
		if f.briefDescription != '':
			functionBriefDescriptionNode = ET.SubElement(functionNode, 'briefdescription')
			functionBriefDescriptionNode.text = f.briefDescription
		functionNode.append(f.detailedDescription)

	def __generateClass(self, cclass, classesNode):
		# Do not include classes that contain nothing
		if len(cclass.events) == 0 and len(cclass.classMethods) == 0 and \
			len(cclass.instanceMethods) == 0 and len(cclass.properties) == 0:
			return
		# Check the capabilities of the class
		has_ref_method = False
		has_unref_method = False
		has_destroy_method = False
		for methodname in cclass.instanceMethods:
			methodname_without_prefix = methodname.replace(cclass.cFunctionPrefix, '')
			if methodname_without_prefix == 'ref':
				has_ref_method = True
			elif methodname_without_prefix == 'unref':
				has_unref_method = True
			elif methodname_without_prefix == 'destroy':
				has_destroy_method = True
		refcountable = False
		destroyable = False
		if has_ref_method and has_unref_method:
			refcountable = True
		if has_destroy_method:
			destroyable = True
		classNodeAttributes = {
			'name' : cclass.name,
			'cfunctionprefix' : cclass.cFunctionPrefix,
			'deprecated' : str(cclass.deprecated).lower(),
			'refcountable' : str(refcountable).lower(),
			'destroyable' : str(destroyable).lower()
		}
		# Generate the XML node for the class
		classNode = ET.SubElement(classesNode, 'class', classNodeAttributes)
		if len(cclass.events) > 0:
			eventsNode = ET.SubElement(classNode, 'events')
			eventnames = []
			for eventname in cclass.events:
				eventnames.append(eventname)
			eventnames.sort()
			for eventname in eventnames:
				self.__generateFunction(eventsNode, 'event', cclass.events[eventname])
		if len(cclass.classMethods) > 0:
			classMethodsNode = ET.SubElement(classNode, 'classmethods')
			methodnames = []
			for methodname in cclass.classMethods:
				methodnames.append(methodname)
			methodnames.sort()
			for methodname in methodnames:
				self.__generateFunction(classMethodsNode, 'classmethod', cclass.classMethods[methodname])
		if len(cclass.instanceMethods) > 0:
			instanceMethodsNode = ET.SubElement(classNode, 'instancemethods')
			methodnames = []
			for methodname in cclass.instanceMethods:
				methodnames.append(methodname)
			methodnames.sort()
			for methodname in methodnames:
				self.__generateFunction(instanceMethodsNode, 'instancemethod', cclass.instanceMethods[methodname])
		if len(cclass.properties) > 0:
			propertiesNode = ET.SubElement(classNode, 'properties')
			propnames = []
			for propname in cclass.properties:
				propnames.append(propname)
			propnames.sort()
			for propname in propnames:
				propertyNodeAttributes = { 'name' : propname }
				propertyNode = ET.SubElement(propertiesNode, 'property', propertyNodeAttributes)
				if cclass.properties[propname].getter is not None:
					self.__generateFunction(propertyNode, 'getter', cclass.properties[propname].getter)
				if cclass.properties[propname].setter is not None:
					self.__generateFunction(propertyNode, 'setter', cclass.properties[propname].setter)
		if cclass.briefDescription != '':
			classBriefDescriptionNode = ET.SubElement(classNode, 'briefdescription')
			classBriefDescriptionNode.text = cclass.briefDescription
		classNode.append(cclass.detailedDescription)

	def generate(self, project):
		print("Generating XML document of Linphone API to '" + self.__outputfile.name + "'")
		apiNode = ET.Element('api')
		project.enums.sort(key = lambda e: e.name)
		if len(project.enums) > 0:
			enumsNode = ET.SubElement(apiNode, 'enums')
			for cenum in project.enums:
				self.__generateEnum(cenum, enumsNode)
		if len(project.classes) > 0:
			classesNode = ET.SubElement(apiNode, 'classes')
			project.classes.sort(key = lambda c: c.name)
			for cclass in project.classes:
				self.__generateClass(cclass, classesNode)
		s = '<?xml version="1.0" encoding="UTF-8" ?>\n'
		s += ET.tostring(apiNode, 'utf-8')
		if project.prettyPrint:
			s = minidom.parseString(s).toprettyxml(indent='\t')
		self.__outputfile.write(s)



def main(argv = None):
	if argv is None:
		argv = sys.argv
	argparser = argparse.ArgumentParser(description="Generate XML version of the Linphone API.")
	argparser.add_argument('-o', '--outputfile', metavar='outputfile', type=argparse.FileType('w'), help="Output XML file describing the Linphone API.")
	argparser.add_argument('--verbose', help="Increase output verbosity", action='store_true')
	argparser.add_argument('--pretty', help="XML pretty print", action='store_true')
	argparser.add_argument('xmldir', help="XML directory generated by doxygen.")
	args = argparser.parse_args()
	if args.outputfile == None:
		args.outputfile = open('api.xml', 'w')
	project = Project()
	if args.verbose:
		project.verbose = True
	if args.pretty:
		project.prettyPrint = True
	project.initFromDir(args.xmldir)
	project.check()
	gen = Generator(args.outputfile)
	gen.generate(project)

if __name__ == "__main__":
	sys.exit(main())
