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
import org.linphone.core.CodecDownloadAction;
import org.linphone.core.CodecDownloadListener;

/**
 * @author Erwan Croze
 */
public class CodecDownloader {
    private CodecDownloadListener codecDownListener;
    private CodecDownloadAction codecDownAction;
    private ArrayList<Object> userData;
    private static String fileDirection = null;
    private static String nameLib;
    private static String urlDownload;
    private static String nameFileDownload;
    private static String licenseMessage = "OpenH264 Video Codec provided by Cisco Systems, Inc.";

    public CodecDownloader() {
        userData = new ArrayList<Object>();
        nameLib = "libopenh264-1.5.so";
        urlDownload = "http://ciscobinary.openh264.org/libopenh264-1.5.0-android19.so.bz2";
        nameFileDownload = "libopenh264-1.5.0-android19.so.bz2";
    }

    public void setCodecDownloadlistener(CodecDownloadListener cdListener) {
        codecDownListener = cdListener;
    }

    public void setCodecDownloadAction(CodecDownloadAction cdAction) {
        codecDownAction = cdAction;
    }

    public CodecDownloadAction getCodecDownloadAction() {
        return codecDownAction;
    }

    public Object getUserData(int i) {
        return userData.get(i);
    }

    public void setUserData(int i, Object d) {
        this.userData.add(i,d);
    }

    public int getUserDataSize() {
        return this.userData.size();
    }

    static public String getLicenseMessage() {
        return licenseMessage;
    }

    static public void setFileDirection(String s) { fileDirection = s; }

    static public void setNameLib(String s) {
        nameLib = s;
    }

    static public String getNameLib() {
        return nameLib;
    }

    static public void setNameFileDownload(String s) {
        nameFileDownload = s;
    }

    static public void setUrlDownload(String s) {
        urlDownload = s;
    }

	/**
     * Indicates whether the lib exists
     * Requirements : fileDirection and nameLib init
     * @return file exists ?
     */
    static public boolean codecExist() {
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
                    codecDownListener.listenerUpdateProgressBar(0,10);

                    InputStream inputStream = urlConnection.getInputStream();
                    FileOutputStream fileOutputStream = new FileOutputStream(fileDirection+"/"+nameFileDownload);
                    int totalSize = urlConnection.getContentLength();

                    byte[] buffer = new byte[4096];
                    int bufferLength;
                    int total = 0;
                    codecDownListener.listenerUpdateProgressBar(total, totalSize);
                    while((bufferLength = inputStream.read(buffer))>0 ){
                        total += bufferLength;
                        fileOutputStream.write(buffer, 0, bufferLength);
                        codecDownListener.listenerUpdateProgressBar(total, totalSize);
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
                    codecDownListener.listenerUpdateProgressBar(2,1);
                } catch (FileNotFoundException e) {
                    codecDownListener.listenerDownloadFailed(e.getMessage());
                } catch (IOException e) {
                    codecDownListener.listenerDownloadFailed(e.getMessage());
                }
            }
        });
        thread.start();
    }
}
