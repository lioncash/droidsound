package com.ssb.droidsound.utils;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * Input signal is converted to mono and FFTs are run at multiple resolutions
 * to provide overall coverage from 55 Hz and above.
 *
 * @author alankila
 */
public class OverlappingFFT {
	public static class Data implements Comparable<Data> {
		private final Long time;
		private final short[][] fft;

		public Data(long time, short[][] fft) {
			this.time = time;
			this.fft = fft;
		}

		public long getTime() {
			return time;
		}

		public short[][] getFft() {
			return fft;
		}

		@Override
		public int compareTo(Data other) {
			return time.compareTo(other.time);
		}
	}

	/** Queue of generated FFT frames for later processing. */
	private final Queue<Data> queue = new ConcurrentLinkedQueue<Data>();

	/**
	 * Accumulation buffers until full FFT has been reached. 16-bit mono.
	 * The buffer at index 0 has 44.1 kHz sample rate, buffer at index 1
	 * has 11.1 kHz and so on.
	 */
	private final short[][] fftSamples = new short[4][1024];

	/** Current index within accumulation buffer */
	private int fftSamplesIdx;

	/** Audio output rate */
	private final int frameRate;

	/** Length of data buffering */
	private final int bufferingMs;

	public OverlappingFFT(int bufsizeFrames, int frameRate) {
		this.frameRate = frameRate;
		bufferingMs = bufsizeFrames * 1000 / frameRate;
	}

	/*
	double phase = 0;
	double phaseInc = 0;
	*/

	public void feed(short[] samples, int posInSamples, int lengthInSamples) {
		for (int i = 0; i < lengthInSamples; i += 2) {
			int mono = samples[posInSamples + i] + samples[posInSamples + i + 1];

			/*
			mono = (int) (Math.sin(phase) * 65535 / 2);
			phase += phaseInc;
			*/

			fftSamples[0][fftSamplesIdx] = (short) (mono >> 1);
			if (++ fftSamplesIdx == fftSamples[0].length) {
				runFfts(posInSamples, lengthInSamples);
				fftSamplesIdx = 0;
			}
		}
		/*
		phaseInc *= 1.003;
		if (phaseInc < 2e-3 || phaseInc > 1) {
			phaseInc = 2e-3;
		}
		*/
	}

	/**
	 * Move 1/4 of the buffer into bit bucket
	 * fill in the empty space at end from new data
	 *
	 * @param in
	 * @param out
	 */
	private static void resample2oct(short[] in, short[] out) {
		int len = in.length;
		System.arraycopy(out, len >> 2, out, 0, len - (len >> 2));
		for (int i = 0; i < len; i += 4) {
			int boxcar = in[i] + in[i + 1] + in[i + 2] + in[i + 3];
			out[len - (len >> 2) + (i >> 2)] = (short) (boxcar >> 2);
		}
	}

	/**
	 * Run FFTs simulating 1024, 4096, 16384 and 65536 points
	 *
	 * @param posInSamples
	 * @param lengthInSamples
	 */
	private void runFfts(int posInSamples, int lengthInSamples) {
		resample2oct(fftSamples[0], fftSamples[1]);
		resample2oct(fftSamples[1], fftSamples[2]);
		resample2oct(fftSamples[2], fftSamples[3]);

		short[][] out = new short[4][fftSamples[0].length >> 1];
		FFT.fft(fftSamples[0], out[0]);
		FFT.fft(fftSamples[1], out[1]);
		FFT.fft(fftSamples[2], out[2]);
		FFT.fft(fftSamples[3], out[3]);

		while (true) {
			Data d = queue.peek();
			if (d != null && d.getTime() + bufferingMs < System.currentTimeMillis()) {
				queue.poll();
				continue;
			}
			break;
		}

        long estimatedPlaybackTime = System.currentTimeMillis() + bufferingMs;
		long time = estimatedPlaybackTime + 1000 * (posInSamples - lengthInSamples) / 2 / frameRate;
		queue.add(new Data(time, out));
	}

	public Queue<Data> getQueue() {
		return queue;
	}
}