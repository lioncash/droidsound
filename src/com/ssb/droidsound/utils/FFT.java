package com.ssb.droidsound.utils;

public class FFT {
	static {
		System.loadLibrary("fft");
	}

	/**
	 * Calculates the complex spectrum from real-valued input.
	 *
	 * Returns [Re(0), Re(1), Im(1), Re(2), Im(2), ..., Re(N/2)]
	 */
	public static native void fft(float[] inout);
}