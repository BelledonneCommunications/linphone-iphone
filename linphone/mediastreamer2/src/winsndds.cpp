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

#ifdef __DIRECTSOUND_ENABLED__

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"

#define UNICODE

#include <mmsystem.h>
#ifdef _MSC_VER
#include <mmreg.h>
#endif
#include <msacm.h>

#include <dsound.h>
#include <dsconf.h>

const GUID GUID_DSCFX_MS_AEC = {0xcdebb919, 0x379a, 0x488a, {0x87, 0x65, 0xf5, 0x3c, 0xfd, 0x36, 0xde, 0x40}};
const GUID GUID_DSCFX_CLASS_AEC = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}}; 

const GUID CLSID_DirectSoundPrivate= { 0x11ab3ec0, 0x25ec, 0x11d1, {0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca}};
const GUID DSPROPSETID_DirectSoundDevice = {0x84624f82, 0x25ec, 0x11d1, {0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca}};

#define WINSNDDS_MINIMUMBUFFER 5

static MSFilter *ms_winsndds_read_new(MSSndCard *card);
static MSFilter *ms_winsndds_write_new(MSSndCard *card);

static HMODULE ms_lib_instance=NULL;
static HRESULT (WINAPI *ms_DllGetClassObject)(REFCLSID , REFIID , LPVOID *);

static HRESULT (WINAPI *ms_DirectSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
static HRESULT (WINAPI *ms_DirectSoundEnumerate)(LPDSENUMCALLBACKW, LPVOID);

static HRESULT (WINAPI *ms_DirectSoundCaptureCreate)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN);
static HRESULT (WINAPI *ms_DirectSoundCaptureEnumerate)(LPDSENUMCALLBACKW, LPVOID);

static HRESULT (WINAPI *ms_DirectSoundFullDuplexCreate)(LPCGUID , LPCGUID ,
														LPCDSCBUFFERDESC , LPCDSBUFFERDESC , HWND ,
														DWORD , LPDIRECTSOUNDFULLDUPLEX* , LPDIRECTSOUNDCAPTUREBUFFER8 *,
														LPDIRECTSOUNDBUFFER8 *, LPUNKNOWN );

typedef struct WinSndDsCard{
	int in_devid;
	int out_devid;
	GUID in_guid;
	GUID out_guid;
	int removed;
}WinSndDsCard;

static BOOL GetWaveIdFromDSoundGUID( GUID i_sGUID, DWORD *dwWaveID)
{ 
    LPKSPROPERTYSET         pKsPropertySet = NULL; 
    LPCLASSFACTORY          pClassFactory  = NULL; 
    HRESULT                 hr;
	BOOL					retval = FALSE;

	PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA psDirectSoundDeviceDescription = NULL; 
	DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA sDirectSoundDeviceDescription;

	memset(&sDirectSoundDeviceDescription,0,sizeof(sDirectSoundDeviceDescription)); 

    hr = ms_DllGetClassObject (CLSID_DirectSoundPrivate, IID_IClassFactory, (LPVOID *)&pClassFactory );

    if(SUCCEEDED(hr)) 
    { 
        hr = pClassFactory->CreateInstance ( NULL, IID_IKsPropertySet, (LPVOID *)&pKsPropertySet ); 
    } 

    // Release the class factory 
    if(pClassFactory) 
    { 
        pClassFactory->Release(); 
    }

    if(SUCCEEDED(hr)) 
	{ 
		ULONG ulBytesReturned = 0;
		sDirectSoundDeviceDescription.DeviceId = i_sGUID; 

		// On the first call the final size is unknown so pass the size of the struct in order to receive
		// "Type" and "DataFlow" values, ulBytesReturned will be populated with bytes required for struct+strings.
        hr = pKsPropertySet->Get(DSPROPSETID_DirectSoundDevice, 
                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION, 
                NULL, 
                0, 
                &sDirectSoundDeviceDescription, 
                sizeof(sDirectSoundDeviceDescription), 
                &ulBytesReturned
            ); 

		if (ulBytesReturned)
		{
			// On the first call it notifies us of the required amount of memory in order to receive the strings.
			// Allocate the required memory, the strings will be pointed to the memory space directly after the struct.
			psDirectSoundDeviceDescription = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA)new BYTE[ulBytesReturned];
			*psDirectSoundDeviceDescription = sDirectSoundDeviceDescription;

			hr = pKsPropertySet->Get(DSPROPSETID_DirectSoundDevice, 
					DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION, 
					NULL, 
					0, 
					psDirectSoundDeviceDescription, 
					ulBytesReturned, 
					&ulBytesReturned
				); 

			*dwWaveID  = psDirectSoundDeviceDescription->WaveDeviceId;
			delete [] psDirectSoundDeviceDescription;
			retval = TRUE;
		}

		pKsPropertySet->Release(); 
	} 

	return retval; 
} 

