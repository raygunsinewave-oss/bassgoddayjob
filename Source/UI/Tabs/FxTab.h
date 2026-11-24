#pragma once

#include "../Widgets.h"
#include "../CurveEditors.h"
#include "../../Analysis/AnalysisData.h"

namespace bgdj
{
class FxTab : public juce::Component
{
public:
    FxTab (BassGodsProcessor& p, AnalysisData& a);
    ~FxTab() override;
    void resized() override;
    void paint (juce::Graphics&) override;
    void selectModule (int fxType);

    /** Built-in module presets: name -> (param suffix -> value). */
    struct ModulePreset { juce::String name; std::vector<std::pair<juce::String, juce::var>> values; };
    static const std::vector<ModulePreset>& presetsFor (int fxType);

private:
    BassGodsProcessor& proc;
    AnalysisData& analysis;
    std::unique_ptr<juce::Component> rackList, detail;
    int selected = 0;
};
} // namespace bgdj
