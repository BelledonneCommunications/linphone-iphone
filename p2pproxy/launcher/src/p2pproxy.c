#include <stdio.h>
#include <jni.h>
#include "p2pproxy.c"

#ifndef P2PPROXY_CLASSPATH
	#define P2PPROXY_INSTALL_PREFIX "/usr/"
#endif 
#ifndef P2PPROXY_JMX_PORT
	#define P2PPROXY_JMX_PORT "5678"
#endif
JNIEnv* p2pproxy_application_jnienv = 0;

int p2pproxy_application_start(int argc, char **argv) {
	JavaVM* jvm;
	JNIEnv* env;
	JavaVMInitArgs args;
	JavaVMOption options[5];
	
	if (p2pproxy_application_jnienv != 0) {
		fprintf(stderr,"p2pproxy already started");
		return P2PPROXY_ERROR_APPLICATION_ALREADY_STARTED;
	}
	args.version = JNI_VERSION_1_6;
	args.nOptions = sizeof (options);
	options[0].optionString = "-Dcom.sun.management.jmxremote";
	options[1].optionString = "-Dcom.sun.management.jmxremote.port="P2PPROXY_JMX_POR";
	options[2].optionString = "-Dcom.sun.management.jmxremote.authenticate=false";
	options[3].optionString = "-Dcom.sun.management.jmxremote.ssl=false";
	options[4].optionString = "-Djava.class.path="P2PPROXY_INSTALL_PREFIX"/share/java/p2pproxy.jar";
 		
	args.options = options;
	args.ignoreUnrecognized = JNI_FALSE;

	JNI_CreateJavaVM(&jvm, (void **)&env, &args);
	return env;
	return P2PPROXY_ERROR;
}


const char* p2pproxy_status_string(int status_code) {
	return P2PPROXY_ERROR;
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
	jclass helloWorldClass;
	jmethodID mainMethod;
	jobjectArray applicationArgs;
	jstring applicationArg0;

	helloWorldClass = (*env)->FindClass(env, "example/jni/InvocationHelloWorld");

	mainMethod = (*env)->GetStaticMethodID(env, helloWorldClass, "main", "([Ljava/lang/String;)V");

	applicationArgs = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/String"), NULL);
	applicationArg0 = (*env)->NewStringUTF(env, "From-C-program");
	(*env)->SetObjectArrayElement(env, applicationArgs, 0, applicationArg0);

	(*env)->CallStaticVoidMethod(env, helloWorldClass, mainMethod, applicationArgs);
}


int main(int argc, char **argv) {
	JNIEnv* env = create_vm();
	invoke_class( env );
}