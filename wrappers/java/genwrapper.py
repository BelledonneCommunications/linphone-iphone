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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

import argparse
import os
import sys
import pystache
import errno

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
print(sys.path)
import genapixml as CApi
import abstractapi as AbsApi
import metadoc

##########################################################################

ENUMS_LIST = {
    'AccountCreatorActivationCodeStatus': 'AccountCreator',
    'AccountCreatorDomainStatus': 'AccountCreator',
    'AccountCreatorEmailStatus': 'AccountCreator',
    'AccountCreatorLanguageStatus': 'AccountCreator',
    'AccountCreatorPasswordStatus': 'AccountCreator',
    'AccountCreatorPhoneNumberStatus': 'AccountCreator',
    'AccountCreatorStatus': 'AccountCreator',
    'AccountCreatorTransportStatus': 'AccountCreator',
    'AccountCreatorUsernameStatus': 'AccountCreator',
    'AddressFamily': 'CallStats',
    'CallDir': 'Call',
    'CallState': 'Call',
    'CallStatus': 'CallLog',
    'ChatMessageState': 'ChatMessage',
    'ConfiguringState': 'Core',
    'CoreLogCollectionUploadState': 'Core',
    'GlobalState': 'Core',
    'FriendListStatus': 'FriendList',
    'IceState': 'CallStats',
    'LimeState': 'Core',
    'MediaDirection': 'Core',
    'MediaEncryption': 'Core',
    'PlayerState': 'Player',
    'RegistrationState': 'Core',
    'SubscribePolicy': 'Friend',
    'TransportType': 'Address',
    'XmlRpcArgType': 'XmlRpcRequest',
    'XmlRpcStatus': 'XmlRpcRequest',
}

##########################################################################

