#include "AppSettings.h"

namespace bgdj
{
juce::File AppSettings::getDataFolder()
{
   #if JUCE_MAC
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Application Support/BASSGODSDAYJOB");
   #else
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("BASSGODSDAYJOB");
   #endif
    f.createDirectory();
    return f;
}

juce::File AppSettings::getUserPresetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BASSGODSDAYJOB/Presets");
    f.createDirectory();
    return f;
}

juce::File AppSettings::getExpansionFolder() { auto f = getDataFolder().getChildFile ("Expansions"); f.createDirectory(); return f; }
juce::File AppSettings::getLogFolder()       { auto f = getDataFolder().getChildFile ("Logs"); f.createDirectory(); return f; }
juce::File AppSettings::getCacheFolder()     { auto f = getDataFolder().getChildFile ("Cache"); f.createDirectory(); return f; }

AppSettings::AppSettings()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "settings";
    o.filenameSuffix = ".xml";
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = 500;
    props = std::make_unique<juce::PropertiesFile> (getDataFolder().getChildFile ("settings.xml"), o);
}

AppSettings::~AppSettings()
{
    save();
}

juce::var AppSettings::get (const juce::String& key, const juce::var& def) const
{
    const juce::ScopedLock sl (lock);
    if (! props->containsKey (key))
        return def;
    const auto s = props->getValue (key);
    if (def.isBool())   return s == "1" || s.equalsIgnoreCase ("true");
    if (def.isInt())    return s.getIntValue();
    if (def.isDouble()) return s.getDoubleValue();
    return s;
}

void AppSettings::set (const juce::String& key, const juce::var& v)
{
    {
        const juce::ScopedLock sl (lock);
        if (v.isBool()) props->setValue (key, (bool) v ? "1" : "0");
        else            props->setValue (key, v.toString());
        if (autoSave())
            props->saveIfNeeded();
    }
    sendChangeMessage();
}

juce::String AppSettings::djName() const
{
    auto n = getString ("djName");
    if (n.trim().isEmpty())
        n = juce::SystemStats::getLogonName();
    return n.trim().isEmpty() ? juce::String ("DJ") : n.trim();
}

static juce::Array<juce::File> folderList (const AppSettings& s, const juce::String& key, const juce::File& def)
{
    juce::Array<juce::File> r;
    if (def != juce::File()) r.add (def);
    juce::StringArray extra;
    extra.addTokens (s.getString (key), "\n", "");
    for (auto& e : extra)
        if (e.trim().isNotEmpty() && juce::File::isAbsolutePath (e.trim()))
            r.addIfNotAlreadyThere (juce::File (e.trim()));
    return r;
}

juce::Array<juce::File> AppSettings::presetFolders() const
{
    const auto custom = getString ("presetFolder");
    const auto def = custom.isNotEmpty() && juce::File::isAbsolutePath (custom) ? juce::File (custom) : getUserPresetFolder();
    return folderList (*this, "extraPresetFolders", def);
}

juce::Array<juce::File> AppSettings::wavetableFolders() const
{
    const auto custom = getString ("wavetableFolder");
    auto def = custom.isNotEmpty() && juce::File::isAbsolutePath (custom) ? juce::File (custom)
                                   : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BASSGODSDAYJOB/Wavetables");
    def.createDirectory();
    return folderList (*this, "extraWavetableFolders", def);
}

juce::Array<juce::File> AppSettings::noiseFolders() const
{
    const auto custom = getString ("noiseFolder");
    auto def = custom.isNotEmpty() && juce::File::isAbsolutePath (custom) ? juce::File (custom)
                                   : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BASSGODSDAYJOB/Noise");
    def.createDirectory();
    return folderList (*this, "extraNoiseFolders", def);
}

void AppSettings::resetToFactory()
{
    {
        const juce::ScopedLock sl (lock);
        props->clear();
        props->saveIfNeeded();
    }
    sendChangeMessage();
}

void AppSettings::save()
{
    const juce::ScopedLock sl (lock);
    props->saveIfNeeded();
}

void AppSettings::log (const juce::String& line)
{
    static juce::CriticalSection logLock;
    const juce::ScopedLock sl (logLock);
    auto f = getLogFolder().getChildFile ("bassgodsdayjob.log");
    if (f.getSize() > 2 * 1024 * 1024)
        f.moveFileTo (getLogFolder().getChildFile ("bassgodsdayjob.old.log"));
    f.appendText (juce::Time::getCurrentTime().toISO8601 (true) + "  " + line + "\n");
}
} // namespace bgdj
