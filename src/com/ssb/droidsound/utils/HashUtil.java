package com.ssb.droidsound.utils;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class HashUtil {
	private static final MessageDigest MD5;
	static {
		try {
			MD5 = MessageDigest.getInstance("MD5");
		} catch (NoSuchAlgorithmException e) {
			throw new ExceptionInInitializerError(e);
		}
	}

	public static byte[] md5(byte[] data) {
		return md5(data, 0, data.length);
	}

	public synchronized static byte[] md5(byte[] data, int start, int length) {
		MD5.update(data, start, length);
		return MD5.digest();
	}
}
