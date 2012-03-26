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
#include "linphonecore_utils.h"
#include <ortp/zrtp.h>

#ifdef TUNNEL_ENABLED
#include "linphone_tunnel.h"
#endif

extern "C" {
#include "mediastreamer2/mediastream.h"
}
#include "mediastreamer2/msjava.h"
#include "private.h"
#include <cpu-features.h>

#ifdef ANDROID
#include <android/log.h>
extern "C" void libmsilbc_init();
#ifdef HAVE_X264
extern "C" void libmsx264_init();
#endif
#ifdef HAVE_AMR
extern "C" void libmsamr_init();
#endif
#ifdef HAVE_SILK
extern "C" void libmssilk_init();
#endif
#ifdef HAVE_G729
extern "C" void libmsbcg729_init();
#endif
#endif /*ANDROID*/

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

int dumbMethodForAllowingUsageOfCpuFeaturesFromStaticLibMediastream() {
	return (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0);
}
#endif /*ANDROID*/

JNIEXPORT jint JNICALL  JNI_OnLoad(JavaVM *ajvm, void *reserved)
{
#ifdef ANDROID
	ms_set_jvm(ajvm);
#endif /*ANDROID*/
	jvm=ajvm;
	return JNI_VERSION_1_2;
}


//LinphoneFactory
extern "C" void Java_org_linphone_core_LinphoneCoreFactoryImpl_setDebugMode(JNIEnv*  env
		,jobject  thiz
		,jboolean isDebug) {
	if (isDebug) {
		linphone_core_enable_logs_with_cb(linphone_android_log_handler);
	} else {
		linphone_core_disable_logs();
	}
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
		vTable.auth_info_requested = authInfoRequested;
		vTable.display_status = displayStatusCb;
		vTable.display_message = displayMessageCb;
		vTable.display_warning = displayMessageCb;
		vTable.global_state_changed = globalStateChange;
		vTable.registration_state_changed = registrationStateChange;
		vTable.call_state_changed = callStateChange;
		vTable.call_encryption_changed = callEncryptionChange;
		vTable.text_received = text_received;
		vTable.new_subscription_request = new_subscription_request;
		vTable.notify_presence_recv = notify_presence_recv;

		listenerClass = (jclass)env->NewGlobalRef(env->GetObjectClass( alistener));

		/*displayStatus(LinphoneCore lc,String message);*/
		displayStatusId = env->GetMethodID(listenerClass,"displayStatus","(Lorg/linphone/core/LinphoneCore;Ljava/lang/String;)V");

		/*void generalState(LinphoneCore lc,int state); */
		globalStateId = env->GetMethodID(listenerClass,"globalState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$GlobalState;Ljava/lang/String;)V");
		globalStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$GlobalState"));
		globalStateFromIntId = env->GetStaticMethodID(globalStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$GlobalState;");

		/*registrationState(LinphoneCore lc, LinphoneProxyConfig cfg, LinphoneCore.RegistrationState cstate, String smessage);*/
		registrationStateId = env->GetMethodID(listenerClass,"registrationState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneProxyConfig;Lorg/linphone/core/LinphoneCore$RegistrationState;Ljava/lang/String;)V");
		registrationStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$RegistrationState"));
		registrationStateFromIntId = env->GetStaticMethodID(registrationStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$RegistrationState;");

		/*callState(LinphoneCore lc, LinphoneCall call, LinphoneCall.State cstate,String message);*/
		callStateId = env->GetMethodID(listenerClass,"callState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;Lorg/linphone/core/LinphoneCall$State;Ljava/lang/String;)V");
		callStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCall$State"));
		callStateFromIntId = env->GetStaticMethodID(callStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCall$State;");

		/*callEncryption(LinphoneCore lc, LinphoneCall call, boolean encrypted,String auth_token);*/
		callEncryptionChangedId=env->GetMethodID(listenerClass,"callEncryptionChanged","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;ZLjava/lang/String;)V");

		/*void ecCalibrationStatus(LinphoneCore.EcCalibratorStatus status, int delay_ms, Object data);*/
		ecCalibrationStatusId = env->GetMethodID(listenerClass,"ecCalibrationStatus","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$EcCalibratorStatus;ILjava/lang/Object;)V");
		ecCalibratorStatusClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$EcCalibratorStatus"));
		ecCalibratorStatusFromIntId = env->GetStaticMethodID(ecCalibratorStatusClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$EcCalibratorStatus;");

		/*void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf, String url)*/
		newSubscriptionRequestId = env->GetMethodID(listenerClass,"newSubscriptionRequest","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriend;Ljava/lang/String;)V");

		/*void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf);*/
		notifyPresenceReceivedId = env->GetMethodID(listenerClass,"notifyPresenceReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriend;)V");

		/*void textReceived(LinphoneCore lc, LinphoneChatRoom cr,LinphoneAddress from,String message);*/
		textReceivedId = env->GetMethodID(listenerClass,"textReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatRoom;Lorg/linphone/core/LinphoneAddress;Ljava/lang/String;)V");

		proxyClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneProxyConfigImpl"));
		proxyCtrId = env->GetMethodID(proxyClass,"<init>", "(J)V");

		callClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCallImpl"));
		callCtrId = env->GetMethodID(callClass,"<init>", "(J)V");

		chatRoomClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneChatRoomImpl"));
		chatRoomCtrId = env->GetMethodID(chatRoomClass,"<init>", "(J)V");

		friendClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneFriendImpl"));;
		friendCtrId =env->GetMethodID(friendClass,"<init>", "(J)V");

		addressClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneAddressImpl"));
		addressCtrId =env->GetMethodID(addressClass,"<init>", "(J)V");

	}

	~LinphoneCoreData() {
		JNIEnv *env = 0;
		jvm->AttachCurrentThread(&env,NULL);
		env->DeleteGlobalRef(core);
		env->DeleteGlobalRef(listener);
		if (userdata) env->DeleteGlobalRef(userdata);
		env->DeleteGlobalRef(listenerClass);
		env->DeleteGlobalRef(globalStateClass);
		env->DeleteGlobalRef(registrationStateClass);
		env->DeleteGlobalRef(callStateClass);
		env->DeleteGlobalRef(proxyClass);
		env->DeleteGlobalRef(callClass);
		env->DeleteGlobalRef(chatRoomClass);
		env->DeleteGlobalRef(friendClass);

	}
	jobject core;
	jobject listener;
	jobject userdata;

	jclass listenerClass;
	jmethodID displayStatusId;
	jmethodID newSubscriptionRequestId;
	jmethodID notifyPresenceReceivedId;
	jmethodID textReceivedId;

	jclass globalStateClass;
	jmethodID globalStateId;
	jmethodID globalStateFromIntId;

	jclass registrationStateClass;
	jmethodID registrationStateId;
	jmethodID registrationStateFromIntId;

	jclass callStateClass;
	jmethodID callStateId;
	jmethodID callStateFromIntId;

	jmethodID callEncryptionChangedId;

	jclass ecCalibratorStatusClass;
	jmethodID ecCalibrationStatusId;
	jmethodID ecCalibratorStatusFromIntId;

	jclass proxyClass;
	jmethodID proxyCtrId;

	jclass callClass;
	jmethodID callCtrId;

	jclass chatRoomClass;
	jmethodID chatRoomCtrId;

	jclass friendClass;
	jmethodID friendCtrId;

	jclass addressClass;
	jmethodID addressCtrId;

	LinphoneCoreVTable vTable;

	static void showInterfaceCb(LinphoneCore *lc) {

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
	static void globalStateChange(LinphoneCore *lc, LinphoneGlobalState gstate,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->globalStateId
							,lcData->core
							,env->CallStaticObjectMethod(lcData->globalStateClass,lcData->globalStateFromIntId,(jint)gstate),
							message ? env->NewStringUTF(message) : NULL);
	}
	static void registrationStateChange(LinphoneCore *lc, LinphoneProxyConfig* proxy,LinphoneRegistrationState state,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->registrationStateId
							,lcData->core
							,env->NewObject(lcData->proxyClass,lcData->proxyCtrId,(jlong)proxy)
							,env->CallStaticObjectMethod(lcData->registrationStateClass,lcData->registrationStateFromIntId,(jint)state),
							message ? env->NewStringUTF(message) : NULL);
	}
	jobject getCall(JNIEnv *env , LinphoneCall *call){
		jobject jobj=0;

		if (call!=NULL){
			void *up=linphone_call_get_user_pointer(call);
			
			if (up==NULL){
				jobj=env->NewObject(callClass,callCtrId,(jlong)call);
				jobj=env->NewGlobalRef(jobj);
				linphone_call_set_user_pointer(call,(void*)jobj);
				linphone_call_ref(call);
			}else{
				jobj=(jobject)up;
			}
		}
		return jobj;
	}

	static void callStateChange(LinphoneCore *lc, LinphoneCall* call,LinphoneCallState state,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jcall;
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->callStateId
							,lcData->core
							,(jcall=lcData->getCall(env,call))
							,env->CallStaticObjectMethod(lcData->callStateClass,lcData->callStateFromIntId,(jint)state),
							message ? env->NewStringUTF(message) : NULL);
		if (state==LinphoneCallReleased){
			linphone_call_set_user_pointer(call,NULL);
			env->DeleteGlobalRef(jcall);
		}
	}
	static void callEncryptionChange(LinphoneCore *lc, LinphoneCall* call, bool_t encrypted,const char* authentication_token) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->callEncryptionChangedId
							,lcData->core
							,lcData->getCall(env,call)
							,encrypted
							,authentication_token ? env->NewStringUTF(authentication_token) : NULL);
	}
	static void notify_presence_recv (LinphoneCore *lc,  LinphoneFriend *my_friend) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->notifyPresenceReceivedId
							,lcData->core
							,env->NewObject(lcData->friendClass,lcData->friendCtrId,(jlong)my_friend));
	}
	static void new_subscription_request (LinphoneCore *lc,  LinphoneFriend *my_friend, const char* url) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->newSubscriptionRequestId
							,lcData->core
							,env->NewObject(lcData->friendClass,lcData->friendCtrId,(jlong)my_friend)
							,url ? env->NewStringUTF(url) : NULL);
	}
	static void text_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->textReceivedId
							,lcData->core
							,env->NewObject(lcData->chatRoomClass,lcData->chatRoomCtrId,(jlong)room)
							,env->NewObject(lcData->addressClass,lcData->addressCtrId,(jlong)from)
							,message ? env->NewStringUTF(message) : NULL);
	}
	static void ecCalibrationStatus(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms, void *data) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM\n");
			return;
		}
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data(lc);
		env->CallVoidMethod(lcData->listener
							,lcData->ecCalibrationStatusId
							,lcData->core
							,env->CallStaticObjectMethod(lcData->ecCalibratorStatusClass,lcData->ecCalibratorStatusFromIntId,(jint)status)
							,delay_ms
							,data ? data : NULL);
		if (data != NULL &&status !=LinphoneEcCalibratorInProgress ) {
			//final state, releasing global ref
			env->DeleteGlobalRef((jobject)data);
		}

	}


};
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_newLinphoneCore(JNIEnv*  env
		,jobject  thiz
		,jobject jlistener
		,jstring juserConfig
		,jstring jfactoryConfig
		,jobject  juserdata){

	const char* userConfig = juserConfig?env->GetStringUTFChars(juserConfig, NULL):NULL;
	const char* factoryConfig = jfactoryConfig?env->GetStringUTFChars(jfactoryConfig, NULL):NULL;
	LinphoneCoreData* ldata = new LinphoneCoreData(env,thiz,jlistener,juserdata);

#ifdef HAVE_ILBC
	libmsilbc_init(); // requires an fpu
#endif
#ifdef HAVE_X264
	libmsx264_init();
#endif
#ifdef HAVE_AMR
	libmsamr_init();
#endif
#ifdef HAVE_SILK
	libmssilk_init();
#endif
#ifdef HAVE_G729
	libmsbcg729_init();
#endif
	jlong nativePtr = (jlong)linphone_core_new(	&ldata->vTable
			,userConfig
			,factoryConfig
			,ldata);
	//clear auth info list
	linphone_core_clear_all_auth_info((LinphoneCore*) nativePtr);
	//clear existing proxy config
	linphone_core_clear_proxy_config((LinphoneCore*) nativePtr);

	if (userConfig) env->ReleaseStringUTFChars(juserConfig, userConfig);
	if (factoryConfig) env->ReleaseStringUTFChars(jfactoryConfig, factoryConfig);
	return nativePtr;
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_delete(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)lc);
	linphone_core_destroy((LinphoneCore*)lc);
	delete lcData;
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

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_getProxyConfigList(JNIEnv* env, jobject thiz, jlong lc) {
	const MSList* proxies = linphone_core_get_proxy_config_list((LinphoneCore*)lc);
	int proxyCount = ms_list_size(proxies);
	jlongArray jProxies = env->NewLongArray(proxyCount);
	jlong *jInternalArray = env->GetLongArrayElements(jProxies, NULL);

	for (int i = 0; i < proxyCount; i++ ) {
		jInternalArray[i] = (unsigned long) (proxies->data);
		proxies = proxies->next;
	}

	env->ReleaseLongArrayElements(jProxies, jInternalArray, 0);

	return jProxies;
}

