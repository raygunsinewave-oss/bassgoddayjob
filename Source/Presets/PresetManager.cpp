#include "PresetManager.h"
#include "ExpansionManager.h"
#include "../PluginProcessor.h"
#include "BinaryData.h"

namespace bgdj
{
PresetManager::PresetManager (BassGodsProcessor& p) : proc (p)
{
    loadDb();
    rescan();
}

void PresetManager::addFromJson (const juce::String& json, const juce::File& file, const juce::String& source)
{
    PresetEntry e;
    juce::String err;
    if (! presetformat::parse (json, e.data, err))
    {
        AppSettings::log ("Preset skipped (" + file.getFullPathName() + "): " + err);
        return;
    }
    e.file = file;
    e.source = source;
    e.data.isFactory = source == "Factory";
    if (e.data.bank.isEmpty())
        e.data.bank = source == "User" ? juce::String ("USER") : source.toUpperCase();
    e.uid = source + "/" + e.data.bank + "/" + e.data.name;
    entries.push_back (std::move (e));
}

void PresetManager::rescan()
{
    const auto currentUid = juce::isPositiveAndBelow (current, (int) entries.size()) ? entries[(size_t) current].uid : juce::String();
    entries.clear();

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String fn (BinaryData::originalFilenames[i]);
        if (! fn.endsWith (presetformat::kExtension))
            continue;
        int size = 0;
        if (const char* d = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
            addFromJson (juce::String::fromUTF8 (d, size), {}, "Factory");
    }

    for (auto& folder : proc.settings->presetFolders())
        if (folder.isDirectory())
            for (const auto& f : juce::RangedDirectoryIterator (folder, true, "*.bgdjpreset"))
                addFromJson (f.getFile().loadFileAsString(), f.getFile(), "User");

    for (auto& e : ExpansionManager::list())
        if (e.enabled)
            for (const auto& f : juce::RangedDirectoryIterator (e.folder.getChildFile ("presets"), true, "*.bgdjpreset"))
                addFromJson (f.getFile().loadFileAsString(), f.getFile(), e.name);

    std::stable_sort (entries.begin(), entries.end(), [] (const PresetEntry& a, const PresetEntry& b)
    {
        if (a.data.isFactory != b.data.isFactory) return a.data.isFactory;
        if (a.data.bank != b.data.bank) return a.data.bank < b.data.bank;
        return a.data.name.compareNatural (b.data.name) < 0;
    });

    current = -1;
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].uid == currentUid) current = (int) i;
    sendChangeMessage();
}

juce::StringArray PresetManager::banks() const
{
    juce::StringArray s;
    for (auto& e : entries) s.addIfNotAlreadyThere (e.data.bank);
    return s;
}

juce::StringArray PresetManager::characters() const
{
    juce::StringArray s;
    for (auto& e : entries) if (e.data.character.isNotEmpty()) s.addIfNotAlreadyThere (e.data.character);
    s.sort (true);
    return s;
}

std::vector<int> PresetManager::filtered (const Filter& f) const
{
    std::vector<int> r;
    juce::StringArray words;
    words.addTokens (f.search.toLowerCase(), " ", "\"");
    words.removeEmptyStrings();
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (f.bank.isNotEmpty() && e.data.bank != f.bank) continue;
        if (f.character.isNotEmpty() && e.data.character != f.character) continue;
        if (f.genre.isNotEmpty() && e.data.genre != f.genre) continue;
        if (f.favouritesOnly && favourites.count (e.uid) == 0) continue;
        if (f.minRating > 0 && getRating (i) < f.minRating) continue;
        if (! words.isEmpty())
        {
            const auto hay = (e.data.name + " " + e.data.allTags().joinIntoString (" ") + " " + e.data.macroLabels.joinIntoString (" ") + " " + e.data.description).toLowerCase();
            bool all = true;
            for (auto& w : words) if (! hay.contains (w)) { all = false; break; }
            if (! all) continue;
        }
        r.push_back (i);
    }
    return r;
}

void PresetManager::load (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) entries.size()))
        return;
    current = index;
    proc.applyPreset (entries[(size_t) index].data);
    abSlots[0].reset();
    abSlots[1].reset();
    activeSlot = 0;
    sendChangeMessage();
}

void PresetManager::step (int delta, const std::vector<int>& within)
{
    if (within.empty()) return;
    auto it = std::find (within.begin(), within.end(), current);
    int pos = it == within.end() ? (delta > 0 ? -1 : 0) : (int) (it - within.begin());
    pos = (pos + delta + (int) within.size()) % (int) within.size();
    load (within[(size_t) pos]);
}

