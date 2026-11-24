#include "ExpansionManager.h"
#include "../Settings/AppSettings.h"

namespace bgdj
{
static juce::StringArray disabledIds()
{
    SharedSettings s;
    juce::StringArray a;
    a.addTokens (s->getString ("disabledExpansions"), "|", "");
    a.removeEmptyStrings();
    return a;
}

juce::Array<ExpansionManager::Expansion> ExpansionManager::list()
{
    juce::Array<Expansion> out;
    const auto disabled = disabledIds();
    for (const auto& entry : juce::RangedDirectoryIterator (AppSettings::getExpansionFolder(), false, "*", juce::File::findDirectories))
    {
        Expansion e;
        e.folder = entry.getFile();
        e.id = e.folder.getFileName();
        const auto meta = juce::JSON::parse (e.folder.getChildFile ("metadata.json"));
        e.name = meta["name"].toString().isNotEmpty() ? meta["name"].toString() : e.id;
        e.author = meta["author"].toString();
        e.version = meta["version"].toString();
        e.description = meta["description"].toString();
        e.enabled = ! disabled.contains (e.id);
        e.numPresets = e.folder.getChildFile ("presets").getNumberOfChildFiles (juce::File::findFiles, "*.bgdjpreset");
        e.numWavetables = e.folder.getChildFile ("wavetables").getNumberOfChildFiles (juce::File::findFiles, "*.wav");
        e.numNoise = e.folder.getChildFile ("noise").getNumberOfChildFiles (juce::File::findFiles, "*.wav");
        out.add (e);
    }
    return out;
}

juce::String ExpansionManager::install (const juce::File& packFile)
{
    if (! packFile.existsAsFile() || ! packFile.hasFileExtension (".bgdjpack"))
        return "Please choose a .bgdjpack file.";

    juce::ZipFile zip (packFile);
    if (zip.getNumEntries() == 0)
        return "The expansion pack is empty or not a valid zip archive.";

    // read metadata to pick a folder name
    juce::String name = packFile.getFileNameWithoutExtension();
    for (int i = 0; i < zip.getNumEntries(); ++i)
        if (zip.getEntry (i)->filename.endsWith ("metadata.json"))
            if (std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i)); in != nullptr)
            {
                const auto meta = juce::JSON::parse (in->readEntireStreamAsString());
                if (meta["name"].toString().isNotEmpty())
                    name = meta["name"].toString();
            }

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        const auto fn = zip.getEntry (i)->filename;
        if (fn.contains ("..") || juce::File::isAbsolutePath (fn))
            return "The expansion pack contains unsafe file paths and was not installed.";
    }

    const auto id = juce::File::createLegalFileName (name).replaceCharacter (' ', '_');
    auto dest = AppSettings::getExpansionFolder().getChildFile (id);
    if (dest.exists())
        dest.deleteRecursively();
    dest.createDirectory();

    const auto result = zip.uncompressTo (dest, true);
    if (result.failed())
    {
        dest.deleteRecursively();
        return "Could not extract the pack: " + result.getErrorMessage();
    }

    // packs zipped with a top-level folder: hoist its contents
    if (! dest.getChildFile ("metadata.json").exists())
    {
        auto subs = dest.findChildFiles (juce::File::findDirectories, false);
        if (subs.size() == 1 && subs[0].getChildFile ("metadata.json").exists())
        {
            for (auto& f : subs[0].findChildFiles (juce::File::findFilesAndDirectories, false))
                f.moveFileTo (dest.getChildFile (f.getFileName()));
            subs[0].deleteRecursively();
        }
    }
    AppSettings::log ("Installed expansion: " + name);
    return {};
}

bool ExpansionManager::remove (const juce::String& id)
{
    auto dir = AppSettings::getExpansionFolder().getChildFile (id);
    if (! dir.isDirectory() || dir.getParentDirectory() != AppSettings::getExpansionFolder())
        return false;
    AppSettings::log ("Removed expansion: " + id);
    return dir.deleteRecursively();
}

void ExpansionManager::setEnabled (const juce::String& id, bool enabled)
{
    auto d = disabledIds();
    d.removeString (id);
    if (! enabled) d.add (id);
    SharedSettings s;
    s->set ("disabledExpansions", d.joinIntoString ("|"));
}

juce::Array<juce::File> ExpansionManager::contentFolders (const juce::String& type)
{
    juce::Array<juce::File> r;
    for (auto& e : list())
        if (e.enabled && e.folder.getChildFile (type).isDirectory())
            r.add (e.folder.getChildFile (type));
    return r;
}
} // namespace bgdj