extern "C" int Java_org_linphone_core_LinphoneCoreImpl_addProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jobject jproxyCfg
		,jlong lc
		,jlong pc) {
	LinphoneProxyConfig* proxy = (LinphoneProxyConfig*)pc;
	linphone_proxy_config_set_user_data(proxy, env->NewGlobalRef(jproxyCfg));

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
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_invite(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jstring juri) {
	const char* uri = env->GetStringUTFChars(juri, NULL);
	LinphoneCoreData *lcd=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)lc);
	LinphoneCall* lCall = linphone_core_invite((LinphoneCore*)lc,uri);
	env->ReleaseStringUTFChars(juri, uri);
	return lcd->getCall(env,lCall);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_inviteAddress(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong to) {
	LinphoneCoreData *lcd=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)lc);
	return lcd->getCall(env, linphone_core_invite_address((LinphoneCore*)lc,(LinphoneAddress*)to));
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateCall(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong call) {
	linphone_core_terminate_call((LinphoneCore*)lc,(LinphoneCall*)call);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getRemoteAddress(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return (jlong)linphone_core_get_current_call_remote_address((LinphoneCore*)lc);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInCall(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {

	return linphone_core_in_call((LinphoneCore*)lc);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInComingInvitePending(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {

	return linphone_core_inc_invite_pending((LinphoneCore*)lc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_acceptCall(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong call) {

	linphone_core_accept_call((LinphoneCore*)lc,(LinphoneCall*)call);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_acceptCallWithParams(JNIEnv *env,
		jobject thiz,
		jlong lc,
		jlong call,
		jlong params){
	linphone_core_accept_call_with_params((LinphoneCore*)lc,(LinphoneCall*)call, (LinphoneCallParams*)params);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_acceptCallUpdate(JNIEnv *env,
		jobject thiz,
		jlong lc,
		jlong call,
		jlong params){
	linphone_core_accept_call_update((LinphoneCore*)lc,(LinphoneCall*)call, (LinphoneCallParams*)params);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_deferCallUpdate(JNIEnv *env,
		jobject thiz,
		jlong lc,
		jlong call){
	linphone_core_defer_call_update((LinphoneCore*)lc,(LinphoneCall*)call);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getCallLog(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jint position) {
		return (jlong)ms_list_nth_data(linphone_core_get_call_logs((LinphoneCore*)lc),position);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getNumberOfCallLogs(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return ms_list_size(linphone_core_get_call_logs((LinphoneCore*)lc));
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setNetworkStateReachable(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jboolean isReachable) {
		linphone_core_set_network_reachable((LinphoneCore*)lc,isReachable);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPlaybackGain(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jfloat gain) {
		linphone_core_set_playback_gain_db((LinphoneCore*)lc,gain);
}

extern "C" float Java_org_linphone_core_LinphoneCoreImpl_getPlaybackGain(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return linphone_core_get_playback_gain_db((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_muteMic(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jboolean isMuted) {
		linphone_core_mute_mic((LinphoneCore*)lc,isMuted);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_interpretUrl(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jstring jurl) {
	const char* url = env->GetStringUTFChars(jurl, NULL);
	jlong result = (jlong)linphone_core_interpret_url((LinphoneCore*)lc,url);
	env->ReleaseStringUTFChars(jurl, url);
	return result;
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_sendDtmf(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jchar dtmf) {
	linphone_core_send_dtmf((LinphoneCore*)lc,dtmf);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_playDtmf(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jchar dtmf
		,jint duration) {
	linphone_core_play_dtmf((LinphoneCore*)lc,dtmf,duration);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_stopDtmf(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	linphone_core_stop_dtmf((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_clearCallLogs(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc) {
	linphone_core_clear_call_logs((LinphoneCore*)lc);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isMicMuted(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return linphone_core_is_mic_muted((LinphoneCore*)lc);
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_findPayloadType(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jmime
																			,jint rate) {
	const char* mime = env->GetStringUTFChars(jmime, NULL);
	jlong result = (jlong)linphone_core_find_payload_type((LinphoneCore*)lc,mime,rate);
	env->ReleaseStringUTFChars(jmime, mime);
	return result;
}
extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_listVideoPayloadTypes(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const MSList* codecs = linphone_core_get_video_codecs((LinphoneCore*)lc);
	int codecsCount = ms_list_size(codecs);
	jlongArray jCodecs = env->NewLongArray(codecsCount);
	jlong *jInternalArray = env->GetLongArrayElements(jCodecs, NULL);

	for (int i = 0; i < codecsCount; i++ ) {
		jInternalArray[i] = (unsigned long) (codecs->data);
		codecs = codecs->next;
	}

	env->ReleaseLongArrayElements(jCodecs, jInternalArray, 0);

	return jCodecs;
}

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_listAudioPayloadTypes(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const MSList* codecs = linphone_core_get_audio_codecs((LinphoneCore*)lc);
	int codecsCount = ms_list_size(codecs);
	jlongArray jCodecs = env->NewLongArray(codecsCount);
	jlong *jInternalArray = env->GetLongArrayElements(jCodecs, NULL);

	for (int i = 0; i < codecsCount; i++ ) {
		jInternalArray[i] = (unsigned long) (codecs->data);
		codecs = codecs->next;
	}

	env->ReleaseLongArrayElements(jCodecs, jInternalArray, 0);

	return jCodecs;
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_enablePayloadType(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt
																			,jboolean enable) {
	return linphone_core_enable_payload_type((LinphoneCore*)lc,(PayloadType*)pt,enable);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableEchoCancellation(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jboolean enable) {
	linphone_core_enable_echo_cancellation((LinphoneCore*)lc,enable);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableEchoLimiter(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jboolean enable) {
	linphone_core_enable_echo_limiter((LinphoneCore*)lc,enable);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isEchoCancellationEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return linphone_core_echo_cancellation_enabled((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isEchoLimiterEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return linphone_core_echo_limiter_enabled((LinphoneCore*)lc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getCurrentCall(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	LinphoneCoreData *lcdata=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)lc);
	
	return lcdata->getCall(env,linphone_core_get_current_call((LinphoneCore*)lc));
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addFriend(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong aFriend
																			) {
	linphone_core_add_friend((LinphoneCore*)lc,(LinphoneFriend*)aFriend);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPresenceInfo(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint minute_away
																			,jstring jalternative_contact
																			,jint status) {
	const char* alternative_contact = jalternative_contact?env->GetStringUTFChars(jalternative_contact, NULL):NULL;
	linphone_core_set_presence_info((LinphoneCore*)lc,minute_away,alternative_contact,(LinphoneOnlineStatus)status);
	if (alternative_contact) env->ReleaseStringUTFChars(jalternative_contact, alternative_contact);
}

extern "C" long Java_org_linphone_core_LinphoneCoreImpl_createChatRoom(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jto) {

	const char* to = env->GetStringUTFChars(jto, NULL);
	LinphoneChatRoom* lResult = linphone_core_create_chat_room((LinphoneCore*)lc,to);
	env->ReleaseStringUTFChars(jto, to);
	return (long)lResult;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableVideo(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jboolean vcap_enabled
																			,jboolean display_enabled) {
	linphone_core_enable_video((LinphoneCore*)lc, vcap_enabled,display_enabled);

}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isVideoEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	return linphone_core_video_enabled((LinphoneCore*)lc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPlayFile(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = jpath?env->GetStringUTFChars(jpath, NULL):NULL;
	linphone_core_set_play_file((LinphoneCore*)lc,path);
	if (path) env->ReleaseStringUTFChars(jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRing(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = jpath?env->GetStringUTFChars(jpath, NULL):NULL;
	linphone_core_set_ring((LinphoneCore*)lc,path);
	if (path) env->ReleaseStringUTFChars(jpath, path);
}
extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getRing(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	const char* path = linphone_core_get_ring((LinphoneCore*)lc);
	if (path) {
		return env->NewStringUTF(path);
	} else {
		return NULL;
	}
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRootCA(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = jpath?env->GetStringUTFChars(jpath, NULL):NULL;
	linphone_core_set_root_ca((LinphoneCore*)lc,path);
	if (path) env->ReleaseStringUTFChars(jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableKeepAlive(JNIEnv*  env
																,jobject  thiz
																,jlong lc
																,jboolean enable) {
	linphone_core_enable_keep_alive((LinphoneCore*)lc,enable);

}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isKeepAliveEnabled(JNIEnv*  env
																,jobject  thiz
																,jlong lc) {
	return linphone_core_keep_alive_enabled((LinphoneCore*)lc);

}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_startEchoCalibration(JNIEnv*  env
																				,jobject  thiz
																				,jlong lc
																				,jobject data) {
	return linphone_core_start_echo_calibration((LinphoneCore*)lc
													, LinphoneCoreData::ecCalibrationStatus
													, data?env->NewGlobalRef(data):NULL);

}

extern "C" int Java_org_linphone_core_LinphoneCoreImpl_getMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (int)linphone_core_get_media_encryption((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,int menc) {
	linphone_core_set_media_encryption((LinphoneCore*)lc,(LinphoneMediaEncryption)menc);
}

extern "C" int Java_org_linphone_core_LinphoneCallParamsImpl_getMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			) {
	return (int)linphone_call_params_get_media_encryption((LinphoneCallParams*)cp);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_mediaEncryptionSupported(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc, jint menc
																			) {
	return linphone_core_media_encryption_supported((LinphoneCore*)lc,(LinphoneMediaEncryption)menc);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_setMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			,int jmenc) {
	linphone_call_params_set_media_encryption((LinphoneCallParams*)cp,(LinphoneMediaEncryption)jmenc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_getMediaEncryptionMandatory(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return linphone_core_is_media_encryption_mandatory((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMediaEncryptionMandatory(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			, jboolean yesno
																			) {
	linphone_core_set_media_encryption_mandatory((LinphoneCore*)lc, yesno);
}

//ProxyConfig

extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_newLinphoneProxyConfig(JNIEnv*  env,jobject  thiz) {
	LinphoneProxyConfig* proxy = linphone_proxy_config_new();
	return  (jlong) proxy;
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_delete(JNIEnv*  env,jobject  thiz,jlong ptr) {
	linphone_proxy_config_destroy((LinphoneProxyConfig*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setIdentity(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jidentity) {
	const char* identity = env->GetStringUTFChars(jidentity, NULL);
	linphone_proxy_config_set_identity((LinphoneProxyConfig*)proxyCfg,identity);
	env->ReleaseStringUTFChars(jidentity, identity);
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getIdentity(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* identity = linphone_proxy_config_get_identity((LinphoneProxyConfig*)proxyCfg);
	if (identity) {
		return env->NewStringUTF(identity);
	} else {
		return NULL;
	}
}
extern "C" int Java_org_linphone_core_LinphoneProxyConfigImpl_setProxy(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jproxy) {
	const char* proxy = env->GetStringUTFChars(jproxy, NULL);
	int err=linphone_proxy_config_set_server_addr((LinphoneProxyConfig*)proxyCfg,proxy);
	env->ReleaseStringUTFChars(jproxy, proxy);
	return err;
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getProxy(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* proxy = linphone_proxy_config_get_addr((LinphoneProxyConfig*)proxyCfg);
	if (proxy) {
		return env->NewStringUTF(proxy);
	} else {
		return NULL;
	}
}
extern "C" int Java_org_linphone_core_LinphoneProxyConfigImpl_setRoute(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jroute) {
	if (jroute != NULL) {
		const char* route = env->GetStringUTFChars(jroute, NULL);
		int err=linphone_proxy_config_set_route((LinphoneProxyConfig*)proxyCfg,route);
		env->ReleaseStringUTFChars(jroute, route);
		return err;
	} else {
		return linphone_proxy_config_set_route((LinphoneProxyConfig*)proxyCfg,NULL);
	}
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getRoute(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* route = linphone_proxy_config_get_route((LinphoneProxyConfig*)proxyCfg);
	if (route) {
		return env->NewStringUTF(route);
	} else {
		return NULL;
	}
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_enableRegister(JNIEnv* env,jobject thiz,jlong proxyCfg,jboolean enableRegister) {
	linphone_proxy_config_enable_register((LinphoneProxyConfig*)proxyCfg,enableRegister);
}
extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_isRegistered(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return linphone_proxy_config_is_registered((LinphoneProxyConfig*)proxyCfg);
}
extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_isRegisterEnabled(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return linphone_proxy_config_register_enabled((LinphoneProxyConfig*)proxyCfg);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_edit(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	linphone_proxy_config_edit((LinphoneProxyConfig*)proxyCfg);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_done(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	linphone_proxy_config_done((LinphoneProxyConfig*)proxyCfg);
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_normalizePhoneNumber(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jnumber) {
	if (jnumber == 0) {
		ms_error("cannot normalized null number");
	}
	const char* number = env->GetStringUTFChars(jnumber, NULL);
	int len = env->GetStringLength(jnumber);
	if (len == 0) {
		ms_warning("cannot normalize empty number");
		return jnumber;
	}
	char targetBuff[2*len];// returned number can be greater than origin (specially in case of prefix insertion
	linphone_proxy_config_normalize_number((LinphoneProxyConfig*)proxyCfg,number,targetBuff,sizeof(targetBuff));
	jstring normalizedNumber = env->NewStringUTF(targetBuff);
	env->ReleaseStringUTFChars(jnumber, number);
	return normalizedNumber;
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getDomain(JNIEnv* env
																			,jobject thiz
																			,jlong proxyCfg) {
	const char* domain = linphone_proxy_config_get_domain((LinphoneProxyConfig*)proxyCfg);
	if (domain) {
		return env->NewStringUTF(domain);
	} else {
		return NULL;
	}
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setDialEscapePlus(JNIEnv* env,jobject thiz,jlong proxyCfg,jboolean value) {
	linphone_proxy_config_set_dial_escape_plus((LinphoneProxyConfig*)proxyCfg,value);
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setDialPrefix(JNIEnv* env
																	,jobject thiz
																	,jlong proxyCfg
																	,jstring jprefix) {
	const char* prefix = env->GetStringUTFChars(jprefix, NULL);
	linphone_proxy_config_set_dial_prefix((LinphoneProxyConfig*)proxyCfg,prefix);
	env->ReleaseStringUTFChars(jprefix, prefix);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_enablePublish(JNIEnv* env
																				,jobject thiz
																				,jlong proxyCfg
																				,jboolean val) {
	linphone_proxy_config_enable_publish((LinphoneProxyConfig*)proxyCfg,val);
}
extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_publishEnabled(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return linphone_proxy_config_publish_enabled((LinphoneProxyConfig*)proxyCfg);
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
	if (jdisplayName && address) {
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

extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_toString(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	char* uri = linphone_address_as_string((LinphoneAddress*)ptr);
	jstring juri =env->NewStringUTF(uri);
	ms_free(uri);
	return juri;
}
extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_toUri(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	char* uri = linphone_address_as_string_uri_only((LinphoneAddress*)ptr);
	jstring juri =env->NewStringUTF(uri);
	ms_free(uri);
	return juri;
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_setDisplayName(JNIEnv*  env
																		,jobject  thiz
																		,jlong address
																		,jstring jdisplayName) {
	const char* displayName = jdisplayName!= NULL?env->GetStringUTFChars(jdisplayName, NULL):NULL;
	linphone_address_set_display_name((LinphoneAddress*)address,displayName);
	if (displayName != NULL) env->ReleaseStringUTFChars(jdisplayName, displayName);
}


//CallLog
extern "C" jlong Java_org_linphone_core_LinphoneCallLogImpl_getFrom(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)((LinphoneCallLog*)ptr)->from;
}
extern "C" jlong Java_org_linphone_core_LinphoneCallLogImpl_getTo(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)((LinphoneCallLog*)ptr)->to;
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallLogImpl_isIncoming(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return ((LinphoneCallLog*)ptr)->dir==LinphoneCallIncoming?JNI_TRUE:JNI_FALSE;
}

/*payloadType*/
extern "C" jstring Java_org_linphone_core_PayloadTypeImpl_toString(JNIEnv*  env,jobject  thiz,jlong ptr) {
	PayloadType* pt = (PayloadType*)ptr;
	char* value = ms_strdup_printf("[%s] clock [%i], bitrate [%i]"
									,payload_type_get_mime(pt)
									,payload_type_get_rate(pt)
									,payload_type_get_bitrate(pt));
	jstring jvalue =env->NewStringUTF(value);
	ms_free(value);
	return jvalue;
}
extern "C" jstring Java_org_linphone_core_PayloadTypeImpl_getMime(JNIEnv*  env,jobject  thiz,jlong ptr) {
	PayloadType* pt = (PayloadType*)ptr;
	jstring jvalue =env->NewStringUTF(payload_type_get_mime(pt));
	return jvalue;
}
extern "C" jint Java_org_linphone_core_PayloadTypeImpl_getRate(JNIEnv*  env,jobject  thiz, jlong ptr) {
	PayloadType* pt = (PayloadType*)ptr;
	return payload_type_get_rate(pt);
}

//LinphoneCall
extern "C" void Java_org_linphone_core_LinphoneCallImpl_finalize(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneCall *call=(LinphoneCall*)ptr;
	linphone_call_unref(call);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getCallLog(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_call_get_call_log((LinphoneCall*)ptr);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_isIncoming(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_call_get_dir((LinphoneCall*)ptr)==LinphoneCallIncoming?JNI_TRUE:JNI_FALSE;
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getRemoteAddress(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_call_get_remote_address((LinphoneCall*)ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneCallImpl_getState(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)linphone_call_get_state((LinphoneCall*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneCallImpl_enableEchoCancellation(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jboolean value) {
	linphone_call_enable_echo_cancellation((LinphoneCall*)ptr,value);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_isEchoCancellationEnabled(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_call_echo_cancellation_enabled((LinphoneCall*)ptr);
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_enableEchoLimiter(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jboolean value) {
	linphone_call_enable_echo_limiter((LinphoneCall*)ptr,value);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_isEchoLimiterEnabled(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_call_echo_limiter_enabled((LinphoneCall*)ptr);
}

extern "C" jobject Java_org_linphone_core_LinphoneCallImpl_getReplacedCall(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneCoreData *lcd=(LinphoneCoreData*)linphone_core_get_user_data(linphone_call_get_core((LinphoneCall*)ptr));	
	return lcd->getCall(env,linphone_call_get_replaced_call((LinphoneCall*)ptr));
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallImpl_getCurrentQuality(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jfloat)linphone_call_get_current_quality((LinphoneCall*)ptr);
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallImpl_getAverageQuality(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jfloat)linphone_call_get_average_quality((LinphoneCall*)ptr);
}


//LinphoneFriend
extern "C" long Java_org_linphone_core_LinphoneFriendImpl_newLinphoneFriend(JNIEnv*  env
																		,jobject  thiz
																		,jstring jFriendUri) {
	LinphoneFriend* lResult;

	if (jFriendUri) {
		const char* friendUri = env->GetStringUTFChars(jFriendUri, NULL);
		lResult= linphone_friend_new_with_addr(friendUri);
		env->ReleaseStringUTFChars(jFriendUri, friendUri);
	} else {
		lResult = linphone_friend_new();
	}
	return (long)lResult;
}
extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong linphoneAddress) {
	linphone_friend_set_addr((LinphoneFriend*)ptr,(LinphoneAddress*)linphoneAddress);
}
extern "C" long Java_org_linphone_core_LinphoneFriendImpl_getAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (long)linphone_friend_get_address((LinphoneFriend*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setIncSubscribePolicy(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jint policy) {
	linphone_friend_set_inc_subscribe_policy((LinphoneFriend*)ptr,(LinphoneSubscribePolicy)policy);
}
extern "C" jint Java_org_linphone_core_LinphoneFriendImpl_getIncSubscribePolicy(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_friend_get_inc_subscribe_policy((LinphoneFriend*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneFriendImpl_enableSubscribes(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jboolean value) {
	linphone_friend_enable_subscribes((LinphoneFriend*)ptr,value);
}
extern "C" jboolean Java_org_linphone_core_LinphoneFriendImpl_isSubscribesEnabled(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_friend_subscribes_enabled((LinphoneFriend*)ptr);
}
extern "C" jboolean Java_org_linphone_core_LinphoneFriendImpl_getStatus(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_friend_get_status((LinphoneFriend*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneFriendImpl_edit(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_friend_edit((LinphoneFriend*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneFriendImpl_done(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	 linphone_friend_done((LinphoneFriend*)ptr);
}
//LinphoneChatRoom
extern "C" long Java_org_linphone_core_LinphoneChatRoomImpl_getPeerAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (long) linphone_chat_room_get_peer_address((LinphoneChatRoom*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_sendMessage(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jmessage) {
	const char* message = env->GetStringUTFChars(jmessage, NULL);
	linphone_chat_room_send_message((LinphoneChatRoom*)ptr,message);
	env->ReleaseStringUTFChars(jmessage, message);

}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoWindowId(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jobject obj) {
	jobject oldWindow = (jobject) linphone_core_get_native_video_window_id((LinphoneCore*)lc);
	if (oldWindow != NULL) {
		env->DeleteGlobalRef(oldWindow);
	}
	if (obj != NULL) {
		obj = env->NewGlobalRef(obj);
	}
	linphone_core_set_native_video_window_id((LinphoneCore*)lc,(unsigned long)obj);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPreviewWindowId(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jobject obj) {
	jobject oldWindow = (jobject) linphone_core_get_native_preview_window_id((LinphoneCore*)lc);
	if (oldWindow != NULL) {
		env->DeleteGlobalRef(oldWindow);
	}
	if (obj != NULL) {
		obj = env->NewGlobalRef(obj);
	}
	linphone_core_set_native_preview_window_id((LinphoneCore*)lc,(unsigned long)obj);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDeviceRotation(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jint rotation) {
	linphone_core_set_device_rotation((LinphoneCore*)lc,rotation);
}


extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setFirewallPolicy(JNIEnv *env, jobject thiz, jlong lc, int enum_value){
	linphone_core_set_firewall_policy((LinphoneCore*)lc,(LinphoneFirewallPolicy)enum_value);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getFirewallPolicy(JNIEnv *env, jobject thiz, jlong lc){
	return linphone_core_get_firewall_policy((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setStunServer(JNIEnv *env, jobject thiz, jlong lc, jstring jserver){
	const char* server = NULL;
	if (jserver) server=env->GetStringUTFChars(jserver, NULL);
	linphone_core_set_stun_server((LinphoneCore*)lc,server);
	if (server) env->ReleaseStringUTFChars(jserver,server);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getStunServer(JNIEnv *env, jobject thiz, jlong lc){
	const char *ret= linphone_core_get_stun_server((LinphoneCore*)lc);
	if (ret==NULL) return NULL;
	jstring jvalue =env->NewStringUTF(ret);
	return jvalue;
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_audioBandwidth(JNIEnv *env, jobject thiz, jlong lcp, jint bw){
	linphone_call_params_set_audio_bandwidth_limit((LinphoneCallParams*)lcp, bw);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableVideo(JNIEnv *env, jobject thiz, jlong lcp, jboolean b){
	linphone_call_params_enable_video((LinphoneCallParams*)lcp, b);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_getVideoEnabled(JNIEnv *env, jobject thiz, jlong lcp){
	return linphone_call_params_video_enabled((LinphoneCallParams*)lcp);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_localConferenceMode(JNIEnv *env, jobject thiz, jlong lcp){
	return linphone_call_params_local_conference_mode((LinphoneCallParams*)lcp);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_destroy(JNIEnv *env, jobject thiz, jlong lc){
	return linphone_call_params_destroy((LinphoneCallParams*)lc);
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_createDefaultCallParams(JNIEnv *env, jobject thiz, jlong lc){
	return (jlong) linphone_core_create_default_call_parameters((LinphoneCore*)lc);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getRemoteParams(JNIEnv *env, jobject thiz, jlong lc){
	return (jlong) linphone_call_params_copy(linphone_call_get_remote_params((LinphoneCall*)lc));
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getCurrentParamsCopy(JNIEnv *env, jobject thiz, jlong lc){
	return (jlong) linphone_call_params_copy(linphone_call_get_current_params((LinphoneCall*)lc));
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_enableCamera(JNIEnv *env, jobject thiz, jlong lc, jboolean b){
	linphone_call_enable_camera((LinphoneCall *)lc, (bool_t) b);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_cameraEnabled(JNIEnv *env, jobject thiz, jlong lc){
	linphone_call_camera_enabled((LinphoneCall *)lc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_inviteAddressWithParams(JNIEnv *env, jobject thiz, jlong lc, jlong addr, jlong params){
	LinphoneCoreData *lcd=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)lc);
	return  lcd->getCall(env,linphone_core_invite_address_with_params((LinphoneCore *)lc, (const LinphoneAddress *)addr, (const LinphoneCallParams *)params));
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_updateAddressWithParams(JNIEnv *env, jobject thiz, jlong lc, jlong call, jlong params){
	return (jint) linphone_core_update_call((LinphoneCore *)lc, (LinphoneCall *)call, (LinphoneCallParams *)params);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_updateCall(JNIEnv *env, jobject thiz, jlong lc, jlong call, jlong params){
	return (jint) linphone_core_update_call((LinphoneCore *)lc, (LinphoneCall *)call, (const LinphoneCallParams *)params);
}


extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPreferredVideoSize(JNIEnv *env, jobject thiz, jlong lc, jint width, jint height){
	MSVideoSize vsize;
	vsize.width = (int)width;
	vsize.height = (int)height;
	linphone_core_set_preferred_video_size((LinphoneCore *)lc, vsize);
}

extern "C" jintArray Java_org_linphone_core_LinphoneCoreImpl_getPreferredVideoSize(JNIEnv *env, jobject thiz, jlong lc){
	MSVideoSize vsize = linphone_core_get_preferred_video_size((LinphoneCore *)lc);
    jintArray arr = env->NewIntArray(2);
	int tVsize [2]= {vsize.width,vsize.height};
    env->SetIntArrayRegion(arr, 0, 2, tVsize);
    return arr;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDownloadBandwidth(JNIEnv *env, jobject thiz, jlong lc, jint bw){
	linphone_core_set_download_bandwidth((LinphoneCore *)lc, (int) bw);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUploadBandwidth(JNIEnv *env, jobject thiz, jlong lc, jint bw){
	linphone_core_set_upload_bandwidth((LinphoneCore *)lc, (int) bw);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDownloadPtime(JNIEnv *env, jobject thiz, jlong lc, jint ptime){
	linphone_core_set_download_ptime((LinphoneCore *)lc, (int) ptime);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUploadPtime(JNIEnv *env, jobject thiz, jlong lc, jint ptime){
	linphone_core_set_upload_ptime((LinphoneCore *)lc, (int) ptime);
}

extern "C" int Java_org_linphone_core_LinphoneProxyConfigImpl_getState(JNIEnv*  env,jobject thiz,jlong ptr) {
	return (int) linphone_proxy_config_get_state((const LinphoneProxyConfig *) ptr);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setExpires(JNIEnv*  env,jobject thiz,jlong ptr,jint delay) {
	linphone_proxy_config_expires((LinphoneProxyConfig *) ptr, (int) delay);
}

extern "C" jint Java_org_linphone_core_LinphoneCallImpl_getDuration(JNIEnv*  env,jobject thiz,jlong ptr) {
	linphone_call_get_duration((LinphoneCall *) ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getSignalingTransportPort(JNIEnv* env,jobject thiz,jlong ptr, jint code) {
	LCSipTransports tr;
	linphone_core_get_sip_transports((LinphoneCore *) ptr, &tr);
		switch (code) {
	case 0:
		return tr.udp_port;
	case 1:
		return tr.tcp_port;
	case 3:
		return tr.tls_port;
	default:
		return -2;
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setSignalingTransportPorts(JNIEnv*  env,jobject thiz,jlong ptr,jint udp, jint tcp, jint tls) {
	LinphoneCore *lc = (LinphoneCore *) ptr;
	LCSipTransports tr;
	tr.udp_port = udp;
	tr.tcp_port = tcp;
	tr.tls_port = tls;

	linphone_core_set_sip_transports(lc, &tr); // tr will be copied
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableIpv6(JNIEnv* env,jobject  thiz
              ,jlong lc, jboolean enable) {
              linphone_core_enable_ipv6((LinphoneCore*)lc,enable);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_adjustSoftwareVolume(JNIEnv* env,jobject  thiz
              ,jlong ptr, jint db) {
	linphone_core_set_playback_gain_db((LinphoneCore *) ptr, db);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_pauseCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	return linphone_core_pause_call((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_pauseAllCalls(JNIEnv *env,jobject thiz,jlong pCore) {
	return linphone_core_pause_all_calls((LinphoneCore *) pCore);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_resumeCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	return linphone_core_resume_call((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInConference(JNIEnv *env,jobject thiz,jlong pCore) {
	return linphone_core_is_in_conference((LinphoneCore *) pCore);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_enterConference(JNIEnv *env,jobject thiz,jlong pCore) {
	return 0 == linphone_core_enter_conference((LinphoneCore *) pCore);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_leaveConference(JNIEnv *env,jobject thiz,jlong pCore) {
	linphone_core_leave_conference((LinphoneCore *) pCore);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addAllToConference(JNIEnv *env,jobject thiz,jlong pCore) {
	linphone_core_add_all_to_conference((LinphoneCore *) pCore);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addToConference(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	linphone_core_add_to_conference((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeFromConference(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	linphone_core_remove_from_conference((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateConference(JNIEnv *env,jobject thiz,jlong pCore) {
	linphone_core_terminate_conference((LinphoneCore *) pCore);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getConferenceSize(JNIEnv *env,jobject thiz,jlong pCore) {
	return linphone_core_get_conference_size((LinphoneCore *) pCore);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateAllCalls(JNIEnv *env,jobject thiz,jlong pCore) {
	linphone_core_terminate_all_calls((LinphoneCore *) pCore);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getCall(JNIEnv *env,jobject thiz,jlong pCore,jint position) {
	LinphoneCoreData *lcd=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)pCore);
	LinphoneCall* lCall = (LinphoneCall*) ms_list_nth_data(linphone_core_get_calls((LinphoneCore *) pCore),position);
	return lcd->getCall(env,lCall);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getCallsNb(JNIEnv *env,jobject thiz,jlong pCore) {
	return ms_list_size(linphone_core_get_calls((LinphoneCore *) pCore));
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_transferCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall, jstring jReferTo) {
	const char* cReferTo=env->GetStringUTFChars(jReferTo, NULL);
	linphone_core_transfer_call((LinphoneCore *) pCore, (LinphoneCall *) pCall, cReferTo);
	env->ReleaseStringUTFChars(jReferTo, cReferTo);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_transferCallToAnother(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall, jlong pDestCall) {
	linphone_core_transfer_call_to_another((LinphoneCore *) pCore, (LinphoneCall *) pCall, (LinphoneCall *) pDestCall);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setZrtpSecretsCache(JNIEnv *env,jobject thiz,jlong pCore, jstring jFile) {
	if (jFile) {
		const char* cFile=env->GetStringUTFChars(jFile, NULL);
		linphone_core_set_zrtp_secrets_file((LinphoneCore *) pCore,cFile);
		env->ReleaseStringUTFChars(jFile, cFile);
	} else {
		linphone_core_set_zrtp_secrets_file((LinphoneCore *) pCore,NULL);
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_findCallFromUri(JNIEnv *env,jobject thiz,jlong pCore, jstring jUri) {
	const char* cUri=env->GetStringUTFChars(jUri, NULL);
	LinphoneCall *call= (LinphoneCall *) linphone_core_find_call_from_uri((LinphoneCore *) pCore,cUri);
	env->ReleaseStringUTFChars(jUri, cUri);
	LinphoneCoreData *lcdata=(LinphoneCoreData*)linphone_core_get_user_data((LinphoneCore*)pCore);
	return (jobject) lcdata->getCall(env,call);
}


extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_setVideoDevice(JNIEnv *env,jobject thiz,jlong pCore,jint id) {
	LinphoneCore* lc = (LinphoneCore *) pCore;
	const char** devices = linphone_core_get_video_devices(lc);
	if (devices == NULL) {
		ms_error("No existing video devices\n");
		return -1;
	}
	int i;
	for(i=0; i<=id; i++) {
		if (devices[i] == NULL)
			break;
		ms_message("Existing device %d : %s\n", i, devices[i]);
		if (i==id) {
			return linphone_core_set_video_device(lc, devices[i]);
		}
	}
	return -1;
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getVideoDevice(JNIEnv *env,jobject thiz,jlong pCore) {
	LinphoneCore* lc = (LinphoneCore *) pCore;
	const char** devices = linphone_core_get_video_devices(lc);
	if (devices == NULL) {
		ms_error("No existing video devices\n");
		return -1;
	}
	const char* cam = linphone_core_get_video_device(lc);
	if (cam == NULL) {
		ms_error("No current video device\n");
	} else {
		int i=0;
		while(devices[i] != NULL) {
			if (strcmp(devices[i], cam) == 0)
				return i;
			i++;
		}
	}
	ms_warning("Returning default (0) device\n");
	return 0;
}

extern "C" jstring Java_org_linphone_core_LinphoneCallImpl_getAuthenticationToken(JNIEnv*  env,jobject thiz,jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	const char* token = linphone_call_get_authentication_token(call);
	if (token == NULL) return NULL;
	return env->NewStringUTF(token);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_isAuthenticationTokenVerified(JNIEnv*  env,jobject thiz,jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	return linphone_call_get_authentication_token_verified(call);
}
extern "C" void Java_org_linphone_core_LinphoneCallImpl_setAuthenticationTokenVerified(JNIEnv*  env,jobject thiz,jlong ptr,jboolean verified) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	linphone_call_set_authentication_token_verified(call, verified);
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallImpl_getPlayVolume(JNIEnv* env, jobject thiz, jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	return linphone_call_get_play_volume(call);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_soundResourcesLocked(JNIEnv* env,jobject thiz,jlong ptr){
	return linphone_core_sound_resources_locked((LinphoneCore *) ptr);
}

// Needed by Galaxy S (can't switch to/from speaker while playing and still keep mic working)
// Implemented directly in msandroid.cpp (sound filters for Android).
extern "C" void msandroid_hack_speaker_state(bool speakerOn);

extern "C" void Java_org_linphone_LinphoneManager_hackSpeakerState(JNIEnv*  env,jobject thiz,jboolean speakerOn){
	msandroid_hack_speaker_state(speakerOn);
}
// End Galaxy S hack functions


extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getMaxCalls(JNIEnv *env,jobject thiz,jlong pCore) {
	return (jint) linphone_core_get_max_calls((LinphoneCore *) pCore);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMaxCalls(JNIEnv *env,jobject thiz,jlong pCore, jint max) {
	linphone_core_set_max_calls((LinphoneCore *) pCore, (int) max);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelAddServerAndMirror(JNIEnv *env,jobject thiz,jlong pCore,
		jstring jHost, jint port, jint mirror, jint delay) {
#ifdef TUNNEL_ENABLED
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	const char* cHost=env->GetStringUTFChars(jHost, NULL);
	linphone_tunnel_add_server_and_mirror(tunnel, cHost, port, mirror, delay);
	env->ReleaseStringUTFChars(jHost, cHost);
#endif
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelAutoDetect(JNIEnv *env,jobject thiz,jlong pCore) {
#ifdef TUNNEL_ENABLED
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_auto_detect(tunnel);
#endif
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelCleanServers(JNIEnv *env,jobject thiz,jlong pCore) {
#ifdef TUNNEL_ENABLED
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_clean_servers(tunnel);
#endif
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelEnable(JNIEnv *env,jobject thiz,jlong pCore, jboolean enable) {
#ifdef TUNNEL_ENABLED
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_enable(tunnel, enable);
#endif
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelEnableLogs(JNIEnv *env,jobject thiz,jlong pCore, jboolean enable) {
#ifdef TUNNEL_ENABLED
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_enable_logs(tunnel, enable);
#endif
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUserAgent(JNIEnv *env,jobject thiz,jlong pCore, jstring name, jstring version){
	const char* cname=env->GetStringUTFChars(name, NULL);
	const char* cversion=env->GetStringUTFChars(version, NULL);
	linphone_core_set_user_agent(cname,cversion);
	env->ReleaseStringUTFChars(name, cname);
	env->ReleaseStringUTFChars(version, cversion);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isTunnelAvailable(JNIEnv *env,jobject thiz){
	return linphone_core_tunnel_available();
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoPolicy(JNIEnv *env, jobject thiz, jlong lc, jboolean autoInitiate, jboolean autoAccept){
	LinphoneVideoPolicy vpol;
	vpol.automatically_initiate = autoInitiate;
	vpol.automatically_accept = autoAccept;
	linphone_core_set_video_policy((LinphoneCore *)lc, &vpol);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setCpuCountNative(JNIEnv *env, jobject thiz, jint count) {
	ms_set_cpu_count(count);
}
