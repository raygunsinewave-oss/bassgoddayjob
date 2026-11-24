#include "PresetFormat.h"

namespace bgdj
{
juce::StringArray PresetData::allTags() const
{
    juce::StringArray t;
    if (genre.isNotEmpty()) t.add (genre);
    if (character.isNotEmpty()) t.add (character);
    if (key.isNotEmpty()) t.add (key);
    if (bpm > 0) t.add (juce::String (bpm) + " BPM");
    t.addArray (extraTags);
    if (bank.isNotEmpty()) t.add (bank);
    return t;
}

namespace presetformat
{
const juce::StringArray& bannedWords()
{
    // Artist / brand names the factory library is inspired by, plus trademarks. Never used in names or tags.
    static const juce::StringArray w { "fred again", "skrillex", "hamdi", "four tet", "dom dolla", "kettama", "solomun",
                                       "serum", "xfer", "massive", "vital", "sylenth", "minecraft", "mojang", "roland", "tb-303",
                                       "korg", "pioneer", "cdj", "ableton", "native instruments" };
    return w;
}

bool parse (const juce::String& json, PresetData& p, juce::String& error)
{
    juce::var root;
    const auto res = juce::JSON::parse (json, root);
    if (res.failed() || ! root.isObject())
    {
        error = "Invalid JSON: " + res.getErrorMessage();
        return false;
    }
    if (root["format"].toString() != "bgdjpreset")
    {
        error = "Not a BASSGODSDAYJOB preset.";
        return false;
    }

    p.name = root["name"].toString();
    p.bank = root["bank"].toString();
    p.author = root.hasProperty ("author") ? root["author"].toString() : juce::String ("Unknown");
    p.description = root["description"].toString();

    const auto tags = root["tags"];
    p.genre = tags["genre"].toString();
    p.character = tags["character"].toString();
    p.key = tags["key"].toString();
    p.bpm = (int) tags["bpm"];
    p.extraTags.clear();
    if (auto* extra = tags["extra"].getArray())
        for (auto& e : *extra)
            p.extraTags.add (e.toString());

    if (auto* macros = root["macros"].getArray())
        for (int i = 0; i < std::min (kNumMacros, macros->size()); ++i)
            p.macroLabels.set (i, (*macros)[i].toString());

    const auto tables = root["tables"];
    const auto userTables = root["user_tables"];
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const juce::String k = juce::String::charToString ((juce::juce_wchar) ('A' + o));
        p.tables[o] = tables[juce::Identifier (k)].toString();
        p.userTables[o] = userTables[juce::Identifier (k)].toString();
    }

    p.params.clear();
    if (auto* obj = root["params"].getDynamicObject())
        for (auto& prop : obj->getProperties())
            p.params.set (prop.name, prop.value);

    p.matrix = root["matrix"];
    p.lfoShapes = root["lfo_shapes"];
    p.duckShape = root["duck_shape"];
    p.fxOrder = root["fx_order"];
    p.tuningType = root["tuning"]["type"].toString();
    p.tuningText = root["tuning"]["text"].toString();

