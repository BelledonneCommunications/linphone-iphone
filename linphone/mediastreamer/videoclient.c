#include "ms.h"
#include "msavdecoder.h"
#include "mstcpclient.h"
#include "mssdlout.h"
#include "mstimer.h"
#include <signal.h>


static gboolean running=TRUE;

static void sighandler(int signum){
	running=FALSE;
}

int main(int argc, char *argv[]){
	MSFilter *source,*decoder,*output;
	MSSync *sync;
	
	if (argc<3){
		fprintf(stderr,"videoclient <ip> <port>\n");
		return -1;
	}
	
	signal(SIGINT,sighandler);
	
	ms_init();
	
	source=ms_tcp_client_new();
	ms_tcp_client_connect(MS_TCP_CLIENT(source),argv[1],atoi(argv[2]));
	
	decoder=ms_h263_decoder_new();
	ms_AVdecoder_set_format(MS_AVDECODER(decoder),"YUV420P");
	
	output=ms_sdl_out_new();
	ms_sdl_out_set_format(MS_SDL_OUT(output),"YUV420P");
	
	ms_filter_add_link(source,decoder);
	ms_filter_add_link(decoder,output);
	
	sync=ms_timer_new();
	
	ms_sync_attach(sync,source);
	
	ms_start(sync);
	
	while(running) sleep(1);
	
	ms_stop(sync);
	
	ms_sync_detach(sync,source);
	
	ms_filter_remove_links(source,decoder);
	ms_filter_remove_links(decoder,output);
	
	ms_filter_destroy(source);
	ms_filter_destroy(decoder);
	ms_filter_destroy(output);
	return 0;
}

