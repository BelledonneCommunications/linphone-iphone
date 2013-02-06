/*
xml2lpc_jni.cc
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "my_jni.h"
extern "C" {
#include "xml2lpc.h"
}
#ifdef USE_JAVAH
#include "xml2lpc_jni.h"
#endif

#include <stdio.h>

struct jni_xml2lpc_ctx {
	JNIEnv *env;
	jobject obj;
	xml2lpc_context *ctx;
};

static bool update_and_check_context(jni_xml2lpc_ctx *jni_ctx, JNIEnv *env, jobject obj) {
	if(jni_ctx != NULL && jni_ctx->ctx != NULL) {
		jni_ctx->env = env;
		jni_ctx->obj = obj; 
		return true;
	}
	return false;
}

#define XML2LPC_CALLBACK_BUFFER_SIZE  1024

extern "C" void Java_org_linphone_tools_Xml2Lpc_callback (void *ctx, xml2lpc_log_level level, const char *fmt, va_list list) {
	jni_xml2lpc_ctx *jni_ctx = (jni_xml2lpc_ctx *)ctx;
	if(jni_ctx->ctx != NULL) {
		JNIEnv *env = jni_ctx->env;
		jobject obj = jni_ctx->obj;
		
		char buffer[XML2LPC_CALLBACK_BUFFER_SIZE];
		vsnprintf(buffer, XML2LPC_CALLBACK_BUFFER_SIZE, fmt, list);
		jstring javaString = env->NewStringUTF(buffer);
		jint javaLevel = level;
		my_jni::callVoidMethod<void>(env, obj, "Xml2Lpc", "printLog", "(ILjava/lang/String;)V", javaLevel, javaString);
	}		
}

extern "C" void Java_org_linphone_tools_Xml2Lpc_init(JNIEnv *env, jobject obj) {
	jni_xml2lpc_ctx *jni_ctx = new jni_xml2lpc_ctx();
	jni_ctx->env = env;
	jni_ctx->obj = obj; 
	jni_ctx->ctx = xml2lpc_context_new(Java_org_linphone_tools_Xml2Lpc_callback, jni_ctx);
	bool result = my_jni::setLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr", jni_ctx);
	if(!result) {
		xml2lpc_context_destroy(jni_ctx->ctx);
		delete jni_ctx;
	}	
}

extern "C" void Java_org_linphone_tools_Xml2Lpc_destroy(JNIEnv *env, jobject obj) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	if(jni_ctx != NULL) {
		jni_ctx->env = env;
		jni_ctx->obj = obj; 
		if(jni_ctx->ctx) {
			xml2lpc_context_destroy(jni_ctx->ctx);
		}
		delete jni_ctx;
		my_jni::setLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr", NULL);
	}
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_setXmlFile(JNIEnv *env, jobject obj, jstring javaXmlFile) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		const char *xmlFile = env->GetStringUTFChars(javaXmlFile, 0);
		ret = xml2lpc_set_xml_file(jni_ctx->ctx, xmlFile);
		env->ReleaseStringChars(javaXmlFile, (jchar *)xmlFile);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_setXmlString(JNIEnv *env, jobject obj, jstring javaXmlString) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		const char *xmlString = env->GetStringUTFChars(javaXmlString, 0);
		ret = xml2lpc_set_xml_string(jni_ctx->ctx, xmlString);
		env->ReleaseStringChars(javaXmlString, (jchar *)xmlString);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_setXsdFile(JNIEnv *env, jobject obj, jstring javaXsdFile) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		const char *xsdFile = env->GetStringUTFChars(javaXsdFile, 0);
		ret = xml2lpc_set_xsd_file(jni_ctx->ctx, xsdFile);
		env->ReleaseStringChars(javaXsdFile, (jchar *)xsdFile);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_setXsdString(JNIEnv *env, jobject obj, jstring javaXsdString) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		const char *xsdString = env->GetStringUTFChars(javaXsdString, 0);
		ret = xml2lpc_set_xsd_string(jni_ctx->ctx, xsdString);
		env->ReleaseStringChars(javaXsdString, (jchar *)xsdString);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_validate(JNIEnv *env, jobject obj) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		ret = xml2lpc_validate(jni_ctx->ctx);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Xml2Lpc_convert(JNIEnv *env, jobject obj, jobject javaLpc) {
	jni_xml2lpc_ctx *jni_ctx = my_jni::getLongField<jni_xml2lpc_ctx*>(env, obj, "Xml2Lpc", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		LpConfig *lpc = my_jni::getLongField<LpConfig*>(env, javaLpc, "LpConfigImpl", "nativePtr");
		if(lpc != NULL) {
			ret = xml2lpc_convert(jni_ctx->ctx, lpc);
		} 
	}
	return ret;
}
