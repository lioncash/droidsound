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
	public static class Data {
		private final int frameRate;
		private final Long time;
		private final int index;
		private final short[] fft;

		public Data(int frameRate, long time, int index, short[] fft) {
			this.frameRate = frameRate;
			this.time = time;
			this.index = index;
			this.fft = fft;
		}

		public int getFrameRate() {
			return frameRate;
		}

		public long getTime() {
			return time;
		}

		public int getIndex() {
			return index;
		}

		public short[] getFft() {
			return fft;
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
	private final short[][] fftSamples = new short[4][1024];

	/** Resamplers */
	private final Downsample2Octaves[] resampler = new Downsample2Octaves[] {
		new Downsample2Octaves(), new Downsample2Octaves(), new Downsample2Octaves()
	};

	/** Current index within accumulation buffer */
	private int fftSamplesIdx;

	/** Number of second level resamplings performed. */
	private int secondLevelLatch;

	/** Cache for the 3rd FFT */
	private short[] thirdFft = new short[512];

	///** Number of third level resamplings performed. */
	//private int thirdLevelLatch;

	///** Cache for the 4th FFT */
	//private short[] fourthFft = new short[512];

	/** Audio output rate */
	private int frameRate;

	/** Length of data buffering */
	private int bufferingMs;

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
	private static void resample2oct(Downsample2Octaves resampler, short[] in, short[] out) {
		final int len = in.length;
		final int overlapLength = len >> 2;
		System.arraycopy(out, overlapLength, out, 0, len - overlapLength);

		final int inoffset = len - (overlapLength << 2);
		for (int i = 0; i < overlapLength; i ++) {
			int j = inoffset + (i << 2);
			out[len - overlapLength + i] = resampler.input(in[j], in[j + 1], in[j + 2], in[j + 3]);
		}
	}

	/**
	 * Run FFTs simulating 1024, 4096, 16384 etc. points
	 *
	 * @param posInSamples
	 * @param lengthInSamples
	 */
	private void runFfts(int posInSamples, int lengthInSamples) {
		/* Consume old junk if the visualizer isn't keeping up */
		while (true) {
			Data d = queue.peek();
			if (d != null && d.getTime() + bufferingMs < System.currentTimeMillis()) {
				queue.poll();
				continue;
			}
			break;
		}

		/* Estimated time when the current head of audio buffer will play back */
		long time = System.currentTimeMillis() + bufferingMs + 1000 * (posInSamples - lengthInSamples) / 2 / frameRate;

		short[] firstFft = new short[512];
		FFT.fft(fftSamples[0], firstFft);

		/* Always generate 2nd level (requisite 25 % overlap reached) */
		resample2oct(resampler[0], fftSamples[0], fftSamples[1]);
		short[] secondFft = new short[512];
		FFT.fft(fftSamples[1], secondFft);

		if (true) { //++ secondLevelLatch == 4) {
			resample2oct(resampler[1], fftSamples[1], fftSamples[2]);
			secondLevelLatch = 0;
			thirdFft = new short[512];
			FFT.fft(fftSamples[2], thirdFft);

			queue.add(new Data(frameRate, time - 1000 * 8192 / frameRate, 2, thirdFft));

			/*
			if (++ thirdLevelLatch == 4) {
				resample2oct(resampler[2], fftSamples[2], fftSamples[3]);
				thirdLevelLatch = 0;
				fourthFft = new short[512];
				FFT.fft(fftSamples[3], fourthFft);

				queue.add(new Data(frameRate, time - 1000 * 32768 / frameRate, 3, fourthFft));
			}*/
		}

		/* Send new data packet. */
		queue.add(new Data(frameRate, time - 1000 * 2048 / frameRate, 1, secondFft));
		queue.add(new Data(frameRate, time - 1000 * 512 / frameRate, 0, firstFft));
	}

	public Queue<Data> getQueue() {
		return queue;
	}

	public void calculateTiming(int frameRate, int bufferFrames) {
		this.frameRate = frameRate;
		bufferingMs = bufferFrames * 1000 / frameRate;
	}
}