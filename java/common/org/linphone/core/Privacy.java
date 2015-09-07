package org.linphone.core;

public interface Privacy {
	public static final int NONE=0;
	public static final int USER=0x1;
	public static final int HEADER=0x2;
	public static final int SESSION=0x4;
	public static final int ID=0x8;
	public static final int CRITICAL=0x10;
	public static final int DEFAULT=0x8000;
}