class JavaTranslator(object):
    def __init__(self, packageName):
        package_dirs = packageName.split('.')
        self.jni_package = ''
        for directory in package_dirs:
            self.jni_package += directory + '_'

        self.docTranslator = metadoc.SandcastleJavaTranslator()

    def throws_exception(self, _type):
        if type(_type) is AbsApi.BaseType:
            if _type.name == 'status':
                return True
        return False

    def translate_argument_name(self, _argName):
        return _argName.to_snake_case()

    def translate_type(self, _type, native=False, jni=False):
        if type(_type) is AbsApi.ListType:
            ptrtype = ''
            if type(_type.containedTypeDesc) is AbsApi.ClassType:
                ptrtype = self.translate_type(_type.containedTypeDesc, native)
            elif type(_type.containedTypeDesc) is AbsApi.BaseType:
                ptrtype = self.translate_type(_type.containedTypeDesc, native)
            elif type(_type.containedTypeDesc) is AbsApi.EnumType:
                ptrtype = self.translate_type(_type.containedTypeDesc, native)
            else:
                if _type.containedTypeDesc:
                    raise AbsApi.Error('translation of bctbx_list_t of ' + _type.containedTypeDesc.name)
                else:
                    raise AbsApi.Error('translation of bctbx_list_t of unknow type !')
            return ptrtype + '[]'
                
        elif type(_type) is AbsApi.ClassType:
            if native:
                return 'Object'
            elif jni:
                return 'jobject'
            return _type.desc.name.to_camel_case()
        elif type(_type) is AbsApi.EnumType:
            if native:
                return 'int'
            elif jni:
                return 'jint'
            name = _type.desc.name.to_camel_case()
            if name in ENUMS_LIST:
                className = ENUMS_LIST[name]
                if name.startswith(className):
                    name = name[len(className):]
                name = className + '.' + name
            return name
        elif type(_type) is AbsApi.BaseType:
            if _type.name == 'string':
                if jni:
                    return 'jstring'
                return 'String'
            elif _type.name == 'integer':
                if jni:
                    return 'jint'
                return 'int'
            elif _type.name == 'floatant':
                if jni:
                    return 'jfloat'
                return 'float'
            elif _type.name == 'size':
                if jni:
                    return 'jint'
                return 'int'
            elif _type.name == 'time':
                if jni:
                    return 'jlong'
                return 'long'
            elif _type.name == 'status':
                if jni:
                    return 'jint'
                if native:
                    return 'int'
                return 'void'
            return _type.name

    def translate_argument(self, _arg, native=False, jni=False):
        return '{0} {1}'.format(self.translate_type(_arg.type, native, jni), self.translate_argument_name(_arg.name))

    def translate_property(self, _property):
        properties = []
        if _property.getter is not None:
            properties.append(self.translate_method(_property.getter))
        if _property.setter is not None:
            properties.append(self.translate_method(_property.setter))
        return properties

    def translate_jni_property(self, _property):
        properties = []
        if _property.getter is not None:
            properties.append(self.translate_jni_method(_property.getter))
        if _property.setter is not None:
            properties.append(self.translate_jni_method(_property.setter))
        return properties

    def translate_method(self, _method):
        methodDict = {}

        methodDict['return'] = self.translate_type(_method.returnType)
        methodDict['return_native'] = self.translate_type(_method.returnType, True)
        methodDict['return_keyword'] = '' if methodDict['return'] == 'void' else 'return '

        methodDict['convertInputClassArrayToLongArray'] = False
        methodDict['convertOutputClassArrayToLongArray'] = type(_method.returnType) is AbsApi.ListType and type(_method.returnType.containedTypeDesc) is AbsApi.ClassType
        if methodDict['convertOutputClassArrayToLongArray']:
            methodDict['native_params_impl_list_param_name'] = 'classArray'
            methodDict['native_params_impl_list_param_type'] = self.translate_type(_method.returnType.containedTypeDesc)

        methodDict['name'] = _method.name.to_camel_case(lower=True)
        methodDict['exception'] = self.throws_exception(_method.returnType)

        methodDict['enumCast'] = type(_method.returnType) is AbsApi.EnumType
        methodDict['classCast'] = type(_method.returnType) is AbsApi.ClassType
        methodDict['params'] = ''
        methodDict['native_params'] = 'long nativePtr'
        methodDict['static_native_params'] = ''
        methodDict['native_params_impl'] = ''
        for arg in _method.args:
            if arg is not _method.args[0]:
                methodDict['params'] += ', '
            methodDict['native_params'] += ', '
            methodDict['native_params_impl'] += ', '

            methodDict['params'] += self.translate_argument(arg)
            methodDict['native_params'] += self.translate_argument(arg, True)
            methodDict['static_native_params'] += self.translate_argument(arg, True)
            if type(arg.type) is AbsApi.ClassType:
                methodDict['native_params_impl'] += '((' + self.translate_type(arg.type) + 'Impl)' + self.translate_argument_name(arg.name) + ').nativePtr'
            elif type(arg.type) is AbsApi.ListType:
                if type(arg.type.containedTypeDesc) is AbsApi.ClassType:
                    methodDict['convertInputClassArrayToLongArray'] = True
                    methodDict['native_params_impl_list_param_name'] = self.translate_argument_name(arg.name)
                    methodDict['native_params_impl_list_param_type'] = self.translate_type(arg.type.containedTypeDesc) + 'Impl'
                    methodDict['native_params_impl'] += 'longArray'
                else:
                    methodDict['native_params_impl'] += self.translate_argument_name(arg.name)
            elif type(arg.type) is AbsApi.EnumType:
                methodDict['native_params_impl'] += self.translate_argument_name(arg.name) + '.toInt()'
            else:
                methodDict['native_params_impl'] += self.translate_argument_name(arg.name)

        methodDict['classicMethod'] = not methodDict['convertInputClassArrayToLongArray'] and not methodDict['convertOutputClassArrayToLongArray']
        methodDict['deprecated'] = _method.deprecated
        methodDict['doc'] = self.docTranslator.translate(_method.briefDescription) if _method.briefDescription is not None else None

        return methodDict

    def translate_jni_method(self, _method):
        methodDict = {}

        methodDict['return'] = self.translate_type(_method.returnType, jni=True)
        methodDict['name'] = 'Java_' + self.jni_package + _method.parent.name.to_camel_case() + 'Impl_' + _method.name.to_camel_case(lower=True)

        methodDict['params'] = 'JNIEnv *env, jobject thiz'
        for arg in _method.args:
            if arg is not _method.args[0]:
                methodDict['params'] += ', '
            methodDict['params'] += self.translate_argument(arg, jni=True)

        return methodDict

    def translate_class(self, _class):
        classDict = {
            'methods'             : [],
            'staticMethods'       : [],
            'jniMethods'          : [],
        }

        classDict['isLinphoneFactory'] = _class.name.to_camel_case() == "Factory"
        classDict['doc'] = self.docTranslator.translate(_class.briefDescription) if _class.briefDescription is not None else None

        for _property in _class.properties:
            try:
                classDict['methods'] += self.translate_property(_property)
                classDict['jniMethods'] += self.translate_jni_property(_property)
            except AbsApi.Error as e:
                print('error while translating {0} property: {1}'.format(_property.name.to_snake_case(), e.args[0]))

        for method in _class.instanceMethods:
            try:
                methodDict = self.translate_method(method)
                jniMethodDict = self.translate_jni_method(method)
                classDict['methods'].append(methodDict)
                classDict['jniMethods'].append(jniMethodDict)
            except AbsApi.Error as e:
                print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

        for method in _class.classMethods:
            try:
                methodDict = self.translate_method(method)
                jniMethodDict = self.translate_jni_method(method)
                classDict['staticMethods'].append(methodDict)
                classDict['jniMethods'].append(jniMethodDict)
            except AbsApi.Error as e:
                print('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

        return classDict

    def translate_interface(self, _class):
        interfaceDict = {
            'methods'             : [],
        }

        interfaceDict['doc'] = self.docTranslator.translate(_class.briefDescription)

        for method in _class.methods:
            interfaceDict['methods'].append(self.translate_method(method))

        return interfaceDict

    def translate_enum(self, _class):
        enumDict = {}

        enumDict['name'] = _class.name.to_camel_case()
        enumDict['doc'] = self.docTranslator.translate(_class.briefDescription)
        enumDict['values'] = []
        i = 0
        lastValue = None

        for enumValue in _class.values:
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
            i += 1
            enumValDict['commarorsemicolon'] = ';' if i == len(_class.values) else ','
            enumDict['values'].append(enumValDict)

        return enumDict

##########################################################################

class JavaEnum(object):
    def __init__(self, package, _enum, translator):
        self._class = translator.translate_enum(_enum)
        self.packageName = package
        self.className = _enum.name.to_camel_case()
        self.filename = self.className + ".java"
        self.values = self._class['values']
        self.doc = self._class['doc']
        self.jniMethods = []

class JavaInterface(object):
    def __init__(self, package, _interface, translator):
        self._class = translator.translate_interface(_interface)
        self.packageName = package
        self.className = _interface.name.to_camel_case()
        self.filename = self.className + ".java"
        self.imports = []
        self.methods = self._class['methods']
        self.doc = self._class['doc']
        self.jniMethods = []

class JavaInterfaceStub(object):
    def __init__(self, _interface):
        self.packageName = _interface.packageName
        self.className = _interface.className
        self.classNameStub =  self.className + "Stub"
        self.filename = self.className + "Stub.java"
        self.methods = _interface.methods

class JavaClass(object):
    def __init__(self, package, _class, translator):
        self._class = translator.translate_class(_class)
        self.isLinphoneFactory = self._class['isLinphoneFactory']
        self.packageName = package
        self.className = _class.name.to_camel_case()
        self.classImplName = self.className + "Impl"
        self.filename = self.className + ".java"
        self.imports = []
        self.methods = self._class['methods']
        self.staticMethods = self._class['staticMethods']
        self.jniMethods = self._class['jniMethods']
        self.doc = self._class['doc']
        self.enums = []

    def add_enum(self, enum):
        if enum.className.startswith(self.className):
            enum.className = enum.className[len(self.className):]
        self.enums.append(enum)

class Jni(object):
    def __init__(self):
        self.methods = {}

    def add_methods(self, name, methods):
        self.methods[name] = methods

##########################################################################

class GenWrapper(object):
    def __init__(self, srcdir, javadir, package, xmldir):
        self.srcdir = srcdir
        self.javadir = javadir
        self.package = package

        project = CApi.Project()
        project.initFromDir(xmldir)
        project.check()

        self.parser = AbsApi.CParser(project)
        self.parser.parse_all()
        self.translator = JavaTranslator(package)
        self.renderer = pystache.Renderer()
        self.jni = Jni()

        self.enums = {}
        self.interfaces = {}
        self.classes = {}
        self.enums_to_remove = []

    def render_all(self):
        for _interface in self.parser.interfacesIndex.values():
            self.render_java_interface(_interface)
        for _class in self.parser.classesIndex.values():
            self.render_java_class(_class)
        for _enum in self.parser.enumsIndex.items():
            if _enum[1] is not None:
                self.render_java_enum(_enum[1])

        for name, value in self.enums.iteritems():
            if name in ENUMS_LIST:
                className = ENUMS_LIST[name]
                print 'Enum ' + name + ' belongs to class ' + className
                self.classes[className].add_enum(value)
                self.enums_to_remove.append(name)

        for enum in self.enums_to_remove:
            self.enums.pop(enum, None)

        for name, value in self.enums.iteritems():
            self.render(value, self.javadir + '/' + value.filename)
        for name, value in self.interfaces.iteritems():
            self.render(value, self.javadir + '/' + value.filename)
        for name, value in self.classes.iteritems():
            self.render(value, self.javadir + '/' + value.filename)

        self.render(self.jni, self.srcdir + '/linphone_jni.cc')

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

    def render_java_enum(self, _class):
        if _class is not None:
            try:
                javaenum = JavaEnum(self.package, _class, self.translator)
                self.enums[javaenum.className] = javaenum
            except AbsApi.Error as e:
                print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
            self.jni.add_methods(javaenum.className, javaenum.jniMethods)

    def render_java_interface(self, _class):
        if _class is not None:
            try:
                javainterface = JavaInterface(self.package, _class, self.translator)
                self.interfaces[javainterface.className] = javainterface
                javaInterfaceStub = JavaInterfaceStub(javainterface)
                self.interfaces[javaInterfaceStub.classNameStub] = javaInterfaceStub
            except AbsApi.Error as e:
                print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
            self.jni.add_methods(javainterface.className, javainterface.jniMethods)

    def render_java_class(self, _class):
        if _class is not None:
            try:
                javaclass = JavaClass(self.package, _class, self.translator)
                self.classes[javaclass.className] = javaclass
            except AbsApi.Error as e:
                print('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
            self.jni.add_methods(javaclass.className, javaclass.jniMethods)

##########################################################################

def main():
    argparser = argparse.ArgumentParser(description='Generate source files for the Java wrapper')
    argparser.add_argument('xmldir', type=str, help='Directory where the XML documentation of the Linphone\'s API generated by Doxygen is placed')
    argparser.add_argument('-o --output', type=str, help='the directory where to generate the source files', dest='outputdir', default='.')
    argparser.add_argument('-p --package', type=str, help='the package name for the wrapper', dest='package', default='org.linphone')
    argparser.add_argument('-n --name', type=str, help='the name of the genarated source file', dest='name', default='linphone_jni.cc')
    args = argparser.parse_args()

    srcdir = args.outputdir + '/src'
    javadir = args.outputdir + '/java'
    package_dirs = args.package.split('.')
    for directory in package_dirs:
        javadir += '/' + directory

    try:
        os.makedirs(srcdir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            print("Cannot create '{0}' dircetory: {1}".format(srcdir, e.strerror))
            sys.exit(1)

    try:
        os.makedirs(javadir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            print("Cannot create '{0}' dircetory: {1}".format(javadir, e.strerror))
            sys.exit(1)

    genwrapper = GenWrapper(srcdir, javadir, args.package, args.xmldir)
    genwrapper.render_all()

if __name__ == '__main__':
    main()
