#include "p2pproxy.h"
#include <pthread.h>
#include <unistd.h>

static void * thread_starter(void *args){
	char* largs[] = {"-seeding-server","-sip", "5058"};
	p2pproxy_application_start(3,largs);
	return NULL;
}


int main(int argc, char **argv) {
	pthread_t th;
	printf("starting p2pproxy tester \n");
	pthread_create(&th,NULL,thread_starter,NULL);
	sleep(3);
	
	if (p2pproxy_application_get_state() == P2PPROXY_CONNECTED) {
		printf("CONNECTED \n");
	} else {
		printf("NOT CONNECTED \n");
	};


	if (p2pproxy_accountmgt_createAccount("sip:titi@p2p.linphone.org") != P2PPROXY_NO_ERROR) {
		printf("cannot create account \n");	
	}
	
	
	if (p2pproxy_accountmgt_isValidAccount("sip:titi@p2p.linphone.org") != P2PPROXY_ACCOUNTMGT_USER_EXIST) {
		printf("user not created \n");	
	}
	
	char string_buffer[256];
	if (p2pproxy_resourcemgt_lookup_sip_proxy(string_buffer,256,"p2p.linphone.org") != P2PPROXY_NO_ERROR) {
		printf("cannot get proxy\n");	
	} else {
		printf("registrar is [%s]\n",string_buffer);
	}
	
	if (p2pproxy_resourcemgt_revoke_sip_proxy(string_buffer) != P2PPROXY_NO_ERROR) {
		printf("cannot fulsh  proxy [%s]\n",string_buffer);	
	}

	if (p2pproxy_resourcemgt_lookup_sip_proxy(string_buffer,256,"toto.linphone.org") != P2PPROXY_RESOURCEMGT_SERVER_NOT_FOUND) {
		printf("unexpected proxy [%s]\n",string_buffer);	
	} else {
		printf("unknown domaine\n");
	}

	if (p2pproxy_accountmgt_deleteAccount("sip:titi@p2p.linphone.org") != P2PPROXY_NO_ERROR) {
		printf("cannot delete account \n");	
	}
	
	
	p2pproxy_application_stop();
	pthread_join(th,NULL);
	return 0;
}