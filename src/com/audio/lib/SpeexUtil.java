package com.audio.lib;

public class SpeexUtil {
    private static final int DEFAULT_COMPRESSION = 4;
    private static SpeexUtil speexUtil;
    static {
        try {
            System.loadLibrary("speex");
        } catch (Throwable var1) {
            var1.printStackTrace();
        }
        speexUtil = null;
    }
    
  //设置为4时压缩比为1/16(与编解码密切相关)
    SpeexUtil() {
        this.open(4);
    }
    
    public static SpeexUtil getInstance() {
        if(speexUtil == null) {
            Class var0 = SpeexUtil.class;
            synchronized(SpeexUtil.class) {
                if(speexUtil == null) {
                    speexUtil = new SpeexUtil();
                }
            }
        }
        return speexUtil;
    }
    
    public native int open(int compression);  
    public native int getFrameSize();  
    public native int decode(byte encoded[], short lin[], int size);  
    public native int encode(short lin[], int offset, byte encoded[], int size);  
    public native void close();  
    
}
