/*
linphonecore_jni.cc
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
#include <jni.h>
#include "linphonecore.h"
#ifdef ANDROID
#include <android/log.h>
#endif /*ANDROID*/

extern "C" void ms_andsnd_register_card(JavaVM *jvm) ;
static JavaVM *jvm=0;

#ifdef ANDROID
static void linphone_android_log_handler(OrtpLogLevel lev, const char *fmt, va_list args){
	int prio;
	switch(lev){
	case ORTP_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
	case ORTP_MESSAGE:	prio = ANDROID_LOG_INFO;	break;
	case ORTP_WARNING:	prio = ANDROID_LOG_WARN;	break;
	case ORTP_ERROR:	prio = ANDROID_LOG_ERROR;	break;
	case ORTP_FATAL:	prio = ANDROID_LOG_FATAL;	break;
	default:		prio = ANDROID_LOG_DEFAULT;	break;
	}
	__android_log_vprint(prio, LOG_DOMAIN, fmt, args);
}
#endif /*ANDROID*/

JNIEXPORT jint JNICALL  JNI_OnLoad(JavaVM *ajvm, void *reserved)
{
	#ifdef ANDROID
	linphone_core_enable_logs_with_cb(linphone_android_log_handler);
	ms_andsnd_register_card(ajvm);
#endif /*ANDROID*/
	jvm=ajvm;
	return JNI_VERSION_1_2;
}



// LinphoneCore