juce::String PresetManager::saveUser (const juce::String& name, const juce::String& bank, const juce::String& character,
                                      const juce::String& genre, const juce::StringArray& extraTags, const juce::String& key, int bpm)
{
    const auto clean = name.trim();
    if (clean.isEmpty())
        return "Please enter a preset name.";
    auto p = proc.capturePreset (clean);
    p.bank = bank.isNotEmpty() ? bank : juce::String ("USER");
    p.author = proc.settings->djName();
    p.character = character; p.genre = genre; p.extraTags = extraTags; p.key = key; p.bpm = bpm;
    auto folder = proc.settings->presetFolders().getFirst().getChildFile (juce::File::createLegalFileName (p.bank));
    folder.createDirectory();
    const auto file = folder.getChildFile (juce::File::createLegalFileName (clean) + presetformat::kExtension);
    if (! file.replaceWithText (presetformat::serialise (p)))
        return "Could not write " + file.getFullPathName();
    proc.currentPresetName = clean;
    rescan();
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].file == file) current = (int) i;
    sendChangeMessage();
    return {};
}

juce::String PresetManager::importFile (const juce::File& f)
{
    PresetData p;
    juce::String err;
    if (! presetformat::parse (f.loadFileAsString(), p, err))
        return err;
    const auto dest = proc.settings->presetFolders().getFirst().getChildFile ("Imported").getChildFile (f.getFileName());
    dest.getParentDirectory().createDirectory();
    f.copyFileTo (dest);
    rescan();
    return {};
}

bool PresetManager::isFavourite (int i) const
{
    return juce::isPositiveAndBelow (i, (int) entries.size()) && favourites.count (entries[(size_t) i].uid) > 0;
}

void PresetManager::toggleFavourite (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) entries.size())) return;
    const auto& uid = entries[(size_t) i].uid;
    if (favourites.count (uid)) favourites.erase (uid); else favourites.insert (uid);
    saveDb();
    sendChangeMessage();
}

int PresetManager::getRating (int i) const
{
    if (! juce::isPositiveAndBelow (i, (int) entries.size())) return 0;
    auto it = ratings.find (entries[(size_t) i].uid);
    return it == ratings.end() ? 0 : it->second;
}

void PresetManager::setRating (int i, int stars)
{
    if (! juce::isPositiveAndBelow (i, (int) entries.size())) return;
    ratings[entries[(size_t) i].uid] = juce::jlimit (0, 5, stars);
    saveDb();
    sendChangeMessage();
}

std::vector<int> PresetManager::similarTo (int index, int maxResults) const
{
    std::vector<std::pair<float, int>> scored;
    if (! juce::isPositiveAndBelow (index, (int) entries.size()))
        return {};
    const auto& a = entries[(size_t) index].data;
    auto num = [] (const PresetData& p, const char* id, float def)
    {
        const auto v = p.params[juce::Identifier (id)];
        return v.isVoid() || v.isString() ? def : (float) (double) v;
    };
    const float aCut = std::log2 (num (a, "f1_cutoff", 2400.0f));
    for (int i = 0; i < (int) entries.size(); ++i)
    {
        if (i == index) continue;
        const auto& b = entries[(size_t) i].data;
        float s = 0.0f;
        if (b.character == a.character) s += 3.0f;
        if (b.genre == a.genre) s += 2.0f;
        if (b.bank == a.bank) s += 1.0f;
        for (auto& t : b.extraTags) if (a.extraTags.contains (t)) s += 0.5f;
        for (int o = 0; o < kNumWtOscs; ++o) if (a.tables[o].isNotEmpty() && a.tables[o] == b.tables[o]) s += 0.75f;
        s -= std::abs (std::log2 (num (b, "f1_cutoff", 2400.0f)) - aCut) * 0.3f;
        s -= std::abs (num (b, "oA_uni", 1) - num (a, "oA_uni", 1)) * 0.1f;
        if (b.bpm > 0 && a.bpm > 0) s -= std::abs (b.bpm - a.bpm) / 40.0f;
        scored.push_back ({ s, i });
    }
    std::sort (scored.begin(), scored.end(), [] (auto& x, auto& y) { return x.first > y.first; });
    std::vector<int> r;
    for (int k = 0; k < std::min (maxResults, (int) scored.size()); ++k)
        r.push_back (scored[(size_t) k].second);
    return r;
}

