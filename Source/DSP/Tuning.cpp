#include "Tuning.h"

namespace bgdj
{
static bool parseSclPitch (juce::String token, double& cents)
{
    token = token.trim().upToFirstOccurrenceOf (" ", false, false).trim();
    if (token.isEmpty())
        return false;
    if (token.containsChar ('.'))
    {
        cents = token.getDoubleValue();
        return true;
    }
    if (token.containsChar ('/'))
    {
        const double num = token.upToFirstOccurrenceOf ("/", false, false).getDoubleValue();
        const double den = token.fromFirstOccurrenceOf ("/", false, false).getDoubleValue();
        if (num <= 0.0 || den <= 0.0)
            return false;
        cents = 1200.0 * std::log2 (num / den);
        return true;
    }
    const double ratio = token.getDoubleValue();
    if (ratio <= 0.0)
        return false;
    cents = 1200.0 * std::log2 (ratio);
    return true;
}

bool TuningTable::loadScl (const juce::String& text, juce::String& error, int rootNote)
{
    juce::StringArray lines;
    lines.addLines (text);
    juce::StringArray content;
    for (auto& l : lines)
        if (! l.trimStart().startsWithChar ('!'))
            content.add (l);

    if (content.size() < 2)
    {
        error = "Scala file has no description/count lines.";
        return false;
    }

    const juce::String description = content[0].trim();
    const int count = content[1].trim().getIntValue();
    if (count <= 0 || count > 1024 || content.size() < 2 + count)
    {
        error = "Scala file note count is invalid.";
        return false;
    }

    std::vector<double> degrees { 0.0 };
    for (int i = 0; i < count; ++i)
    {
        double c = 0.0;
        if (! parseSclPitch (content[2 + i], c))
        {
            error = "Could not parse scale degree " + juce::String (i + 1) + ".";
            return false;
        }
        degrees.push_back (c);
    }

    const double period = degrees.back();
    const int steps = count;
    const double rootHz = 440.0 * std::pow (2.0, (rootNote - 69) / 12.0);

    for (int n = 0; n < 128; ++n)
    {
        const int rel = n - rootNote;
        const int octave = (int) std::floor ((double) rel / steps);
        const int deg = rel - octave * steps;
        const double cents = octave * period + degrees[(size_t) deg];
        hz[(size_t) n] = rootHz * std::pow (2.0, cents / 1200.0);
    }

    name = description.isNotEmpty() ? description : juce::String ("Scala scale");
    sourceText = text;
    sourceType = "scl";
    return true;
}

bool TuningTable::loadTun (const juce::String& text, juce::String& error)
{
    juce::StringArray lines;
    lines.addLines (text);
    juce::String section;
    bool any = false;
    std::array<double, 128> out = hz;
    double baseFreq = 8.1757989156;   // AnaMark default: note 0 frequency

    for (auto l : lines)
    {
        l = l.upToFirstOccurrenceOf (";", false, false).trim();
        if (l.isEmpty())
            continue;
        if (l.startsWithChar ('['))
        {
            section = l.toLowerCase();
            continue;
        }
        const auto key = l.upToFirstOccurrenceOf ("=", false, false).trim().toLowerCase();
        const auto val = l.fromFirstOccurrenceOf ("=", false, false).trim();
        if (section == "[tuning]" && key == "basefreq")
            baseFreq = val.getDoubleValue();
        if (key.startsWith ("note "))
        {
            const int n = key.substring (5).trim().getIntValue();
            if (n < 0 || n > 127)
                continue;
            if (section == "[exact tuning]")
            {
                out[(size_t) n] = val.getDoubleValue();
                any = true;
            }
            else if (section == "[tuning]")
            {
                out[(size_t) n] = baseFreq * std::pow (2.0, val.getDoubleValue() / 1200.0);
                any = true;
            }
        }
    }

    if (! any)
    {
        error = "No [Tuning] or [Exact Tuning] note entries found.";
        return false;
    }
    for (auto& f : out)
        if (f <= 0.0 || ! std::isfinite (f))
        {
            error = "Tuning file contains invalid frequencies.";
            return false;
        }

    hz = out;
    name = "AnaMark tuning";
    sourceText = text;
    sourceType = "tun";
    return true;
}

bool TuningTable::loadFile (const juce::File& f, juce::String& error)
{
    const auto text = f.loadFileAsString();
    const auto ext = f.getFileExtension().toLowerCase();
    bool ok = false;
    if (ext == ".scl")
        ok = loadScl (text, error);
    else if (ext == ".tun")
        ok = loadTun (text, error);
    else
        error = "Unsupported tuning file type (use .scl or .tun).";
    if (ok)
        name = f.getFileNameWithoutExtension();
    return ok;
}

bool TuningTable::restore (const juce::String& type, const juce::String& text)
{
    juce::String err;
    if (type == "scl") return loadScl (text, err);
    if (type == "tun") return loadTun (text, err);
    setEqual();
    return true;
}
} // namespace bgdj
