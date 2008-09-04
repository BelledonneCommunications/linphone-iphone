/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* this file is specifically distributed under a BSD license */

/**
* Copyright (C) 2007  Hiroki Mori (himori@users.sourceforge.net)
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"

MSFilter *ms_ca_read_new(MSSndCard *card);
MSFilter *ms_ca_write_new(MSSndCard *card);

typedef struct CAData{
	char *pcmdev;
	char *mixdev;
	AudioUnit caOutAudioUnit;
	AudioUnit caInAudioUnit;
	AudioStreamBasicDescription caOutASBD;
	AudioStreamBasicDescription caInASBD;
	AURenderCallbackStruct caOutRenderCallback;
	AURenderCallbackStruct caInRenderCallback;
	AudioConverterRef caOutConverter;
	AudioConverterRef caInConverter;
	int pcmfd;
	int rate;
	int bits;
	ms_mutex_t mutex;
	queue_t rq;
	MSBufferizer * bufferizer;
	bool_t read_started;
	bool_t write_started;
	bool_t stereo;
	void *caSourceBuffer;
	AudioBufferList	*fAudioBuffer, *fMSBuffer;
} CAData;

// Convenience function to dispose of our audio buffers
void DestroyAudioBufferList(AudioBufferList* list)
{
	UInt32						i;
	
	if(list) {
		for(i = 0; i < list->mNumberBuffers; i++) {
			if(list->mBuffers[i].mData)
			free(list->mBuffers[i].mData);
		}
		free(list);
	}
}

// Convenience function to allocate our audio buffers
AudioBufferList *AllocateAudioBufferList(UInt32 numChannels, UInt32 size)
{
	AudioBufferList*			list;
	UInt32						i;
	
	list = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + numChannels * sizeof(AudioBuffer));
	if(list == NULL)
	return NULL;
	
	list->mNumberBuffers = numChannels;
	for(i = 0; i < numChannels; ++i) {
		list->mBuffers[i].mNumberChannels = 1;
		list->mBuffers[i].mDataByteSize = size;
		list->mBuffers[i].mData = malloc(size);
		if(list->mBuffers[i].mData == NULL) {
			DestroyAudioBufferList(list);
			return NULL;
		}
	}
	return list;
}

OSStatus writeACInputProc (
	AudioConverterRef inAudioConverter,
	UInt32 *ioNumberDataPackets,
	AudioBufferList *ioData,
	AudioStreamPacketDescription **outDataPacketDescription,
	void* inUserData)
{
    OSStatus    err = noErr;
	CAData *d=(CAData*)inUserData;
	UInt32 packetSize = (d->bits / 8) * (d->stereo ? 2 : 1);
//	ms_error("writeACInputProc %d", *ioNumberDataPackets);

	if(*ioNumberDataPackets) {
		if(d->caSourceBuffer != NULL) {
			free(d->caSourceBuffer);
			d->caSourceBuffer = NULL;
		}

		d->caSourceBuffer = (void *) calloc (1, *ioNumberDataPackets * packetSize);

		ioData->mBuffers[0].mData = d->caSourceBuffer;			// tell the Audio Converter where it's source data is

		ms_mutex_lock(&d->mutex);
		int readsize = ms_bufferizer_read(d->bufferizer,d->caSourceBuffer,*ioNumberDataPackets * packetSize);
		ms_mutex_unlock(&d->mutex);
		if(readsize != *ioNumberDataPackets * packetSize) {
		  /* ms_error("ms_bufferizer_read error request = %d result = %d", *ioNumberDataPackets * packetSize, readsize); */
			memset(d->caSourceBuffer, 0, *ioNumberDataPackets * packetSize);
			ioData->mBuffers[0].mDataByteSize = *ioNumberDataPackets * packetSize;		// tell the Audio Converter how much source data there is
		} else {
			ioData->mBuffers[0].mDataByteSize = readsize;		// tell the Audio Converter how much source data there is
		}
	}

	return err;
}

