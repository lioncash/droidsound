package com.ssb.droidsound.view;

import java.util.Arrays;
import java.util.Queue;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.SurfaceView;

import com.ssb.droidsound.utils.Color;
import com.ssb.droidsound.utils.FrequencyAnalysis;
import com.ssb.droidsound.utils.Log;

public class VisualizationView extends SurfaceView {
	protected static final String TAG = VisualizationView.class.getSimpleName();

	private Color[] colors;

	private Queue<FrequencyAnalysis.Data> queue;

	private final float[] fft = new float[12*8 * 3];

	private final Paint white;

	private final Paint fftPaint;

	public VisualizationView(Context context, AttributeSet attrs) {
		super(context, attrs);
		setWillNotDraw(false);
		white = new Paint();
		white.setColor(0xffffffff);
		white.setTextAlign(Paint.Align.CENTER);

		fftPaint = new Paint();
	}

	public void setColors(Color[] colors) {
		this.colors = colors;
	}

	/**
	 * Set the data array. Visualizer will update as long as this exists.
	 *
	 * @param data
	 */
	public void setData(Queue<FrequencyAnalysis.Data> data) {
		this.queue = data;
		Arrays.fill(fft, 0);
		invalidate();
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if (queue == null) {
			Log.i("VisualizationView", "Stop updates");
			return;
		}

		long futureDelay = updateFftData();
		postInvalidateDelayed(futureDelay);

		if (fft == null) {
			return;
		}

		int width = getWidth();
		int height = getHeight();

		fftPaint.setStrokeWidth(width * 3f / fft.length - 1f);

		for (int i = 1; i < fft.length - 1; i += 3) {
			double energyPrev = fft[i-1];
			double energyNorm = fft[i];
			double energyNext = fft[i+1];

			double hump = 2 * energyNorm / (energyPrev + energyNext + 1e-10);
			float saturation = (float) Math.max(0, Math.min(1, hump - 1));
			fftPaint.setColor(colors[(i / 3) % 12].toRGB((1 + saturation) * 0.5f, saturation));

			float x = (i + 0.5f) / (fft.length) * width;

			double dbIn = (energyPrev + energyNorm + energyNext) / 3;
			double dB = Math.log(dbIn) / Math.log(10) * 10 / 30;

			canvas.drawLine(x, height, x, height * (1f - (float) dB), fftPaint);
		}
	}

	private long updateFftData() {
		synchronized (queue) {
			while (true) {
				FrequencyAnalysis.Data data = queue.peek();
				if (data == null) {
					Log.i("VisualizationView", "Data underrun. Retry in 100 ms.");
					return 100;
				}

				long ctm = System.currentTimeMillis();
				if (data.getTime() > ctm) {
					return data.getTime() - ctm;
				}

				data = queue.poll();
				if (data != null) {
					updateFromFrequencies(data.getFrequencies());
				}
			}
		}
	}

	private void updateFromFrequencies(float[] frequencies) {
		for (int i = 0; i < fft.length; i += 1) {
			fft[i] = Math.max(fft[i] * .5f, frequencies[i]);
		}
	}
}