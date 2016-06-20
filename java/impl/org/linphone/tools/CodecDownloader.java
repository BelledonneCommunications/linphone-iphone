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
    void listenerPluginReloaded();
}

/**
 * @author Erwan Croze
 */
public class CodecDownloader implements DialogInterface.OnClickListener, CodecDownloadListener{
    private Context context;
    private static String nameLib;
    private static String urlDownload;
    private static String nameFileDownload;
    private static String messageUpdateDownload;
    private static String messageEndSetup;
    private static Handler mHandler;
    private static ProgressDialog progress;

    public CodecDownloader(Context c) {
        this.context = c;
        this.mHandler = new Handler(this.context.getMainLooper());
        this.progress = null;
        nameLib = "libopenh264-1.5.so";
        urlDownload = "http://ciscobinary.openh264.org/libopenh264-1.5.0-android19.so.bz2";
        nameFileDownload = "libopenh264-1.5.0-android19.so.bz2";
        messageUpdateDownload = "Downloading codec";
        messageEndSetup = "Codec OpenH264 downloaded";
    }

    public void askPopUp(String msg, String no, String yes) {
        if (this.context == null) return;
        AlertDialog.Builder builder = new AlertDialog.Builder(this.context);
        builder.setCancelable(false);
        builder.setMessage(msg).setPositiveButton(yes, this)
                .setNegativeButton(no, this).show();
    }

    public void onClick(DialogInterface dialog, int which) {
        switch (which){
            case DialogInterface.BUTTON_POSITIVE:
                downloadCodec();
                break;

            case DialogInterface.BUTTON_NEGATIVE:
                // Disable H264
                PayloadType h264 = null;
                for (PayloadType pt : LinphoneManager.getLcIfManagerNotDestroyedOrNull().getVideoCodecs()) {
                    if (pt.getMime().equals("H264")) h264 = pt;
                }

                if (h264 == null) return;

                if (LinphonePreferences.instance().isFirstLaunch()) {
                    try {
                        LinphoneManager.getLcIfManagerNotDestroyedOrNull().enablePayloadType(h264, false);
                    } catch (LinphoneCoreException e) {
                        e.printStackTrace();
                    }
                }
                break;
        }
    }
    static public void setMessageEndSetup(String s) {
        messageEndSetup = s;
    }

    static public void setMessageUpdateDownloadDl(String s) {
        messageUpdateDownload = s;
    }

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

    static public boolean codecExist(Context ctxt) {
        if (ctxt == null) return false;
        return new File(ctxt.getFilesDir()+"/" + nameLib).exists();
    }

    public void downloadCodec() {
        if (context == null) return;
        Thread thread = new Thread(new Runnable()
        {
            @Override
            public void run()
            {
                try {
                    String path = context.getFilesDir()+"/" + nameLib;
                    URL url = new URL(urlDownload);
                    HttpURLConnection urlConnection = (HttpURLConnection)url.openConnection();
                    urlConnection.connect();

                    InputStream inputStream = urlConnection.getInputStream();
                    FileOutputStream fileOutputStream = new FileOutputStream(context.getFilesDir()+"/"+nameFileDownload);
                    int totalSize = urlConnection.getContentLength();

                    byte[] buffer = new byte[4096];
                    int bufferLength;
                    int total = 0;
                    listenerDownloadStarting();
                    while((bufferLength = inputStream.read(buffer))>0 ){
                        total += bufferLength;
                        fileOutputStream.write(buffer, 0, bufferLength);
                        listenerUpdateMsg(total, totalSize);
                    }

                    fileOutputStream.close();
                    inputStream.close();

                    FileInputStream in = new FileInputStream(context.getFilesDir()+"/"+nameFileDownload);
                    FileOutputStream out = new FileOutputStream(path);
                    BZip2CompressorInputStream bzIn = new BZip2CompressorInputStream(in);

                    while ((bufferLength = bzIn.read(buffer))>0) {
                        out.write(buffer, 0, bufferLength);
                    }
                    in.close();
                    out.close();
                    bzIn.close();

                    new File(context.getFilesDir()+"/"+nameFileDownload).delete();
                    listenerDownloadEnding();
                    LinphoneCoreFactoryImpl.loadOptionalLibraryWithPath(path);
                    LinphoneManager.getLc().reloadMsPlugins(null);
                    listenerPluginReloaded();
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
        if (context == null) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                progress = new ProgressDialog(context);
                progress.setCanceledOnTouchOutside(false);
                progress.setCancelable(false);
                progress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            }
        });
    }

    @Override
    public void listenerUpdateMsg(final int now, final int max) {
        if (progress == null) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                progress.setMessage(messageUpdateDownload);
                progress.setMax(max);
                progress.setProgress(now);
                progress.show();
            }
        });
    }

    @Override
    public void listenerDownloadEnding() {
        if (progress == null) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                progress.dismiss();

            }
        });
    }

    @Override
    public void listenerDownloadFailed(final String error) {
        if (progress == null) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                if(progress != null) progress.dismiss();
                AlertDialog.Builder builder = new AlertDialog.Builder(context);
                builder.setMessage(error);
                builder.show();
            }
        });
    }

    @Override
    public void listenerPluginReloaded() {
        if (progress == null) return;
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(context);
                builder.setMessage(messageEndSetup);
                builder.setCancelable(false);
                builder.setNeutralButton("Ok",null);
                builder.show();
            }
        });
    }
}
