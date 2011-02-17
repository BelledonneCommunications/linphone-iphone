/*
VideoSize.java
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
 * @author Guillaume Beraudo
 */
public final class VideoSize {
	public static final int QCIF = 0;
	public static final int CIF = 1;
	public static final int HVGA = 2;
	public static final int QVGA = 3;

	public int width;
	public int height;

	public VideoSize() {}
	public VideoSize(int width, int height) {
		this.width = width;
		this.height = height;
	}

	public static final VideoSize createStandard(int code, boolean inverted) {
		switch (code) {
		case QCIF:
			return inverted? new VideoSize(144, 176) : new VideoSize(176, 144);
		case CIF:
			return inverted? new VideoSize(288, 352) : new VideoSize(352, 288);
		case HVGA:
			return inverted? new VideoSize(320,480) : new VideoSize(480, 320);
		case QVGA:
			return inverted? new VideoSize(240, 320) : new VideoSize(320, 240);
		default:
			return new VideoSize(); // Invalid one
		}
	}
	
	public boolean isValid() {
		return width > 0 && height > 0;
	}
	
	// Generated
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + height;
		result = prime * result + width;
		return result;
	}
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		VideoSize other = (VideoSize) obj;
		if (height != other.height)
			return false;
		if (width != other.width)
			return false;
		return true;
	}
	
	
	public String toString() {
		return "width = "+width + " height = " + height;
	}
	public boolean isPortrait() {
		return height >= width;
	}
	public VideoSize createInverted() {
		return new VideoSize(height, width);
	}
	
}
