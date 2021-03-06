package com.ssb.droidsound.bo;

import java.util.HashMap;
import java.util.Map;

import android.net.Uri;

/**
 * This container represents a playable file at specific subsong
 * setting.
 *
 * @author alankila
 */
public class FilesEntry {
	private static final Map<String, String> MAIN_TO_AUX = new HashMap<String, String>();
	static {
		MAIN_TO_AUX.put("MDAT", "SMPL");
		MAIN_TO_AUX.put("TFX", "SAM");
		MAIN_TO_AUX.put("SNG", "INS");
		MAIN_TO_AUX.put("RJP", "SMP");
		MAIN_TO_AUX.put("JPN", "SMP");
		MAIN_TO_AUX.put("DUM", "INS");
		MAIN_TO_AUX.put("MUS", "STR");
	}

	private final Uri url;
	private final String format;
	private final String title;
	private final String composer;
	private final int date;

	public FilesEntry(Uri url, String format, String title, String composer, int date) {
		this.url = url;
		this.format = format;
		this.title = title;
		this.composer = composer;
		this.date = date;
	}

	/**
	 * This function is tasked with identifying the song file's type from the
	 * url, and replacing the songfile's suffix with a known alternate
	 * suffix.
	 * <p>
	 * This is a layering violation because in theory only the plugin
	 * responsible for the file can make this decision, but given that the
	 * list of possible substitutions is not very large and so on, maybe
	 * we can live with this.
	 */
	public Uri getSecondaryUrl() {
		if ("zip".equals(url.getScheme())) {
			String second = getSecondaryName(url.getQueryParameter("path"));
			if (second == null) {
				return null;
			}
			return Uri.parse("file://" + url.getEncodedPath() + "?path=" + Uri.encode(second));
		} else {
			String second = getSecondaryName(url.getPath());
			if (second == null) {
				return null;
			}
			return Uri.parse("file://" + Uri.encode(second, "/"));
		}
	}

	private String getSecondaryName(String name) {
		{
			int lastDot = name.lastIndexOf('.');
			String ext = name.substring(lastDot + 1);
			String extUpper = ext.toUpperCase();
			if (MAIN_TO_AUX.containsKey(extUpper)) {
				String alt = MAIN_TO_AUX.get(extUpper);
				if (! ext.equals(extUpper)) {
					alt = alt.toLowerCase();
				}
				return name.substring(lastDot + 1) + alt;
			}
		}

		{
			int lastSlash = name.lastIndexOf('/');
			int firstDot = name.indexOf('.', lastSlash + 1);
			if (firstDot != -1) {
				String ext = name.substring(lastSlash + 1, firstDot);
				String extUpper = ext.toUpperCase();
				if (MAIN_TO_AUX.containsKey(extUpper)) {
					String alt = MAIN_TO_AUX.get(extUpper);
					if (! ext.equals(extUpper)) {
						alt = alt.toLowerCase();
					}
					return name.substring(0, lastSlash + 1) + alt + name.substring(firstDot);
				}
			}
		}

		return null;
	}

	public Uri getUrl() {
		return url;
	}

	public String getFormat() {
		return format;
	}

	public String getTitle() {
		return title;
	}

	public String getComposer() {
		return composer;
	}

	public int getDate() {
		return date;
	}

	@Override
	public int hashCode() {
		return url.hashCode();
	}

	@Override
	public boolean equals(Object other) {
		if (other instanceof FilesEntry) {
			return url.equals(((FilesEntry) other).url);
		}
		return false;
	}
}
