#include "Diagnostics.h"
#include "AppSettings.h"
#include "../PluginProcessor.h"

namespace bgdj::diagnostics
{
static juce::String& gpuString()
{
    static juce::String s { "Not initialised (software renderer or scene off)" };
    return s;
}

void setGpuDescription (const juce::String& s) { gpuString() = s; }
juce::String getGpuDescription()               { return gpuString(); }

juce::String systemReport (const BassGodsProcessor& p)
{
    juce::String r;
    r << "BASSGODSDAYJOB " << BGDJ_VERSION_STRING << "\n"
      << "Build: " << __DATE__ << " " << __TIME__ << "\n"
      << "Format: " << juce::AudioProcessor::getWrapperTypeDescription (p.wrapperType) << "\n"
      << "Host: " << p.getHostDescription() << "\n"
      << "OS: " << juce::SystemStats::getOperatingSystemName() << (juce::SystemStats::isOperatingSystem64Bit() ? " (64-bit)" : "") << "\n"
      << "CPU: " << juce::SystemStats::getCpuVendor() << " " << juce::SystemStats::getCpuModel()
      << ", " << juce::SystemStats::getNumCpus() << " logical cores\n"
      << "Memory: " << juce::SystemStats::getMemorySizeInMegabytes() << " MB\n"
      << "GPU: " << getGpuDescription() << "\n"
      << "Sample rate: " << p.getSampleRate() << " Hz\n"
      << "Buffer size (last block): " << p.lastBlockSize.load() << " samples, announced " << p.getBlockSize() << "\n"
      << "Audio thread load: " << juce::String (p.loadMeasurer.getLoadAsPercentage(), 1) << " %\n"
      << "Xruns since reset: " << p.getXruns() << "\n"
      << "Current preset: " << p.currentPresetName << "\n"
      << "Safe mode: " << (p.settings->safeMode() ? "on" : "off") << "\n"
      << "Software rendering forced: " << (p.settings->forceSoftwareRendering() ? "yes" : "no") << "\n"
      << "Factory presets embedded: " << BGDJ_FACTORY_PRESET_COUNT << "\n";
    return r;
}

juce::String exportZip (const BassGodsProcessor& p, const juce::File& zipFile)
{
    juce::ZipFile::Builder zb;
    auto tmp = juce::File::createTempFile (".txt");
    tmp.replaceWithText (systemReport (p));
    zb.addFile (tmp, 9, "system_report.txt");
    const auto settingsFile = AppSettings::getDataFolder().getChildFile ("settings.xml");
    if (settingsFile.existsAsFile()) zb.addFile (settingsFile, 9, "settings.xml");
    for (auto& f : AppSettings::getLogFolder().findChildFiles (juce::File::findFiles, false, "*"))
        zb.addFile (f, 9, "logs/" + f.getFileName());

    zipFile.deleteFile();
    juce::FileOutputStream out (zipFile);
    if (! out.openedOk())
    {
        tmp.deleteFile();
        return "Could not create " + zipFile.getFullPathName();
    }
    double progress = 0.0;
    const bool ok = zb.writeToStream (out, &progress);
    tmp.deleteFile();
    return ok ? juce::String() : juce::String ("Writing the zip failed.");
}

static void crashHandler (void*)
{
    AppSettings::log ("CRASH\n" + juce::SystemStats::getStackBacktrace());
}

void installCrashHandlerIfEnabled (bool isStandalone)
{
    SharedSettings s;
    // only the standalone app owns its process; plugins must never replace the host's crash handler
    if (isStandalone && s->getBool ("crashReports", false))
        juce::SystemStats::setApplicationCrashHandler (crashHandler);
}

void checkForUpdates (std::function<void (juce::String)> callback)
{
    const juce::String repo (BGDJ_UPDATE_REPO);
    if (repo.isEmpty())
    {
        callback ("Update checks are not configured in this build (no release repository was set).");
        return;
    }
    juce::Thread::launch ([repo, callback]
    {
        juce::String msg;
        juce::URL url ("https://api.github.com/repos/" + repo + "/releases/latest");
        auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs (6000)
                        .withExtraHeaders ("User-Agent: BASSGODSDAYJOB-update-check");
        if (auto in = url.createInputStream (opts))
        {
            const auto json = juce::JSON::parse (in->readEntireStreamAsString());
            const auto tag = json["tag_name"].toString().trimCharactersAtStart ("vV");
            if (tag.isEmpty()) msg = "Could not read the latest release information.";
            else if (tag == BGDJ_VERSION_STRING) msg = "You're on the latest version (" + tag + ").";
            else msg = "Version " + tag + " is available (you have " BGDJ_VERSION_STRING ").\n" + json["html_url"].toString();
        }
        else
            msg = "Could not reach the update server.";
        juce::MessageManager::callAsync ([callback, msg] { callback (msg); });
    });
}
} // namespace bgdj::diagnostics
