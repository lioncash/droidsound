package com.ssb.droidsound.plugins;

import java.util.Arrays;
import java.util.List;

import com.ssb.droidsound.utils.HashUtil;

/**
 * Base class for the implementation of different
 * sound playback plugins.
 */
public abstract class DroidSoundPlugin {

	/**
	 * Music info about a track.
	 */
	public static class MusicInfo
	{
		/** Title of the track */
		public String title;
		/** Artist/Composer of the track */
		public String composer;
		/** Copyright of the track */
		public String copyright;
		/** File format of the track */
		public String format;

		/** Number of sound channels the track uses */
		public int channels;
		/** Date the track was released */
		public int date = -1;
	}

	private static final MusicInfo EMPTY_INFO = new MusicInfo();

	private static final List<DroidSoundPlugin> PLUGINS = Arrays.asList(
			new VICEPlugin(),
			new GMEPlugin(),
			new SC68Plugin(),
			new SexyPSFPlugin(),
			new VGMStreamPlugin(),
			new UADEPlugin(),
			new ModPlugin(),
			new HivelyPlugin()
	);

	public static final int INFO_TITLE = 0;
	public static final int INFO_AUTHOR = 1;
	public static final int INFO_LENGTH = 2;
	public static final int INFO_TYPE = 3;
	public static final int INFO_COPYRIGHT = 4;
	public static final int INFO_GAME = 5;
	public static final int INFO_SUBTUNE_COUNT = 6;
	public static final int INFO_STARTTUNE = 7;
	public static final int INFO_SUBTUNE_TITLE = 8;
	public static final int INFO_SUBTUNE_AUTHOR = 9;
	public static final int INFO_SUBTUNE_NO = 10;
	public static final int INFO_COMMENT = 11;

	public static final int OPT_FILTER = 1;
	public static final int OPT_RESAMPLING = 2;
	public static final int OPT_NTSC = 3;
	public static final int OPT_SPEED_HACK = 4;
	public static final int OPT_PANNING = 5;
	public static final int OPT_FILTER_BIAS = 6;
	public static final int OPT_SID_MODEL = 7;

	/**
	 * Gets a list of all the currently supported plugins.
	 *
	 * @return A list of all the currently supported plugins.
	 */
	public static List<DroidSoundPlugin> getPluginList() {
		return PLUGINS;
	}

	/**
	 * Performs an MD5 checksum check upon the given data.
	 *
	 * @param data The data to perform MD5 on.
	 *
	 * @return The resulting MD5 checksum hash.
	 */
	public byte[] md5(byte[] data) {
		return HashUtil.md5(data);
	}

	/**
	 * Checks whether the filename of the file (given as 'name')
	 * contains an extension that is supported by the plugin.
	 *
	 * @param name Filename to check.
	 *
	 * @return True, if the file is supported by this plugin; false otherwise.
	 */
	public abstract boolean canHandle(String name);

	/**
	 * Loads a given file.
	 *
	 * @param name   Name of the file (not necessarily required for loading the file).
	 * @param module Data of the file to load.
	 *
	 * @return True, if the file was loaded successfully; false otherwise.
	 */
	protected abstract boolean load(String name, byte[] module);

	/**
	 * Loads two given files that support the use of stereo playback via two files.
	 *
	 * @param f1    First file
	 * @param data1 Data of the first file.
	 * @param f2    Second file.
	 * @param data2 Data of the second file.
	 *
	 * @return True, if the file(s) could be loaded; false otherwise.
	 */
	public boolean load(String f1, byte[] data1, String f2, byte[] data2) {
		if (f2 != null) {
			throw new RuntimeException("This plugin is not handling a 2nd file.");
		}
		return load(f1, data1);
	}

	/**
	 * Gets the sample data for the track currently being played.
	 *
	 * @param dest Destination sample buffer.
	 *
	 * @return The amount of generated samples.
	 */
	public abstract int getSoundData(short[] dest);

	/**
	 * Handles the unloading of any native-side allocations, etc.
	 */
	public abstract void unload();

	/**
	 * Retrieves integer info based on the the given 'what'.
	 * This can be things like song length, subtune number, etc.
	 *
	 * @param what Identifier of the integer information to get.
	 *
	 * @return Integer information corresponding to the given 'what'.
	 */
	public int getIntInfo(int what) {
		return 0;
	}

	/**
	 * Seeks 'msec' milliseconds into the track.
	 *
	 * @param msec Number of milliseconds to seek into the track.
	 *
	 * @return Whether or not seeking 'msec' milliseconds was possible.
	 */
	public boolean seekTo(int msec) {
		return false;
	}

	/**
	 * Sets the tune to play.
	 *
	 * @param tune The numeric offset of the tune to play.
	 *
	 * @return True, if the tune was successfully set; false otherwise.
	 */
	public boolean setTune(int tune) {
		return false;
	}

	/**
	 * Retrieves detailed info about the currently loaded track/tune.
	 *
	 * @return Detailed info about the currently loaded track/tune.
	 */
	public String[] getDetailedInfo() {
		return null;
	}

	/**
	 * Enables the setting of options on the native-side of the plugin.
	 *
	 * @param string Key name of the value to set.
	 * @param val    The value to set to the given key.
	 */
	public abstract void setOption(String string, Object val);

	/**
	 * Whether or not this plugin is able to perform seeking.
	 *
	 * @return True, if the plugin support seeking; false otherwise.
	 */
	public boolean canSeek() {
		return false;
	}

	/**
	 * Gets the current plugin version.
	 *
	 * @return The current plugin version.
	 */
	public abstract String getVersion();

	/**
	 * Retrieves format-specific information about the currently loaded track.
	 *
	 * @param name   The name of the track/tune file.
	 * @param module The data of the track/tune file.
	 *
	 * @return The {@link MusicInfo} instance relating to the current track/tune.
	 */
	protected abstract MusicInfo getMusicInfo(String name, byte[] module);

	private static void fixInfo(String basename, MusicInfo info) {
		if (info.composer != null) {
			info.composer = info.composer.trim();
			if ("".equals(info.composer)) {
				info.composer = null;
			}
		}

		if (info.title != null) {
			info.title = info.title.trim();
			if ("".equals(info.title)) {
				info.title = null;
			}
		}

		if (info.title == null) {
			info.title = basename;
		}

		if (info.date == -1 && info.copyright != null && info.copyright.length() >= 4) {
			info.date = 0;
			try {
				int year = Integer.parseInt(info.copyright.substring(0,4));
				if (year > 1000 && year < 2100) {
					info.date = year * 10000;
				}
			} catch (NumberFormatException e) {
			}
		}
	}

	public static MusicInfo identify(String name1, byte[] module1) {
		boolean handle = false;
		for (DroidSoundPlugin plugin : DroidSoundPlugin.getPluginList()) {
			if (plugin.canHandle(name1)) {
				handle = true;
				MusicInfo info = plugin.getMusicInfo(name1, module1);
				if (info != null) {
					fixInfo(name1, info);
					return info;
				}
			}
		}
		return handle ? EMPTY_INFO : null;
	}

	/**
	 * Gets the sampling frequency of the currently loaded track.
	 * <P>
	 * Possibly used when sampling frequencies for different tracks
	 * might not consistently be the same.
	 *
	 * @return Sampling frequency of the currently loaded track.
	 */
	public int getFrameRate() {
		return 44100;
	}
}
