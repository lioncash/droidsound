package com.ssb.droidsound.view;

import java.util.Arrays;
import java.util.Queue;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.SurfaceView;

import com.ssb.droidsound.utils.Color;
import com.ssb.droidsound.utils.OverlappingFFT;

public class VisualizationView extends SurfaceView {
	public static final int BINS = 12 * 8;

	protected static final String TAG = VisualizationView.class.getSimpleName();

	private final double minFreq = 55; /* A */
	private final double[] fft = new double[BINS * 3];
	private Color[] colors;

	private Queue<OverlappingFFT.Data> queue;

	private final short[][] buf = new short[3][512];

	private final Paint white;
	private final Paint fftPaint;

	public VisualizationView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setWillNotDraw(false);
		white = new Paint();
		white.setColor(0xffffffff);
		white.setTextAlign(Paint.Align.CENTER);

		fftPaint = new Paint();
		//fftPaint.setAntiAlias(true);
	}

	public void setColors(Color[] colors) {
		this.colors = colors;
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		fftPaint.setStrokeWidth((float) w / fft.length * 3f - 1f);
	}

	/**
	 * Set the data array. Visualizer will update as long as this exists.
	 *
	 * @param data
	 */
	public void setData(Queue<OverlappingFFT.Data> data) {
		this.queue = data;
		Arrays.fill(fft, 0);
		invalidate();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if (queue == null) {
			return;
		}

		long futureDelay = updateFftData();
		postInvalidateDelayed(futureDelay);

		int width = getWidth();
		int height = getHeight();

		for (int i = 1; i < fft.length - 1; i += 3) {
			double dbPrev = fft[i-1];
			double dbNorm = fft[i];
			double dbNext = fft[i+1];

			double hump = 2 * dbNorm / (dbPrev + dbNext + 1e-10);
			float saturation = (float) Math.max(0, Math.min(1, hump - 1));
			fftPaint.setColor(colors[(i / 3) % 12].toRGB((1 + saturation) * 0.5f, saturation));

			float x = (i + 0.5f) / (fft.length) * width;

			double dbIn = Math.max(Math.max(dbPrev, dbNorm), dbNext);
			double dB = Math.log(dbIn) / Math.log(10) * 10 / 60 + 0.8;

			canvas.drawLine(x, height, x, height * (1f - (float) dB), fftPaint);
		}
	}

	private long updateFftData() {
		while (true) {
			long ctm = System.currentTimeMillis();
			OverlappingFFT.Data data = queue.peek();
			if (data == null) {
				return 100;
			}
			if (data.getTime() > ctm) {
				return data.getTime() - ctm;
			}

			data = queue.poll();
			if (data != null) {
				buf[data.getIndex()] = data.getFft();
				if (data.getIndex() == 0) {
					updateFftData(data.getFrameRate() / 2);
				}
			}
		}
	}

	private double projectFft(double idx) {
		return minFreq * Math.pow(2, idx / 12.0);
	}

	private static double getBin(short[] buf, int i) {
		double re = buf[i << 1];
		double im = buf[(i << 1) | 1];
		return re * re + im * im;
	}

	private static double getInterpolated(short[] buf, double x) {
		int i = (int) x;
		double c1 = getBin(buf, i);
		double c2 = getBin(buf, i + 1);
		return c1 + (c2 - c1) * (x - i);
	}

	private void updateFftData(final double halfFrameRate) {
		final double len = buf[0].length >> 1;
		for (int i = 0; i < fft.length; i ++) {
			final double startFreq = projectFft((i - 1 - 0.5) / 3);
			final double endFreq = projectFft((i - 1 + 0.5) / 3);

			double startIdx = startFreq / halfFrameRate * len;
			double endIdx = endFreq / halfFrameRate * len;

			/* Select correct FFT set: if the queried region is small enough,
			 * we can choose one of the higher resolution versions we may have. */
			final int scale = (int) Math.ceil(Math.log(endIdx / len) / Math.log(4));
			final int bufIdx = Math.min(buf.length - 1, -scale);
			final double s = Math.pow(4, bufIdx);
			startIdx *= s;
			endIdx *= s;

			/* Hack to improve resolution when FFT runs out of bins */
			startIdx = (startIdx + Math.min(endIdx, Math.ceil(startIdx))) * .5;
			endIdx = (endIdx + Math.max(startIdx, Math.floor(endIdx))) * .5;

			/* Try to avoid access past end of fft array (low sample rate?) */
			if (endIdx >= fft.length - 1) {
				fft[i] = 1e-99;
				continue;
			}

			double lenSqMax = getInterpolated(buf[bufIdx], startIdx);
			lenSqMax = Math.max(lenSqMax, getInterpolated(buf[bufIdx], endIdx));
			int x = (int) startIdx + 1;
			int xEnd = (int) endIdx + 1;
			while (x < xEnd) {
				double c = getBin(buf[bufIdx], x);
				lenSqMax = Math.max(lenSqMax, c);
				x += 1;
			}

			/* 60 dB correlates with 1 << 20 above (in 3 dB units because of no sqrt). */
			fft[i] = lenSqMax / (1 << 20);
		}
	}
}