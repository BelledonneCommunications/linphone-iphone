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

#ifndef UNICODE
#define UNICODE
#endif

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"

#include <mmsystem.h>
#ifdef _MSC_VER
#include <mmreg.h>
#endif
#include <msacm.h>

#if defined(_WIN32_WCE)
/*#define DISABLE_SPEEX */
/*#define WCE_OPTICON_WORKAROUND 1000 */
#endif
#ifndef DISABLE_SPEEX
#include <speex/speex_preprocess.h>
#endif

#define WINSND_NBUFS 10
#define WINSND_OUT_NBUFS 20
#define WINSND_NSAMPLES 160
#define WINSND_MINIMUMBUFFER 5

static MSFilter *ms_winsnd_read_new(MSSndCard *card);
static MSFilter *ms_winsnd_write_new(MSSndCard *card);

typedef struct WinSndCard{
	int in_devid;
	int out_devid;
	int removed;
}WinSndCard;

static void winsndcard_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent){
	WinSndCard *d=(WinSndCard*)card->data;

	UINT uMixerID;
	DWORD dwMixerHandle;
	MIXERLINE MixerLine;
	MIXERLINE Line;
	UINT uLineIndex;

	MIXERLINECONTROLS mlc = {0};
	MIXERCONTROL mc = {0};
	MIXERCONTROLDETAILS mcd = {0};
	MIXERCONTROLDETAILS_UNSIGNED mcdu = {0};

	MMRESULT mr = MMSYSERR_NOERROR;
	DWORD dwVolume = ((0xFFFF) * percent) / 100;
	
	WORD wLeftVol, wRightVol;
	DWORD dwNewVol;
	wLeftVol = LOWORD(dwVolume); // get higher WORD
	wRightVol = LOWORD(dwVolume); // get lower WORD

	dwNewVol = MAKELONG(wLeftVol, wRightVol);

	switch(e){
		case MS_SND_CARD_PLAYBACK:
		case MS_SND_CARD_MASTER:
			{
				mr = mixerGetID( (HMIXEROBJ)d->out_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
				if ( mr != MMSYSERR_NOERROR )
				{
					ms_error("winsndcard_set_level: mixerGetID failed. (0x%x)", mr);
					return;
				}
				mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_level: mixerOpen failed. (0x%x)", mr);
					return;
				}
				memset( &MixerLine, 0, sizeof(MIXERLINE) );
				MixerLine.cbStruct = sizeof(MIXERLINE);
				if (MS_SND_CARD_MASTER==e)
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
				else
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_level: mixerGetLineInfo failed. (0x%x)", mr);
					return;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				mlc.cbStruct = sizeof(MIXERLINECONTROLS);
				mlc.dwLineID = MixerLine.dwLineID;
				mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				mlc.cControls = 1;
				mlc.pamxctrl = &mc;
				mlc.cbmxctrl = sizeof(MIXERCONTROL);
				mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, 
					&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);


				mcdu.dwValue = 65535*percent/100; /* the volume is a number between 0 and 65535 */

				mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mcd.hwndOwner = 0;
				mcd.dwControlID = mc.dwControlID;
				mcd.paDetails = &mcdu;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mcd.cChannels = 1;
				mr = mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, 
					&mcd, MIXER_SETCONTROLDETAILSF_VALUE);

				if (mr != MMSYSERR_NOERROR)
				{
					ms_error("winsndcard_set_level: mixerSetControlDetails failed. (0x%x)", mr);
					return;
				}
			}
			break;
		case MS_SND_CARD_CAPTURE:
			mr = mixerGetID( (HMIXEROBJ)d->in_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
			if ( mr != MMSYSERR_NOERROR )
			{
				ms_error("winsndcard_set_level: mixerGetID failed. (0x%x)", mr);
				return;
			}
			mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_set_level: mixerGetLineInfo failed. (0x%x)", mr);
				return;
			}
			memset( &MixerLine, 0, sizeof(MIXERLINE) );
			MixerLine.cbStruct = sizeof(MIXERLINE);
			MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
			mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_set_level: mixerGetLineInfo failed. (0x%x)", mr);
				return;
			}
			/* ms_message("Name: %s\n", MixerLine.szName); */
			/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
			/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

			mlc.cbStruct = sizeof(MIXERLINECONTROLS);
			mlc.dwLineID = MixerLine.dwLineID;
			mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mlc.cControls = 1;
			mlc.pamxctrl = &mc;
			mlc.cbmxctrl = sizeof(MIXERCONTROL);
			mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, 
				&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

			if (mr == MMSYSERR_NOERROR)
			{
				mcdu.dwValue = 65535*percent/100; /* the volume is a number between 0 and 65535 */

				mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mcd.hwndOwner = 0;
				mcd.dwControlID = mc.dwControlID;
				mcd.paDetails = &mcdu;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mcd.cChannels = 1;
				mr = mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, 
					&mcd, MIXER_SETCONTROLDETAILSF_VALUE);

				if (mr == MMSYSERR_NOERROR)
				{
					return;
				}
				ms_error("winsndcard_set_level: mixerSetControlDetails failed. (0x%x)", mr);
				ms_warning("winsndcard_set_level: control the SRC_MICROPHONE instead");
			}
			else
			{
				ms_error("winsndcard_set_level: mixerGetLineControls failed. (0x%x)", mr);
				ms_warning("winsndcard_set_level: control the SRC_MICROPHONE instead");
			}

			/* In case capture doesn't work: use the SRC_MICROPHONE volume */

			for (uLineIndex = 0; uLineIndex < MixerLine.cConnections; uLineIndex++)
			{
				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_LINEID);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_level: mixerGetLineInfo failed. (0x%x)", mr);
					return;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_SOURCE);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_level: mixerGetLineInfo failed. (0x%x)", mr);
					return;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == Line.dwComponentType)  
				{
					LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
					MIXERLINECONTROLS mxlctrl = {sizeof(mxlctrl), Line.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 1, sizeof(MIXERCONTROL), pmxctrl};  
					if(!mixerGetLineControls((HMIXEROBJ)dwMixerHandle, &mxlctrl,  
						MIXER_GETLINECONTROLSF_ONEBYTYPE)){  
							DWORD cChannels = Line.cChannels;
							LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned;
							MIXERCONTROLDETAILS mxcd;
							if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)  
								cChannels = 1;  
							pUnsigned =  
								(LPMIXERCONTROLDETAILS_UNSIGNED)  
								malloc(cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));

							mxcd.cbStruct = sizeof(mxcd);
							mxcd.dwControlID = pmxctrl->dwControlID;
							mxcd.cChannels = cChannels;
							mxcd.hwndOwner = (HWND)0;
							mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mxcd.paDetails = (LPVOID) pUnsigned;

							mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
								MIXER_SETCONTROLDETAILSF_VALUE);  
							pUnsigned[0].dwValue = pUnsigned[cChannels - 1].dwValue
								=  pmxctrl->Bounds.dwMaximum*percent/100;
							mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
								MIXER_SETCONTROLDETAILSF_VALUE);  
							free(pmxctrl);  
							free(pUnsigned);  
					}  
					else  
						free(pmxctrl);  
				}
			}
			mixerClose( (HMIXER)dwMixerHandle );
			if (mr != MMSYSERR_NOERROR)
			{
				ms_error("winsndcard_set_level: mixerClose failed. (0x%x)", mr);
				return;
			}
			break;
		default:
			ms_warning("winsnd_card_set_level: unsupported command.");
	}
}

