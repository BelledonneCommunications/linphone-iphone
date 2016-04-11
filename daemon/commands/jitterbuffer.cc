#include "jitterbuffer.h"
#include <iostream>

#include <mediastreamer2/msrtp.h>
#include <private.h>
using namespace::std;


class JitterBufferResponse : public Response{
public:
	JitterBufferResponse(LinphoneCore *lc,bool audio, bool video){
		ostringstream ostr;
		if (audio)
			ostr<<"audio-jitter-buffer-size: "<<linphone_core_get_audio_jittcomp(lc)<<endl;
		if (video)
			ostr<<"video-jitter-buffer-size: "<<linphone_core_get_video_jittcomp(lc)<<endl;
		setBody(ostr.str().c_str());
	}
};

JitterBufferCommand::JitterBufferCommand() : DaemonCommand("jitter-buffer",
	"jitter-buffer <stream-type> [size <milliseconds>]" ,
	"Control jitter buffer parameters.\n"
	"jitter-buffer <stream-type> size <milliseconds> : sets the nominal jitter buffer size in milliseconds. Has no effect in a running call.\n"
	"jitter-buffer <stream-type> : gets the nominal jitter buffer size."
	){
	addExample(new DaemonCommandExample("jitter-buffer","Status: Ok\n\n"
				"audio-jitter-buffer-size: 60\nvideo-jitter-buffer-size: 60\n"));
	addExample(new DaemonCommandExample("jitter-buffer audio","Status: Ok\n\n"
				"audio-jitter-buffer-size: 60\n"));
	addExample(new DaemonCommandExample("jitter-buffer audio size 80","Status: Ok\n\n"
				"audio-jitter-buffer-size: 80\n"));
}


void JitterBufferCommand::exec(Daemon *app, const char *args){
	istringstream istr(args);
	string arg1;
	istr>>arg1;
	if (istr.fail()){
		app->sendResponse(JitterBufferResponse(app->getCore(),true,true));
	}else{
		if (arg1!="audio" && arg1!="video"){
			app->sendResponse("Invalid argument.");
			return;
		}
		string arg2;
		istr>>arg2;
		if (istr.fail()){
			app->sendResponse(JitterBufferResponse(app->getCore(),arg1=="audio",arg1=="video"));
		}else{
			if (arg2=="size"){
				int arg3;
				istr>>arg3;
				if (!istr.fail()){
					if (arg1=="audio")
						linphone_core_set_audio_jittcomp(app->getCore(),arg3);
					else if (arg1=="video")
						linphone_core_set_video_jittcomp(app->getCore(),arg3);
				}
				app->sendResponse(JitterBufferResponse(app->getCore(),arg1=="audio",arg1=="video"));
			}else{
				app->sendResponse(Response("Bad command argument."));
			}
		}
	}
}

JitterBufferResetCommand::JitterBufferResetCommand() : DaemonCommand("jitter-buffer-reset",
	"jitter-buffer-reset [call|stream] id [audio|video]" ,
	"Reset the RTP jitter buffer for a given call or stream id and stream type."
	){
	addExample(new DaemonCommandExample("jitter-buffer-reset call 3 audio",
		"Status: Ok\n"));
	addExample(new DaemonCommandExample("jitter-buffer-reset stream 12",
		"Status: Ok\n"));
}


void JitterBufferResetCommand::exec(Daemon *app, const char *args){
	istringstream istr(args);
	string arg1;
	istr>>arg1;
	if (istr.fail()){
		app->sendResponse(Response("Missing arguments"));
	}else{
		if (arg1!="call" && arg1!="stream"){
			app->sendResponse(Response("Invalid command syntax."));
			return;
		}
		int arg2;
		istr>>arg2;
		if (istr.fail()){
			app->sendResponse(Response("Missing call or stream id."));
			return;
		}else{
			MSFilter *rtprecv=NULL;
			if (arg1=="call"){
				LinphoneCall *call=app->findCall(arg2);
				string streamtype;
				if (call==NULL){
					app->sendResponse(Response("No such call id"));
					return;
				}
				istr>>streamtype;
				if (streamtype=="video"){
					rtprecv=call->videostream ? call->videostream->ms.rtprecv : NULL;
				}else{
					rtprecv=call->audiostream ? call->audiostream->ms.rtprecv : NULL;
				}
			}else{
				AudioStream *stream=app->findAudioStream(arg2);
				if (stream==NULL){
					app->sendResponse(Response("No such stream id"));
					return;
				}
				rtprecv=stream->ms.rtprecv;
			}
			if (rtprecv==NULL){
				app->sendResponse(Response("No such active stream"));
				return;
			}
			ms_filter_call_method_noarg(rtprecv,MS_RTP_RECV_RESET_JITTER_BUFFER);
			app->sendResponse(Response());
		}
	}
}


