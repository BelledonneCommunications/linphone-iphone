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
	printf("starting p2pproxy tester");
	pthread_create(&th,NULL,thread_starter,NULL);
	
	sleep(1000);
	/*p2pproxy_application_stop();*/
	pthread_join(th,NULL);
	return 0;
}