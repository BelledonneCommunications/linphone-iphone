#include <stdio.h>
#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include "p2pproxy.h"

#ifndef P2PPROXY_JMX_PORT
	#define P2PPROXY_JMX_PORT "5678"
#endif
#ifndef P2PPROXY_INSTALLDIR
	#define P2PPROXY_INSTALLDIR "/usr/share/java/fonis"
#endif 
#ifndef P2PPROXY_BUILDDIR
	#define P2PPROXY_BUILDDIR "../antbuild/dist/p2pproxy_0.1"
#endif
#define NUMBER_OF_OPTION 7
JavaVM *p2pproxy_application_jvm = 0;

#define GET_JNI_ENV \
	jint lResult = 0 ;\
	JNIEnv *lJniEnv = 0;\
	jclass  lMainClass = 0;\
	if (p2pproxy_application_jvm==0) return P2PPROXY_ERROR_APPLICATION_NOT_STARTED; \
	lResult = (*p2pproxy_application_jvm)->AttachCurrentThread(p2pproxy_application_jvm,(void**)&lJniEnv,NULL);\
	if (lResult != 0) { \
		fprintf(stderr,"cannot attach VM\n");\
		return P2PPROXY_ERROR;\
	}\
	lMainClass = (*lJniEnv)->FindClass(lJniEnv, "org/linphone/p2pproxy/core/P2pProxyMain");\
	if (lMainClass == 0) { \
		fprintf(stderr,"cannot load class org/linphone/p2pproxy/core/P2pProxyMain\n");\
		return P2PPROXY_ERROR; \
	} \


int p2pproxy_application_start(int argc, char **argv) {
	JNIEnv *lJniEnv = 0;
	jclass  lMainClass = 0;
	JavaVMInitArgs args;
	JavaVMOption options[NUMBER_OF_OPTION];
	jint res=-1;
	jmethodID mainMethod;
	jobjectArray applicationArgsList;
	jstring applicationArg;
	int i=0;
	int optioncount=0;

	if (p2pproxy_application_jvm != 0) {
		fprintf(stderr,"p2pproxy already started");
		return P2PPROXY_ERROR_APPLICATION_ALREADY_STARTED;
	}
	args.version = JNI_VERSION_1_4;
	
	/*options[optioncount++].optionString = "-verbose:jni";*/
	/*options[optioncount++].optionString = "-verbose:class";*/
	/*options[optioncount++].optionString = "-verbose:class";*/
	options[optioncount++].optionString = "-Djava.class.path="P2PPROXY_BUILDDIR"/p2pproxy.jar:"\
								P2PPROXY_INSTALLDIR"/p2pproxy.jar:"\
								P2PPROXY_BUILDDIR"/log4j.jar:"\
								P2PPROXY_INSTALLDIR"/log4j.jar";



	options[optioncount++].optionString = "-Dcom.sun.management.jmxremote";
	options[optioncount++].optionString = "-Dcom.sun.management.jmxremote.port="P2PPROXY_JMX_PORT;
	options[optioncount++].optionString = "-Dcom.sun.management.jmxremote.authenticate=false";
	options[optioncount++].optionString = "-Dcom.sun.management.jmxremote.ssl=false";
	options[optioncount++].optionString = "-Dorg.linphone.p2pproxy.install.dir="P2PPROXY_INSTALLDIR;
	options[optioncount++].optionString = "-Dorg.linphone.p2pproxy.build.dir="P2PPROXY_BUILDDIR;

	args.nOptions = NUMBER_OF_OPTION;
	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;
	

	res = JNI_CreateJavaVM(&p2pproxy_application_jvm,  (void**)& lJniEnv, &args);
	if (res < 0) {
		fprintf(stderr,"cannot start p2pproxy vm [%i]",res);
		return P2PPROXY_ERROR;
	}
	lMainClass = (*lJniEnv)->FindClass(lJniEnv, "org/linphone/p2pproxy/core/P2pProxyMain");

	if (lMainClass == 0) {
		fprintf(stderr,"cannot load class org/linphone/p2pproxy/core/P2pProxyMain\n");
		return P2PPROXY_ERROR;
	}
	mainMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "main", "([Ljava/lang/String;)V");

	applicationArgsList = (*lJniEnv)->NewObjectArray(lJniEnv, argc, (*lJniEnv)->FindClass(lJniEnv, "java/lang/String"), NULL);
	
	for (i=0;i<argc;i++) {
		applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, argv[i]);
		(*lJniEnv)->SetObjectArrayElement(lJniEnv, applicationArgsList, i, applicationArg);

	}

	(*lJniEnv)->CallStaticVoidMethod(lJniEnv, lMainClass, mainMethod, applicationArgsList);
	(*p2pproxy_application_jvm)->DestroyJavaVM(p2pproxy_application_jvm);
	p2pproxy_application_jvm = 0;
	return P2PPROXY_NO_ERROR;
}
	


const char* p2pproxy_status_string(int status_code) {
	return 0;
}


int p2pproxy_accountmgt_createAccount(const char* user_name) {
	jmethodID createAccountMethod;
	jstring applicationArg;
	GET_JNI_ENV
	createAccountMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "createAccount", "(Ljava/lang/String;)I");
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, user_name);
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, createAccountMethod, applicationArg);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
}

