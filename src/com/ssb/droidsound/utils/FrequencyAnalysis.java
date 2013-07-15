package com.ssb.droidsound.utils;

import java.util.PriorityQueue;
import java.util.Queue;

/**
 * Input signal is converted to mono and FFTs are run at multiple resolutions
 * to provide overall coverage from 55 Hz and above.
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

	/** Input samples buffer 1 */
	private final short[] sample1 = new short[2048];

	/** Input samples buffer 2 */
	private final short[] sample2 = new short[2048];

	/** FFT buffer 1 */
	private final short[] fft1 = new short[1024];

	/** FFT buffer 2 */
	private final short[] fft2 = new short[1024];

	/** Current index within accumulation buffer */
	private int sample2Idx;

	/** Audio output rate */
	private int frameRate;

	/** Length of data buffering */
	private int bufferingMs;

	/*double x;
	double xf = 0.001;*/

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
			int mono = (samples[i] + samples[i + 1]) >> 1;
			/*mono = (int) (8192 * Math.sin(x));
			x += xf;*/
			sample2[(sample2.length >> 1) + sample2Idx] = (short) (mono >> 1);
			if (++ sample2Idx == sample2.length >> 1) {
				runFfts(time + 1000 * (i - posInSamples - lengthInSamples) / 2 / frameRate);
				System.arraycopy(sample2, sample2.length >> 1, sample2, 0, sample2.length >> 1);
				sample2Idx = 0;
			}
		}
		//xf += 1e-4;
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

		/* fft1 is defined as N samples lagged version of fft2
		 * that means data at end of fft1 must be moved at start of it,
		 * and fft2 data minus the old N samples concatenated to it
		 */
		int overlap = 128;
		System.arraycopy(sample1, sample1.length - overlap, sample1, 0, overlap);
		System.arraycopy(sample2, 0, sample1, overlap, sample2.length - overlap);

		FFT.fft(sample1, fft1);
		FFT.fft(sample2, fft2);

		double minfreq = 55; /* A */
		float[] bins = new float[12 * 8 * 3]; /* 8 octaves, 3 notes per bin for estimating tonality */

		/* Enhance frequency estimate by doing the FFT twice with slight time lag between the sample
		 * frames. We can use the change in phase we see in a bin to estimate the true frequency
		 * within the bin. Only when period is less than 16 samples will we run into difficulties
		 * estimating the value, and this happens for signals > 10 kHz where the bins are densely placed
		 * enough anyway.
		 */
		for (int i = 1; i < fft2.length >> 1; i += 1) {
			double phase1 = Math.atan2(fft1[(i << 1) | 1], fft1[i << 1]);
			double phase2 = Math.atan2(fft2[(i << 1) | 1], fft2[i << 1]);
			double phase = phase2 - phase1;

	        /* what is the expected phase difference at overlaps? These should bracket it. */
	        double minbin = overlap * (i - 0.5) / sample2.length * 2 * Math.PI;
	        double maxbin = overlap * (i + 0.5) / sample2.length * 2 * Math.PI;

	        if (maxbin - minbin < 2 * Math.PI) {
	        	/* We try corrections as long as we can, in theory, uniquely identify a phase candidate
	        	 * within the range of maxbin .. minbin. */
	        	while (phase < minbin) {
	        		phase += 2 * Math.PI;
	        	}
	        	phase = (phase - minbin) / (maxbin - minbin) - 0.5;
	        } else {
	        	phase = 0;
	        }

	        double estfreq = frameRate * (i + phase) / fft2.length;
	        if (estfreq < minfreq) {
	        	continue;
	        }
	        int note = (int) Math.round(Math.log(estfreq / minfreq) / Math.log(2) * 12 * 3);
	        if (note < bins.length) {
	        	double re = fft2[(i << 1)];
	        	double im = fft2[(i << 1) | 1];
	        	float energy = (float) (Math.sqrt(re * re + im * im));
	        	bins[note] += energy;
	        }
		}

		synchronized (queue) {
			queue.add(new Data(time - 1000 * 512 / frameRate, bins));
		}
	}

	public Queue<Data> getQueue() {
		return queue;
	}

	public void calculateTiming(int frameRate, int bufferFrames) {
		this.frameRate = frameRate;
		bufferingMs = bufferFrames * 1000 / frameRate;
	}
}
