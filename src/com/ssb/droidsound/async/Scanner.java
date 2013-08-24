package com.ssb.droidsound.async;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipInputStream;

import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteStatement;
import android.net.Uri;
import android.os.AsyncTask;
import android.provider.BaseColumns;

import com.ssb.droidsound.app.Application;
import com.ssb.droidsound.bo.FilesEntry;
import com.ssb.droidsound.bo.Playlist;
import com.ssb.droidsound.database.SongDatabase;
import com.ssb.droidsound.plugins.DroidSoundPlugin;
import com.ssb.droidsound.utils.Log;
import com.ssb.droidsound.utils.StreamUtil;

/**
 * Perform a collection scan against a writable database instance.
 * This class used to be a Service until I decided that android's
 * conception of Services is so useless (due to lack of synchronous
 * bindService) that I am no longer willing to accept the level of
 * bullshit it takes to use them.
 *
 * @author alankila
 */
public class Scanner extends AsyncTask<Void, Intent, Void> {
	public static final String ACTION_SCAN = "com.ssb.droidsound.SCAN";

	private static AtomicBoolean scanning = new AtomicBoolean();
	private static final String TAG = Scanner.class.getSimpleName();
	private static final Charset ISO88591 = Charset.forName("ISO-8859-1");

	private final SQLiteDatabase db;
	private final boolean full;

	private final SQLiteStatement filesStatement;
	private final int FILES_PARENT_ID = 1;
	private final int FILES_FILENAME = 2;
	private final int FILES_MODIFY_TIME = 3;
	private final int FILES_TYPE = 4;
	private final int FILES_URL = 5;
	private final int FILES_TITLE = 6;
	private final int FILES_COMPOSER = 7;
	private final int FILES_DATE = 8;
	private final int FILES_FORMAT = 9;

	private final SQLiteStatement songlengthStatement;
	private final int SONGLENGTH_FILE_ID = 1;
	private final int SONGLENGTH_MD5 = 2;
	private final int SONGLENGTH_SUBSONG = 3;
	private final int SONGLENGTH_TIME_MS = 4;

	public Scanner(SQLiteDatabase db, boolean full) {
		this.db = db;
		this.full = full;

		filesStatement = db.compileStatement("INSERT INTO files (parent_id, filename, modify_time, type, url, title, composer, date, format) VALUES (?, ?, ?, ?, ?, ? ,?, ?, ?)");
		songlengthStatement = db.compileStatement("INSERT INTO songlength (file_id, md5, subsong, timeMs) VALUES (?, ?, ?, ?)");
	}

	@Override
	protected void onPreExecute() {
		if (scanning.compareAndSet(false, true)) {
			Application.payProtectionMoney(1);
		} else {
			cancel(false);
		}
	}

	@Override
	protected void onProgressUpdate(Intent... intents) {
		for (Intent i : intents) {
			Application.broadcast(i);
		}
	}

	private void sendUpdate(int pct) {
		Intent intent = new Intent(ACTION_SCAN);
		intent.putExtra("progress", pct);
		intent.putExtra("scanning", true);
		publishProgress(intent);
	}

	@Override
	protected Void doInBackground(Void... ignored) {
		try {
			Log.i(TAG, "Starts.");
			doScan(Application.getModsDirectory());
			Log.i(TAG, "Ends.");
		} catch (IOException e) {
			Log.w(TAG, "Unable to finish scan", e);
		}
		return null;
	}

	@Override
	protected void onPostExecute(Void result) {
		Intent endIntent = new Intent(ACTION_SCAN);
		endIntent.putExtra("progress", 100);
		endIntent.putExtra("scanning", false);
		Application.broadcast(endIntent);
		Application.payProtectionMoney(-1);
		scanning.set(false);
	}

	private void bind(SQLiteStatement stmt, int key) {
		stmt.bindNull(key);
	}

	private void bind(SQLiteStatement stmt, int key, String obj) {
		if (obj == null) {
			bind(stmt, key);
		} else {
			stmt.bindString(key, obj);
		}
	}

	private void bind(SQLiteStatement stmt, int key, Long value) {
		if (value == null) {
			bind(stmt, key);
		} else {
			stmt.bindLong(key, value);
		}
	}

	private void bind(SQLiteStatement stmt, int key, Integer value) {
		if (value == null) {
			bind(stmt, key);
		} else {
			stmt.bindLong(key, value);
		}
	}

