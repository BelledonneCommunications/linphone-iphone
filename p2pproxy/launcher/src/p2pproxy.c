#include <stdio.h>
#include <jni.h>
#include "p2pproxy.h"

#ifndef P2PPROXY_JMX_PORT
	#define P2PPROXY_JMX_PORT "5678"
#endif
#ifndef P2PPROXY_INSTALLDIR
	#define P2PPROXY_INSTALLDIR "/usr/local/share/java"
#endif 
#ifndef P2PPROXY_BUILDDIR
	#define P2PPROXY_BUILDDIR ".././antbuild/dist/p2pproxy_0.1"
#endif
JNIEnv* p2pproxy_application_jnienv = 0;
JavaVM* p2pproxy_application_jvm = 0;

int p2pproxy_application_start(int argc, char **argv) {

	JavaVMInitArgs args;
	JavaVMOption options[8];
	jint res=-1;
	jclass lP2pProxyMainClass;
	jmethodID mainMethod;
	jobjectArray applicationArgsList;
	jstring applicationArg;
	int i=0;

	if (p2pproxy_application_jnienv != 0) {
		fprintf(stderr,"p2pproxy already started");
		return P2PPROXY_ERROR_APPLICATION_ALREADY_STARTED;
	}
	args.version = JNI_VERSION_1_4;
	args.nOptions = 8;
	options[0].optionString = "-verbose:jni";
	options[1].optionString = "-Djava.class.path="P2PPROXY_BUILDDIR"/p2pproxy.jar:"\
								P2PPROXY_INSTALLDIR"/p2pproxy.jar:"\
								P2PPROXY_BUILDDIR"/log4j.jar:"\
								P2PPROXY_INSTALLDIR"/log4j.jar";



	options[2].optionString = "-Dcom.sun.management.jmxremote";
	options[3].optionString = "-Dcom.sun.management.jmxremote.port="P2PPROXY_JMX_PORT;
	options[4].optionString = "-Dcom.sun.management.jmxremote.authenticate=false";
	options[5].optionString = "-Dcom.sun.management.jmxremote.ssl=false";
	options[6].optionString = "-Dorg.linphone.p2pproxy.install.dir="P2PPROXY_INSTALLDIR;
	options[7].optionString = "-Dorg.linphone.p2pproxy.build.dir="P2PPROXY_BUILDDIR;


	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

	res = JNI_CreateJavaVM(&p2pproxy_application_jvm, (void **)&p2pproxy_application_jnienv, &args);
	if (res < 0) {
		fprintf(stderr,"cannot start p2pproxy vm [%i]",res);
		return P2PPROXY_ERROR;
	}

	lP2pProxyMainClass = (*p2pproxy_application_jnienv)->FindClass(p2pproxy_application_jnienv, "org/linphone/p2pproxy/core/P2pProxyMain");

	if (lP2pProxyMainClass == 0) {
		fprintf(stderr,"cannot find class org/linphone/p2pproxy/core/P2pProxyMain");
		return P2PPROXY_ERROR;
	}
	mainMethod = (*p2pproxy_application_jnienv)->GetStaticMethodID(p2pproxy_application_jnienv, lP2pProxyMainClass, "main", "([Ljava/lang/String;)V");

	applicationArgsList = (*p2pproxy_application_jnienv)->NewObjectArray(p2pproxy_application_jnienv, argc, (*p2pproxy_application_jnienv)->FindClass(p2pproxy_application_jnienv, "java/lang/String"), NULL);
	
	for (i=0;i<argc;i++) {
		applicationArg = (*p2pproxy_application_jnienv)->NewStringUTF(p2pproxy_application_jnienv, *argv++);
		(*p2pproxy_application_jnienv)->SetObjectArrayElement(p2pproxy_application_jnienv, applicationArgsList, 0, applicationArg);

	}


	(*p2pproxy_application_jnienv)->CallStaticVoidMethod(p2pproxy_application_jnienv, lP2pProxyMainClass, mainMethod, applicationArgsList);

	return P2PPROXY_NO_ERROR;
}


const char* p2pproxy_status_string(int status_code) {
	return 0;
}


int p2pproxy_accountmgt_createAccount(const char* user_name) {
	return P2PPROXY_ERROR;
}

int p2pproxy_accountmgt_isValidAccount(const char* user_name) {
	return P2PPROXY_ERROR;
}

int p2pproxy_accountmgt_deleteAccount(const char* user_name) {
	return P2PPROXY_ERROR;
}



JNIEnv* create_vm() {

}

void invoke_class(JNIEnv* env) {
	
}