static int winsndcard_get_level(MSSndCard *card, MSSndCardMixerElem e){
	WinSndCard *d=(WinSndCard*)card->data;

	UINT uMixerID;
	DWORD dwMixerHandle;
	MIXERLINE MixerLine;
	MIXERLINE Line;
	UINT uLineIndex;

	MIXERLINECONTROLS mlc = {0};
	MIXERCONTROL mc = {0};
	MIXERCONTROLDETAILS mcd = {0};
	MIXERCONTROLDETAILS_UNSIGNED mcdu = {0};

	MMRESULT mr = MMSYSERR_NOERROR;
	int percent;

	switch(e){
		case MS_SND_CARD_MASTER:
		case MS_SND_CARD_PLAYBACK:
			{
				mr = mixerGetID( (HMIXEROBJ)d->out_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
				if ( mr != MMSYSERR_NOERROR )
				{
					ms_error("winsndcard_get_level: mixerGetID failed. (0x%x)", mr);
					return -1;
				}
				mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_get_level: mixerOpen failed. (0x%x)", mr);
					return -1;
				}
				memset( &MixerLine, 0, sizeof(MIXERLINE) );
				MixerLine.cbStruct = sizeof(MIXERLINE);
				if (MS_SND_CARD_MASTER==e)
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
				else
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_get_level: mixerGetLineInfo failed. (0x%x)", mr);
					return -1;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				mlc.cbStruct = sizeof(MIXERLINECONTROLS);
				mlc.dwLineID = MixerLine.dwLineID;
				mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				mlc.cControls = 1;
				mlc.pamxctrl = &mc;
				mlc.cbmxctrl = sizeof(MIXERCONTROL);
				mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, 
					&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);
				if (mr != MMSYSERR_NOERROR)
				{
					ms_error("winsndcard_get_level: mixerGetLineControls failed. (0x%x)", mr);
					return -1;
				}

				mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mcd.hwndOwner = 0;
				mcd.dwControlID = mc.dwControlID;
				mcd.paDetails = &mcdu;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mcd.cChannels = 1;
				mr = mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mcd,  
					MIXER_SETCONTROLDETAILSF_VALUE);  
				percent = (mcdu.dwValue *100) / (mc.Bounds.dwMaximum);

				if (mr != MMSYSERR_NOERROR)
				{
					ms_error("winsndcard_get_level: mixerGetControlDetails failed. (0x%x)", mr);
					return -1;
				}
				return percent;
			}
			break;
		case MS_SND_CARD_CAPTURE:
			mr = mixerGetID( (HMIXEROBJ)d->in_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
			if ( mr != MMSYSERR_NOERROR )
			{
				ms_error("winsndcard_get_level: mixerGetID failed. (0x%x)", mr);
				return -1;
			}
			mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_get_level: mixerOpen failed. (0x%x)", mr);
				return -1;
			}
			memset( &MixerLine, 0, sizeof(MIXERLINE) );
			MixerLine.cbStruct = sizeof(MIXERLINE);
			MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
			mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_get_level: mixerGetLineInfo failed. (0x%x)", mr);
				return -1;
			}

			mlc.cbStruct = sizeof(MIXERLINECONTROLS);
			mlc.dwLineID = MixerLine.dwLineID;
			mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
			mlc.cControls = 1;
			mlc.pamxctrl = &mc;
			mlc.cbmxctrl = sizeof(MIXERCONTROL);
			mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, 
				&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);
			if (mr == MMSYSERR_NOERROR)
			{
				mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mcd.hwndOwner = 0;
				mcd.dwControlID = mc.dwControlID;
				mcd.paDetails = &mcdu;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mcd.cChannels = 1;
				mr = mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mcd,  
					MIXER_SETCONTROLDETAILSF_VALUE);  
				percent = (mcdu.dwValue *100) / (mc.Bounds.dwMaximum);

				if (mr == MMSYSERR_NOERROR)
				{
					return percent;
				}
				ms_error("winsndcard_get_level: mixerGetControlDetails failed. (0x%x)", mr);
				ms_warning("winsndcard_get_level: control the SRC_MICROPHONE instead");
			}
			else
			{
				ms_error("winsndcard_get_level: mixerGetLineControls failed. (0x%x)", mr);
				ms_warning("winsndcard_get_level: control the SRC_MICROPHONE instead");
			}
	
			/* ms_message("Name: %s\n", MixerLine.szName); */
			/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
			/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

			for (uLineIndex = 0; uLineIndex < MixerLine.cConnections; uLineIndex++)
			{
				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_LINEID);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_get_level: mixerGetLineInfo failed. (0x%x)", mr);
					return -1;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_SOURCE);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_get_level: mixerGetLineInfo failed. (0x%x)", mr);
					return -1;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == Line.dwComponentType)  
				{
					LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
					MIXERLINECONTROLS mxlctrl = {sizeof(mxlctrl), Line.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 1, sizeof(MIXERCONTROL), pmxctrl};  
					if(!mixerGetLineControls((HMIXEROBJ)dwMixerHandle, &mxlctrl,  
						MIXER_GETLINECONTROLSF_ONEBYTYPE)){  
							DWORD cChannels = Line.cChannels;
							LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned;
							MIXERCONTROLDETAILS mxcd;
							if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)  
								cChannels = 1;  
							pUnsigned =  
								(LPMIXERCONTROLDETAILS_UNSIGNED)  
								malloc(cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));

							mxcd.cbStruct = sizeof(mxcd);
							mxcd.dwControlID = pmxctrl->dwControlID;
							mxcd.cChannels = cChannels;
							mxcd.hwndOwner = (HWND)0;
							mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mxcd.paDetails = (LPVOID) pUnsigned;

							mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
								MIXER_SETCONTROLDETAILSF_VALUE);  
							percent = (pUnsigned[0].dwValue *100) / (pmxctrl->Bounds.dwMaximum);
							free(pmxctrl);
							free(pUnsigned);
					}  
					else  
						free(pmxctrl);  
				}
			}
			mixerClose( (HMIXER)dwMixerHandle );
			if (mr != MMSYSERR_NOERROR)
			{
				ms_error("winsndcard_get_level: mixerClose failed. (0x%x)", mr);
				return -1;
			}
			return percent;
			break;
		default:
			ms_warning("winsndcard_get_level: unsupported command.");
			return -1;
	}
	return -1;
}

