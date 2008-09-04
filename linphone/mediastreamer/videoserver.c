#include "ms.h"
#include "msv4l.h"
#include "msavencoder.h"
#include "mstcpserv.h"
#include "mstimer.h"
#include <signal.h>


static gboolean running=TRUE;

static void sighandler(int signum){
	running=FALSE;
}

int main(int arg, char *argv[]){
	MSFilter *source,*encoder,*sender;
	MSSync *sync;
	
	signal(SIGINT,sighandler);
	
	ms_init();
	
	source=ms_v4l_new();
	ms_v4l_set_device(MS_V4L(source),"/dev/video0");
	ms_v4l_start(MS_V4L(source));
	
	encoder=ms_h263_encoder_new();
	
	sender=ms_tcp_serv_new();
	
	ms_filter_add_link(source,encoder);
	ms_filter_add_link(encoder,sender);
	
	sync=ms_timer_new();
	
	ms_sync_attach(sync,source);
	
	ms_start(sync);
	
	while(running) sleep(1);
	
	ms_stop(sync);
	
	ms_sync_detach(sync,source);
	
	ms_v4l_stop(MS_V4L(source));
	
	ms_filter_remove_links(source,encoder);
	ms_filter_remove_links(encoder,sender);
	
	ms_filter_destroy(source);
	ms_filter_destroy(encoder);
	ms_filter_destroy(sender);
	return 0;
}

