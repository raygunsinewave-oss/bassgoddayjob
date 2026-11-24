#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    // only BASSGODSDAYJOB suites; results are collected after each category because
    // the runner resets them on every run
    const juce::StringArray categories = argc > 1 ? juce::StringArray (argv[1]) : juce::StringArray { "DSP", "Engine", "Presets", "Scene" };
    int failures = 0, passes = 0;
    for (auto& c : categories)
    {
        runner.runTestsInCategory (c);
        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            failures += runner.getResult (i)->failures;
            passes += runner.getResult (i)->passes;
        }
    }
    std::printf ("\n==== BASSGODSDAYJOB tests: %d passed, %d failed ====\n", passes, failures);
    return failures == 0 ? 0 : 1;
}