static void winsndcard_set_source(MSSndCard *card, MSSndCardCapture source){

	switch(source){
		case MS_SND_CARD_MIC:
			break;
		case MS_SND_CARD_LINE:
			break;
	}	
}

static int winsndcard_set_control(MSSndCard *card, MSSndCardControlElem e, int val){
	WinSndCard *d=(WinSndCard*)card->data;

	UINT uMixerID;
	DWORD dwMixerHandle;
	MIXERLINE MixerLine;
	MIXERLINE Line;
	UINT uLineIndex;

	MIXERLINECONTROLS mlc = {0};
	MIXERCONTROL mc = {0};
	MIXERCONTROLDETAILS mcd = {0};
	MIXERCONTROLDETAILS_BOOLEAN bMute;

	MMRESULT mr = MMSYSERR_NOERROR;

	switch(e){
		case MS_SND_CARD_CAPTURE_MUTE:

			bMute.fValue = (val>0);

			mr = mixerGetID( (HMIXEROBJ)d->in_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
			if ( mr != MMSYSERR_NOERROR )
			{
				ms_error("winsndcard_set_control: mixerGetID failed. (0x%x)", mr);
				return -1;
			}
			mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_set_control: mixerOpen failed. (0x%x)", mr);
				return -1;
			}
			memset( &MixerLine, 0, sizeof(MIXERLINE) );
			MixerLine.cbStruct = sizeof(MIXERLINE);
			MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
			mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
			if ( mr != MMSYSERR_NOERROR )
			{
				mixerClose( (HMIXER)dwMixerHandle );
				ms_error("winsndcard_set_control: mixerGetLineInfo failed. (0x%x)", mr);
				return -1;
			}
			/* ms_message("Name: %s\n", MixerLine.szName); */
			/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
			/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

			for (uLineIndex = 0; uLineIndex < MixerLine.cConnections; uLineIndex++)
			{
				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_LINEID);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_control: mixerGetLineInfo failed. (0x%x)", mr);
					return -1;
				}
				
				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				memset( &Line, 0, sizeof(MIXERLINE) );
				Line.cbStruct = sizeof(MIXERLINE);
				Line.dwDestination = MixerLine.dwDestination;
				Line.dwSource = uLineIndex;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &Line, MIXER_GETLINEINFOF_SOURCE);
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_control: mixerGetLineInfo failed. (0x%x)", mr);
					return -1;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("LineID: %d\n", MixerLine.dwLineID); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == Line.dwComponentType)  
				{
					/* unmute */
					/* Find a mute control, if any, of the microphone line  */

					LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
					MIXERLINECONTROLS mxlctrl = {sizeof(mxlctrl), Line.dwLineID, MIXERCONTROL_CONTROLTYPE_MUTE, 1, sizeof(MIXERCONTROL), pmxctrl};  
					if(!mixerGetLineControls((HMIXEROBJ)dwMixerHandle, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE)){  
						DWORD cChannels = Line.cChannels;
						LPMIXERCONTROLDETAILS_BOOLEAN pbool;
						MIXERCONTROLDETAILS mxcd;

						if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)  
							cChannels = 1;  
						pbool = (LPMIXERCONTROLDETAILS_BOOLEAN) malloc(cChannels * sizeof(
							MIXERCONTROLDETAILS_BOOLEAN));

						mxcd.cbStruct = sizeof(mxcd);
						mxcd.dwControlID = pmxctrl->dwControlID;
						mxcd.cChannels = cChannels;
						mxcd.hwndOwner = (HWND)0;
						mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
						mxcd.paDetails = (LPVOID) pbool;

						mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
							MIXER_SETCONTROLDETAILSF_VALUE);  
						/* Unmute the microphone line (for both channels) */
						pbool[0].fValue = pbool[cChannels - 1].fValue = val; /* 0 -> unmute; */
						mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
							MIXER_SETCONTROLDETAILSF_VALUE);  
						free(pmxctrl);  
						free(pbool);  
					}  
					else  
						free(pmxctrl);  
				}
			}
			mixerClose( (HMIXER)dwMixerHandle );
			if (mr != MMSYSERR_NOERROR)
			{
				ms_error("winsndcard_set_control: mixerClose failed. (0x%x)", mr);
				return -1;
			}
			return 0;

		case MS_SND_CARD_MASTER_MUTE:
		case MS_SND_CARD_PLAYBACK_MUTE:
			{
				bMute.fValue = (val>0);

				mr = mixerGetID( (HMIXEROBJ)d->out_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
				if ( mr != MMSYSERR_NOERROR )
				{
					ms_error("winsndcard_set_control: mixerGetID failed. (0x%x)", mr);
					return -1;
				}
				mr = mixerOpen( (LPHMIXER)&dwMixerHandle, uMixerID, 0L, 0L, 0L );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_control: mixerOpen failed. (0x%x)", mr);
					return -1;
				}
				memset( &MixerLine, 0, sizeof(MIXERLINE) );
				MixerLine.cbStruct = sizeof(MIXERLINE);
				if (MS_SND_CARD_MASTER_MUTE==e)
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
				else
					MixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
				mr = mixerGetLineInfo( (HMIXEROBJ)dwMixerHandle, &MixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE );
				if ( mr != MMSYSERR_NOERROR )
				{
					mixerClose( (HMIXER)dwMixerHandle );
					ms_error("winsndcard_set_control: mixerSetControlDetails failed. (0x%x)", mr);
					return -1;
				}

				/* ms_message("Name: %s\n", MixerLine.szName); */
				/* ms_message("Source Line: %d\n", MixerLine.dwSource); */
				/* ms_message("ComponentType: %d\n", MixerLine.dwComponentType); */

				mlc.cbStruct = sizeof(MIXERLINECONTROLS);
				mlc.dwLineID = MixerLine.dwLineID;
				mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE; //MIXERCONTROL_CONTROLTYPE_VOLUME;
				mlc.cControls = 1;
				mlc.pamxctrl = &mc;
				mlc.cbmxctrl = sizeof(MIXERCONTROL);
				mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, 
					&mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

				mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mcd.hwndOwner = 0;
				mcd.dwControlID = mc.dwControlID;
				mcd.paDetails = &bMute;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
				mcd.cChannels = 1;
				mr = mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, 
					&mcd, MIXER_SETCONTROLDETAILSF_VALUE);

				if (mr != MMSYSERR_NOERROR)
				{
					ms_error("winsndcard_set_control: mixerSetControlDetails failed. (0x%x)", mr);
					return -1;
				}
				return 0;
			}
			break;
		default:
			ms_warning("winsndcard_set_control: unsupported command.");
	}
	return -1;
}

