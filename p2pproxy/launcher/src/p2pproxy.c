#include <stdio.h>
#include <jni.h>
#include <string.h>
#include "p2pproxy.h"

#ifndef P2PPROXY_JMX_PORT
	#define P2PPROXY_JMX_PORT "5678"
#endif
#ifndef P2PPROXY_INSTALLDIR
	#define P2PPROXY_INSTALLDIR "/usr/local/share/java"
#endif 
#ifndef P2PPROXY_BUILDDIR
	#define P2PPROXY_BUILDDIR "../antbuild/dist/p2pproxy_0.1"
#endif
#define NUMBER_OF_OPTION 7
JavaVM* p2pproxy_application_jvm = 0;

#define GET_JNI_ENV \
	jint lResult = 0 ;\
	JNIEnv* lJniEnv = 0;\
	jclass  lMainClass = 0;\
	lResult = (*p2pproxy_application_jvm)->AttachCurrentThread(p2pproxy_application_jvm,&lJniEnv,NULL);\
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
	JNIEnv* lJniEnv = 0;
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
	args.ignoreUnrecognized = JNI_FALSE;	int lResult;
	

	res = JNI_CreateJavaVM(&p2pproxy_application_jvm, (void **)&lJniEnv, &args);
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

int p2pproxy_resourcemgt_lookup_sip_proxy(char* proxy_uri,size_t size, char* domaine) {
	jmethodID lLookupSipProxyUriMethod;
	jstring lJStringResult; 
	const jbyte* lString;
	jboolean  lIsCopy;
	jstring applicationArg;
	
	GET_JNI_ENV
	 
	
	applicationArg = (*lJniEnv)->NewStringUTF(lJniEnv, domaine);
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

int p2pproxy_resourcemgt_revoke_sip_proxy(char* proxy_uri) {
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
void p2pproxy_application_stop() {
	jmethodID stopMethod = 0;
	GET_JNI_ENV
	
	stopMethod = (*lJniEnv)->GetStaticMethodID(lJniEnv, lMainClass, "stop", "()V");
	(*lJniEnv)->CallStaticVoidMethod(lJniEnv, lMainClass, stopMethod);
	(*p2pproxy_application_jvm)->DetachCurrentThread(p2pproxy_application_jvm);
	return;

}


