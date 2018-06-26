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
import errno
import logging
import os
import pystache
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
import genapixml as CApi
import abstractapi as AbsApi
import metadoc
import metaname


CORE_ACCESSOR_LIST = [
    'Call',
    'ChatRoom',
    'Event',
    'Friend',
    'FriendList',
    'NatPolicy',
    'Player',
    'ProxyConfig'
]


class JNINameTranslator(metaname.Translator):
    _instance = None

    @staticmethod
    def get():
        if JNINameTranslator._instance is None:
            JNINameTranslator._instance = JNINameTranslator()
        return JNINameTranslator._instance

    def translate_class_name(self, name, **params):
        translated_name = name.to_camel_case()
        if name.prev is not None and type(name.prev) is not metaname.NamespaceName:
            return name.prev.translate(self) + self._getseparator(name.prev) + translated_name
        else:
            return translated_name

    def translate_interface_name(self, name, **params):
        return self.translate_class_name(name, **params)

    def translate_enum_name(self, name, **params):
        return self.translate_class_name(name, **params)

    def translate_enumerator_name(self, name, **params):
        raise NotImplemented()

    def translate_method_name(self, name, **params):
        raise NotImplemented()

    def translate_namespace_name(self, name, **params):
        translated_name = name.to_snake_case()
        if name.prev is not None:
            return name.prev.translate(self) + self._getseparator(name.prev) + translated_name
        else:
            return translated_name

    def translate_argument_name(self, name, **params):
        raise NotImplemented()

    def translate_property_name(self, name, **params):
        raise NotImplemented()

    def _getseparator(self, previous_name):
        if isinstance(previous_name, metaname.NamespaceName):
            return '/'
        elif isinstance(previous_name, metaname.ClassName):
            return '$'
        else:
            raise TypeError("no separator for '{0}' type".format(type(previous_name)))


class JNILangTranslator(AbsApi.Translator):
    _instance = None

    @staticmethod
    def get():
        if JNILangTranslator._instance is None:
            JNILangTranslator._instance = JNILangTranslator()
        return JNILangTranslator._instance

    def translate_base_type(self, type_):
        if type_.name == 'string':
            return 'Ljava/lang/String;'
        elif type_.name == 'integer':
            return 'I'
        elif type_.name == 'boolean':
            return 'Z'
        elif type_.name == 'floatant':
            return 'F'
        elif type_.name == 'size':
            return 'I'
        elif type_.name == 'time':
            return 'I'
        elif type_.name == 'status':
            return 'I'
        elif type_.name == 'string_array':
            return '[Ljava/lang/String;'
        elif type_.name == 'character':
            return 'C'
        elif type_.name == 'void':
            return 'V'
        return type_.name


