#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bgdj
{
/** Global (per-user, not per-project) settings, shared by every plugin instance.
    Stored as XML in the user's application-data folder. */
class AppSettings : public juce::ChangeBroadcaster
{
public:
    AppSettings();
    ~AppSettings() override;

    static juce::File getDataFolder();          // .../BASSGODSDAYJOB
    static juce::File getUserPresetFolder();    // default user preset folder
    static juce::File getExpansionFolder();
    static juce::File getLogFolder();
    static juce::File getCacheFolder();

    juce::var get (const juce::String& key, const juce::var& def = {}) const;
    void set (const juce::String& key, const juce::var& v);
    bool getBool (const juce::String& key, bool def) const     { return (bool) get (key, def); }
    int getInt (const juce::String& key, int def) const        { return (int) get (key, def); }
    double getDouble (const juce::String& key, double def) const { return (double) get (key, def); }
    juce::String getString (const juce::String& key, const juce::String& def = {}) const { return get (key, def).toString(); }

    // ---- typed helpers ---------------------------------------------------------------------
    float uiScale() const               { return (float) getDouble ("uiScale", 1.0); }
    juce::String theme() const          { return getString ("theme", "Neon Club"); }
    bool sceneEnabled() const           { return getBool ("sceneOn", true) && ! safeMode(); }
    int sceneQuality() const            { return getInt ("sceneQuality", 1); }   // 0 low, 1 med, 2 high
    bool crowdSfxEnabled() const        { return getBool ("crowdSfx", true); }
    bool tooltips() const               { return getBool ("tooltips", true); }
    int knobMode() const                { return getInt ("knobMode", 1); }       // 0 circular, 1 vertical, 2 horizontal
    int fineModifier() const            { return getInt ("fineModifier", 0); }   // 0 shift, 1 ctrl/cmd, 2 alt
    juce::String djName() const;
    bool forceSoftwareRendering() const { return getBool ("forceSoftware", false); }
    bool safeMode() const               { return getBool ("safeMode", false); }
    bool analyzersEnabled() const       { return getBool ("analyzersOn", true) && ! safeMode(); }
    bool autoSave() const               { return getBool ("autoSave", true); }
    juce::Array<juce::File> presetFolders() const;      // default + custom
    juce::Array<juce::File> wavetableFolders() const;
    juce::Array<juce::File> noiseFolders() const;

    void resetToFactory();
    void save();

    /** Last error / log line helper used by Diagnostics. */
    static void log (const juce::String& line);

private:
    std::unique_ptr<juce::PropertiesFile> props;
    juce::CriticalSection lock;
};

/** Shared instance accessor (one per process). */
struct SharedSettings
{
    juce::SharedResourcePointer<AppSettings> settings;
    AppSettings* operator->() const { return &settings.get(); }
    AppSettings& operator*() const  { return settings.get(); }
};
} // namespace bgdj
