package com.ssb.droidsound.utils;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * Input signal is converted to mono and FFT is taken with overlap.
 * Change of phase within frequency bin is used to enhance the estimate of
 * the frequency within the bin.
 *
 * @author alankila
 */
public class FrequencyAnalysis {
	public static class Data {
		private final long time;
		private final float[] samples;

		public Data(long time, float[] samples) {
			this.time = time;
			this.samples = samples;
		}

		public long getTime() {
			return time;
		}

		public float[] getSamples() {
			return samples;
		}
	}

	/** Queue of generated FFT frames for later processing. */
	private final Queue<Data> queue = new ConcurrentLinkedQueue<Data>();

	/** Input samples buffer */
	private float[] sample;

	/** Current index within input samples buffer */
	private int sampleIdx;

	/** Redo FFT after overlap samples */
	private final int overlap = 768;

	/** Sample data (historical) */
	private final float[] sampleHistory = new float[2048];

	/** Window */
	private final float[] window = new float[2048];

	/** FFT buffer 1 */
	private final float[] fft1 = new float[2048];

	/** FFT buffer 2 */
	private final float[] fft2 = new float[2048];

	/** Which FFT buffer is more recent? */
	private boolean swap;

	/** Audio output rate */
	private int frameRate;

	/** Length of data buffering */
	private int bufferingMs;

	public FrequencyAnalysis() {
		sample = new float[overlap];
		for (int i = 0; i < window.length; i += 1) {
			window[i] = (float) (0.53836 - 0.46164 * Math.cos(i * 2 * Math.PI / (window.length - 1)));
		}
	}

	/**
	 * Read interleaved stereo sample data, buffer it, then fire it into queue.
	 *
	 * @param samples input array
	 * @param posInSamples start offset in input array
	 * @param lengthInSamples number of samples to read
	 */
	public void feed(short[] samples, int posInSamples, int lengthInSamples) {
		/* Consume old junk if the visualizer isn't keeping up */
		while (true) {
			Data d = queue.peek();
			if (d != null && d.getTime() + 2 * bufferingMs < System.currentTimeMillis()) {
				queue.poll();
				continue;
			}
			break;
		}

		/* Estimated time when the current head of audio buffer will play back */
		long time = System.currentTimeMillis() + bufferingMs;
		for (int i = posInSamples; i < posInSamples + lengthInSamples; i += 2) {
			float mono = samples[i] + samples[i + 1];
			sample[sampleIdx] = mono;
			if (++ sampleIdx == sample.length) {
				Data d = new Data(time + 1000 * (i - (posInSamples + lengthInSamples)) / 2 / frameRate, sample);
				queue.add(d);
				sample = new float[overlap];
				sampleIdx = 0;
			}
		}
	}

	/**
	 * Run FFT for input data (from a processing or UI thread)
	 *
	 * @param time the expected playback time of the input buffer
	 */
	public float[] runFfts(float[] sample) {
		System.arraycopy(sampleHistory, sample.length, sampleHistory, 0, sampleHistory.length - sample.length);
		System.arraycopy(sample, 0, sampleHistory, sampleHistory.length - sample.length, sample.length);

		float[] fft1 = swap ? this.fft1 : this.fft2;
		float[] fft2 = swap ? this.fft2 : this.fft1;
		swap = !swap;

		for (int i = 0; i < sampleHistory.length; i += 1) {
			fft2[i] = sampleHistory[i] * window[i];
		}
		FFT.fft(fft2);

		double minfreq = 55; /* A = 440, 220, 110, 55, 27.5 */
		float[] bins = new float[12 * 9 * 3]; /* 12 notes per octave, 9 octaves, 3 bins per note */

		for (int i = 1; i < (fft2.length >> 1) - 1; i += 1) {
			double phase1 = Math.atan2(fft1[i * 2 + 1], fft1[i * 2]);
			double phase2 = Math.atan2(fft2[i * 2 + 1], fft2[i * 2]);
			double phase = phase1 - phase2;
			/* phase has range -2pi to 2pi */

	        /* what is the expected phase difference at overlaps? */
	        double zerobin = fract((double) overlap * i / sampleHistory.length) * 2 * Math.PI;
	        double binwidth = (double) overlap / sampleHistory.length * 2 * Math.PI;
	        /* zerobin has range 0 .. 2pi */

	        /* Select phase interpretation that minimizes phase-zerobin */
	        if (phase < zerobin - Math.PI) {
	        	phase += 2 * Math.PI;
	        } else if (phase > zerobin + Math.PI) {
	        	phase -= 2 * Math.PI;
	        }

	        double frequency = frameRate * (i + (phase - zerobin) / binwidth) / (fft2.length >> 1);
	        if (frequency <= 0) {
	        	continue;
	        }
	        float note = (float) (Math.log(frequency / minfreq) / Math.log(2) * 12 * 3 + 1);
	        if (note >= 0.0f && note <= bins.length - 1.5f) {
	        	float re = fft2[i * 2];
	        	float im = fft2[i * 2 + 1];
	        	float magnitude = (float) (Math.sqrt(re * re + im * im) / Math.log(frequency)) / 256;

	        	float fract = note - (float) Math.floor(note);
	        	bins[(int) Math.floor(note)] += magnitude * (1.0f - fract);
	        	bins[1 + (int) Math.floor(note)] += magnitude * fract;
	        }
		}

		return bins;
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