OSStatus readACInputProc (AudioConverterRef inAudioConverter,
				     UInt32* ioNumberDataPackets,
				     AudioBufferList* ioData,
				     AudioStreamPacketDescription** ioASPD,
				     void* inUserData)
{
	CAData *d=(CAData*)inUserData;
	AudioBufferList* l_inputABL = d->fAudioBuffer;
	UInt32 totalInputBufferSizeBytes = ((*ioNumberDataPackets) * sizeof (float));
	int counter = d->caInASBD.mChannelsPerFrame;
	ioData->mNumberBuffers = d->caInASBD.mChannelsPerFrame;

	while (--counter >= 0)  {
		AudioBuffer* l_ioD_AB = &(ioData->mBuffers[counter]);
		l_ioD_AB->mNumberChannels = 1;
		l_ioD_AB->mData = (float*)(l_inputABL->mBuffers[counter].mData);
		l_ioD_AB->mDataByteSize = totalInputBufferSizeBytes;
	}

	return (noErr);
}

OSStatus readRenderProc(void *inRefCon, 
	AudioUnitRenderActionFlags *inActionFlags,
	const AudioTimeStamp *inTimeStamp, 
	UInt32 inBusNumber,
	UInt32 inNumFrames, 
	AudioBufferList *ioData)
{
	CAData *d=(CAData*)inRefCon;
	OSStatus	err = noErr;

	// Render into audio buffer
	err = AudioUnitRender(d->caInAudioUnit, inActionFlags, inTimeStamp, inBusNumber,
				inNumFrames, d->fAudioBuffer);
	if(err != noErr)
		ms_error("AudioUnitRender %d size = %d", err, d->fAudioBuffer->mBuffers[0].mDataByteSize);

	UInt32 AvailableOutputBytes = inNumFrames * sizeof (float);
    UInt32 propertySize = sizeof (AvailableOutputBytes);
    err = AudioConverterGetProperty (d->caInConverter,
		   kAudioConverterPropertyCalculateOutputBufferSize,
				     &propertySize,
				     &AvailableOutputBytes);

	if(err != noErr)
		ms_error("AudioConverterGetProperty %d", err);

	UInt32 ActualOutputFrames = AvailableOutputBytes / sizeof (short);
	err = AudioConverterFillComplexBuffer (d->caInConverter,
	   (AudioConverterComplexInputDataProc)(readACInputProc),
					   inRefCon,
					   &ActualOutputFrames,
					   d->fMSBuffer,
					   NULL);
	if(err != noErr)
		ms_error("readRenderProc:AudioConverterFillComplexBuffer %08x mNumberBuffers = %d", err, ioData->mNumberBuffers);

	mblk_t *rm=NULL;
	rm=allocb(d->fMSBuffer->mBuffers[0].mDataByteSize,0);
	memcpy(rm->b_wptr, d->fMSBuffer->mBuffers[0].mData, d->fMSBuffer->mBuffers[0].mDataByteSize);
//	memset(rm->b_wptr, 0, d->fMSBuffer->mBuffers[0].mDataByteSize);
	rm->b_wptr+=d->fMSBuffer->mBuffers[0].mDataByteSize;
	ms_mutex_lock(&d->mutex);
	putq(&d->rq,rm);
	ms_mutex_unlock(&d->mutex);
	rm=NULL;

	return err;
}

OSStatus writeRenderProc(void *inRefCon, 
	AudioUnitRenderActionFlags *inActionFlags,
	const AudioTimeStamp *inTimeStamp, 
	UInt32 inBusNumber,
	UInt32 inNumFrames, 
	AudioBufferList *ioData)
{
    OSStatus err= noErr;
    void *inInputDataProcUserData=NULL;
	CAData *d=(CAData*)inRefCon;
	if(d->write_started != FALSE) {
		AudioStreamPacketDescription* outPacketDescription = NULL;
		err = AudioConverterFillComplexBuffer(d->caOutConverter, writeACInputProc, inRefCon,
			&inNumFrames, ioData, outPacketDescription);
		if(err != noErr)
			ms_error("writeRenderProc:AudioConverterFillComplexBuffer err %08x %d", err, ioData->mNumberBuffers);
	}
    return err;
}

static void ca_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent)
{
	CAData *d=(CAData*)card->data;
}

static int ca_get_level(MSSndCard *card, MSSndCardMixerElem e)
{
	CAData *d=(CAData*)card->data;
	return 0;
}

static void ca_set_source(MSSndCard *card, MSSndCardCapture source)
{
	CAData *d=(CAData*)card->data;
}

