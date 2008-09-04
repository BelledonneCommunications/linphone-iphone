#include "basiccall.h"
#include <signal.h>
static int flag = 1;
void stop(int sign){
	flag = 0;
}


int main(){
	BasicCall *call;
	char *id;
	CallMember *memberA, *memberB;
	MediaFlow *flow, *flow1;

	signal(SIGINT, stop);
	call = basic_call_new();
	memberA = basic_call_get_member(call,MemberA);
	memberB = basic_call_get_member(call,MemberB);

	id = "test_voice";
	printf("\n");
	flow = media_flow_new(id, MEDIA_FLOW_VOICE);
	
	basic_call_add_flow(call, flow);

	call_member_setup_flow(memberA, flow, "file://temp", "oss://0");
	call_member_setup_flow(memberB, flow, "oss://0", "oss://0");

	media_flow_setup_fd(flow, memberA, memberB, MEDIA_FLOW_HALF_DUPLEX); 
	basic_call_start_flow(call, flow);

	while(flag){
		sleep(1);
	}

}
