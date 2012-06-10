package com.ssb.droidsound.plugins;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Map;

import com.ssb.droidsound.app.Application;

public class SexyPSFPlugin extends DroidSoundPlugin {
    
    private long songFile = 0;
    private String[] info = null;
    
    
    static {
        System.loadLibrary("sexypsf");
    }
    
    private static String fromData(byte [] data, int start, int len) throws UnsupportedEncodingException {
        int i = start;
        for(; i<start+len; i++) {
            if(data[i] == 0) {
                i++;
                break;
            }
        }
        return new String(data, start, i-start, "ISO-8859-1").trim();
    }
    
    private Map<String, String> getTags(byte [] module, int size) {
        ByteBuffer src = ByteBuffer.wrap(module, 0, size);      
        src.order(ByteOrder.LITTLE_ENDIAN);     
        byte[] id = new byte[4];
        src.get(id);
        
        //for(int i=0; i<128; i++)
        //  info[i] = null;
        
         info = new String [128];
        
        if(id[0] == 'P' && id[1] == 'S' && id[2] == 'F' && id[3] == 1) {
            
            
            int resLen = src.getInt();
            int comprLen = src.getInt();

            src.position(resLen + comprLen + 16);
            
            if(src.remaining() >= 5) {
                
                byte [] tagHeader = new byte[5];        
                src.get(tagHeader);
                
                if(new String(tagHeader).equals("[TAG]")) {
                    
                    byte [] tagData = new byte [ size - comprLen - resLen - 21];
                    src.get(tagData);
                    
                    try {
                        String tags = new String(tagData, "ISO-8859-1").trim();
                        
                        String [] lines = tags.split("\n");
                        
                        HashMap<String, String> tagMap = new HashMap<String, String>();
                        
                        for(String line : lines) {
                            String parts [] = line.split("=");
                            tagMap.put(parts[0], parts[1]);
                        }
                        return tagMap;
                    } catch (UnsupportedEncodingException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
            }
        }
        return null;
    }
    
    @Override
    public boolean canHandle(String name) {
        String ext = name.substring(name.indexOf('.') + 1).toUpperCase();
        return ext.equals("PSF") || ext.equals("MINIPSF");
    }
    
    @Override
    protected boolean load(String name, byte[] module) {
        // TODO Auto-generated method stub
        return false;
    }
    
    @Override
    public boolean load(String f1, byte[] data1, String f2, byte[] data2) {
        File tmpDir = Application.getTmpDirectory();
        for (File f : tmpDir.listFiles()) {
            if (! f.getName().startsWith(".")) {
                f.delete();
            }
        }

        try {
            FileOutputStream fo1 = new FileOutputStream(new File(tmpDir, f1));
            fo1.write(data1);
            fo1.close();
            if (f2 != null) {
                FileOutputStream fo2 = new FileOutputStream(new File(tmpDir, f2));
                fo2.write(data2);
                fo2.close();
            }
        }
        catch (IOException ioe) {
            throw new RuntimeException(ioe);
        }

        songFile = N_loadFile(new File(tmpDir, f1).getPath());
        return true;
    }
    
    
    @Override
    public int getSoundData(short[] dest) {
        return N_getSoundData(songFile, dest, dest.length);
    }
    
    
    @Override
    public void unload() {
        if(songFile != 0)
            N_unload(songFile);
        else if(info != null)
            info = null;  
    }
    
    
    @Override
    public void setOption(String string, Object val) {
        /* No options yet */
    }
    
    
    @Override
    public String getVersion() {
        return "SexyPSF 0.4.7";
    }
    
    @Override
    public String[] getDetailedInfo() {
        String[] info = new String[6];
        info[0] = "Format:";
        info[1] = "PSF (Playstation 1)";
        info[2] = "Game:";
        info[3] = N_getStringInfo(songFile, INFO_GAME);
        info[4] = "Copyright:";
        info[5] = N_getStringInfo(songFile, INFO_COPYRIGHT);
        return info;
    }
    
    @Override
    public int getIntInfo(int what) {
        
        if(info != null) {
            if(what == INFO_LENGTH) {
                String [] parts = info[what].split("=");
                int secs = 0;
                if(parts != null && parts.length == 2)
                    secs = Integer.parseInt(parts[0]) * 60 + Integer.parseInt(parts[1]);
                return secs;
            }
            return 0;
        }
        return N_getIntInfo(songFile, what);
    }
    
    @Override
    protected MusicInfo getMusicInfo(String name, byte[] module) {
        ByteBuffer src = ByteBuffer.wrap(module, 0, module.length);      
        src.order(ByteOrder.LITTLE_ENDIAN);     
        byte[] id = new byte[4];
        src.get(id);
        
        info = new String [128];
         
        Map<String, String> tagMap = getTags(module, module.length);
        if(tagMap != null) {
            info[INFO_TITLE] = tagMap.get("title");
            info[INFO_AUTHOR] = tagMap.get("artist");
            info[INFO_GAME] = tagMap.get("game");
            info[INFO_COPYRIGHT] = tagMap.get("copyright");
            info[INFO_LENGTH] = tagMap.get("length");
            return new MusicInfo();
        }
        
        return null;
    }

    native private long N_loadFile(String name);
    native private void N_unload(long song);
    native private int N_getSoundData(long song, short[] dest, int size);
    native private String N_getStringInfo(long song, int what);
    native private int N_getIntInfo(long song, int what);
}