static int winsndcard_get_control(MSSndCard *card, MSSndCardControlElem e){
	WinSndCard *d=(WinSndCard*)card->data;
	return -1;
}

static void winsndcard_init(MSSndCard *card){
	WinSndCard *c=(WinSndCard *)ms_new(WinSndCard,1);
	c->removed=0;
	card->data=c;
}

static void winsndcard_uninit(MSSndCard *card){
	ms_free(card->data);
}

static void winsndcard_detect(MSSndCardManager *m);
static  MSSndCard *winsndcard_dup(MSSndCard *obj);
static void winsndcard_unload(MSSndCardManager *m);

MSSndCardDesc winsnd_card_desc={
	"MME",
	winsndcard_detect,
	winsndcard_init,
	winsndcard_set_level,
	winsndcard_get_level,
	winsndcard_set_source,
	winsndcard_set_control,
	winsndcard_get_control,
	ms_winsnd_read_new,
	ms_winsnd_write_new,
	winsndcard_uninit,
	winsndcard_dup,
	winsndcard_unload
};

static  MSSndCard *winsndcard_dup(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&winsnd_card_desc);
	card->name=ms_strdup(obj->name);
	card->data=ms_new(WinSndCard,1);
	memcpy(card->data,obj->data,sizeof(WinSndCard));
	return card;
}

static MSSndCard *winsndcard_new(const char *name, int in_dev, int out_dev, unsigned cap){
	MSSndCard *card=ms_snd_card_new(&winsnd_card_desc);
	WinSndCard *d=(WinSndCard*)card->data;
	card->name=ms_strdup(name);
	d->in_devid=in_dev;
	d->out_devid=out_dev;
	card->capabilities=cap;
	return card;
}

static void add_or_update_card(MSSndCardManager *m, const char *name, int indev, int outdev, unsigned int capability){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsnd_card_desc.driver_type)==0
			&& strcmp(card->name,name)==0){
			/*update already entered card */
			WinSndCard *d=(WinSndCard*)card->data;
			card->capabilities|=capability;
			if (indev!=-1) 
				d->in_devid=indev;
			if (outdev!=-1)
				d->out_devid=outdev;
			d->removed=0;
			return;
		}
	}
	/* add this new card:*/
	ms_snd_card_manager_add_card(m,winsndcard_new(name,indev,outdev,capability));
}


static void _winsndcard_detect(MSSndCardManager *m){
	MMRESULT mr = NOERROR;
	unsigned int nOutDevices = waveOutGetNumDevs ();
	unsigned int nInDevices = waveInGetNumDevs ();
	unsigned int item;

	if (nOutDevices>nInDevices)
		nInDevices = nOutDevices;

	for (item = 0; item < nInDevices; item++){

		WAVEINCAPS incaps;
		WAVEOUTCAPS outcaps;
		mr = waveInGetDevCaps (item, &incaps, sizeof (WAVEINCAPS));
		if (mr == MMSYSERR_NOERROR)
		{
#if defined(_WIN32_WCE)
			char card[256];
			snprintf(card, sizeof(card), "Input card %i", item);
			add_or_update_card(m,card,item,-1,MS_SND_CARD_CAP_CAPTURE);
			/* _tprintf(L"new card: %s", incaps.szPname); */
#else
			char szName[256];
			WideCharToMultiByte(CP_UTF8,0,incaps.szPname,-1,szName,256,0,0);
			add_or_update_card(m,szName,item,-1,MS_SND_CARD_CAP_CAPTURE);
#endif
		}
		mr = waveOutGetDevCaps (item, &outcaps, sizeof (WAVEOUTCAPS));
		if (mr == MMSYSERR_NOERROR)
		{
#if defined(_WIN32_WCE)
			char card[256];
			snprintf(card, sizeof(card), "Output card %i", item);
			add_or_update_card(m,card,-1,item,MS_SND_CARD_CAP_PLAYBACK);
			/* _tprintf(L"new card: %s", outcaps.szPname); */
#else
			char szName[256];
			WideCharToMultiByte(CP_UTF8,0,outcaps.szPname,-1,szName,256,0,0);
			add_or_update_card(m,szName,-1,item,MS_SND_CARD_CAP_PLAYBACK);
#endif
		}
	}
}

