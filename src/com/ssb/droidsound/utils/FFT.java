package com.ssb.droidsound.utils;

public class FFT {
	static {
		System.loadLibrary("fft");
	}

	/**
	 * Calculates the complex spectrum from real-valued input.
	 *
	 * Returns [Re(0), Re(N/2), Re(0), Im(0), Re(1), Im(1), ...]
	 */
	public static native void fft(float[] inout);
}