class LinphoneCoreData {
public:
	LinphoneCoreData(JNIEnv*  env, jobject lc,jobject alistener, jobject auserdata) {

		core = env->NewGlobalRef(lc);
		listener = env->NewGlobalRef(alistener);
		userdata = auserdata?env->NewGlobalRef(auserdata):0;
		memset(&vTable,0,sizeof(vTable));
		vTable.show = showInterfaceCb;
		vTable.inv_recv = inviteReceivedCb;
		vTable.auth_info_requested = authInfoRequested;
		vTable.display_status = displayStatusCb;
		vTable.display_message = displayMessageCb;
		vTable.display_warning = displayMessageCb;
		vTable.general_state = generalStateChange;

		listernerClass = (jclass)env->NewGlobalRef(env->GetObjectClass( alistener));
		/*displayStatus(LinphoneCore lc,String message);*/
		displayStatusId = env->GetMethodID(listernerClass,"displayStatus","(Lorg/linphone/core/LinphoneCore;Ljava/lang/String;)V");
		/*void generalState(LinphoneCore lc,int state); */
		generalStateId = env->GetMethodID(listernerClass,"generalState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$GeneralState;)V");

		generalStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$GeneralState"));
		generalStateFromIntId = env->GetStaticMethodID(generalStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$GeneralState;");
	}

	~LinphoneCoreData() {
		JNIEnv *env = 0;
		jvm->AttachCurrentThread(&env,NULL);
		env->DeleteGlobalRef(core);
		env->DeleteGlobalRef(listener);
		if (userdata) env->DeleteGlobalRef(userdata);
		env->DeleteGlobalRef(listernerClass);
		env->DeleteGlobalRef(generalStateClass);
	}
	jobject core;
	jobject listener;
	jobject userdata;

	jclass listernerClass;
	jclass generalStateClass;
	jmethodID displayStatusId;
	jmethodID generalStateId;
	jmethodID generalStateFromIntId;
	LinphoneCoreVTable vTable;

	static void showInterfaceCb(LinphoneCore *lc) {

	}
	static  void inviteReceivedCb(LinphoneCore *lc, const char *from) {

	}
	static void byeReceivedCb(LinphoneCore *lc, const char *from) {

	}
	static void displayStatusCb(LinphoneCore *lc, const char *message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener,lcData->displayStatusId,lcData->core,env->NewStringUTF(message));
	}
	static void displayMessageCb(LinphoneCore *lc, const char *message) {

	}
	static void authInfoRequested(LinphoneCore *lc, const char *realm, const char *username) {

	}
	static void generalStateChange(LinphoneCore *lc, LinphoneGeneralState *gstate) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->generalStateId
							,lcData->core
							,env->CallStaticObjectMethod(lcData->generalStateClass,lcData->generalStateFromIntId,gstate->new_state));
	}

};
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_newLinphoneCore(JNIEnv*  env
		,jobject  thiz
		,jobject jlistener
		,jstring juserConfig
		,jstring jfactoryConfig
		,jobject  juserdata){

	const char* userConfig = env->GetStringUTFChars(juserConfig, NULL);
	const char* factoryConfig = env->GetStringUTFChars(jfactoryConfig, NULL);
	LinphoneCoreData* ldata = new LinphoneCoreData(env,thiz,jlistener,juserdata);
	jlong nativePtr = (jlong)linphone_core_new(	&ldata->vTable
			,userConfig
			,factoryConfig
			,ldata);
	//clear auth info list
	linphone_core_clear_all_auth_info((LinphoneCore*) nativePtr);
	//clear existing proxy config
	linphone_core_clear_proxy_config((LinphoneCore*) nativePtr);

	env->ReleaseStringUTFChars(juserConfig, userConfig);
	env->ReleaseStringUTFChars(jfactoryConfig, factoryConfig);
	return nativePtr;
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_clearProxyConfigs(JNIEnv* env, jobject thiz,jlong lc) {
	linphone_core_clear_proxy_config((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDefaultProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong pc) {
	linphone_core_set_default_proxy((LinphoneCore*)lc,(LinphoneProxyConfig*)pc);
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getDefaultProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	LinphoneProxyConfig *config=0;
	linphone_core_get_default_proxy((LinphoneCore*)lc,&config);
	return (jlong)config;
}

extern "C" int Java_org_linphone_core_LinphoneCoreImpl_addProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong pc) {
	return linphone_core_add_proxy_config((LinphoneCore*)lc,(LinphoneProxyConfig*)pc);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_clearAuthInfos(JNIEnv* env, jobject thiz,jlong lc) {
	linphone_core_clear_all_auth_info((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addAuthInfo(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong pc) {
	linphone_core_add_auth_info((LinphoneCore*)lc,(LinphoneAuthInfo*)pc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_iterate(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	linphone_core_iterate((LinphoneCore*)lc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_invite(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jstring juri) {
	const char* uri = env->GetStringUTFChars(juri, NULL);
	linphone_core_invite((LinphoneCore*)lc,uri);
	env->ReleaseStringUTFChars(juri, uri);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateCall(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	linphone_core_terminate_call((LinphoneCore*)lc,NULL);
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getRemoteAddress(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return (jlong)linphone_core_get_remote_uri((LinphoneCore*)lc);
}



//ProxyConfig

extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_newLinphoneProxyConfig(JNIEnv*  env,jobject  thiz) {
	return  (jlong) linphone_proxy_config_new();
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_delete(JNIEnv*  env,jobject  thiz,jlong ptr) {
	linphone_proxy_config_destroy((LinphoneProxyConfig*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setIdentity(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jidentity) {
	const char* identity = env->GetStringUTFChars(jidentity, NULL);
	linphone_proxy_config_set_identity((LinphoneProxyConfig*)proxyCfg,identity);
	env->ReleaseStringUTFChars(jidentity, identity);
}
extern "C" int Java_org_linphone_core_LinphoneProxyConfigImpl_setProxy(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jproxy) {
	const char* proxy = env->GetStringUTFChars(jproxy, NULL);
	int err=linphone_proxy_config_set_server_addr((LinphoneProxyConfig*)proxyCfg,proxy);
	env->ReleaseStringUTFChars(jproxy, proxy);
	return err;
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_enableRegister(JNIEnv* env,jobject thiz,jlong proxyCfg,jboolean enableRegister) {
	linphone_proxy_config_enable_register((LinphoneProxyConfig*)proxyCfg,enableRegister);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_edit(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	linphone_proxy_config_edit((LinphoneProxyConfig*)proxyCfg);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_done(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	linphone_proxy_config_done((LinphoneProxyConfig*)proxyCfg);
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_normalizePhoneNumber(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jnumber) {
	const char* number = env->GetStringUTFChars(jnumber, NULL);
	int len = env->GetStringLength(jnumber);
	char targetBuff[len];
	linphone_proxy_config_normalize_number((LinphoneProxyConfig*)proxyCfg,number,targetBuff,sizeof(targetBuff));
	jstring normalizedNumber = env->NewStringUTF(targetBuff);
	env->ReleaseStringUTFChars(jnumber, number);
	return normalizedNumber;
}
//Auth Info

extern "C" jlong Java_org_linphone_core_LinphoneAuthInfoImpl_newLinphoneAuthInfo(JNIEnv* env
		, jobject thiz
		, jstring jusername
		, jstring juserid
		, jstring jpassword
		, jstring jha1
		, jstring jrealm) {

	const char* username = env->GetStringUTFChars(jusername, NULL);
	const char* password = env->GetStringUTFChars(jpassword, NULL);
	jlong auth = (jlong)linphone_auth_info_new(username,NULL,password,NULL,NULL);

	env->ReleaseStringUTFChars(jusername, username);
	env->ReleaseStringUTFChars(jpassword, password);
	return auth;

}
extern "C" void Java_org_linphone_core_LinphoneAuthInfoImpl_delete(JNIEnv* env
		, jobject thiz
		, jlong ptr) {
	linphone_auth_info_destroy((LinphoneAuthInfo*)ptr);
}

//LinphoneAddress

extern "C" jlong Java_org_linphone_core_LinphoneAddressImpl_newLinphoneAddressImpl(JNIEnv*  env
																					,jobject  thiz
																					,jstring juri
																					,jstring jdisplayName) {
	const char* uri = env->GetStringUTFChars(juri, NULL);
	LinphoneAddress* address = linphone_address_new(uri);
	if (jdisplayName) {
		const char* displayName = env->GetStringUTFChars(jdisplayName, NULL);
		linphone_address_set_display_name(address,displayName);
		env->ReleaseStringUTFChars(jdisplayName, displayName);
	}
	env->ReleaseStringUTFChars(juri, uri);

	return  (jlong) address;
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_delete(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	linphone_address_destroy((LinphoneAddress*)ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getDisplayName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* displayName = linphone_address_get_display_name((LinphoneAddress*)ptr);
	if (displayName) {
		return env->NewStringUTF(displayName);
	} else {
		return 0;
	}
}
extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getUserName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* userName = linphone_address_get_username((LinphoneAddress*)ptr);
	if (userName) {
		return env->NewStringUTF(userName);
	} else {
		return 0;
	}
}
extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getDomain(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* domain = linphone_address_get_domain((LinphoneAddress*)ptr);
	if (domain) {
		return env->NewStringUTF(domain);
	} else {
		return 0;
	}
}
