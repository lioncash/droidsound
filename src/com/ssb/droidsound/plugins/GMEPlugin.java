package com.ssb.droidsound.plugins;

import java.io.File;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class GMEPlugin extends DroidSoundPlugin {
	private static final Charset ISO88591 = Charset.forName("ISO-8859-1");
	static {
		System.loadLibrary("gme");
	}

	private static String fromData(byte[] module, int start, int len) {
		return new String(module, start, len, ISO88591).replaceFirst("\u0000.*", "");
	}

	private static final Set<String> EXTENSIONS = new HashSet<String>(Arrays.asList(
			"SPC", "GYM", "NSF", "NSFE", "GBS", "AY", "SAP", "VGM", "VGZ",
			"HES", "KSS"));

	private long currentSong = 0;


	@Override
	public boolean canHandle(String name) {
		String ext = name.substring(name.indexOf('.') + 1).toUpperCase();
		return EXTENSIONS.contains(ext);
	}

	@Override
	public void setOption(String string, Object val) {
		/* No options */
	}

	@Override
	public String[] getDetailedInfo() {
		List<String> list = new ArrayList<String>();

		String s = N_getStringInfo(currentSong, INFO_TYPE);
		if (s != null && s.length() > 0) {
			list.add("Format:");
			list.add(s);
			list.add("");
		}
		s = N_getStringInfo(currentSong, INFO_COPYRIGHT);
		if (s != null && s.length() > 0) {
			list.add("Copyright:");
			list.add(s);
			list.add("");
		}
		s = N_getStringInfo(currentSong, INFO_GAME);
		if (s != null && s.length() > 0) {
			list.add("Game:");
			list.add(s);
			list.add("");
		}
		
		s = N_getStringInfo(currentSong, INFO_COMMENT);
		if (s != null && s.length() > 0) {
			list.add("Comments:");
			list.add(s);
			list.add("");
		}

		String[] info = new String[list.size()];
		for (int i = 0; i < info.length; i++) {
			info[i] = list.get(i);
		}

		return info;
	}

	@Override
	protected boolean load(String name, byte[] data) {
		currentSong = N_load(data, data.length);
		return currentSong != 0;
	}

	public boolean loadInfo(File file) {
		currentSong = N_loadFile(file.getPath());
		return currentSong != 0;
	}

	@Override
	public void unload() {
		N_unload(currentSong);
		currentSong = 0;
	}
	
	@Override
	public boolean seekTo(int seconds) {
		return N_seekTo(currentSong, seconds);
	}
	
	@Override
	public boolean canSeek() {
		return true;
	}

	@Override
	public String getVersion() {
		return "Game Music Emu v0.5.5";
	}

	// Expects Stereo, 44.1Khz, signed, big-endian shorts
	@Override
	public int getSoundData(short[] dest) {
		return N_getSoundData(currentSong, dest, dest.length);
	}

	@Override
	public boolean setTune(int tune) {
		return N_setTune(currentSong, tune);
	}

	@Override
	protected MusicInfo getMusicInfo(String name, byte[] module) {
		if (module.length < 27) {
			return null;
		}

		String magic;
		magic = new String(module, 0, 4, ISO88591);
		if (magic.equals("NESM")) {
			MusicInfo info = new MusicInfo();
			info.title = fromData(module, 0xe, 32);
			info.composer = fromData(module, 0x2e, 32);
			info.copyright = fromData(module, 0x4e, 32);
			info.format = "NES";
			return info;
		}
		
		//TODO: Make this get the composer and title
		magic = new String(module, 0, 4, ISO88591);
		if (magic.equals("NSFE")) {
			MusicInfo info = new MusicInfo();
			info.format = "NES";
			return info;
		}

		magic = new String(module, 0, 27, ISO88591);
		if (magic.equals("SNES-SPC700 Sound File Data")) {
			MusicInfo info = new MusicInfo();
			info.format = "SNES";
			if (module[0x23] == 0x1a) {
				info.title = fromData(module, 0x2e, 32);
				String game = fromData(module, 0x4e, 32);
				if (game.length() > 0) {
					info.title = game + " - " + info.title;
				}
				info.composer = fromData(module, 0xb1, 32);
			}
			return info;
		}

		return null;
	}

	@Override
	public int getIntInfo(int what){
	    return N_getIntInfo(currentSong, what);
	}

	native private long N_load(byte[] module, int size);

	native private long N_loadFile(String name);

	native private void N_unload(long song);

	// Expects Stereo, 44.1Khz, signed, big-endian shorts
	native private int N_getSoundData(long song, short[] dest, int size);

	native private boolean N_seekTo(long song, int seconds);

	native private boolean N_setTune(long song, int tune);

	native private String N_getStringInfo(long song, int what);

	native private int N_getIntInfo(long song, int what);

}