int p2pproxy_accountmgt_isValidAccount(const char* user_name) {
	jmethodID isValidAccountMethod;
	jstring applicationArg; 
	GET_JNI_ENV
	isValidAccountMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "isValidAccount", "(Ljava/lang/String;)I");
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, user_name);
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, isValidAccountMethod, applicationArg);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
}

int p2pproxy_accountmgt_deleteAccount(const char* user_name) {
	jmethodID deleteAccountMethod;
	jstring applicationArg; 
	GET_JNI_ENV
	deleteAccountMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "deleteAccount", "(Ljava/lang/String;)I");
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, user_name);
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, deleteAccountMethod, applicationArg);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
}

int p2pproxy_resourcemgt_lookup_sip_proxy(char* proxy_uri,size_t size, const char* domain) {
	jmethodID lLookupSipProxyUriMethod;
	jstring lJStringResult; 
	const char* lString;
	jboolean  lIsCopy;
	jstring applicationArg;
	
	GET_JNI_ENV
	 
	
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, domain);
	lLookupSipProxyUriMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "lookupSipProxyUri", "(Ljava/lang/String;)Ljava/lang/String;");
	lJStringResult = (*lJniEnv)->CallStaticObjectMethod(lJniEnv, lMainClass, lLookupSipProxyUriMethod, applicationArg);
	if (lJStringResult == 0) {
		return P2PPROXY_RESOURCEMGT_SERVER_NOT_FOUND;
	}
	lString =  (*lJniEnv)->GetStringUTFChars(lJniEnv, lJStringResult, &lIsCopy);
	memcpy(proxy_uri,lString,size);
	(*lJniEnv)->ReleaseStringUTFChars(lJniEnv, lJStringResult, lString);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return P2PPROXY_NO_ERROR;
}

int p2pproxy_resourcemgt_revoke_sip_proxy(const char* proxy_uri) {
	jmethodID revokeProxyMethod;
	jstring applicationArg;
	GET_JNI_ENV
	
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, proxy_uri);
	revokeProxyMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "revokeSipProxy", "(Ljava/lang/String;)I");
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, revokeProxyMethod,applicationArg);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
}
int p2pproxy_application_get_state() {
	jmethodID stateMethod;
	GET_JNI_ENV
	stateMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "getState", "()I");
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, stateMethod);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
	
}

int p2pproxy_application_stop() {
	jmethodID stopMethod = 0;
	GET_JNI_ENV
	
	stopMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "stop", "()V");
	(*lJniEnv)->CallStaticVoidMethod(lJniEnv, lMainClass, stopMethod);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return 0;
}


p2pproxy_resourcemgt_resource_list_t* p2pproxy_resourcemgt_new_resource_list() {
	p2pproxy_resourcemgt_resource_list_t* resource_list = malloc(sizeof (p2pproxy_resourcemgt_resource_list_t));
	resource_list->size = 0;
	return resource_list;
}

void p2pproxy_resourcemgt_delete_resource_list(p2pproxy_resourcemgt_resource_list_t* resource_list) {
	int i;
	if (resource_list == 0 ) {
		return;
	}
	for (i=0;i<resource_list->size;i++) {
		free(resource_list->resource_uri[i]);
	}
	resource_list->size = 0;
	free(resource_list);  
}

int p2pproxy_resourcemgt_lookup_media_resource(p2pproxy_resourcemgt_resource_list_t* resource_list, const char* domain)  {
	jmethodID lLookupMediaResourceMethod;
	jarray lJStringResults;
	jstring lResourceInstance; 
	const char* lString;
	jboolean  lIsCopy;
	jstring applicationArg;
	int i;
	jsize lStringSize;
	jsize lArraySize;
	
	GET_JNI_ENV
	 
	
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, domain);
	lLookupMediaResourceMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "lookupMediaServerAddress", "(Ljava/lang/String;)[Ljava/lang/String;");
	lJStringResults = (*lJniEnv)->CallStaticObjectMethod(lJniEnv, lMainClass, lLookupMediaResourceMethod, applicationArg);
	if (lJStringResults == 0) {
		return P2PPROXY_RESOURCEMGT_SERVER_NOT_FOUND;
	}
	
	lArraySize = (*lJniEnv)->GetArrayLength(lJniEnv, lJStringResults);
	for (i=0;i<lArraySize & i<P2PPROXY_MAX_RESOURCE_LIST_SIZE; i++) {
		lResourceInstance =  (*lJniEnv)->GetObjectArrayElement(lJniEnv,lJStringResults, i);
		lString =  (*lJniEnv)->GetStringUTFChars(lJniEnv, lResourceInstance, &lIsCopy);
		lStringSize = (*lJniEnv)->GetStringLength(lJniEnv, lResourceInstance);
		resource_list->resource_uri[i] = malloc(lStringSize);
		strcpy(resource_list->resource_uri[i],lString);
		resource_list->size=i;
		(*lJniEnv)->ReleaseStringUTFChars(lJniEnv, lResourceInstance, lString);
	}
	
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return P2PPROXY_NO_ERROR;
}
int p2pproxy_resourcemgt_revoke_media_resource(const char* resource_uri) {
	jmethodID revokeMediaResourceMethod;
	jstring applicationArg;
	GET_JNI_ENV
	
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, resource_uri);
	revokeMediaResourceMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "revokeMediaServer", "(Ljava/lang/String;)I");
	lResult = (*lJniEnv)->CallStaticIntMethod(lJniEnv, lMainClass, revokeMediaResourceMethod,applicationArg);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return lResult;
}
