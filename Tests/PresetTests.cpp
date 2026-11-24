#include "TestHelpers.h"
#include "Presets/PresetFormat.h"
#include "DSP/Wavetable.h"
#include "BinaryData.h"

namespace bgdj::test
{
struct PresetTests : public juce::UnitTest
{
    PresetTests() : juce::UnitTest ("Factory presets", "Presets") {}
    void runTest() override
    {
        LayoutHost host;
        beginTest ("every embedded preset parses");
        std::vector<PresetData> presets;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            const juce::String fn (BinaryData::originalFilenames[i]);
            if (! fn.endsWith (".bgdjpreset")) continue;
            int size = 0;
            const char* d = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
            PresetData p;
            juce::String err;
            expect (presetformat::parse (juce::String::fromUTF8 (d, size), p, err), fn + ": " + err);
            presets.push_back (p);
        }

        beginTest ("exactly 430 factory presets in the specified banks");
        expectEquals ((int) presets.size(), 430);
        std::map<juce::String, int> banks;
        for (auto& p : presets) ++banks[p.bank];
        const std::pair<const char*, int> expected[] = {
            { "SUB & 808", 50 }, { "ROLLING & REESE", 45 }, { "UKG & BASSLINE", 40 }, { "TECH HOUSE RUBBER", 50 },
            { "GROWL & RIDDIM", 45 }, { "HYBRID / COLOUR", 30 }, { "EMOTIVE / WARM", 35 }, { "ORGANIC / TEXTURE", 30 },
            { "BREAKS & JUNGLE", 30 }, { "ACID", 30 }, { "HARDGROOVE / RUMBLE", 25 }, { "FX & NEURO", 20 } };
        for (auto& [b, n] : expected)
            expectEquals (banks[b], n, juce::String ("bank ") + b);

        beginTest ("names are unique and every preset validates against the parameter layout");
        std::set<juce::String> names;
        for (auto& p : presets)
        {
            expect (names.insert (p.name.toLowerCase()).second, "duplicate name " + p.name);
            const auto problems = presetformat::validate (p, host.apvts);
            expect (problems.isEmpty(), problems.joinIntoString ("; "));
            for (int o = 0; o < kNumWtOscs; ++o)
                if (p.tables[o].isNotEmpty())
                    expect (factory::indexOf (p.tables[o]) >= 0, p.name + ": unknown table " + p.tables[o]);
            expect (p.character.isNotEmpty() && p.genre.isNotEmpty(), p.name + ": missing tags");
            expect (p.macroLabels.size() == 4, p.name + ": macro labels");
        }

        beginTest ("End Poem resource");
        int size = 0;
        const char* poem = BinaryData::getNamedResource ("end_poem_txt", size);
        expect (poem != nullptr);
        const auto text = juce::String::fromUTF8 (poem, size);
        expect (text.startsWith ("I see the player you mean."));
        expect (text.trim().endsWith ("Wake up."));
        expect (text.indexOf ("PLAYERNAME") >= 0);
        expect (text.indexOf ("[scrambled]") >= 0);
    }
};

static PresetTests presetTests;
} // namespace bgdj::test