void PresetManager::abStore (int slot)  { abSlots[(size_t) slot] = std::make_unique<PresetData> (proc.capturePreset (proc.currentPresetName)); }

void PresetManager::abRecall (int slot)
{
    if (abSlots[(size_t) slot])
    {
        const auto name = proc.currentPresetName;
        proc.applyPreset (*abSlots[(size_t) slot]);
        proc.currentPresetName = name;
    }
    activeSlot = slot;
    sendChangeMessage();
}

void PresetManager::abToggle()
{
    abStore (activeSlot);
    const int other = 1 - activeSlot;
    if (! abSlots[(size_t) other])
        abSlots[(size_t) other] = std::make_unique<PresetData> (*abSlots[(size_t) activeSlot]);
    abRecall (other);
}

void PresetManager::abCopyAtoB()
{
    abStore (activeSlot);
    abSlots[(size_t) (1 - activeSlot)] = std::make_unique<PresetData> (*abSlots[(size_t) activeSlot]);
    sendChangeMessage();
}

void PresetManager::randomize (const juce::String& section, float amount)
{
    const auto ids = params::paramsInSection (section, proc.apvts);
    for (auto& id : ids)
    {
        auto* p = proc.apvts.getParameter (id);
        if (p == nullptr) continue;
        float v = p->getValue();
        float r = random.nextFloat();
        // keep things musical: levels and cutoffs within useful ranges, uni counts moderate
        if (id.endsWith ("_level")) r = 0.4f + r * 0.5f;
        if (id.endsWith ("_uni")) r = r * r * 0.5f;
        if (id.endsWith ("_cutoff")) r = 0.25f + r * 0.6f;
        if (id.endsWith ("_reso")) r *= 0.8f;
        if (id.startsWith ("e1_sus")) r = 0.5f + r * 0.5f;
        if (id.startsWith ("e1_att")) r *= 0.2f;
        if (id == "oA_on") r = 1.0f;
        if (id.endsWith ("_oct") && id.startsWith ("o")) r = 0.25f + r * 0.5f;
        if (id.startsWith ("fx_") && id.endsWith ("_on")) r = r > 0.7f ? 1.0f : 0.0f;
        if (id == "fx_util_on") r = 1.0f;
        v = v + (r - v) * amount;
        p->setValueNotifyingHost (v);
    }
    if (section == "all" || section == "osc")
        for (int o = 0; o < kNumWtOscs; ++o)
            if (random.nextFloat() < amount)
                proc.setFactoryTable (o, random.nextInt (factory::numTables()));
    if (section == "all" || section == "lfo")
    {
        for (int k = 0; k < 2; ++k)
            proc.modMatrix.addRouting ((int) ModSource::Lfo1 + random.nextInt (3),
                                       random.nextInt ((int) params::modDestinations().size()),
                                       (random.nextFloat() - 0.5f) * 0.6f * amount);
    }
    proc.currentPresetName = "Random (" + section + ")";
    ++proc.presetVersion;
    sendChangeMessage();
}

void PresetManager::clearCachesAndRebuild()
{
    WavetableLibrary::getInstance().clear();
    NoiseBank::getInstance().rescan (proc.settings->noiseFolders());
    AppSettings::getCacheFolder().deleteRecursively();
    AppSettings::getCacheFolder().createDirectory();
    rescan();
}

void PresetManager::loadDb()
{
    const auto v = juce::JSON::parse (AppSettings::getDataFolder().getChildFile ("presetdb.json"));
    if (auto* favs = v["favourites"].getArray())
        for (auto& f : *favs) favourites.insert (f.toString());
    if (auto* r = v["ratings"].getDynamicObject())
        for (auto& prop : r->getProperties()) ratings[prop.name.toString()] = (int) prop.value;
}

void PresetManager::saveDb() const
{
    auto* root = new juce::DynamicObject();
    juce::Array<juce::var> favs;
    for (auto& f : favourites) favs.add (f);
    root->setProperty ("favourites", favs);
    auto* r = new juce::DynamicObject();
    for (auto& [k, v] : ratings) r->setProperty (k, v);
    root->setProperty ("ratings", juce::var (r));
    AppSettings::getDataFolder().getChildFile ("presetdb.json").replaceWithText (juce::JSON::toString (juce::var (root)));
}
} // namespace bgdj
