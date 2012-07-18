/* Utils.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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


#import "Utils.h"
#include "linphonecore.h"

@implementation LinphoneLogger

+ (void)log:(LinphoneLoggerSeverity) severity format:(NSString *)format,... {
    va_list args;
	va_start (args, format);
    NSString *str = [[NSString alloc] initWithFormat: format arguments:args];
    if(severity <= LinphoneLoggerLog) {
        ms_message("%s", [str UTF8String]);
    } else if(severity <= LinphoneLoggerDebug) {
        ms_debug("%s", [str UTF8String]);
    } else if(severity <= LinphoneLoggerWarning) {
        ms_warning("%s", [str UTF8String]);
    } else if(severity <= LinphoneLoggerError) {
        ms_error("%s", [str UTF8String]);
    } else if(severity <= LinphoneLoggerFatal) {
        ms_fatal("%s", [str UTF8String]);
    }
    [str release];
    va_end (args);
}

+ (void)logc:(LinphoneLoggerSeverity) severity format:(const char *)format,... {
    va_list args;
	va_start (args, format);
    if(severity <= LinphoneLoggerLog) {
        ortp_logv(ORTP_MESSAGE, format, args);
    } else if(severity <= LinphoneLoggerDebug) {
        ortp_logv(ORTP_DEBUG, format, args);
    } else if(severity <= LinphoneLoggerWarning) {
        ortp_logv(ORTP_WARNING, format, args);
    } else if(severity <= LinphoneLoggerError) { 
        ortp_logv(ORTP_ERROR, format, args);
    } else if(severity <= LinphoneLoggerFatal) {
        ortp_logv(ORTP_FATAL, format, args);
    }
	va_end (args);
}

@end
