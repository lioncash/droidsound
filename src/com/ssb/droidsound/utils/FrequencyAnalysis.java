package com.ssb.droidsound.utils;

import java.util.PriorityQueue;
import java.util.Queue;

/**
 * Input signal is converted to mono and FFT is taken with overlap.
 * Change of phase within frequency bin is used to enhance the estimate of
 * the frequency within the bin.
 *
 * @author alankila
 */
public class FrequencyAnalysis {
	public static class Data implements Comparable<Data> {
		private final long time;
		private final float[] fft;

		public Data(long time, float[] fft) {
			this.time = time;
			this.fft = fft;
		}

		public long getTime() {
			return time;
		}

		public float[] getFrequencies() {
			return fft;
		}

		@Override
		public int compareTo(Data other) {
			if (time < other.time) {
				return -1;
			}
			if (time > other.time) {
				return 1;
			}
			return 0;
		}
	}

	/** Queue of generated FFT frames for later processing. */
	private final Queue<Data> queue = new PriorityQueue<Data>();

	/** Input samples buffer */
	private final float[] sample = new float[2048];

	/** Window */
	private final float[] window = new float[2048];

	/** Redo FFT after overlap samples */
	private final int overlap = 768;

	/** FFT buffer 1 */
	private final float[] fft1 = new float[2048];

	/** FFT buffer 2 */
	private final float[] fft2 = new float[2048];

	/** Which FFT buffer is more recent? */
	private boolean swap;

	/** Current index within accumulation buffer */
	private int sampleIdx;

	/** Audio output rate */
	private int frameRate;

	/** Length of data buffering */
	private int bufferingMs;

	public FrequencyAnalysis() {
		for (int i = 0; i < window.length; i += 1) {
			window[i] = (float) (0.53836 - 0.46164 * Math.cos(i * 2 * Math.PI / (window.length - 1)));
		}
	}

	/**
	 * Read interleaved stereo sample data
	 *
	 * @param samples input array
	 * @param posInSamples start offset in input array
	 * @param lengthInSamples number of samples to read
	 */
	public void feed(short[] samples, int posInSamples, int lengthInSamples) {
		/* Estimated time when the current head of audio buffer will play back */
		long time = System.currentTimeMillis() + bufferingMs;
		for (int i = posInSamples; i < posInSamples + lengthInSamples; i += 2) {
			float mono = samples[i] + samples[i + 1];
			sample[sampleIdx] = mono;
			if (++ sampleIdx == sample.length) {
				runFfts(time + 1000 * (i - posInSamples - lengthInSamples) / 2 / frameRate);
				System.arraycopy(sample, overlap, sample, 0, sample.length - overlap);
				sampleIdx = sample.length - overlap;
			}
		}
	}

	/**
	 * Run FFTs at 1024 points
	 *
	 * @param time the expected playback time of the input buffer
	 */
	private void runFfts(long time) {
		/* Consume old junk if the visualizer isn't keeping up */
		synchronized (queue) {
			while (true) {
				Data d = queue.peek();
				if (d != null && d.getTime() + bufferingMs < System.currentTimeMillis()) {
					queue.poll();
					continue;
				}
				break;
			}
		}

		float[] fft1 = swap ? this.fft1 : this.fft2;
		float[] fft2 = swap ? this.fft2 : this.fft1;
		swap = !swap;

		for (int i = 0; i < sample.length; i += 1) {
			fft2[i] = sample[i] * window[i];
		}
		FFT.fft(fft2);

		double minfreq = 27.5; /* A = 440, 220, 110, 55, 27.5 */
		float[] bins = new float[12 * 9 * 3]; /* 12 notes per octave, 9 octaves, 3 bins per note */

		for (int i = 1; i < (fft2.length >> 1) - 1; i += 1) {
			double phase1 = Math.atan2(fft1[i * 2 + 1], fft1[i * 2]);
			double phase2 = Math.atan2(fft2[i * 2 + 1], fft2[i * 2]);
			double phase = phase1 - phase2;
			/* phase has range -2pi to 2pi */

	        /* what is the expected phase difference at overlaps? */
	        double zerobin = fract((double) overlap * i / sample.length) * 2 * Math.PI;
	        double binwidth = (double) overlap / sample.length * 2 * Math.PI;
	        /* zerobin has range 0 .. 2pi */

	        /* Select phase interpretation that minimizes phase-zerobin */
	        if (phase < zerobin - Math.PI) {
	        	phase += 2 * Math.PI;
	        } else if (phase > zerobin + Math.PI) {
	        	phase -= 2 * Math.PI;
	        }

	        double estfreq = frameRate * (i + (phase - zerobin) / binwidth) / fft2.length;
	        if (estfreq <= 0) {
	        	continue;
	        }
	        int note = (int) Math.round(Math.log(estfreq / minfreq) / Math.log(2) * 12 * 3 + 1);
	        if (note >= 0 && note < bins.length) {
	        	float re = fft2[i * 2];
	        	float im = fft2[i * 2 + 1];
	        	float magnitude = (float) (Math.sqrt(re * re + im * im) / 65536.0);
	        	bins[note] += magnitude;
	        }
		}

		synchronized (queue) {
			queue.add(new Data(time - 1000 * (sample.length >> 1) / frameRate, bins));
		}
	}

	private double fract(double x) {
		return x - Math.floor(x);
	}

	public Queue<Data> getQueue() {
		return queue;
	}

	public void calculateTiming(int frameRate, int bufferFrames) {
		this.frameRate = frameRate;
		bufferingMs = bufferFrames * 1000 / frameRate;
	}
}
