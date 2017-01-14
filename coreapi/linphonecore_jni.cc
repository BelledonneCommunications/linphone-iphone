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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <jni.h>
#ifdef USE_JAVAH
#include "linphonecore_jni.h"
#endif
#include "linphone/core_utils.h"
#include <mediastreamer2/zrtp.h>


extern "C" {
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msmediaplayer.h"
#include "mediastreamer2/msutils.h"
#include "mediastreamer2/devices.h"
}
#include "mediastreamer2/msjava.h"
#include "private.h"
#include "linphone/account_creator.h"
#include <cpu-features.h>

#include "linphone/lpconfig.h"

#ifdef ANDROID
#include <android/log.h>

#include <belle-sip/wakelock.h>
#endif /*ANDROID*/

#define RETURN_USER_DATA_OBJECT(javaclass, funcprefix, cobj) \
	{ \
		jclass jUserDataObjectClass; \
		jmethodID jUserDataObjectCtor; \
		jobject jUserDataObj; \
		jUserDataObj = (jobject)funcprefix ## _get_user_data(cobj); \
		if (jUserDataObj == NULL) { \
			jUserDataObjectClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/" javaclass)); \
			jUserDataObjectCtor = env->GetMethodID(jUserDataObjectClass,"<init>", "(J)V"); \
			jUserDataObj = env->NewObject(jUserDataObjectClass, jUserDataObjectCtor, (jlong)funcprefix ## _ref(cobj)); \
			jUserDataObj = env->NewGlobalRef(jUserDataObj); \
			funcprefix ## _set_user_data(cobj, jUserDataObj); \
			env->DeleteGlobalRef(jUserDataObjectClass); \
		} \
		return jUserDataObj; \
	}

static JavaVM *jvm=0;
static const char* LogDomain = "Linphone";
static jclass handler_class;
static jmethodID loghandler_id;
static jobject handler_obj=NULL;

static jobject create_java_linphone_content(JNIEnv *env, const LinphoneContent *content);
static jobject create_java_linphone_buffer(JNIEnv *env, const LinphoneBuffer *buffer);
static LinphoneBuffer* create_c_linphone_buffer_from_java_linphone_buffer(JNIEnv *env, jobject jbuffer);
static jobject getTunnelConfig(JNIEnv *env, LinphoneTunnelConfig *cfg);

#ifdef ANDROID
void linphone_android_log_handler(int prio, char *str) {
	char *current;
	char *next;

	if (strlen(str) < 512) {
		__android_log_write(prio, LogDomain, str);
	} else {
		current = str;
		while ((next = strchr(current, '\n')) != NULL) {

			*next = '\0';
			if (next != str && next[-1] == '\r')
				next[-1] = '\0';
			__android_log_write(prio, LogDomain, current);
			current = next + 1;
		}
		__android_log_write(prio, LogDomain, current);
	}
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreFactoryImpl__1setLogHandler(JNIEnv *env, jobject jfactory, jobject jhandler) {
	if (handler_obj) {
		env->DeleteGlobalRef(handler_obj);
		handler_obj = NULL;
	}
	if (jhandler) {
		handler_class = (jclass) env->GetObjectClass(jhandler);
		loghandler_id = env->GetMethodID(handler_class, "log", "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/Throwable;)V");

		if (loghandler_id == NULL) {
			ms_fatal("log method not found");
		}
		handler_obj = env->NewGlobalRef(jhandler);
	}
}

static void linphone_android_ortp_log_handler(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
	char str[4096];
	const char *levname = "undef";
	vsnprintf(str, sizeof(str) - 1, fmt, args);
	str[sizeof(str) - 1] = '\0';

	int prio;
	switch(lev) {
		case ORTP_DEBUG:	prio = ANDROID_LOG_DEBUG;	levname="debug"; break;
		case ORTP_MESSAGE:	prio = ANDROID_LOG_INFO;	levname="message"; break;
		case ORTP_WARNING:	prio = ANDROID_LOG_WARN;	levname="warning"; break;
		case ORTP_ERROR:	prio = ANDROID_LOG_ERROR;	levname="error"; break;
		case ORTP_FATAL:	prio = ANDROID_LOG_FATAL;	levname="fatal"; break;
		default:			prio = ANDROID_LOG_DEFAULT;	break;
	}

	if (handler_obj) {
		JNIEnv *env = ms_get_jni_env();
		jstring jdomain = env->NewStringUTF(LogDomain);
		jstring jlevname = env->NewStringUTF(levname);
		jstring jstr = env->NewStringUTF(str);
		env->CallVoidMethod(handler_obj, loghandler_id, jdomain, (jint)lev, jlevname, jstr, NULL);
		if (jdomain) env->DeleteLocalRef(jdomain);
		if (jlevname) env->DeleteLocalRef(jlevname);
		if (jstr) env->DeleteLocalRef(jstr);
	} else {
		linphone_android_log_handler(prio, str);
	}
}

int dumbMethodForAllowingUsageOfCpuFeaturesFromStaticLibMediastream() {
	return (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM && (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0);
}

int dumbMethodForAllowingUsageOfMsAudioDiffFromStaticLibMediastream() {
	return ms_audio_diff(NULL, NULL, NULL, 0, NULL, NULL);
}

extern "C" void setAndroidLogHandler() {
	linphone_core_enable_logs_with_cb(linphone_android_ortp_log_handler);
}

extern "C" void setMediastreamerAndroidContext(JNIEnv *env, void *context) {
	jclass ms_class = env->FindClass("org/linphone/mediastream/MediastreamerAndroidContext");
	jmethodID set_context = env->GetStaticMethodID(ms_class, "setContext", "(Ljava/lang/Object;)V");
	env->CallStaticVoidMethod(ms_class, set_context, (jobject)context);
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

static const char* GetStringUTFChars(JNIEnv* env, jstring string) {
	const char *cstring = string ? env->GetStringUTFChars(string, NULL) : NULL;
	return cstring;
}

static void ReleaseStringUTFChars(JNIEnv* env, jstring string, const char *cstring) {
	if (string) env->ReleaseStringUTFChars(string, cstring);
}

//LinphoneFactory
extern "C" void Java_org_linphone_core_LinphoneCoreFactoryImpl_setDebugMode(JNIEnv*  env
		,jobject  thiz
		,jboolean isDebug
		,jstring  jdebugTag) {
	if (isDebug) {
		LogDomain = GetStringUTFChars(env, jdebugTag);
		linphone_core_enable_logs_with_cb(linphone_android_ortp_log_handler);
	} else {
		linphone_core_disable_logs();
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreFactoryImpl__1createTunnelConfig(JNIEnv*  env, jobject  thiz){
	jobject jobj;
	LinphoneTunnelConfig *cfg =  linphone_tunnel_config_new();
	jobj = getTunnelConfig(env, cfg); //this will take a ref.
	linphone_tunnel_config_unref(cfg);
	return jobj;
}

extern "C" void Java_org_linphone_core_LinphoneCoreFactoryImpl_enableLogCollection(JNIEnv* env
		,jobject  thiz
		,jboolean enable) {
	linphone_core_enable_log_collection(enable ? LinphoneLogCollectionEnabledWithoutPreviousLogHandler : LinphoneLogCollectionDisabled);
}

extern "C" void Java_org_linphone_core_LinphoneCoreFactoryImpl_setLogCollectionPath(JNIEnv* env
		,jobject  thiz
		,jstring jpath) {

	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_log_collection_path(path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreFactoryImpl_getAllDialPlanNative(JNIEnv *env, jobject thiz) {
	LinphoneDialPlan *countries;
	jclass addr_class = env->FindClass("org/linphone/core/DialPlanImpl");
	jmethodID addr_constructor = env->GetMethodID(addr_class, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)V");
	jobjectArray jaddr_array;
	int i, size = 0;
	countries = (LinphoneDialPlan *)linphone_dial_plan_get_all();

	while (countries[size].country != NULL) size++;

	jaddr_array = env->NewObjectArray(size, addr_class, NULL);

	for (i=0; i < size ; i++) {
		jstring jcountry = env->NewStringUTF(countries[i].country);
		jstring jiso = env->NewStringUTF(countries[i].iso_country_code);
		jstring jccc = env->NewStringUTF(countries[i].ccc);
		jint jnnl = (jint)countries[i].nnl;
		jstring jicp = env->NewStringUTF(countries[i].icp);

		jobject jaddr = env->NewObject(addr_class, addr_constructor, jcountry, jiso, jccc, jnnl, jicp);

		env->SetObjectArrayElement(jaddr_array, i, jaddr);

		env->DeleteLocalRef(jcountry);
		env->DeleteLocalRef(jiso);
		env->DeleteLocalRef(jccc);
		env->DeleteLocalRef(jicp);
	}
	return jaddr_array;
}

// LinphoneCore

class LinphoneJavaBindings {
public:
	LinphoneJavaBindings(JNIEnv *env) {
		listenerClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCoreListener"));

		authMethodClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$AuthMethod"));
		authMethodFromIntId = env->GetStaticMethodID(authMethodClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$AuthMethod;");

		/*displayStatus(LinphoneCore lc,String message);*/
		displayStatusId = env->GetMethodID(listenerClass,"displayStatus","(Lorg/linphone/core/LinphoneCore;Ljava/lang/String;)V");

		/*void generalState(LinphoneCore lc,int state); */
		globalStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$GlobalState"));
		globalStateFromIntId = env->GetStaticMethodID(globalStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$GlobalState;");
		globalStateId = env->GetMethodID(listenerClass,"globalState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$GlobalState;Ljava/lang/String;)V");

		/*registrationState(LinphoneCore lc, LinphoneProxyConfig cfg, LinphoneCore.RegistrationState cstate, String smessage);*/
		registrationStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$RegistrationState"));
		registrationStateFromIntId = env->GetStaticMethodID(registrationStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$RegistrationState;");
		registrationStateId = env->GetMethodID(listenerClass,"registrationState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneProxyConfig;Lorg/linphone/core/LinphoneCore$RegistrationState;Ljava/lang/String;)V");

		/*callState(LinphoneCore lc, LinphoneCall call, LinphoneCall.State cstate,String message);*/
		callStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCall$State"));
		callStateFromIntId = env->GetStaticMethodID(callStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCall$State;");
		callStateId = env->GetMethodID(listenerClass,"callState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;Lorg/linphone/core/LinphoneCall$State;Ljava/lang/String;)V");

		transferStateId = env->GetMethodID(listenerClass,"transferState","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;Lorg/linphone/core/LinphoneCall$State;)V");

		/*callStatsUpdated(LinphoneCore lc, LinphoneCall call, LinphoneCallStats stats);*/
		callStatsUpdatedId = env->GetMethodID(listenerClass, "callStatsUpdated", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;Lorg/linphone/core/LinphoneCallStats;)V");

		/*callEncryption(LinphoneCore lc, LinphoneCall call, boolean encrypted,String auth_token);*/
		callEncryptionChangedId = env->GetMethodID(listenerClass,"callEncryptionChanged","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;ZLjava/lang/String;)V");

		/*void ecCalibrationStatus(LinphoneCore.EcCalibratorStatus status, int delay_ms, Object data);*/
		ecCalibratorStatusClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$EcCalibratorStatus"));
		ecCalibratorStatusFromIntId = env->GetStaticMethodID(ecCalibratorStatusClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$EcCalibratorStatus;");
		ecCalibrationStatusId = env->GetMethodID(listenerClass,"ecCalibrationStatus","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$EcCalibratorStatus;ILjava/lang/Object;)V");

		/*void newSubscriptionRequest(LinphoneCore lc, LinphoneFriend lf, String url)*/
		newSubscriptionRequestId = env->GetMethodID(listenerClass,"newSubscriptionRequest","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriend;Ljava/lang/String;)V");

		authInfoRequestedId = env->GetMethodID(listenerClass,"authInfoRequested","(Lorg/linphone/core/LinphoneCore;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		authenticationRequestedId = env->GetMethodID(listenerClass,"authenticationRequested","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneAuthInfo;Lorg/linphone/core/LinphoneCore$AuthMethod;)V");

		/*void notifyPresenceReceived(LinphoneCore lc, LinphoneFriend lf);*/
		notifyPresenceReceivedId = env->GetMethodID(listenerClass,"notifyPresenceReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriend;)V");

		messageReceivedId = env->GetMethodID(listenerClass,"messageReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatRoom;Lorg/linphone/core/LinphoneChatMessage;)V");

		isComposingReceivedId = env->GetMethodID(listenerClass,"isComposingReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatRoom;)V");

		dtmfReceivedId = env->GetMethodID(listenerClass,"dtmfReceived","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;I)V");

		infoReceivedId = env->GetMethodID(listenerClass,"infoReceived", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCall;Lorg/linphone/core/LinphoneInfoMessage;)V");

		subscriptionStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/SubscriptionState"));
		subscriptionStateFromIntId = env->GetStaticMethodID(subscriptionStateClass,"fromInt","(I)Lorg/linphone/core/SubscriptionState;");
		subscriptionStateId = env->GetMethodID(listenerClass,"subscriptionStateChanged", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneEvent;Lorg/linphone/core/SubscriptionState;)V");

		publishStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/PublishState"));
		publishStateFromIntId = env->GetStaticMethodID(publishStateClass,"fromInt","(I)Lorg/linphone/core/PublishState;");
		publishStateId = env->GetMethodID(listenerClass,"publishStateChanged", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneEvent;Lorg/linphone/core/PublishState;)V");

		notifyRecvId = env->GetMethodID(listenerClass,"notifyReceived", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneEvent;Ljava/lang/String;Lorg/linphone/core/LinphoneContent;)V");

		configuringStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$RemoteProvisioningState"));
		configuringStateFromIntId = env->GetStaticMethodID(configuringStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneCore$RemoteProvisioningState;");
		configuringStateId = env->GetMethodID(listenerClass,"configuringStatus","(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$RemoteProvisioningState;Ljava/lang/String;)V");

		fileTransferProgressIndicationId = env->GetMethodID(listenerClass, "fileTransferProgressIndication", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;I)V");

		fileTransferSendId = env->GetMethodID(listenerClass, "fileTransferSend", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;Ljava/nio/ByteBuffer;I)I");

		fileTransferRecvId = env->GetMethodID(listenerClass, "fileTransferRecv", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;[BI)V");

		logCollectionUploadStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCore$LogCollectionUploadState"));
		logCollectionUploadStateFromIntId = env->GetStaticMethodID(logCollectionUploadStateClass, "fromInt", "(I)Lorg/linphone/core/LinphoneCore$LogCollectionUploadState;");
		logCollectionUploadProgressId = env->GetMethodID(listenerClass, "uploadProgressIndication", "(Lorg/linphone/core/LinphoneCore;II)V");
		logCollectionUploadStateId = env->GetMethodID(listenerClass, "uploadStateChanged", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneCore$LogCollectionUploadState;Ljava/lang/String;)V");

		chatMessageStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneChatMessage$State"));
		chatMessageStateFromIntId = env->GetStaticMethodID(chatMessageStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneChatMessage$State;");

		authInfoClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneAuthInfoImpl"));
		authInfoCtrId = env->GetMethodID(authInfoClass,"<init>", "(J)V");

		proxyClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneProxyConfigImpl"));
		proxyCtrId = env->GetMethodID(proxyClass,"<init>", "(Lorg/linphone/core/LinphoneCoreImpl;J)V");

		callClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCallImpl"));
		callCtrId = env->GetMethodID(callClass,"<init>", "(J)V");
		callSetAudioStatsId = env->GetMethodID(callClass, "setAudioStats", "(Lorg/linphone/core/LinphoneCallStats;)V");
		callSetVideoStatsId = env->GetMethodID(callClass, "setVideoStats", "(Lorg/linphone/core/LinphoneCallStats;)V");

		chatMessageClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneChatMessageImpl"));
		if (chatMessageClass) {
			chatMessageCtrId = env->GetMethodID(chatMessageClass,"<init>", "(J)V");
		}

		chatRoomClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneChatRoomImpl"));
		chatRoomCtrId = env->GetMethodID(chatRoomClass,"<init>", "(J)V");

		friendClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneFriendImpl"));
		friendCtrId = env->GetMethodID(friendClass,"<init>", "(J)V");

		friendListClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneFriendListImpl"));
		friendListCtrId = env->GetMethodID(friendListClass,"<init>", "(J)V");
		friendListCreatedId = env->GetMethodID(listenerClass, "friendListCreated", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriendList;)V");
		friendListRemovedId = env->GetMethodID(listenerClass, "friendListRemoved", "(Lorg/linphone/core/LinphoneCore;Lorg/linphone/core/LinphoneFriendList;)V");
		friendListSyncStateClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneFriendList$State"));
		friendListSyncStateFromIntId = env->GetStaticMethodID(friendListSyncStateClass,"fromInt","(I)Lorg/linphone/core/LinphoneFriendList$State;");

		natPolicyClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneNatPolicyImpl"));
		natPolicyCtrId = env->GetMethodID(natPolicyClass, "<init>", "(J)V");

		addressClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneAddressImpl"));
		addressCtrId = env->GetMethodID(addressClass,"<init>", "(J)V");

		callStatsClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneCallStatsImpl"));
		callStatsId = env->GetMethodID(callStatsClass, "<init>", "(JJ)V");

		infoMessageClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneInfoMessageImpl"));
		infoMessageCtor = env->GetMethodID(infoMessageClass,"<init>", "(J)V");

		linphoneEventClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneEventImpl"));
		linphoneEventCtrId = env->GetMethodID(linphoneEventClass,"<init>", "(J)V");

		subscriptionDirClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/SubscriptionDir"));
		subscriptionDirFromIntId = env->GetStaticMethodID(subscriptionDirClass,"fromInt","(I)Lorg/linphone/core/SubscriptionDir;");

		msFactoryClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/mediastream/Factory"));
		msFactoryCtrId = env->GetMethodID(msFactoryClass,"<init>", "(J)V");

		accountCreatorClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneAccountCreatorImpl"));
		accountCreatorCtrId = env->GetMethodID(accountCreatorClass, "<init>", "(J)V");
		accountCreatorStatusClass = (jclass)env->NewGlobalRef(env->FindClass("org/linphone/core/LinphoneAccountCreator$Status"));
		accountCreatorStatusFromIntId = env->GetStaticMethodID(accountCreatorStatusClass,"fromInt","(I)Lorg/linphone/core/LinphoneAccountCreator$Status;");
	}

	void setCore(jobject c) {
		core = c;
	}

	jobject getCore() {
		return core;
	}

	~LinphoneJavaBindings() {
		JNIEnv *env = 0;
		jvm->AttachCurrentThread(&env,NULL);
		env->DeleteGlobalRef(listenerClass);
		env->DeleteGlobalRef(authMethodClass);
		env->DeleteGlobalRef(globalStateClass);
		env->DeleteGlobalRef(configuringStateClass);
		env->DeleteGlobalRef(registrationStateClass);
		env->DeleteGlobalRef(callStateClass);
		env->DeleteGlobalRef(chatMessageStateClass);
		env->DeleteGlobalRef(authInfoClass);
		env->DeleteGlobalRef(proxyClass);
		env->DeleteGlobalRef(callClass);
		env->DeleteGlobalRef(chatMessageClass);
		env->DeleteGlobalRef(chatRoomClass);
		env->DeleteGlobalRef(friendClass);
		env->DeleteGlobalRef(friendListClass);
		env->DeleteGlobalRef(friendListSyncStateClass);
		env->DeleteGlobalRef(natPolicyClass);
		env->DeleteGlobalRef(infoMessageClass);
		env->DeleteGlobalRef(linphoneEventClass);
		env->DeleteGlobalRef(subscriptionStateClass);
		env->DeleteGlobalRef(subscriptionDirClass);
		env->DeleteGlobalRef(logCollectionUploadStateClass);
		env->DeleteGlobalRef(msFactoryClass);
		env->DeleteGlobalRef(accountCreatorClass);
		env->DeleteGlobalRef(accountCreatorStatusClass);
	}

	jobject core;

	jclass listenerClass;
	jmethodID displayStatusId;
	jmethodID newSubscriptionRequestId;
	jmethodID notifyPresenceReceivedId;
	jmethodID messageReceivedId;
	jmethodID isComposingReceivedId;
	jmethodID dtmfReceivedId;
	jmethodID callStatsUpdatedId;
	jmethodID transferStateId;
	jmethodID infoReceivedId;
	jmethodID subscriptionStateId;
	jmethodID authInfoRequestedId;
	jmethodID authenticationRequestedId;
	jmethodID publishStateId;
	jmethodID notifyRecvId;

	jclass authMethodClass;
	jmethodID authMethodFromIntId;

	jclass configuringStateClass;
	jmethodID configuringStateId;
	jmethodID configuringStateFromIntId;

	jclass globalStateClass;
	jmethodID globalStateId;
	jmethodID globalStateFromIntId;

	jclass registrationStateClass;
	jmethodID registrationStateId;
	jmethodID registrationStateFromIntId;

	jclass callStateClass;
	jmethodID callStateId;
	jmethodID callStateFromIntId;

	jclass callStatsClass;
	jmethodID callStatsId;
	jmethodID callSetAudioStatsId;
	jmethodID callSetVideoStatsId;

	jclass chatMessageStateClass;
	jmethodID chatMessageStateFromIntId;

	jmethodID callEncryptionChangedId;

	jclass ecCalibratorStatusClass;
	jmethodID ecCalibrationStatusId;
	jmethodID ecCalibratorStatusFromIntId;

	jclass authInfoClass;
	jmethodID authInfoCtrId;

	jclass proxyClass;
	jmethodID proxyCtrId;

	jclass callClass;
	jmethodID callCtrId;

	jclass chatMessageClass;
	jmethodID chatMessageCtrId;

	jclass chatRoomClass;
	jmethodID chatRoomCtrId;

	jclass friendClass;
	jmethodID friendCtrId;

	jclass friendListClass;
	jmethodID friendListCtrId;
	jmethodID friendListCreatedId;
	jmethodID friendListRemovedId;
	jclass friendListSyncStateClass;
	jmethodID friendListSyncStateFromIntId;

	jclass natPolicyClass;
	jmethodID natPolicyCtrId;

	jclass addressClass;
	jmethodID addressCtrId;

	jclass infoMessageClass;
	jmethodID infoMessageCtor;

	jclass linphoneEventClass;
	jmethodID linphoneEventCtrId;

	jclass subscriptionStateClass;
	jmethodID subscriptionStateFromIntId;

	jclass publishStateClass;
	jmethodID publishStateFromIntId;

	jclass subscriptionDirClass;
	jmethodID subscriptionDirFromIntId;

	jmethodID fileTransferProgressIndicationId;
	jmethodID fileTransferSendId;
	jmethodID fileTransferRecvId;

	jclass logCollectionUploadStateClass;
	jmethodID logCollectionUploadStateId;
	jmethodID logCollectionUploadStateFromIntId;
	jmethodID logCollectionUploadProgressId;

	jclass msFactoryClass;
	jmethodID msFactoryCtrId;

	jclass accountCreatorClass;
	jmethodID accountCreatorCtrId;
	jclass accountCreatorStatusClass;
	jmethodID accountCreatorStatusFromIntId;
};

/*
 * returns the java LinphoneProxyConfig associated with a C LinphoneProxyConfig.
**/
jobject getProxy(JNIEnv *env, LinphoneProxyConfig *proxy, jobject core){
	jobject jobj=0;

	if (proxy!=NULL){
		LinphoneCore *lc = linphone_proxy_config_get_core(proxy);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up=linphone_proxy_config_get_user_data(proxy);

		if (up==NULL){
			jobj=env->NewObject(ljb->proxyClass, ljb->proxyCtrId, core, (jlong)proxy);
			linphone_proxy_config_set_user_data(proxy,(void*)env->NewWeakGlobalRef(jobj));
			linphone_proxy_config_ref(proxy);
		}else{
 			//promote the weak ref to local ref
			jobj=env->NewLocalRef((jobject)up);
			if (jobj == NULL){
				//the weak ref was dead
				jobj=env->NewObject(ljb->proxyClass, ljb->proxyCtrId, core, (jlong)proxy);
				linphone_proxy_config_set_user_data(proxy,(void*)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getAuthInfo(JNIEnv *env, LinphoneCore *lc, LinphoneAuthInfo *authInfo, jobject core){
	jobject jobj = 0;

	if (authInfo && lc) {
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		jobj = env->NewObject(ljb->authInfoClass, ljb->authInfoCtrId, core, (jlong)authInfo);
	}
	return jobj;
}

jobject getCall(JNIEnv *env, LinphoneCall *call){
	jobject jobj=0;

	if (call!=NULL){
		LinphoneCore *lc = linphone_call_get_core(call);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up=linphone_call_get_user_pointer(call);

		if (up==NULL){
			jobj=env->NewObject(ljb->callClass, ljb->callCtrId, (jlong)call);
			jobj=env->NewGlobalRef(jobj);
			linphone_call_set_user_pointer(call,(void*)jobj);
			linphone_call_ref(call);
		}else{
			jobj=(jobject)up;
		}
	}
	return jobj;
}

jobject getChatMessage(JNIEnv *env, LinphoneChatMessage *msg){
	jobject jobj = 0;

	if (msg != NULL){
		LinphoneChatRoom *room = linphone_chat_message_get_chat_room(msg);
		LinphoneCore *lc = linphone_chat_room_get_core(room);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up = linphone_chat_message_get_user_data(msg);

		if (up == NULL) {
			jobj = env->NewObject(ljb->chatMessageClass, ljb->chatMessageCtrId, (jlong)msg);
			void *userdata = (void*)env->NewWeakGlobalRef(jobj);
			linphone_chat_message_set_user_data(msg, userdata);
			linphone_chat_message_ref(msg);
		} else {
			jobj = env->NewLocalRef((jobject)up);
			if (jobj == NULL) {
				// takes implicit local ref
				jobj = env->NewObject(ljb->chatMessageClass, ljb->chatMessageCtrId, (jlong)msg);
				linphone_chat_message_set_user_data(msg, (void*)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getChatRoom(JNIEnv *env, LinphoneChatRoom *room) {
	jobject jobj = 0;

	if (room != NULL){
		LinphoneCore *lc = linphone_chat_room_get_core(room);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up = linphone_chat_room_get_user_data(room);
		if (up == NULL) {
			// take implicit local ref
			jobj = env->NewObject(ljb->chatRoomClass, ljb->chatRoomCtrId, (jlong)room);
			linphone_chat_room_set_user_data(room, (void*)env->NewWeakGlobalRef(jobj));
			linphone_chat_room_ref(room);
		} else {
			jobj = env->NewLocalRef((jobject)up);
			if (jobj == NULL) {
				// takes implicit local ref
				jobj = env->NewObject(ljb->chatRoomClass, ljb->chatRoomCtrId, (jlong)room);
				linphone_chat_room_set_user_data(room, (void*)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getFriend(JNIEnv *env, LinphoneFriend *lfriend){
	jobject jobj=0;


	if (lfriend != NULL){
		LinphoneCore *lc = linphone_friend_get_core(lfriend);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up=linphone_friend_get_user_data(lfriend);

		if (up == NULL){

			// take implicit local ref
			jobj=env->NewObject(ljb->friendClass, ljb->friendCtrId, (jlong)lfriend);
			linphone_friend_set_user_data(lfriend,(void*)env->NewWeakGlobalRef(jobj));
			linphone_friend_ref(lfriend);
		}else{

			jobj=env->NewLocalRef((jobject)up);
			if (jobj == NULL){
				// takes implicit local ref
				jobj=env->NewObject(ljb->friendClass, ljb->friendCtrId, (jlong)lfriend);
				linphone_friend_set_user_data(lfriend,(void*)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getFriendList(JNIEnv *env, LinphoneFriendList *lfriendList){
	jobject jobj=0;

	if (lfriendList != NULL){
		LinphoneCore *lc = linphone_friend_list_get_core(lfriendList);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up=linphone_friend_list_get_user_data(lfriendList);

		if (up == NULL){
			jobj=env->NewObject(ljb->friendListClass, ljb->friendListCtrId, (jlong)lfriendList);
			linphone_friend_list_set_user_data(lfriendList,(void*)env->NewWeakGlobalRef(jobj));
			linphone_friend_list_ref(lfriendList);
		}else{

			jobj=env->NewLocalRef((jobject)up);
			if (jobj == NULL){
				jobj=env->NewObject(ljb->friendListClass, ljb->friendListCtrId, (jlong)lfriendList);
				linphone_friend_list_set_user_data(lfriendList,(void*)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getNatPolicy(JNIEnv *env, LinphoneNatPolicy *lNatPolicy) {
	jobject jobj = 0;

	if (lNatPolicy != NULL) {
		LinphoneCore *lc = lNatPolicy->lc;
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up = linphone_nat_policy_get_user_data(lNatPolicy);
		if (up == NULL) {
			jobj = env->NewObject(ljb->natPolicyClass, ljb->natPolicyCtrId, (jlong)lNatPolicy);
			linphone_nat_policy_set_user_data(lNatPolicy, (void *)env->NewWeakGlobalRef(jobj));
			linphone_nat_policy_ref(lNatPolicy);
		} else {
			jobj = env->NewLocalRef((jobject)up);
			if (jobj == NULL) {
				jobj = env->NewObject(ljb->natPolicyClass, ljb->natPolicyCtrId, (jlong)lNatPolicy);
				linphone_nat_policy_set_user_data(lNatPolicy, (void *)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getEvent(JNIEnv *env, LinphoneEvent *lev){
	if (lev==NULL) return NULL;
	jobject jev=(jobject)linphone_event_get_user_data(lev);
	if (jev==NULL){
		LinphoneCore *lc = linphone_event_get_core(lev);
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		jev=env->NewObject(ljb->linphoneEventClass, ljb->linphoneEventCtrId, (jlong)linphone_event_ref(lev));
		jev=env->NewGlobalRef(jev);
		linphone_event_set_user_data(lev,jev);
	}
	return jev;
}

jobject getXmlRpcRequest(JNIEnv *env, LinphoneXmlRpcRequest *lrequest) {
	jobject jobj = 0;

	if (lrequest != NULL) {
		jclass xmlRpcSessionClass = (jclass)env->FindClass("org/linphone/core/LinphoneXmlRpcRequestImpl");
		jmethodID xmlRpcSessionCtrId = env->GetMethodID(xmlRpcSessionClass, "<init>", "(J)V");

		void *up = linphone_xml_rpc_request_get_user_data(lrequest);
		if (up == NULL) {
			jobj = env->NewObject(xmlRpcSessionClass, xmlRpcSessionCtrId, (jlong)lrequest);
			linphone_xml_rpc_request_set_user_data(lrequest, (void *)env->NewWeakGlobalRef(jobj));
			linphone_xml_rpc_request_ref(lrequest);
		} else {
			jobj = env->NewLocalRef((jobject)up);
			if (jobj == NULL) {
				jobj = env->NewObject(xmlRpcSessionClass, xmlRpcSessionCtrId, (jlong)lrequest);
				linphone_xml_rpc_request_set_user_data(lrequest, (void *)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

jobject getAccountCreator(JNIEnv *env, LinphoneAccountCreator *account_creator) {
	jobject jobj = 0;

	if (account_creator != NULL) {
		LinphoneCore *lc = account_creator->core;
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

		void *up = linphone_account_creator_get_user_data(account_creator);
		if (up == NULL) {
			jobj = env->NewObject(ljb->accountCreatorClass, ljb->accountCreatorCtrId, (jlong)account_creator);
			linphone_account_creator_set_user_data(account_creator, (void *)env->NewWeakGlobalRef(jobj));
			linphone_account_creator_ref(account_creator);
		} else {
			jobj = env->NewLocalRef((jobject)up);
			if (jobj == NULL) {
				jobj = env->NewObject(ljb->accountCreatorClass, ljb->accountCreatorCtrId, (jlong)account_creator);
				linphone_account_creator_set_user_data(account_creator, (void *)env->NewWeakGlobalRef(jobj));
			}
		}
	}
	return jobj;
}

class LinphoneCoreData {
public:
	LinphoneCoreData(JNIEnv *env, jobject lc, LinphoneCoreVTable *vTable, jobject alistener, LinphoneJavaBindings *ljb) {
		core = env->NewGlobalRef(lc);
		listener = env->NewGlobalRef(alistener);

		memset(vTable, 0, sizeof(LinphoneCoreVTable));

		if (ljb->displayStatusId) {
			vTable->display_status = displayStatusCb;
		}

		if (ljb->globalStateId) {
			vTable->global_state_changed = globalStateChange;
		}

		if (ljb->registrationStateId) {
			vTable->registration_state_changed = registrationStateChange;
		}

		if (ljb->callStateId) {
			vTable->call_state_changed = callStateChange;
		}

		if (ljb->transferStateId) {
			vTable->transfer_state_changed = transferStateChanged;
		}

		if (ljb->callStatsUpdatedId) {
			vTable->call_stats_updated = callStatsUpdated;
		}

		if (ljb->callEncryptionChangedId) {
			vTable->call_encryption_changed = callEncryptionChange;
		}

		if (ljb->newSubscriptionRequestId) {
			vTable->new_subscription_requested = new_subscription_requested;
		}

		if (ljb->authInfoRequestedId) {
			vTable->auth_info_requested = authInfoRequested;
		}

		if (ljb->authenticationRequestedId) {
			vTable->authentication_requested = authenticationRequested;
		}

		if (ljb->notifyPresenceReceivedId) {
			vTable->notify_presence_received = notify_presence_received;
		}

		if (ljb->messageReceivedId) {
			vTable->message_received = message_received;
		}

		if (ljb->isComposingReceivedId) {
			vTable->is_composing_received = is_composing_received;
		}

		if (ljb->dtmfReceivedId) {
			vTable->dtmf_received = dtmf_received;
		}

		if (ljb->infoReceivedId) {
			vTable->info_received = infoReceived;
		}

		if (ljb->subscriptionStateId) {
			vTable->subscription_state_changed = subscriptionStateChanged;
		}

		if (ljb->publishStateId) {
			vTable->publish_state_changed = publishStateChanged;
		}

		if (ljb->notifyRecvId) {
			vTable->notify_received = notifyReceived;
		}

		if (ljb->configuringStateId) {
			vTable->configuring_status = configuringStatus;
		}

		if (ljb->fileTransferProgressIndicationId) {
			vTable->file_transfer_progress_indication = fileTransferProgressIndication;
		}

		if (ljb->fileTransferSendId) {
			vTable->file_transfer_send = fileTransferSend;
		}

		if (ljb->fileTransferRecvId) {
			vTable->file_transfer_recv = fileTransferRecv;
		}

		if (ljb->logCollectionUploadProgressId) {
			vTable->log_collection_upload_progress_indication = logCollectionUploadProgressIndication;
		}
		if (ljb->logCollectionUploadStateId) {
			vTable->log_collection_upload_state_changed = logCollectionUploadStateChange;
		}

		if (ljb->friendListCreatedId) {
			vTable->friend_list_created = friendListCreated;
		}
		if (ljb->friendListRemovedId) {
			vTable->friend_list_removed = friendListRemoved;
		}
	}

	~LinphoneCoreData() {
		JNIEnv *env = 0;
		jvm->AttachCurrentThread(&env,NULL);
		env->DeleteGlobalRef(core);
		env->DeleteGlobalRef(listener);
	}

	jobject core;
	jobject listener;

	LinphoneCoreVTable vTable;

	static void displayStatusCb(LinphoneCore *lc, const char *message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jstring msg = message ? env->NewStringUTF(message) : NULL;
		env->CallVoidMethod(lcData->listener,ljb->displayStatusId,lcData->core,msg);
		handle_possible_java_exception(env, lcData->listener);
		if (msg) {
			env->DeleteLocalRef(msg);
		}
	}
	static void authInfoRequested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jstring r = realm ? env->NewStringUTF(realm) : NULL;
		jstring u = username ? env->NewStringUTF(username) : NULL;
		jstring d = domain ? env->NewStringUTF(domain) : NULL;
		env->CallVoidMethod(lcData->listener,
							ljb->authInfoRequestedId,
							lcData->core,
							r,
							u,
							d
   						);
		handle_possible_java_exception(env, lcData->listener);
		if (r) {
			env->DeleteLocalRef(r);
		}
		if (u) {
			env->DeleteLocalRef(u);
		}
		if (d) {
			env->DeleteLocalRef(d);
		}
	}
	static void authenticationRequested(LinphoneCore *lc, LinphoneAuthInfo *auth_info, LinphoneAuthMethod method) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener,
							ljb->authenticationRequestedId,
							lcData->core,
							getAuthInfo(env, lc, auth_info, lcData->core),
							env->CallStaticObjectMethod(ljb->authMethodClass,ljb->authMethodFromIntId,(jint)method)
   						);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void setCoreIfNotDone(JNIEnv *env, jobject jcore, LinphoneCore *lc){
		jclass objClass = env->GetObjectClass(jcore);
		jfieldID myFieldID = env->GetFieldID(objClass, "nativePtr", "J");
		jlong fieldVal = env->GetLongField(jcore, myFieldID);
		if (fieldVal == 0){
			env->SetLongField(jcore, myFieldID, (jlong)lc);
		}
	}

	static void globalStateChange(LinphoneCore *lc, LinphoneGlobalState gstate,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);

		jobject jcore = lcData->core;
		/*at this stage, the java object may not be aware of its C object, because linphone_core_new() hasn't returned yet.*/
		setCoreIfNotDone(env,jcore, lc);

		jstring msg = message ? env->NewStringUTF(message) : NULL;
		env->CallVoidMethod(lcData->listener
							,ljb->globalStateId
							,lcData->core
							,env->CallStaticObjectMethod(ljb->globalStateClass,ljb->globalStateFromIntId,(jint)gstate),
							msg);
		handle_possible_java_exception(env, lcData->listener);
		if (msg) {
			env->DeleteLocalRef(msg);
		}
	}
	static void registrationStateChange(LinphoneCore *lc, LinphoneProxyConfig* proxy,LinphoneRegistrationState state,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jproxy;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jstring msg = message ? env->NewStringUTF(message) : NULL;
		env->CallVoidMethod(lcData->listener
							,ljb->registrationStateId
							,lcData->core
							,(jproxy=getProxy(env,proxy,lcData->core))
							,env->CallStaticObjectMethod(ljb->registrationStateClass,ljb->registrationStateFromIntId,(jint)state),
							msg);
		handle_possible_java_exception(env, lcData->listener);
		if (msg) {
			env->DeleteLocalRef(msg);
		}
		if (jproxy) {
			env->DeleteLocalRef(jproxy);
		}
	}

	static void callStateChange(LinphoneCore *lc, LinphoneCall* call,LinphoneCallState state,const char* message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jcall;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jstring msg = message ? env->NewStringUTF(message) : NULL;
		env->CallVoidMethod(lcData->listener
							,ljb->callStateId
							,lcData->core
							,(jcall=getCall(env,call))
							,env->CallStaticObjectMethod(ljb->callStateClass,ljb->callStateFromIntId,(jint)state),
							msg);
		handle_possible_java_exception(env, lcData->listener);
		if (state==LinphoneCallReleased) {
			linphone_call_set_user_pointer(call,NULL);
			env->DeleteGlobalRef(jcall);
		}
		if (msg) {
			env->DeleteLocalRef(msg);
		}
	}
	static void callEncryptionChange(LinphoneCore *lc, LinphoneCall* call, bool_t encrypted,const char* authentication_token) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->callEncryptionChangedId
							,lcData->core
							,getCall(env,call)
							,encrypted
							,authentication_token ? env->NewStringUTF(authentication_token) : NULL);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void notify_presence_received(LinphoneCore *lc,  LinphoneFriend *my_friend) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jfriend = NULL;

		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jfriend = getFriend(env, my_friend);
		env->CallVoidMethod(lcData->listener
							,ljb->notifyPresenceReceivedId
							,lcData->core
							,jfriend);
		handle_possible_java_exception(env, lcData->listener);
		env->DeleteLocalRef(jfriend);
	}
	static void new_subscription_requested(LinphoneCore *lc,  LinphoneFriend *my_friend, const char* url) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jobject jfriend =  getFriend(env, my_friend);
		env->CallVoidMethod(lcData->listener
							,ljb->newSubscriptionRequestId
							,lcData->core
							,jfriend
							,url ? env->NewStringUTF(url) : NULL);
		env->DeleteLocalRef(jfriend);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void dtmf_received(LinphoneCore *lc, LinphoneCall *call, int dtmf) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->dtmfReceivedId
							,lcData->core
							,getCall(env,call)
							,dtmf);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *msg) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		jobject jmsg;
		jobject jroom;
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		/*note: we call linphone_chat_message_ref() because the application does not acquire the object when invoked from a callback*/
		env->CallVoidMethod(lcData->listener
							,ljb->messageReceivedId
							,lcData->core
							,(jroom = getChatRoom(env, room))
							,(jmsg = getChatMessage(env, msg)));
		handle_possible_java_exception(env, lcData->listener);

		if (jmsg) {
			env->DeleteLocalRef(jmsg);
		}
		if (jroom) {
			env->DeleteLocalRef(jroom);
		}
	}
	static void is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		jobject jroom;
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->isComposingReceivedId
							,lcData->core
							,(jroom = getChatRoom(env, room)));
		handle_possible_java_exception(env, lcData->listener);

		if (jroom) {
			env->DeleteLocalRef(jroom);
		}
	}
	static void ecCalibrationStatus(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay_ms, void *data) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = (LinphoneCoreVTable*) data;
		if (table && ljb) {
			LinphoneCoreData* lcData = (LinphoneCoreData*) linphone_core_v_table_get_user_data(table);
			if (ljb->ecCalibrationStatusId) {
				jobject state = env->CallStaticObjectMethod(ljb->ecCalibratorStatusClass, ljb->ecCalibratorStatusFromIntId, (jint)status);
				env->CallVoidMethod(lcData->listener
								,ljb->ecCalibrationStatusId
								,lcData->core
								,state
								,delay_ms
								,NULL);
				handle_possible_java_exception(env, lcData->listener);
			}
			if (status != LinphoneEcCalibratorInProgress) {
				linphone_core_v_table_destroy(table);
			}
		}

	}
	static void callStatsUpdated(LinphoneCore *lc, LinphoneCall* call, const LinphoneCallStats *stats) {
		JNIEnv *env = 0;
		jobject statsobj;
		jobject callobj;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		statsobj = env->NewObject(ljb->callStatsClass, ljb->callStatsId, (jlong)call, (jlong)stats);
		callobj = getCall(env, call);
		if (stats->type == LINPHONE_CALL_STATS_AUDIO)
			env->CallVoidMethod(callobj, ljb->callSetAudioStatsId, statsobj);
		else if (stats->type == LINPHONE_CALL_STATS_VIDEO){
			env->CallVoidMethod(callobj, ljb->callSetVideoStatsId, statsobj);
		}else{
			//text stats not updated yet.
		}
		env->CallVoidMethod(lcData->listener, ljb->callStatsUpdatedId, lcData->core, callobj, statsobj);
		handle_possible_java_exception(env, lcData->listener);
		if (statsobj) env->DeleteLocalRef(statsobj);
	}
	static void transferStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState remote_call_state){
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jcall;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->transferStateId
							,lcData->core
							,(jcall=getCall(env,call))
							,env->CallStaticObjectMethod(ljb->callStateClass,ljb->callStateFromIntId,(jint)remote_call_state)
							);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void infoReceived(LinphoneCore *lc, LinphoneCall*call, const LinphoneInfoMessage *info){
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneInfoMessage *copy_info=linphone_info_message_copy(info);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->infoReceivedId
							,lcData->core
							,getCall(env,call)
							,env->NewObject(ljb->infoMessageClass,ljb->infoMessageCtor,(jlong)copy_info)
							);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void subscriptionStateChanged(LinphoneCore *lc, LinphoneEvent *ev, LinphoneSubscriptionState state){
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jevent;
		jobject jstate;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jevent=getEvent(env,ev);
		jstate=env->CallStaticObjectMethod(ljb->subscriptionStateClass,ljb->subscriptionStateFromIntId,(jint)state);
		env->CallVoidMethod(lcData->listener
							,ljb->subscriptionStateId
							,lcData->core
							,jevent
							,jstate
							);
		handle_possible_java_exception(env, lcData->listener);
		if (state==LinphoneSubscriptionTerminated){
			/*loose the java reference */
			linphone_event_set_user_data(ev,NULL);
			env->DeleteGlobalRef(jevent);
		}
	}
	static void publishStateChanged(LinphoneCore *lc, LinphoneEvent *ev, LinphonePublishState state){
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jevent;
		jobject jstate;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jevent=getEvent(env,ev);
		jstate=env->CallStaticObjectMethod(ljb->publishStateClass,ljb->publishStateFromIntId,(jint)state);
		env->CallVoidMethod(lcData->listener
							,ljb->publishStateId
							,lcData->core
							,jevent
							,jstate
							);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void notifyReceived(LinphoneCore *lc, LinphoneEvent *ev, const char *evname, const LinphoneContent *content){
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		jobject jevent;
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jevent=getEvent(env,ev);
		env->CallVoidMethod(lcData->listener
							,ljb->notifyRecvId
							,lcData->core
							,jevent
							,env->NewStringUTF(evname)
							,content ? create_java_linphone_content(env,content) : NULL
							);
		handle_possible_java_exception(env, lcData->listener);
	}

	static void configuringStatus(LinphoneCore *lc, LinphoneConfiguringState status, const char *message) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener, ljb->configuringStateId, lcData->core, env->CallStaticObjectMethod(ljb->configuringStateClass,ljb->configuringStateFromIntId,(jint)status), message ? env->NewStringUTF(message) : NULL);
		handle_possible_java_exception(env, lcData->listener);
	}

	static void fileTransferProgressIndication(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total) {
		JNIEnv *env = 0;
		jobject jmsg;
		size_t progress = (offset * 100) / total;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;
		env->CallVoidMethod(lcData->listener,
				ljb->fileTransferProgressIndicationId,
				lcData->core,
				(jmsg = getChatMessage(env, message)),
				jcontent,
				progress);
		if (jcontent) {
			env->DeleteLocalRef(jcontent);
		}
		if (jmsg) {
			env->DeleteLocalRef(jmsg);
		}
		handle_possible_java_exception(env, lcData->listener);
	}

	static void fileTransferSend(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, char* buff, size_t* size) {
		JNIEnv *env = 0;
		jobject jmsg;
		size_t asking = *size;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;
		jobject jbuffer = buff ? env->NewDirectByteBuffer(buff, asking) : NULL;
		*size = env->CallIntMethod(lcData->listener,
				ljb->fileTransferSendId,
				lcData->core,
				(jmsg = getChatMessage(env, message)),
				jcontent,
				jbuffer,
				asking);
		if (jcontent) {
			env->DeleteLocalRef(jcontent);
		}
		if (jbuffer) {
			env->DeleteLocalRef(jbuffer);
		}
		if (jmsg) {
			env->DeleteLocalRef(jmsg);
		}
		handle_possible_java_exception(env, lcData->listener);
	}

	static void fileTransferRecv(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size) {
		JNIEnv *env = 0;
		jobject jmsg;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);

		jbyteArray jbytes = env->NewByteArray(size);
		env->SetByteArrayRegion(jbytes, 0, size, (jbyte*)buff);
		jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;

		env->CallVoidMethod(lcData->listener,
				ljb->fileTransferRecvId,
				lcData->core,
				(jmsg = getChatMessage(env, message)),
				jcontent,
				jbytes,
				size);
		if (jcontent) {
			env->DeleteLocalRef(jcontent);
		}
		if (jmsg) {
			env->DeleteLocalRef(jmsg);
		}
		handle_possible_java_exception(env, lcData->listener);
	}
	static void logCollectionUploadProgressIndication(LinphoneCore *lc, size_t offset, size_t total) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->logCollectionUploadProgressId
							,lcData->core
							,(jlong)offset
							,(jlong)total);
		handle_possible_java_exception(env, lcData->listener);
	}
	static void logCollectionUploadStateChange(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		jstring msg = info ? env->NewStringUTF(info) : NULL;
		env->CallVoidMethod(lcData->listener
							,ljb->logCollectionUploadStateId
							,lcData->core
							,env->CallStaticObjectMethod(ljb->logCollectionUploadStateClass,ljb->logCollectionUploadStateFromIntId,(jint)state),
							msg);
		handle_possible_java_exception(env, lcData->listener);
		if (msg) {
			env->DeleteLocalRef(msg);
		}
	}
	static void friendListCreated(LinphoneCore *lc, LinphoneFriendList *list) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->friendListCreatedId
							,lcData->core
							,getFriendList(env, list));
		handle_possible_java_exception(env, lcData->listener);
	}
	static void friendListRemoved(LinphoneCore *lc, LinphoneFriendList *list) {
		JNIEnv *env = 0;
		jint result = jvm->AttachCurrentThread(&env,NULL);
		if (result != 0) {
			ms_error("cannot attach VM");
			return;
		}

		jobject jfriendlist;
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
		LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
		env->CallVoidMethod(lcData->listener
							,ljb->friendListRemovedId
							,lcData->core
							,(jfriendlist = getFriendList(env, list)));
		handle_possible_java_exception(env, lcData->listener);

		if (jfriendlist) {
			env->DeleteLocalRef(jfriendlist);
		}
	}

private:
	static inline void handle_possible_java_exception(JNIEnv *env, jobject listener)
	{
		if (env->ExceptionCheck()) {
			env->ExceptionDescribe();
			env->ExceptionClear();
			ms_error("Listener %p raised an exception",listener);
		}
	}
};

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_newLinphoneCore(JNIEnv*  env
		,jobject thiz
		,jobject jlistener
		,jstring juserConfig
		,jstring jfactoryConfig
		,jobject juserdata){

	const char* userConfig = GetStringUTFChars(env, juserConfig);
	const char* factoryConfig = GetStringUTFChars(env, jfactoryConfig);

	LinphoneJavaBindings *ljb = new LinphoneJavaBindings(env);

	LinphoneCoreVTable *vTable = linphone_core_v_table_new();
	LinphoneCoreData* ldata = new LinphoneCoreData(env, thiz, vTable, jlistener, ljb);
	linphone_core_v_table_set_user_data(vTable, ldata);


	jobject core = env->NewGlobalRef(thiz);
	ljb->setCore(core);
	LinphoneCore *lc = linphone_core_new(vTable, userConfig, factoryConfig, ljb);

	jlong nativePtr = (jlong)lc;
	ReleaseStringUTFChars(env, juserConfig, userConfig);
	ReleaseStringUTFChars(env, jfactoryConfig, factoryConfig);
	return nativePtr;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_delete(JNIEnv* env, jobject thiz, jlong native_ptr) {
	LinphoneCore *lc=(LinphoneCore*)native_ptr;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jobject multicast_lock = lc->multicast_lock;
	jobject multicast_lock_class = lc->multicast_lock_class;
	jobject wifi_lock = lc->wifi_lock;
	jobject wifi_lock_class = lc->wifi_lock_class;

	linphone_core_destroy(lc);

	if (wifi_lock) env->DeleteGlobalRef(wifi_lock);
	if (wifi_lock_class) env->DeleteGlobalRef(wifi_lock_class);
	if (multicast_lock) env->DeleteGlobalRef(multicast_lock);
	if (multicast_lock_class) env->DeleteGlobalRef(multicast_lock_class);

	if (ljb) {
		jobject core = ljb->getCore();
		if (core) {
			env->DeleteGlobalRef(core);
		}
		delete ljb;
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addListener(JNIEnv* env, jobject thiz, jlong lc, jobject jlistener) {
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *) linphone_core_get_user_data((LinphoneCore *)lc);
	LinphoneCoreVTable *vTable = linphone_core_v_table_new();
	LinphoneCoreData* ldata = new LinphoneCoreData(env, thiz, vTable, jlistener, ljb);
	linphone_core_v_table_set_user_data(vTable, ldata);
	linphone_core_add_listener((LinphoneCore*)lc, vTable);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeListener(JNIEnv* env, jobject thiz, jlong lc, jobject jlistener) {
	bctbx_list_t* iterator;
	LinphoneCore *core = (LinphoneCore*)lc;
	//jobject listener = env->NewGlobalRef(jlistener);
	for (iterator = core->vtable_refs; iterator != NULL; ) {
		VTableReference *ref=(VTableReference*)(iterator->data);
		LinphoneCoreVTable *vTable = ref->valid ? ref->cbs->vtable : NULL;
		iterator = iterator->next; //Because linphone_core_remove_listener may change the list
		if (vTable && !ref->internal) {
			LinphoneCoreData *data = (LinphoneCoreData*) linphone_core_v_table_get_user_data(vTable);
			if (data && env->IsSameObject(data->listener, jlistener)) {
				linphone_core_remove_listener(core, vTable);
				delete data;
				linphone_core_v_table_destroy(vTable);
			}
		}
	}
	//env->DeleteGlobalRef(listener);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_uploadLogCollection(JNIEnv* env, jobject thiz, jlong lc) {
	LinphoneCore *core = (LinphoneCore*)lc;
	linphone_core_upload_log_collection(core);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_resetLogCollection(JNIEnv* env, jobject thiz) {
	linphone_core_reset_log_collection();
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_migrateToMultiTransport(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return (jint) linphone_core_migrate_to_multi_transport((LinphoneCore *)lc);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    createInfoMessage
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_LinphoneCoreImpl_createInfoMessage(JNIEnv *, jobject jobj, jlong lcptr){
	return (jlong) linphone_core_create_info_message((LinphoneCore*)lcptr);
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCallImpl_sendInfoMessage(JNIEnv *env, jobject jobj, jlong callptr, jlong infoptr){
	return linphone_call_send_info_message((LinphoneCall*)callptr,(LinphoneInfoMessage*)infoptr);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_stopRinging(JNIEnv* env, jobject  thiz, jlong lc) {
	linphone_core_stop_ringing((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setChatDatabasePath(JNIEnv* env, jobject thiz, jlong lc, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_chat_database_path((LinphoneCore*)lc, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setCallLogsDatabasePath( JNIEnv* env, jobject thiz, jlong lc, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_call_logs_database_path((LinphoneCore*)lc, path);
	ReleaseStringUTFChars(env, jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setFriendsDatabasePath( JNIEnv* env, jobject thiz, jlong lc, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_friends_database_path((LinphoneCore*)lc, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPrimaryContact2(JNIEnv* env, jobject  thiz, jlong lc, jstring jcontact) {
	const char* contact = GetStringUTFChars(env, jcontact);
	linphone_core_set_primary_contact((LinphoneCore*)lc, contact);
	ReleaseStringUTFChars(env, jcontact, contact);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getPrimaryContact(JNIEnv* env, jobject  thiz, jlong lc) {
	LinphoneAddress* identity = linphone_core_get_primary_contact_parsed((LinphoneCore*)lc);
	return identity ? env->NewStringUTF(linphone_address_as_string(identity)) : NULL;
}


extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPrimaryContact(JNIEnv* env, jobject  thiz, jlong lc, jstring jdisplayname, jstring jusername) {
	const char* displayname = GetStringUTFChars(env, jdisplayname);
	const char* username = GetStringUTFChars(env, jusername);

	LinphoneAddress *parsed = linphone_core_get_primary_contact_parsed((LinphoneCore*)lc);
	if (parsed != NULL) {
		linphone_address_set_display_name(parsed, displayname);
		linphone_address_set_username(parsed, username);
		char *contact = linphone_address_as_string(parsed);
		linphone_core_set_primary_contact((LinphoneCore*)lc, contact);
	}

	ReleaseStringUTFChars(env, jdisplayname, displayname);
	ReleaseStringUTFChars(env, jusername, username);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getPrimaryContactUsername(JNIEnv* env, jobject  thiz, jlong lc) {
	LinphoneAddress* identity = linphone_core_get_primary_contact_parsed((LinphoneCore*)lc);
	const char * username = linphone_address_get_username(identity);
	return username ? env->NewStringUTF(username) : NULL;
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getPrimaryContactDisplayName(JNIEnv* env, jobject  thiz, jlong lc) {
	LinphoneAddress* identity = linphone_core_get_primary_contact_parsed((LinphoneCore*)lc);
	const char * displayname = linphone_address_get_display_name(identity);
	return displayname ? env->NewStringUTF(displayname) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_clearProxyConfigs(JNIEnv* env, jobject thiz,jlong lc) {
	linphone_core_clear_proxy_config((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDefaultProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jlong pc) {
	linphone_core_set_default_proxy((LinphoneCore*)lc,(LinphoneProxyConfig*)pc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getDefaultProxyConfig(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	LinphoneProxyConfig *config = linphone_core_get_default_proxy_config((LinphoneCore*)lc);
	if (config != NULL) {
		jobject jproxy = getProxy(env,config,thiz);
		return jproxy;
	} else {
		return NULL;
	}
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_getProxyConfigList(JNIEnv* env, jobject thiz, jlong lc) {
	const bctbx_list_t* proxies = linphone_core_get_proxy_config_list((LinphoneCore*)lc);
	size_t proxyCount = bctbx_list_size(proxies);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data((LinphoneCore *)lc);
	jobjectArray jProxies = env->NewObjectArray(proxyCount,ljb->proxyClass,NULL);

	for (size_t i = 0; i < proxyCount; i++ ) {
		LinphoneProxyConfig* proxy = (LinphoneProxyConfig*)proxies->data;
		jobject jproxy = getProxy(env,proxy,thiz);
		if(jproxy != NULL){
			env->SetObjectArrayElement(jProxies, i, jproxy);
		}
		proxies = proxies->next;
	}

	return jProxies;
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_addProxyConfig(	JNIEnv*  env
		,jobject  thiz
		,jobject jproxyCfg
		,jlong lc
		,jlong pc) {
	LinphoneProxyConfig* proxy = (LinphoneProxyConfig*)pc;
	return (jint)linphone_core_add_proxy_config((LinphoneCore*)lc,proxy);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeProxyConfig(JNIEnv* env, jobject thiz, jlong lc, jlong proxy) {
	linphone_core_remove_proxy_config((LinphoneCore*)lc, (LinphoneProxyConfig*)proxy);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeAuthInfo(JNIEnv* env, jobject thiz, jlong lc, jlong authInfo) {
	linphone_core_remove_auth_info((LinphoneCore*)lc, (LinphoneAuthInfo*)authInfo);
}

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_getAuthInfosList(JNIEnv* env, jobject thiz,jlong lc) {
	const bctbx_list_t* authInfos = linphone_core_get_auth_info_list((LinphoneCore*)lc);
	size_t listCount = bctbx_list_size(authInfos);
	jlongArray jAuthInfos = env->NewLongArray(listCount);
	jlong *jInternalArray = env->GetLongArrayElements(jAuthInfos, NULL);

	for (size_t i = 0; i < listCount; i++ ) {
		jInternalArray[i] = (unsigned long) (authInfos->data);
		authInfos = authInfos->next;
	}

	env->ReleaseLongArrayElements(jAuthInfos, jInternalArray, 0);

	return jAuthInfos;
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_findAuthInfos(JNIEnv* env, jobject thiz, jlong lc, jstring jusername, jstring jrealm, jstring jdomain) {
	const char* username = GetStringUTFChars(env, jusername);
	const char* realm = GetStringUTFChars(env, jrealm);
	const char* domain = GetStringUTFChars(env, jdomain);
	const LinphoneAuthInfo *authInfo = linphone_core_find_auth_info((LinphoneCore*)lc, realm, username, domain);

	ReleaseStringUTFChars(env, jrealm, realm);
	ReleaseStringUTFChars(env, jdomain, domain);
	ReleaseStringUTFChars(env, jusername, username);

	return (jlong) authInfo;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_clearAuthInfos(JNIEnv* env, jobject thiz,jlong lc) {
	linphone_core_clear_all_auth_info((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_refreshRegisters(JNIEnv* env, jobject thiz,jlong lc) {
	linphone_core_refresh_registers((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addAuthInfo(JNIEnv* env
		,jobject  thiz
		,jlong lc
		,jlong pc) {
	linphone_core_add_auth_info((LinphoneCore*)lc,(LinphoneAuthInfo*)pc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_iterate(JNIEnv* env
		,jobject  thiz
		,jlong lc) {
	linphone_core_iterate((LinphoneCore*)lc);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_invite(JNIEnv* env
		,jobject  thiz
		,jlong lc
		,jstring juri) {
	const char* uri = GetStringUTFChars(env, juri);
	LinphoneCall* lCall = linphone_core_invite((LinphoneCore*)lc,uri);
	ReleaseStringUTFChars(env, juri, uri);
	return getCall(env,lCall);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_inviteAddress(JNIEnv* env
		,jobject  thiz
		,jlong lc
		,jlong to) {
	return getCall(env, linphone_core_invite_address((LinphoneCore*)lc,(LinphoneAddress*)to));
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateCall(JNIEnv* env
		,jobject  thiz
		,jlong lc
		,jlong call) {
	linphone_core_terminate_call((LinphoneCore*)lc,(LinphoneCall*)call);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_declineCall(JNIEnv* env
		,jobject  thiz
		,jlong lc
		,jlong call, jint reason) {
	linphone_core_decline_call((LinphoneCore*)lc,(LinphoneCall*)call,(LinphoneReason)reason);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getRemoteAddress(JNIEnv* env
		,jobject  thiz
		,jlong lc) {
	return (jlong)linphone_core_get_current_call_remote_address((LinphoneCore*)lc);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInCall(JNIEnv* env
		,jobject  thiz
		,jlong lc) {

	return (jboolean)linphone_core_in_call((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneFriendImpl_isPresenceReceived(JNIEnv* env
		,jobject  thiz
		,jlong lf) {

	return (jboolean)linphone_friend_is_presence_received((LinphoneFriend*)lf);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInComingInvitePending(JNIEnv* env
		,jobject  thiz
		,jlong lc) {

	return (jboolean)linphone_core_inc_invite_pending((LinphoneCore*)lc);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_acceptCall(JNIEnv* env
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

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_acceptEarlyMedia(JNIEnv *env, jobject thiz, jlong lc, jlong c) {
	LinphoneCore *core = (LinphoneCore *)lc;
	LinphoneCall *call = (LinphoneCall *)c;
	int ret = linphone_core_accept_early_media(core, call);
	return (jboolean) ret == 0;
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_acceptEarlyMediaWithParams(JNIEnv *env, jobject thiz, jlong lc, jlong c, jlong params) {
	LinphoneCore *core = (LinphoneCore *)lc;
	LinphoneCall *call = (LinphoneCall *)c;
	const LinphoneCallParams *call_params = (LinphoneCallParams *) params;
	int ret = linphone_core_accept_early_media_with_params(core, call, call_params);
	return (jboolean) ret == 0;
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getCallLog(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jint position) {
		return (jlong)bctbx_list_nth_data(linphone_core_get_call_logs((LinphoneCore*)lc),position);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getNumberOfCallLogs(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return (jint)bctbx_list_size(linphone_core_get_call_logs((LinphoneCore*)lc));
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getLastOutgoingCallLog(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return (jlong)linphone_core_get_last_outgoing_call_log((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_migrateCallLogs(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		linphone_core_migrate_logs_from_rc_to_db((LinphoneCore *)lc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getMSFactory(JNIEnv*  env
		,jobject  thiz
		,jlong lc){
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data((LinphoneCore *)lc);
	MSFactory *factory = linphone_core_get_ms_factory((LinphoneCore*)lc);
	return env->NewObject(ljb->msFactoryClass, ljb->msFactoryCtrId, (jlong)factory);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMtu(JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jint mtu) {
		linphone_core_set_mtu((LinphoneCore*)lc,mtu);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getMtu(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return linphone_core_get_mtu((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setNetworkStateReachable(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jboolean isReachable) {
		linphone_core_set_network_reachable((LinphoneCore*)lc,isReachable);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isNetworkStateReachable(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return (jboolean)linphone_core_is_network_reachable((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMicrophoneGain(JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jfloat gain) {
		linphone_core_set_mic_gain_db((LinphoneCore*)lc,gain);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPlaybackGain(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jfloat gain) {
		linphone_core_set_playback_gain_db((LinphoneCore*)lc,gain);
}

extern "C" jfloat Java_org_linphone_core_LinphoneCoreImpl_getPlaybackGain(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
		return (jfloat)linphone_core_get_playback_gain_db((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_muteMic(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jboolean isMuted) {
	linphone_core_enable_mic((LinphoneCore*)lc, !isMuted);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_interpretUrl(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jstring jurl) {
	const char* url = GetStringUTFChars(env, jurl);
	jlong result = (jlong)linphone_core_interpret_url((LinphoneCore*)lc,url);
	ReleaseStringUTFChars(env, jurl, url);
	return result;
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_sendDtmfs(	JNIEnv*  env
		,jobject  thiz
		,jlong lc
		,jstring dtmf) {
	int result;
	const char* dtmfs = GetStringUTFChars(env, dtmf);
	result = linphone_call_send_dtmfs(linphone_core_get_current_call((LinphoneCore*)lc),dtmfs);
	ReleaseStringUTFChars(env, dtmf, dtmfs);
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

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getMissedCallsCount(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc) {
	return (jint)linphone_core_get_missed_calls_count((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_resetMissedCallsCount(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc) {
	linphone_core_reset_missed_calls_count((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeCallLog(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc, jlong log) {
	linphone_core_remove_call_log((LinphoneCore*)lc, (LinphoneCallLog*) log);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_clearCallLogs(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc) {
	linphone_core_clear_call_logs((LinphoneCore*)lc);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isMicMuted(	JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	return (jboolean)linphone_core_is_mic_muted((LinphoneCore*)lc);
}
extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_findPayloadType(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jmime
																			,jint rate
																			,jint channels) {
	const char* mime = GetStringUTFChars(env, jmime);
	jlong result = (jlong)linphone_core_find_payload_type((LinphoneCore*)lc,mime,rate,channels);
	ReleaseStringUTFChars(env, jmime, mime);
	return result;
}

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_listVideoPayloadTypes(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const bctbx_list_t* codecs = linphone_core_get_video_codecs((LinphoneCore*)lc);
	size_t codecsCount = bctbx_list_size(codecs);
	jlongArray jCodecs = env->NewLongArray(codecsCount);
	jlong *jInternalArray = env->GetLongArrayElements(jCodecs, NULL);

	for (size_t i = 0; i < codecsCount; i++ ) {
		jInternalArray[i] = (unsigned long) (codecs->data);
		codecs = codecs->next;
	}

	env->ReleaseLongArrayElements(jCodecs, jInternalArray, 0);

	return jCodecs;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setVideoCodecs(JNIEnv *env, jobject thiz, jlong lc, jlongArray jCodecs) {
	bctbx_list_t *pts = NULL;
	int codecsCount = env->GetArrayLength(jCodecs);
	jlong *codecs = env->GetLongArrayElements(jCodecs, NULL);
	for (int i = 0; i < codecsCount; i++) {
		PayloadType *pt = (PayloadType *)codecs[i];
		bctbx_list_append(pts, pt);
	}
	linphone_core_set_video_codecs((LinphoneCore *)lc, pts);
	env->ReleaseLongArrayElements(jCodecs, codecs, 0);
}

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_listAudioPayloadTypes(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const bctbx_list_t* codecs = linphone_core_get_audio_codecs((LinphoneCore*)lc);
	size_t codecsCount = bctbx_list_size(codecs);
	jlongArray jCodecs = env->NewLongArray(codecsCount);
	jlong *jInternalArray = env->GetLongArrayElements(jCodecs, NULL);

	for (size_t i = 0; i < codecsCount; i++ ) {
		jInternalArray[i] = (unsigned long) (codecs->data);
		codecs = codecs->next;
	}

	env->ReleaseLongArrayElements(jCodecs, jInternalArray, 0);

	return jCodecs;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setAudioCodecs(JNIEnv *env, jobject thiz, jlong lc, jlongArray jCodecs) {
	bctbx_list_t *pts = NULL;
	int codecsCount = env->GetArrayLength(jCodecs);
	jlong *codecs = env->GetLongArrayElements(jCodecs, NULL);
	for (int i = 0; i < codecsCount; i++) {
		PayloadType *pt = (PayloadType *)codecs[i];
		pts = bctbx_list_append(pts, pt);
	}
	linphone_core_set_audio_codecs((LinphoneCore *)lc, pts);
	env->ReleaseLongArrayElements(jCodecs, codecs, 0);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_enablePayloadType(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt
																			,jboolean enable) {
	return (jint)linphone_core_enable_payload_type((LinphoneCore*)lc,(PayloadType*)pt,enable);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isPayloadTypeEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt) {
	return (jboolean) linphone_core_payload_type_enabled((LinphoneCore*)lc, (PayloadType*)pt);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_payloadTypeIsVbr(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt) {
	return (jboolean) linphone_core_payload_type_is_vbr((LinphoneCore*)lc, (PayloadType*)pt);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPayloadTypeBitrate(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt
																			,jint bitrate) {
	linphone_core_set_payload_type_bitrate((LinphoneCore*)lc,(PayloadType*)pt,bitrate);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getPayloadTypeBitrate(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong pt) {
	return (jint)linphone_core_get_payload_type_bitrate((LinphoneCore*)lc,(PayloadType*)pt);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPayloadTypeNumber(JNIEnv*  env
                                                                            ,jobject  thiz
                                                                            ,jlong lc
                                                                            ,jlong pt
                                                                            ,jint number) {
    linphone_core_set_payload_type_number((LinphoneCore*)lc,(PayloadType*)pt,number);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getPayloadTypeNumber(JNIEnv*  env
                                                                            ,jobject  thiz
                                                                            ,jlong lc
                                                                            ,jlong pt) {
    return (jint)linphone_core_get_payload_type_number((LinphoneCore*)lc,(PayloadType*)pt);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_enableAdaptiveRateControl(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jboolean enable) {
	linphone_core_enable_adaptive_rate_control((LinphoneCore*)lc, enable);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isAdaptiveRateControlEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jboolean)linphone_core_adaptive_rate_control_enabled((LinphoneCore*)lc);
}
extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getAdaptiveRateAlgorithm(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	const char* alg = linphone_core_get_adaptive_rate_algorithm((LinphoneCore*)lc);
	if (alg) {
		return env->NewStringUTF(alg);
	} else {
		return NULL;
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAdaptiveRateAlgorithm(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jalg) {
	const char* alg = GetStringUTFChars(env, jalg);
	linphone_core_set_adaptive_rate_algorithm((LinphoneCore*)lc,alg);
	ReleaseStringUTFChars(env, jalg, alg);

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
	return (jboolean)linphone_core_echo_cancellation_enabled((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isEchoLimiterEnabled(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jboolean)linphone_core_echo_limiter_enabled((LinphoneCore*)lc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getCurrentCall(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return getCall(env,linphone_core_get_current_call((LinphoneCore*)lc));
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addFriend(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong aFriend
																			) {
	linphone_core_add_friend((LinphoneCore*)lc,(LinphoneFriend*)aFriend);
}

extern "C" jint Java_org_linphone_core_LinphoneFriendListImpl_importFriendsFromVCardFile(JNIEnv* env, jobject thiz, jlong list, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	int count = linphone_friend_list_import_friends_from_vcard4_file((LinphoneFriendList*)list, path);
	ReleaseStringUTFChars(env, jpath, path);
	return count;
}

extern "C" jint Java_org_linphone_core_LinphoneFriendListImpl_importFriendsFromVCardBuffer(JNIEnv* env, jobject thiz, jlong list, jstring jbuffer) {
	const char* buffer = GetStringUTFChars(env, jbuffer);
	int count = linphone_friend_list_import_friends_from_vcard4_buffer((LinphoneFriendList*)list, buffer);
	ReleaseStringUTFChars(env, jbuffer, buffer);
	return count;
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_exportFriendsToVCardFile(JNIEnv* env, jobject thiz, jlong list, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_friend_list_export_friends_as_vcard4_file((LinphoneFriendList*)list, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_enableSubscriptions(JNIEnv* env, jobject thiz, jlong list, jboolean enable) {
	linphone_friend_list_enable_subscriptions((LinphoneFriendList*)list, enable);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_addFriendList(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong friendList
																			) {
	linphone_core_add_friend_list((LinphoneCore*)lc,(LinphoneFriendList*)friendList);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeFriendList(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jlong friendList
																			) {
	LinphoneFriendList *list = (LinphoneFriendList *)friendList;
	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(list);
	if (cbs != NULL) {
		jobject listener = (jobject) linphone_friend_list_cbs_get_user_data(cbs);
		if (listener != NULL) {
			env->DeleteGlobalRef(listener);
		}
	}
	linphone_core_remove_friend_list((LinphoneCore*)lc, list);
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_getFriendList(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const bctbx_list_t* friends = linphone_core_get_friend_list((LinphoneCore*)lc);
	size_t friendsSize = bctbx_list_size(friends);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data((LinphoneCore *)lc);
	jobjectArray jFriends = env->NewObjectArray(friendsSize,ljb->friendClass,NULL);

	for (size_t i = 0; i < friendsSize; i++) {
		LinphoneFriend* lfriend = (LinphoneFriend*)friends->data;
		jobject jfriend = getFriend(env,lfriend);
		if(jfriend != NULL){
			env->SetObjectArrayElement(jFriends, i, jfriend);
			env->DeleteLocalRef(jfriend);
		}
		friends = friends->next;
	}

	return jFriends;
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_getFriendLists(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const bctbx_list_t* friends = linphone_core_get_friends_lists((LinphoneCore*)lc);
	size_t friendsSize = bctbx_list_size(friends);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data((LinphoneCore *)lc);
	jobjectArray jFriends = env->NewObjectArray(friendsSize,ljb->friendListClass,NULL);

	for (size_t i = 0; i < friendsSize; i++) {
		LinphoneFriendList* lfriend = (LinphoneFriendList*)friends->data;
		jobject jfriend =  getFriendList(env,lfriend);
		if(jfriend != NULL){
			env->SetObjectArrayElement(jFriends, i, jfriend);
		}
		friends = friends->next;
	}

	return jFriends;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPresenceInfo(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint minutes_away
																			,jstring jalternative_contact
																			,jint status) {
	const char* alternative_contact = GetStringUTFChars(env, jalternative_contact);
	linphone_core_set_presence_info((LinphoneCore*)lc,minutes_away,alternative_contact,(LinphoneOnlineStatus)status);
	ReleaseStringUTFChars(env, jalternative_contact, alternative_contact);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getPresenceInfo(JNIEnv *env, jobject thiz, jlong lc) {
	return (jint)linphone_core_get_presence_info((LinphoneCore *)lc);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setPresenceModel
 * Signature: (JILjava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setPresenceModel(JNIEnv *env, jobject jobj, jlong ptr, jlong modelPtr) {
	LinphoneCore *lc = (LinphoneCore *)ptr;
	LinphonePresenceModel *model = (LinphonePresenceModel *)modelPtr;
	linphone_core_set_presence_model(lc, model);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getPresenceModel
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_getPresenceModel(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphoneCore *lc = (LinphoneCore *)ptr;
	LinphonePresenceModel *model = linphone_core_get_presence_model(lc);
	if (model == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceModelImpl", linphone_presence_model, model)
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getOrCreateChatRoom(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jto) {

	const char* to = GetStringUTFChars(env, jto);
	LinphoneChatRoom* lResult = linphone_core_get_chat_room_from_uri((LinphoneCore*)lc,to);
	ReleaseStringUTFChars(env, jto, to);
	return getChatRoom(env, lResult);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getChatRoom(JNIEnv*  env
																		,jobject  thiz
																		,jlong lc
																		,jlong to) {
	LinphoneChatRoom* lResult = linphone_core_get_chat_room((LinphoneCore*)lc,(LinphoneAddress *)to);
	return getChatRoom(env, lResult);
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
	return (jboolean)linphone_core_video_enabled((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isVideoSupported(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	return (jboolean)linphone_core_video_supported((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isVCardSupported(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	return (jboolean)linphone_core_vcard_supported();
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPlayFile(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_play_file((LinphoneCore*)lc,path);
	ReleaseStringUTFChars(env, jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRing(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_ring((LinphoneCore*)lc,path);
	ReleaseStringUTFChars(env, jpath, path);
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
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setTone(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint toneid
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_tone((LinphoneCore *)lc, (LinphoneToneID)toneid, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setCallErrorTone(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint reason
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_call_error_tone((LinphoneCore *)lc, (LinphoneReason)reason, path);
	ReleaseStringUTFChars(env, jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRootCA(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_root_ca((LinphoneCore*)lc,path);
	ReleaseStringUTFChars(env, jpath, path);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRootCAData(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jdata) {
	const char* data = GetStringUTFChars(env, jdata);
	linphone_core_set_root_ca_data((LinphoneCore*)lc, data);
	ReleaseStringUTFChars(env, jdata, data);
}
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRingback(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_ringback((LinphoneCore*)lc,path);
	ReleaseStringUTFChars(env, jpath, path);

}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setProvisioningUri(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_provisioning_uri((LinphoneCore*)lc,path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getProvisioningUri(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc) {
	const char* path = linphone_core_get_provisioning_uri((LinphoneCore*)lc);
	return path ? env->NewStringUTF(path) : NULL;
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
	return (jboolean)linphone_core_keep_alive_enabled((LinphoneCore*)lc);

}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDefaultSoundDevices(JNIEnv*  env
                                                                               ,jobject  thiz
                                                                               ,jlong lc) {
    linphone_core_set_default_sound_devices((LinphoneCore*)lc);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_startEchoCalibration(JNIEnv*  env
																				,jobject  thiz
																				,jlong lc
																				,jobject data) {
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *) linphone_core_get_user_data((LinphoneCore *)lc);
	LinphoneCoreVTable *vTable = linphone_core_v_table_new();
	LinphoneCoreData* ldata = new LinphoneCoreData(env, thiz, vTable, data, ljb);
	linphone_core_v_table_set_user_data(vTable, ldata);

	return (jint)linphone_core_start_echo_calibration((LinphoneCore*)lc, ldata->ecCalibrationStatus, NULL, NULL, vTable);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_needsEchoCalibration(JNIEnv *env, jobject thiz, jlong lcptr) {
	LinphoneCore *lc = (LinphoneCore*) lcptr;
	MSFactory * factory = linphone_core_get_ms_factory(lc);
	MSDevicesInfo *devices = ms_factory_get_devices_info(factory);
	SoundDeviceDescription *sound_description = ms_devices_info_get_sound_device_description(devices);
	if (sound_description == NULL) return TRUE;
	if (sound_description->flags & DEVICE_HAS_BUILTIN_AEC) return FALSE;
	if (sound_description->delay != 0) return FALSE;
	return TRUE;
}

extern "C" int Java_org_linphone_core_LinphoneCoreImpl_startEchoTester(JNIEnv*  env
																				,jobject  thiz
																				,jlong lc
																				,jint rate) {
	return linphone_core_start_echo_tester((LinphoneCore*)lc, rate);
}

extern "C" int Java_org_linphone_core_LinphoneCoreImpl_stopEchoTester(JNIEnv*  env
																				,jobject  thiz
																				,jlong lc) {
	return linphone_core_stop_echo_tester((LinphoneCore*)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_hasCrappyOpenGL(JNIEnv *env, jobject thiz, jlong lcptr) {
	LinphoneCore *lc = (LinphoneCore*) lcptr;
	MSFactory * factory = linphone_core_get_ms_factory(lc);
	MSDevicesInfo *devices = ms_factory_get_devices_info(factory);
	SoundDeviceDescription *sound_description = ms_devices_info_get_sound_device_description(devices);
	if (sound_description == NULL) return FALSE;
	if (sound_description->flags & DEVICE_HAS_CRAPPY_OPENGL) return TRUE;
	return FALSE;
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_hasBuiltInEchoCanceler(JNIEnv *env, jobject thiz, jlong lcptr) {
	LinphoneCore *lc = (LinphoneCore*) lcptr;
	MSFactory * factory = linphone_core_get_ms_factory(lc);
	MSDevicesInfo *devices = ms_factory_get_devices_info(factory);
	SoundDeviceDescription *sound_description = ms_devices_info_get_sound_device_description(devices);
	if (sound_description == NULL) return FALSE;
	if (sound_description->flags & DEVICE_HAS_BUILTIN_AEC) return TRUE;
	return FALSE;
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jint)linphone_core_get_media_encryption((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint menc) {
	linphone_core_set_media_encryption((LinphoneCore*)lc,(LinphoneMediaEncryption)menc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_mediaEncryptionSupported(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc, jint menc
																			) {
	return (jboolean)linphone_core_media_encryption_supported((LinphoneCore*)lc,(LinphoneMediaEncryption)menc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isMediaEncryptionMandatory(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jboolean)linphone_core_is_media_encryption_mandatory((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setMediaEncryptionMandatory(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			, jboolean yesno
																			) {
	linphone_core_set_media_encryption_mandatory((LinphoneCore*)lc, yesno);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isLimeEncryptionAvailable (JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jboolean) linphone_core_lime_available((LinphoneCore*)lc);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getLimeEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			) {
	return (jint)linphone_core_lime_enabled((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setLimeEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jint menc) {
	linphone_core_enable_lime((LinphoneCore*)lc,(LinphoneLimeState)menc);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    disableChat
 * Signature: (JI)V
 */
extern "C" JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_disableChat(JNIEnv *env, jobject jobj, jlong ptr, jint reason){
	linphone_core_disable_chat((LinphoneCore*)ptr,(LinphoneReason)reason);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    enableChat
 * Signature: (J)V
 */
extern "C" JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_enableChat(JNIEnv *env, jobject jobj, jlong ptr){
	linphone_core_enable_chat((LinphoneCore*)ptr);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    chatEnabled
 * Signature: (J)Z
 */
extern "C" JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneCoreImpl_chatEnabled(JNIEnv *env, jobject jobj, jlong ptr){
	return (jboolean) linphone_core_chat_enabled((LinphoneCore*)ptr);
}

//ProxyConfig

extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_createProxyConfig(JNIEnv* env, jobject thiz, jlong lc) {
	LinphoneProxyConfig* proxy = linphone_core_create_proxy_config((LinphoneCore *)lc);
	linphone_proxy_config_set_user_data(proxy,env->NewWeakGlobalRef(thiz));
	return (jlong) proxy;
}

extern "C" void  Java_org_linphone_core_LinphoneProxyConfigImpl_finalize(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneProxyConfig *proxy=(LinphoneProxyConfig*)ptr;
	linphone_proxy_config_set_user_data(proxy,NULL);
	linphone_proxy_config_unref(proxy);
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setIdentity(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jidentity) {
	const char* identity = GetStringUTFChars(env, jidentity);
	linphone_proxy_config_set_identity((LinphoneProxyConfig*)proxyCfg,identity);
	ReleaseStringUTFChars(env, jidentity, identity);
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getIdentity(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* identity = linphone_proxy_config_get_identity((LinphoneProxyConfig*)proxyCfg);
	if (identity) {
		return env->NewStringUTF(identity);
	} else {
		return NULL;
	}
}
extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_getAddress(JNIEnv* env, jobject thiz, jlong proxyCfg) {
	return (jlong) linphone_proxy_config_get_identity_address((LinphoneProxyConfig*)proxyCfg);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setAddress(JNIEnv* env,jobject thiz,jlong proxyCfg,jlong jidentity) {
	linphone_proxy_config_set_identity_address((LinphoneProxyConfig*)proxyCfg, (LinphoneAddress*) jidentity);
}
extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_setProxy(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jproxy) {
	const char* proxy = GetStringUTFChars(env, jproxy);
	jint err=linphone_proxy_config_set_server_addr((LinphoneProxyConfig*)proxyCfg,proxy);
	ReleaseStringUTFChars(env, jproxy, proxy);
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
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setContactParameters(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jparams) {
	const char* params = GetStringUTFChars(env, jparams);
	linphone_proxy_config_set_contact_parameters((LinphoneProxyConfig*)proxyCfg, params);
	ReleaseStringUTFChars(env, jparams, params);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setContactUriParameters(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jparams) {
	const char* params = GetStringUTFChars(env, jparams);
	linphone_proxy_config_set_contact_uri_parameters((LinphoneProxyConfig*)proxyCfg, params);
	ReleaseStringUTFChars(env, jparams, params);
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getContactParameters(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* params = linphone_proxy_config_get_contact_parameters((LinphoneProxyConfig*)proxyCfg);
	return params ? env->NewStringUTF(params) : NULL;
}
extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getContactUriParameters(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char* params = linphone_proxy_config_get_contact_uri_parameters((LinphoneProxyConfig*)proxyCfg);
	return params ? env->NewStringUTF(params) : NULL;
}


extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_setRoute(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jroute) {
	if (jroute != NULL) {
		const char* route = GetStringUTFChars(env, jroute);
		jint err=linphone_proxy_config_set_route((LinphoneProxyConfig*)proxyCfg,route);
		ReleaseStringUTFChars(env, jroute, route);
		return err;
	} else {
		return (jint)linphone_proxy_config_set_route((LinphoneProxyConfig*)proxyCfg,NULL);
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
	return (jboolean)linphone_proxy_config_is_registered((LinphoneProxyConfig*)proxyCfg);
}
extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_isRegisterEnabled(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return (jboolean)linphone_proxy_config_register_enabled((LinphoneProxyConfig*)proxyCfg);
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
	char * normalized_phone;
	const char* number = GetStringUTFChars(env, jnumber);
	int len = env->GetStringLength(jnumber);
	if (len == 0) {
		ms_warning("cannot normalize empty number");
		return jnumber;
	}
	normalized_phone = linphone_proxy_config_normalize_phone_number((LinphoneProxyConfig*)proxyCfg,number);
	jstring normalizedNumber = env->NewStringUTF(normalized_phone ? normalized_phone : number);
	ReleaseStringUTFChars(env, jnumber, number);
	ms_free(normalized_phone);
	return normalizedNumber;
}
extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_normalizeSipUri(JNIEnv* env,jobject thiz,jlong proxyCfg,jstring jusername) {
	const char* username = GetStringUTFChars(env, jusername);
	LinphoneAddress *addr = linphone_proxy_config_normalize_sip_uri((LinphoneProxyConfig*)proxyCfg, username);
	ReleaseStringUTFChars(env, jusername, username);
	return (jlong) addr;
}
extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_lookupCCCFromIso(JNIEnv* env, jobject thiz, jlong proxyCfg, jstring jiso) {
	const char* iso = GetStringUTFChars(env, jiso);
	int prefix = linphone_dial_plan_lookup_ccc_from_iso(iso);
	ReleaseStringUTFChars(env, jiso, iso);
	return (jint) prefix;
}
extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_lookupCCCFromE164(JNIEnv* env, jobject thiz, jlong proxyCfg, jstring je164) {
	const char* e164 = GetStringUTFChars(env, je164);
	int prefix = linphone_dial_plan_lookup_ccc_from_e164(e164);
	ReleaseStringUTFChars(env, je164, e164);
	return (jint) prefix;
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

extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_getDialEscapePlus(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return (jboolean) linphone_proxy_config_get_dial_escape_plus((LinphoneProxyConfig*)proxyCfg);
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setDialPrefix(JNIEnv* env
																	,jobject thiz
																	,jlong proxyCfg
																	,jstring jprefix) {
	const char* prefix = GetStringUTFChars(env, jprefix);
	linphone_proxy_config_set_dial_prefix((LinphoneProxyConfig*)proxyCfg,prefix);
	ReleaseStringUTFChars(env, jprefix, prefix);
}

extern "C" jstring Java_org_linphone_core_LinphoneProxyConfigImpl_getDialPrefix(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	const char * prefix = linphone_proxy_config_get_dial_prefix((LinphoneProxyConfig*)proxyCfg);
	return prefix ? env->NewStringUTF(prefix) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_enablePublish(JNIEnv* env
																				,jobject thiz
																				,jlong proxyCfg
																				,jboolean val) {
	linphone_proxy_config_enable_publish((LinphoneProxyConfig*)proxyCfg,val);
}
extern "C" jboolean Java_org_linphone_core_LinphoneProxyConfigImpl_publishEnabled(JNIEnv* env,jobject thiz,jlong proxyCfg) {
	return (jboolean)linphone_proxy_config_publish_enabled((LinphoneProxyConfig*)proxyCfg);
}

extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_getError(JNIEnv*  env,jobject thiz,jlong ptr) {
	return linphone_proxy_config_get_error((LinphoneProxyConfig *) ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneProxyConfigImpl_getErrorInfo(JNIEnv*  env,jobject thiz,jlong ptr) {
	return (jlong)linphone_proxy_config_get_error_info((LinphoneProxyConfig *) ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_getPublishExpires(JNIEnv*  env,jobject thiz,jlong ptr) {
	return (jint)linphone_proxy_config_get_publish_expires((LinphoneProxyConfig *) ptr);
}
extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setPublishExpires(JNIEnv*  env
																					,jobject thiz
																					,jlong ptr
																					,jint jval) {
	linphone_proxy_config_set_publish_expires((LinphoneProxyConfig *) ptr, jval);
}
//Auth Info

extern "C" jlong Java_org_linphone_core_LinphoneAuthInfoImpl_newLinphoneAuthInfo(JNIEnv* env
		, jobject thiz ) {
	return (jlong)linphone_auth_info_new(NULL,NULL,NULL,NULL,NULL,NULL);
}
extern "C" void Java_org_linphone_core_LinphoneAuthInfoImpl_delete(JNIEnv* env
		, jobject thiz
		, jlong ptr) {
	linphone_auth_info_destroy((LinphoneAuthInfo*)ptr);
}
/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getPassword
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getPassword
(JNIEnv *env , jobject, jlong auth_info) {
	const char* passwd = linphone_auth_info_get_passwd((LinphoneAuthInfo*)auth_info);
	if (passwd) {
		return env->NewStringUTF(passwd);
	} else {
		return NULL;
	}
}
/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getRealm
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getRealm
(JNIEnv *env , jobject, jlong auth_info) {
	const char* realm = linphone_auth_info_get_realm((LinphoneAuthInfo*)auth_info);
	if (realm) {
		return env->NewStringUTF(realm);
	} else {
		return NULL;
	}
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getDomain
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getDomain
(JNIEnv *env , jobject, jlong auth_info) {
	const char* domain = linphone_auth_info_get_domain((LinphoneAuthInfo*)auth_info);
	if (domain) {
		return env->NewStringUTF(domain);
	} else {
		return NULL;
	}
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getUsername
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getUsername
(JNIEnv *env , jobject, jlong auth_info) {
	const char* username = linphone_auth_info_get_username((LinphoneAuthInfo*)auth_info);
	if (username) {
		return env->NewStringUTF(username);
	} else {
		return NULL;
	}
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setPassword
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setPassword
(JNIEnv *env, jobject, jlong auth_info, jstring jpassword) {
	const char* password = GetStringUTFChars(env, jpassword);
	linphone_auth_info_set_passwd((LinphoneAuthInfo*)auth_info,password);
	ReleaseStringUTFChars(env, jpassword, password);
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setRealm
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setRealm
(JNIEnv *env, jobject, jlong auth_info, jstring jrealm) {
	const char* realm = GetStringUTFChars(env, jrealm);
	linphone_auth_info_set_realm((LinphoneAuthInfo*)auth_info,realm);
	ReleaseStringUTFChars(env, jrealm, realm);
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setDomain
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setDomain
(JNIEnv *env, jobject, jlong auth_info, jstring jdomain) {
	const char* domain = GetStringUTFChars(env, jdomain);
	linphone_auth_info_set_domain((LinphoneAuthInfo*)auth_info, domain);
	ReleaseStringUTFChars(env, jdomain, domain);
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setUsername
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setUsername
(JNIEnv *env, jobject, jlong auth_info, jstring jusername) {
	const char* username = GetStringUTFChars(env, jusername);
	linphone_auth_info_set_username((LinphoneAuthInfo*)auth_info,username);
	ReleaseStringUTFChars(env, jusername, username);
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setAuthUserId
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setUserId
(JNIEnv *env, jobject, jlong auth_info, jstring juserid) {
	const char* userid = GetStringUTFChars(env, juserid);
	linphone_auth_info_set_userid((LinphoneAuthInfo*)auth_info,userid);
	ReleaseStringUTFChars(env, juserid, userid);
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getAuthUserId
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getUserId
(JNIEnv *env , jobject, jlong auth_info) {
	const char* userid = linphone_auth_info_get_userid((LinphoneAuthInfo*)auth_info);
	if (userid) {
		return env->NewStringUTF(userid);
	} else {
		return NULL;
	}
}

/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    setHa1
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setHa1
(JNIEnv *env, jobject, jlong auth_info, jstring jha1) {
	const char* ha1 = GetStringUTFChars(env, jha1);
	linphone_auth_info_set_ha1((LinphoneAuthInfo*)auth_info,ha1);
	ReleaseStringUTFChars(env, jha1, ha1);
}


/*
 * Class:     org_linphone_core_LinphoneAuthInfoImpl
 * Method:    getHa1
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getHa1
(JNIEnv *env , jobject, jlong auth_info) {
	const char* ha1 = linphone_auth_info_get_ha1((LinphoneAuthInfo*)auth_info);
	if (ha1) {
		return env->NewStringUTF(ha1);
	} else {
		return NULL;
	}
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setTlsCertificate
(JNIEnv *env, jobject, jlong auth_info, jstring jcert) {
	const char* cert = GetStringUTFChars(env, jcert);
	linphone_auth_info_set_tls_cert((LinphoneAuthInfo*)auth_info,cert);
	ReleaseStringUTFChars(env, jcert, cert);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setTlsKey
(JNIEnv *env, jobject, jlong auth_info, jstring jkey) {
	const char* key = GetStringUTFChars(env, jkey);
	linphone_auth_info_set_tls_key((LinphoneAuthInfo*)auth_info,key);
	ReleaseStringUTFChars(env, jkey, key);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setTlsCertificatePath
(JNIEnv *env, jobject, jlong auth_info, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_auth_info_set_tls_cert_path((LinphoneAuthInfo*)auth_info,path);
	ReleaseStringUTFChars(env, jpath, path);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_setTlsKeyPath
(JNIEnv *env, jobject, jlong auth_info, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_auth_info_set_tls_key_path((LinphoneAuthInfo*)auth_info,path);
	ReleaseStringUTFChars(env, jpath, path);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getTlsCertificate
(JNIEnv *env , jobject, jlong auth_info) {
	const char* cert = linphone_auth_info_get_tls_cert((LinphoneAuthInfo*)auth_info);
	if (cert) {
		return env->NewStringUTF(cert);
	} else {
		return NULL;
	}
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getTlsKey
(JNIEnv *env , jobject, jlong auth_info) {
	const char* key = linphone_auth_info_get_tls_key((LinphoneAuthInfo*)auth_info);
	if (key) {
		return env->NewStringUTF(key);
	} else {
		return NULL;
	}
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getTlsCertificatePath
(JNIEnv *env , jobject, jlong auth_info) {
	const char* path = linphone_auth_info_get_tls_cert_path((LinphoneAuthInfo*)auth_info);
	if (path) {
		return env->NewStringUTF(path);
	} else {
		return NULL;
	}
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneAuthInfoImpl_getTlsKeyPath
(JNIEnv *env , jobject, jlong auth_info) {
	const char* path = linphone_auth_info_get_tls_key_path((LinphoneAuthInfo*)auth_info);
	if (path) {
		return env->NewStringUTF(path);
	} else {
		return NULL;
	}
}


//LinphoneAddress

extern "C" jlong Java_org_linphone_core_LinphoneAddressImpl_newLinphoneAddressImpl(JNIEnv*  env
																					,jobject  thiz
																					,jstring juri
																					,jstring jdisplayName) {
	const char* uri = GetStringUTFChars(env, juri);
	LinphoneAddress* address = linphone_address_new(uri);
	if (jdisplayName && address) {
		const char* displayName = GetStringUTFChars(env, jdisplayName);
		linphone_address_set_display_name(address,displayName);
		ReleaseStringUTFChars(env, jdisplayName, displayName);
	}
	ReleaseStringUTFChars(env, juri, uri);

	return  (jlong) address;
}

extern "C" jlong Java_org_linphone_core_LinphoneAddressImpl_ref(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_address_ref((LinphoneAddress*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneAddressImpl_clone(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) (ptr ? linphone_address_clone((const LinphoneAddress*)ptr) : NULL);
}

extern "C" void Java_org_linphone_core_LinphoneAddressImpl_unref(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	linphone_address_unref((LinphoneAddress*)ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getDisplayName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* displayName = linphone_address_get_display_name((LinphoneAddress*)ptr);
	if (displayName) {
		return env->NewStringUTF(displayName);
	} else {
		return NULL;
	}
}
extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getUserName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* userName = linphone_address_get_username((LinphoneAddress*)ptr);
	if (userName) {
		return env->NewStringUTF(userName);
	} else {
		return NULL;
	}
}
extern "C" jstring Java_org_linphone_core_LinphoneAddressImpl_getDomain(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char* domain = linphone_address_get_domain((LinphoneAddress*)ptr);
	if (domain) {
		return env->NewStringUTF(domain);
	} else {
		return NULL;
	}
}
extern "C" jint Java_org_linphone_core_LinphoneAddressImpl_getTransport(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneTransportType transporttype = linphone_address_get_transport((LinphoneAddress*)ptr);
	return (jint)transporttype;
}
extern "C" jint Java_org_linphone_core_LinphoneAddressImpl_getPort(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	int port = linphone_address_get_port((LinphoneAddress*)ptr);
	return (jint)port;
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
	const char* displayName = GetStringUTFChars(env, jdisplayName);
	linphone_address_set_display_name((LinphoneAddress*)address,displayName);
	ReleaseStringUTFChars(env, jdisplayName, displayName);
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_setUserName(JNIEnv*  env
																		,jobject  thiz
																		,jlong address
																		,jstring juserName) {
	const char* userName = GetStringUTFChars(env, juserName);
	linphone_address_set_username((LinphoneAddress*)address,userName);
	ReleaseStringUTFChars(env, juserName, userName);
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_setDomain(JNIEnv*  env
																		,jobject  thiz
																		,jlong address
																		,jstring jdomain) {
	const char* domain = GetStringUTFChars(env, jdomain);
	linphone_address_set_domain((LinphoneAddress*)address,domain);
	ReleaseStringUTFChars(env, jdomain, domain);
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_setTransport(JNIEnv*  env
																		,jobject  thiz
																		,jlong address
																		,jint jtransport) {
	linphone_address_set_transport((LinphoneAddress*)address, (LinphoneTransportType) jtransport);
}
extern "C" void Java_org_linphone_core_LinphoneAddressImpl_setPort(JNIEnv*  env
									,jobject  thiz
									,jlong address
									,jint jport) {
	linphone_address_set_port((LinphoneAddress*)address, (LinphoneTransportType) jport);
}

//CallLog
extern "C" jlong Java_org_linphone_core_LinphoneCallLogImpl_getFrom(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)((LinphoneCallLog*)ptr)->from;
}
extern "C" jint Java_org_linphone_core_LinphoneCallLogImpl_getStatus(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)((LinphoneCallLog*)ptr)->status;
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
extern "C" jstring Java_org_linphone_core_LinphoneCallLogImpl_getStartDate(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	jstring jvalue =env->NewStringUTF(((LinphoneCallLog*)ptr)->start_date);
	return jvalue;
}
extern "C" jlong Java_org_linphone_core_LinphoneCallLogImpl_getTimestamp(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return static_cast<long> (((LinphoneCallLog*)ptr)->start_date_time);
}
extern "C" jint Java_org_linphone_core_LinphoneCallLogImpl_getCallDuration(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)((LinphoneCallLog*)ptr)->duration;
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallLogImpl_wasConference(JNIEnv *env, jobject thiz, jlong ptr) {
	return linphone_call_log_was_conference((LinphoneCallLog *)ptr);
}

/* CallStats */
extern "C" jint Java_org_linphone_core_LinphoneCallStatsImpl_getMediaType(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jint)((LinphoneCallStats *)stats_ptr)->type;
}
extern "C" jint Java_org_linphone_core_LinphoneCallStatsImpl_getIceState(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jint)((LinphoneCallStats *)stats_ptr)->ice_state;
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getDownloadBandwidth(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jfloat)((LinphoneCallStats *)stats_ptr)->download_bandwidth;
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getUploadBandwidth(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jfloat)((LinphoneCallStats *)stats_ptr)->upload_bandwidth;
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getSenderLossRate(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	const LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	return (jfloat) linphone_call_stats_get_sender_loss_rate(stats);
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getReceiverLossRate(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	const LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	return (jfloat) linphone_call_stats_get_receiver_loss_rate(stats);
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getSenderInterarrivalJitter(JNIEnv *env, jobject thiz, jlong stats_ptr, jlong call_ptr) {
	LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	LinphoneCall *call = (LinphoneCall *)call_ptr;
	return (jfloat) linphone_call_stats_get_sender_interarrival_jitter(stats, call);
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getReceiverInterarrivalJitter(JNIEnv *env, jobject thiz, jlong stats_ptr, jlong call_ptr) {
	LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	LinphoneCall *call = (LinphoneCall *)call_ptr;
	return (jfloat) linphone_call_stats_get_receiver_interarrival_jitter(stats, call);
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getRoundTripDelay(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jfloat)((LinphoneCallStats *)stats_ptr)->round_trip_delay;
}
extern "C" jlong Java_org_linphone_core_LinphoneCallStatsImpl_getLatePacketsCumulativeNumber(JNIEnv *env, jobject thiz, jlong stats_ptr, jlong call_ptr) {
	LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	LinphoneCall *call = (LinphoneCall *)call_ptr;
	return (jlong) linphone_call_stats_get_late_packets_cumulative_number(stats, call);
}
extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getJitterBufferSize(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jfloat)((LinphoneCallStats *)stats_ptr)->jitter_stats.jitter_buffer_size_ms;
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getLocalLossRate(JNIEnv *env, jobject thiz,jlong stats_ptr) {
	const LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	return stats->local_loss_rate;
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallStatsImpl_getLocalLateRate(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	const LinphoneCallStats *stats = (LinphoneCallStats *)stats_ptr;
	return stats->local_late_rate;
}

extern "C" void Java_org_linphone_core_LinphoneCallStatsImpl_updateStats(JNIEnv *env, jobject thiz, jlong call_ptr, jint mediatype) {
	if (mediatype==LINPHONE_CALL_STATS_AUDIO)
		linphone_call_get_audio_stats((LinphoneCall*)call_ptr);
	else
		linphone_call_get_video_stats((LinphoneCall*)call_ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneCallStatsImpl_getEncoderName(JNIEnv *env, jobject thiz, jlong stats_ptr, jlong call_ptr, jlong payload_ptr) {
    LinphoneCore *lc = linphone_call_get_core((LinphoneCall*)call_ptr);
    PayloadType* jpayload = (PayloadType*)payload_ptr;
    jstring jencodername =env->NewStringUTF(ms_factory_get_encoder(linphone_core_get_ms_factory(lc), jpayload->mime_type)->text);
    return jencodername;
}

extern "C" jstring Java_org_linphone_core_LinphoneCallStatsImpl_getDecoderName(JNIEnv *env, jobject thiz, jlong stats_ptr, jlong call_ptr, jlong payload_ptr) {
    LinphoneCore *lc = linphone_call_get_core((LinphoneCall*)call_ptr);
    PayloadType* jpayload = (PayloadType*)payload_ptr;
    jstring jdecodername =env->NewStringUTF(ms_factory_get_decoder(linphone_core_get_ms_factory(lc), jpayload->mime_type)->text);
    return jdecodername;
}

extern "C" jint Java_org_linphone_core_LinphoneCallStatsImpl_getIpFamilyOfRemote(JNIEnv *env, jobject thiz, jlong stats_ptr) {
	return (jint) ((LinphoneCallStats *)stats_ptr)->rtp_remote_family;
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
	return (jint)payload_type_get_rate(pt);
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

extern "C" jlongArray Java_org_linphone_core_LinphoneCoreImpl_getCallLogs(JNIEnv*  env
		,jobject  thiz
		,jlong lc) {
	const bctbx_list_t *logs = linphone_core_get_call_logs((LinphoneCore *) lc);
	size_t logsCount = bctbx_list_size(logs);
	jlongArray jLogs = env->NewLongArray(logsCount);
	jlong *jInternalArray = env->GetLongArrayElements(jLogs, NULL);

	for (size_t i = 0; i < logsCount; i++) {
		jInternalArray[i] = (unsigned long) (logs->data);
		logs = logs->next;
	}

	env->ReleaseLongArrayElements(jLogs, jInternalArray, 0);

	return jLogs;
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_takeSnapshot(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr, jstring path) {
	const char* filePath = GetStringUTFChars(env, path);
	linphone_call_take_video_snapshot((LinphoneCall*)ptr, filePath);
	ReleaseStringUTFChars(env, path, filePath);
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_zoomVideo(		JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr, jfloat zoomFactor, jfloat cx, jfloat cy) {
	linphone_call_zoom_video((LinphoneCall*)ptr, zoomFactor, &cx, &cy);
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

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getDiversionAddress(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_call_get_diversion_address((LinphoneCall*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getErrorInfo(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_call_get_error_info((LinphoneCall*)ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneCallImpl_getReason(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)linphone_call_get_reason((LinphoneCall*)ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneCallImpl_getRemoteUserAgent(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneCall *call = (LinphoneCall *)ptr;
	const char *value=linphone_call_get_remote_user_agent(call);
	jstring jvalue=NULL;
	if (value) jvalue=env->NewStringUTF(value);
	return jvalue;
}

extern "C" jstring Java_org_linphone_core_LinphoneCallImpl_getRemoteContact(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneCall *call = (LinphoneCall *)ptr;
	const char *value=linphone_call_get_remote_contact(call);
	jstring jvalue = NULL;
	if (value) jvalue=env->NewStringUTF(value);
	return jvalue;
}

extern "C" jint Java_org_linphone_core_LinphoneCallImpl_getState(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)linphone_call_get_state((LinphoneCall*)ptr);
}

/*
 * Class:     org_linphone_core_LinphoneCallImpl
 * Method:    getTransferState
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCallImpl_getTransferState(JNIEnv *, jobject jobj, jlong callptr){
	LinphoneCall *call=(LinphoneCall*)callptr;
	return linphone_call_get_transfer_state(call);
}

/*
 * Class:     org_linphone_core_LinphoneCallImpl
 * Method:    getTransfererCall
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCallImpl_getTransfererCall(JNIEnv *env, jobject jCall, jlong callptr){
	LinphoneCall *call=(LinphoneCall*)callptr;
	LinphoneCall *ret=linphone_call_get_transferer_call(call);
	return getCall(env,ret);
}

/*
 * Class:     org_linphone_core_LinphoneCallImpl
 * Method:    getTransferTargetCall
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCallImpl_getTransferTargetCall(JNIEnv *env, jobject jCall, jlong callptr){
	LinphoneCall *call=(LinphoneCall*)callptr;
	LinphoneCall *ret=linphone_call_get_transfer_target_call(call);
	return getCall(env,ret);
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
	return (jboolean)linphone_call_echo_cancellation_enabled((LinphoneCall*)ptr);
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
	return (jboolean)linphone_call_echo_limiter_enabled((LinphoneCall*)ptr);
}

extern "C" jobject Java_org_linphone_core_LinphoneCallImpl_getReplacedCall(	JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return getCall(env,linphone_call_get_replaced_call((LinphoneCall*)ptr));
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

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getPlayer(JNIEnv *env, jobject thiz, jlong callPtr) {
	return (jlong)linphone_call_get_player((LinphoneCall *)callPtr);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_mediaInProgress(	JNIEnv*  env
                                                                            ,jobject  thiz
                                                                            ,jlong ptr) {
	return (jboolean) linphone_call_media_in_progress((LinphoneCall*)ptr);
}

//LinphoneFriend
extern "C" jlong Java_org_linphone_core_LinphoneFriendImpl_newLinphoneFriend(JNIEnv*  env
																		,jobject  thiz
																		,jstring jFriendUri) {
	LinphoneFriend* lResult;

	if (jFriendUri) {
		const char* friendUri = GetStringUTFChars(env, jFriendUri);
		lResult = linphone_friend_new_with_address(friendUri);
		linphone_friend_set_user_data(lResult,env->NewWeakGlobalRef(thiz));
		ReleaseStringUTFChars(env, jFriendUri, friendUri);
	} else {
		lResult = linphone_friend_new();
		linphone_friend_set_user_data(lResult,env->NewWeakGlobalRef(thiz));
	}
	return (jlong)lResult;
}

extern "C" jlong Java_org_linphone_core_LinphoneFriendListImpl_newLinphoneFriendList(JNIEnv*  env
																		,jobject  thiz, jlong lc) {
	LinphoneFriendList* fl = linphone_core_create_friend_list((LinphoneCore *)lc);
	linphone_friend_list_set_user_data(fl,env->NewWeakGlobalRef(thiz));
	return (jlong)fl;
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_setUri(JNIEnv* env, jobject thiz, jlong list, jstring juri) {
	const char* uri = GetStringUTFChars(env, juri);
	linphone_friend_list_set_uri((LinphoneFriendList*)list, uri);
	ReleaseStringUTFChars(env, juri, uri);
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_synchronizeFriendsFromServer(JNIEnv* env, jobject thiz, jlong list) {
	linphone_friend_list_synchronize_friends_from_server((LinphoneFriendList*)list);
}

static void contact_created(LinphoneFriendList *list, LinphoneFriend *lf) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(list);
	jobject listener = (jobject) linphone_friend_list_cbs_get_user_data(cbs);

	if (listener == NULL) {
		ms_error("contact_created() notification without listener");
		return ;
	}
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneFriendCreated","(Lorg/linphone/core/LinphoneFriendList;Lorg/linphone/core/LinphoneFriend;)V");
	jobject jlist = getFriendList(env, list);
	jobject jfriend = getFriend(env, lf);
	env->DeleteLocalRef(clazz);
	env->CallVoidMethod(listener, method, jlist, jfriend);
	env->DeleteLocalRef(jfriend);
	if (jlist) {
		env->DeleteLocalRef(jlist);
	}
}

static void contact_updated(LinphoneFriendList *list, LinphoneFriend *lf_new, LinphoneFriend *lf_old) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(list);
	jobject listener = (jobject) linphone_friend_list_cbs_get_user_data(cbs);

	if (listener == NULL) {
		ms_error("contact_updated() notification without listener");
		return ;
	}
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneFriendUpdated","(Lorg/linphone/core/LinphoneFriendList;Lorg/linphone/core/LinphoneFriend;Lorg/linphone/core/LinphoneFriend;)V");
	jobject jlist = getFriendList(env, list);
	jobject jfriend_new = getFriend(env, lf_new);
	jobject jfriend_old = getFriend(env, lf_old);
	env->DeleteLocalRef(clazz);
	env->CallVoidMethod(listener, method, jlist, jfriend_new, jfriend_old);
	env->DeleteLocalRef(jfriend_new);
	env->DeleteLocalRef(jfriend_old);
	if (jlist) {
		env->DeleteLocalRef(jlist);
	}
}

static void contact_removed(LinphoneFriendList *list, LinphoneFriend *lf) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(list);
	jobject listener = (jobject) linphone_friend_list_cbs_get_user_data(cbs);

	if (listener == NULL) {
		ms_error("contact_removed() notification without listener");
		return ;
	}
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneFriendDeleted","(Lorg/linphone/core/LinphoneFriendList;Lorg/linphone/core/LinphoneFriend;)V");
	jobject jlist = getFriendList(env, list);
	jobject jfriend = getFriend(env, lf);
	env->DeleteLocalRef(clazz);
	env->CallVoidMethod(listener, method, jlist, jfriend);
	env->DeleteLocalRef(jfriend);
	if (jlist) {
		env->DeleteLocalRef(jlist);
	}
}

static void sync_status_changed(LinphoneFriendList *list, LinphoneFriendListSyncStatus status, const char *message) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneFriendListCbs *cbs = linphone_friend_list_get_callbacks(list);
	jobject listener = (jobject) linphone_friend_list_cbs_get_user_data(cbs);

	if (listener == NULL) {
		ms_error("sync_status_changed() notification without listener");
		return ;
	}
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneFriendSyncStatusChanged","(Lorg/linphone/core/LinphoneFriendList;Lorg/linphone/core/LinphoneFriendList$State;Ljava/lang/String;)V");
	jobject jlist = getFriendList(env, list);
	env->DeleteLocalRef(clazz);

	LinphoneCore *lc = linphone_friend_list_get_core((LinphoneFriendList *)list);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	jstring msg = message ? env->NewStringUTF(message) : NULL;
	env->CallVoidMethod(listener, method, jlist, env->CallStaticObjectMethod(ljb->friendListSyncStateClass, ljb->friendListSyncStateFromIntId, (jint)status), msg);
	if (msg) {
		env->DeleteLocalRef(msg);
	}
	if (jlist) {
		env->DeleteLocalRef(jlist);
	}
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_setListener(JNIEnv* env, jobject  thiz, jlong ptr, jobject jlistener) {
	jobject listener = env->NewGlobalRef(jlistener);
	LinphoneFriendList *list = (LinphoneFriendList *)ptr;
	LinphoneFriendListCbs *cbs;

	cbs = linphone_friend_list_get_callbacks(list);
	linphone_friend_list_cbs_set_user_data(cbs, listener);
	linphone_friend_list_cbs_set_contact_created(cbs, contact_created);
	linphone_friend_list_cbs_set_contact_updated(cbs, contact_updated);
	linphone_friend_list_cbs_set_contact_deleted(cbs, contact_removed);
	linphone_friend_list_cbs_set_sync_status_changed(cbs, sync_status_changed);
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong linphoneAddress) {
	linphone_friend_set_address((LinphoneFriend*)ptr,(LinphoneAddress*)linphoneAddress);
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jname) {
	const char* name = GetStringUTFChars(env, jname);
	linphone_friend_set_name((LinphoneFriend*)ptr, name);
	ReleaseStringUTFChars(env, jname, name);
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setFamilyName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jname) {
	LinphoneFriend *lf = (LinphoneFriend*)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char* name = GetStringUTFChars(env, jname);
		linphone_vcard_set_family_name(lvc, name);
		ReleaseStringUTFChars(env, jname, name);
	}
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setGivenName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jname) {
	LinphoneFriend *lf = (LinphoneFriend*)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char* name = GetStringUTFChars(env, jname);
		linphone_vcard_set_given_name(lvc, name);
		ReleaseStringUTFChars(env, jname, name);
	}
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_setRLSUri(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jrlsUri) {
	const char* uri = jrlsUri ? GetStringUTFChars(env, jrlsUri) : NULL;
	linphone_friend_list_set_rls_uri((LinphoneFriendList*)ptr, uri);
	if (jrlsUri) ReleaseStringUTFChars(env, jrlsUri, uri);
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_setRLSAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong jrlsAddress) {
	linphone_friend_list_set_rls_address((LinphoneFriendList*)ptr, (LinphoneAddress*)jrlsAddress);
}

extern "C" jobject Java_org_linphone_core_LinphoneFriendListImpl_findFriendByUri(JNIEnv*  env
																		,jobject  thiz
																		,jlong friendListptr
																		,jstring juri) {
	const char* uri = GetStringUTFChars(env, juri);
	LinphoneFriend* lFriend;
	lFriend = linphone_friend_list_find_friend_by_uri((LinphoneFriendList*)friendListptr, uri);
	ReleaseStringUTFChars(env, juri, uri);
	if(lFriend != NULL) {
		jobject jfriend = getFriend(env,lFriend);
		// don't release local ref since it will be handled above by java
		return jfriend;
	} else {
		return NULL;
	}
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_addFriend(JNIEnv*  env
																		,jobject  thiz
																		,jlong friendListptr
																		,jlong friendPtr) {
	linphone_friend_list_add_friend((LinphoneFriendList*)friendListptr, (LinphoneFriend*)friendPtr);
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_addLocalFriend(JNIEnv*  env
																		,jobject  thiz
																		,jlong friendListptr
																		,jlong friendPtr) {
	linphone_friend_list_add_local_friend((LinphoneFriendList*)friendListptr, (LinphoneFriend*)friendPtr);
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneFriendListImpl_getFriendList(JNIEnv* env, jobject thiz, jlong list) {
	const bctbx_list_t* friends = linphone_friend_list_get_friends((LinphoneFriendList *)list);
	size_t friendsSize = bctbx_list_size(friends);
	LinphoneCore *lc = linphone_friend_list_get_core((LinphoneFriendList *)list);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	jobjectArray jFriends = env->NewObjectArray(friendsSize,ljb->friendClass,NULL);

	for (size_t i = 0; i < friendsSize; i++) {
		LinphoneFriend* lfriend = (LinphoneFriend*)friends->data;
		jobject jfriend =  getFriend(env,lfriend);
		if(jfriend != NULL){
			env->SetObjectArrayElement(jFriends, i, jfriend);
			env->DeleteLocalRef(jfriend);
		}
		friends = friends->next;
	}

	return jFriends;
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_updateSubscriptions(JNIEnv*  env
																		,jobject  thiz
																		,jlong friendListptr) {
	linphone_friend_list_update_subscriptions((LinphoneFriendList*)friendListptr);
}

extern "C" jlongArray Java_org_linphone_core_LinphoneFriendImpl_getAddresses(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const bctbx_list_t *addresses = linphone_friend_get_addresses((LinphoneFriend*)ptr);
	size_t size = bctbx_list_size(addresses);
	jlongArray jaddresses = env->NewLongArray(size);
	jlong *jInternalArray = env->GetLongArrayElements(jaddresses, NULL);
	for (size_t i = 0; i < size; i++) {
		jInternalArray[i] = (unsigned long) (addresses->data);
		addresses = bctbx_list_next(addresses);
	}
	env->ReleaseLongArrayElements(jaddresses, jInternalArray, 0);
	return jaddresses;
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_addAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong jaddress) {
	linphone_friend_add_address((LinphoneFriend*)ptr, (LinphoneAddress*)jaddress);
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_removeAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong jaddress) {
	linphone_friend_remove_address((LinphoneFriend*)ptr, (LinphoneAddress*)jaddress);
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneFriendImpl_getPhoneNumbers(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	bctbx_list_t *phone_numbers = linphone_friend_get_phone_numbers((LinphoneFriend*)ptr);
	bctbx_list_t *list = phone_numbers;
	size_t size = bctbx_list_size(phone_numbers);
	jobjectArray jphonenumbers = env->NewObjectArray(size, env->FindClass("java/lang/String"), env->NewStringUTF(""));
	for (size_t i = 0; i < size; i++) {
		const char *phone = (const char *)phone_numbers->data;
		env->SetObjectArrayElement(jphonenumbers, i, env->NewStringUTF(phone));
		phone_numbers = bctbx_list_next(phone_numbers);
	}
	bctbx_list_free(list);
	return jphonenumbers;
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_addPhoneNumber(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jphone) {
	if (jphone) {
		const char* phone = GetStringUTFChars(env, jphone);
		linphone_friend_add_phone_number((LinphoneFriend*)ptr, phone);
		ReleaseStringUTFChars(env, jphone, phone);
	}
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_removePhoneNumber(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jphone) {
	if (jphone) {
		const char* phone = GetStringUTFChars(env, jphone);
		linphone_friend_remove_phone_number((LinphoneFriend*)ptr, phone);
		ReleaseStringUTFChars(env, jphone, phone);
	}
}

extern "C" jlong Java_org_linphone_core_LinphoneFriendImpl_getAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_friend_get_address((LinphoneFriend*)ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneFriendImpl_getName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char *name = linphone_friend_get_name((LinphoneFriend*)ptr);
	return name ? env->NewStringUTF(name) : NULL;
}

extern "C" jstring Java_org_linphone_core_LinphoneFriendImpl_getFamilyName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneFriend *lf = (LinphoneFriend*)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char *name = linphone_vcard_get_family_name(lvc);
		return name ? env->NewStringUTF(name) : NULL;
	}
	return NULL;
}

extern "C" jstring Java_org_linphone_core_LinphoneFriendImpl_getGivenName(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneFriend *lf = (LinphoneFriend*)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char *name = linphone_vcard_get_given_name(lvc);
		return name ? env->NewStringUTF(name) : NULL;
	}
	return NULL;
}

extern "C" jstring Java_org_linphone_core_LinphoneFriendImpl_getOrganization(JNIEnv* env,
																		jobject thiz,
																		jlong ptr) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char *org = linphone_vcard_get_organization(lvc);
		return org ? env->NewStringUTF(org) : NULL;
	}
	return NULL;
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setOrganization(JNIEnv* env,
																		jobject thiz,
																		jlong ptr,
																		jstring jorg) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	LinphoneVcard *lvc = linphone_friend_get_vcard(lf);
	if (lvc) {
		const char* org = GetStringUTFChars(env, jorg);
		linphone_vcard_set_organization(lvc, org);
		ReleaseStringUTFChars(env, jorg, org);
	}
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
	return (jint)linphone_friend_get_inc_subscribe_policy((LinphoneFriend*)ptr);
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
	return (jboolean)linphone_friend_subscribes_enabled((LinphoneFriend*)ptr);
}
extern "C" jint Java_org_linphone_core_LinphoneFriendImpl_getStatus(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jint)linphone_friend_get_status((LinphoneFriend*)ptr);
}
extern "C" jobject Java_org_linphone_core_LinphoneFriendImpl_getCore(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneCore *lc=linphone_friend_get_core((LinphoneFriend*)ptr);
	if (lc!=NULL){
		LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
		jobject core = ljb->getCore();
		return core;
	}
	return NULL;
}

extern "C" jobject Java_org_linphone_core_LinphoneFriendListImpl_getCore(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneCore *lc=linphone_friend_get_core((LinphoneFriend*)ptr);
	if (lc!=NULL){
		jobject core = (jobject)linphone_core_get_user_data(lc);
		return core;
	}
	return NULL;
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setRefKey(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jkey) {
	const char* key = GetStringUTFChars(env, jkey);
	linphone_friend_set_ref_key((LinphoneFriend*)ptr,key);
	ReleaseStringUTFChars(env, jkey, key);
}
extern "C" jstring Java_org_linphone_core_LinphoneFriendImpl_getRefKey(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char * key = linphone_friend_get_ref_key((LinphoneFriend *)ptr);
    return key ? env->NewStringUTF(key) : NULL;
}


extern "C" void  Java_org_linphone_core_LinphoneFriendImpl_finalize(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneFriend *lfriend=(LinphoneFriend*)ptr;
	linphone_friend_set_user_data(lfriend,NULL);
	linphone_friend_unref(lfriend);
}

extern "C" void Java_org_linphone_core_LinphoneFriendListImpl_finalize(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	LinphoneFriendList *lfriendList=(LinphoneFriendList*)ptr;
	linphone_friend_list_set_user_data(lfriendList,NULL);
	linphone_friend_list_unref(lfriendList);
}

extern "C" void Java_org_linphone_core_LinphoneFriendImpl_setPresenceModel(JNIEnv *env, jobject jobj, jlong ptr, jlong modelPtr) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	LinphonePresenceModel *model = (LinphonePresenceModel *)modelPtr;
	linphone_friend_set_presence_model(lf, model);
}

extern "C" jboolean Java_org_linphone_core_LinphoneFriendImpl_isAlreadyPresentInFriendList(JNIEnv* env, jobject thiz, jlong ptr) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	return lf->friend_list != NULL;
}

extern "C" jobject Java_org_linphone_core_LinphoneFriendImpl_getPresenceModelForUri(JNIEnv* env, jobject thiz, jlong ptr, jstring juri) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	const char *uri = GetStringUTFChars(env, juri);
	LinphonePresenceModel *model = (LinphonePresenceModel *)linphone_friend_get_presence_model_for_uri_or_tel(lf, uri);
	ReleaseStringUTFChars(env, juri, uri);
	if (model == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceModelImpl", linphone_presence_model, model);
}

extern

/*
 * Class:     org_linphone_core_LinphoneFriendImpl
 * Method:    getPresenceModel
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneFriendImpl_getPresenceModel(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphoneFriend *lf = (LinphoneFriend *)ptr;
	LinphonePresenceModel *model = (LinphonePresenceModel *)linphone_friend_get_presence_model(lf);
	if (model == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceModelImpl", linphone_presence_model, model);
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
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_removeFriend(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jlong lf) {
	linphone_core_remove_friend((LinphoneCore*)ptr, (LinphoneFriend*)lf);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getFriendByAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jaddress) {
	const char* address = GetStringUTFChars(env, jaddress);
	LinphoneFriend *lf = linphone_core_get_friend_by_address((LinphoneCore*)ptr, address);
	ReleaseStringUTFChars(env, jaddress, address);
	if(lf != NULL) {
		jobject jfriend = getFriend(env,lf);
		return jfriend;
	} else {
		return NULL;
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_createFriend( JNIEnv* env, jobject thiz, jlong ptr) {
	LinphoneFriend *lf = linphone_core_create_friend((LinphoneCore*)ptr);
	if(lf != NULL) {
		jobject jfriend = getFriend(env,lf);
		return jfriend;
	} else {
		return NULL;
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_createFriendWithAddress( JNIEnv* env, jobject thiz, jlong ptr, jstring jaddress) {
	const char* address = GetStringUTFChars(env, jaddress);
	LinphoneFriend *lf = linphone_core_create_friend_with_address((LinphoneCore*)ptr, address);
	ReleaseStringUTFChars(env, jaddress, address);
	if(lf != NULL) {
		jobject jfriend = getFriend(env,lf);
		return jfriend;
	} else {
		return NULL;
	}
}

extern "C" jobjectArray _LinphoneChatRoomImpl_getHistory(JNIEnv* env, jobject thiz, jlong ptr, bctbx_list_t* history) {
	LinphoneChatRoom *room = (LinphoneChatRoom *)ptr;
	LinphoneCore *lc = linphone_chat_room_get_core(room);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	bctbx_list_t *list = history;
	size_t historySize = bctbx_list_size(history);
	jobjectArray jHistory = env->NewObjectArray(historySize, ljb->chatMessageClass, NULL);

	for (size_t i = 0; i < historySize; i++) {
		LinphoneChatMessage *msg = (LinphoneChatMessage *)history->data;
		jobject jmsg = getChatMessage(env, msg);
		if (jmsg != NULL) {
			env->SetObjectArrayElement(jHistory, i, jmsg);
			env->DeleteLocalRef(jmsg);
		}

		history = history->next;
	}
	/*getChatMessage() acquired a ref that is "transfered" to the java object. We must drop
		* the reference given by linphone_chat_room_get_history_range()*/
	bctbx_list_free_with_data(list, (void (*)(void*))linphone_chat_message_unref);
	return jHistory;
}
extern "C" jobjectArray Java_org_linphone_core_LinphoneChatRoomImpl_getHistoryRange(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jint start
																		,jint end) {
	bctbx_list_t* history = linphone_chat_room_get_history_range((LinphoneChatRoom*)ptr, start, end);
	return _LinphoneChatRoomImpl_getHistory(env, thiz, ptr, history);
}
extern "C" jobjectArray Java_org_linphone_core_LinphoneChatRoomImpl_getHistory(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jint limit) {
	bctbx_list_t* history = linphone_chat_room_get_history((LinphoneChatRoom*)ptr, limit);
	return _LinphoneChatRoomImpl_getHistory(env, thiz, ptr, history);
}
extern "C" jlong Java_org_linphone_core_LinphoneChatRoomImpl_getPeerAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) linphone_chat_room_get_peer_address((LinphoneChatRoom*)ptr);
}
extern "C" jlong Java_org_linphone_core_LinphoneChatRoomImpl_createLinphoneChatMessage(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jmessage) {
	const char* message = GetStringUTFChars(env, jmessage);
	LinphoneChatMessage *chatMessage = linphone_chat_room_create_message((LinphoneChatRoom *)ptr, message);
	ReleaseStringUTFChars(env, jmessage, message);

	return (jlong) chatMessage;
}
extern "C" jlong Java_org_linphone_core_LinphoneChatRoomImpl_createLinphoneChatMessage2(JNIEnv* env
																		,jobject thiz
																		,jlong ptr
																		,jstring jmessage
																		,jstring jurl
																		,jint state
																		,jlong time
																		,jboolean read
																		,jboolean incoming) {
	const char* message = GetStringUTFChars(env, jmessage);
	const char* url = GetStringUTFChars(env, jurl);

	LinphoneChatMessage *chatMessage = linphone_chat_room_create_message_2(
				(LinphoneChatRoom *)ptr, message, url, (LinphoneChatMessageState)state,
				(time_t)time, read, incoming);

	ReleaseStringUTFChars(env, jmessage, message);
	ReleaseStringUTFChars(env, jurl, url);

	return (jlong) chatMessage;
}
extern "C" jint Java_org_linphone_core_LinphoneChatRoomImpl_getHistorySize		(JNIEnv*  env
																				  ,jobject  thiz
																				  ,jlong ptr) {
	return (jint) linphone_chat_room_get_history_size((LinphoneChatRoom*)ptr);
}
extern "C" jint Java_org_linphone_core_LinphoneChatRoomImpl_getUnreadMessagesCount(JNIEnv*  env
																				  ,jobject  thiz
																				  ,jlong ptr) {
	return (jint) linphone_chat_room_get_unread_messages_count((LinphoneChatRoom*)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_deleteHistory(JNIEnv*  env
																	,jobject  thiz
																	,jlong ptr) {
	linphone_chat_room_delete_history((LinphoneChatRoom*)ptr);
}
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneChatRoomImpl_compose(JNIEnv *env, jobject thiz, jlong ptr) {
	linphone_chat_room_compose((LinphoneChatRoom *)ptr);
}
JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneChatRoomImpl_isRemoteComposing(JNIEnv *env, jobject thiz, jlong ptr) {
	return (jboolean)linphone_chat_room_is_remote_composing((LinphoneChatRoom *)ptr);
}
extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_deleteMessage(JNIEnv*  env
																	,jobject  thiz
																	,jlong room
																	,jlong msg) {
	linphone_chat_room_delete_message((LinphoneChatRoom*)room, (LinphoneChatMessage*)msg);
}
extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_markAsRead(JNIEnv*  env
																	   ,jobject  thiz
																	   ,jlong ptr) {
	linphone_chat_room_mark_as_read((LinphoneChatRoom*)ptr);
}


extern "C" jlong Java_org_linphone_core_LinphoneChatRoomImpl_createFileTransferMessage(JNIEnv* env, jobject thiz, jlong ptr, jstring jname, jstring jtype, jstring jsubtype, jint data_size) {
	LinphoneCore *lc = linphone_chat_room_get_core((LinphoneChatRoom*) ptr);
	LinphoneContent * content = linphone_core_create_content(lc);
	LinphoneChatMessage *message = NULL;
	const char *tmp;

	linphone_content_set_type(content, tmp = GetStringUTFChars(env, jtype));
	ReleaseStringUTFChars(env, jtype, tmp);

	linphone_content_set_subtype(content, tmp = GetStringUTFChars(env, jsubtype));
	ReleaseStringUTFChars(env, jsubtype, tmp);

	linphone_content_set_name(content, tmp = GetStringUTFChars(env, jname));
	ReleaseStringUTFChars(env, jname, tmp);

	linphone_content_set_size(content, data_size);

	message = linphone_chat_room_create_file_transfer_message((LinphoneChatRoom *)ptr, content);

	linphone_content_unref(content);

	return (jlong) message;
}



extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_cancelFileTransfer(JNIEnv* env, jobject  thiz, jlong ptr) {
	linphone_chat_message_cancel_file_transfer((LinphoneChatMessage *)ptr);
}

extern "C" jobject Java_org_linphone_core_LinphoneChatMessageImpl_getFileTransferInformation(JNIEnv* env, jobject  thiz, jlong ptr) {
	const LinphoneContent *content = linphone_chat_message_get_file_transfer_information((LinphoneChatMessage *)ptr);
	if (content)
		return create_java_linphone_content(env, content);
	return NULL;
}

extern "C" jstring Java_org_linphone_core_LinphoneChatMessageImpl_getAppData(JNIEnv* env, jobject  thiz, jlong ptr) {
	const char * app_data = linphone_chat_message_get_appdata((LinphoneChatMessage *)ptr);
	return app_data ? env->NewStringUTF(app_data) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_setAppData(JNIEnv* env, jobject  thiz, jlong ptr, jstring appdata) {
	const char * data = GetStringUTFChars(env, appdata);
	linphone_chat_message_set_appdata((LinphoneChatMessage *)ptr, data);
	ReleaseStringUTFChars(env, appdata, data);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setFileTransferServer(JNIEnv* env, jobject thiz, jlong ptr, jstring server_url) {
	const char * url = GetStringUTFChars(env, server_url);
	linphone_core_set_file_transfer_server((LinphoneCore *)ptr, url);
	ReleaseStringUTFChars(env, server_url, url);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getFileTransferServer(JNIEnv* env, jobject thiz, jlong ptr) {
	const char * server_url = linphone_core_get_file_transfer_server((LinphoneCore *)ptr);
	return server_url ? env->NewStringUTF(server_url) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_store(JNIEnv*  env
																			,jobject  thiz
																			,jlong ptr) {
	linphone_chat_message_store((LinphoneChatMessage*)ptr);
}

extern "C" jbyteArray Java_org_linphone_core_LinphoneChatMessageImpl_getText(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	const char *message = linphone_chat_message_get_text((LinphoneChatMessage*)ptr);
	if (message){
		size_t length = strlen(message);
		jbyteArray array = env->NewByteArray(length);
		env->SetByteArrayRegion(array, 0, length, (const jbyte*)message);
		return array;
	}
	return NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneChatMessageImpl_getReason(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return linphone_chat_message_get_reason((LinphoneChatMessage*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneChatMessageImpl_getErrorInfo(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong)linphone_chat_message_get_error_info((LinphoneChatMessage*)ptr);
}

extern "C" jstring Java_org_linphone_core_LinphoneChatMessageImpl_getCustomHeader(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr, jstring jheader_name) {
	const char *name = GetStringUTFChars(env, jheader_name);
	const char *value=linphone_chat_message_get_custom_header((LinphoneChatMessage*)ptr,name);
	ReleaseStringUTFChars(env, jheader_name, name);
	return value ? env->NewStringUTF(value) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_addCustomHeader(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr, jstring jheader_name, jstring jheader_value) {
	const char *name = GetStringUTFChars(env, jheader_name);
	const char *value = GetStringUTFChars(env, jheader_value);
	linphone_chat_message_add_custom_header((LinphoneChatMessage*)ptr,name,value);
	ReleaseStringUTFChars(env, jheader_name, name);
	ReleaseStringUTFChars(env, jheader_value, value);
}

extern "C" jstring Java_org_linphone_core_LinphoneChatMessageImpl_getExternalBodyUrl(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	jstring jvalue =env->NewStringUTF(linphone_chat_message_get_external_body_url((LinphoneChatMessage*)ptr));
	return jvalue;
}
extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_setExternalBodyUrl(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jurl) {
	const char* url = GetStringUTFChars(env, jurl);
	linphone_chat_message_set_external_body_url((LinphoneChatMessage *)ptr, url);
	ReleaseStringUTFChars(env, jurl, url);
}
extern "C" jlong Java_org_linphone_core_LinphoneChatMessageImpl_getFrom(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) linphone_chat_message_get_from((LinphoneChatMessage*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneChatMessageImpl_getTo(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) linphone_chat_message_get_to((LinphoneChatMessage*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneChatMessageImpl_getPeerAddress(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) linphone_chat_message_get_peer_address((LinphoneChatMessage*)ptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneChatMessageImpl_getTime(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr) {
	return (jlong) linphone_chat_message_get_time((LinphoneChatMessage*)ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneChatMessageImpl_getStatus(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	return (jint) linphone_chat_message_get_state((LinphoneChatMessage*)ptr);
}

extern "C" jboolean Java_org_linphone_core_LinphoneChatMessageImpl_isRead(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	return (jboolean) linphone_chat_message_is_read((LinphoneChatMessage*)ptr);
}

extern "C" jboolean Java_org_linphone_core_LinphoneChatMessageImpl_isOutgoing(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	return (jboolean) linphone_chat_message_is_outgoing((LinphoneChatMessage*)ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneChatMessageImpl_getStorageId(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	return (jint) linphone_chat_message_get_storage_id((LinphoneChatMessage*)ptr);
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_setFileTransferFilepath(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_chat_message_set_file_transfer_filepath((LinphoneChatMessage*)ptr, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" jint Java_org_linphone_core_LinphoneChatMessageImpl_downloadFile(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	return (jint) linphone_chat_message_download_file((LinphoneChatMessage*)ptr);
}

static void message_state_changed(LinphoneChatMessage* msg, LinphoneChatMessageState state) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	jobject listener = (jobject) msg->message_state_changed_user_data;

	if (listener == NULL) {
		ms_error("message_state_changed() notification without listener");
		return ;
	}
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneChatMessageStateChanged","(Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneChatMessage$State;)V");
	jobject jmessage = getChatMessage(env, msg);
	env->DeleteLocalRef(clazz);

	LinphoneChatRoom *room = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(room);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	env->CallVoidMethod(listener, method, jmessage, env->CallStaticObjectMethod(ljb->chatMessageStateClass, ljb->chatMessageStateFromIntId, (jint)state));

	if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateNotDelivered) {
		env->DeleteGlobalRef(listener);
		msg->message_state_changed_user_data = NULL;
	}
	if (jmessage) {
		env->DeleteLocalRef(jmessage);
	}
}

static void file_transfer_progress_indication(LinphoneChatMessage *msg, const LinphoneContent* content, size_t offset, size_t total) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	jobject listener = (jobject) msg->message_state_changed_user_data;
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneChatMessageFileTransferProgressChanged", "(Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;II)V");
	env->DeleteLocalRef(clazz);
	jobject jmessage = getChatMessage(env, msg);
	jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;
	env->CallVoidMethod(listener, method, jmessage, jcontent, offset, total);
	if (jcontent) {
		env->DeleteLocalRef(jcontent);
	}
	if (jmessage) {
		env->DeleteLocalRef(jmessage);
	}
}

static void file_transfer_recv(LinphoneChatMessage *msg, const LinphoneContent* content, const LinphoneBuffer *buffer) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	jobject listener = (jobject) msg->message_state_changed_user_data;
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneChatMessageFileTransferReceived", "(Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;Lorg/linphone/core/LinphoneBuffer;)V");
	env->DeleteLocalRef(clazz);

	jobject jmessage = getChatMessage(env, msg);
	jobject jbuffer = buffer ? create_java_linphone_buffer(env, buffer) : NULL;
	jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;
	env->CallVoidMethod(listener, method, jmessage, jcontent, jbuffer);
	if (jbuffer) {
		env->DeleteLocalRef(jbuffer);
	}
	if (jcontent) {
		env->DeleteLocalRef(jcontent);
	}
	if (jmessage) {
		env->DeleteLocalRef(jmessage);
	}
}

static LinphoneBuffer* file_transfer_send(LinphoneChatMessage *msg,  const LinphoneContent* content, size_t offset, size_t size) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	LinphoneBuffer *buffer = NULL;
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return buffer;
	}

	jobject listener = (jobject) msg->message_state_changed_user_data;
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneChatMessageFileTransferSent","(Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneContent;IILorg/linphone/core/LinphoneBuffer;)V");
	env->DeleteLocalRef(clazz);

	jobject jmessage = getChatMessage(env, msg);
	jobject jbuffer = create_java_linphone_buffer(env, NULL);
	jobject jcontent = content ? create_java_linphone_content(env, content) : NULL;
	env->CallVoidMethod(listener, method, jmessage, jcontent, offset, size, jbuffer);
	if (jcontent) {
		env->DeleteLocalRef(jcontent);
	}
	if (jmessage) {
		env->DeleteLocalRef(jmessage);
	}

	buffer = create_c_linphone_buffer_from_java_linphone_buffer(env, jbuffer);
	env->DeleteLocalRef(jbuffer);
	return buffer;
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_setListener(JNIEnv* env, jobject  thiz, jlong ptr, jobject jlistener) {
	jobject listener = env->NewGlobalRef(jlistener);
	LinphoneChatMessage *message = (LinphoneChatMessage *)ptr;
	LinphoneChatMessageCbs *cbs;

	message->message_state_changed_user_data = listener;
	cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, message_state_changed);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
	linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_recv);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, file_transfer_send);
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_unref(JNIEnv*  env
																		 ,jobject  thiz
																		 ,jlong ptr) {
	linphone_chat_message_set_user_data((LinphoneChatMessage*)ptr, NULL);
	linphone_chat_message_unref((LinphoneChatMessage*)ptr);
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_getChatRooms(JNIEnv*  env
																		   ,jobject  thiz
																		   ,jlong ptr) {
	LinphoneCore *lc = (LinphoneCore*)ptr;
	const bctbx_list_t* chats = linphone_core_get_chat_rooms(lc);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	size_t chatsSize = bctbx_list_size(chats);
	jobjectArray jChats = env->NewObjectArray(chatsSize, ljb->chatRoomClass, NULL);

	for (size_t i = 0; i < chatsSize; i++) {
		LinphoneChatRoom *room = (LinphoneChatRoom *)chats->data;
		jobject jroom = getChatRoom(env, room);
		if (jroom != NULL) {
			env->SetObjectArrayElement(jChats, i, jroom);
			env->DeleteLocalRef(jroom);
		}
		chats = chats->next;
	}

	return jChats;
}

extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_sendMessage(JNIEnv*  env
																		,jobject  thiz
																		,jlong ptr
																		,jstring jmessage) {
	const char* message = GetStringUTFChars(env, jmessage);
	linphone_chat_room_send_message((LinphoneChatRoom*)ptr, message);
	ReleaseStringUTFChars(env, jmessage, message);
}

static void chat_room_impl_callback(LinphoneChatMessage* msg, LinphoneChatMessageState state, void* ud) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	jobject listener = (jobject) ud;
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onLinphoneChatMessageStateChanged","(Lorg/linphone/core/LinphoneChatMessage;Lorg/linphone/core/LinphoneChatMessage$State;)V");
	jobject jmessage=(jobject)linphone_chat_message_get_user_data(msg);

	LinphoneChatRoom *room = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(room);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	env->CallVoidMethod(
			listener,
			method,
			jmessage,
			env->CallStaticObjectMethod(ljb->chatMessageStateClass,ljb->chatMessageStateFromIntId,(jint)state));

	if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateNotDelivered) {
		env->DeleteGlobalRef(listener);
		env->DeleteGlobalRef(jmessage);
		linphone_chat_message_set_user_data(msg,NULL);
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneChatRoomImpl_getCore(JNIEnv*  env
																		,jobject  thiz
																		,jlong chatroom_ptr){
	LinphoneCore *lc=linphone_chat_room_get_core((LinphoneChatRoom*)chatroom_ptr);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	jobject core = ljb->getCore();
	return core;
}

extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_sendMessage2(JNIEnv*  env
																		,jobject  thiz
																		,jlong chatroom_ptr
																		,jobject message
																		,jlong messagePtr
																		,jobject jlistener) {
	jobject listener = env->NewGlobalRef(jlistener);
	message = env->NewGlobalRef(message);
	linphone_chat_message_ref((LinphoneChatMessage*)messagePtr);
	linphone_chat_message_set_user_data((LinphoneChatMessage*)messagePtr, message);
	linphone_chat_room_send_message2((LinphoneChatRoom*)chatroom_ptr, (LinphoneChatMessage*)messagePtr, chat_room_impl_callback, (void*)listener);
}

extern "C" void Java_org_linphone_core_LinphoneChatRoomImpl_sendChatMessage(JNIEnv*  env
																		,jobject  thiz
																		,jlong chatroom_ptr
																		,jobject message
																		,jlong messagePtr) {
	message = env->NewGlobalRef(message);
	linphone_chat_message_ref((LinphoneChatMessage*)messagePtr);
	linphone_chat_message_set_user_data((LinphoneChatMessage*)messagePtr, message);
	linphone_chat_room_send_chat_message((LinphoneChatRoom*)chatroom_ptr, (LinphoneChatMessage*)messagePtr);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoWindowId(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jobject obj) {
	jobject oldWindow = (jobject) linphone_core_get_native_video_window_id((LinphoneCore*)lc);
	if (obj != NULL) {
		obj = env->NewGlobalRef(obj);
		ms_message("Java_org_linphone_core_LinphoneCoreImpl_setVideoWindowId(): NewGlobalRef(%p)",obj);
	}else ms_message("Java_org_linphone_core_LinphoneCoreImpl_setVideoWindowId(): setting to NULL");
	linphone_core_set_native_video_window_id((LinphoneCore*)lc,(void *)obj);
	if (oldWindow != NULL) {
		ms_message("Java_org_linphone_core_LinphoneCoreImpl_setVideoWindowId(): DeleteGlobalRef(%p)",oldWindow);
		env->DeleteGlobalRef(oldWindow);
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPreviewWindowId(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jobject obj) {
	jobject oldWindow = (jobject) linphone_core_get_native_preview_window_id((LinphoneCore*)lc);
	if (obj != NULL) {
		obj = env->NewGlobalRef(obj);
		ms_message("Java_org_linphone_core_LinphoneCoreImpl_setPreviewWindowId(): NewGlobalRef(%p)",obj);
	}else ms_message("Java_org_linphone_core_LinphoneCoreImpl_setPreviewWindowId(): setting to NULL");
	linphone_core_set_native_preview_window_id((LinphoneCore*)lc,(void *)obj);
	if (oldWindow != NULL) {
		ms_message("Java_org_linphone_core_LinphoneCoreImpl_setPreviewWindowId(): DeleteGlobalRef(%p)",oldWindow);
		env->DeleteGlobalRef(oldWindow);
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDeviceRotation(JNIEnv* env
																		,jobject thiz
																		,jlong lc
																		,jint rotation) {
	linphone_core_set_device_rotation((LinphoneCore*)lc,rotation);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setRemoteRingbackTone(JNIEnv *env, jobject thiz, jlong lc, jstring jtone){
	const char* tone = GetStringUTFChars(env, jtone);
	linphone_core_set_remote_ringback_tone((LinphoneCore*)lc,tone);
	ReleaseStringUTFChars(env, jtone,tone);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getRemoteRingbackTone(JNIEnv *env, jobject thiz, jlong lc){
	const char *ret= linphone_core_get_remote_ringback_tone((LinphoneCore*)lc);
	if (ret==NULL) return NULL;
	jstring jvalue =env->NewStringUTF(ret);
	return jvalue;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setFirewallPolicy(JNIEnv *env, jobject thiz, jlong lc, jint enum_value){
	linphone_core_set_firewall_policy((LinphoneCore*)lc,(LinphoneFirewallPolicy)enum_value);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getFirewallPolicy(JNIEnv *env, jobject thiz, jlong lc){
	return (jint)linphone_core_get_firewall_policy((LinphoneCore*)lc);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_createNatPolicy(JNIEnv *env, jobject thiz, jlong lc) {
	LinphoneNatPolicy *nat_policy = linphone_core_create_nat_policy((LinphoneCore *)lc);
	return (nat_policy != NULL) ? getNatPolicy(env, nat_policy) : NULL;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setNatPolicy(JNIEnv *env, jobject thiz, jlong lc, jlong jpolicy) {
	linphone_core_set_nat_policy((LinphoneCore *)lc, (LinphoneNatPolicy *)jpolicy);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_getNatPolicy(JNIEnv *env, jobject thiz, jlong lc) {
	LinphoneNatPolicy *nat_policy = linphone_core_get_nat_policy((LinphoneCore *)lc);
	return (nat_policy != NULL) ? getNatPolicy(env, nat_policy) : NULL;
}


extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setStunServer(JNIEnv *env, jobject thiz, jlong lc, jstring jserver){
	const char* server = GetStringUTFChars(env, jserver);
	linphone_core_set_stun_server((LinphoneCore*)lc,server);
	ReleaseStringUTFChars(env, jserver,server);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getStunServer(JNIEnv *env, jobject thiz, jlong lc){
	const char *ret= linphone_core_get_stun_server((LinphoneCore*)lc);
	if (ret==NULL) return NULL;
	jstring jvalue =env->NewStringUTF(ret);
	return jvalue;
}

//CallParams
extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_isLowBandwidthEnabled(JNIEnv *env, jobject thiz, jlong cp) {
	return (jboolean) linphone_call_params_low_bandwidth_enabled((LinphoneCallParams *)cp);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableLowBandwidth(JNIEnv *env, jobject thiz, jlong cp, jboolean enable) {
	linphone_call_params_enable_low_bandwidth((LinphoneCallParams *)cp, enable);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallParamsImpl_getUsedAudioCodec(JNIEnv *env, jobject thiz, jlong cp) {
	return (jlong)linphone_call_params_get_used_audio_codec((LinphoneCallParams *)cp);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallParamsImpl_getUsedVideoCodec(JNIEnv *env, jobject thiz, jlong cp) {
	return (jlong)linphone_call_params_get_used_video_codec((LinphoneCallParams *)cp);
}

extern "C" jint Java_org_linphone_core_LinphoneCallParamsImpl_getMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			) {
	return (jint)linphone_call_params_get_media_encryption((LinphoneCallParams*)cp);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_setMediaEncryption(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			,jint jmenc) {
	linphone_call_params_set_media_encryption((LinphoneCallParams*)cp,(LinphoneMediaEncryption)jmenc);
}

extern "C" jint Java_org_linphone_core_LinphoneCallParamsImpl_getPrivacy(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			) {
	return (jint)linphone_call_params_get_privacy((LinphoneCallParams*)cp);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_setPrivacy(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			,jint privacy) {
	linphone_call_params_set_privacy((LinphoneCallParams*)cp,privacy);
}

extern "C" jstring Java_org_linphone_core_LinphoneCallParamsImpl_getSessionName(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			) {
	const char* name = linphone_call_params_get_session_name((LinphoneCallParams*)cp);
	return name ? env->NewStringUTF(name) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_setSessionName(JNIEnv*  env
																			,jobject  thiz
																			,jlong cp
																			,jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	linphone_call_params_set_session_name((LinphoneCallParams*)cp,name);
	ReleaseStringUTFChars(env, jname, name);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_audioBandwidth(JNIEnv *env, jobject thiz, jlong lcp, jint bw){
	linphone_call_params_set_audio_bandwidth_limit((LinphoneCallParams*)lcp, bw);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableVideo(JNIEnv *env, jobject thiz, jlong lcp, jboolean b){
	linphone_call_params_enable_video((LinphoneCallParams*)lcp, b);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_getVideoEnabled(JNIEnv *env, jobject thiz, jlong lcp){
	return (jboolean)linphone_call_params_video_enabled((LinphoneCallParams*)lcp);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableVideoMulticast(JNIEnv *env, jobject thiz, jlong lcp, jboolean b){
	linphone_call_params_enable_video_multicast((LinphoneCallParams*)lcp, b);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_videoMulticastEnabled(JNIEnv *env, jobject thiz, jlong lcp){
	return (jboolean)linphone_call_params_video_multicast_enabled((LinphoneCallParams*)lcp);
}
extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableAudioMulticast(JNIEnv *env, jobject thiz, jlong lcp, jboolean b){
	linphone_call_params_enable_audio_multicast((LinphoneCallParams*)lcp, b);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_audioMulticastEnabled(JNIEnv *env, jobject thiz, jlong lcp){
	return (jboolean)linphone_call_params_audio_multicast_enabled((LinphoneCallParams*)lcp);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_localConferenceMode(JNIEnv *env, jobject thiz, jlong lcp){
	return (jboolean)linphone_call_params_get_local_conference_mode((LinphoneCallParams*)lcp);
}

extern "C" jstring Java_org_linphone_core_LinphoneCallParamsImpl_getCustomHeader(JNIEnv *env, jobject thiz, jlong lcp, jstring jheader_name){
	const char* header_name = GetStringUTFChars(env, jheader_name);
	const char *header_value = linphone_call_params_get_custom_header((LinphoneCallParams*)lcp,header_name);
	ReleaseStringUTFChars(env, jheader_name, header_name);
	return header_value ? env->NewStringUTF(header_value) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_addCustomHeader(JNIEnv *env, jobject thiz, jlong lcp, jstring jheader_name, jstring jheader_value){
	const char* header_name = GetStringUTFChars(env, jheader_name);
	const char* header_value = GetStringUTFChars(env, jheader_value);
	linphone_call_params_add_custom_header((LinphoneCallParams*)lcp,header_name,header_value);
	ReleaseStringUTFChars(env, jheader_name, header_name);
	ReleaseStringUTFChars(env, jheader_value, header_value);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_addCustomSdpAttribute(JNIEnv *env, jobject thiz, jlong ptr, jstring jname, jstring jvalue) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = GetStringUTFChars(env, jvalue);
	linphone_call_params_add_custom_sdp_attribute((LinphoneCallParams *)ptr, name, value);
	ReleaseStringUTFChars(env, jname, name);
	ReleaseStringUTFChars(env, jvalue, value);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_addCustomSdpMediaAttribute(JNIEnv *env, jobject thiz, jlong ptr, jint jtype, jstring jname, jstring jvalue) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = GetStringUTFChars(env, jvalue);
	linphone_call_params_add_custom_sdp_media_attribute((LinphoneCallParams *)ptr, (LinphoneStreamType)jtype, name, value);
	ReleaseStringUTFChars(env, jname, name);
	ReleaseStringUTFChars(env, jvalue, value);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_getCustomSdpAttribute(JNIEnv *env, jobject thiz, jlong ptr, jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = linphone_call_params_get_custom_sdp_attribute((LinphoneCallParams *)ptr, name);
	ReleaseStringUTFChars(env, jname, name);
	return value ? env->NewStringUTF(value) : NULL;
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_getCustomSdpMediaAttribute(JNIEnv *env, jobject thiz, jlong ptr, jint jtype, jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = linphone_call_params_get_custom_sdp_media_attribute((LinphoneCallParams *)ptr, (LinphoneStreamType)jtype, name);
	ReleaseStringUTFChars(env, jname, name);
	return value ? env->NewStringUTF(value) : NULL;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_clearCustomSdpAttributes(JNIEnv *env, jobject thiz, jlong ptr) {
	linphone_call_params_clear_custom_sdp_attributes((LinphoneCallParams *)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_clearCustomSdpMediaAttributes(JNIEnv *env, jobject thiz, jlong ptr, jint jtype) {
	linphone_call_params_clear_custom_sdp_media_attributes((LinphoneCallParams *)ptr, (LinphoneStreamType)jtype);
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_setRecordFile(JNIEnv *env, jobject thiz, jlong lcp, jstring jrecord_file){
	if (jrecord_file){
		const char* record_file = GetStringUTFChars(env, jrecord_file);
		linphone_call_params_set_record_file((LinphoneCallParams*)lcp,record_file);
		ReleaseStringUTFChars(env, jrecord_file, record_file);
	}else linphone_call_params_set_record_file((LinphoneCallParams*)lcp,NULL);
}

extern "C" jintArray Java_org_linphone_core_LinphoneCallParamsImpl_getSentVideoSize(JNIEnv *env, jobject thiz, jlong lcp) {
	const LinphoneCallParams *params = (LinphoneCallParams *) lcp;
	MSVideoSize vsize = linphone_call_params_get_sent_video_size(params);
	jintArray arr = env->NewIntArray(2);
	int tVsize [2]= {vsize.width,vsize.height};
	env->SetIntArrayRegion(arr, 0, 2, tVsize);
	return arr;
}

extern "C" jintArray Java_org_linphone_core_LinphoneCallParamsImpl_getReceivedVideoSize(JNIEnv *env, jobject thiz, jlong lcp) {
	const LinphoneCallParams *params = (LinphoneCallParams *) lcp;
	MSVideoSize vsize = linphone_call_params_get_received_video_size(params);
	jintArray arr = env->NewIntArray(2);
	int tVsize [2]= {vsize.width,vsize.height};
	env->SetIntArrayRegion(arr, 0, 2, tVsize);
	return arr;
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_getAudioDirection(JNIEnv *env, jobject thiz, jlong ptr) {
	return (jint)linphone_call_params_get_audio_direction((LinphoneCallParams *)ptr);
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_getVideoDirection(JNIEnv *env, jobject thiz, jlong ptr) {
	return (jint)linphone_call_params_get_video_direction((LinphoneCallParams *)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_setAudioDirection(JNIEnv *env, jobject thiz, jlong ptr, jint jdir) {
	linphone_call_params_set_audio_direction((LinphoneCallParams *)ptr, (LinphoneMediaDirection)jdir);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallParamsImpl_setVideoDirection(JNIEnv *env, jobject thiz, jlong ptr, jint jdir) {
	linphone_call_params_set_video_direction((LinphoneCallParams *)ptr, (LinphoneMediaDirection)jdir);
}


extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_destroy(JNIEnv *env, jobject thiz, jlong lc){
	return linphone_call_params_destroy((LinphoneCallParams*)lc);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    createCallParams
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_LinphoneCoreImpl_createCallParams(JNIEnv *env, jobject jcore, jlong coreptr, jlong callptr){
	return (jlong)linphone_core_create_call_params((LinphoneCore*)coreptr, (LinphoneCall*)callptr);
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getRemoteParams(JNIEnv *env, jobject thiz, jlong lc){
	if (linphone_call_get_remote_params((LinphoneCall*)lc) == NULL) {
			return (jlong) 0;
	}
	return (jlong) linphone_call_params_copy(linphone_call_get_remote_params((LinphoneCall*)lc));
}

extern "C" jlong Java_org_linphone_core_LinphoneCallImpl_getCurrentParamsCopy(JNIEnv *env, jobject thiz, jlong lc){
	return (jlong) linphone_call_params_copy(linphone_call_get_current_params((LinphoneCall*)lc));
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_enableCamera(JNIEnv *env, jobject thiz, jlong lc, jboolean b){
	linphone_call_enable_camera((LinphoneCall *)lc, (bool_t) b);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_cameraEnabled(JNIEnv *env, jobject thiz, jlong lc){
	return (jboolean)linphone_call_camera_enabled((LinphoneCall *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_startRecording(JNIEnv *env, jobject thiz, jlong lc){
	linphone_call_start_recording((LinphoneCall *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCallImpl_stopRecording(JNIEnv *env, jobject thiz, jlong lc){
	linphone_call_stop_recording((LinphoneCall *)lc);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_inviteAddressWithParams(JNIEnv *env, jobject thiz, jlong lc, jlong addr, jlong params){
	return  getCall(env,linphone_core_invite_address_with_params((LinphoneCore *)lc, (const LinphoneAddress *)addr, (const LinphoneCallParams *)params));
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

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setPreferredFramerate(JNIEnv *env, jobject thiz, jlong lc, jfloat framerate){
	linphone_core_set_preferred_framerate((LinphoneCore *)lc, framerate);
}

extern "C" float Java_org_linphone_core_LinphoneCoreImpl_getPreferredFramerate(JNIEnv *env, jobject thiz, jlong lc){
	return linphone_core_get_preferred_framerate((LinphoneCore *)lc);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setPreferredVideoSizeByName(JNIEnv *env, jobject thiz, jlong lc, jstring jName) {
	const char* cName = GetStringUTFChars(env, jName);
	linphone_core_set_preferred_video_size_by_name((LinphoneCore *)lc, cName);
	ReleaseStringUTFChars(env, jName, cName);
}

extern "C" jintArray Java_org_linphone_core_LinphoneCoreImpl_getPreferredVideoSize(JNIEnv *env, jobject thiz, jlong lc){
	MSVideoSize vsize = linphone_core_get_preferred_video_size((LinphoneCore *)lc);
	jintArray arr = env->NewIntArray(2);
	int tVsize [2]= {vsize.width,vsize.height};
	env->SetIntArrayRegion(arr, 0, 2, tVsize);
	return arr;
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getDownloadBandwidth(JNIEnv *env, jobject thiz, jlong lc) {
	return (jint) linphone_core_get_download_bandwidth((LinphoneCore *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDownloadBandwidth(JNIEnv *env, jobject thiz, jlong lc, jint bw){
	linphone_core_set_download_bandwidth((LinphoneCore *)lc, (int) bw);
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getUploadBandwidth(JNIEnv *env, jobject thiz, jlong lc) {
	return (jint) linphone_core_get_upload_bandwidth((LinphoneCore *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUploadBandwidth(JNIEnv *env, jobject thiz, jlong lc, jint bw){
	linphone_core_set_upload_bandwidth((LinphoneCore *)lc, (int) bw);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUseSipInfoForDtmfs(JNIEnv *env, jobject thiz, jlong lc, jboolean use){
	linphone_core_set_use_info_for_dtmf((LinphoneCore *)lc, (bool) use);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_getUseSipInfoForDtmfs(JNIEnv *env, jobject thiz, jlong lc){
	return linphone_core_get_use_info_for_dtmf((LinphoneCore *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUseRfc2833ForDtmfs(JNIEnv *env, jobject thiz, jlong lc, jboolean use){
	linphone_core_set_use_rfc2833_for_dtmf((LinphoneCore *)lc, (bool) use);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_getUseRfc2833ForDtmfs(JNIEnv *env, jobject thiz, jlong lc){
	return (jboolean) linphone_core_get_use_rfc2833_for_dtmf((LinphoneCore *)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setDownloadPtime(JNIEnv *env, jobject thiz, jlong lc, jint ptime){
	linphone_core_set_download_ptime((LinphoneCore *)lc, (int) ptime);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUploadPtime(JNIEnv *env, jobject thiz, jlong lc, jint ptime){
	linphone_core_set_upload_ptime((LinphoneCore *)lc, (int) ptime);
}

extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_getState(JNIEnv*  env,jobject thiz,jlong ptr) {
	return (jint) linphone_proxy_config_get_state((const LinphoneProxyConfig *) ptr);
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setExpires(JNIEnv*  env,jobject thiz,jlong ptr,jint delay) {
	linphone_proxy_config_set_expires((LinphoneProxyConfig *) ptr, (int) delay);
}

extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_getExpires(JNIEnv*  env,jobject thiz,jlong ptr) {
	return linphone_proxy_config_get_expires((LinphoneProxyConfig *) ptr);
}

extern "C" void Java_org_linphone_core_LinphoneProxyConfigImpl_setPrivacy(JNIEnv*  env,jobject thiz,jlong ptr,jint privacy) {
	linphone_proxy_config_set_privacy((LinphoneProxyConfig *) ptr, (int) privacy);
}

extern "C" jint Java_org_linphone_core_LinphoneProxyConfigImpl_getPrivacy(JNIEnv*  env,jobject thiz,jlong ptr) {
	return linphone_proxy_config_get_privacy((LinphoneProxyConfig *) ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_enableAvpf(JNIEnv *env, jobject thiz, jlong ptr, jboolean enable) {
	linphone_proxy_config_enable_avpf((LinphoneProxyConfig *)ptr, (bool)enable);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_avpfEnabled(JNIEnv *env, jobject thiz, jlong ptr) {
	return linphone_proxy_config_avpf_enabled((LinphoneProxyConfig *)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_setAvpfRRInterval(JNIEnv *env, jobject thiz, jlong ptr, jint interval) {
	linphone_proxy_config_set_avpf_rr_interval((LinphoneProxyConfig *)ptr, (uint8_t)interval);
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_getAvpfRRInterval(JNIEnv *env, jobject thiz, jlong ptr) {
	return (jint)linphone_proxy_config_get_avpf_rr_interval((LinphoneProxyConfig *)ptr);
}



JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_enableQualityReporting(JNIEnv *env, jobject thiz, jlong ptr, jboolean enable) {
	linphone_proxy_config_enable_quality_reporting((LinphoneProxyConfig *)ptr, (bool)enable);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_quality_reportingEnabled(JNIEnv *env, jobject thiz, jlong ptr) {
	return linphone_proxy_config_quality_reporting_enabled((LinphoneProxyConfig *)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_setQualityReportingInterval(JNIEnv *env, jobject thiz, jlong ptr, jint interval) {
	linphone_proxy_config_set_quality_reporting_interval((LinphoneProxyConfig *)ptr, (uint8_t)interval);
}

JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_getQualityReportingInterval(JNIEnv *env, jobject thiz, jlong ptr) {
	return (jint)linphone_proxy_config_get_quality_reporting_interval((LinphoneProxyConfig *)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_setQualityReportingCollector(JNIEnv *env, jobject thiz, jlong ptr, jstring jcollector) {
	if (jcollector){
		const char *collector = GetStringUTFChars(env, jcollector);
		linphone_proxy_config_set_quality_reporting_collector((LinphoneProxyConfig *)ptr, collector);
		ReleaseStringUTFChars(env, jcollector, collector);
	}
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_getQualityReportingCollector(JNIEnv *env, jobject thiz, jlong ptr) {
	jstring jvalue = env->NewStringUTF(linphone_proxy_config_get_quality_reporting_collector((LinphoneProxyConfig *)ptr));
	return jvalue;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_setRealm(JNIEnv *env, jobject thiz, jlong ptr, jstring jrealm) {
	if (jrealm){
		const char *realm = GetStringUTFChars(env, jrealm);
		linphone_proxy_config_set_realm((LinphoneProxyConfig *)ptr, realm);
		ReleaseStringUTFChars(env, jrealm, realm);
	}
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_getRealm(JNIEnv *env, jobject thiz, jlong ptr) {
	jstring jvalue = env->NewStringUTF(linphone_proxy_config_get_realm((LinphoneProxyConfig *)ptr));
	return jvalue;
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_isPhoneNumber(JNIEnv *env, jobject thiz, jlong ptr, jstring jusername) {
	if(jusername){
		const char *username = GetStringUTFChars(env, jusername);
		bool_t res = linphone_proxy_config_is_phone_number((LinphoneProxyConfig *)ptr, username);
		ReleaseStringUTFChars(env, jusername, username);
		return (jboolean) res;
	} else {
		return JNI_FALSE;
	}
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_setCustomHeader(JNIEnv *env, jobject thiz, jlong prt, jstring jname, jstring jvalue) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = GetStringUTFChars(env, jvalue);
	linphone_proxy_config_set_custom_header((LinphoneProxyConfig*) prt, name, value);
	ReleaseStringUTFChars(env, jname, name);
	ReleaseStringUTFChars(env, jvalue, value);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneProxyConfigImpl_getCustomHeader(JNIEnv *env, jobject thiz, jlong ptr, jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = linphone_proxy_config_get_custom_header((LinphoneProxyConfig *)ptr, name);
	ReleaseStringUTFChars(env, jname, name);
	return value ? env->NewStringUTF(value) : NULL;
}


extern "C" jint Java_org_linphone_core_LinphoneCallImpl_getDuration(JNIEnv*  env,jobject thiz,jlong ptr) {
	return (jint)linphone_call_get_duration((LinphoneCall *) ptr);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setSipDscp(JNIEnv* env,jobject thiz,jlong ptr, jint dscp){
	linphone_core_set_sip_dscp((LinphoneCore*)ptr,dscp);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getSipDscp(JNIEnv* env,jobject thiz,jlong ptr){
	return linphone_core_get_sip_dscp((LinphoneCore*)ptr);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getGlobalState(JNIEnv* env,jobject thiz,jlong ptr){
	return linphone_core_get_global_state((LinphoneCore*)ptr);
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

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isIpv6Enabled(JNIEnv* env,jobject thiz, jlong lc) {
	return (jboolean)linphone_core_ipv6_enabled((LinphoneCore*)lc);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_adjustSoftwareVolume(JNIEnv* env,jobject  thiz
			  ,jlong ptr, jint db) {
	linphone_core_set_playback_gain_db((LinphoneCore *) ptr, db);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_pauseCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	return (jint)linphone_core_pause_call((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_pauseAllCalls(JNIEnv *env,jobject thiz,jlong pCore) {
	return (jint)linphone_core_pause_all_calls((LinphoneCore *) pCore);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_resumeCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall) {
	return (jint)linphone_core_resume_call((LinphoneCore *) pCore, (LinphoneCall *) pCall);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isInConference(JNIEnv *env,jobject thiz,jlong pCore) {
	return (jboolean)linphone_core_is_in_conference((LinphoneCore *) pCore);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_enterConference(JNIEnv *env,jobject thiz,jlong pCore) {
	return (jboolean)(0 == linphone_core_enter_conference((LinphoneCore *) pCore));
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
	return (jint)linphone_core_get_conference_size((LinphoneCore *) pCore);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_createConference(JNIEnv *env, jobject thiz, jlong corePtr, jobject jparams) {
	jclass params_class = env->FindClass("org/linphone/core/LinphoneConferenceParamsImpl");
	jclass conference_class = env->FindClass("org/linphone/core/LinphoneConferenceImpl");
	jfieldID params_native_ptr_attr = env->GetFieldID(params_class, "nativePtr", "J");
	jmethodID conference_constructor = env->GetMethodID(conference_class, "<init>", "(J)V");
	LinphoneConferenceParams *params = NULL;
	LinphoneConference *conference;

	if(jparams) params = (LinphoneConferenceParams *)env->GetLongField(jparams, params_native_ptr_attr);
	conference = linphone_core_create_conference_with_params((LinphoneCore *)corePtr, params);
	if(conference) return env->NewObject(conference_class, conference_constructor, (jlong)conference);
	else return NULL;
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getConference(JNIEnv *env, jobject thiz, jlong pCore) {
	jclass conference_class = env->FindClass("org/linphone/core/LinphoneConferenceImpl");
	jmethodID conference_constructor = env->GetMethodID(conference_class, "<init>", "(J)V");
	LinphoneConference *conf = linphone_core_get_conference((LinphoneCore *)pCore);
	if(conf) return env->NewObject(conference_class, conference_constructor, (jlong)conf);
	else return NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_startConferenceRecording(JNIEnv *env,jobject thiz,jlong pCore, jstring jpath){
	int err=-1;
	if (jpath){
		const char *path = GetStringUTFChars(env, jpath);
		err=linphone_core_start_conference_recording((LinphoneCore*)pCore,path);
		ReleaseStringUTFChars(env, jpath, path);
	}
	return err;
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_stopConferenceRecording(JNIEnv *env,jobject thiz,jlong pCore){
	int err=linphone_core_stop_conference_recording((LinphoneCore*)pCore);
	return err;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_terminateAllCalls(JNIEnv *env,jobject thiz,jlong pCore) {
	linphone_core_terminate_all_calls((LinphoneCore *) pCore);
}
extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_getCall(JNIEnv *env,jobject thiz,jlong pCore,jint position) {
	LinphoneCall* lCall = (LinphoneCall*) bctbx_list_nth_data(linphone_core_get_calls((LinphoneCore *) pCore),position);
	return getCall(env,lCall);
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getCallsNb(JNIEnv *env,jobject thiz,jlong pCore) {
	return (jint)bctbx_list_size(linphone_core_get_calls((LinphoneCore *) pCore));
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_transferCall(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall, jstring jReferTo) {
	const char* cReferTo = GetStringUTFChars(env, jReferTo);
	jint err = linphone_core_transfer_call((LinphoneCore *) pCore, (LinphoneCall *) pCall, cReferTo);
	ReleaseStringUTFChars(env, jReferTo, cReferTo);
	return err;
}
extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_transferCallToAnother(JNIEnv *env,jobject thiz,jlong pCore, jlong pCall, jlong pDestCall) {
	return (jint)linphone_core_transfer_call_to_another((LinphoneCore *) pCore, (LinphoneCall *) pCall, (LinphoneCall *) pDestCall);
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_startReferedCall(JNIEnv *env, jobject thiz, jlong lc, jlong callptr, jlong params){
	return  getCall(env,linphone_core_start_refered_call((LinphoneCore *)lc, (LinphoneCall *)callptr, (const LinphoneCallParams *)params));
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setZrtpSecretsCache(JNIEnv *env,jobject thiz,jlong pCore, jstring jFile) {
	if (jFile) {
		const char* cFile =GetStringUTFChars(env, jFile);
		linphone_core_set_zrtp_secrets_file((LinphoneCore *) pCore,cFile);
		ReleaseStringUTFChars(env, jFile, cFile);
	} else {
		linphone_core_set_zrtp_secrets_file((LinphoneCore *) pCore,NULL);
	}
}

extern "C" jobject Java_org_linphone_core_LinphoneCoreImpl_findCallFromUri(JNIEnv *env,jobject thiz,jlong pCore, jstring jUri) {
	const char* cUri = GetStringUTFChars(env, jUri);
	const LinphoneCall *call=linphone_core_find_call_from_uri((const LinphoneCore *) pCore,cUri);
	ReleaseStringUTFChars(env, jUri, cUri);
	return (jobject) getCall(env,(LinphoneCall*)call);
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
			return (jint)linphone_core_set_video_device(lc, devices[i]);
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

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_listSupportedVideoResolutions(JNIEnv *env, jobject thiz, jlong lc) {
	const MSVideoSizeDef *pdef = linphone_core_get_supported_video_sizes((LinphoneCore *)lc);
	int count = 0;
	int i = 0;
	for (; pdef->name!=NULL; pdef++) {
		i++;
	}
	count = i;

	jobjectArray resolutions = (jobjectArray) env->NewObjectArray(count, env->FindClass("java/lang/String"), env->NewStringUTF(""));
	pdef = linphone_core_get_supported_video_sizes((LinphoneCore *)lc);
	i = 0;
	for (; pdef->name!=NULL; pdef++) {
		env->SetObjectArrayElement(resolutions, i, env->NewStringUTF(pdef->name));
		i++;
	}

	return resolutions;
}

extern "C" jstring Java_org_linphone_core_LinphoneCallImpl_getAuthenticationToken(JNIEnv*  env,jobject thiz,jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	const char* token = linphone_call_get_authentication_token(call);
	if (token == NULL) return NULL;
	return env->NewStringUTF(token);
}
extern "C" jboolean Java_org_linphone_core_LinphoneCallImpl_isAuthenticationTokenVerified(JNIEnv*  env,jobject thiz,jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	return (jboolean)linphone_call_get_authentication_token_verified(call);
}
extern "C" void Java_org_linphone_core_LinphoneCallImpl_setAuthenticationTokenVerified(JNIEnv*  env,jobject thiz,jlong ptr,jboolean verified) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	linphone_call_set_authentication_token_verified(call, verified);
}

extern "C" jobject Java_org_linphone_core_LinphoneCallImpl_getConference(JNIEnv *env, jobject thiz, jlong ptr) {
	jclass conference_class = env->FindClass("org/linphone/core/LinphoneConferenceImpl");
	jmethodID conference_constructor = env->GetMethodID(conference_class, "<init>", "(J)V");
	LinphoneConference *conf = linphone_call_get_conference((LinphoneCall *)ptr);
	if(conf) return env->NewObject(conference_class, conference_constructor, (jlong)conf);
	return NULL;
}

extern "C" jfloat Java_org_linphone_core_LinphoneCallImpl_getPlayVolume(JNIEnv* env, jobject thiz, jlong ptr) {
	LinphoneCall *call = (LinphoneCall *) ptr;
	return (jfloat)linphone_call_get_play_volume(call);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_soundResourcesLocked(JNIEnv* env,jobject thiz,jlong ptr){
	return (jboolean)linphone_core_sound_resources_locked((LinphoneCore *) ptr);
}

// Needed by Galaxy S (can't switch to/from speaker while playing and still keep mic working)
// Implemented directly in msandroid.cpp (sound filters for Android).
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_forceSpeakerState(JNIEnv *env, jobject thiz, jlong ptr, jboolean speakerOn) {
	LinphoneCore *lc = (LinphoneCore *)ptr;
	LinphoneCall *call = linphone_core_get_current_call(lc);
	if (call && call->audiostream && call->audiostream->soundread) {
		bool_t on = speakerOn;
		ms_filter_call_method(call->audiostream->soundread, MS_AUDIO_CAPTURE_FORCE_SPEAKER_STATE, &on);
	}
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
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel;
	if (!tunnel) return;

	const char* cHost = GetStringUTFChars(env, jHost);
	LinphoneTunnelConfig *tunnelconfig = linphone_tunnel_config_new();
	linphone_tunnel_config_set_host(tunnelconfig, cHost);
	linphone_tunnel_config_set_port(tunnelconfig, port);
	linphone_tunnel_config_set_delay(tunnelconfig, delay);
	linphone_tunnel_config_set_remote_udp_mirror_port(tunnelconfig, mirror);
	linphone_tunnel_add_server(tunnel, tunnelconfig);
	ReleaseStringUTFChars(env, jHost, cHost);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelAddServer(JNIEnv *env, jobject thiz, jlong pCore, jlong tunnelconfigptr) {
	LinphoneTunnel *tunnel = linphone_core_get_tunnel((LinphoneCore *)pCore);
	if(tunnel != NULL) {
		LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig*) tunnelconfigptr;
		if (cfg) {
			linphone_tunnel_add_server(tunnel, cfg);
		}else{
			ms_error("Java TunnelConfig object has no associated C object");
		}
	} else {
		ms_error("LinphoneCore.tunnelAddServer(): tunnel feature is not enabled");
	}
}

extern "C" jobjectArray Java_org_linphone_core_LinphoneCoreImpl_tunnelGetServers(JNIEnv *env, jobject thiz, jlong pCore) {
	LinphoneTunnel *tunnel = linphone_core_get_tunnel((LinphoneCore *)pCore);
	jclass tunnelConfigClass = env->FindClass("org/linphone/core/TunnelConfigImpl");
	jobjectArray tunnelConfigArray = NULL;

	if(tunnel != NULL) {
		const bctbx_list_t *servers = linphone_tunnel_get_servers(tunnel);
		const bctbx_list_t *it;
		int i;

		tunnelConfigArray = env->NewObjectArray(bctbx_list_size(servers), tunnelConfigClass, NULL);
		for(it = servers, i=0; it != NULL; it = it->next, i++) {
			LinphoneTunnelConfig *conf =  (LinphoneTunnelConfig *)it->data;
			jobject elt = getTunnelConfig(env, conf);
			env->SetObjectArrayElement(tunnelConfigArray, i, elt);
		}
	}
	env->DeleteLocalRef(tunnelConfigClass);
	return tunnelConfigArray;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelSetHttpProxy(JNIEnv *env,jobject thiz,jlong pCore,
		jstring jHost, jint port, jstring username, jstring password) {

	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel;
	if (!tunnel) return;
	const char* cHost = GetStringUTFChars(env, jHost);
	const char* cUsername = GetStringUTFChars(env, username);
	const char* cPassword = GetStringUTFChars(env, password);
	linphone_tunnel_set_http_proxy(tunnel,cHost, port,cUsername,cPassword);
	ReleaseStringUTFChars(env, jHost, cHost);
	ReleaseStringUTFChars(env, username, cUsername);
	ReleaseStringUTFChars(env, password, cPassword);
}


extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelAutoDetect(JNIEnv *env,jobject thiz,jlong pCore) {
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_auto_detect(tunnel);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelCleanServers(JNIEnv *env,jobject thiz,jlong pCore) {
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_clean_servers(tunnel);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelEnable(JNIEnv *env,jobject thiz,jlong pCore, jboolean enable) {
	LinphoneTunnel *tunnel=((LinphoneCore *) pCore)->tunnel; if (!tunnel) return;
	linphone_tunnel_enable(tunnel, enable);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelSetMode(JNIEnv *env, jobject thiz, jlong pCore, jint mode) {
	LinphoneTunnel *tunnel = ((LinphoneCore *)pCore)->tunnel;
	if(tunnel != NULL) {
		linphone_tunnel_set_mode(tunnel, (LinphoneTunnelMode)mode);
	}
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_tunnelGetMode(JNIEnv *env, jobject thiz, jlong pCore) {
	LinphoneTunnel *tunnel = ((LinphoneCore *)pCore)->tunnel;
	if(tunnel != NULL) {
		return (jint)linphone_tunnel_get_mode(tunnel);
	} else {
		return 0;
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_tunnelEnableSip(JNIEnv *env, jobject thiz, jlong pCore, jboolean enable) {
	LinphoneTunnel *tunnel = ((LinphoneCore *)pCore)->tunnel;
	if(tunnel != NULL) {
		linphone_tunnel_enable_sip(tunnel, (bool_t)enable);
	}
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_tunnelSipEnabled(JNIEnv *env, jobject thiz, jlong pCore) {
	LinphoneTunnel *tunnel = ((LinphoneCore *)pCore)->tunnel;
	if(tunnel != NULL) {
		return (jboolean)linphone_tunnel_sip_enabled(tunnel);
	} else {
		return JNI_FALSE;
	}
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setUserAgent(JNIEnv *env,jobject thiz,jlong pCore, jstring name, jstring version){
	const char* cname = GetStringUTFChars(env, name);
	const char* cversion = GetStringUTFChars(env, version);
	linphone_core_set_user_agent((LinphoneCore *)pCore,cname,cversion);
	ReleaseStringUTFChars(env, name, cname);
	ReleaseStringUTFChars(env, version, cversion);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_isTunnelAvailable(JNIEnv *env,jobject thiz){
	return (jboolean)linphone_core_tunnel_available();
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoPolicy(JNIEnv *env, jobject thiz, jlong lc, jboolean autoInitiate, jboolean autoAccept){
	LinphoneVideoPolicy vpol;
	vpol.automatically_initiate = autoInitiate;
	vpol.automatically_accept = autoAccept;
	linphone_core_set_video_policy((LinphoneCore *)lc, &vpol);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_getVideoAutoInitiatePolicy(JNIEnv *env, jobject thiz, jlong lc){
	const LinphoneVideoPolicy *vpol = linphone_core_get_video_policy((LinphoneCore *)lc);
	return (jboolean) vpol->automatically_initiate;
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_getVideoAutoAcceptPolicy(JNIEnv *env, jobject thiz, jlong lc){
	const LinphoneVideoPolicy *vpol = linphone_core_get_video_policy((LinphoneCore *)lc);
	return (jboolean) vpol->automatically_accept;
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setStaticPicture(JNIEnv *env, jobject thiz, jlong lc, jstring path) {
	const char *cpath = GetStringUTFChars(env, path);
	linphone_core_set_static_picture((LinphoneCore *)lc, cpath);
	ReleaseStringUTFChars(env, path, cpath);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setCpuCountNative(JNIEnv *env, jobject thiz, jlong coreptr, jint count) {
	MSFactory *factory = linphone_core_get_ms_factory((LinphoneCore*)coreptr);
	ms_factory_set_cpu_count(factory, count);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAudioJittcomp(JNIEnv *env, jobject thiz, jlong lc, jint value) {
	linphone_core_set_audio_jittcomp((LinphoneCore *)lc, value);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoJittcomp(JNIEnv *env, jobject thiz, jlong lc, jint value) {
	linphone_core_set_video_jittcomp((LinphoneCore *)lc, value);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAudioPort(JNIEnv *env, jobject thiz, jlong lc, jint port) {
	linphone_core_set_audio_port((LinphoneCore *)lc, port);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoPort(JNIEnv *env, jobject thiz, jlong lc, jint port) {
	linphone_core_set_video_port((LinphoneCore *)lc, port);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAudioPortRange(JNIEnv *env, jobject thiz, jlong lc, jint min_port, jint max_port) {
	linphone_core_set_audio_port_range((LinphoneCore *)lc, min_port, max_port);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoPortRange(JNIEnv *env, jobject thiz, jlong lc, jint min_port, jint max_port) {
	linphone_core_set_video_port_range((LinphoneCore *)lc, min_port, max_port);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAudioDscp(JNIEnv* env,jobject thiz,jlong ptr, jint dscp){
	linphone_core_set_audio_dscp((LinphoneCore*)ptr,dscp);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAndroidPowerManager(JNIEnv *env, jclass cls, jobject pm) {
#ifdef ANDROID
	if(pm != NULL) belle_sip_wake_lock_init(env, pm);
	else belle_sip_wake_lock_uninit(env);
#endif
}

/*released in Java_org_linphone_core_LinphoneCoreImpl_delete*/
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAndroidWifiLock(JNIEnv *env, jobject thiz, jlong ptr, jobject wifi_lock) {
#ifdef ANDROID
	LinphoneCore *lc=(LinphoneCore*)ptr;
	if (lc->wifi_lock) {
		env->DeleteGlobalRef(lc->wifi_lock);
		env->DeleteGlobalRef(lc->wifi_lock_class);
	}
	if (wifi_lock != NULL) {
		lc->wifi_lock=env->NewGlobalRef(wifi_lock);
		lc->wifi_lock_class = env->FindClass("android/net/wifi/WifiManager$WifiLock");
		lc->wifi_lock_class = (jclass)env->NewGlobalRef(lc->wifi_lock_class); /*to make sure methodid are preserved*/
		lc->wifi_lock_acquire_id = env->GetMethodID(lc->wifi_lock_class, "acquire", "()V");
		lc->wifi_lock_release_id = env->GetMethodID(lc->wifi_lock_class, "release", "()V");
	} else {
		lc->wifi_lock=NULL;
		lc->wifi_lock_class=NULL;
	}
#endif
}
/*released in Java_org_linphone_core_LinphoneCoreImpl_delete*/
extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setAndroidMulticastLock(JNIEnv *env, jobject thiz, jlong ptr, jobject multicast_lock) {
#ifdef ANDROID
	LinphoneCore *lc=(LinphoneCore*)ptr;
	if (lc->multicast_lock) {
		env->DeleteGlobalRef(lc->multicast_lock);
		env->DeleteGlobalRef(lc->multicast_lock_class);
	}
	if (multicast_lock != NULL) {
		lc->multicast_lock=env->NewGlobalRef(multicast_lock);
		lc->multicast_lock_class = env->FindClass("android/net/wifi/WifiManager$MulticastLock");
		lc->multicast_lock_class = (jclass)env->NewGlobalRef(lc->multicast_lock_class);/*to make sure methodid are preserved*/
		lc->multicast_lock_acquire_id = env->GetMethodID(lc->multicast_lock_class, "acquire", "()V");
		lc->multicast_lock_release_id = env->GetMethodID(lc->multicast_lock_class, "release", "()V");
	} else {
		lc->multicast_lock=NULL;
		lc->multicast_lock_class=NULL;
	}
#endif
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getAudioDscp(JNIEnv* env,jobject thiz,jlong ptr){
	return linphone_core_get_audio_dscp((LinphoneCore*)ptr);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVideoDscp(JNIEnv* env,jobject thiz,jlong ptr, jint dscp){
	linphone_core_set_video_dscp((LinphoneCore*)ptr,dscp);
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getVideoDscp(JNIEnv* env,jobject thiz,jlong ptr){
	return linphone_core_get_video_dscp((LinphoneCore*)ptr);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setIncomingTimeout(JNIEnv *env, jobject thiz, jlong lc, jint timeout) {
	linphone_core_set_inc_timeout((LinphoneCore *)lc, timeout);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setInCallTimeout(JNIEnv *env, jobject thiz, jlong lc, jint timeout) {
	linphone_core_set_in_call_timeout((LinphoneCore *)lc, timeout);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getVersion(JNIEnv*  env,jobject  thiz,jlong ptr) {
	jstring jvalue =env->NewStringUTF(linphone_core_get_version());
	return jvalue;
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_getConfig(JNIEnv *env, jobject thiz, jlong lc) {
	return (jlong) linphone_core_get_config((LinphoneCore *)lc);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCoreImpl_upnpAvailable(JNIEnv *env, jobject thiz, jlong lc) {
	return (jboolean) linphone_core_upnp_available();
}

extern "C" jint Java_org_linphone_core_LinphoneCoreImpl_getUpnpState(JNIEnv *env, jobject thiz, jlong lc) {
	return (jint) linphone_core_get_upnp_state((LinphoneCore *)lc);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getUpnpExternalIpaddress(JNIEnv *env, jobject thiz, jlong lc) {
	jstring jvalue = env->NewStringUTF(linphone_core_get_upnp_external_ipaddress((LinphoneCore *)lc));
	return jvalue;
}

static LinphoneContent *create_content_from_java_args(JNIEnv *env, LinphoneCore *lc, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding, jstring jname){
	LinphoneContent *content = NULL;
	if (jtype){
		content = linphone_core_create_content(lc);
		void *data = (void*)env->GetByteArrayElements(jdata,NULL);
		const char *tmp;

		linphone_content_set_type(content, tmp = GetStringUTFChars(env, jtype));
		ReleaseStringUTFChars(env, jtype, tmp);

		linphone_content_set_subtype(content, tmp = GetStringUTFChars(env, jsubtype));
		ReleaseStringUTFChars(env, jsubtype, tmp);

		if (jname){
			linphone_content_set_name(content, tmp = GetStringUTFChars(env, jname));
			ReleaseStringUTFChars(env, jname, tmp);
		}

		if (jencoding){
			linphone_content_set_encoding(content, tmp = GetStringUTFChars(env, jencoding));
			ReleaseStringUTFChars(env, jencoding, tmp);
		}

		linphone_content_set_buffer(content, data, env->GetArrayLength(jdata));
		env->ReleaseByteArrayElements(jdata,(jbyte*)data,JNI_ABORT);
	}
	return content;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    subscribe
 * Signature: (JJLjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_subscribe(JNIEnv *env, jobject jcore, jlong coreptr, jlong addrptr,
		jstring jevname, jint expires, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding){
	LinphoneCore *lc=(LinphoneCore*)coreptr;
	LinphoneAddress *addr=(LinphoneAddress*)addrptr;
	LinphoneContent * content = create_content_from_java_args(env, (LinphoneCore*)coreptr, jtype, jsubtype, jdata, jencoding, NULL);
	LinphoneEvent *ev;
	jobject jev=NULL;
	const char *evname = GetStringUTFChars(env, jevname);


	ev=linphone_core_subscribe(lc,addr,evname,expires, content);
	if (content) linphone_content_unref(content);
	ReleaseStringUTFChars(env, jevname, evname);
	if (ev){
		jev=getEvent(env,ev);
	}
	return jev;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    publish
 * Signature: (JJLjava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_publish(JNIEnv *env, jobject jobj, jlong coreptr, jlong addrptr, jstring jevname, jint expires,
																		  jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding){
	LinphoneCore *lc=(LinphoneCore*)coreptr;
	LinphoneAddress *addr=(LinphoneAddress*)addrptr;
	LinphoneContent * content = create_content_from_java_args(env, (LinphoneCore*)coreptr, jtype, jsubtype, jdata, jencoding, NULL);
	LinphoneEvent *ev;
	jobject jev=NULL;
	const char *evname = GetStringUTFChars(env, jevname);

	ev=linphone_core_publish(lc,addr,evname,expires, content);
	if (content) linphone_content_unref(content);
	ReleaseStringUTFChars(env, jevname, evname);
	if (ev){
		jev=getEvent(env,ev);
	}
	return jev;
}

// LpConfig
extern "C" jlong Java_org_linphone_core_LpConfigImpl_newLpConfigImpl(JNIEnv *env, jobject thiz, jstring file) {
	const char *cfile = GetStringUTFChars(env, file);
	LpConfig *lp = lp_config_new(cfile);
	ReleaseStringUTFChars(env, file, cfile);
	return (jlong) lp;
}

extern "C" jlong Java_org_linphone_core_LpConfigImpl_newLpConfigImplFromBuffer(JNIEnv *env, jobject thiz, jstring buffer) {
	const char *cbuffer = GetStringUTFChars(env, buffer);
	LpConfig *lp = lp_config_new_from_buffer(cbuffer);
	ReleaseStringUTFChars(env, buffer, cbuffer);
	return (jlong) lp;
}

extern "C" void Java_org_linphone_core_LpConfigImpl_sync(JNIEnv *env, jobject thiz, jlong lpc) {
	LpConfig *lp = (LpConfig *)lpc;
	lp_config_sync(lp);
}

extern "C" void Java_org_linphone_core_LpConfigImpl_delete(JNIEnv *env, jobject thiz, jlong lpc) {
	LpConfig *lp = (LpConfig *)lpc;
	lp_config_destroy(lp);
}

extern "C" void Java_org_linphone_core_LpConfigImpl_setInt(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jint value) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		lp_config_set_int((LpConfig *)lpc, csection, ckey, (int) value);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
}

extern "C" jint Java_org_linphone_core_LpConfigImpl_getInt(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jint defaultValue) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		int returnValue = lp_config_get_int((LpConfig *)lpc, csection, ckey, (int) defaultValue);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		return (jint) returnValue;
}

extern "C" void Java_org_linphone_core_LpConfigImpl_setFloat(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jfloat value) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		lp_config_set_float((LpConfig *)lpc, csection, ckey, (float) value);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
}

extern "C" jfloat Java_org_linphone_core_LpConfigImpl_getFloat(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jfloat defaultValue) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		float returnValue = lp_config_get_float((LpConfig *)lpc, csection, ckey, (float) defaultValue);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		return (jfloat) returnValue;
}

extern "C" void Java_org_linphone_core_LpConfigImpl_setBool(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jboolean value) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		lp_config_set_int((LpConfig *)lpc, csection, ckey, value ? 1 : 0);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
}

extern "C" jboolean Java_org_linphone_core_LpConfigImpl_getBool(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jboolean defaultValue) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		int returnValue = lp_config_get_int((LpConfig *)lpc, csection, ckey, defaultValue ? 1 : 0);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		return (jboolean) returnValue == 1;
}

extern "C" void Java_org_linphone_core_LpConfigImpl_setString(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jstring value) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		const char *cvalue = GetStringUTFChars(env, value);
		lp_config_set_string((LpConfig *)lpc, csection, ckey, cvalue);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		ReleaseStringUTFChars(env, value, cvalue);
}

extern "C" jstring Java_org_linphone_core_LpConfigImpl_getString(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jstring defaultValue) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		const char *cvalue = GetStringUTFChars(env, defaultValue);

		const char *returnValue = lp_config_get_string((LpConfig *)lpc, csection, ckey, cvalue);

		jstring jreturnValue = NULL;
		if (returnValue)
			jreturnValue = env->NewStringUTF(returnValue);

		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		ReleaseStringUTFChars(env, defaultValue, cvalue);

		return jreturnValue;
}
extern "C" void Java_org_linphone_core_LpConfigImpl_setIntRange(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jint min, jint max) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		lp_config_set_range((LpConfig *)lpc, csection, ckey, min, max);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
}

extern "C" jintArray Java_org_linphone_core_LpConfigImpl_getIntRange(JNIEnv *env, jobject thiz, jlong lpc,
		jstring section, jstring key, jint defaultmin, jint defaultmax) {
		const char *csection = GetStringUTFChars(env, section);
		const char *ckey = GetStringUTFChars(env, key);
		int *values = (int*)calloc(2, sizeof(int));
		lp_config_get_range((LpConfig *)lpc, csection, ckey, &values[0], &values[1], defaultmin, defaultmax);
		jintArray returnValues = env->NewIntArray(2);
		env->SetIntArrayRegion(returnValues, 0, 2, values);
		ms_free(values);
		ReleaseStringUTFChars(env, section, csection);
		ReleaseStringUTFChars(env, key, ckey);
		return returnValues;
}

static jobject create_java_linphone_content(JNIEnv *env, const LinphoneContent *icontent){
	jclass contentClass;
	jmethodID ctor;
	jstring jtype, jsubtype, jencoding, jname;
	jbyteArray jdata = NULL;
	jint jsize = 0;
	const char *tmp;
	void *data;

	contentClass = (jclass)env->FindClass("org/linphone/core/LinphoneContentImpl");
	ctor = env->GetMethodID(contentClass,"<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[BLjava/lang/String;I)V");

	jtype = env->NewStringUTF(linphone_content_get_type(icontent));
	jsubtype = env->NewStringUTF(linphone_content_get_subtype(icontent));
	jencoding = ((tmp = linphone_content_get_encoding(icontent))) ? env->NewStringUTF(tmp) : NULL;
	jname = ((tmp = linphone_content_get_name(icontent))) ? env->NewStringUTF(tmp) : NULL;
	jsize = (jint) linphone_content_get_size(icontent);

	data = (!linphone_content_is_multipart(icontent) ? linphone_content_get_buffer(icontent) : NULL);
	if (data){
		jdata = env->NewByteArray(linphone_content_get_size(icontent));
		env->SetByteArrayRegion(jdata, 0, linphone_content_get_size(icontent), (jbyte*)data);
	}

	jobject jobj = env->NewObject(contentClass, ctor, jname, jtype, jsubtype, jdata, jencoding, jsize);

	env->DeleteLocalRef(contentClass);
	env->DeleteLocalRef(jtype);
	env->DeleteLocalRef(jsubtype);
	if (jencoding) {
		env->DeleteLocalRef(jencoding);
	}
	if (jname) {
		env->DeleteLocalRef(jname);
	}

	return jobj;
}

static jobject create_java_linphone_buffer(JNIEnv *env, const LinphoneBuffer *buffer) {
	jclass bufferClass;
	jmethodID ctor;
	jbyteArray jdata = NULL;
	jint jsize = 0;

	bufferClass = (jclass)env->FindClass("org/linphone/core/LinphoneBufferImpl");
	ctor = env->GetMethodID(bufferClass,"<init>", "([BI)V");
	jsize = buffer ? (jint) buffer->size : 0;

	if (buffer && buffer->content) {
		jdata = env->NewByteArray(buffer->size);
		env->SetByteArrayRegion(jdata, 0, buffer->size, (jbyte*)buffer->content);
	}

	jobject jobj = env->NewObject(bufferClass, ctor, jdata, jsize);
	env->DeleteLocalRef(bufferClass);
	return jobj;
}

static LinphoneBuffer* create_c_linphone_buffer_from_java_linphone_buffer(JNIEnv *env, jobject jbuffer) {
	jclass bufferClass;
	jmethodID getSizeMethod, getDataMethod;
	LinphoneBuffer *buffer = NULL;
	jint jsize;
	jobject jdata;
	jbyteArray jcontent;
	uint8_t *content;

	bufferClass = (jclass)env->FindClass("org/linphone/core/LinphoneBufferImpl");
	getSizeMethod = env->GetMethodID(bufferClass, "getSize", "()I");
	getDataMethod = env->GetMethodID(bufferClass, "getContent", "()[B");

	jsize = env->CallIntMethod(jbuffer, getSizeMethod);
	jdata = env->CallObjectMethod(jbuffer, getDataMethod);
	jcontent = reinterpret_cast<jbyteArray>(jdata);
	if (jcontent != NULL) {
		content = (uint8_t*)env->GetByteArrayElements(jcontent, NULL);
		buffer = linphone_buffer_new_from_data(content, (size_t)jsize);
		env->ReleaseByteArrayElements(jcontent, (jbyte*)content, JNI_ABORT);
	}
	env->DeleteLocalRef(bufferClass);

	return buffer;
}

/*
 * Class:     org_linphone_core_LinphoneInfoMessageImpl
 * Method:    getContent
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneInfoMessageImpl_getContent(JNIEnv *env, jobject jobj, jlong infoptr){
	const LinphoneContent *content=linphone_info_message_get_content((LinphoneInfoMessage*)infoptr);
	if (content){
		return create_java_linphone_content(env,content);
	}
	return NULL;
}

/*
 * Class:     org_linphone_core_LinphoneInfoMessageImpl
 * Method:    setContent
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneInfoMessageImpl_setContent(JNIEnv *env, jobject jobj, jlong infoptr, jstring jtype, jstring jsubtype, jstring jdata){
	LinphoneInfoMessage *infomsg = (LinphoneInfoMessage*) infoptr;
	LinphoneContent * content = linphone_content_new();
	const char *tmp;

	linphone_content_set_type(content, tmp = GetStringUTFChars(env, jtype));
	ReleaseStringUTFChars(env, jtype, tmp);

	linphone_content_set_type(content, tmp = GetStringUTFChars(env, jsubtype));
	ReleaseStringUTFChars(env, jsubtype, tmp);

	linphone_content_set_string_buffer(content, tmp = GetStringUTFChars(env, jdata));
	ReleaseStringUTFChars(env, jdata, tmp);

	linphone_info_message_set_content(infomsg, content);
	linphone_content_unref(content);
}

/*
 * Class:     org_linphone_core_LinphoneInfoMessageImpl
 * Method:    addHeader
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneInfoMessageImpl_addHeader(JNIEnv *env, jobject jobj, jlong infoptr, jstring jname, jstring jvalue){
	const char *name = GetStringUTFChars(env, jname);
	const char *value = GetStringUTFChars(env, jvalue);
	linphone_info_message_add_header((LinphoneInfoMessage*)infoptr,name,value);
	ReleaseStringUTFChars(env, jname, name);
	ReleaseStringUTFChars(env, jvalue, value);
}

/*
 * Class:     org_linphone_core_LinphoneInfoMessageImpl
 * Method:    getHeader
 * Signature: (JLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneInfoMessageImpl_getHeader(JNIEnv *env, jobject jobj, jlong infoptr, jstring jname){
	const char *name = GetStringUTFChars(env, jname);
	const char *ret=linphone_info_message_get_header((LinphoneInfoMessage*)infoptr,name);
	ReleaseStringUTFChars(env, jname, name);
	return ret ? env->NewStringUTF(ret) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneInfoMessageImpl
 * Method:    delete
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneInfoMessageImpl_delete(JNIEnv *env, jobject jobj , jlong infoptr){
	linphone_info_message_destroy((LinphoneInfoMessage*)infoptr);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneEventImpl_getCore(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneCore *lc=linphone_event_get_core((LinphoneEvent*)evptr);
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	jobject core = ljb->getCore();
	return core;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    getEventName
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneEventImpl_getEventName(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	const char *evname=linphone_event_get_name(ev);
	return evname ? env->NewStringUTF(evname) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    acceptSubscription
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_acceptSubscription(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return linphone_event_accept_subscription(ev);
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    denySubscription
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_denySubscription(JNIEnv *env, jobject jobj, jlong evptr, int reason){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return linphone_event_deny_subscription(ev,(LinphoneReason)reason);
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    notify
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_notify(JNIEnv *env, jobject jobj, jlong evptr, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding){
	LinphoneContent * content = create_content_from_java_args(env, linphone_event_get_core((LinphoneEvent *)evptr),
						jtype, jsubtype, jdata, jencoding, NULL);
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	jint err;

	err=linphone_event_notify(ev, content);

	if (content){
		linphone_content_unref(content);
	}
	return err;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    updateSubscribe
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_updateSubscribe(JNIEnv *env, jobject jobj, jlong evptr, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding){
	LinphoneContent * content = create_content_from_java_args(env, linphone_event_get_core((LinphoneEvent *)evptr),
						jtype, jsubtype, jdata, jencoding, NULL);
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	jint err;

	err=linphone_event_update_subscribe(ev, content);

	if (content) linphone_content_unref(content);
	return err;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    updatePublish
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_updatePublish(JNIEnv *env, jobject jobj, jlong evptr, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding){
	LinphoneContent * content = create_content_from_java_args(env, linphone_event_get_core((LinphoneEvent *)evptr),
						jtype, jsubtype, jdata, jencoding, NULL);
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	jint err;

	err=linphone_event_update_publish(ev, content);

	if (content) linphone_content_unref(content);
	return err;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    terminate
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_terminate(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	linphone_event_terminate(ev);
	return 0;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    getReason
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_getReason(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return linphone_event_get_reason(ev);
}

JNIEXPORT jlong JNICALL Java_org_linphone_core_LinphoneEventImpl_getErrorInfo(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return (jlong)linphone_event_get_error_info(ev);
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    getSubscriptionDir
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_getSubscriptionDir(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return linphone_event_get_subscription_dir(ev);
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    getSubscriptionState
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneEventImpl_getSubscriptionState(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	return linphone_event_get_subscription_state(ev);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_createSubscribe(JNIEnv *env, jobject thiz, jlong jcore, jlong jaddr, jstring jeventname, jint expires) {
	LinphoneCore *lc = (LinphoneCore*) jcore;
	LinphoneAddress *addr = (LinphoneAddress*) jaddr;
	LinphoneEvent *event;
	jobject jevent = NULL;
	const char *event_name = GetStringUTFChars(env, jeventname);

	event = linphone_core_create_subscribe(lc, addr, event_name, expires);
	ReleaseStringUTFChars(env, jeventname, event_name);
	if (event) {
		jevent = getEvent(env, event);
	}
	return jevent;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneEventImpl_sendSubscribe(JNIEnv *env, jobject thiz, jlong eventptr, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding) {
	LinphoneContent *content = create_content_from_java_args(env, linphone_event_get_core((LinphoneEvent*)eventptr),
							jtype, jsubtype, jdata, jencoding, NULL);

	linphone_event_send_subscribe((LinphoneEvent*) eventptr, content);
	if (content) linphone_content_unref(content);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneCoreImpl_createPublish(JNIEnv *env, jobject thiz, jlong jcore, jlong jaddr, jstring jeventname, jint expires) {
	LinphoneCore *lc = (LinphoneCore*) jcore;
	LinphoneAddress *addr = (LinphoneAddress*) jaddr;
	LinphoneEvent *event;
	jobject jevent = NULL;
	const char *event_name = GetStringUTFChars(env, jeventname);

	event = linphone_core_create_publish(lc, addr, event_name, expires);
	ReleaseStringUTFChars(env, jeventname, event_name);
	if (event) {
		jevent = getEvent(env, event);
	}
	return jevent;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneEventImpl_sendPublish(JNIEnv *env, jobject thiz, jlong eventptr, jstring jtype, jstring jsubtype, jbyteArray jdata, jstring jencoding) {
	LinphoneContent *content = create_content_from_java_args(env, linphone_event_get_core((LinphoneEvent*)eventptr),
							jtype, jsubtype, jdata, jencoding, NULL);
	linphone_event_send_publish((LinphoneEvent*) eventptr, content);
	if (content) linphone_content_unref(content);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneEventImpl_addCustomHeader(JNIEnv *env, jobject thiz, jlong jevent, jstring jname, jstring jvalue) {
	const char *name = GetStringUTFChars(env, jname);
	const char *value = GetStringUTFChars(env, jvalue);
	linphone_event_add_custom_header((LinphoneEvent*) jevent, name, value);
	ReleaseStringUTFChars(env, jname, name);
	ReleaseStringUTFChars(env, jvalue, value);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneEventImpl_getCustomHeader(JNIEnv *env, jobject thiz, jlong jevent, jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	const char *header = linphone_event_get_custom_header((LinphoneEvent*) jevent, name);
	jstring jheader = header ? env->NewStringUTF(header) : NULL;
	ReleaseStringUTFChars(env, jname, name);
	return jheader;
}

/*
 * Class:     org_linphone_core_LinphoneEventImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneEventImpl_unref(JNIEnv *env, jobject jobj, jlong evptr){
	LinphoneEvent *ev=(LinphoneEvent*)evptr;
	linphone_event_unref(ev);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    newPresenceModelImpl
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_newPresenceModelImpl__(JNIEnv *env, jobject jobj) {
	LinphonePresenceModel *model = linphone_presence_model_new();
	model = linphone_presence_model_ref(model);
	return (jlong)model;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    newPresenceModelImpl
 * Signature: (ILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_newPresenceModelImpl__ILjava_lang_String_2(JNIEnv *env, jobject jobj, jint type, jstring description) {
	LinphonePresenceModel *model;
	const char *cdescription = GetStringUTFChars(env, description);
	model = linphone_presence_model_new_with_activity((LinphonePresenceActivityType)type, cdescription);
	model = linphone_presence_model_ref(model);
	ReleaseStringUTFChars(env, description, cdescription);
	return (jlong)model;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    newPresenceModelImpl
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_newPresenceModelImpl__ILjava_lang_String_2Ljava_lang_String_2Ljava_lang_String_2(
		JNIEnv *env, jobject jobj, jint type, jstring description, jstring note, jstring lang) {
	LinphonePresenceModel *model;
	const char *cdescription = GetStringUTFChars(env, description);
	const char *cnote =GetStringUTFChars(env, note);
	const char *clang = GetStringUTFChars(env, lang);
	model = linphone_presence_model_new_with_activity_and_note((LinphonePresenceActivityType)type, cdescription, cnote, clang);
	model = linphone_presence_model_ref(model);
	ReleaseStringUTFChars(env, description, cdescription);
	ReleaseStringUTFChars(env, note, cnote);
	ReleaseStringUTFChars(env, lang, clang);
	return (jlong)model;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresenceModelImpl_unref(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	linphone_presence_model_unref(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getBasicStatus
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_getBasicStatus(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_get_basic_status(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    setBasicStatus
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_setBasicStatus(JNIEnv *env, jobject jobj, jlong ptr, jint basic_status) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_set_basic_status(model, (LinphonePresenceBasicStatus)basic_status);
}


/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getTimestamp
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_getTimestamp(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jlong)linphone_presence_model_get_timestamp(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getContact
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceModelImpl_getContact(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	char *ccontact = linphone_presence_model_get_contact(model);
	jstring jcontact = ccontact ? env->NewStringUTF(ccontact) : NULL;
	if (ccontact) ms_free(ccontact);
	return jcontact;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    setContact
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresenceModelImpl_setContact(JNIEnv *env, jobject jobj, jlong ptr, jstring contact) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	const char *ccontact = GetStringUTFChars(env, contact);
	linphone_presence_model_set_contact(model, ccontact);
	ReleaseStringUTFChars(env, contact, ccontact);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getActivity
 * Signature: (J)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceModelImpl_getActivity(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	LinphonePresenceActivity *activity = linphone_presence_model_get_activity(model);
	if (activity == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceActivityImpl", linphone_presence_activity, activity)
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    setActivity
 * Signature: (JILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_setActivity(JNIEnv *env, jobject jobj, jlong ptr, jint acttype, jstring description) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	const char *cdescription = GetStringUTFChars(env, description);
	jint res = (jint)linphone_presence_model_set_activity(model, (LinphonePresenceActivityType)acttype, cdescription);
	ReleaseStringUTFChars(env, description, cdescription);
	return res;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNbActivities
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_getNbActivities(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jlong)linphone_presence_model_get_nb_activities(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNthActivity
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceModelImpl_getNthActivity(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	LinphonePresenceActivity *activity = linphone_presence_model_get_nth_activity(model, (unsigned int)idx);
	if (activity == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceActivityImpl", linphone_presence_activity, activity)
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    addActivity
 * Signature: (JILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_addActivity(JNIEnv *env, jobject jobj, jlong ptr, jlong activityPtr) {
	return (jint)linphone_presence_model_add_activity((LinphonePresenceModel *)ptr, (LinphonePresenceActivity *)activityPtr);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    clearActivities
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_clearActivities(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_clear_activities(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNote
 * Signature: (JLjava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceModelImpl_getNote(JNIEnv *env , jobject jobj, jlong ptr, jstring lang) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	const char *clang = GetStringUTFChars(env, lang);
	LinphonePresenceNote *note = linphone_presence_model_get_note(model, clang);
	ReleaseStringUTFChars(env, lang, clang);
	if (note == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceNoteImpl", linphone_presence_note, note)
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    addNote
 * Signature: (JLjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_addNote(JNIEnv *env, jobject jobj, jlong ptr, jstring description, jstring lang) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	const char *cdescription = GetStringUTFChars(env, description);
	const char *clang = GetStringUTFChars(env, lang);
	jint res = (jint)linphone_presence_model_add_note(model, cdescription, clang);
	ReleaseStringUTFChars(env, description, cdescription);
	ReleaseStringUTFChars(env, lang, clang);
	return res;
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    clearNotes
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_clearNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_clear_notes(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNbServices
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_getNbServices(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jlong)linphone_presence_model_get_nb_services(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNthService
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceModelImpl_getNthService(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	LinphonePresenceService *service = linphone_presence_model_get_nth_service(model, (unsigned int)idx);
	if (service == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceServiceImpl", linphone_presence_service, service)
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    addService
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_addService(JNIEnv *env, jobject jobj, jlong ptr, jlong servicePtr) {
	return (jint)linphone_presence_model_add_service((LinphonePresenceModel *)ptr, (LinphonePresenceService *)servicePtr);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    clearServices
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_clearServices(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_clear_services(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNbPersons
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceModelImpl_getNbPersons(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jlong)linphone_presence_model_get_nb_persons(model);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    getNthPerson
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceModelImpl_getNthPerson(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	LinphonePresencePerson *person = linphone_presence_model_get_nth_person(model, (unsigned int)idx);
	if (person == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresencePersonImpl", linphone_presence_person, person)
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    addPerson
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_addPerson(JNIEnv *env, jobject jobj, jlong ptr, jlong personPtr) {
	return (jint)linphone_presence_model_add_person((LinphonePresenceModel *)ptr, (LinphonePresencePerson *)personPtr);
}

/*
 * Class:     org_linphone_core_PresenceModelImpl
 * Method:    clearPersons
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceModelImpl_clearPersons(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceModel *model = (LinphonePresenceModel *)ptr;
	return (jint)linphone_presence_model_clear_persons(model);
}

/*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    newPresenceActivityImpl
 * Signature: (ILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceActivityImpl_newPresenceActivityImpl(JNIEnv *env, jobject jobj, jint type, jstring description) {
	LinphonePresenceActivity *activity;
	const char *cdescription = GetStringUTFChars(env, description);
	activity = linphone_presence_activity_new((LinphonePresenceActivityType)type, cdescription);
	activity = linphone_presence_activity_ref(activity);
	ReleaseStringUTFChars(env, description, cdescription);
	return (jlong)activity;
}

  /*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresenceActivityImpl_unref(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	linphone_presence_activity_unref(activity);
}

  /*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    toString
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceActivityImpl_toString(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	char *cactstr = linphone_presence_activity_to_string(activity);
	jstring jactstr = cactstr ? env->NewStringUTF(cactstr) : NULL;
	if (cactstr) ms_free(cactstr);
	return jactstr;
}

/*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    getType
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceActivityImpl_getType(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	return (jint)linphone_presence_activity_get_type(activity);
}

/*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    setType
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceActivityImpl_setType(JNIEnv *env, jobject jobj, jlong ptr, jint type) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	return (jint)linphone_presence_activity_set_type(activity, (LinphonePresenceActivityType)type);
}

/*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    getDescription
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceActivityImpl_getDescription(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	const char *cdescription = linphone_presence_activity_get_description(activity);
	return cdescription ? env->NewStringUTF(cdescription) : NULL;
}

/*
 * Class:     org_linphone_core_PresenceActivityImpl
 * Method:    setDescription
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceActivityImpl_setDescription(JNIEnv *env, jobject jobj, jlong ptr, jstring description) {
	LinphonePresenceActivity *activity = (LinphonePresenceActivity *)ptr;
	const char *cdescription = GetStringUTFChars(env, description);
	linphone_presence_activity_set_description(activity, cdescription);
	ReleaseStringUTFChars(env, description, cdescription);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    newPresenceServiceImpl
 * Signature: (Ljava/lang/String;ILjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceServiceImpl_newPresenceServiceImpl(JNIEnv *env, jobject jobj, jstring id, jint basic_status, jstring contact) {
	LinphonePresenceService *service;
	const char *cid = GetStringUTFChars(env, id);
	const char *ccontact = GetStringUTFChars(env, contact);
	service = linphone_presence_service_new(cid, (LinphonePresenceBasicStatus)basic_status, ccontact);
	service = linphone_presence_service_ref(service);
	ReleaseStringUTFChars(env, id, cid);
	ReleaseStringUTFChars(env, contact, ccontact);
	return (jlong)service;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresenceServiceImpl_unref(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	linphone_presence_service_unref(service);
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    getId
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceServiceImpl_getId(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	char *cid = linphone_presence_service_get_id(service);
	jstring jid = cid ? env->NewStringUTF(cid) : NULL;
	if (cid) ms_free(cid);
	return jid;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    setId
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_setId(JNIEnv *env, jobject jobj, jlong ptr, jstring id) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	const char *cid = GetStringUTFChars(env, id);
	linphone_presence_service_set_id(service, cid);
	ReleaseStringUTFChars(env, id, cid);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    getBasicStatus
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_getBasicStatus(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	return (jint)linphone_presence_service_get_basic_status(service);
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    setBasicStatus
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_setBasicStatus(JNIEnv *env, jobject jobj, jlong ptr, jint basic_status) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	return (jint)linphone_presence_service_set_basic_status(service, (LinphonePresenceBasicStatus)basic_status);
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    getContact
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceServiceImpl_getContact(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	char *ccontact = linphone_presence_service_get_contact(service);
	jstring jcontact = ccontact ? env->NewStringUTF(ccontact) : NULL;
	if (ccontact) ms_free(ccontact);
	return jcontact;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    setContact
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_setContact(JNIEnv *env, jobject jobj, jlong ptr, jstring contact) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	const char *ccontact = GetStringUTFChars(env, contact);
	linphone_presence_service_set_contact(service, ccontact);
	ReleaseStringUTFChars(env, contact, ccontact);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    getNbNotes
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceServiceImpl_getNbNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	return (jlong)linphone_presence_service_get_nb_notes(service);
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    getNthNote
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresenceServiceImpl_getNthNote(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	LinphonePresenceNote *note = linphone_presence_service_get_nth_note(service, (unsigned int)idx);
	if (note == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceNoteImpl", linphone_presence_note, note)
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    addNote
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_addNote(JNIEnv *env, jobject jobj, jlong ptr, jlong notePtr) {
	return (jint)linphone_presence_service_add_note((LinphonePresenceService *)ptr, (LinphonePresenceNote *)notePtr);
}

/*
 * Class:     org_linphone_core_PresenceServiceImpl
 * Method:    clearNotes
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceServiceImpl_clearNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceService *service = (LinphonePresenceService *)ptr;
	return (jint)linphone_presence_service_clear_notes(service);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    newPresencePersonImpl
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresencePersonImpl_newPresencePersonImpl(JNIEnv *env, jobject jobj, jstring id) {
	LinphonePresencePerson *person;
	const char *cid = GetStringUTFChars(env, id);
	person = linphone_presence_person_new(cid);
	person = linphone_presence_person_ref(person);
	ReleaseStringUTFChars(env, id, cid);
	return (jlong)person;
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresencePersonImpl_unref(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	linphone_presence_person_unref(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getId
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresencePersonImpl_getId(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	char *cid = linphone_presence_person_get_id(person);
	jstring jid = cid ? env->NewStringUTF(cid) : NULL;
	if (cid) ms_free(cid);
	return jid;
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    setId
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_setId(JNIEnv *env, jobject jobj, jlong ptr, jstring id) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	const char *cid = GetStringUTFChars(env, id);
	linphone_presence_person_set_id(person, cid);
	ReleaseStringUTFChars(env, id, cid);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNbActivities
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresencePersonImpl_getNbActivities(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jlong)linphone_presence_person_get_nb_activities(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNthActivity
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresencePersonImpl_getNthActivity(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	LinphonePresenceActivity *activity = linphone_presence_person_get_nth_activity(person, (unsigned int)idx);
	if (activity == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceActivityImpl", linphone_presence_activity, activity)
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    addActivity
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_addActivity(JNIEnv *env, jobject jobj, jlong ptr, jlong activityPtr) {
	return (jint)linphone_presence_person_add_activity((LinphonePresencePerson *)ptr, (LinphonePresenceActivity *)activityPtr);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    clearActivities
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_clearActivities(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jint)linphone_presence_person_clear_activities(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNbNotes
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresencePersonImpl_getNbNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jlong)linphone_presence_person_get_nb_notes(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNthNote
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresencePersonImpl_getNthNote(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	LinphonePresenceNote *note = linphone_presence_person_get_nth_note(person, (unsigned int)idx);
	if (note == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceNoteImpl", linphone_presence_note, note)
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    addNote
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_addNote(JNIEnv *env, jobject jobj, jlong ptr, jlong notePtr) {
	return (jint)linphone_presence_person_add_note((LinphonePresencePerson *)ptr, (LinphonePresenceNote *)notePtr);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    clearNotes
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_clearNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jint)linphone_presence_person_clear_notes(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNbActivitiesNotes
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresencePersonImpl_getNbActivitiesNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jlong)linphone_presence_person_get_nb_activities_notes(person);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    getNthActivitiesNote
 * Signature: (JJ)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_linphone_core_PresencePersonImpl_getNthActivitiesNote(JNIEnv *env, jobject jobj, jlong ptr, jlong idx) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	LinphonePresenceNote *note = linphone_presence_person_get_nth_activities_note(person, (unsigned int)idx);
	if (note == NULL) return NULL;
	RETURN_USER_DATA_OBJECT("PresenceNoteImpl", linphone_presence_note, note)
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    addActivitiesNote
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_addActivitiesNote(JNIEnv *env, jobject jobj, jlong ptr, jlong notePtr) {
	return (jint)linphone_presence_person_add_activities_note((LinphonePresencePerson *)ptr, (LinphonePresenceNote *)notePtr);
}

/*
 * Class:     org_linphone_core_PresencePersonImpl
 * Method:    clearActivitesNotes
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresencePersonImpl_clearActivitesNotes(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresencePerson *person = (LinphonePresencePerson *)ptr;
	return (jint)linphone_presence_person_clear_activities_notes(person);
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    newPresenceNoteImpl
 * Signature: (Ljava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_linphone_core_PresenceNoteImpl_newPresenceNoteImpl(JNIEnv *env, jobject jobj, jstring content, jstring lang) {
	LinphonePresenceNote *note;
	const char *ccontent = GetStringUTFChars(env, content);
	const char *clang = GetStringUTFChars(env, lang);
	note = linphone_presence_note_new(ccontent, clang);
	note = linphone_presence_note_ref(note);
	ReleaseStringUTFChars(env, lang, clang);
	ReleaseStringUTFChars(env, content, ccontent);
	return (jlong)note;
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    unref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PresenceNoteImpl_unref(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceNote *note = (LinphonePresenceNote *)ptr;
	linphone_presence_note_unref(note);
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    getContent
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceNoteImpl_getContent(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceNote *note = (LinphonePresenceNote *)ptr;
	const char *ccontent = linphone_presence_note_get_content(note);
	return ccontent ? env->NewStringUTF(ccontent) : NULL;
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    setContent
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceNoteImpl_setContent(JNIEnv *env, jobject jobj, jlong ptr, jstring content) {
	LinphonePresenceNote *note = (LinphonePresenceNote *)ptr;
	const char *ccontent = GetStringUTFChars(env, content);
	linphone_presence_note_set_content(note, ccontent);
	ReleaseStringUTFChars(env, content, ccontent);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    getLang
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PresenceNoteImpl_getLang(JNIEnv *env, jobject jobj, jlong ptr) {
	LinphonePresenceNote *note = (LinphonePresenceNote *)ptr;
	const char *clang = linphone_presence_note_get_lang(note);
	return clang ? env->NewStringUTF(clang) : NULL;
}

/*
 * Class:     org_linphone_core_PresenceNoteImpl
 * Method:    setLang
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_PresenceNoteImpl_setLang(JNIEnv *env, jobject jobj, jlong ptr, jstring lang) {
	LinphonePresenceNote *note = (LinphonePresenceNote *)ptr;
	const char *clang = GetStringUTFChars(env, lang);
	linphone_presence_note_set_lang(note, clang);
	ReleaseStringUTFChars(env, lang, clang);
	return (jint)0;
}

/*
 * Class:     org_linphone_core_PayloadTypeImpl
 * Method:    setRecvFmtp
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PayloadTypeImpl_setRecvFmtp(JNIEnv *env, jobject jobj, jlong ptr, jstring jfmtp){
	PayloadType *pt=(PayloadType *)ptr;
	const char *fmtp = GetStringUTFChars(env, jfmtp);
	payload_type_set_recv_fmtp(pt,fmtp);
	ReleaseStringUTFChars(env, jfmtp, fmtp);
}

/*
 * Class:     org_linphone_core_PayloadTypeImpl
 * Method:    getRecvFmtp
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PayloadTypeImpl_getRecvFmtp(JNIEnv *env, jobject jobj, jlong ptr){
	PayloadType *pt=(PayloadType *)ptr;
	const char *fmtp=pt->recv_fmtp;
	return fmtp ? env->NewStringUTF(fmtp) : NULL;
}

/*
 * Class:     org_linphone_core_PayloadTypeImpl
 * Method:    setSendFmtp
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_PayloadTypeImpl_setSendFmtp(JNIEnv *env, jobject jobj, jlong ptr , jstring jfmtp){
	PayloadType *pt=(PayloadType *)ptr;
	const char *fmtp = GetStringUTFChars(env, jfmtp);
	payload_type_set_send_fmtp(pt,fmtp);
	ReleaseStringUTFChars(env, jfmtp, fmtp);
}

/*
 * Class:     org_linphone_core_PayloadTypeImpl
 * Method:    getSendFmtp
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_PayloadTypeImpl_getSendFmtp(JNIEnv *env, jobject jobj, jlong ptr){
	PayloadType *pt=(PayloadType *)ptr;
	const char *fmtp=pt->send_fmtp;
	return fmtp ? env->NewStringUTF(fmtp) : NULL;
}


JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_enableSdp200Ack(JNIEnv*  env
																			,jobject  thiz
																			,jlong lc
																			,jboolean enable) {
	linphone_core_enable_sdp_200_ack((LinphoneCore*)lc,enable);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneCoreImpl_isSdp200AckEnabled(JNIEnv*  env
																					,jobject  thiz
																					,jlong lc) {
	return (jboolean)linphone_core_sdp_200_ack_enabled((const LinphoneCore*)lc);
}

/* Header for class org_linphone_core_ErrorInfoImpl */
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_linphone_core_ErrorInfoImpl
 * Method:    getReason
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_ErrorInfoImpl_getReason(JNIEnv *env, jobject jobj, jlong ei){
	return linphone_error_info_get_reason((const LinphoneErrorInfo*)ei);
}

/*
 * Class:     org_linphone_core_ErrorInfoImpl
 * Method:    getProtocolCode
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_ErrorInfoImpl_getProtocolCode(JNIEnv *env, jobject jobj, jlong ei){
	return linphone_error_info_get_protocol_code((const LinphoneErrorInfo*)ei);
}

/*
 * Class:     org_linphone_core_ErrorInfoImpl
 * Method:    getPhrase
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_ErrorInfoImpl_getPhrase(JNIEnv *env, jobject jobj, jlong ei){
	const char *tmp=linphone_error_info_get_phrase((const LinphoneErrorInfo*)ei);
	return tmp ? env->NewStringUTF(tmp) : NULL;
}

/*
 * Class:     org_linphone_core_ErrorInfoImpl
 * Method:    getDetails
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_ErrorInfoImpl_getDetails(JNIEnv *env, jobject jobj, jlong ei){
	const char *tmp=linphone_error_info_get_details((const LinphoneErrorInfo*)ei);
	return tmp ? env->NewStringUTF(tmp) : NULL;
}

#ifdef __cplusplus
}
#endif

/* Linphone Player */
struct LinphonePlayerData {
	LinphonePlayerData(JNIEnv *env, jobject listener, jobject jLinphonePlayer) :
		mListener(env->NewGlobalRef(listener)),
		mJLinphonePlayer(env->NewGlobalRef(jLinphonePlayer))
	{
		mListenerClass = (jclass)env->NewGlobalRef(env->GetObjectClass(listener));
		mEndOfFileMethodID = env->GetMethodID(mListenerClass, "endOfFile", "(Lorg/linphone/core/LinphonePlayer;)V");
		if(mEndOfFileMethodID == NULL) {
			ms_error("Could not get endOfFile method ID");
			env->ExceptionClear();
		}
	}

	~LinphonePlayerData() {
		JNIEnv *env;
		jvm->AttachCurrentThread(&env, NULL);
		env->DeleteGlobalRef(mListener);
		env->DeleteGlobalRef(mListenerClass);
		env->DeleteGlobalRef(mJLinphonePlayer);
	}

	jobject mListener;
	jclass mListenerClass;
	jobject mJLinphonePlayer;
	jmethodID mEndOfFileMethodID;
};

static void _eof_callback(LinphonePlayer *player, void *user_data) {
	JNIEnv *env;
	LinphonePlayerData *player_data = (LinphonePlayerData *)user_data;
	jvm->AttachCurrentThread(&env, NULL);
	env->CallVoidMethod(player_data->mListener, player_data->mEndOfFileMethodID, player_data->mJLinphonePlayer);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_open(JNIEnv *env, jobject jPlayer, jlong ptr, jstring filename, jobject listener) {
	LinphonePlayerData *data = NULL;
	LinphonePlayerEofCallback cb = NULL;
	const char *cfilename = GetStringUTFChars(env, filename);
	if(listener) {
		data = new LinphonePlayerData(env, listener, jPlayer);
		cb = _eof_callback;
	}
	if(linphone_player_open((LinphonePlayer *)ptr, cfilename, cb, data) == -1) {
		if(data) delete data;
		ReleaseStringUTFChars(env, filename, cfilename);
		return -1;
	}
	ReleaseStringUTFChars(env, filename, cfilename);
	return 0;
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_start(JNIEnv *env, jobject jobj, jlong ptr) {
	return (jint)linphone_player_start((LinphonePlayer *)ptr);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_pause(JNIEnv *env, jobject jobj, jlong ptr) {
	return (jint)linphone_player_pause((LinphonePlayer *)ptr);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_seek(JNIEnv *env, jobject jobj, jlong ptr, jint timeMs) {
	return (jint)linphone_player_seek((LinphonePlayer *)ptr, timeMs);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_getState(JNIEnv *env, jobject jobj, jlong ptr) {
	return (jint)linphone_player_get_state((LinphonePlayer *)ptr);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_getDuration(JNIEnv *env, jobject jobj, jlong ptr) {
	return (jint)linphone_player_get_duration((LinphonePlayer *)ptr);
}

extern "C" jint Java_org_linphone_core_LinphonePlayerImpl_getCurrentPosition(JNIEnv *env, jobject jobj, jlong ptr) {
	return (jint)linphone_player_get_current_position((LinphonePlayer *)ptr);
}

extern "C" void Java_org_linphone_core_LinphonePlayerImpl_close(JNIEnv *env, jobject playerPtr, jlong ptr) {
	LinphonePlayer *player = (LinphonePlayer *)ptr;
	if(player->user_data) {
		LinphonePlayerData *data = (LinphonePlayerData *)player->user_data;
		if(data) delete data;
		player->user_data = NULL;
	}
	linphone_player_close(player);
}

extern "C" void Java_org_linphone_core_LinphonePlayerImpl_destroy(JNIEnv *env, jobject jobj, jlong playerPtr) {
	LinphonePlayer *player = (LinphonePlayer *)playerPtr;
	if(player == NULL) {
		ms_error("Cannot destroy the LinphonePlayerImpl object. Native pointer is NULL");
		return;
	}
	if(player->user_data) {
		delete (LinphonePlayerData *)player->user_data;
	}
	jobject window_id = (jobject)ms_media_player_get_window_id((MSMediaPlayer *)player->impl);
	if(window_id) env->DeleteGlobalRef(window_id);
	linphone_player_destroy(player);
}

extern "C" jlong Java_org_linphone_core_LinphoneCoreImpl_createLocalPlayer(JNIEnv *env, jobject jobj, jlong ptr, jobject window) {
	jobject window_ref = NULL;
	window_ref = env->NewGlobalRef(window);
	LinphonePlayer *player = linphone_core_create_local_player((LinphoneCore *)ptr, NULL, "MSAndroidDisplay", (void *)window_ref);
	if(player == NULL) {
		ms_error("Fails to create a player");
		if(window_ref) env->DeleteGlobalRef(window_ref);
		return 0;
	} else {
		return (jlong)player;
	}
}


/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setAudioMulticastAddr
 * Signature: (JLjava/lang/String;)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_setAudioMulticastAddr
  (JNIEnv * env , jobject, jlong ptr, jstring value) {
	const char *char_value = GetStringUTFChars(env, value);
	LinphoneCore *lc=(LinphoneCore*)ptr;
	int result = linphone_core_set_audio_multicast_addr(lc,char_value);
	ReleaseStringUTFChars(env, value, char_value);
	return result;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setVideoMulticastAddr
 * Signature: (JLjava/lang/String;)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_setVideoMulticastAddr
  (JNIEnv * env, jobject, jlong ptr, jstring value) {
	const char *char_value = GetStringUTFChars(env, value);
	LinphoneCore *lc=(LinphoneCore*)ptr;
	int result = linphone_core_set_video_multicast_addr(lc,char_value);
	ReleaseStringUTFChars(env, value, char_value);
	return result;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getAudioMulticastAddr
 * Signature: (J)Ljava/lang/String;
 */
extern "C" jstring JNICALL Java_org_linphone_core_LinphoneCoreImpl_getAudioMulticastAddr
  (JNIEnv *env , jobject, jlong ptr) {
	const char *tmp=linphone_core_get_audio_multicast_addr((LinphoneCore*)ptr);
	return tmp ? env->NewStringUTF(tmp) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getVideoMulticastAddr
 * Signature: (J)Ljava/lang/String;
 */
extern "C" jstring JNICALL Java_org_linphone_core_LinphoneCoreImpl_getVideoMulticastAddr
  (JNIEnv * env, jobject, jlong ptr) {
	const char *tmp=linphone_core_get_video_multicast_addr((LinphoneCore*)ptr);
	return tmp ? env->NewStringUTF(tmp) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setAudioMulticastTtl
 * Signature: (JI)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_setAudioMulticastTtl
  (JNIEnv *, jobject, jlong ptr, jint value) {
	return linphone_core_set_audio_multicast_ttl((LinphoneCore*)ptr,value);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setVideoMulticastTtl
 * Signature: (JI)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_setVideoMulticastTtl
  (JNIEnv *, jobject, jlong ptr, jint value) {
	return linphone_core_set_video_multicast_ttl((LinphoneCore*)ptr,value);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getAudioMulticastTtl
 * Signature: (J)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getAudioMulticastTtl
  (JNIEnv *, jobject, jlong ptr) {
	return linphone_core_get_audio_multicast_ttl((LinphoneCore*)ptr);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getVideoMulticastTtl
 * Signature: (J)I
 */
extern "C" jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getVideoMulticastTtl
  (JNIEnv *, jobject, jlong ptr) {
	return linphone_core_get_video_multicast_ttl((LinphoneCore*)ptr);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    enableAudioMulticast
 * Signature: (JZ)V
 */
extern "C" void JNICALL Java_org_linphone_core_LinphoneCoreImpl_enableAudioMulticast
  (JNIEnv *, jobject, jlong ptr, jboolean yesno) {
	return linphone_core_enable_audio_multicast((LinphoneCore*)ptr,yesno);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    audioMulticastEnabled
 * Signature: (J)Z
 */
extern "C" jboolean JNICALL Java_org_linphone_core_LinphoneCoreImpl_audioMulticastEnabled
  (JNIEnv *, jobject, jlong ptr) {
	return linphone_core_audio_multicast_enabled((LinphoneCore*)ptr);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    enableVideoMulticast
 * Signature: (JZ)V
 */
extern "C" void JNICALL Java_org_linphone_core_LinphoneCoreImpl_enableVideoMulticast
  (JNIEnv *, jobject, jlong ptr, jboolean yesno) {
	return linphone_core_enable_video_multicast((LinphoneCore*)ptr,yesno);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    videoMulticastEnabled
 * Signature: (J)Z
 */
extern "C" jboolean JNICALL Java_org_linphone_core_LinphoneCoreImpl_videoMulticastEnabled
  (JNIEnv *, jobject, jlong ptr) {
	return linphone_core_video_multicast_enabled((LinphoneCore*)ptr);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setDnsServers(JNIEnv *env, jobject thiz, jlong lc, jobjectArray servers){
	bctbx_list_t *l = NULL;

	if (servers != NULL){
		int count = env->GetArrayLength(servers);

		for (int i=0; i < count; i++) {
			jstring server = (jstring) env->GetObjectArrayElement(servers, i);
			const char *str = GetStringUTFChars(env, server);
			if (str){
				l = bctbx_list_append(l, ms_strdup(str));
				ReleaseStringUTFChars(env, server, str);
			}
		}
	}
	linphone_core_set_dns_servers((LinphoneCore*)lc, l);
	bctbx_list_free_with_data(l, ms_free);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_enableDnsSrv(JNIEnv *env, jobject thiz, jlong lc, jboolean yesno) {
	linphone_core_enable_dns_srv((LinphoneCore *)lc, yesno);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneCoreImpl_dnsSrvEnabled(JNIEnv *env, jobject thiz, jlong lc) {
	return linphone_core_dns_srv_enabled((LinphoneCore *)lc);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setVideoPreset(JNIEnv *env, jobject thiz, jlong lc, jstring preset) {
	const char *char_preset = GetStringUTFChars(env, preset);
	linphone_core_set_video_preset((LinphoneCore *)lc, char_preset);
	ReleaseStringUTFChars(env, preset, char_preset);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneCoreImpl_getVideoPreset(JNIEnv *env, jobject thiz, jlong lc) {
	const char *tmp = linphone_core_get_video_preset((LinphoneCore *)lc);
	return tmp ? env->NewStringUTF(tmp) : NULL;
}

extern "C" jobject Java_org_linphone_core_LinphoneCallImpl_getChatRoom(JNIEnv* env ,jobject thiz, jlong ptr) {
	return getChatRoom(env, linphone_call_get_chat_room((LinphoneCall *) ptr));
}

extern "C" void Java_org_linphone_core_LinphoneCallParamsImpl_enableRealTimeText(JNIEnv* env ,jobject thiz, jlong ptr, jboolean yesno) {
	linphone_call_params_enable_realtime_text((LinphoneCallParams *)ptr, yesno);
}

extern "C" jboolean Java_org_linphone_core_LinphoneCallParamsImpl_realTimeTextEnabled(JNIEnv* env ,jobject thiz, jlong ptr) {
	return linphone_call_params_realtime_text_enabled((LinphoneCallParams *)ptr);
}

extern "C" void Java_org_linphone_core_LinphoneChatMessageImpl_putChar(JNIEnv* env ,jobject thiz, jlong ptr, jlong character) {
	linphone_chat_message_put_char((LinphoneChatMessage *)ptr, character);
}

extern "C" void  Java_org_linphone_core_LinphoneChatRoomImpl_finalize(JNIEnv* env, jobject thiz, jlong ptr) {
	LinphoneChatRoom *room = (LinphoneChatRoom *)ptr;
	linphone_chat_room_set_user_data(room, NULL);
	linphone_chat_room_unref(room);
}

extern "C" jobject Java_org_linphone_core_LinphoneChatRoomImpl_getCall(JNIEnv* env ,jobject thiz, jlong ptr) {
	return getCall(env, linphone_chat_room_get_call((LinphoneChatRoom *)ptr));
}

extern "C" jlong Java_org_linphone_core_LinphoneChatRoomImpl_getChar(JNIEnv* env ,jobject thiz, jlong ptr) {
	return linphone_chat_room_get_char((LinphoneChatRoom *)ptr);
}

static void _next_video_frame_decoded_callback(LinphoneCall *call, void *user_data) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);

	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	jobject listener = (jobject) user_data;
	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onNextVideoFrameDecoded","(Lorg/linphone/core/LinphoneCall;)V");
	env->DeleteLocalRef(clazz);

	jobject jcall = getCall(env, call);
	env->CallVoidMethod(listener, method, jcall);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCallImpl_setListener(JNIEnv* env, jobject thiz, jlong ptr, jobject jlistener) {
	jobject listener = env->NewGlobalRef(jlistener);
	LinphoneCall *call = (LinphoneCall *)ptr;
	linphone_call_set_next_video_frame_decoded_callback(call, _next_video_frame_decoded_callback, listener);
}


/*
 * returns the java TunnelConfig associated with a C LinphoneTunnelConfig.
**/
static jobject getTunnelConfig(JNIEnv *env, LinphoneTunnelConfig *cfg){
	jobject jobj=0;

	if (cfg != NULL){
		jclass tunnelConfigClass = env->FindClass("org/linphone/core/TunnelConfigImpl");
		jmethodID ctor = env->GetMethodID(tunnelConfigClass,"<init>", "(J)V");

		void *up=linphone_tunnel_config_get_user_data(cfg);

		if (up==NULL){
			jobj=env->NewObject(tunnelConfigClass,ctor,(jlong)cfg);
			linphone_tunnel_config_set_user_data(cfg,(void*)env->NewWeakGlobalRef(jobj));
			linphone_tunnel_config_ref(cfg);
		}else{
			//promote the weak ref to local ref
			jobj=env->NewLocalRef((jobject)up);
			if (jobj == NULL){
				//the weak ref was dead
				jobj=env->NewObject(tunnelConfigClass,ctor,(jlong)cfg);
				linphone_tunnel_config_set_user_data(cfg,(void*)env->NewWeakGlobalRef(jobj));
			}
		}
		env->DeleteLocalRef(tunnelConfigClass);
	}
	return jobj;
}


/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    getHost
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_TunnelConfigImpl_getHost(JNIEnv *env, jobject obj, jlong ptr){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	const char *host = linphone_tunnel_config_get_host(cfg);
	if (host){
		return env->NewStringUTF(host);
	}
	return NULL;
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    setHost
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_TunnelConfigImpl_setHost(JNIEnv *env, jobject obj, jlong ptr, jstring jstr){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	const char* host = GetStringUTFChars(env, jstr);
	linphone_tunnel_config_set_host(cfg, host);
	ReleaseStringUTFChars(env, jstr, host);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    getPort
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_TunnelConfigImpl_getPort(JNIEnv *env, jobject jobj, jlong ptr){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	return linphone_tunnel_config_get_port(cfg);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    setPort
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_TunnelConfigImpl_setPort(JNIEnv *env, jobject jobj, jlong ptr, jint port){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	linphone_tunnel_config_set_port(cfg, port);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    getRemoteUdpMirrorPort
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_TunnelConfigImpl_getRemoteUdpMirrorPort(JNIEnv *env, jobject jobj, jlong ptr){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	return linphone_tunnel_config_get_remote_udp_mirror_port(cfg);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    setRemoteUdpMirrorPort
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_TunnelConfigImpl_setRemoteUdpMirrorPort(JNIEnv *env, jobject jobj, jlong ptr, jint port){
	 LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	 linphone_tunnel_config_set_remote_udp_mirror_port(cfg, port);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    getDelay
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_TunnelConfigImpl_getDelay(JNIEnv *env, jobject jobj, jlong ptr){
	 LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	 return linphone_tunnel_config_get_delay(cfg);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    setDelay
 * Signature: (JI)I
 */
JNIEXPORT void JNICALL Java_org_linphone_core_TunnelConfigImpl_setDelay(JNIEnv *env, jobject jobj, jlong ptr, jint delay){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	linphone_tunnel_config_set_delay(cfg, delay);
}

/*
 * Class:     org_linphone_core_TunnelConfigImpl
 * Method:    destroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_TunnelConfigImpl_destroy(JNIEnv *env, jobject jobj, jlong ptr){
	LinphoneTunnelConfig *cfg = (LinphoneTunnelConfig *)ptr;
	linphone_tunnel_config_set_user_data(cfg, NULL);
	linphone_tunnel_config_unref(cfg);
}


/*
 * Class:     org_linphone_core_LinphoneCallLogImpl
 * Method:    getCallId
 * Signature: (J)I
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneCallLogImpl_getCallId(JNIEnv *env, jobject jobj, jlong pcl){
	const char *str = linphone_call_log_get_call_id((LinphoneCallLog*)pcl);
	return str ? env->NewStringUTF(str) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setHttpProxyHost
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setHttpProxyHost(JNIEnv *env, jobject jobj, jlong core, jstring jhost){
	const char *host = GetStringUTFChars(env, jhost);
	linphone_core_set_http_proxy_host((LinphoneCore*)core, host);
	ReleaseStringUTFChars(env, jhost, host);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setHttpProxyPort
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setHttpProxyPort(JNIEnv *env, jobject jobj, jlong core, jint port){
	linphone_core_set_http_proxy_port((LinphoneCore*)core, port);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getHttpProxyHost
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneCoreImpl_getHttpProxyHost(JNIEnv *env , jobject jobj, jlong core){
	const char * host = linphone_core_get_http_proxy_host((LinphoneCore *)core);
	return host ? env->NewStringUTF(host) : NULL;
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getHttpProxyPort
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getHttpProxyPort(JNIEnv *env, jobject jobj, jlong core){
	return linphone_core_get_http_proxy_port((LinphoneCore *)core);
}


/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setSipTransportTimeout
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setSipTransportTimeout(JNIEnv *env, jobject jobj, jlong pcore, jint timeout){
	linphone_core_set_sip_transport_timeout((LinphoneCore*)pcore, timeout);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getSipTransportTimeout
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getSipTransportTimeout(JNIEnv *env, jobject jobj, jlong pcore){
	return linphone_core_get_sip_transport_timeout((LinphoneCore*)pcore);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setNortpTimeout
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setNortpTimeout(JNIEnv *env, jobject obj, jlong core, jint timeout){
	linphone_core_set_nortp_timeout((LinphoneCore*)core, timeout);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    getNortpTimeout
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_org_linphone_core_LinphoneCoreImpl_getNortpTimeout(JNIEnv *env, jobject obj, jlong core){
	return linphone_core_get_nortp_timeout((LinphoneCore*)core);
}



extern "C" jlong Java_org_linphone_core_LinphoneConferenceParamsImpl_createInstance(JNIEnv *env, jobject thiz, jobject jcore) {
	jclass core_class = env->FindClass("org/linphone/core/LinphoneCoreImpl");
	jfieldID native_ptr_attr = env->GetFieldID(core_class, "nativePtr", "J");
	LinphoneCore *core = NULL;
	if(jcore) core = (LinphoneCore *)env->GetLongField(jcore, native_ptr_attr);
	return (jlong)linphone_conference_params_new(core);
}

extern "C" jlong Java_org_linphone_core_LinphoneConferenceParamsImpl_copyInstance(JNIEnv *env, jobject thiz, jlong paramsPtr) {
	return (jlong)linphone_conference_params_clone((LinphoneConferenceParams *)paramsPtr);
}

extern "C" void Java_org_linphone_core_LinphoneConferenceParamsImpl_destroyInstance(JNIEnv *env, jobject thiz, jlong paramsPtr) {
	linphone_conference_params_free((LinphoneConferenceParams *)paramsPtr);
}

extern "C" void Java_org_linphone_core_LinphoneConferenceParamsImpl_enableVideo(JNIEnv *env, jobject thiz, jlong paramsPtr, jboolean enable) {
	linphone_conference_params_enable_video((LinphoneConferenceParams *)paramsPtr, enable);
}

extern "C" jboolean Java_org_linphone_core_LinphoneConferenceParamsImpl_isVideoRequested(JNIEnv *env, jobject thiz, jlong paramsPtr) {
	return linphone_conference_params_video_requested((LinphoneConferenceParams *)paramsPtr);
}



extern "C" jobjectArray Java_org_linphone_core_LinphoneConferenceImpl_getParticipants(JNIEnv *env, jobject thiz, jlong pconference) {
	bctbx_list_t *participants, *it;
	jclass addr_class = env->FindClass("org/linphone/core/LinphoneAddressImpl");
	jmethodID addr_constructor = env->GetMethodID(addr_class, "<init>", "(J)V");
	jobjectArray jaddr_list;
	int i;

	participants = linphone_conference_get_participants((LinphoneConference *)pconference);
	jaddr_list = env->NewObjectArray(bctbx_list_size(participants), addr_class, NULL);
	for(it=participants, i=0; it; it=bctbx_list_next(it), i++) {
		LinphoneAddress *addr = (LinphoneAddress *)it->data;
		jobject jaddr = env->NewObject(addr_class, addr_constructor, (jlong)addr);
		env->SetObjectArrayElement(jaddr_list, i, jaddr);
	}
	bctbx_list_free(participants);
	return jaddr_list;
}

extern "C" jint Java_org_linphone_core_LinphoneConferenceImpl_removeParticipant(JNIEnv *env, jobject thiz, jlong pconference, jobject uri) {
	jfieldID native_ptr_attr = env->GetFieldID(env->GetObjectClass(uri), "nativePtr", "J");
	LinphoneAddress *addr = (LinphoneAddress *)env->GetLongField(uri, native_ptr_attr);
	return linphone_conference_remove_participant((LinphoneConference *)pconference, addr);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setSipNetworkReachable
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setSipNetworkReachable(JNIEnv *env, jobject jobj, jlong pcore, jboolean reachable){
	linphone_core_set_sip_network_reachable((LinphoneCore*)pcore, (bool_t) reachable);
}

/*
 * Class:     org_linphone_core_LinphoneCoreImpl
 * Method:    setMediaNetworkReachable
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setMediaNetworkReachable(JNIEnv *env, jobject jobj, jlong pcore, jboolean reachable){
	linphone_core_set_media_network_reachable((LinphoneCore*)pcore, (bool_t) reachable);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_setUserCertificatesPath(JNIEnv *env, jobject jobj, jlong pcore, jstring jpath){
	const char *path = GetStringUTFChars(env, jpath);
	linphone_core_set_user_certificates_path((LinphoneCore*)pcore, path);
	ReleaseStringUTFChars(env, jpath, path);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_reloadMsPlugins(JNIEnv *env, jobject jobj, jlong pcore, jstring jpath) {
	const char *path = GetStringUTFChars(env, jpath);
	linphone_core_reload_ms_plugins((LinphoneCore*)pcore, path);
	ReleaseStringUTFChars(env, jpath, path);
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneCoreImpl_reloadSoundDevices(JNIEnv *env, jobject jobj, jlong pcore) {
	linphone_core_reload_sound_devices((LinphoneCore*)pcore);
}

JNIEXPORT jobject JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_getCore(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneCore *lc = ((LinphoneNatPolicy *)jNatPolicy)->lc;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);
	return ljb->getCore();
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_clear(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	linphone_nat_policy_clear(nat_policy);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_stunEnabled(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	return (linphone_nat_policy_stun_enabled(nat_policy) == FALSE) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_enableStun(JNIEnv *env, jobject thiz, jlong jNatPolicy, jboolean jEnable) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	linphone_nat_policy_enable_stun(nat_policy, (jEnable == JNI_FALSE) ? FALSE : TRUE);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_turnEnabled(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	return (linphone_nat_policy_turn_enabled(nat_policy) == FALSE) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_enableTurn(JNIEnv *env, jobject thiz, jlong jNatPolicy, jboolean jEnable) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	linphone_nat_policy_enable_turn(nat_policy, (jEnable == JNI_FALSE) ? FALSE : TRUE);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_iceEnabled(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	return (linphone_nat_policy_ice_enabled(nat_policy) == FALSE) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_enableIce(JNIEnv *env, jobject thiz, jlong jNatPolicy, jboolean jEnable) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	linphone_nat_policy_enable_ice(nat_policy, (jEnable == JNI_FALSE) ? FALSE : TRUE);
}

JNIEXPORT jboolean JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_upnpEnabled(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	return (linphone_nat_policy_upnp_enabled(nat_policy) == FALSE) ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_enableUpnp(JNIEnv *env, jobject thiz, jlong jNatPolicy, jboolean jEnable) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	linphone_nat_policy_enable_upnp(nat_policy, (jEnable == JNI_FALSE) ? FALSE : TRUE);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_getStunServer(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	const char *stun_server = linphone_nat_policy_get_stun_server(nat_policy);
	return (stun_server != NULL) ? env->NewStringUTF(stun_server) : NULL;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_setStunServer(JNIEnv *env, jobject thiz, jlong jNatPolicy, jstring jStunServer) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	const char *stun_server = GetStringUTFChars(env, jStunServer);
	linphone_nat_policy_set_stun_server(nat_policy, stun_server);
	ReleaseStringUTFChars(env, jStunServer, stun_server);
}

JNIEXPORT jstring JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_getStunServerUsername(JNIEnv *env, jobject thiz, jlong jNatPolicy) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	const char *stun_server = linphone_nat_policy_get_stun_server_username(nat_policy);
	return (stun_server != NULL) ? env->NewStringUTF(stun_server) : NULL;
}

JNIEXPORT void JNICALL Java_org_linphone_core_LinphoneNatPolicyImpl_setStunServerUsername(JNIEnv *env, jobject thiz, jlong jNatPolicy, jstring jStunServerUsername) {
	LinphoneNatPolicy *nat_policy = (LinphoneNatPolicy *)jNatPolicy;
	const char *stun_server_username = GetStringUTFChars(env, jStunServerUsername);
	linphone_nat_policy_set_stun_server_username(nat_policy, stun_server_username);
	ReleaseStringUTFChars(env, jStunServerUsername, stun_server_username);
}

// XML RPC wrapper

static void xml_request_response(LinphoneXmlRpcRequest *request) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneXmlRpcRequestCbs *cbs = linphone_xml_rpc_request_get_callbacks(request);
	jobject listener = (jobject) linphone_xml_rpc_request_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("xml_request_response() notification without listener");
		return ;
	}

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onXmlRpcRequestResponse","(Lorg/linphone/core/LinphoneXmlRpcRequest;)V");
	env->DeleteLocalRef(clazz);

	env->CallVoidMethod(listener, method, getXmlRpcRequest(env, request));
}

extern "C" jlong Java_org_linphone_core_LinphoneXmlRpcRequestImpl_newLinphoneXmlRpcRequest(JNIEnv *env, jobject thiz, jstring jmethodname, jint returntype) {
	const char *methodname = GetStringUTFChars(env, jmethodname);
	LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new(methodname, (LinphoneXmlRpcArgType)returntype);
	ReleaseStringUTFChars(env, jmethodname, methodname);
	return (jlong) request;
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcRequestImpl_unref(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	linphone_xml_rpc_request_set_user_data(request, NULL);
	linphone_xml_rpc_request_unref(request);
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcRequestImpl_addIntArg(JNIEnv *env, jobject thiz, jlong ptr, jint arg) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	linphone_xml_rpc_request_add_int_arg(request, (int)arg);
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcRequestImpl_addStringArg(JNIEnv *env, jobject thiz, jlong ptr, jstring jarg) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	const char *arg = GetStringUTFChars(env, jarg);
	linphone_xml_rpc_request_add_string_arg(request, arg);
	ReleaseStringUTFChars(env, jarg, arg);
}

extern "C" jstring Java_org_linphone_core_LinphoneXmlRpcRequestImpl_getContent(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	const char *content = linphone_xml_rpc_request_get_content(request);
	return content ? env->NewStringUTF(content) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneXmlRpcRequestImpl_getStatus(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	return (jint) linphone_xml_rpc_request_get_status(request);
}

extern "C" jint Java_org_linphone_core_LinphoneXmlRpcRequestImpl_getIntResponse(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	return (jint) linphone_xml_rpc_request_get_int_response(request);
}

extern "C" jstring Java_org_linphone_core_LinphoneXmlRpcRequestImpl_getStringResponse(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	const char *response = linphone_xml_rpc_request_get_string_response(request);
	return response ? env->NewStringUTF(response) : NULL;
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcRequestImpl_setListener(JNIEnv* env, jobject thiz, jlong ptr, jobject jlistener) {
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)ptr;
	jobject listener = env->NewGlobalRef(jlistener);
	LinphoneXmlRpcRequestCbs *cbs;

	cbs = linphone_xml_rpc_request_get_callbacks(request);
	linphone_xml_rpc_request_cbs_set_user_data(cbs, listener);
	linphone_xml_rpc_request_cbs_set_response(cbs, xml_request_response);
}


extern "C" jlong Java_org_linphone_core_LinphoneXmlRpcSessionImpl_newLinphoneXmlRpcSession(JNIEnv *env, jobject thiz, jlong ptr, jstring jurl) {
	LinphoneCore *lc = (LinphoneCore *)ptr;
	const char *url = GetStringUTFChars(env, jurl);
	LinphoneXmlRpcSession *session = linphone_xml_rpc_session_new(lc, url);
	ReleaseStringUTFChars(env, jurl, url);
	return (jlong) session;
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcSessionImpl_unref(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession *)ptr;
	linphone_xml_rpc_session_set_user_data(session, NULL);
	linphone_xml_rpc_session_unref(session);
}

extern "C" void Java_org_linphone_core_LinphoneXmlRpcSessionImpl_sendRequest(JNIEnv *env, jobject thiz, jlong ptr, jlong requestPtr) {
	LinphoneXmlRpcSession *session = (LinphoneXmlRpcSession *)ptr;
	LinphoneXmlRpcRequest *request = (LinphoneXmlRpcRequest *)requestPtr;
	linphone_xml_rpc_session_send_request(session, request);
}

// Account creator

static void account_creator_is_account_used(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}
	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorIsAccountUsed","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_create_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorAccountCreated","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_activate_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorAccountActivated","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_link_phone_number_with_account(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorAccountLinkedWithPhoneNumber","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_activate_phone_number_link(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorPhoneNumberLinkActivated","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_is_account_linked(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorIsAccountLinked","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_is_phone_number_used(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorIsPhoneNumberUsed","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_is_account_activated(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorIsAccountActivated","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_phone_account_recovered(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorPhoneAccountRecovered","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

static void account_creator_password_updated(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	JNIEnv *env = 0;
	jint result = jvm->AttachCurrentThread(&env,NULL);
	if (result != 0) {
		ms_error("cannot attach VM\n");
		return;
	}

	ms_warning("test callback password updated");

	LinphoneAccountCreatorCbs *cbs = linphone_account_creator_get_callbacks(creator);
	jobject listener = (jobject) linphone_account_creator_cbs_get_user_data(cbs);
	if (listener == NULL) {
		ms_error("account_creator_response() notification without listener");
		return ;
	}

	LinphoneCore *lc = (LinphoneCore *)creator->core;
	LinphoneJavaBindings *ljb = (LinphoneJavaBindings *)linphone_core_get_user_data(lc);

	jclass clazz = (jclass) env->GetObjectClass(listener);
	jmethodID method = env->GetMethodID(clazz, "onAccountCreatorPasswordUpdated","(Lorg/linphone/core/LinphoneAccountCreator;Lorg/linphone/core/LinphoneAccountCreator$Status;)V");
	env->DeleteLocalRef(clazz);

	jobject statusObject = env->CallStaticObjectMethod(ljb->accountCreatorStatusClass, ljb->accountCreatorStatusFromIntId, (jint)status);
	env->CallVoidMethod(listener, method, getAccountCreator(env, creator), statusObject);
}

extern "C" jlong Java_org_linphone_core_LinphoneAccountCreatorImpl_newLinphoneAccountCreator(JNIEnv *env, jobject thiz, jlong core, jstring jurl) {
	const char *url = GetStringUTFChars(env, jurl);
	LinphoneAccountCreator *account_creator = linphone_account_creator_new((LinphoneCore *)core, url);
	ReleaseStringUTFChars(env, jurl, url);
	return (jlong) account_creator;
}

extern "C" void Java_org_linphone_core_LinphoneAccountCreatorImpl_unref(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	linphone_account_creator_set_user_data(account_creator, NULL);
	linphone_account_creator_unref(account_creator);
}

extern "C" void Java_org_linphone_core_LinphoneAccountCreatorImpl_setListener(JNIEnv* env, jobject thiz, jlong ptr, jobject jlistener) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	jobject listener = env->NewGlobalRef(jlistener);
	LinphoneAccountCreatorCbs *cbs;

	cbs = linphone_account_creator_get_callbacks(account_creator);
	linphone_account_creator_cbs_set_user_data(cbs, listener);
	linphone_account_creator_cbs_set_is_account_used(cbs, account_creator_is_account_used);
	linphone_account_creator_cbs_set_create_account(cbs, account_creator_create_account);
	linphone_account_creator_cbs_set_activate_account(cbs, account_creator_activate_account);
	linphone_account_creator_cbs_set_link_phone_number_with_account(cbs, account_creator_link_phone_number_with_account);
	linphone_account_creator_cbs_set_activate_phone_number_link(cbs, account_creator_activate_phone_number_link);
	linphone_account_creator_cbs_set_is_account_activated(cbs, account_creator_is_account_activated);
	linphone_account_creator_cbs_set_recover_phone_account(cbs, account_creator_phone_account_recovered);
	linphone_account_creator_cbs_set_is_phone_number_used(cbs, account_creator_is_phone_number_used);
	linphone_account_creator_cbs_set_is_account_linked(cbs, account_creator_is_account_linked);
	linphone_account_creator_cbs_set_update_hash(cbs, account_creator_password_updated);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setUsername(JNIEnv *env, jobject thiz, jlong ptr, jstring jusername) {
	const char *username = GetStringUTFChars(env, jusername);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_username(account_creator, username);
	ReleaseStringUTFChars(env, jusername, username);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getUsername(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *username = linphone_account_creator_get_username(account_creator);
	return username ? env->NewStringUTF(username) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setPhoneNumber(JNIEnv *env, jobject thiz, jlong ptr, jstring jphonenumber, jstring jcountrycode) {
	const char *phone_number = GetStringUTFChars(env, jphonenumber);
	const char *country_code = GetStringUTFChars(env, jcountrycode);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_phone_number(account_creator, phone_number, country_code);
	ReleaseStringUTFChars(env, jphonenumber, phone_number);
	ReleaseStringUTFChars(env, jcountrycode, country_code);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getPhoneNumber(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *phone_number = linphone_account_creator_get_phone_number(account_creator);
	return phone_number ? env->NewStringUTF(phone_number) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setPassword(JNIEnv *env, jobject thiz, jlong ptr, jstring jpassword) {
	const char *password = GetStringUTFChars(env, jpassword);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_password(account_creator, password);
	ReleaseStringUTFChars(env, jpassword, password);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getPassword(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *password = linphone_account_creator_get_password(account_creator);
	return password ? env->NewStringUTF(password) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setHa1(JNIEnv *env, jobject thiz, jlong ptr, jstring jha1) {
	const char *ha1 = GetStringUTFChars(env, jha1);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_ha1(account_creator, ha1);
	ReleaseStringUTFChars(env, jha1, ha1);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getHa1(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *ha1 = linphone_account_creator_get_ha1(account_creator);
	return ha1 ? env->NewStringUTF(ha1) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setActivationCode(JNIEnv *env, jobject thiz, jlong ptr, jstring jcode) {
	const char *activation_code = GetStringUTFChars(env, jcode);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_activation_code(account_creator, activation_code);
	ReleaseStringUTFChars(env, jcode, activation_code);
	return (jint) status;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setLanguage(JNIEnv *env, jobject thiz, jlong ptr, jstring jlang) {
	const char *lang = GetStringUTFChars(env, jlang);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_language(account_creator, lang);
	ReleaseStringUTFChars(env, jlang, lang);
	return (jint) status;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setTransport(JNIEnv *env, jobject thiz, jlong ptr, jint jtransport) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_transport(account_creator, (LinphoneTransportType)jtransport);
	return (jint) status;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_getTransport(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneTransportType transport = linphone_account_creator_get_transport(account_creator);
	return (jint) transport;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setDomain(JNIEnv *env, jobject thiz, jlong ptr, jstring jdomain) {
	const char *domain = GetStringUTFChars(env, jdomain);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_domain(account_creator, domain);
	ReleaseStringUTFChars(env, jdomain, domain);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getDomain(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *domain = linphone_account_creator_get_domain(account_creator);
	return domain ? env->NewStringUTF(domain) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setRoute(JNIEnv *env, jobject thiz, jlong ptr, jstring jroute) {
	const char *route = GetStringUTFChars(env, jroute);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_route(account_creator, route);
	ReleaseStringUTFChars(env, jroute, route);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getRoute(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *route = linphone_account_creator_get_route(account_creator);
	return route ? env->NewStringUTF(route) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setDisplayName(JNIEnv *env, jobject thiz, jlong ptr, jstring jname) {
	const char *name = GetStringUTFChars(env, jname);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_display_name(account_creator, name);
	ReleaseStringUTFChars(env, jname, name);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getDisplayName(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *display_name = linphone_account_creator_get_display_name(account_creator);
	return display_name ? env->NewStringUTF(display_name) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_setEmail(JNIEnv *env, jobject thiz, jlong ptr, jstring jemail) {
	const char *email = GetStringUTFChars(env, jemail);
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneAccountCreatorStatus status = linphone_account_creator_set_email(account_creator, email);
	ReleaseStringUTFChars(env, jemail, email);
	return (jint) status;
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getPrefix(JNIEnv *env, jobject thiz, jlong ptr, jstring jphonenumber) {
	char buff[4];
	const char *phone_number = GetStringUTFChars(env, jphonenumber);
	int prefix = linphone_dial_plan_lookup_ccc_from_e164(phone_number);
	snprintf(buff, sizeof(buff), "%d", prefix);
	ReleaseStringUTFChars(env, jphonenumber, phone_number);
	return (jstring) env->NewStringUTF(buff);
}

extern "C" jstring Java_org_linphone_core_LinphoneAccountCreatorImpl_getEmail(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char *email = linphone_account_creator_get_email(account_creator);
	return email ? env->NewStringUTF(email) : NULL;
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_isAccountUsed(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_is_account_used(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_createAccount(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_create_account(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_activateAccount(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_activate_account(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_isAccountLinked(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_is_account_linked(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_isPhoneNumberUsed(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_is_phone_number_used(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_isAccountActivated(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_is_account_activated(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_linkPhoneNumberWithAccount(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_link_phone_number_with_account(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_activatePhoneNumberLink(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_activate_phone_number_link(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_recoverPhoneAccount(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	return (jint) linphone_account_creator_recover_phone_account(account_creator);
}

extern "C" jint Java_org_linphone_core_LinphoneAccountCreatorImpl_updatePassword(JNIEnv *env, jobject thiz, jlong ptr, jstring jpasswd) {
	jint status;
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	const char* passwd = GetStringUTFChars(env, jpasswd);
	status = (jint) linphone_account_creator_update_password(account_creator, passwd);
	ReleaseStringUTFChars(env, jpasswd, passwd);
	return status;
}

extern "C" jobject Java_org_linphone_core_LinphoneAccountCreatorImpl_configure(JNIEnv *env, jobject thiz, jlong ptr) {
	LinphoneAccountCreator *account_creator = (LinphoneAccountCreator *)ptr;
	LinphoneProxyConfig  *lpc = linphone_account_creator_configure(account_creator);
	LinphoneCore *lc = account_creator->core;
	LinphoneCoreVTable *table = linphone_core_get_current_vtable(lc);
	LinphoneCoreData* lcData = (LinphoneCoreData*)linphone_core_v_table_get_user_data(table);
	return getProxy(env, lpc, lcData->core);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setTlsCertificate(JNIEnv *env, jobject thiz, jlong lc, jstring jcert) {
	const char* cert = GetStringUTFChars(env, jcert);
	linphone_core_set_tls_cert((LinphoneCore*)lc, cert);
	ReleaseStringUTFChars(env, jcert, cert);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setTlsKey(JNIEnv *env, jobject, jlong lc, jstring jkey) {
	const char* key = GetStringUTFChars(env, jkey);
	linphone_core_set_tls_key((LinphoneCore*)lc, key);
	ReleaseStringUTFChars(env, jkey, key);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setTlsCertificatePath(JNIEnv *env, jobject, jlong lc, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_tls_cert_path((LinphoneCore*)lc, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setTlsKeyPath(JNIEnv *env, jobject, jlong lc, jstring jpath) {
	const char* path = GetStringUTFChars(env, jpath);
	linphone_core_set_tls_key_path((LinphoneCore*)lc, path);
	ReleaseStringUTFChars(env, jpath, path);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVerifyServerCertificates(JNIEnv *env, jobject, jlong lc, jboolean enabled) {
	linphone_core_verify_server_certificates((LinphoneCore*)lc, enabled);
}

extern "C" void Java_org_linphone_core_LinphoneCoreImpl_setVerifyServerCN(JNIEnv *env, jobject, jlong lc, jboolean enabled) {
	linphone_core_verify_server_cn((LinphoneCore*)lc, enabled);
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getTlsCertificate(JNIEnv *env , jobject, jlong lc) {
	const char* cert = linphone_core_get_tls_cert((LinphoneCore*)lc);
	if (cert) {
		return env->NewStringUTF(cert);
	} else {
		return NULL;
	}
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getTlsKey(JNIEnv *env , jobject, jlong lc) {
	const char* key = linphone_core_get_tls_key((LinphoneCore*)lc);
	if (key) {
		return env->NewStringUTF(key);
	} else {
		return NULL;
	}
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getTlsCertificatePath(JNIEnv *env , jobject, jlong lc) {
	const char* path = linphone_core_get_tls_cert_path((LinphoneCore*)lc);
	if (path) {
		return env->NewStringUTF(path);
	} else {
		return NULL;
	}
}

extern "C" jstring Java_org_linphone_core_LinphoneCoreImpl_getTlsKeyPath(JNIEnv *env , jobject, jlong lc) {
	const char* path = linphone_core_get_tls_key_path((LinphoneCore*)lc);
	if (path) {
		return env->NewStringUTF(path);
	} else {
		return NULL;
	}
}