static void winsnddscard_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent){
	WinSndDsCard *d=(WinSndDsCard*)card->data;
	DWORD waveout_devid = WAVE_MAPPER;
	DWORD wavein_devid = WAVE_MAPPER;

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

	GetWaveIdFromDSoundGUID(d->in_guid, &wavein_devid);
	GetWaveIdFromDSoundGUID(d->out_guid, &waveout_devid);

	switch(e){
		case MS_SND_CARD_PLAYBACK:
		case MS_SND_CARD_MASTER:
			{

				mr = mixerGetID( (HMIXEROBJ)waveout_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
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

				if (mr != MMSYSERR_NOERROR)
				{
					ms_error("winsndcard_set_level: mixerGetLineControls failed. (0x%x)", mr);
					return;
				}

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
			mr = mixerGetID( (HMIXEROBJ)wavein_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
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
					mr = mixerGetLineControls((HMIXEROBJ)dwMixerHandle, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE);
					if(!mr){
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

						mr = mixerGetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
							MIXER_SETCONTROLDETAILSF_VALUE);  
						if(!mr){
							pUnsigned[0].dwValue = pUnsigned[cChannels - 1].dwValue
								=  pmxctrl->Bounds.dwMaximum*percent/100;
							mr = mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, &mxcd,  
								MIXER_SETCONTROLDETAILSF_VALUE);  
							if(mr){
								ms_error("winsndcard_set_level: mixerSetControlDetails failed. (0x%x)", mr);
							}
						}
						else  
						{
							ms_error("winsndcard_set_level: mixerGetControlDetails failed. (0x%x)", mr);
						}
						free(pmxctrl);  
						free(pUnsigned);
					}  
					else  
					{
						ms_error("winsndcard_set_level: mixerGetLineControls failed. (0x%x)", mr);
						free(pmxctrl);
					}
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

static int winsnddscard_get_level(MSSndCard *card, MSSndCardMixerElem e){
	WinSndDsCard *d=(WinSndDsCard*)card->data;
	DWORD waveout_devid = WAVE_MAPPER;
	DWORD wavein_devid = WAVE_MAPPER;

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

	GetWaveIdFromDSoundGUID(d->in_guid, &wavein_devid);
	GetWaveIdFromDSoundGUID(d->out_guid, &waveout_devid);

	switch(e){
		case MS_SND_CARD_MASTER:
		case MS_SND_CARD_PLAYBACK:
			{
				mr = mixerGetID( (HMIXEROBJ)waveout_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
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
			mr = mixerGetID( (HMIXEROBJ)wavein_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
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

static void winsnddscard_set_source(MSSndCard *card, MSSndCardCapture source){

	switch(source){
		case MS_SND_CARD_MIC:
			break;
		case MS_SND_CARD_LINE:
			break;
	}	
}

static int winsnddscard_set_control(MSSndCard *card, MSSndCardControlElem e, int val){
	WinSndDsCard *d=(WinSndDsCard*)card->data;
	DWORD waveout_devid = WAVE_MAPPER;
	DWORD wavein_devid = WAVE_MAPPER;

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

	GetWaveIdFromDSoundGUID(d->in_guid, &wavein_devid);
	GetWaveIdFromDSoundGUID(d->out_guid, &waveout_devid);

	switch(e){
		case MS_SND_CARD_CAPTURE_MUTE:
			bMute.fValue = (val>0);

			mr = mixerGetID( (HMIXEROBJ)wavein_devid, &uMixerID, MIXER_OBJECTF_WAVEIN );
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

			mlc.cbStruct = sizeof(MIXERLINECONTROLS);
			mlc.dwLineID = MixerLine.dwLineID;
			mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE; //MIXERCONTROL_CONTROLTYPE_VOLUME;
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
				mcd.paDetails = &bMute;
				mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
				mcd.cChannels = 1;
				mr = mixerSetControlDetails((HMIXEROBJ)dwMixerHandle, 
					&mcd, MIXER_SETCONTROLDETAILSF_VALUE);

				if (mr == MMSYSERR_NOERROR)
				{
					return 0;
				}
				ms_error("winsndcard_set_control: mixerSetControlDetails failed. (0x%x)", mr);
				ms_warning("winsndcard_get_level: control the SRC_MICROPHONE instead");
			}
			else
			{
				ms_error("winsndcard_set_control: mixerGetLineControls failed. (0x%x)", mr);
				ms_warning("winsndcard_get_level: control the SRC_MICROPHONE instead");
			}


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

				mr = mixerGetID( (HMIXEROBJ)waveout_devid, &uMixerID, MIXER_OBJECTF_WAVEOUT );
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

static int winsnddscard_get_control(MSSndCard *card, MSSndCardControlElem e){
	WinSndDsCard *d=(WinSndDsCard*)card->data;
	return -1;
}

static void winsnddscard_init(MSSndCard *card){
	WinSndDsCard *c=(WinSndDsCard *)ms_new(WinSndDsCard,1);
	c->removed=0;
	card->data=c;
}

static void winsnddscard_uninit(MSSndCard *card){
	ms_free(card->data);
}

static void winsnddscard_detect(MSSndCardManager *m);
static  MSSndCard *winsnddscard_dup(MSSndCard *obj);
static void winsnddscard_unload(MSSndCardManager *m);

MSSndCardDesc winsndds_card_desc={
	"DirectSound",
	winsnddscard_detect,
	winsnddscard_init,
	winsnddscard_set_level,
	winsnddscard_get_level,
	winsnddscard_set_source,
	winsnddscard_set_control,
	NULL,
	ms_winsndds_read_new,
	ms_winsndds_write_new,
	winsnddscard_uninit,
	winsnddscard_dup,
	winsnddscard_unload
};

static  MSSndCard *winsnddscard_dup(MSSndCard *obj){
	MSSndCard *card=ms_snd_card_new(&winsndds_card_desc);
	card->name=ms_strdup(obj->name);
	card->data=ms_new(WinSndDsCard,1);
	memcpy(card->data,obj->data,sizeof(WinSndDsCard));
	return card;
}

static MSSndCard *winsnddscard_new(const char *name, LPGUID lpguid, int in_dev, int out_dev, unsigned cap){
	MSSndCard *card=ms_snd_card_new(&winsndds_card_desc);
	WinSndDsCard *d=(WinSndDsCard*)card->data;
	card->name=ms_strdup(name);
	d->in_devid=in_dev;
	d->out_devid=out_dev;
	card->capabilities=cap;
	memset(&d->out_guid, 0, sizeof(GUID));
	memset(&d->in_guid, 0, sizeof(GUID));
	if (out_dev!=-1)
	{
		if (lpguid!=NULL)
			memcpy(&d->out_guid, lpguid, sizeof(GUID));
	}
	else
	{
		if (lpguid!=NULL)
			memcpy(&d->in_guid, lpguid, sizeof(GUID));
	}
	return card;
}

static void add_or_update_card(MSSndCardManager *m, const char *name, LPGUID lpguid, int indev, int outdev, unsigned int capability){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsndds_card_desc.driver_type)==0
			&& strcmp(card->name,name)==0){
			/*update already entered card */
			WinSndDsCard *d=(WinSndDsCard*)card->data;
			card->capabilities|=capability;
			if (indev!=-1) 
				d->in_devid=indev;
			if (outdev!=-1)
				d->out_devid=outdev;

			if (outdev!=-1)
			{
				if (lpguid!=NULL)
					memcpy(&d->out_guid, lpguid, sizeof(GUID));
				else
					memset(&d->out_guid, 0, sizeof(GUID));
			}
			if (indev!=-1)
			{
				if (lpguid!=NULL)
					memcpy(&d->in_guid, lpguid, sizeof(GUID));
				else
					memset(&d->in_guid, 0, sizeof(GUID));
			}
			/* if (d->in_devid!=-1 && d->out_devid!=-1) */
			/*	ms_message("DS: new full duplex card %s", name); */
			d->removed=0;
			return;
		}
	}
	/* add this new card:*/
	ms_snd_card_manager_add_card(m,winsnddscard_new(name,lpguid, indev,outdev,capability));
}

static BOOL CALLBACK enumerate_capture_devices_callback(LPGUID lpGUID,
														LPCWSTR lpszDesc,
														LPCWSTR lpszDrvName,
														LPVOID lpContext )
{
	MSSndCardManager *m = (MSSndCardManager*)lpContext;
	static int dev_index=0;

	if ( lpGUID == NULL ) /* primary device */
	{
		char szName[256];
		wchar_t snd_card_name[256];
		swprintf(snd_card_name, 256, L"%s", lpszDesc);
		WideCharToMultiByte(CP_UTF8,0,snd_card_name,-1,szName,256,0,0);

		add_or_update_card(m,szName,lpGUID,dev_index,-1,MS_SND_CARD_CAP_CAPTURE);
		dev_index++;
	}
	else
	{
		char szName[256];
		wchar_t snd_card_name[256];
		swprintf(snd_card_name, 256, L"%s", lpszDesc);
		WideCharToMultiByte(CP_UTF8,0,snd_card_name,-1,szName,256,0,0);

		add_or_update_card(m,szName,lpGUID,dev_index,-1,MS_SND_CARD_CAP_CAPTURE);
		dev_index++;
	}

	return true;
}

static BOOL CALLBACK enumerate_playback_devices_callback(LPGUID lpGUID,
														 LPCWSTR lpszDesc,
														 LPCWSTR lpszDrvName,
														 LPVOID lpContext )
{
	MSSndCardManager *m = (MSSndCardManager*)lpContext;
	static int dev_index=0;

	if ( lpGUID == NULL ) /* primary device */
	{
		char szName[256];
		wchar_t snd_card_name[256];
		swprintf(snd_card_name, 256, L"%s", lpszDesc);
		WideCharToMultiByte(CP_UTF8,0,snd_card_name,-1,szName,256,0,0);

		add_or_update_card(m,szName,lpGUID,-1,dev_index,MS_SND_CARD_CAP_PLAYBACK);
		dev_index++;
	}
	else
	{
		char szName[256];
		wchar_t snd_card_name[256];
		swprintf(snd_card_name, 256, L"%s", lpszDesc);
		WideCharToMultiByte(CP_UTF8,0,snd_card_name,-1,szName,256,0,0);

		add_or_update_card(m,szName,lpGUID,-1,dev_index,MS_SND_CARD_CAP_PLAYBACK);
		dev_index++;
	}

	return true;
}

static void _winsnddscard_detect(MSSndCardManager *m){
	MMRESULT mr = NOERROR;

	if (ms_lib_instance==NULL)
	{
		ms_lib_instance = LoadLibrary("dsound.dll");
		if( ms_lib_instance == NULL )
		{
			/* error */
			ms_debug("winsnddscard_detect: no support for dsound (missing dsound.dll)\n");
			return;
		}

		ms_DllGetClassObject =(HRESULT (WINAPI *)(REFCLSID, REFIID , LPVOID *))
			GetProcAddress( ms_lib_instance, "DllGetClassObject" );

		ms_DirectSoundCreate =(HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN))
			GetProcAddress( ms_lib_instance, "DirectSoundCreate" );

		ms_DirectSoundEnumerate =(HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))
			GetProcAddress( ms_lib_instance, "DirectSoundEnumerateW" );

		ms_DirectSoundCaptureCreate =(HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN))
			GetProcAddress( ms_lib_instance, "DirectSoundCaptureCreate" );

		ms_DirectSoundCaptureEnumerate =(HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))
			GetProcAddress( ms_lib_instance, "DirectSoundCaptureEnumerateW" );

		ms_DirectSoundFullDuplexCreate =(HRESULT (WINAPI *)(LPCGUID , LPCGUID ,
			LPCDSCBUFFERDESC , LPCDSBUFFERDESC , HWND ,
			DWORD , LPDIRECTSOUNDFULLDUPLEX* , LPDIRECTSOUNDCAPTUREBUFFER8 *,
			LPDIRECTSOUNDBUFFER8 *, LPUNKNOWN))
			GetProcAddress( ms_lib_instance, "DirectSoundFullDuplexCreate" );

		if( ms_DllGetClassObject == NULL ||
			ms_DirectSoundCreate == NULL ||
			ms_DirectSoundEnumerate == NULL ||
			ms_DirectSoundCaptureEnumerate == NULL ||
			ms_DirectSoundCaptureCreate == NULL )
		{
			/* error */
			ms_debug("winsnddscard_detect: no support for dsound\n");
			return;
		}
	}

	ms_DirectSoundCaptureEnumerate( (LPDSENUMCALLBACK)enumerate_capture_devices_callback, (void *)m );
	ms_DirectSoundEnumerate( (LPDSENUMCALLBACK)enumerate_playback_devices_callback, (void *)m );
}

static void deactivate_removed_cards(MSSndCardManager *m){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsndds_card_desc.driver_type)==0){
			/*mark all cards as potentially removed, detect will check them immediately after */
			WinSndDsCard *d=(WinSndDsCard*)card->data;
			if (d->removed)	card->capabilities=0;
		}
	}
}

