package org.linphone.tools;


import android.os.Build;

import org.linphone.core.LinphoneCore;
import org.linphone.mediastream.Log;

/**
 * Created by brieucviel on 09/12/2016.
 */

public class H264Helper {
    private static String FILTER_NAME_OPENH264_ENC = "MSOpenH264Enc" ;
    private static String FILTER_NAME_OPENH264_DEC = "MSOpenH264Dec" ;
    private static String FILTER_NAME_MEDIA_CODEC_ENC = "MSMediaCodecH264Enc" ;
    private static String FILTER_NAME_MEDIA_CODEC_DEC = "MSMediaCodecH264Dec" ;

    public static String MODE_AUTO = "Auto" ;
    public static String MODE_OPENH264 = "OpenH264" ;
    public static String MODE_MEDIA_CODEC = "MediaCodec" ;


    /**
     * H264Helper
     */
    public H264Helper() {
    }


    /**
     * Define the Codec to use between MediaCodec and OpenH264
     * Possible mode are:
     *      - Auto to let the system choose in function of you OS version,
     *      - OpenH264 to enable OpenH264 Encoder and Decoder,
     *      - Mediacodec to enable Mediacodec only.
     * @param mode String value between Auto, OpenH264 and MediaCodec
     */
    public static void setH264Mode(String mode, LinphoneCore linphoneCore){
        if(mode.equals(MODE_OPENH264)){
            Log.i("H264Helper"," setH264Mode  MODE_OPENH264 - Mode = "+mode);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_DEC , false);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_ENC , false);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_DEC , true);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_ENC , true);
        }else if(mode.equals(MODE_MEDIA_CODEC)){
            Log.i("H264Helper"," setH264Mode  MODE_MEDIA_CODEC - Mode = "+mode);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_DEC , false);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_ENC , false);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_DEC , true);
            linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_ENC , true);
        }else if(mode.equals(MODE_AUTO)){
            Log.i("H264Helper"," setH264Mode  MODE_AUTO - Mode = "+mode);
           // if  android >= 5.0 use MediaCodec
           if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
               Log.i("H264Helper"," setH264Mode  MODE_AUTO 1 - Mode = "+mode);
               Log.i("LinphoneCoreFactoryImpl"," Openh264 disabled on the project, now using MediaCodec");
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_DEC , false);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_ENC , false);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_DEC , true);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_ENC , true);
           }
           //otherwise use OpenH264
           else{
               Log.i("H264Helper"," setH264Mode  MODE_AUTO 2 - Mode = "+mode);
               Log.i("LinphoneCoreFactoryImpl"," Openh264 enabled on the project");
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_DEC , false);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_MEDIA_CODEC_ENC , false);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_DEC , true);
               linphoneCore.getMSFactory().enableFilterFromName(FILTER_NAME_OPENH264_ENC , true);
           }
        }else {
            Log.i("LinphoneCoreFactoryImpl"," Error: Openh264 mode not reconized !");
        }
        Log.i("H264Helper"," setH264Mode - Mode = "+mode);
    }

}