static void ca_init(MSSndCard *card){
	ms_debug("ca_init");
	OSStatus err;
	UInt32 param;
	AudioDeviceID fInputDeviceID;
	CAData *d=ms_new(CAData,1);

	ComponentDescription desc;  

	// Get Default Output audio unit
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	Component comp = FindNextComponent(NULL, &desc);
		if (comp == NULL) return;

	err = OpenAComponent(comp, &d->caOutAudioUnit);
	if(err != noErr) return;

	// Get Default Input audio unit
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	comp = FindNextComponent(NULL, &desc);
		if (comp == NULL) return;

	err = OpenAComponent(comp, &d->caInAudioUnit);
	if(err != noErr) return;

	AudioUnitInitialize(d->caOutAudioUnit);
	AudioUnitInitialize(d->caInAudioUnit);

	UInt32 asbdsize = sizeof(AudioStreamBasicDescription);
	memset((char *)&d->caOutASBD, 0, asbdsize);
	memset((char *)&d->caInASBD, 0, asbdsize);

	// Setup Output audio unit
	OSStatus result = AudioUnitGetProperty (d->caOutAudioUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Output,
							0,
							&d->caOutASBD,
							&asbdsize);

	result = AudioUnitSetProperty (d->caOutAudioUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Input,
							0,
							&d->caOutASBD,
							asbdsize);

	// Setup Input audio unit
	// Enable input on the AUHAL
	param = 1;
	result = AudioUnitSetProperty(d->caInAudioUnit,
							kAudioOutputUnitProperty_EnableIO,
							kAudioUnitScope_Input,
							1,
							&param,
							sizeof(UInt32));

// Select the default input device
	param = sizeof(AudioDeviceID);
	result = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,
							&param,
							&fInputDeviceID);

	// Set the current device to the default input unit.
	result = AudioUnitSetProperty(d->caInAudioUnit,
							kAudioOutputUnitProperty_CurrentDevice,
							kAudioUnitScope_Global,
							0,
							&fInputDeviceID,
							sizeof(AudioDeviceID));

	AudioStreamBasicDescription tmpASBD;
	result = AudioUnitGetProperty (d->caInAudioUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Input,
							0,
							&tmpASBD,
							&asbdsize);
	
	int fAudioChannels = 1;
	d->caInASBD.mChannelsPerFrame = fAudioChannels;
	d->caInASBD.mSampleRate = tmpASBD.mSampleRate;
	d->caInASBD.mFormatID = kAudioFormatLinearPCM;
	d->caInASBD.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked |
										kAudioFormatFlagIsNonInterleaved;
	if (d->caInASBD.mFormatID == kAudioFormatLinearPCM && fAudioChannels == 1)
		d->caInASBD.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
	d->caInASBD.mFormatFlags = kAudioFormatFlagIsFloat;
	if (htonl(0x1234) == 0x1234)
	  d->caInASBD.mFormatFlags |= kAudioFormatFlagIsBigEndian;
	d->caInASBD.mBitsPerChannel = sizeof(Float32) * 8;
	d->caInASBD.mBytesPerFrame = d->caInASBD.mBitsPerChannel / 8;
	d->caInASBD.mFramesPerPacket = 1;
	d->caInASBD.mBytesPerPacket = d->caInASBD.mBytesPerFrame;

	err = AudioUnitSetProperty(d->caInAudioUnit,
							kAudioUnitProperty_StreamFormat,
							kAudioUnitScope_Output,
							1,
							&d->caInASBD,
							sizeof(AudioStreamBasicDescription));

	d->caSourceBuffer=NULL;

	// Get the number of frames in the IO buffer(s)
	param = sizeof(UInt32);
	UInt32 fAudioSamples;
	result = AudioUnitGetProperty(d->caInAudioUnit,
							kAudioDevicePropertyBufferFrameSize,
							kAudioUnitScope_Global,
							0,
							&fAudioSamples,
							&param);
	if(err != noErr)
	{
		fprintf(stderr, "failed to get audio sample size\n");
		return;
	}
	// Allocate our low device audio buffers
	d->fAudioBuffer = AllocateAudioBufferList(d->caInASBD.mChannelsPerFrame,
						fAudioSamples * d->caInASBD.mBytesPerFrame);
	if(d->fAudioBuffer == NULL)
	{
		fprintf(stderr, "failed to allocate buffers\n");
		return;
	}
	// Allocate our low device audio buffers
	d->fMSBuffer = AllocateAudioBufferList(d->caInASBD.mChannelsPerFrame,
						fAudioSamples * d->caInASBD.mBytesPerFrame);
	if(d->fMSBuffer == NULL)
	{
		fprintf(stderr, "failed to allocate buffers\n");
		return;
	}

	d->pcmdev=NULL;
	d->mixdev=NULL;
	d->pcmfd=-1;
	d->read_started=FALSE;
	d->write_started=FALSE;
	d->bits=16;
	d->rate=8000;
	d->stereo=FALSE;
	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	card->data=d;
}

