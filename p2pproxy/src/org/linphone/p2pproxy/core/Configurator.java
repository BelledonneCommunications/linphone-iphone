/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

Configurator.java - .

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
package org.linphone.p2pproxy.core;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Date;
import java.util.InvalidPropertiesFormatException;
import java.util.Properties;
import java.util.Set;

import org.apache.log4j.Logger;

@SuppressWarnings("serial")
public class Configurator extends Properties {
	private final static Logger mLog = Logger.getLogger(Configurator.class);
	private final File mFile;
	public Configurator (File aFile) throws InvalidPropertiesFormatException, FileNotFoundException, IOException {
		super();
		mFile = aFile;
	      if (mFile.exists()) {
	    	  loadFromXML(new FileInputStream(mFile));
	      }
	}
	/**
	 * save to disk
	 * @throws IOException 
	 * @throws FileNotFoundException 
	 */
	public void save() throws FileNotFoundException, IOException {
		storeToXML(new FileOutputStream(mFile),new Date().toString());
	}
	public Object setProperty(String key,String value) {
		Object lReturn = super.setProperty(key, value);
		try {
			save();
		} catch (Exception e) {
			mLog.error("enable to save prop ["+key+"] value ["+value+"]", e);
		}
		return lReturn;
	}
    public void serProperties(Properties aProperties) {
       for (Object key :aProperties.keySet()){
          setProperty((String)key,aProperties.getProperty((String)key));
       }
    }

}
