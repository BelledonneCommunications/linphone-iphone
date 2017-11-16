/*
linphone
Copyright (C) 2017 Belledonne Communications SARL

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

#ifdef __ANDROID__

#include "private.h"
#include "platform-helpers.h"
#include <jni.h>





namespace LinphonePrivate{

class AndroidPlatformHelpers : public PlatformHelpers{
public:
	AndroidPlatformHelpers(LinphoneCore *lc, void *system_context);
	virtual void setDnsServers();
	virtual void acquireWifiLock();
	virtual void releaseWifiLock();
	virtual void acquireMcastLock();
	virtual void releaseMcastLock();
	virtual void acquireCpuLock();
	virtual void releaseCpuLock();
	~AndroidPlatformHelpers();
private:
	int callVoidMethod(jmethodID id);
	static jmethodID getMethodId(JNIEnv *env, jclass klass, const char *method, const char *signature);
	jobject mJavaHelper;
	jmethodID mWifiLockAcquireId;
	jmethodID mWifiLockReleaseId;
	jmethodID mMcastLockAcquireId;
	jmethodID mMcastLockReleaseId;
	jmethodID mCpuLockAcquireId;
	jmethodID mCpuLockReleaseId;
	jmethodID mGetDnsServersId;
	jmethodID mGetPowerManagerId;

};

jmethodID AndroidPlatformHelpers::getMethodId(JNIEnv *env, jclass klass, const char *method, const char *signature){
	jmethodID id = env->GetMethodID(klass, method, signature);
	if (id == 0){
		ms_fatal("Could not find java method '%s %s'", method, signature);
	}
	return id;
}

AndroidPlatformHelpers::AndroidPlatformHelpers(LinphoneCore *lc, void *system_context) : PlatformHelpers(lc) {
	JNIEnv *env=ms_get_jni_env();
	jclass klass = env->FindClass("org/linphone/core/util/AndroidPlatformHelper");
	if (!klass){
		ms_fatal("Could not find java AndroidPlatformHelper class");
		return;
	}
	jmethodID ctor = env->GetMethodID(klass,"<init>", "(Ljava/lang/Object;)V");
	mJavaHelper = env->NewObject(klass, ctor, (jobject)system_context);
	if (!mJavaHelper){
		ms_error("Could not instanciate AndroidPlatformHelper object.");
		return;
	}
	mJavaHelper = (jobject) env->NewGlobalRef(mJavaHelper);

	mWifiLockAcquireId = getMethodId(env, klass, "acquireWifiLock", "()V");
	mWifiLockReleaseId = getMethodId(env, klass, "releaseWifiLock", "()V");
	mMcastLockAcquireId = getMethodId(env, klass, "acquireMcastLock", "()V");
	mMcastLockReleaseId = getMethodId(env, klass, "releaseMcastLock", "()V");
	mCpuLockAcquireId = getMethodId(env, klass, "acquireCpuLock", "()V");
	mCpuLockReleaseId = getMethodId(env, klass, "releaseCpuLock", "()V");
	mGetDnsServersId = getMethodId(env, klass, "getDnsServers", "()[Ljava/lang/String;");
	mGetPowerManagerId = getMethodId(env, klass, "getPowerManager", "()Ljava/lang/Object;");

	jobject pm = env->CallObjectMethod(mJavaHelper,mGetPowerManagerId);
	belle_sip_wake_lock_init(env, pm);

	ms_message("AndroidPlatformHelpers is fully initialised");
}

AndroidPlatformHelpers::~AndroidPlatformHelpers(){
	if (mJavaHelper){
		JNIEnv *env = ms_get_jni_env();
		belle_sip_wake_lock_uninit(env);
		env->DeleteGlobalRef(mJavaHelper);
		mJavaHelper = NULL;
	}
}


void AndroidPlatformHelpers::setDnsServers(){
	if (!mJavaHelper || linphone_core_get_dns_set_by_app(mCore)) return;
	JNIEnv *env=ms_get_jni_env();
	if (env && mJavaHelper) {
		jobjectArray jservers = (jobjectArray)env->CallObjectMethod(mJavaHelper,mGetDnsServersId);
		bctbx_list_t *l = NULL;
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			ms_error("AndroidPlatformHelpers::setDnsServers() exception");
			return;
		}
		if (jservers != NULL){
			int count = env->GetArrayLength(jservers);

			for (int i=0; i < count; i++) {
				jstring jserver = (jstring) env->GetObjectArrayElement(jservers, i);
				const char *str = env->GetStringUTFChars(jserver, NULL);
				if (str){
					l = bctbx_list_append(l, ms_strdup(str));
					env->ReleaseStringUTFChars(jserver, str);
				}
			}
		}
		linphone_core_set_dns_servers(mCore, l);
		bctbx_list_free_with_data(l, ms_free);
	}
}

void AndroidPlatformHelpers::acquireWifiLock(){
	callVoidMethod(mWifiLockAcquireId);
}

void AndroidPlatformHelpers::releaseWifiLock(){
	callVoidMethod(mWifiLockReleaseId);
}

void AndroidPlatformHelpers::acquireMcastLock(){
	callVoidMethod(mMcastLockAcquireId);
}

void AndroidPlatformHelpers::releaseMcastLock(){
	callVoidMethod(mMcastLockReleaseId);
}

void AndroidPlatformHelpers::acquireCpuLock(){
	callVoidMethod(mCpuLockAcquireId);
}

void AndroidPlatformHelpers::releaseCpuLock(){
	callVoidMethod(mCpuLockReleaseId);
}


int AndroidPlatformHelpers::callVoidMethod(jmethodID id) {
	JNIEnv *env=ms_get_jni_env();
	if (env && mJavaHelper) {
		env->CallVoidMethod(mJavaHelper,id);
		if (env->ExceptionCheck()) {
			env->ExceptionClear();
			return -1;
		} else
			return 0;
	} else
		return -1;
}

PlatformHelpers *createAndroidPlatformHelpers(LinphoneCore *lc, void *system_context){
	return new AndroidPlatformHelpers(lc, system_context);
}

}//end of namespace




#endif