	/**
	 * Insert a directory node to a parent.
	 *
	 * @param name
	 * @param parentId
	 * @return rowid
	 */
	private long insertDirectory(String name, Long parentId) {
		bind(filesStatement, FILES_PARENT_ID, parentId);
		bind(filesStatement, FILES_FILENAME, name);
		bind(filesStatement, FILES_MODIFY_TIME);
		bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_DIRECTORY);
		bind(filesStatement, FILES_URL);
		bind(filesStatement, FILES_TITLE, name);
		bind(filesStatement, FILES_COMPOSER);
		bind(filesStatement, FILES_DATE);
		bind(filesStatement, FILES_FORMAT);
		return filesStatement.executeInsert();
	}

	private static String makeZipUrl(File zipFile, File songFile) {
		return "zip://" + Uri.encode(zipFile.getAbsolutePath(), "/")
				+ "?path=" + Uri.encode(songFile.getAbsolutePath());
	}

	private static String makeFileUrl(File songFile) {
		return "file://" + Uri.encode(songFile.getAbsolutePath(), "/");
	}

	/**
	 * Insert a file node to a parent.
	 *
	 * @param songFile
	 * @param data
	 * @param modifyTime
	 * @param parentId
	 */
	private void insertFile(File zipFile, File songFile, byte[] data, long modifyTime, Long parentId) {
		/* We need a positive identification for accepting a file. */
		DroidSoundPlugin.MusicInfo info = DroidSoundPlugin.identify(songFile.getName(), data);
		if (info == null) {
			return;
		}

		String url = zipFile != null ? makeZipUrl(zipFile, songFile) : makeFileUrl(songFile);

		bind(filesStatement, FILES_PARENT_ID, parentId);
		bind(filesStatement, FILES_FILENAME, songFile.getName());
		bind(filesStatement, FILES_MODIFY_TIME, modifyTime);
		bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_FILE);
		bind(filesStatement, FILES_URL, url);
		bind(filesStatement, FILES_TITLE, info.title != null ? info.title : songFile.getName());
		bind(filesStatement, FILES_COMPOSER, info.composer != null ? info.composer : songFile.getParentFile().getName());
		bind(filesStatement, FILES_DATE, info.date);
		bind(filesStatement, FILES_FORMAT, info.format);
		filesStatement.executeInsert();
	}

	/**
	 * Insert a playlist node.
	 * <p>
	 * The playlist node is a bit tricky because it encodes the full path to the file
	 * sans the base directory of the collection. They are also serialized in different
	 * kind of files and only cached in db.
	 *
	 * @param rowId
	 * @param f
	 */
	private void insertPlaylist(long rowId, File f) {
		Playlist pl = new Playlist(f);

		int i = 0;
		for (FilesEntry sf : pl.getSongs()) {
			bind(filesStatement, FILES_PARENT_ID, rowId);
			bind(filesStatement, FILES_FILENAME, String.valueOf(++ i));
			bind(filesStatement, FILES_MODIFY_TIME);
			bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_FILE);
			bind(filesStatement, FILES_URL, String.valueOf(sf.getUrl()));
			bind(filesStatement, FILES_TITLE, sf.getTitle());
			bind(filesStatement, FILES_COMPOSER, sf.getComposer());
			bind(filesStatement, FILES_DATE, sf.getDate());
			bind(filesStatement, FILES_FORMAT, sf.getFormat());
			filesStatement.executeInsert();
		}
	}

	/**
	 * Zip scanner
	 *
	 * @param zipFile
	 * @return
	 * @throws ZipException
	 * @throws IOException
	 */
	private void scanZip(File zipFile, Long parentId) throws ZipException, IOException {
		Log.i(TAG, "Scanning ZIP %s for files...", zipFile.getPath());
		db.execSQL("PRAGMA foreign_keys=ON");
		db.delete("files", "parent_id = ?", new String [] { String.valueOf(parentId) });
		db.execSQL("PRAGMA foreign_keys=OFF");

		FileInputStream fis = new FileInputStream(zipFile);
		ZipInputStream zis = new ZipInputStream(fis);
		Map<String, Long> pathMap = new HashMap<String, Long>();
		pathMap.put("", parentId);
		ZipEntry ze;
		int shownPct = -1;
		while (null != (ze = zis.getNextEntry())) {
			int slash = ze.getName().lastIndexOf('/');
			/** Path inside zip to file */
			final String path;
			/** Name of file */
			final String name;
			if (slash == -1) {
				path = zipFile.getPath();
				name = ze.getName();
			} else {
				path = ze.getName().substring(0, slash);
				name = ze.getName().substring(slash  + 1);
			}
			final long pathParentId;
			if ("".equals(name)) {
				/* Is directory. Pick the part before the last filename */
				int slash2 = path.lastIndexOf('/');
				final String pathFilename;
				if (slash2 == -1) {
					/* top level in zip: parent to given directory */
					pathParentId = parentId;
					pathFilename = path;
				} else {
					/* Parent of the path (if any) */
					String parent = path.substring(0, slash2);
					pathFilename = path.substring(slash2 + 1);
					pathParentId = pathMap.get(parent);
				}

				Log.i(TAG, "New directory: %s", ze.getName());
				long rowId = insertDirectory(pathFilename, pathParentId);
				pathMap.put(path, rowId);
			} else {
				pathParentId = pathMap.get(path);
				if (name.equals("Songlengths.txt")) {
					bind(filesStatement, FILES_PARENT_ID, pathParentId);
					bind(filesStatement, FILES_FILENAME, name);
					bind(filesStatement, FILES_MODIFY_TIME);
					bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_SONGLENGTH);
					bind(filesStatement, FILES_URL);
					bind(filesStatement, FILES_TITLE);
					bind(filesStatement, FILES_COMPOSER);
					bind(filesStatement, FILES_DATE);
					bind(filesStatement, FILES_FORMAT);
					long rowId = filesStatement.executeInsert();
					scanSonglengthsTxt(rowId, zis);
				} else {
					insertFile(zipFile, new File(path, name), StreamUtil.readFully(zis, ze.getSize()), 0, pathParentId);
				}
			}

			int pct = (int) (fis.getChannel().position() / (fis.getChannel().size() / 100));
			if (shownPct != pct) {
				sendUpdate(pct);
			}
			zis.closeEntry();
		}

		zis.close();
	}

	/**
	 * Dir scan. Roots of the tree are those with parentId = null.
	 */
	private void scanFiles(File dir, Long parentId) throws IOException {
		sendUpdate(0);
		String where;
		String[] whereCond;
		if (parentId == null) {
			where = "parent_id IS NULL";
			whereCond = null;
		} else {
			where = "parent_id = ?";
			whereCond = new String[] { String.valueOf(parentId) };
		}
		final Cursor fileCursor = db.query(
				"files",
				new String[] { BaseColumns._ID, "filename", "modify_time" },
				where, whereCond,
				null, null, null
		);

		/** Going to recurse into these */
		Map<Long, File> directoriesToRecurse = new HashMap<Long, File>();

		/**
		 * Set of files in examined directory. Entries are removed from this set
		 * if they are also found on the database, so the remainder corresponds to files to add.
		 */
		List<File> files = new ArrayList<File>();
		for (File f : dir.listFiles()) {
			if (! f.getName().startsWith(".")) {
				files.add(f);
			}
		}

		Set<Long> delFiles = new HashSet<Long>();
		/* Compare db and fs */
		while (fileCursor.moveToNext()) {
			long id = fileCursor.getLong(0);
			File file = new File(dir, fileCursor.getString(1));
			long lastScan = fileCursor.getLong(2);

			if (files.contains(file)) {
				if (file.isDirectory()) {
					directoriesToRecurse.put(id, file);
					files.remove(file);
				} else {
					/* Has recorded modify-time changed? If so, rescan. */
					if (lastScan != file.lastModified()) {
						delFiles.add(id);
					} else {
						files.remove(file);
					}
				}
			} else {
				delFiles.add(id);
			}
		}
		fileCursor.close();

		/* Delete files that need refresh or which did not exist anymore. */
		db.beginTransaction();
		db.execSQL("PRAGMA foreign_keys=ON");
		for (long id : delFiles) {
			db.delete("files", BaseColumns._ID + " = ?", new String[] { String.valueOf(id) } );
		}
		db.execSQL("PRAGMA foreign_keys=OFF");

		List<File> zipsToScan = new ArrayList<File>();
		List<File> songLengthsToDo = new ArrayList<File>();
		List<File> directoriesToAdd = new ArrayList<File>();

		/* Add files/directories that did not exist */
		int pct = 0;
		for (int i = 0; i < files.size(); i ++) {
			File f = files.get(i);
			if (f.isFile()) {
				Log.i(TAG, "New file: %s", f.getPath());
				String fnUpper = f.getName().toUpperCase(Locale.ROOT);
				if (fnUpper.endsWith(".ZIP")) {
					zipsToScan.add(f);
				} else if (fnUpper.endsWith(".PLIST")) {
					bind(filesStatement, FILES_PARENT_ID, parentId);
					bind(filesStatement, FILES_FILENAME, f.getName());
					bind(filesStatement, FILES_MODIFY_TIME, f.lastModified());
					bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_PLAYLIST);
					bind(filesStatement, FILES_URL, f.getAbsolutePath());
					bind(filesStatement, FILES_TITLE, f.getName().substring(0, f.getName().length() - 6));
					bind(filesStatement, FILES_COMPOSER);
					bind(filesStatement, FILES_DATE);
					bind(filesStatement, FILES_FORMAT);
					long rowId = filesStatement.executeInsert();
					insertPlaylist(rowId, f);
				} else if (f.getName().equals("Songlengths.txt")) {
					songLengthsToDo.add(f);
				} else {
					FileInputStream fi = new FileInputStream(f);
					insertFile(null, f, StreamUtil.readFully(fi, f.length()), f.lastModified(), parentId);
					fi.close();
				}
			} else if (f.isDirectory()) {
				Log.i(TAG, "New directory: %s", f.getPath());
				directoriesToAdd.add(f);
			}

			int nextPct = i * 100 / files.size();
			if (pct != nextPct) {
				pct = nextPct;
				sendUpdate(pct);
			}
		}
		db.setTransactionSuccessful();
		db.endTransaction();
		sendUpdate(100);

		for (File f : directoriesToAdd) {
			/* This is unfortunate, we need to create the directory before we can recurse into it.
			 * Thankfully the transactionality here is not critical as we are talking about
			 * real files here, a future scan will look them up just fine. */
			long rowId = insertDirectory(f.getName(), parentId);
			scanFiles(f, rowId);
		}

		for (File f : songLengthsToDo) {
			db.beginTransaction();
			bind(filesStatement, FILES_PARENT_ID, parentId);
			bind(filesStatement, FILES_FILENAME, f.getName());
			bind(filesStatement, FILES_MODIFY_TIME, f.lastModified());
			bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_SONGLENGTH);
			bind(filesStatement, FILES_URL);
			bind(filesStatement, FILES_TITLE, f.getName());
			bind(filesStatement, FILES_COMPOSER);
			bind(filesStatement, FILES_DATE);
			bind(filesStatement, FILES_FORMAT);
			long rowId = filesStatement.executeInsert();
			FileInputStream fi = new FileInputStream(f);
			scanSonglengthsTxt(rowId, fi);
			fi.close();
			db.setTransactionSuccessful();
			db.endTransaction();
		}

		for (File f : zipsToScan) {
			db.beginTransaction();
			bind(filesStatement, FILES_PARENT_ID, parentId);
			bind(filesStatement, FILES_FILENAME, f.getName());
			bind(filesStatement, FILES_MODIFY_TIME, f.lastModified());
			bind(filesStatement, FILES_TYPE, SongDatabase.TYPE_ZIP);
			bind(filesStatement, FILES_URL, "file://" + Uri.encode(f.getAbsolutePath(), "/"));
			bind(filesStatement, FILES_TITLE, f.getName().substring(0, f.getName().length() - 4));
			bind(filesStatement, FILES_COMPOSER);
			bind(filesStatement, FILES_DATE);
			bind(filesStatement, FILES_FORMAT);
			long rowId = filesStatement.executeInsert();
			scanZip(f, rowId);
			db.setTransactionSuccessful();
			db.endTransaction();
		}

		/* Continue scanning into found directories */
		for (Entry<Long, File> e : directoriesToRecurse.entrySet()) {
			scanFiles(e.getValue(), e.getKey());
		}
	}

	private void scanSonglengthsTxt(long fileId, InputStream is) throws IOException {
		BufferedReader br = new BufferedReader(new InputStreamReader(is, ISO88591));

		Pattern md5AndTimes = Pattern.compile("([a-fA-f0-9]{32})=(.*)");
		Pattern timestamps = Pattern.compile("([0-9]{1,2}):([0-9]{2})");

		String line;
		while (null != (line = br.readLine())) {
			Matcher m = md5AndTimes.matcher(line);
			if (! m.matches()) {
				continue;
			}
			String md5 = m.group(1).toLowerCase(Locale.ROOT);
			String times = m.group(2);

			m = timestamps.matcher(times);
			int song = 0;
			while (m.find()) {
				int minutes = Integer.valueOf(m.group(1));
				int seconds = Integer.valueOf(m.group(2));
				bind(songlengthStatement, SONGLENGTH_FILE_ID, fileId);
				bind(songlengthStatement, SONGLENGTH_MD5, md5);
				bind(songlengthStatement, SONGLENGTH_SUBSONG, ++ song);
				bind(songlengthStatement, SONGLENGTH_TIME_MS, (minutes * 60 + seconds) * 1000);
				songlengthStatement.executeInsert();
			}
		}
	}

	private void doScan(File modsDir) throws IOException {
		if (full) {
			sendUpdate(0);
			db.delete("files", null, null);
			sendUpdate(50);
			db.delete("songlength", null, null);
			sendUpdate(100);
		}
		scanFiles(modsDir, null);
	}
}
