package com.ssb.droidsound.activity;

import java.util.List;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;

import com.ssb.droidsound.R;
import com.ssb.droidsound.app.Application;
import com.ssb.droidsound.plugins.DroidSoundPlugin;
import com.ssb.droidsound.utils.Log;

public class SettingsActivity extends PreferenceActivity {
	protected static final String TAG = SettingsActivity.class.getSimpleName();

	protected static class AudiopPrefsListener implements
			OnPreferenceChangeListener {
		private final DroidSoundPlugin plugin;

		protected AudiopPrefsListener(DroidSoundPlugin pi) {
			plugin = pi;
		}

		@Override
		public boolean onPreferenceChange(Preference preference, Object newValue) {
			String k = preference.getKey();
			String k2 = k.substring(k.indexOf('.') + 1);
			plugin.setOption(k2, newValue);
			return true;
		}
	}

	@Override
	public void onBuildHeaders(List<Header> target) {
		loadHeadersFromResource(R.xml.preferences_headers, target);
	}

	public static class AudioFragment extends PreferenceFragment {
		@Override
		public void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);
			addPreferencesFromResource(R.xml.preferences_audio);

			for (int i = 0; i < getPreferenceScreen().getPreferenceCount(); i++) {
				Preference p = getPreferenceScreen().getPreference(i);
				Log.d(TAG, "Pref '%s'", p.getKey());
				if (p instanceof PreferenceGroup) {
					PreferenceGroup pg = (PreferenceGroup) p;

					DroidSoundPlugin plugin = null;
					for (DroidSoundPlugin pl : DroidSoundPlugin.getPluginList()) {
						if (pl.getClass().getSimpleName().equals(pg.getKey())) {
							plugin = pl;
							break;
						}
					}
					if (plugin != null) {
						for (int j = 0; j < pg.getPreferenceCount(); j++) {
							p = pg.getPreference(j);
							Log.d(TAG, "Pref %s for %s", p.getKey(), plugin
									.getClass().getName());
							p.setOnPreferenceChangeListener(new AudiopPrefsListener(
									plugin));
						}
					}
				}
			}
		}
	}

	public static class DatabaseFragment extends PreferenceFragment {
		@Override
		public void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);
			addPreferencesFromResource(R.xml.preferences_database);

			Preference pref = findPreference("rescan_pref");
			pref.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
				@Override
				public boolean onPreferenceClick(Preference preference) {
					CheckBoxPreference cbp = (CheckBoxPreference) findPreference("rescan_full");
					Application.getSongDatabase().scan(cbp.isChecked());
					return true;
				}
			});
		}
	}

	public static class AboutFragment extends PreferenceFragment {
		@Override
		public void onCreate(Bundle savedInstanceState) {
			super.onCreate(savedInstanceState);
			addPreferencesFromResource(R.xml.preferences_about);

			PreferenceCategory pc = new PreferenceCategory(getActivity());
			pc.setTitle(R.string.app_name);
			getPreferenceScreen().addPreference(pc);

			Preference p = new Preference(getActivity());
			p.setTitle("Original version");
			p.setSummary("(c) 2010, 2011 by Jonas Minnberg");
			getPreferenceScreen().addPreference(p);

			p = new Preference(getActivity());
			p.setTitle("Android 4.0 rewrite");
			p.setSummary("(c) 2011,2012 by Antti S. Lankila");
			getPreferenceScreen().addPreference(p);

			p = new Preference(getActivity());
			p.setTitle("Icons");
			p.setSummary("G-Flat SVG by poptones");
			getPreferenceScreen().addPreference(p);

			pc = new PreferenceCategory(getActivity());
			pc.setTitle("Contained projects");
			getPreferenceScreen().addPreference(pc);

			for (DroidSoundPlugin pl : DroidSoundPlugin.getPluginList()) {
				p = new Preference(getActivity());
				p.setTitle(pl.getClass().getSimpleName());
				p.setSummary(pl.getVersion());
				getPreferenceScreen().addPreference(p);
			}
		}
	}
}