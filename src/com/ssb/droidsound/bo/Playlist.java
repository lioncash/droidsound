package com.ssb.droidsound.bo;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.net.Uri;

/**
 * Represents snapshots of database entries.
 * ID is not provided because it is not reliable (could change from scan to scan).
 *
 * @author alankila
 */
public class Playlist {
	private static final Charset UTF8 = Charset.forName("UTF-8");
	private static final int VERSION = 1;

	private final long id;
	private final File file;
	private final List<FilesEntry> songs;

	private String title;

	public Playlist(long id, File file) {
		this.id = id;
		this.file = file;

		title = file.getName();
		int dot = title.lastIndexOf('.');
		if(dot > 0) {
			title = title.substring(0, dot);
		}

		songs = new ArrayList<FilesEntry>();
		try {
			byte[] data = new byte[(int) file.length()];
			if (data.length != 0) {
				DataInputStream dis = new DataInputStream(new FileInputStream(file));
				dis.readFully(data);
				dis.close();

				JSONArray json = new JSONArray(new String(data, UTF8));
				for (int i = 0; i < json.length(); i ++) {
					JSONObject obj = json.getJSONObject(i);
					FilesEntry sf = deserialize(obj);
					if (sf != null) {
						songs.add(sf);
					}
				}
			}
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	 }

	public void persist() {
		try {
			FileOutputStream fos = new FileOutputStream(file);
			JSONArray arr = new JSONArray();
			for (FilesEntry s : songs) {
				JSONObject obj = serialize(s);
				arr.put(obj);
			}
			fos.write(arr.toString().getBytes(UTF8));
			fos.close();
		} catch (Exception e) {
			throw new RuntimeException(e);
		}
	}

	private static JSONObject serialize(FilesEntry songFile) throws JSONException {
		JSONObject obj = new JSONObject();
		obj.put("url", String.valueOf(songFile.getUrl()));
		obj.put("format", songFile.getFormat());
		obj.put("title", songFile.getTitle());
		obj.put("composer", songFile.getComposer());
		obj.put("date", songFile.getDate());
		obj.put("version", VERSION);
		return obj;
	}

	private static FilesEntry deserialize(JSONObject obj) throws JSONException {
		if (obj.has("version") && obj.getInt("version") == VERSION) {
			return new FilesEntry(
					0,
					Uri.parse(obj.getString("url")),
					(obj.has("format") ? obj.getString("format") : null),
					(obj.has("title") ? obj.getString("title") : null),
					(obj.has("composer") ? obj.getString("composer") : null),
					obj.getInt("date")
			);
		} else {
			return null;
		}
	}

	public long getId() {
		return id;
	}

	public String getTitle() {
		return title;
	}

	public List<FilesEntry> getSongs() {
		return songs;
	}
}