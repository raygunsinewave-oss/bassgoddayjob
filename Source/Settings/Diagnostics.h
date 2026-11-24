#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bgdj
{
class BassGodsProcessor;

namespace diagnostics
{
    /** Set by the editor once an OpenGL context reports its renderer. */
    void setGpuDescription (const juce::String& s);
    juce::String getGpuDescription();

    juce::String systemReport (const BassGodsProcessor& p);

    /** Writes a .zip with the report, settings and logs. Returns error text or empty. */
    juce::String exportZip (const BassGodsProcessor& p, const juce::File& zipFile);

    /** Installs a crash handler (standalone only, only if the user opted in). */
    void installCrashHandlerIfEnabled (bool isStandalone);

    /** Asynchronously checks GitHub releases for a newer version.
        callback(message) runs on the message thread. */
    void checkForUpdates (std::function<void (juce::String)> callback);
}
} // namespace bgdj