static void ca_uninit(MSSndCard *card){
	CAData *d=(CAData*)card->data;
	if (d->pcmdev!=NULL) ms_free(d->pcmdev);
	if (d->mixdev!=NULL) ms_free(d->mixdev);
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(d);
}

static void ca_detect(MSSndCardManager *m);
static MSSndCard *ca_duplicate(MSSndCard *obj);

MSSndCardDesc ca_card_desc={
	.driver_type="CA",
	.detect=ca_detect,
	.init=ca_init,
	.set_level=ca_set_level,
	.get_level=ca_get_level,
	.set_capture=ca_set_source,
	.create_reader=ms_ca_read_new,
	.create_writer=ms_ca_write_new,
	.uninit=ca_uninit,
	.duplicate=ca_duplicate
};

static MSSndCard *ca_duplicate(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&ca_card_desc);
	CAData *dcard=(CAData*)card->data;
	CAData *dobj=(CAData*)obj->data;
	dcard->pcmdev=ms_strdup(dobj->pcmdev);
	dcard->mixdev=ms_strdup(dobj->mixdev);
	card->name=ms_strdup(obj->name);
	return card;
}

static MSSndCard *ca_card_new(){
	MSSndCard *card=ms_snd_card_new(&ca_card_desc);
	card->name=ms_strdup("Core Audio");
	return card;
}

static void ca_detect(MSSndCardManager *m){
	ms_debug("ca_detect");
	MSSndCard *card=ca_card_new();
        ms_snd_card_manager_add_card(m,card);
}

static void ca_start_r(MSSndCard *card){
	OSStatus err= noErr;
	CAData *d=(CAData*)card->data;
	ms_debug("ca_start_r");

	if (d->read_started==FALSE){
		AudioStreamBasicDescription outASBD;
		outASBD = d->caInASBD;
		outASBD.mSampleRate = d->rate;
		outASBD.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		if (htonl(0x1234) == 0x1234)
		  outASBD.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
		outASBD.mBytesPerPacket = (d->bits / 8) * outASBD.mChannelsPerFrame;
		outASBD.mBytesPerFrame = (d->bits / 8) * outASBD.mChannelsPerFrame;
		outASBD.mFramesPerPacket = 1;
		outASBD.mBitsPerChannel = d->bits;

		err = AudioConverterNew( &d->caInASBD, &outASBD, &d->caInConverter);
		if(err != noErr)
			ms_error("AudioConverterNew %x %d", err, outASBD.mBytesPerFrame);
		else
			CAShow(d->caInConverter);

		d->caInRenderCallback.inputProc = readRenderProc;
		d->caInRenderCallback.inputProcRefCon = d;
		err = AudioUnitSetProperty(d->caInAudioUnit,
						kAudioOutputUnitProperty_SetInputCallback,
						kAudioUnitScope_Global,
						0,
						&d->caInRenderCallback,
						sizeof(AURenderCallbackStruct));

		if(AudioOutputUnitStart(d->caInAudioUnit) == noErr)
			d->read_started = TRUE;
	}
}

static void ca_stop_r(MSSndCard *card){
	CAData *d=(CAData*)card->data;
	OSErr err;
	if(d->read_started == TRUE) {
		if(AudioOutputUnitStop(d->caInAudioUnit) == noErr)
			d->read_started=FALSE;
	}
}

