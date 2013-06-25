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
#include "lpc2xml.h"
}
#ifdef USE_JAVAH
#include "lpc2xml_jni.h"
#endif

#include <stdio.h>

struct jni_lpc2xml_ctx {
	JNIEnv *env;
	jobject obj;
	lpc2xml_context *ctx;
};

static bool update_and_check_context(jni_lpc2xml_ctx *jni_ctx, JNIEnv *env, jobject obj) {
	if(jni_ctx != NULL && jni_ctx->ctx != NULL) {
		jni_ctx->env = env;
		jni_ctx->obj = obj; 
		return true;
	}
	return false;
}

#define LPC2XML_CALLBACK_BUFFER_SIZE  1024

extern "C" void Java_org_linphone_tools_Lpc2Xml_callback (void *ctx, lpc2xml_log_level level, const char *fmt, va_list list) {
	jni_lpc2xml_ctx *jni_ctx = (jni_lpc2xml_ctx *)ctx;
	if(jni_ctx->ctx != NULL) {
		JNIEnv *env = jni_ctx->env;
		jobject obj = jni_ctx->obj;
		
		char buffer[LPC2XML_CALLBACK_BUFFER_SIZE];
		vsnprintf(buffer, LPC2XML_CALLBACK_BUFFER_SIZE, fmt, list);
		jstring javaString = env->NewStringUTF(buffer);
		jint javaLevel = level;
		my_jni::callVoidMethod<void>(env, obj, "Lpc2Xml", "printLog", "(ILjava/lang/String;)V", javaLevel, javaString);
	}		
}

extern "C" void Java_org_linphone_tools_Lpc2Xml_init(JNIEnv *env, jobject obj) {
	jni_lpc2xml_ctx *jni_ctx = new jni_lpc2xml_ctx();
	jni_ctx->env = env;
	jni_ctx->obj = obj; 
	jni_ctx->ctx = lpc2xml_context_new(Java_org_linphone_tools_Lpc2Xml_callback, obj);
	bool result = my_jni::setLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr", jni_ctx);
	if(!result) {
		lpc2xml_context_destroy(jni_ctx->ctx);
		delete jni_ctx;
	}	
}

extern "C" void Java_org_linphone_tools_Lpc2Xml_destroy(JNIEnv *env, jobject obj) {
	jni_lpc2xml_ctx *jni_ctx = my_jni::getLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr");
	if(jni_ctx != NULL) {
		jni_ctx->env = env;
		jni_ctx->obj = obj; 
		
		if(jni_ctx->ctx != NULL) {
			lpc2xml_context_destroy(jni_ctx->ctx);
		}
		delete jni_ctx;
		my_jni::setLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr", NULL);
	}
}

extern "C" jint Java_org_linphone_tools_Lpc2Xml_setLpc(JNIEnv *env, jobject obj, jobject javaLpc) {
	jni_lpc2xml_ctx *jni_ctx = my_jni::getLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		LpConfig *lpc = my_jni::getLongField<LpConfig*>(env, javaLpc, "LpConfigImpl", "nativePtr");
		if(lpc != NULL) {
			lpc2xml_set_lpc(jni_ctx->ctx, lpc);
		} 
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Lpc2Xml_convertFile(JNIEnv *env, jobject obj, jstring javaFile) {
	jni_lpc2xml_ctx *jni_ctx = my_jni::getLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		const char *file = env->GetStringUTFChars(javaFile, 0);
		ret = lpc2xml_convert_file(jni_ctx->ctx, file);
		env->ReleaseStringChars(javaFile, (jchar *)file);
	}
	return ret;
}

extern "C" jint Java_org_linphone_tools_Lpc2Xml_convertString(JNIEnv *env, jobject obj, jobject javaStringBuffer) {
	jni_lpc2xml_ctx *jni_ctx = my_jni::getLongField<jni_lpc2xml_ctx*>(env, obj, "Lpc2Xml", "internalPtr");
	jint ret = -666;
	if(update_and_check_context(jni_ctx, env, obj)) {
		char *string = NULL;
		ret = lpc2xml_convert_string(jni_ctx->ctx, &string);
		if(string != NULL) {
			jstring javaString = env->NewStringUTF(string);
			my_jni::callObjectMethod<jobject>(env, obj, "StringBuffer", "append", "(Ljava/lang/String;)Ljava/lang/StringBuffer;", javaString);
			env->ReleaseStringChars(javaString, (jchar *)string);
		}		
	}
	return ret;
}