static void deactivate_removed_cards(MSSndCardManager *m){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsnd_card_desc.driver_type)==0){
			/*mark all cards as potentially removed, detect will check them immediately after */
			WinSndCard *d=(WinSndCard*)card->data;
			if (d->removed)	card->capabilities=0;
		}
	}
}

static void mark_as_removed(MSSndCardManager *m){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsnd_card_desc.driver_type)==0){
			/*mark all cards as potentially removed, detect will check them immediately after */
			WinSndCard *d=(WinSndCard*)card->data;
			d->removed=1;
		}
	}
}

static ms_thread_t poller_thread=NULL;
static bool_t poller_running=TRUE;

static void * new_device_polling_thread(void *ignore){
	MSSndCardManager *m;
	/*check for new devices every 5 seconds*/
	while(poller_running){
		ms_sleep(5);
		if (poller_running){
			m=ms_snd_card_manager_get();
			if(!m) break;
			mark_as_removed(m);
			_winsndcard_detect(m);
			deactivate_removed_cards(m);
		}
	}
	return NULL;
}

static void stop_poller(){
	poller_running=FALSE;
	ms_thread_join(poller_thread,NULL);
	poller_thread=NULL;
}

static void winsndcard_unload(MSSndCardManager *m){
	stop_poller();
}

static void winsndcard_detect(MSSndCardManager *m){
	_winsndcard_detect(m);
	if (poller_thread==NULL)
		ms_thread_create(&poller_thread,NULL,new_device_polling_thread,NULL);
}

typedef struct WinSnd{
	int dev_id;
	HWAVEIN indev;
	HWAVEOUT outdev;
	WAVEFORMATEX wfx;
	WAVEHDR hdrs_read[WINSND_NBUFS];
	WAVEHDR hdrs_write[WINSND_OUT_NBUFS];
	queue_t rq;
	ms_mutex_t mutex;
	uint64_t bytes_read;
	unsigned int nbufs_playing;
	bool_t running;

	int32_t stat_input;
	int32_t stat_output;
	int32_t stat_notplayed;

	int32_t stat_minimumbuffer;

	queue_t write_rq;
#ifndef DISABLE_SPEEX
	SpeexPreprocessState *pst;
	int pst_frame_size;
#endif
	int ready;
	int workaround; /* workaround for opticon audio device */
}WinSnd;

static void winsnd_apply_settings(WinSnd *d){
	d->wfx.nBlockAlign=d->wfx.nChannels*d->wfx.wBitsPerSample/8;
	d->wfx.nAvgBytesPerSec=d->wfx.nSamplesPerSec*d->wfx.nBlockAlign;
}


/*#define _TRUE_TIME*/
#ifndef _TRUE_TIME
static uint64_t winsnd_get_cur_time( void *data){
	WinSnd *d=(WinSnd*)data;
	uint64_t curtime=(d->bytes_read*1000)/(uint64_t)d->wfx.nAvgBytesPerSec;
	/* ms_debug("winsnd_get_cur_time: bytes_read=%u return %lu\n",d->bytes_read,(unsigned long)curtime); */
	return curtime;
}
#endif


static void winsnd_init(MSFilter *f){
	WinSnd *d=(WinSnd *)ms_new0(WinSnd,1);
	d->wfx.wFormatTag = WAVE_FORMAT_PCM;
	d->wfx.cbSize = 0;
	d->wfx.nAvgBytesPerSec = 16000;
	d->wfx.nBlockAlign = 2;
	d->wfx.nChannels = 1;
	d->wfx.nSamplesPerSec = 8000;
	d->wfx.wBitsPerSample = 16;
	qinit(&d->rq);
	qinit(&d->write_rq);
#ifndef DISABLE_SPEEX
	d->pst=NULL;
	d->pst_frame_size=0;
#endif
	d->ready=0;
	d->workaround=0;
	ms_mutex_init(&d->mutex,NULL);
	f->data=d;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=WINSND_MINIMUMBUFFER;
}

static void winsnd_uninit(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	flushq(&d->rq,0);
	flushq(&d->write_rq,0);
#ifndef DISABLE_SPEEX
	if (d->pst!=NULL)
		speex_preprocess_state_destroy(d->pst);
	d->pst=NULL;
	d->pst_frame_size=0;
#endif
	d->ready=0;
	d->workaround=0;
	ms_mutex_destroy(&d->mutex);
	ms_free(f->data);
}

static void add_input_buffer(WinSnd *d, WAVEHDR *hdr, int buflen){
	mblk_t *m=allocb(buflen,0);
	MMRESULT mr;
	memset(hdr,0,sizeof(*hdr));
	if (buflen==0) ms_error("add_input_buffer: buflen=0 !");
	hdr->lpData=(LPSTR)m->b_wptr;
	hdr->dwBufferLength=buflen;
	hdr->dwFlags = 0;
	hdr->dwUser = (DWORD)m;
	mr = waveInPrepareHeader (d->indev,hdr,sizeof(*hdr));
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInPrepareHeader() error");
		return ;
	}
	mr=waveInAddBuffer(d->indev,hdr,sizeof(*hdr));
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInAddBuffer() error");
		return ;
	}
}