static void ca_start_w(MSSndCard *card){
	OSStatus err= noErr;
	ms_debug("ca_start_w");
	CAData *d=(CAData*)card->data;
	if (d->write_started==FALSE){
		AudioStreamBasicDescription inASBD;
		inASBD = d->caOutASBD;
		inASBD.mSampleRate = d->rate;
		inASBD.mFormatID = kAudioFormatLinearPCM;
		// http://developer.apple.com/documentation/MusicAudio/Reference/CoreAudioDataTypesRef/Reference/reference.html
		inASBD.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
		if (htonl(0x1234) == 0x1234)
		  inASBD.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
		inASBD.mChannelsPerFrame = d->stereo ? 2 : 1;
		inASBD.mBytesPerPacket = (d->bits / 8) * inASBD.mChannelsPerFrame;
		inASBD.mBytesPerFrame = (d->bits / 8) * inASBD.mChannelsPerFrame;
		inASBD.mFramesPerPacket = 1;
		inASBD.mBitsPerChannel = d->bits;


		err = AudioConverterNew( &inASBD, &d->caOutASBD, &d->caOutConverter);
		if(err != noErr)
			ms_error("AudioConverterNew %x %d", err, inASBD.mBytesPerFrame);
		else
			CAShow(d->caOutConverter);

		if (inASBD.mChannelsPerFrame == 1 && d->caOutASBD.mChannelsPerFrame == 2)
		{
			if (d->caOutConverter)
			{
				// This should be as large as the number of output channels,
				// each element specifies which input channel's data is routed to that output channel
				SInt32 channelMap[] = { 0, 0 };
				err = AudioConverterSetProperty(d->caOutConverter, kAudioConverterChannelMap, 2*sizeof(SInt32), channelMap);
			}
		}

		memset((char*)&d->caOutRenderCallback, 0, sizeof(AURenderCallbackStruct));
		d->caOutRenderCallback.inputProc = writeRenderProc;
		d->caOutRenderCallback.inputProcRefCon = d;
		err = AudioUnitSetProperty (d->caOutAudioUnit, 
                            kAudioUnitProperty_SetRenderCallback, 
                            kAudioUnitScope_Input, 
                            0,
                            &d->caOutRenderCallback, 
                            sizeof(AURenderCallbackStruct));
		if(err != noErr)
			ms_error("AudioUnitSetProperty %x", err);

		if(err == noErr) {
			if(AudioOutputUnitStart(d->caOutAudioUnit) == noErr)
				d->write_started=TRUE;
		}
	}
}

static void ca_stop_w(MSSndCard *card){
	CAData *d=(CAData*)card->data;
	OSErr err;
	if(d->write_started == TRUE) {
		if(AudioOutputUnitStop(d->caOutAudioUnit) == noErr)
			d->write_started=FALSE;
	}
}

static mblk_t *ca_get(MSSndCard *card){
	CAData *d=(CAData*)card->data;
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void ca_put(MSSndCard *card, mblk_t *m){
	CAData *d=(CAData*)card->data;
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}


static void ca_read_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	ca_start_r(card);
}

static void ca_read_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	ca_stop_r(card);
}

static void ca_read_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ca_get(card))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void ca_write_preprocess(MSFilter *f){
	ms_debug("ca_write_preprocess");
	MSSndCard *card=(MSSndCard*)f->data;
	ca_start_w(card);
}

static void ca_write_postprocess(MSFilter *f){
	ms_debug("ca_write_postprocess");
	MSSndCard *card=(MSSndCard*)f->data;
	ca_stop_w(card);
}

static void ca_write_process(MSFilter *f){
//	ms_debug("ca_write_process");
	MSSndCard *card=(MSSndCard*)f->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		ca_put(card,m);
	}
}

static int set_rate(MSFilter *f, void *arg){
	ms_debug("set_rate %d", *((int*)arg));
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d=(CAData*)card->data;
	d->rate=*((int*)arg);
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	ms_debug("set_nchannels %d", *((int*)arg));
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d=(CAData*)card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}

static MSFilterMethod ca_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc ca_read_desc={
	.id=MS_CA_READ_ID,
	.name="MSCARead",
	.text="Sound capture filter for MacOS X Core Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.preprocess=ca_read_preprocess,
	.process=ca_read_process,
	.postprocess=ca_read_postprocess,
	.methods=ca_methods
};


MSFilterDesc ca_write_desc={
	.id=MS_CA_WRITE_ID,
	.name="MSCAWrite",
	.text="Sound playback filter for MacOS X Core Audio drivers",
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.preprocess=ca_write_preprocess,
	.process=ca_write_process,
	.postprocess=ca_write_postprocess,
	.methods=ca_methods
};

MSFilter *ms_ca_read_new(MSSndCard *card){
	ms_debug("ms_ca_read_new");
	MSFilter *f=ms_filter_new_from_desc(&ca_read_desc);
	f->data=card;
	return f;
}


MSFilter *ms_ca_write_new(MSSndCard *card){
	ms_debug("ms_ca_write_new");
	MSFilter *f=ms_filter_new_from_desc(&ca_write_desc);
	f->data=card;
	return f;
}

MS_FILTER_DESC_EXPORT(ca_read_desc)
MS_FILTER_DESC_EXPORT(ca_write_desc)
