package com.ssb.droidsound.plugins;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import android.util.Log;

import com.ssb.droidsound.app.Application;

public class VGMStreamPlugin extends DroidSoundPlugin {
	private static final String TAG = VGMStreamPlugin.class.getSimpleName();

	static {
		System.loadLibrary("vgmstream");
	}

	/* 
	 * These are the supported extensions so far. I can guarantee more will come in the future.
	 * Most of the formats need to be tested before being added to the list
	 *             
	 **/

	/* Status of extensions in terms of stability:
	 *
	 * Everything works fine now. Any formats which are reported to have issues will go here.
	 * in this comment so I know what to fix.
	 *
	 * */

	private static final Set<String> EXTENSIONS = new HashSet<String>(Arrays.asList(
			"AAX", "ADX", "AFC", "AIX", "AMTS", "AST", "AUS", "BMDX", "BNS", "BRSTM",
			"DSP", "EXST", "GCM", "GCSW", "GMS", "GSB", "HPS", "IKM", "ILD", "ISD", 
			"LEG", "LOGG", "MATX", "MIB", "MIC", "MUS", "NPSF", "PNB", "PSH", "RIFF", 
			"RRDS", "RSF", "RSTM", "RWS", "RXW", "SAD", "SPD", "SPM", "STR", "STRM", 
			"STX", "SVAG", "SVS", "THP", "VPK", "VSF", "WAVM", "WSI", "WVS", "XMU", 
			"XSS", "YMF"));

	@Override
	public boolean canHandle(String name) {
		String ext = name.substring(name.indexOf('.') + 1).toUpperCase();
		return EXTENSIONS.contains(ext);
	}

	private long currentSong;

	@Override
	protected boolean load(String name, byte[] data) {
		throw new RuntimeException("This should not be called");
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

		currentSong = N_loadFile(new File(tmpDir, f1).getPath());
		return currentSong != 0;
	}

	@Override
	public int getSoundData(short[] dest) {
		return N_getSoundData(currentSong, dest, dest.length);
	}

	@Override
	public void unload() {
		N_unload(currentSong);
		currentSong = 0;
	}
	
	@Override
	public String[] getDetailedInfo() {
        String channels = N_getStringInfo(currentSong, 50);
        String sampleRate = N_getStringInfo(currentSong, 51);
        String totalPlaySamples = N_getStringInfo(currentSong, 52);
        String frameSize = N_getStringInfo(currentSong, 53);
        String samplesPerFrame = N_getStringInfo(currentSong, 54);
        
        return new String[] {
        		String.format("Channels: %s", channels),
        		"",
        		String.format("Sample Rate: %s Hz", sampleRate),
        		"",
        		String.format("Total Play Samples: %s", totalPlaySamples),
        		"",
        		String.format("Frame Size: %s bytes", frameSize),
        		"",
        		String.format("Samples Per Frame: %s", samplesPerFrame)
        };
	}

	@Override
	public void setOption(String string, Object val) {
		/* No Options */
	}


	@Override
	public String getVersion() {
		return "vgmstream revision 995";
	}

	@Override
	protected MusicInfo getMusicInfo(String name, byte[] module) {
		/* To be implemented */
		return null;
	}
	
	@Override
	public int getIntInfo(int what){
	    return N_getIntInfo(currentSong, what);
	}

	@Override
	public int getFrameRate() {

		int freq = N_getFrameRate(currentSong);
		
		Log.v(TAG, "Frequency reported by Android: " + freq + "hz");
		
		return N_getFrameRate(currentSong);
	}

	native private static int N_getFrameRate(long song);
	native private static void N_setOption(int what, int val);
	native private long N_loadFile(String name);
	native private void N_unload(long song);

	// Expects Stereo, 44.1Khz, signed, big-endian shorts
	native private int N_getSoundData(long song, short[] dest, int size);
	native private String N_getStringInfo(long song, int what);
	native private int N_getIntInfo(long song, int what);

}