    if (p.name.isEmpty())
    {
        error = "Preset has no name.";
        return false;
    }
    return true;
}

juce::String serialise (const PresetData& p)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "bgdjpreset");
    root->setProperty ("version", 1);
    root->setProperty ("name", p.name);
    root->setProperty ("bank", p.bank);
    root->setProperty ("author", p.author);
    if (p.description.isNotEmpty()) root->setProperty ("description", p.description);

    auto* tags = new juce::DynamicObject();
    tags->setProperty ("genre", p.genre);
    tags->setProperty ("character", p.character);
    tags->setProperty ("key", p.key);
    tags->setProperty ("bpm", p.bpm);
    juce::Array<juce::var> extra;
    for (auto& t : p.extraTags) extra.add (t);
    tags->setProperty ("extra", extra);
    root->setProperty ("tags", juce::var (tags));

    juce::Array<juce::var> macros;
    for (auto& m : p.macroLabels) macros.add (m);
    root->setProperty ("macros", macros);

    auto* tables = new juce::DynamicObject();
    auto* userTables = new juce::DynamicObject();
    bool anyUser = false;
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const juce::String k = juce::String::charToString ((juce::juce_wchar) ('A' + o));
        if (p.tables[o].isNotEmpty()) tables->setProperty (k, p.tables[o]);
        if (p.userTables[o].isNotEmpty()) { userTables->setProperty (k, p.userTables[o]); anyUser = true; }
    }
    root->setProperty ("tables", juce::var (tables));
    if (anyUser) root->setProperty ("user_tables", juce::var (userTables));
    else delete userTables;

    auto* params = new juce::DynamicObject();
    for (auto& nv : p.params)
        params->setProperty (nv.name, nv.value);
    root->setProperty ("params", juce::var (params));

    if (! p.matrix.isVoid())    root->setProperty ("matrix", p.matrix);
    if (! p.lfoShapes.isVoid()) root->setProperty ("lfo_shapes", p.lfoShapes);
    if (! p.duckShape.isVoid()) root->setProperty ("duck_shape", p.duckShape);
    if (! p.fxOrder.isVoid())   root->setProperty ("fx_order", p.fxOrder);
    if (p.tuningType.isNotEmpty())
    {
        auto* t = new juce::DynamicObject();
        t->setProperty ("type", p.tuningType);
        t->setProperty ("text", p.tuningText);
        root->setProperty ("tuning", juce::var (t));
    }
    return juce::JSON::toString (juce::var (root), false);
}

bool resolveValue (const juce::RangedAudioParameter& param, const juce::var& v, float& real)
{
    if (auto* choice = dynamic_cast<const juce::AudioParameterChoice*> (&param))
    {
        if (v.isString())
        {
            const int idx = choice->choices.indexOf (v.toString(), true);
            if (idx < 0) return false;
            real = (float) idx;
            return true;
        }
        real = (float) juce::jlimit (0, choice->choices.size() - 1, (int) v);
        return true;
    }
    if (dynamic_cast<const juce::AudioParameterBool*> (&param) != nullptr)
    {
        real = v.isBool() ? ((bool) v ? 1.0f : 0.0f) : ((double) v >= 0.5 ? 1.0f : 0.0f);
        return true;
    }
    if (v.isString() && ! v.toString().containsOnly ("-0123456789.eE"))
        return false;
    const auto range = param.getNormalisableRange();
    real = juce::jlimit (range.start, range.end, (float) (double) v);
    return true;
}

juce::StringArray validate (const PresetData& p, const juce::AudioProcessorValueTreeState& state)
{
    juce::StringArray problems;
    for (auto& nv : p.params)
    {
        auto* param = state.getParameter (nv.name.toString());
        float real = 0.0f;
        if (param == nullptr)
            problems.add (p.name + ": unknown parameter " + nv.name.toString());
        else if (! resolveValue (*param, nv.value, real))
            problems.add (p.name + ": bad value for " + nv.name.toString() + " (" + nv.value.toString() + ")");
    }
    if (auto* arr = p.matrix.getArray())
        for (auto& s : *arr)
        {
            if (params::modSourceNames().indexOf (s["src"].toString()) <= 0)
                problems.add (p.name + ": bad mod source " + s["src"].toString());
            if (params::modDestIndexForParam (s["dst"].toString()) < 0)
                problems.add (p.name + ": bad mod destination " + s["dst"].toString());
        }
    const auto lower = (p.name + " " + p.allTags().joinIntoString (" ")).toLowerCase();
    for (auto& w : bannedWords())
        if (lower.contains (w))
            problems.add (p.name + ": contains banned word '" + w + "'");
    return problems;
}
} // namespace presetformat
} // namespace bgdj
