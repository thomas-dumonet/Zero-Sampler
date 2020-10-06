/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "StepSequencerDemo.h"

class Application    : public JUCEApplication
{
public:
    //==============================================================================
    Application() {}

    const String getApplicationName() override       { return "StepSequencerDemo"; }
    const String getApplicationVersion() override    { return "0.0.1"; }

    void initialise (const String&) override         { new StepSequencerDemo(); }
    void shutdown() override                         {  }

private:

};

//==============================================================================
START_JUCE_APPLICATION (Application)
