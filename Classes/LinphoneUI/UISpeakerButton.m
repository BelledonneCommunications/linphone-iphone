/* UISpeakerButton.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */       

#import "UISpeakerButton.h"
#import <AudioToolbox/AudioToolbox.h>
#include "linphonecore.h"

@implementation UISpeakerButton
static AudioSessionPropertyID routeChangeID = kAudioSessionProperty_AudioRouteChange;

static void audioRouteChangeListenerCallback (
                                       void                   *inUserData,                                 // 1
                                       AudioSessionPropertyID inPropertyID,                                // 2
                                       UInt32                 inPropertyValueSize,                         // 3
                                       const void             *inPropertyValue                             // 4
                                       ) {
    if (inPropertyID != kAudioSessionProperty_AudioRouteChange) return; // 5
    [(UISpeakerButton*)inUserData update];  
   
}

- (id)init {
    if((self = [super init]) != nil) {
   
        AudioSessionInitialize(NULL, NULL, NULL, NULL);
        OSStatus lStatus = AudioSessionAddPropertyListener(routeChangeID, audioRouteChangeListenerCallback, self);
        if (lStatus) {
            ms_error ("cannot register route change handler [%ld]",lStatus);
        }
    }
    return self;
}


- (void)onOn {
	//redirect audio to speaker
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
							 , sizeof (audioRouteOverride)
							 , &audioRouteOverride);
	
}
- (void)onOff {
	UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
	AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
							 , sizeof (audioRouteOverride)
							 , &audioRouteOverride);
}
- (bool)onUpdate {
    CFStringRef lNewRoute=CFSTR("Unknown");
    UInt32 lNewRouteSize = sizeof(lNewRoute);
    OSStatus lStatus = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute
                                                ,&lNewRouteSize
                                                ,&lNewRoute);
    if (!lStatus && CFStringGetLength(lNewRoute) > 0) {
        char route[64];
        CFStringGetCString(lNewRoute, route,sizeof(route), kCFStringEncodingUTF8);
        ms_message("Current audio route is [%s]",route);
        return (    kCFCompareEqualTo == CFStringCompare (lNewRoute,CFSTR("Speaker"),0) 
                ||  kCFCompareEqualTo == CFStringCompare (lNewRoute,CFSTR("SpeakerAndMicrophone"),0));
    } else 
        return false;
}

- (void)dealloc {
    OSStatus lStatus = AudioSessionRemovePropertyListenerWithUserData(routeChangeID, audioRouteChangeListenerCallback, self);
	if (lStatus) {
		ms_error ("cannot un register route change handler [%ld]",lStatus);
	}
	[super dealloc];
}

@end
