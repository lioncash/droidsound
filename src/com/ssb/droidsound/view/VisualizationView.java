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

	private final float[] fft = new float[13 * 8 * 3];

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
			float energyPrev = fft[i-1];
			float energyNorm = fft[i];
			float energyNext = fft[i+1];

			float hump = 2 * energyNorm / (energyPrev + energyNext + 1e-10f);
			float saturation = Math.max(0, Math.min(1, hump - 1));
			fftPaint.setColor(colors[(i / 3) % 12].toRGB((1 + saturation) * 0.5f, saturation));

			float x = (i + 0.5f) / (fft.length) * width;

			float dbIn = (energyPrev + energyNorm + energyNext) / 3;
			float dB = (float) (Math.log(dbIn + 1e-10f) / Math.log(10) * 10) / 24;

			canvas.drawLine(x, height, x, height * (1f - dB), fftPaint);
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