static void CALLBACK 
read_callback (HWAVEIN waveindev, UINT uMsg, DWORD dwInstance, DWORD dwParam1,
			   DWORD dwParam2)
{
	WAVEHDR *wHdr=(WAVEHDR *) dwParam1;
	MSFilter *f=(MSFilter *)dwInstance;
	WinSnd *d=(WinSnd*)f->data;
	mblk_t *m;
	int bsize;
	switch (uMsg){
		case WIM_OPEN:
			ms_debug("read_callback : WIM_OPEN");
			break;
		case WIM_CLOSE:
			ms_debug("read_callback : WIM_CLOSE");
			break;
		case WIM_DATA:
			bsize=wHdr->dwBytesRecorded;
			if (bsize<=0) {
#if 0
				if (d->running==TRUE) /* avoid adding buffer back when calling waveInReset */
				{
					MMRESULT mr;
					mr=waveInAddBuffer(d->indev,wHdr,sizeof(*wHdr));
					if (mr != MMSYSERR_NOERROR){
						ms_error("waveInAddBuffer() error");
						return ;
					}
					ms_warning("read_callback : EMPTY DATA, WIM_DATA (%p,%i)",wHdr,bsize);
				}
				m=(mblk_t*)wHdr->dwUser;
				wHdr->dwUser=0;
				freemsg(m);
				return;
#endif
			}

			/* ms_warning("read_callback : WIM_DATA (%p,%i)",wHdr,bsize); */
			m=(mblk_t*)wHdr->dwUser;
			m->b_wptr+=bsize;
			wHdr->dwUser=0;
			ms_mutex_lock(&d->mutex);
			putq(&d->rq,m);
			ms_mutex_unlock(&d->mutex);
			d->bytes_read+=wHdr->dwBufferLength;
			d->stat_input++;
			d->stat_input++;
#ifdef WIN32_TIMERS
			if (f->ticker->TimeEvent!=NULL)
				SetEvent(f->ticker->TimeEvent);
#endif
			break;
	}
}


static void winsnd_read_preprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	MMRESULT mr;
	int i;
	int bsize;
	DWORD dwFlag;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=WINSND_MINIMUMBUFFER;

	winsnd_apply_settings(d);
	/* Init Microphone device */
	dwFlag = CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT;
	mr = waveInOpen (&d->indev, d->dev_id, &d->wfx,
		(DWORD) read_callback, (DWORD)f, dwFlag);
	if (mr != MMSYSERR_NOERROR)
	{
		ms_error("Failed to prepare windows sound device. (waveInOpen:0x%i)", mr);
		if (d->dev_id != WAVE_MAPPER)
			dwFlag = WAVE_MAPPED | CALLBACK_FUNCTION;
		mr = waveInOpen (&d->indev, d->dev_id, &d->wfx,
			(DWORD) read_callback, (DWORD)f, dwFlag);
	}
	if (mr != MMSYSERR_NOERROR)
	{
		ms_error("Failed to prepare windows sound device. (waveInOpen:0x%i)", mr);
		mr = waveInOpen (&d->indev, WAVE_MAPPER, &d->wfx,
			(DWORD) read_callback, (DWORD)f, CALLBACK_FUNCTION);
		if (mr != MMSYSERR_NOERROR)
		{
			d->indev=NULL;
			ms_error("Failed to prepare windows sound device. (waveInOpen:0x%i)", mr);
			return ;
		}
	}
#ifndef _TRUE_TIME
	ms_mutex_lock(&f->ticker->lock);
	ms_ticker_set_time_func(f->ticker,winsnd_get_cur_time,d);
	ms_mutex_unlock(&f->ticker->lock);
#endif

	bsize=WINSND_NSAMPLES*d->wfx.nAvgBytesPerSec/8000;
	ms_debug("Using input buffers of %i bytes",bsize);
	for(i=0;i<WINSND_NBUFS;++i){
		WAVEHDR *hdr=&d->hdrs_read[i];
		add_input_buffer(d,hdr,bsize);
	}
	d->running=TRUE;
	mr=waveInStart(d->indev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInStart() error");
#ifndef _TRUE_TIME
		ms_mutex_lock(&f->ticker->lock);
		ms_ticker_set_time_func(f->ticker,NULL,NULL);
		ms_mutex_unlock(&f->ticker->lock);
#endif
		return ;
	}
}

static void winsnd_read_postprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	MMRESULT mr;
	int i;
#ifndef _TRUE_TIME
	ms_mutex_lock(&f->ticker->lock);
	ms_ticker_set_time_func(f->ticker,NULL,NULL);
	ms_mutex_unlock(&f->ticker->lock);
#endif
	d->running=FALSE;
	mr=waveInStop(d->indev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInStop() error");
		return ;
	}
	mr=waveInReset(d->indev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInReset() error");
		return ;
	}
	for(i=0;i<WINSND_NBUFS;++i){
		WAVEHDR *hdr=&d->hdrs_read[i];
		if (hdr->dwFlags & WHDR_PREPARED)
		{
			mr = waveInUnprepareHeader(d->indev,hdr,sizeof (*hdr));
			if (mr != MMSYSERR_NOERROR){
				ms_error("waveInUnPrepareHeader() error");
			}
		}
	}
	mr = waveInClose(d->indev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveInClose() error");
		return ;
	}

	ms_message("Shutting down sound device (playing: %i) (input-output: %i) (notplayed: %i)", d->nbufs_playing, d->stat_input - d->stat_output, d->stat_notplayed);
	flushq(&d->rq,0);
}

