package com.ssb.droidsound.utils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import android.content.res.AssetManager;

import com.ssb.droidsound.app.Application;

public class Unzipper {
	public static void unzipAsset(String assetName, File path) {
		try {
			AssetManager am = Application.getAssetManager();
			InputStream is = am.open(assetName);
			ZipInputStream zis = new ZipInputStream(is);
			ZipEntry ze;
			byte[] buffer = new byte[1024];
			while (null != (ze = zis.getNextEntry())) {
				String name = ze.getName();
				File out = new File(path, name);
				if (name.endsWith("/")) {
					out.mkdir();
					continue;
				}

				OutputStream os = new FileOutputStream(out);
				int data;
				while (0 < (data = zis.read(buffer))) {
					os.write(buffer, 0, data);
				}
				os.close();

				zis.closeEntry();
			}
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
	}
}