class JavaTranslator(object):
    def __init__(self, packageName, exceptions):
        self.exceptions = exceptions
        package_dirs = packageName.split('.')
        self.jni_package = ''
        self.jni_path = ''
        for directory in package_dirs:
            self.jni_package += directory + '_'
            self.jni_path += directory + '/'

        self.nameTranslator = metaname.Translator.get('Java')
        self.langTranslator = AbsApi.Translator.get('Java')
        self.docTranslator = metadoc.JavaDocTranslator()

        self.jninameTranslator = JNINameTranslator.get()
        self.jnilangTranslator = JNILangTranslator.get()

        self.clangTranslator = AbsApi.Translator.get('C')

    def throws_exception(self, _type):
        if not self.exceptions:
            return False
        if type(_type) is AbsApi.BaseType:
            if _type.name == 'status':
                return True
        return False

    def translate_property(self, _property, _hasCoreAccessor):
        properties = []
        if _property.getter is not None:
            properties.append(self.translate_method(_property.getter, _hasCoreAccessor))
        if _property.setter is not None:
            properties.append(self.translate_method(_property.setter, _hasCoreAccessor))
        return properties

    def translate_jni_property(self, class_, _property):
        properties = []
        if _property.getter is not None:
            properties.append(self.translate_jni_method(class_, _property.getter))
        if _property.setter is not None:
            properties.append(self.translate_jni_method(class_, _property.setter))
        return properties

    def generate_listener(self, name, _class):
        methodDict = {}
        methodDict['return'] = 'void'
        methodDict['return_native'] = 'void'
        methodDict['return_keyword'] = ''
        methodDict['convertInputClassArrayToLongArray'] = False
        methodDict['name'] = name
        methodDict['exception'] = False
        methodDict['enumCast'] = False
        methodDict['classCast'] = False

        methodDict['params'] = _class.name.to_camel_case() + 'Listener listener'
        methodDict['native_params'] = 'long nativePtr, ' + _class.name.to_camel_case() + 'Listener listener'
        methodDict['static_native_params'] = ''
        methodDict['native_params_impl'] = 'nativePtr, listener'

        methodDict['deprecated'] = False
        methodDict['doc'] = None

        return methodDict

    def generate_add_listener(self, _class):
        return self.generate_listener('addListener', _class)

    def generate_remove_listener(self, _class):
        return self.generate_listener('removeListener', _class)

    def generate_set_listener(self, _class):
        return self.generate_listener('setListener', _class)

    def translate_method(self, _method, _hasCoreAccessor=False):
        methodDict = {}

        namespace = _method.find_first_ancestor_by_type(AbsApi.Namespace)

        methodDict['return'] = _method.returnType.translate(self.langTranslator, isReturn=True, namespace=namespace)
        methodDict['return_native'] = _method.returnType.translate(self.langTranslator, native=True, isReturn=True, namespace=namespace)
        methodDict['return_keyword'] = '' if methodDict['return'] == 'void' else 'return '
        methodDict['hasReturn'] = not methodDict['return'] == 'void'

        methodDict['convertInputClassArrayToLongArray'] = False

        methodDict['name'] = _method.name.to_camel_case(lower=True)
        methodDict['isNotGetCore'] = not methodDict['name'] == 'getCore'
        methodDict['hasCoreAccessor'] = _hasCoreAccessor
        methodDict['exception'] = self.throws_exception(_method.returnType)

        methodDict['enumCast'] = type(_method.returnType) is AbsApi.EnumType
        methodDict['classCast'] = type(_method.returnType) is AbsApi.ClassType

        methodDict['params'] = ', '.join([arg.translate(self.langTranslator, namespace=namespace) for arg in _method.args])
        methodDict['native_params'] = ', '.join(['long nativePtr'] + [arg.translate(self.langTranslator, native=True, namespace=namespace) for arg in _method.args])
        methodDict['static_native_params'] = ', '.join([arg.translate(self.langTranslator, native=True, namespace=namespace) for arg in _method.args])
        methodDict['native_params_impl'] = ', '.join(
            ['nativePtr'] + [arg.name.translate(self.nameTranslator) + ('.toInt()' if type(arg.type) is AbsApi.EnumType else '') for arg in _method.args])

        methodDict['deprecated'] = _method.deprecated
        methodDict['doc'] = _method.briefDescription.translate(self.docTranslator) if _method.briefDescription is not None else None

        return methodDict

    def translate_jni_method(self, class_, _method, static=False):
        jni_blacklist = ['linphone_call_set_native_video_window_id',
                        'linphone_core_set_native_preview_window_id',
                        'linphone_core_set_native_video_window_id']

        namespace = class_.find_first_ancestor_by_type(AbsApi.Namespace)
        className = class_.name.translate(self.nameTranslator)

        methodDict = {'notEmpty': True}
        methodDict['classCName'] = class_.name.to_c()
        methodDict['className'] = className
        methodDict['classImplName'] = className + 'Impl'
        methodDict['isLinphoneFactory'] = (className == 'Factory')
        methodDict['jniPath'] = self.jni_path

        methodDict['return'] = _method.returnType.translate(self.langTranslator, jni=True, isReturn=True, namespace=namespace)
        methodDict['hasListReturn'] = methodDict['return'] == 'jobjectArray'
        methodDict['hasByteArrayReturn'] = methodDict['return'] == 'jbyteArray'
        methodDict['hasReturn'] = not methodDict['return'] == 'void' and not methodDict['hasListReturn'] and not methodDict['hasByteArrayReturn']
        methodDict['hasStringReturn'] = methodDict['return'] == 'jstring'
        methodDict['hasNormalReturn'] = not methodDict['hasListReturn'] and not methodDict['hasStringReturn'] and not methodDict['hasByteArrayReturn']
        methodDict['name'] = 'Java_' + self.jni_package + className + 'Impl_' + _method.name.translate(self.nameTranslator)
        methodDict['notStatic'] = not static

        if _method.name.to_c() == 'linphone_factory_create_core':
            methodDict['c_name'] = 'linphone_factory_create_core_3'
        elif _method.name.to_c() == 'linphone_factory_create_core_with_config':
            methodDict['c_name'] = 'linphone_factory_create_core_with_config_3'
        else:
            methodDict['c_name'] = _method.name.to_c()

        methodDict['returnObject'] = methodDict['hasReturn'] and type(_method.returnType) is AbsApi.ClassType
        methodDict['returnClassName'] = _method.returnType.translate(self.langTranslator, namespace=namespace)
        methodDict['isRealObjectArray'] = False
        methodDict['isStringObjectArray'] = False
        methodDict['c_type_return'] = _method.returnType.translate(self.clangTranslator)

        if methodDict['c_name'] in jni_blacklist:
            return {'notEmpty': False}

        if methodDict['hasListReturn']:
            if type(_method.returnType) is AbsApi.BaseType and _method.returnType.name == 'string_array':
                methodDict['isStringObjectArray'] = True
            elif type(_method.returnType.containedTypeDesc) is AbsApi.BaseType:
                methodDict['isStringObjectArray'] = True
            elif type(_method.returnType.containedTypeDesc) is AbsApi.ClassType:
                methodDict['isRealObjectArray'] = True
                methodDict['objectCPrefix'] = 'linphone_' + _method.returnType.containedTypeDesc.desc.name.to_snake_case()
                methodDict['objectClassCName'] = 'Linphone' + _method.returnType.containedTypeDesc.desc.name.to_camel_case()
                methodDict['objectClassName'] = _method.returnType.containedTypeDesc.desc.name.to_camel_case()
                methodDict['objectClassImplName'] = _method.returnType.containedTypeDesc.desc.name.to_camel_case() + 'Impl'

        methodDict['params'] = 'JNIEnv *env, jobject thiz, jlong ptr'
        methodDict['params_impl'] = ''
        methodDict['strings'] = []
        methodDict['objects'] = []
        methodDict['lists'] = []
        methodDict['array'] = []
        methodDict['bytes'] = []
        methodDict['returnedObjectGetter'] = ''
        for arg in _method.args:
            methodDict['params'] += ', '
            if static:
                if arg is not _method.args[0]:
                    methodDict['params_impl'] += ', '
            else:
                methodDict['params_impl'] += ', '

            methodDict['params'] += arg.translate(self.langTranslator, jni=True, namespace=namespace)
            argname = arg.name.translate(self.nameTranslator)

            if type(arg.type) is AbsApi.ClassType:
                classCName = 'Linphone' + arg.type.desc.name.to_camel_case()
                if classCName[-8:] == 'Listener':
                   classCName = 'Linphone' + arg.type.desc.name.to_camel_case()[:-8] + 'Cbs'
                methodDict['objects'].append({'object': argname, 'objectClassCName': classCName})
                methodDict['params_impl'] += 'c_' + argname

            elif type(arg.type) is AbsApi.ListType:
                isStringList = type(arg.type.containedTypeDesc) is AbsApi.BaseType and arg.type.containedTypeDesc.name == 'string'
                isObjList = type(arg.type.containedTypeDesc) is AbsApi.ClassType
                methodDict['lists'].append({'list': argname, 'isStringList': isStringList, 'isObjList': isObjList, 'objectClassCName': arg.type.containedTypeDesc.name})
                methodDict['params_impl'] += 'bctbx_list_' + argname

            elif type(arg.type) is AbsApi.EnumType:
                argCType = arg.type.name
                methodDict['params_impl'] += '(' + argCType + ') ' + argname

            elif type(arg.type) is AbsApi.BaseType:
                if arg.type.name == 'integer' and arg.type.size is not None and arg.type.isref:
                    methodDict['bytes'].append({'bytesargname': argname, 'bytesargtype' : arg.type.translate(self.clangTranslator)})
                    methodDict['params_impl'] += 'c_' + argname
                elif arg.type.name == 'string':
                    methodDict['strings'].append({'string': argname})
                    methodDict['params_impl'] += 'c_' + argname
                else:
                    methodDict['params_impl'] += '(' + arg.type.translate(self.clangTranslator) + ')' + argname
            else:
                methodDict['params_impl'] += argname

        return methodDict

    def translate_class(self, _class):
        classDict = {
            'methods': [],
            'jniMethods': [],
        }

        classDict['isLinphoneFactory'] = _class.name.to_camel_case() == "Factory"
        classDict['isLinphoneCore'] = _class.name.to_camel_case() == "Core"
        hasCoreAccessor = _class.name.to_camel_case() in CORE_ACCESSOR_LIST
        classDict['hasCoreAccessor'] = hasCoreAccessor
        classDict['doc'] = _class.briefDescription.translate(self.docTranslator) if _class.briefDescription is not None else None
        classDict['refCountable'] = _class.refcountable

        for _property in _class.properties:
            try:
                classDict['methods'] += self.translate_property(_property, hasCoreAccessor)
                classDict['jniMethods'] += self.translate_jni_property(_class, _property)
            except AbsApi.Error as e:
                logging.error('error while translating {0} property: {1}'.format(_property.name.to_snake_case(), e.args[0]))

        for method in _class.instanceMethods:
            try:
                methodDict = self.translate_method(method, hasCoreAccessor)
                jniMethodDict = self.translate_jni_method(_class, method)
                classDict['methods'].append(methodDict)
                classDict['jniMethods'].append(jniMethodDict)
            except AbsApi.Error as e:
                logging.error('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

        for method in _class.classMethods:
            try:
                methodDict = self.translate_method(method, hasCoreAccessor)
                jniMethodDict = self.translate_jni_method(_class, method, True)
                classDict['methods'].append(methodDict)
                classDict['jniMethods'].append(jniMethodDict)
            except AbsApi.Error as e:
                logging.error('Could not translate {0}: {1}'.format(method.name.to_snake_case(fullName=True), e.args[0]))

        islistenable = _class.listenerInterface is not None
        if islistenable:
            isMultiListener = (_class.multilistener)
            if isMultiListener:
                classDict['methods'].append(self.generate_add_listener(_class))
                classDict['methods'].append(self.generate_remove_listener(_class))
            else:
                classDict['methods'].append(self.generate_set_listener(_class))

        return classDict

    def translate_jni_interface(self, _class, className, _method):
        methodDict = {}
        methodDict['classCName'] = className.to_c()
        methodDict['className'] = className.translate(self.nameTranslator)
        methodDict['classImplName'] = methodDict['className'] + 'Impl'
        methodDict['jniPath'] = self.jni_path
        methodDict['cPrefix'] = _class.name.to_snake_case(fullName=True)
        methodDict['callbackName'] = '_{0}_cb'.format(_method.name.to_snake_case(fullName=True))
        methodDict['jname'] = _method.name.translate(self.nameTranslator)
        methodDict['return'] = _method.returnType.translate(self.clangTranslator)
        methodDict['jniUpcallMethod'] = 'CallVoidMethod'
        methodDict['isJniUpcallBasicType'] = False
        methodDict['isJniUpcallObject'] = False
        if type(_method.returnType) is AbsApi.ClassType:
            methodDict['jniUpcallMethod'] = 'CallObjectMethod'
            methodDict['isJniUpcallObject'] = True
            methodDict['jniUpcallType'] = 'jobject'
        elif type(_method.returnType) is AbsApi.BaseType:
            if not _method.returnType.name == 'void':
                methodDict['jniUpcallMethod'] = 'CallIntMethod'
                methodDict['jniUpcallType'] = _method.returnType.translate(self.langTranslator, jni=True)
                methodDict['isJniUpcallBasicType'] = True
        methodDict['returnIfFail'] = '' if  methodDict['return'] == 'void' else ' NULL'
        methodDict['hasReturn'] = not methodDict['return'] == 'void'
        methodDict['isSingleListener'] = not _class.multilistener
        methodDict['isMultiListener'] = _class.multilistener

        methodDict['firstParam'] = ''
        methodDict['jobjects'] = []
        methodDict['jenums'] = []
        methodDict['jstrings'] = []
        methodDict['params'] = ''
        methodDict['jparams'] = '('
        methodDict['params_impl'] = ''
        for arg in _method.args:
            argname = arg.name.translate(self.nameTranslator)
            if arg is not _method.args[0]:
                methodDict['params'] += ', '
                methodDict['params_impl'] += ', '
            else:
                methodDict['firstParam'] = argname

            methodDict['params'] += '{0} {1}'.format(arg.type.translate(self.clangTranslator), argname)

            if type(arg.type) is AbsApi.ClassType:
                methodDict['jparams'] += 'L' + self.jni_path + arg.type.desc.name.to_camel_case() + ';'
                methodDict['params_impl'] += 'j_' + argname
                methodDict['jobjects'].append({'objectName': argname, 'className': arg.type.desc.name.to_camel_case(), })
            elif type(arg.type) is AbsApi.BaseType:
                methodDict['jparams'] += arg.type.translate(self.jnilangTranslator)
                if arg.type.name == 'string':
                    methodDict['params_impl'] += 'j_' + argname
                    methodDict['jstrings'].append({'stringName': argname,})
                else:
                    methodDict['params_impl'] += argname
            elif type(arg.type) is AbsApi.EnumType:
                methodDict['jparams'] += 'L' + self.jni_path + arg.type.desc.name.translate(self.jninameTranslator) + ';'
                methodDict['params_impl'] += 'j_' + argname
                methodDict['jenums'].append({'enumName': argname, 'cEnumPrefix': arg.type.desc.name.to_snake_case(fullName=True)})
            elif type(arg.type) is AbsApi.ListType:
                methodDict['jparams'] += '[L' + self.jni_path + arg.type.containedTypeDesc.name + ';'
                methodDict['params_impl'] += 'NULL'

        methodDict['jparams'] += ')'
        if (methodDict['return'] == 'void'):
            methodDict['jparams'] += 'V'
        else:
            if type(_method.returnType) is AbsApi.ClassType:
                methodDict['jparams'] += 'L' + self.jni_path + _method.returnType.desc.name.to_camel_case() + ';'
            elif type(_method.returnType) is AbsApi.BaseType:
                methodDict['jparams'] += self.translate_java_jni_base_type_name(_method.returnType.name)
            else:
                pass #TODO

        return methodDict

    def translate_interface(self, _class):
        interfaceDict = {
            'methods': [],
            'jniMethods': [],
        }

        interfaceDict['doc'] = _class.briefDescription.translate(self.docTranslator)

        for method in _class.instanceMethods:
            interfaceDict['methods'].append(self.translate_method(method))
            interfaceDict['jniMethods'].append(self.translate_jni_interface(_class.listenedClass, _class.name, method))

        return interfaceDict

    def translate_enum(self, enum):
        enumDict = {
            'jniMethods': [],
        }

        enumDict['name'] = enum.name.to_camel_case()
        enumDict['doc'] = enum.briefDescription.translate(self.docTranslator)
        enumDict['values'] = []
        i = 0
        lastValue = None

        enumDict['jniPath'] = self.jni_path

        for enumerator in enum.enumerators:
            enumValDict = {}
            enumValDict['name'] = enumerator.name.to_camel_case()
            enumValDict['doc'] = enumerator.briefDescription.translate(self.docTranslator)
            if isinstance(enumerator.value, int):
                lastValue = enumerator.value
                enumValDict['value'] = str(enumerator.value)
            elif isinstance(enumerator.value, AbsApi.Flag):
                enumValDict['value'] = '1<<' + str(enumerator.value.position)
            else:
                if lastValue is not None:
                    enumValDict['value'] = lastValue + 1
                    lastValue += 1
                else:
                    enumValDict['value'] = i
            i += 1
            enumValDict['commarorsemicolon'] = ';' if i == len(enum.enumerators) else ','
            enumDict['values'].append(enumValDict)

        return enumDict

##########################################################################

class JavaEnum(object):
    def __init__(self, package, _enum, translator):
        javaNameTranslator = metaname.Translator.get('Java')
        self._class = translator.translate_enum(_enum)
        self.packageName = package
        self.className = _enum.name.translate(javaNameTranslator)
        self.cPrefix = _enum.name.to_snake_case(fullName=True)
        self.filename = self.className + ".java"
        self.values = self._class['values']
        self.doc = self._class['doc']
        self.jniName = _enum.name.translate(JNINameTranslator.get())

class JniInterface(object):
    def __init__(self, javaClass, apiClass):
        self.isSingleListener = (not apiClass.multilistener)
        self.isMultiListener = (apiClass.multilistener)
        self.className = javaClass.className
        self.classCName = javaClass.cName
        self.cPrefix = javaClass.cPrefix
        self.callbacks = []
        listener = apiClass.listenerInterface
        for method in listener.instanceMethods:
            self.callbacks.append({
                'callbackName': '_{0}_cb'.format(method.name.to_snake_case(fullName=True)),
                'callback': method.name.to_snake_case()[3:], # Remove the on_
            })

class JavaInterface(object):
    def __init__(self, package, _interface, translator):
        self._class = translator.translate_interface(_interface)
        self.packageName = package
        self.className = _interface.name.to_camel_case()
        self.filename = self.className + ".java"
        self.cPrefix = 'linphone_' + _interface.name.to_snake_case()
        self.imports = []
        self.methods = self._class['methods']
        self.doc = self._class['doc']
        self.jniMethods = self._class['jniMethods']

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
        self.isLinphoneCore = self._class['isLinphoneCore']
        self.isNotLinphoneFactory = not self.isLinphoneFactory
        self.cName = _class.name.to_c()
        self.hasCoreAccessor = self._class['hasCoreAccessor']
        self.cPrefix = _class.name.to_snake_case(fullName=True)
        self.packageName = package
        self.className = _class.name.to_camel_case()
        self.classImplName = self.className + "Impl"
        self.refCountable = self._class['refCountable']
        self.factoryName = _class.name.to_snake_case()
        self.filename = self.className + ".java"
        self.imports = []
        self.methods = self._class['methods']
        self.jniMethods = self._class['jniMethods']
        self.doc = self._class['doc']
        self.enums = []
        for enum in _class.enums:
            self.enums.append(JavaEnum(package, enum, translator))
        self.jniInterface = None
        if _class.listenerInterface is not None:
            self.jniInterface = JniInterface(self, _class)

    def add_enum(self, enum):
        if enum.className.startswith(self.className):
            enum.className = enum.className[len(self.className):]
        self.enums.append(enum)

class Jni(object):
    def __init__(self, package):
        self.enums = []
        self.interfaces = []
        self.callbacks = []
        self.objects = []
        self.methods = []
        self.jni_package = ''
        self.jni_path = ''
        self.coreListener = []
        package_dirs = package.split('.')
        for directory in package_dirs:
            self.jni_package += directory + '_'
            self.jni_path += directory + '/'

    def add_enum(self, javaEnum):
        obj = {
            'jniPrefix': self.jni_package,
            'jniPath': self.jni_path,
            'jniName': javaEnum.jniName,
            'cPrefix': javaEnum.cPrefix,
            'className': javaEnum.className,
        }
        self.enums.append(obj)

    def add_object(self, javaClass):
        if javaClass.className == 'Factory':
            return
        obj = {
            'jniPrefix': self.jni_package,
            'jniPath': self.jni_path,
            'cPrefix': javaClass.cPrefix,
            'className': javaClass.className,
            'classCName': javaClass.cName,
            'classImplName': javaClass.classImplName,
            'refCountable': javaClass.refCountable
        }
        self.objects.append(obj)

        jniInterface = javaClass.jniInterface
        if jniInterface is not None:
            interface = {
                'isSingleListener': jniInterface.isSingleListener,
                'isMultiListener': jniInterface.isMultiListener,
                'classCName': jniInterface.classCName,
                'className': jniInterface.className,
                'cPrefix': jniInterface.cPrefix,
                'jniPackage': self.jni_package,
                'factoryName': javaClass.factoryName,
                'callbacksList': []
            }
            for callback in jniInterface.callbacks:
                interface['callbacksList'].append(callback)
                if obj['className'] == 'Core':
                    self.coreListener.append(callback)
            self.interfaces.append(interface)

    def add_callbacks(self, name, callbacks):
        for callback in callbacks:
            self.callbacks.append(callback)

    def add_methods(self, name, methods):
        for method in methods:
            self.methods.append(method)

class Proguard(object):
    def __init__(self, package):
        self.package = package
        self.classes = []
        self.enums = []
        self.listeners = []

    def add_class(self, javaClass):
        obj = {
            'package': self.package,
            'className': javaClass.className,
            'classImplName': javaClass.classImplName,
        }
        self.classes.append(obj)

        for javaEnum in javaClass.enums:
            enumObj = {
                'package': self.package,
                'className': javaClass.className + "$" + javaEnum.className,
            }
            self.enums.append(enumObj)

    def add_enum(self, javaEnum):
        obj = {
            'package': self.package,
            'className': javaEnum.className,
        }
        self.enums.append(obj)

    def add_interface(self, javaInterface):
        obj = {
            'package': self.package,
            'className': javaInterface.className,
        }
        self.listeners.append(obj)

##########################################################################

class GenWrapper(object):
    def __init__(self, srcdir, javadir, package, xmldir, exceptions):
        self.srcdir = srcdir
        self.javadir = javadir
        self.package = package
        self.exceptions = exceptions

        project = CApi.Project()
        project.initFromDir(xmldir)
        project.check()

        self.parser = AbsApi.CParser(project)
        self.parser.functionBl = [
            'linphone_factory_create_core_with_config',
            'linphone_factory_create_core',
            'linphone_factory_create_core_2',
            'linphone_factory_create_core_with_config_2',
            'linphone_vcard_get_belcard',
            'linphone_core_get_current_vtable',
            'linphone_factory_get',
            'linphone_factory_clean',
            'linphone_call_zoom_video',
            'linphone_core_get_zrtp_cache_db',
            'linphone_config_get_range'
        ]
        self.parser.parse_all()
        self.translator = JavaTranslator(package, exceptions)
        self.renderer = pystache.Renderer()
        self.jni = Jni(package)
        self.proguard = Proguard(package)

        self.enums = {}
        self.interfaces = {}
        self.classes = {}

    def render_all(self):
        for _interface in self.parser.namespace.interfaces:
            self.render_java_interface(_interface)
        for _class in self.parser.namespace.classes:
            self.render_java_class(_class)
        for enum in self.parser.namespace.enums:
            self.render_java_enum(enum)

        for name, value in self.enums.items():
            self.render(value, self.javadir + '/' + value.filename)
            self.proguard.add_enum(value)
        for name, value in self.interfaces.items():
            self.render(value, self.javadir + '/' + value.filename)
            self.proguard.add_interface(value)
        for name, value in self.classes.items():
            self.render(value, self.javadir + '/' + value.filename)
            self.jni.add_object(value)
            self.proguard.add_class(value)

        self.render(self.jni, self.srcdir + '/linphone_jni.cc')
        self.render(self.proguard, self.srcdir + '/proguard.txt')

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
                self.jni.add_enum(javaenum)
            except AbsApi.Error as e:
                logging.error('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))

    def render_java_interface(self, _class):
        if _class is not None:
            try:
                javainterface = JavaInterface(self.package, _class, self.translator)
                self.interfaces[javainterface.className] = javainterface
                javaInterfaceStub = JavaInterfaceStub(javainterface)
                self.interfaces[javaInterfaceStub.classNameStub] = javaInterfaceStub
            except AbsApi.Error as e:
                logging.error('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
            self.jni.add_callbacks(javainterface.className, javainterface.jniMethods)

    def render_java_class(self, _class):
        if _class is not None:
            try:
                javaclass = JavaClass(self.package, _class, self.translator)
                self.classes[javaclass.className] = javaclass
                for enum in javaclass.enums:
                    self.jni.add_enum(enum)
            except AbsApi.Error as e:
                logging.error('Could not translate {0}: {1}'.format(_class.name.to_camel_case(fullName=True), e.args[0]))
            self.jni.add_methods(javaclass.className, javaclass.jniMethods)

##########################################################################

if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Generate source files for the Java wrapper')
    argparser.add_argument('xmldir', type=str, help='Directory where the XML documentation of the Linphone\'s API generated by Doxygen is placed')
    argparser.add_argument('-o --output', type=str, help='the directory where to generate the source files', dest='outputdir', default='.')
    argparser.add_argument('-p --package', type=str, help='the package name for the wrapper', dest='package', default='org.linphone.core')
    argparser.add_argument('-n --name', type=str, help='the name of the genarated source file', dest='name', default='linphone_jni.cc')
    argparser.add_argument('-e --exceptions', type=bool, help='enable the wrapping of LinphoneStatus into CoreException', dest='exceptions', default=False)
    argparser.add_argument('-v --verbose', action='store_true', dest='verbose_mode', default=False, help='Verbose mode.')
    args = argparser.parse_args()

    loglevel = logging.INFO if args.verbose_mode else logging.ERROR
    logging.basicConfig(format='%(levelname)s[%(name)s]: %(message)s', level=loglevel)

    srcdir = args.outputdir + '/src'
    javadir = args.outputdir + '/java'
    package_dirs = args.package.split('.')
    for directory in package_dirs:
        javadir += '/' + directory

    try:
        os.makedirs(srcdir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            logging.critical("Cannot create '{0}' dircetory: {1}".format(srcdir, e.strerror))
            sys.exit(1)

    try:
        os.makedirs(javadir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            logging.critical("Cannot create '{0}' dircetory: {1}".format(javadir, e.strerror))
            sys.exit(1)

    genwrapper = GenWrapper(srcdir, javadir, args.package, args.xmldir, args.exceptions)
    genwrapper.render_all()
