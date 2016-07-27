/*
my_jni.cc
Copyright (C) 2013  Belledonne Communications, Grenoble, France

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __MY_JNI__H
#define __MY_JNI__H
#include <jni.h>
extern "C" {
#include "linphonecore_utils.h"
}

#define defCallMethod(Type)												\
template <typename ReturnType>												\
static ReturnType call##Type##Method(JNIEnv *env, jobject obj, const char *className, const char *methodName,		\
					const char *methodSignature, ...) {						\
	jclass my_class = env->GetObjectClass(obj);									\
	if(my_class == 0) {												\
		ms_error("Can't get %s JNI class", className);								\
		return NULL;												\
	}														\
	jmethodID my_method = env->GetMethodID(my_class, methodName, methodSignature);					\
	if(my_method == 0) {												\
		ms_error("Can't get %s %s %s method", className, methodName, methodSignature);					\
		return NULL;												\
	}														\
	va_list vl;													\
	va_start(vl, methodSignature);											\
	ReturnType ret = env->Call##Type##MethodV(obj, my_method, vl);							\
	va_end(vl);													\
	return ret;													\
}															\

#define defGetterTypeField(Type, JavaType, JavaStringType)								\
template <typename ValueType> 												\
static ValueType get##Type##Field(JNIEnv *env, jobject obj, const char *className, const char *fieldName) {		\
	jclass my_class = env->GetObjectClass(obj);									\
	if(my_class == 0) {												\
		ms_error("Can't get %s JNI class", className);								\
		return NULL;												\
	}														\
	jfieldID my_field = env->GetFieldID(my_class, fieldName, JavaStringType);					\
	if(my_field == 0) {												\
		ms_error("Can't get %s %s field", className, fieldName);						\
		return NULL;												\
	}														\
	return (ValueType) env->Get##Type##Field(obj, my_field);							\
}															\

#define defSetterTypeField(Type, JavaType, JavaStringType) 								\
template <typename ValueType>												\
static bool set##Type##Field(JNIEnv *env, jobject obj, const char *className, const char *fieldName, ValueType val) {	\
	jclass my_class = env->GetObjectClass(obj);									\
	if(my_class == 0) {												\
		ms_error("Can't get %s JNI class", className);								\
		return false;												\
	}														\
	jfieldID my_field = env->GetFieldID(my_class, fieldName, JavaStringType);					\
	if(my_field == 0) {												\
		ms_error("Can't get %s %s field", className, fieldName);						\
		return false;												\
	}														\
	env->Set##Type##Field(obj, my_field, (JavaType) val);								\
	return true;													\
}															\

#define defGetterAndSetterTypeField(Type, JavaType, JavaStringType) 							\
	defGetterTypeField(Type, JavaType, JavaStringType) 								\
	defSetterTypeField(Type, JavaType, JavaStringType) 								\

namespace my_jni {
	template <typename ReturnType>
	static void callVoidMethod(JNIEnv *env, jobject obj, const char *className, const char *methodName,
						const char *methodSignature, ...) {
		jclass my_class = env->GetObjectClass(obj);
		if(my_class == 0) {
			ms_error("Can't get %s JNI class", className);
			return;
		}
		jmethodID my_method = env->GetMethodID(my_class, methodName, methodSignature);
		if(my_method == 0) {
			ms_error("Can't get %s %s %s method", className, methodName, methodSignature);
			return;
		}
		va_list vl;
		va_start(vl, methodSignature);
		env->CallVoidMethodV(obj, my_method, vl);
		va_end(vl);
	}
	defCallMethod(Object)
	defGetterAndSetterTypeField(Long, jlong, "J")
}

#endif //__MY_JNI__H
