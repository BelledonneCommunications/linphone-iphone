package org.linphone.tools;
/*
CodecDownloader.java
Copyright (C) 2016  Belledonne Communications, Grenoble, France

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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;

import org.apache.commons.compress.compressors.bzip2.*;
import org.linphone.core.OpenH264DownloadHelperListener;
import org.linphone.core.OpenH264DownloadHelperAction;

/**
 * @author Erwan Croze
 */
public class OpenH264DownloadHelper {
    private OpenH264DownloadHelperListener openH264DownloadHelperListener;
    private OpenH264DownloadHelperAction openH264DownloadHelperAction;
    private ArrayList<Object> userData;
    private String fileDirection = null;
    private String nameLib;
    private String urlDownload;
    private String nameFileDownload;
    private String licenseMessage;

	/**
     * Default values
     * nameLib = "libopenh264-1.5.so"
     * urlDownload = "http://ciscobinary.openh264.org/libopenh264-1.5.0-android19.so.bz2"
     * nameFileDownload = "libopenh264-1.5.0-android19.so.bz2"
     */
    public OpenH264DownloadHelper() {
        userData = new ArrayList<Object>();
        licenseMessage = "OpenH264 Video Codec provided by Cisco Systems, Inc.";
        nameLib = "libopenh264-1.5.so";
        urlDownload = "http://ciscobinary.openh264.org/libopenh264-1.5.0-android19.so.bz2";
        nameFileDownload = "libopenh264-1.5.0-android19.so.bz2";
    }

	/**
     * Set OpenH264DownloadHelperListener
     * @param h264Listener
     */
    public void setOpenH264HelperListener(OpenH264DownloadHelperListener h264Listener) {
        openH264DownloadHelperListener = h264Listener;
    }

	/**
     * Set OpenH264DownloadHelperAction
     * @param h264Action
     */
    public void setOpenH264HelperAction(OpenH264DownloadHelperAction h264Action) {
        openH264DownloadHelperAction = h264Action;
    }

	/**
     * @return OpenH264DownloadHelperAction
     */
    public OpenH264DownloadHelperAction getOpenH264DownloadHelperAction() {
        return openH264DownloadHelperAction;
    }

	/**
     * @return OpenH264DownloadHelperListener
     */
    public OpenH264DownloadHelperListener getOpenH264DownloadHelperListener() {
        return openH264DownloadHelperListener;
    }

	/**
     * @param index of object in UserData list
     * @constraints (index >= 0 && index < userData.size())
     * @return object if constraints are met
     */
    public Object getUserData(int index) {
        if (index < 0 || index >= userData.size()) return null;
        return userData.get(index);
    }

	/**
     * Adding of object into UserData list
     * @param object
     * @return index of object in UserData list
     */
    public int setUserDate(Object object) {
        this.userData.add(object);
        return this.userData.indexOf(object);
    }

	/**
     * @param index
     * @param object
     * @constraints (index >= 0 && index < userData.size())
     */
    public void setUserData(int index, Object object) {
        if (index < 0 || index > userData.size()) return;
        this.userData.add(index,object);
    }

	/**
     * @return size of UserData list
     */
    public int getUserDataSize() {
        return this.userData.size();
    }

	/**
     * @return OpenH264 license message
     */
    public String getLicenseMessage() {
        return licenseMessage;
    }

	/**
     * Set path for file storage
     * @param path
     */
    public void setFileDirection(String path) { fileDirection = path; }

	/**
     * Set filename to storage for OpenH264 codec
     * @param name
     */
    public void setNameLib(String name) {
        nameLib = name;
    }

	/**
     * @return filename of OpenH264 codec
     */
    public String getNameLib() {
        return nameLib;
    }

	/**
     * Set name download file
     * @param name : must be the same name relative to the url
     */
    public void setNameFileDownload(String name) {
        nameFileDownload = name;
    }

	/**
     * Set new url
     * @param url : must be a Cisco Url to OpenH264 and .bzip2 file
     */
    public void setUrlDownload(String url) {
        urlDownload = url;
    }

	/**
     * Indicates whether the lib exists
     * Requirements : fileDirection and nameLib init
     * @return file exists ?
     */
    public boolean isCodecFound() {
        return new File(fileDirection+"/" + nameLib).exists();
    }

	/**
     * Try to download codec
     * Requirements :
     *  fileDirection
     *  nameFileDownload
     *  urlDownload
     *  nameLib
     *  codecDownListener
     */
    public void downloadCodec() {
        Thread thread = new Thread(new Runnable()
        {
            @Override
            public void run()
            {
                try {
                    String path = fileDirection+"/" + nameLib;
                    URL url = new URL(urlDownload);
                    HttpURLConnection urlConnection = (HttpURLConnection)url.openConnection();
                    urlConnection.connect();
                    openH264DownloadHelperListener.OnProgress(0,10);

                    InputStream inputStream = urlConnection.getInputStream();
                    FileOutputStream fileOutputStream = new FileOutputStream(fileDirection+"/"+nameFileDownload);
                    int totalSize = urlConnection.getContentLength();

                    byte[] buffer = new byte[4096];
                    int bufferLength;
                    int total = 0;
                    openH264DownloadHelperListener.OnProgress(total, totalSize);
                    while((bufferLength = inputStream.read(buffer))>0 ){
                        total += bufferLength;
                        fileOutputStream.write(buffer, 0, bufferLength);
                        openH264DownloadHelperListener.OnProgress(total, totalSize);
                    }

                    fileOutputStream.close();
                    inputStream.close();

                    FileInputStream in = new FileInputStream(fileDirection+"/"+nameFileDownload);
                    FileOutputStream out = new FileOutputStream(path);
                    BZip2CompressorInputStream bzIn = new BZip2CompressorInputStream(in);

                    while ((bufferLength = bzIn.read(buffer))>0) {
                        out.write(buffer, 0, bufferLength);
                    }
                    in.close();
                    out.close();
                    bzIn.close();

                    new File(fileDirection+"/"+nameFileDownload).delete();
                    openH264DownloadHelperListener.OnProgress(2,1);
                } catch (FileNotFoundException e) {
                    openH264DownloadHelperListener.OnError(e.getLocalizedMessage());
                } catch (IOException e) {
                    openH264DownloadHelperListener.OnError(e.getLocalizedMessage());
                }
            }
        });
        thread.start();
    }
}
