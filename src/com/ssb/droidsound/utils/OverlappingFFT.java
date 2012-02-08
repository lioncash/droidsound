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

	public static class Downsample2Octaves {
		/**
		 * This is first half of sinc() * hamming with 25 % transition band,
		 * and approximately 30 dB suppression of aliasing.
		 */
		private static final short[] FILTER = {
			-483,
			-2140,
			7018,
			28374,
		};

		/** Partial evaluation of the filter (first half) */
		private int state;

		/**
		 * Perform 1/4 resampling with 4 input samples yielding 1 output sample
		 *
		 * @param x1
		 * @param x2
		 * @param x3
		 * @param x4
		 * @return resampled output
		 */
		public short input(short x1, short x2, short x3, short x4) {
			int y = state
					+ x1 * FILTER[3]
					+ x2 * FILTER[2]
					+ x3 * FILTER[1]
					+ x4 * FILTER[0];

			/* In the next round, remember the partial state of the filter */
			state = x1 * FILTER[0] + x2 * FILTER[1] + x3 * FILTER[2] + x4 * FILTER[3];
			return (short) (y >> 16);
		}

	}

	/** Queue of generated FFT frames for later processing. */
	private final Queue<Data> queue = new ConcurrentLinkedQueue<Data>();

	/**
	 * Accumulation buffers until full FFT has been reached. 16-bit mono.
	 * The buffer at index 0 has 44.1 kHz sample rate, buffer at index 1
	 * has 11.1 kHz and so on.
	 */
	private final short[][] fftSamples = new short[3][1024];

	/** Resamplers */
	private final Downsample2Octaves[] resampler = new Downsample2Octaves[] {
		new Downsample2Octaves(), new Downsample2Octaves()
	};

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

	public void feed(short[] samples, int posInSamples, int lengthInSamples) {
		for (int i = 0; i < lengthInSamples; i += 2) {
			int mono = samples[posInSamples + i] + samples[posInSamples + i + 1];

			fftSamples[0][fftSamplesIdx] = (short) (mono >> 1);
			if (++ fftSamplesIdx == fftSamples[0].length) {
				runFfts(posInSamples, lengthInSamples);
				fftSamplesIdx = 0;
			}
		}
	}

	/**
	 * Move a portion of the buffer into bit bucket
	 * fill in the empty space at end from new data, resampling it as we go.
	 *
	 * @param in
	 * @param out
	 */
	private static void resample2oct(Downsample2Octaves resampler, final int overlapLength, short[] in, short[] out) {
		final int len = in.length;
		System.arraycopy(out, overlapLength, out, 0, len - overlapLength);

		final int inoffset = len - (overlapLength << 2);
		for (int i = 0; i < overlapLength; i ++) {
			int j = inoffset + (i << 2);
			out[len - overlapLength + i] = resampler.input(in[j], in[j + 1], in[j + 2], in[j + 3]);
		}
	}

	/**
	 * Run FFTs simulating 1024, 4096, 16384 and 65536 points
	 *
	 * @param posInSamples
	 * @param lengthInSamples
	 */
	private void runFfts(int posInSamples, int lengthInSamples) {
		int overlapLength = fftSamples[0].length;
		for (int i = 0; i < fftSamples.length - 1; i ++) {
			overlapLength >>= 2;
			resample2oct(resampler[i], overlapLength, fftSamples[i], fftSamples[i + 1]);
		}

		short[][] out = new short[fftSamples.length][fftSamples[0].length >> 1];
		for (int i = 0; i < fftSamples.length; i ++) {
			FFT.fft(fftSamples[i], out[i]);
		}

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