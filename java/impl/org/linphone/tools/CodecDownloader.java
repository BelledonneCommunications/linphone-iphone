package org.linphone.tools;
/*
AssistantActivity.java
Copyright (C) 2015  Belledonne Communications, Grenoble, France

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

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.preference.CheckBoxPreference;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import org.apache.commons.compress.compressors.bzip2.*;
import org.linphone.LinphoneManager;
import org.linphone.LinphonePreferences;
import org.linphone.core.LinphoneCoreException;
import org.linphone.core.LinphoneCoreFactoryImpl;
import org.linphone.core.PayloadType;

interface CodecDownloadListener{
    void listenerDownloadStarting();
    void listenerUpdateMsg(int now, int max);
    void listenerDownloadEnding();
    void listenerDownloadFailed(String error);
}

interface CodecDownloadAction{
    void startDownload(Context context, Object obj);
}

/**
 * @author Erwan Croze
 */
public class CodecDownloader implements CodecDownloadListener,CodecDownloadAction{
    private static String fileDirection = null;
    private static String nameLib;
    private static String urlDownload;
    private static String nameFileDownload;
    private static String licenseMessage = "OpenH264 Video Codec provided by Cisco Systems, Inc.";

    public CodecDownloader() {
        nameLib = "libopenh264-1.5.so";
        urlDownload = "http://ciscobinary.openh264.org/libopenh264-1.5.0-android19.so.bz2";
        nameFileDownload = "libopenh264-1.5.0-android19.so.bz2";
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
                    listenerDownloadStarting();

                    InputStream inputStream = urlConnection.getInputStream();
                    FileOutputStream fileOutputStream = new FileOutputStream(fileDirection+"/"+nameFileDownload);
                    int totalSize = urlConnection.getContentLength();

                    byte[] buffer = new byte[4096];
                    int bufferLength;
                    int total = 0;
                    listenerUpdateMsg(total, totalSize);
                    while((bufferLength = inputStream.read(buffer))>0 ){
                        total += bufferLength;
                        fileOutputStream.write(buffer, 0, bufferLength);
                        listenerUpdateMsg(total, totalSize);
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
                    listenerDownloadEnding();
                } catch (FileNotFoundException e) {
                    listenerDownloadFailed(e.getMessage());
                } catch (IOException e) {
                    listenerDownloadFailed(e.getMessage());
                }
            }
        });
        thread.start();
    }

    @Override
    public void listenerDownloadStarting() {
    }

    @Override
    public void listenerUpdateMsg(final int now, final int max) {
    }

    @Override
    public void listenerDownloadEnding() {
    }

    @Override
    public void listenerDownloadFailed(final String error) {
    }

    @Override
    public void startDownload(Context context, Object obj) {
    }
}
