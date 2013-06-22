package com.ssb.droidsound.preference;

import android.content.Context;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.SeekBar;

/**
 * Feeble attempt at trying to use these android classes to render a slider that you can use to set
 * a value in preference screen. Clicking OK should persist the preference for real, clicking Cancel should
 * restore the original cached value.
 *
 * The value is persisted as a String because the original preference this class is replacing used
 * a string, and it doesn't seem to be possible to change the storage data type without app prefs wipe.
 *
 * @author alankila
 */
public class SliderPreference extends DialogPreference {
	private static final String ANDROID_NS = "http://schemas.android.com/apk/res/android";
	private static final String DROIDSOUND_NS = "https://github.com/lioncash/droidsound";

	/** XML prefs */
	private final int defaultValue, minValue, maxValue;

	/** Internal view */
	private SeekBar seekBar;

	/** The cached original value before user mucked it up with slider, between minValue and maxValue */
	private int initialValue;

	/** The current value as given by slider, between minValue and maxValue */
	private int value;

	public SliderPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
		defaultValue = attrs.getAttributeIntValue(ANDROID_NS, "defaultValue", 0);
		minValue = attrs.getAttributeIntValue(DROIDSOUND_NS, "minValue", 0);
		maxValue = attrs.getAttributeIntValue(DROIDSOUND_NS, "maxValue", 100);
	}

	/** NB: Called before onCreateDialogView.
	 * No idea what this providedValue argument is and why on Earth I should use it. */
	@Override
	protected void onSetInitialValue(boolean restorePersistedValue, Object providedValue) {
		super.onSetInitialValue(restorePersistedValue, providedValue);
		if (restorePersistedValue) {
			value = Integer.valueOf(getPersistedString(String.valueOf(defaultValue)));
		} else {
			value = defaultValue;
		}
		initialValue = value;
	}

	@Override
	protected View onCreateDialogView() {
		seekBar = new SeekBar(getContext());
		return seekBar;
	}

	@Override
	protected void onBindDialogView(View view) {
		super.onBindDialogView(view);
		seekBar.setMax(maxValue - minValue);
		seekBar.setProgress(value - minValue);
		seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
			}

			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				int newValue = progress + minValue;
				if (callChangeListener(newValue)) {
					value = newValue;
				}
			}
		});
	}

    @Override
	protected void onDialogClosed(boolean positiveResult) {
		if (positiveResult) {
			if (shouldPersist()) {
				persistString(String.valueOf(value));
			}
			initialValue = value;
		} else {
			callChangeListener(initialValue);
			seekBar.setProgress(initialValue - minValue);
		}
	}
}