static void winsnd_read_process(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	mblk_t *m;
	int i;
	ms_mutex_lock(&d->mutex);
	while((m=getq(&d->rq))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
	ms_mutex_unlock(&d->mutex);
	for(i=0;i<WINSND_NBUFS;++i){
		WAVEHDR *hdr=&d->hdrs_read[i];
		if (hdr->dwUser==0) {
			MMRESULT mr;
			mr=waveInUnprepareHeader(d->indev,hdr,sizeof(*hdr));
			if (mr!=MMSYSERR_NOERROR)
				ms_warning("winsnd_read_process: Fail to unprepare header!");
			add_input_buffer(d,hdr,hdr->dwBufferLength);
		}
	}
}

static void CALLBACK
write_callback(HWAVEOUT outdev, UINT uMsg, DWORD dwInstance,
			   DWORD dwParam1, DWORD dwParam2)
{
	WAVEHDR *hdr=(WAVEHDR *) dwParam1;
	WinSnd *d=(WinSnd*)dwInstance;

	switch (uMsg){
		case WOM_OPEN:
			break;
		case WOM_CLOSE:
		case WOM_DONE:
			if (hdr){
				d->nbufs_playing--;
			}
			if (d->stat_output==0)
			{
				d->stat_input=1; /* reset */
				d->stat_notplayed=0;
			}
			d->stat_output++;
			break;
	}
}

static void winsnd_write_preprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	MMRESULT mr;
	DWORD dwFlag;
	int i;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;
	d->stat_minimumbuffer=WINSND_MINIMUMBUFFER;

	winsnd_apply_settings(d);
	/* Init Microphone device */
	dwFlag = CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT;
	mr = waveOutOpen (&d->outdev, d->dev_id, &d->wfx,
		(DWORD) write_callback, (DWORD)d, dwFlag);
	if (mr != MMSYSERR_NOERROR)
	{
		ms_error("Failed to open windows sound device %i. (waveOutOpen:0x%i)",d->dev_id, mr);
		if (d->dev_id != WAVE_MAPPER)
			dwFlag = WAVE_MAPPED | CALLBACK_FUNCTION;
		mr = waveOutOpen (&d->outdev, d->dev_id, &d->wfx,
			(DWORD) write_callback, (DWORD)d, dwFlag);
	}
	if (mr != MMSYSERR_NOERROR)
	{
		ms_error("Failed to open windows sound device %i. (waveOutOpen:0x%i)",d->dev_id, mr);
		mr = waveOutOpen (&d->outdev, WAVE_MAPPER, &d->wfx,
			(DWORD) write_callback, (DWORD)d, CALLBACK_FUNCTION);
		if (mr != MMSYSERR_NOERROR)
		{
			ms_error("Failed to open windows sound device %i. (waveOutOpen:0x%i)",d->dev_id, mr);
			d->outdev=NULL;
			return ;
		}
	}
	for(i=0;i<WINSND_OUT_NBUFS;++i){
		WAVEHDR *hdr=&d->hdrs_write[i];
		hdr->dwFlags=0;
		hdr->dwUser=0;
	}
}

static void winsnd_write_postprocess(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	MMRESULT mr;
	int i;
	if (d->outdev==NULL) return;
	mr=waveOutReset(d->outdev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveOutReset() error");
		return ;
	}
	for(i=0;i<WINSND_OUT_NBUFS;++i){
		WAVEHDR *hdr=&d->hdrs_write[i];
		mblk_t *old;
		if (hdr->dwFlags & WHDR_DONE){
			mr=waveOutUnprepareHeader(d->outdev,hdr,sizeof(*hdr));
			if (mr != MMSYSERR_NOERROR){
				ms_error("waveOutUnprepareHeader error");
			}
			old=(mblk_t*)hdr->dwUser;
			if (old) freemsg(old);
			hdr->dwUser=0;
		}
	}
	mr=waveOutClose(d->outdev);
	if (mr != MMSYSERR_NOERROR){
		ms_error("waveOutClose() error");
		return ;
	}
	ms_message("Shutting down sound device (playing: %i) (d->write_rq.q_mcount=%i) (input-output: %i) (notplayed: %i)", d->nbufs_playing, d->write_rq.q_mcount, d->stat_input - d->stat_output, d->stat_notplayed);
	flushq(&d->write_rq,0);
	d->ready=0;
	d->workaround=0;

#ifndef DISABLE_SPEEX
	if (d->pst!=NULL)
		speex_preprocess_state_destroy(d->pst);
	d->pst=NULL;
	d->pst_frame_size=0;
#endif
}

static void winsnd_write_process(MSFilter *f){
	WinSnd *d=(WinSnd*)f->data;
	mblk_t *m,*old;
	MMRESULT mr;
	int i;
	int discarded=0;
	int possible_size=0;

	if (d->outdev==NULL) {
		ms_queue_flush(f->inputs[0]);
		return;
	}

	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		possible_size = msgdsize(m);
#ifndef DISABLE_SPEEX
		if (d->pst_frame_size==0)
		{
			d->pst_frame_size=possible_size;

			d->pst = speex_preprocess_state_init(d->pst_frame_size/2, d->wfx.nSamplesPerSec);
			if (d->pst!=NULL) {
				float f;
				i=1;
				speex_preprocess_ctl(d->pst, SPEEX_PREPROCESS_SET_VAD, &i);
				i=0;
				speex_preprocess_ctl(d->pst, SPEEX_PREPROCESS_SET_DENOISE, &i);
				i=0;
				speex_preprocess_ctl(d->pst, SPEEX_PREPROCESS_SET_AGC, &i);
				f=8000;
				speex_preprocess_ctl(d->pst, SPEEX_PREPROCESS_SET_AGC_LEVEL, &f);
				i=0;
				speex_preprocess_ctl(d->pst, SPEEX_PREPROCESS_SET_DEREVERB, &i);
			}
		}
#endif

		putq(&d->write_rq,m);
	}

#ifdef AMD_HACK
	/* too many sound card are crappy on windows... */
	d->stat_minimumbuffer=15;
	if (d->wfx.nSamplesPerSec>=32000) /* better results for high rates */
		d->stat_minimumbuffer=8;
#endif

	if (d->wfx.nSamplesPerSec>=32000) /* better results for high rates */
	{
		if (d->nbufs_playing+d->write_rq.q_mcount<4)
		{
			d->ready=0;
		}
	}
	else
	{
		if (d->nbufs_playing+d->write_rq.q_mcount<7)
		{
			d->ready=0;
		}
	}
#if defined(WCE_OPTICON_WORKAROUND)
	if (d->workaround==0)
	{
		d->workaround=1;
		Sleep(WCE_OPTICON_WORKAROUND);
	}
