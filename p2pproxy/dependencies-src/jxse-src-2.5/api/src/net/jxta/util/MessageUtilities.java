/*
 * Copyright (c) 2001-2007 Sun Microsystems, Inc.  All right reserved.
 *  
 *  The Sun Project JXTA(TM) Software License
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *  
 *  3. The end-user documentation included with the redistribution, if any, must 
 *     include the following acknowledgment: "This product includes software 
 *     developed by Sun Microsystems, Inc. for JXTA(TM) technology." 
 *     Alternately, this acknowledgment may appear in the software itself, if 
 *     and wherever such third-party acknowledgments normally appear.
 *  
 *  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must 
 *     not be used to endorse or promote products derived from this software 
 *     without prior written permission. For written permission, please contact 
 *     Project JXTA at http://www.jxta.org.
 *  
 *  5. Products derived from this software may not be called "JXTA", nor may 
 *     "JXTA" appear in their name, without prior written permission of Sun.
 *  
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SUN 
 *  MICROSYSTEMS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 *  JXTA is a registered trademark of Sun Microsystems, Inc. in the United 
 *  States and other countries.
 *  
 *  Please see the license information page at :
 *  <http://www.jxta.org/project/www/license.html> for instructions on use of 
 *  the license in source files.
 *  
 *  ====================================================================
 *  
 *  This software consists of voluntary contributions made by many individuals 
 *  on behalf of Project JXTA. For more information on Project JXTA, please see 
 *  http://www.jxta.org.
 *  
 *  This license is based on the BSD license adopted by the Apache Foundation. 
 */

package net.jxta.util;


import net.jxta.endpoint.Message;
import net.jxta.endpoint.MessageElement;
import net.jxta.endpoint.StringMessageElement;


/**
 * @deprecated These utilities are unsupported and known to have problems. They
 *             are not recommended for use and will be removed in a future JXTA release
 *             THIS CLASS IS SCHEDULED TO BE REMOVED AFTER 2.5.
 */
@Deprecated
public final class MessageUtilities {

    private MessageUtilities() {}

    // Fix-Me: I didn't implement byte, short or float
    // Fix-Me: I didn't implement arrays, ie addInt(Message message, String tagName, int values[]), etc
    // Fix-Me: I didn't implement add DocumentSerializable yet

    public static void addInt(Message message, String tagName, int value) {
        StringMessageElement stringMessageElement = new StringMessageElement(tagName, Integer.toString(value), null);

        message.addMessageElement(stringMessageElement);
    }

    public static int getInt(Message message, String tagName, int defaultValue) {
        StringMessageElement stringMessageElement = (StringMessageElement) message.getMessageElement(tagName);

        if (stringMessageElement != null) {
            return Integer.parseInt(stringMessageElement.toString());
        } else {
            return defaultValue;
        }
    }

    public static void addLong(Message message, String tagName, long value) {
        StringMessageElement stringMessageElement = new StringMessageElement(tagName, Long.toString(value), null);

        message.addMessageElement(stringMessageElement);
    }

    public static long getLong(Message message, String tagName, long defaultValue) {
        StringMessageElement stringMessageElement = (StringMessageElement) message.getMessageElement(tagName);

        if (stringMessageElement != null) {
            return Long.parseLong(stringMessageElement.toString());
        } else {
            return defaultValue;
        }
    }

    public static void addDouble(Message message, String tagName, double value) {
        StringMessageElement stringMessageElement = new StringMessageElement(tagName, Double.toString(value), null);

        message.addMessageElement(stringMessageElement);
    }

    public static double getDouble(Message message, String tagName, double defaultValue) {
        StringMessageElement stringMessageElement = (StringMessageElement) message.getMessageElement(tagName);

        if (stringMessageElement != null) {
            return Double.parseDouble(stringMessageElement.toString());
        } else {
            return defaultValue;
        }
    }

    public static void addBoolean(Message message, String tagName, boolean value) {
        StringMessageElement stringMessageElement = new StringMessageElement(tagName, value ? "true" : "false", null);

        message.addMessageElement(stringMessageElement);
    }

    public static boolean getBoolean(Message message, String tagName, boolean defaultValue) {
        StringMessageElement stringMessageElement = (StringMessageElement) message.getMessageElement(tagName);

        if (stringMessageElement != null) {
            return "true".equals(stringMessageElement.toString());
        } else {
            return defaultValue;
        }
    }

    public static void addString(Message message, String tagName, String value) {
        StringMessageElement stringMessageElement = new StringMessageElement(tagName, value, null);

        message.addMessageElement(stringMessageElement);
    }

    public static String getString(Message message, String tagName, String defaultValue) {
        MessageElement stringMessageElement = message.getMessageElement(tagName);

        if (stringMessageElement != null) {
            return stringMessageElement.toString();
        } else {
            return defaultValue;
        }
    }

}

