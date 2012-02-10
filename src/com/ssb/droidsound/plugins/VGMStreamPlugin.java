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

	/* These are the supported extensions so far. I can guarantee more will come in the future.*/
	/* Most of the formats need to be tested before being actually added to the list           */

	/* Status of extensions in terms of stability:
	 *
	 * ADX: Plays back fine.
	 * AAX: Plays back fine.
	 * DSP: Plays back to fast (needs upsampling)
	 * HPS: Plays back, cuts out before song is done (needs upsampling) 
	 * YMF: Plays back fine.
	 *
	 * */

	private static final Set<String> EXTENSIONS = new HashSet<String>(Arrays.asList("AAX", "ADX", "YMF", "HPS", "DSP"));

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
	public void setOption(String string, Object val) {
		/* To be implemented */
	}

	@Override
	public boolean setTune(int tune) {
		return N_setTune(currentSong, tune);
	}

	@Override
	public String getVersion() {
		return "vgmstream revision 973";
	}

	@Override
	protected MusicInfo getMusicInfo(String name, byte[] module) {
		/* To be implemented */
		return null;
	}

	@Override
	public int getFrameRate() {

		int freq = N_getFrameRate(currentSong);
		
		Log.i(TAG, "Frequency reported by Android: " + freq + "hz");
		
		return N_getFrameRate(currentSong);
		
	}

	native private static int N_getFrameRate(long song);
	native private static void N_setOption(int what, int val);
	native private long N_loadFile(String name);
	native private void N_unload(long song);

	// Expects Stereo, 44.1Khz, signed, big-endian shorts
	native private int N_getSoundData(long song, short [] dest, int size);
	native private boolean N_seekTo(long song, int seconds);
	native private boolean N_setTune(long song, int tune);
	native private String N_getStringInfo(long song, int what);
	native private int N_getIntInfo(long song, int what);

}