#endif

	while((m=peekq(&d->write_rq))!=NULL){

#ifndef DISABLE_SPEEX
		int vad=1;
		if (d->pst!=NULL && msgdsize(m)==d->pst_frame_size && d->pst_frame_size<=4096)
		{
			char tmp[4096];
			memcpy(tmp, m->b_rptr, msgdsize(m));
			vad = speex_preprocess(d->pst, (short*)tmp, NULL);

			if (d->ready==0)
			{
				if (vad==0)
				{
					int missing;
					missing = 10 - d->write_rq.q_mcount - d->nbufs_playing;
					if (d->wfx.nSamplesPerSec>=32000) /* better results for high rates */
						missing = 6 - d->write_rq.q_mcount - d->nbufs_playing;

					ms_message("WINSND trouble: inserting %i silence", missing);
					while(missing>0)
					{
						old=dupb(m);
						putq(&d->write_rq,old);
						missing--;
					}
				}
				d->ready=1;
			}
		}
#else
		if (d->ready==0)
		{
			int missing;
			missing = 10 - d->write_rq.q_mcount - d->nbufs_playing;
			if (d->wfx.nSamplesPerSec>=32000) /* better results for high rates */
				missing = 6 - d->write_rq.q_mcount - d->nbufs_playing;
			ms_message("WINSND trouble: inserting %i silence", missing);
			while(missing>0)
			{
				old=dupb(m);
				putq(&d->write_rq,old);
				missing--;
			}
			d->ready=1;
		}
#endif

		for(i=0;i<d->stat_minimumbuffer;++i){
			WAVEHDR *hdr=&d->hdrs_write[i];
			if (hdr->dwFlags & WHDR_DONE){
				old=(mblk_t*)hdr->dwUser;
				mr=waveOutUnprepareHeader(d->outdev,hdr,sizeof(*hdr));
				if (mr != MMSYSERR_NOERROR){
					ms_error("waveOutUnprepareHeader error");
				}
				freemsg(old);
				hdr->dwUser=0;
			}
			if (hdr->dwUser==0){
				hdr->lpData=(LPSTR)m->b_rptr;
				hdr->dwBufferLength=msgdsize(m);
				hdr->dwFlags = 0;
				hdr->dwUser = (DWORD)m;
				mr = waveOutPrepareHeader(d->outdev,hdr,sizeof(*hdr));
				if (mr != MMSYSERR_NOERROR){
					ms_error("waveOutPrepareHeader() error");
					getq(&d->write_rq);
					freemsg(m);
					discarded++;
					d->stat_notplayed++;
					break;
				}
				mr=waveOutWrite(d->outdev,hdr,sizeof(*hdr));
				if (mr != MMSYSERR_NOERROR){
					ms_error("waveOutWrite() error");
					getq(&d->write_rq);
					freemsg(m);
					discarded++;
					d->stat_notplayed++;
					break;
				}else {
					getq(&d->write_rq);
					d->nbufs_playing++;
					/* ms_debug("waveOutWrite() done"); */
				}
				break;
			}
		}
		if (i==d->stat_minimumbuffer){
			/* ms_error("winsnd_write_process: All buffers are busy."); */
#ifndef DISABLE_SPEEX
			if (d->pst==NULL)
			{
				/* initial behavior (detection in process?) */
				getq(&d->write_rq);
				freemsg(m);
				discarded++;
				d->stat_notplayed++;
			}
			else
			{
				if (vad==0)
				{
					getq(&d->write_rq);
					freemsg(m);
					ms_message("WINSND trouble: silence removed");
					discarded++;
					d->stat_notplayed++;
				}
			}
#else
			getq(&d->write_rq);
			freemsg(m);
			discarded++;
			d->stat_notplayed++;
#endif

			break;
		}
	}
}

static int get_rate(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	*((int*)arg)=d->wfx.nSamplesPerSec;
	return 0;
}

static int set_rate(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	d->wfx.nSamplesPerSec=*((int*)arg);
	d->wfx.nSamplesPerSec=44100;
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	d->wfx.nChannels=*((int*)arg);
	return 0;
}

static int winsnd_get_stat_input(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;
	return d->stat_input;
}

static int winsnd_get_stat_ouptut(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;

	return d->stat_output;
}

static int winsnd_get_stat_discarded(MSFilter *f, void *arg){
	WinSnd *d=(WinSnd*)f->data;

	return d->stat_notplayed;
}

static MSFilterMethod winsnd_methods[]={
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate	},
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	MS_FILTER_GET_STAT_INPUT, winsnd_get_stat_input },
	{	MS_FILTER_GET_STAT_OUTPUT, winsnd_get_stat_ouptut },
	{	MS_FILTER_GET_STAT_DISCARDED, winsnd_get_stat_discarded },
	{	0				, NULL		}
};

MSFilterDesc winsnd_read_desc={
	MS_WINSND_READ_ID,
	"MMERead",
	"MME capture filter for Windows",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	winsnd_init,
	winsnd_read_preprocess,
	winsnd_read_process,
	winsnd_read_postprocess,
	winsnd_uninit,
	winsnd_methods
};


MSFilterDesc winsnd_write_desc={
	MS_WINSND_WRITE_ID,
	"MMEWrite",
	"MME playback filter for Windows",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	winsnd_init,
	winsnd_write_preprocess,
	winsnd_write_process,
	winsnd_write_postprocess,
	winsnd_uninit,
	winsnd_methods
};

MSFilter *ms_winsnd_read_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&winsnd_read_desc);
	WinSndCard *wc=(WinSndCard*)card->data;
	WinSnd *d=(WinSnd*)f->data;
	d->dev_id=wc->in_devid;
	return f;
}


MSFilter *ms_winsnd_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&winsnd_write_desc);
	WinSndCard *wc=(WinSndCard*)card->data;
	WinSnd *d=(WinSnd*)f->data;
	d->dev_id=wc->out_devid;
	return f;
}

MS_FILTER_DESC_EXPORT(winsnd_read_desc)
MS_FILTER_DESC_EXPORT(winsnd_write_desc)