static void mark_as_removed(MSSndCardManager *m){
	MSSndCard *card;
	const MSList *elem=ms_snd_card_manager_get_list(m);
	for(;elem!=NULL;elem=elem->next){
		card=(MSSndCard*)elem->data;
		if (strcmp(card->desc->driver_type, winsndds_card_desc.driver_type)==0){
			/*mark all cards as potentially removed, detect will check them immediately after */
			WinSndDsCard *d=(WinSndDsCard*)card->data;
			d->removed=1;
		}
	}
}

static ms_thread_t poller_thread=NULL;
static bool_t poller_running=TRUE;

static void * new_device_polling_thread(void *ignore){
	MSSndCardManager *m;
	/*check for new devices every 2 seconds*/
	while(poller_running){
		ms_sleep(5);
		if (poller_running){
			m=ms_snd_card_manager_get();
			if(!m) break;
			mark_as_removed(m);
			_winsnddscard_detect(m);
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

static void winsnddscard_unload(MSSndCardManager *m){
	stop_poller();
}

static void winsnddscard_detect(MSSndCardManager *m){
	_winsnddscard_detect(m);
	if (poller_thread==NULL)
		ms_thread_create(&poller_thread,NULL,new_device_polling_thread,NULL);
}

typedef struct WinSndDs{
	int dev_id;
	GUID in_guid;
	GUID out_guid;

	ms_thread_t thread;
	ms_mutex_t thread_lock;
	ms_cond_t thread_cond;
	bool_t thread_running;

	MSBufferizer output_buff;
	LPDIRECTSOUNDFULLDUPLEX lpDirectSoundFullDuplex;
	LPDIRECTSOUND lpDirectSound;
	LPDIRECTSOUNDBUFFER  lpDirectSoundOutputBuffer;
	double               dsw_framesWritten;
	UINT                 writeOffset;      /* last read position */

	LPDIRECTSOUNDCAPTURE lpDirectSoundCapture;
	LPDIRECTSOUNDCAPTUREBUFFER  lpDirectSoundInputBuffer;
	UINT                 readOffset;      /* last read position */

	int              framesPerDSBuffer;

	WAVEFORMATEX wfx;
	queue_t rq;
	ms_mutex_t mutex;
	uint64_t bytes_read;
	unsigned int nbufs_playing;

	int32_t stat_input;
	int32_t stat_output;
	int32_t stat_notplayed;

}WinSndDs;

void *  
winsndds_read_thread(void *arg)
{
	WinSndDs *d=(WinSndDs*)arg;

	ms_mutex_lock(&d->thread_lock);
	ms_cond_signal(&d->thread_cond);
	ms_mutex_unlock(&d->thread_lock);

	while(d->thread_running)
	{
		HRESULT hr;
		DWORD capturePos;
		DWORD readPos;
		long filled = 0;
		long bytesFilled = 0;
		LPBYTE            lpInBuf1 = NULL;
		LPBYTE            lpInBuf2 = NULL;
		DWORD             dwInSize1 = 0;
		DWORD             dwInSize2 = 0;

		hr = IDirectSoundCaptureBuffer_GetCurrentPosition( d->lpDirectSoundInputBuffer,
			&capturePos, &readPos );
		if( hr != DS_OK )
		{
			continue;
		}

		filled = readPos - d->readOffset;
		if( filled < 0 ) filled += d->framesPerDSBuffer;
		bytesFilled = filled;

		hr = IDirectSoundCaptureBuffer_Lock ( d->lpDirectSoundInputBuffer,
			d->readOffset, bytesFilled,
			(void **) &lpInBuf1, &dwInSize1,
			(void **) &lpInBuf2, &dwInSize2, 0);
		if (hr != DS_OK)
		{
			Sleep(10);
			continue;
		}

		if (dwInSize1==0)
		{
			Sleep(10);
		}
		else if (dwInSize1>=bytesFilled)
		{
			mblk_t *m=allocb(bytesFilled,0);
			memcpy(m->b_rptr, lpInBuf1, bytesFilled);
			m->b_wptr+=bytesFilled;
			ms_mutex_lock(&d->mutex);
			putq(&d->rq,m);
			ms_mutex_unlock(&d->mutex);
			d->bytes_read+=bytesFilled;
			/* ms_message("bytesFilled=%i\n",bytesFilled); */
		}
		else
		{
			mblk_t *m=allocb(bytesFilled,0);
			memcpy(m->b_rptr, lpInBuf1, dwInSize1);
			memcpy(m->b_rptr+dwInSize1, lpInBuf2, dwInSize2);
			m->b_wptr+=bytesFilled;
			ms_mutex_lock(&d->mutex);
			putq(&d->rq,m);
			ms_mutex_unlock(&d->mutex);
			d->bytes_read+=bytesFilled;
			/* ms_message("bytesFilled=%i\n",bytesFilled); */
		}

		d->readOffset = (d->readOffset + bytesFilled) % d->framesPerDSBuffer;

		IDirectSoundCaptureBuffer_Unlock( d->lpDirectSoundInputBuffer,
			lpInBuf1, dwInSize1, lpInBuf2, dwInSize2);
	}

	ms_mutex_lock(&d->thread_lock);
	ms_cond_signal(&d->thread_cond);
	ms_mutex_unlock(&d->thread_lock);
	ms_thread_exit(NULL);
	return NULL;
}

static void winsndds_apply_settings(WinSndDs *d){
	d->wfx.nBlockAlign=d->wfx.nChannels*d->wfx.wBitsPerSample/8;
	d->wfx.nAvgBytesPerSec=d->wfx.nSamplesPerSec*d->wfx.nBlockAlign;
}

static uint64_t winsndds_get_cur_time( void *data){
	WinSndDs *d=(WinSndDs*)data;
	uint64_t curtime=(d->bytes_read*1000)/(uint64_t)d->wfx.nAvgBytesPerSec;
	return curtime;
}


static void winsndds_init(MSFilter *f){
	WinSndDs *d=(WinSndDs *)ms_new0(WinSndDs,1);
	d->wfx.wFormatTag = WAVE_FORMAT_PCM;
	d->wfx.cbSize = 0;
	d->wfx.nAvgBytesPerSec = 16000;
	d->wfx.nBlockAlign = 2;
	d->wfx.nChannels = 1;
	d->wfx.nSamplesPerSec = 8000;
	d->wfx.wBitsPerSample = 16;
	qinit(&d->rq);
	ms_mutex_init(&d->mutex,NULL);
	f->data=d;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;

	d->framesPerDSBuffer = 320 * (8000 / 1000);

	d->thread = NULL;
	ms_mutex_init(&d->thread_lock,NULL);
	ms_cond_init(&d->thread_cond,NULL);
	d->thread_running = FALSE;

	ms_bufferizer_init(&d->output_buff);
}

static void winsndds_uninit(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;

	d->thread = NULL;
	d->thread_running = FALSE;
	ms_cond_destroy(&d->thread_cond);
	ms_mutex_destroy(&d->thread_lock);
	ms_bufferizer_uninit(&d->output_buff);

	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(f->data);
}

static void winsndds_read_preprocess(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;
	DSCBUFFERDESC  captureDesc;
	HRESULT hr;

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;

	d->framesPerDSBuffer = d->wfx.nAvgBytesPerSec/4;
	winsndds_apply_settings(d);
	ms_message("full duplex and echo canceller! (%x)" ,d->lpDirectSound);
	ms_DirectSoundCaptureCreate( &d->in_guid, &d->lpDirectSoundCapture, NULL );

	ZeroMemory(&captureDesc, sizeof(DSCBUFFERDESC));
	captureDesc.dwSize = sizeof(DSCBUFFERDESC);
	captureDesc.dwFlags =  0;
	captureDesc.dwBufferBytes = d->framesPerDSBuffer;
	captureDesc.lpwfxFormat = &d->wfx;

	if ((hr = IDirectSoundCapture_CreateCaptureBuffer( d->lpDirectSoundCapture,
		&captureDesc, &d->lpDirectSoundInputBuffer, NULL)) != DS_OK)
	{
		return;
	}
	d->readOffset = 0;

	hr = IDirectSoundCaptureBuffer_Start( d->lpDirectSoundInputBuffer, DSCBSTART_LOOPING );

	ms_mutex_lock(&f->ticker->lock);
	ms_ticker_set_time_func(f->ticker,winsndds_get_cur_time,d);
	ms_mutex_unlock(&f->ticker->lock);

	d->thread_running=TRUE;
	ms_thread_create(&d->thread,NULL,winsndds_read_thread,d);
	ms_mutex_lock(&d->thread_lock);
	ms_cond_wait(&d->thread_cond,&d->thread_lock);
	ms_mutex_unlock(&d->thread_lock);

	return;
}

static void winsndds_read_postprocess(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;

	ms_mutex_lock(&d->thread_lock);
	d->thread_running=FALSE;
	ms_cond_wait(&d->thread_cond,&d->thread_lock);
	ms_mutex_unlock(&d->thread_lock);
	ms_thread_join(d->thread,NULL);

	ms_mutex_lock(&f->ticker->lock);
	ms_ticker_set_time_func(f->ticker,NULL,NULL);
	ms_mutex_unlock(&f->ticker->lock);

	if( d->lpDirectSoundInputBuffer )
	{
		IDirectSoundCaptureBuffer_Stop( d->lpDirectSoundInputBuffer );
		IDirectSoundCaptureBuffer_Release( d->lpDirectSoundInputBuffer );
		d->lpDirectSoundInputBuffer = NULL;
	}

	if( d->lpDirectSoundCapture )
	{
		IDirectSoundCapture_Release( d->lpDirectSoundCapture );
		d->lpDirectSoundCapture = NULL;
	}

	ms_message("Shutting down sound device (playing: %i) (input-output: %i) (notplayed: %i)", d->nbufs_playing, d->stat_input - d->stat_output, d->stat_notplayed);
	flushq(&d->rq,0);
}

static void winsndds_read_process(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;
	mblk_t *m;

	ms_mutex_lock(&d->mutex);
	while((m=getq(&d->rq))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
	ms_mutex_unlock(&d->mutex);
}

static void winsndds_write_preprocess(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;

	DWORD          dwDataLen;
	DWORD          playCursor;
	HWND           hWnd;
	HRESULT        hr;
	LPDIRECTSOUNDBUFFER pPrimaryBuffer;
	DSBUFFERDESC   primaryDesc;
	DSBUFFERDESC   secondaryDesc;
	unsigned char* pDSBuffData;
	DWORD outputBufferWriteOffsetBytes;

	MSList *filters=NULL;
	MSFilter *f_capture_filter=NULL;
	WinSndDs *d_capture_filter=NULL;

	filters=ms_filter_find_neighbours(f);
	if (filters!=NULL)
	{
		MSList *it;
		/* search for another winsndds filter */
		for(it=filters;it!=NULL;it=it->next)
		{
			f_capture_filter = (MSFilter*)it->data;
			if (f_capture_filter->desc->id == MS_WINSNDDS_READ_ID)
			{
				/* found */
				d_capture_filter=(WinSndDs*)f_capture_filter->data;
				break;
			}
			f_capture_filter=NULL;
		}
		ms_list_free(filters);
	}

	d->stat_input=0;
	d->stat_output=0;
	d->stat_notplayed=0;

	d->framesPerDSBuffer = d->wfx.nAvgBytesPerSec/4;
	winsndds_apply_settings(d);

	if (d_capture_filter!=NULL
		&& d_capture_filter->lpDirectSoundCapture!=NULL
		&& IsEqualIID(d_capture_filter->in_guid, d->in_guid))
	{
		DSCBUFFERDESC captureDesc;

		winsndds_read_postprocess(f_capture_filter);

		DSCEFFECTDESC dscfx[1];
		ZeroMemory( &dscfx[0], sizeof( DSCEFFECTDESC ) );
		dscfx[0].dwSize = sizeof( DSCEFFECTDESC );
		dscfx[0].dwFlags = DSCFX_LOCSOFTWARE ;
		dscfx[0].guidDSCFXClass = GUID_DSCFX_CLASS_AEC;
		dscfx[0].guidDSCFXInstance = GUID_DSCFX_MS_AEC;
		dscfx[0].dwReserved1 = 0;
		dscfx[0].dwReserved2 = 0;

		d_capture_filter->framesPerDSBuffer = d_capture_filter->wfx.nAvgBytesPerSec/4;
		winsndds_apply_settings(d_capture_filter);

		ZeroMemory(&captureDesc, sizeof(DSCBUFFERDESC));
		captureDesc.dwSize = sizeof(DSCBUFFERDESC);
		captureDesc.dwFlags =  DSCBCAPS_CTRLFX;
		captureDesc.dwBufferBytes = d_capture_filter->framesPerDSBuffer;
		captureDesc.lpwfxFormat = &d_capture_filter->wfx;
		captureDesc.dwFXCount = 1;
		captureDesc.lpDSCFXDesc = dscfx;

		ZeroMemory(&secondaryDesc, sizeof(DSBUFFERDESC));
		secondaryDesc.dwSize = sizeof(DSBUFFERDESC);
		secondaryDesc.dwFlags =  DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2
			| DSBCAPS_LOCSOFTWARE;
		secondaryDesc.dwBufferBytes = d->framesPerDSBuffer;
		secondaryDesc.lpwfxFormat = &d->wfx;

		hWnd = GetDesktopWindow();
		hr = ms_DirectSoundFullDuplexCreate(&d_capture_filter->in_guid,
			&d->out_guid,
			&captureDesc,
			&secondaryDesc,
			hWnd,
			DSSCL_NORMAL,
			&d->lpDirectSoundFullDuplex,
			(LPDIRECTSOUNDCAPTUREBUFFER8*)&d_capture_filter->lpDirectSoundInputBuffer,
			(LPDIRECTSOUNDBUFFER8*)&d->lpDirectSoundOutputBuffer,
			NULL);

		if (hr!=DS_OK)
		{
			ms_message("full duplex and echo canceller: disabled!");
			captureDesc.dwFlags =  0;
			captureDesc.dwFXCount = 0;
			captureDesc.lpDSCFXDesc = NULL;

			hr = ms_DirectSoundFullDuplexCreate(&d_capture_filter->in_guid,
				&d->out_guid,
				&captureDesc,
				&secondaryDesc,
				hWnd,
				DSSCL_NORMAL,
				&d->lpDirectSoundFullDuplex,
				(LPDIRECTSOUNDCAPTUREBUFFER8*)&d_capture_filter->lpDirectSoundInputBuffer,
				(LPDIRECTSOUNDBUFFER8*)&d->lpDirectSoundOutputBuffer,
				NULL);
		}
		if (hr!=DS_OK)
		{
			ms_message("full duplex and echo canceller: disabled!");
			return;
		}
		ms_message("full duplex and echo canceller: activated!");

		d_capture_filter->readOffset = 0;

		hr = IDirectSoundCaptureBuffer_Start( d_capture_filter->lpDirectSoundInputBuffer, DSCBSTART_LOOPING );

		ms_mutex_lock(&f->ticker->lock);
		ms_ticker_set_time_func(f_capture_filter->ticker,winsndds_get_cur_time,d_capture_filter);
		ms_mutex_unlock(&f->ticker->lock);

		d_capture_filter->thread_running=TRUE;
		ms_thread_create(&d_capture_filter->thread,NULL,winsndds_read_thread,d_capture_filter);
		ms_mutex_lock(&d_capture_filter->thread_lock);
		ms_cond_wait(&d_capture_filter->thread_cond,&d_capture_filter->thread_lock);
		ms_mutex_unlock(&d_capture_filter->thread_lock);
	}
	else
	{
		ms_DirectSoundCreate( &d->out_guid, &d->lpDirectSound, NULL );


		hWnd = GetDesktopWindow();
		if ((hr = IDirectSound_SetCooperativeLevel( d->lpDirectSound,
			hWnd, DSSCL_PRIORITY)) != DS_OK) //DSSCL_EXCLUSIVE)) != DS_OK)
		{
			return ;
		}

		ZeroMemory(&primaryDesc, sizeof(DSBUFFERDESC));
		primaryDesc.dwSize        = sizeof(DSBUFFERDESC);
		primaryDesc.dwFlags       = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_PRIMARYBUFFER;
		primaryDesc.dwBufferBytes = 0;
		primaryDesc.lpwfxFormat   = NULL;
		if ((hr = IDirectSound_CreateSoundBuffer( d->lpDirectSound,
			&primaryDesc, &pPrimaryBuffer, NULL)) != DS_OK)
		{
			return ;
		}

		if ((hr = IDirectSoundBuffer_SetFormat( pPrimaryBuffer, &d->wfx)) != DS_OK)
		{
			return ;
		}
		IDirectSoundBuffer_Release(pPrimaryBuffer);

		ZeroMemory(&secondaryDesc, sizeof(DSBUFFERDESC));
		secondaryDesc.dwSize = sizeof(DSBUFFERDESC);
		secondaryDesc.dwFlags =  DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		secondaryDesc.dwBufferBytes = d->framesPerDSBuffer;
		secondaryDesc.lpwfxFormat = &d->wfx;
		if ((hr = IDirectSound_CreateSoundBuffer( d->lpDirectSound,
			&secondaryDesc, &d->lpDirectSoundOutputBuffer, NULL)) != DS_OK)
		{
			return ;
		}
	}

	if ((hr = IDirectSoundBuffer_Lock( d->lpDirectSoundOutputBuffer, 0,
		d->framesPerDSBuffer,
		(LPVOID*)&pDSBuffData,
		&dwDataLen, NULL, 0, 0)) != DS_OK)
	{
		return ;
	}

	ZeroMemory(pDSBuffData, dwDataLen);
	if ((hr = IDirectSoundBuffer_Unlock( d->lpDirectSoundOutputBuffer,
		pDSBuffData, dwDataLen, NULL, 0)) != DS_OK)
	{
		return ;
	}

	hr = IDirectSoundBuffer_GetCurrentPosition( d->lpDirectSoundOutputBuffer,
		&playCursor, &outputBufferWriteOffsetBytes );
	if( hr != DS_OK )
	{
		return ;
	}

	hr = IDirectSoundBuffer_SetCurrentPosition( d->lpDirectSoundOutputBuffer, 0 );
	if( hr != DS_OK )
	{
		return ;
	}

	hr = IDirectSoundBuffer_Play( d->lpDirectSoundOutputBuffer, 0, 0, DSBPLAY_LOOPING);
	if( hr != DS_OK )
	{
		return ;
	}
	d->writeOffset=-1;

	return ;
}

static void winsndds_write_postprocess(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;

	if( d->lpDirectSoundOutputBuffer )
	{
		IDirectSoundBuffer_Stop( d->lpDirectSoundOutputBuffer );
		IDirectSoundBuffer_Release( d->lpDirectSoundOutputBuffer );
		d->lpDirectSoundOutputBuffer = NULL;
	}

	if( d->lpDirectSound )
	{
		IDirectSound_Release( d->lpDirectSound );
		d->lpDirectSound = NULL;
	}

	if( d->lpDirectSoundFullDuplex )
	{
		IDirectSoundFullDuplex_Release( d->lpDirectSoundFullDuplex );
		d->lpDirectSoundFullDuplex = NULL;
	}

	ms_message("Shutting down sound device (playing: %i) (input-output: %i) (notplayed: %i)", d->nbufs_playing, d->stat_input - d->stat_output, d->stat_notplayed);
	d->writeOffset=-1;
}

static void winsndds_write_process(MSFilter *f){
	WinSndDs *d=(WinSndDs*)f->data;
	int discarded=0;
	DWORD dwStatus;
	HRESULT hr;

	if (d->lpDirectSoundOutputBuffer==NULL) {
		ms_queue_flush(f->inputs[0]);
		return;
	}

	ms_bufferizer_put_from_queue(&d->output_buff,f->inputs[0]);

	if (d->writeOffset==-1)
	{
		if (ms_bufferizer_get_avail(&d->output_buff)>=d->framesPerDSBuffer)
		{
			DWORD playCursor;
			DWORD outputBufferWriteOffsetBytes;
			IDirectSoundBuffer_GetCurrentPosition( d->lpDirectSoundOutputBuffer,
				&playCursor, &outputBufferWriteOffsetBytes );
			d->writeOffset = outputBufferWriteOffsetBytes;
		}
		else
			return;
	}

	DWORD current_playOffset;
	long msize_max = 0;
	DWORD currentWriteOffset;
	IDirectSoundBuffer_GetCurrentPosition( d->lpDirectSoundOutputBuffer,
		&current_playOffset, &currentWriteOffset );

	msize_max = current_playOffset - currentWriteOffset;
	if( msize_max < 0 ) msize_max += d->framesPerDSBuffer;

	/* write from d->writeOffset up to current_playOffset */
	msize_max=current_playOffset-d->writeOffset;
	if( msize_max < 0 ) msize_max += d->framesPerDSBuffer;

	//ms_message("DS information: last_writeOffset=%i current_playOffset=%i current_writeOffset=%i max_writable=%i",
	//	d->writeOffset, current_playOffset, currentWriteOffset, msize_max);

	hr = IDirectSoundBuffer_GetStatus (d->lpDirectSoundOutputBuffer, &dwStatus);
	if (dwStatus & DSBSTATUS_BUFFERLOST) {
		hr = IDirectSoundBuffer_Restore (d->lpDirectSoundOutputBuffer);
		d->writeOffset = 0;
		ms_message("DSBSTATUS_BUFFERLOST: restoring buffer");
	}

	if (msize_max==0)
		return;
	int msize=d->framesPerDSBuffer/4;
	if (msize>msize_max)
		msize = msize_max;
	while (ms_bufferizer_get_avail(&d->output_buff)>=msize)
	{
		LPBYTE lpOutBuf1 = NULL;
		LPBYTE lpOutBuf2 = NULL;
		DWORD  dwOutSize1 = 0;
		DWORD  dwOutSize2 = 0;
		char input[15360];

		hr = IDirectSoundBuffer_Lock ( d->lpDirectSoundOutputBuffer,
			d->writeOffset, msize,
			(void **) &lpOutBuf1, &dwOutSize1,
			(void **) &lpOutBuf2, &dwOutSize2, 0); /* DSBLOCK_FROMWRITECURSOR); */
		if (hr != DS_OK)
		{
			ms_error("DirectSound IDirectSoundBuffer_Lock failed, hresult = 0x%x\n", hr);
			break;
		}

		if (dwOutSize1==0)
		{
			ms_error("no free room to play sample\n");
		}
		else if (dwOutSize1+dwOutSize2!=msize)
		{
			ms_bufferizer_read(&d->output_buff,(uint8_t*)input,dwOutSize1+dwOutSize2);
			memcpy(lpOutBuf1, input, dwOutSize1);
			memcpy(lpOutBuf2, input+dwOutSize1, dwOutSize2);
		}
		else if (dwOutSize1>=msize)
		{
			ms_bufferizer_read(&d->output_buff,(uint8_t*)input,msize);
			memcpy(lpOutBuf1, input, msize);
		}
		else
		{
			ms_bufferizer_read(&d->output_buff,(uint8_t*)input,msize);
			memcpy(lpOutBuf1, input, dwOutSize1);
			memcpy(lpOutBuf2, input+dwOutSize1, dwOutSize2);
		}

		d->writeOffset=(d->writeOffset+dwOutSize1+dwOutSize2) % d->framesPerDSBuffer;
		msize_max = msize_max - (dwOutSize1+dwOutSize2);
		if (msize>msize_max)
			msize = msize_max;
		IDirectSoundBuffer_Unlock( d->lpDirectSoundOutputBuffer,
			lpOutBuf1, dwOutSize1, lpOutBuf2, dwOutSize2);
		if (dwOutSize1==0)
			break;
		if (dwOutSize1+dwOutSize2!=msize)
			break;
	}
	if (msize==0)
	{
		if (ms_bufferizer_get_avail(&d->output_buff)>=3*d->wfx.nSamplesPerSec/50)
		{
			ms_warning("Removing extra data for sound card %i", ms_bufferizer_get_avail(&d->output_buff));
			ms_bufferizer_uninit(&d->output_buff);
			ms_bufferizer_init(&d->output_buff);
		}
	}

	if (discarded>0)
		ms_warning("Extra data for sound card removed (%i buf), (playing: %i) (input-output: %i)", discarded, d->nbufs_playing, d->stat_input - d->stat_output);
}

static int get_rate(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;
	*((int*)arg)=d->wfx.nSamplesPerSec;
	return 0;
}

static int set_rate(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;
	d->wfx.nSamplesPerSec=*((int*)arg);
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;
	d->wfx.nChannels=*((int*)arg);
	return 0;
}

static int winsndds_get_stat_input(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;
	return d->stat_input;
}

static int winsndds_get_stat_ouptut(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;

	return d->stat_output;
}

static int winsndds_get_stat_discarded(MSFilter *f, void *arg){
	WinSndDs *d=(WinSndDs*)f->data;

	return d->stat_notplayed;
}

static MSFilterMethod winsndds_methods[]={
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate	},
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	MS_FILTER_GET_STAT_INPUT, winsndds_get_stat_input },
	{	MS_FILTER_GET_STAT_OUTPUT, winsndds_get_stat_ouptut },
	{	MS_FILTER_GET_STAT_DISCARDED, winsndds_get_stat_discarded },
	{	0				, NULL		}
};

MSFilterDesc winsndds_read_desc={
	MS_WINSNDDS_READ_ID,
	"DirecSoundRead",
	"DirectSound capture filter for Windows",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	winsndds_init,
	winsndds_read_preprocess,
	winsndds_read_process,
	winsndds_read_postprocess,
	winsndds_uninit,
	winsndds_methods
};


MSFilterDesc winsndds_write_desc={
	MS_WINSNDDS_WRITE_ID,
	"DirecSoundWrite",
	"DirectSound playback filter for Windows",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	winsndds_init,
	winsndds_write_preprocess,
	winsndds_write_process,
	winsndds_write_postprocess,
	winsndds_uninit,
	winsndds_methods
};

MSFilter *ms_winsndds_read_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&winsndds_read_desc);
	WinSndDsCard *wc=(WinSndDsCard*)card->data;
	WinSndDs *d=(WinSndDs*)f->data;
	d->dev_id=wc->in_devid;
	memcpy(&d->in_guid, &wc->in_guid, sizeof(GUID));
	memcpy(&d->out_guid, &wc->out_guid, sizeof(GUID));
	return f;
}


MSFilter *ms_winsndds_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&winsndds_write_desc);
	WinSndDsCard *wc=(WinSndDsCard*)card->data;
	WinSndDs *d=(WinSndDs*)f->data;
	d->dev_id=wc->out_devid;
	memcpy(&d->in_guid, &wc->in_guid, sizeof(GUID));
	memcpy(&d->out_guid, &wc->out_guid, sizeof(GUID));
	return f;
}

MS_FILTER_DESC_EXPORT(winsndds_read_desc)
MS_FILTER_DESC_EXPORT(winsndds_write_desc)

#endif
