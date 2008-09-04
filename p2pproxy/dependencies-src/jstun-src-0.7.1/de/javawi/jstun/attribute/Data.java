package de.javawi.jstun.attribute;

import java.util.logging.Logger;

import de.javawi.jstun.util.UtilityException;

public class Data extends MessageAttribute {
	private static Logger logger = Logger.getLogger(Data.class.getName());
	@Override
	public byte[] getBytes() throws UtilityException {
		// TODO Auto-generated method stub
		return null;
	}
	public static MessageAttribute parse(byte[] data) throws MessageAttributeParsingException {
		Data ma = new Data();
		
		logger.finer("Message Attribute: Data Address parsed: " + ma.toString() + ".");
		return ma;
	}
}
