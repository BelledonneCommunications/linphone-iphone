/*
LinphoneLogHandler.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
package org.linphone.core;
/**
 * Interface to implement for handling liblinphone log.
 * <br> use {@link LinphoneCoreFactory#setLogHandler(LinphoneLogHandler)}
 *
 */
public interface LinphoneLogHandler {
	public static final int Fatal=1<<4;
	public static final int Error=1<<3;
	public static final int Warn=1<<2;
	public static final int Info=1<<1;
	public static final int Debug=1;
	
	/**
	 * Method invoked for each traces
	 * @param loggerName
	 * @param level
	 * @param levelString
	 * @param msg
	 * @param e
	 */
	public void log(String loggerName, int level, String levelString, String msg, Throwable e);
}
