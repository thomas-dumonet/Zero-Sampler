/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             StepSequencerDemo
  version:          0.0.1
  vendor:           Tracktion
  website:          www.tracktion.com
  description:      This example shows how to build a step squencer and associated UI.

  dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_audio_utils,
                    juce_core, juce_data_structures, juce_dsp, juce_events, juce_graphics,
                    juce_gui_basics, juce_gui_extra, juce_osc, tracktion_engine
  exporters:        linux_make, vs2017, xcode_iphone, xcode_mac

  moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1, JUCE_PLUGINHOST_AU=1

  type:             Component
  mainClass:        StepSequencerDemo

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include "common/Utilities.h"
#include "common/BinaryData.h"
#include "common/BinaryData.cpp"

//==============================================================================
namespace
{
    static void loadFileIntoSamplerChannel (te::StepClip& clip, int channelIndex, const File& f)
    {
        // Find SamplerPlugin for the Clip's Track
        if (auto sampler = clip.getTrack()->pluginList.findFirstPluginOfType<te::SamplerPlugin>())
        {
            // Update the Sound layer source
            sampler->setSoundMedia (channelIndex, f.getFullPathName());

            // Then update the channel name
            clip.getChannels()[channelIndex]->name = f.getFileNameWithoutExtension();
        }
        else
        {
            jassertfalse; // No SamplerPlugin added yet?
        }
    }
}

//==============================================================================
class StepSequencerDemo :
{
public:
    //==============================================================================
    StepSequencerDemo()
    {
        transport.addChangeListener (this);

        createStepClip();
        createSamplerPlugin (createSampleFiles());

        stepEditor = std::make_unique<StepEditor> (*getClip());
        Helpers::addAndMakeVisible (*this, { &settingsButton, &playPauseButton, &randomiseButton,
                                             &clearButton, &tempoSlider, stepEditor.get() });

        updatePlayButtonText();

        settingsButton.onClick  = [this] { EngineHelpers::showAudioDeviceSettings (engine); };
        playPauseButton.onClick = [this] { EngineHelpers::togglePlay (edit); };
        randomiseButton.onClick = [this] { getClip()->getPattern (0).randomiseSteps(); };
        clearButton.onClick     = [this] { getClip()->getPattern (0).clear(); };

        tempoSlider.setRange (30.0, 220.0, 0.1);
        tempoSlider.setValue (edit.tempoSequence.getTempos()[0]->getBpm(), dontSendNotification);
        tempoSlider.addListener (this);

        setSize (600, 400);
    }

    ~StepSequencerDemo()
    {
        // Clean up our temporary sample files and projects
        engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    }

    //==============================================================================
    void sliderValueChanged (Slider*) override
    {        
        if (! ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
            edit.tempoSequence.getTempos()[0]->setBpm (tempoSlider.getValue());
    }

    void sliderDragEnded (Slider*) override
    {
        edit.tempoSequence.getTempos()[0]->setBpm (tempoSlider.getValue());
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto r = getLocalBounds();

        {
            auto topR = r.removeFromTop (30);
            settingsButton.setBounds (topR.removeFromLeft (topR.getWidth() / 3).reduced (2));
            playPauseButton.setBounds (topR.removeFromLeft (topR.getWidth() / 2).reduced (2));
            randomiseButton.setBounds (topR.removeFromLeft (topR.getWidth() / 2).reduced (2));
            clearButton.setBounds (topR.reduced (2));
        }

        {
            auto bottomR = r.removeFromBottom (60);
            tempoSlider.setBounds (bottomR.reduced (2));
        }

        stepEditor->setBounds (r);
    }

private:
    //==============================================================================
    te::Engine engine { ProjectInfo::projectName };
    te::Edit edit { engine, te::createEmptyEdit (engine), te::Edit::forEditing, nullptr, 0 };
    te::TransportControl& transport { edit.getTransport() };

    TextButton settingsButton { "Settings" }, playPauseButton { "Play" }, randomiseButton { "Randomise" }, clearButton { "Clear" };
    Slider tempoSlider;
    std::unique_ptr<StepEditor> stepEditor;

    //==============================================================================
    te::StepClip::Ptr createStepClip()
    {
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt (edit, 0))
        {
            // Find length of 2 bar
            const te::EditTimeRange editTimeRange (0, edit.tempoSequence.barsBeatsToTime ({ 2, 0.0 }));
            track->insertNewClip (te::TrackItem::Type::step, "Step Clip", editTimeRange, nullptr);

            if (auto stepClip = getClip()) {
                stepClip->setLength(edit.tempoSequence.barsBeatsToTime({ 2, 0.0 }), true);
                return EngineHelpers::loopAroundClip(*stepClip);
            }
        }

        return {};
    }

    Array<File> createSampleFiles()
    {
        Array<File> files;
        const auto destDir = edit.getTempDirectory (true);
        jassert (destDir != File());

        using namespace DemoBinaryData;

        for (int i = 0; i < namedResourceListSize; ++i)
        {
            const auto f = destDir.getChildFile (originalFilenames[i]);

            int dataSizeInBytes = 0;
            const char* data = getNamedResource (namedResourceList[i], dataSizeInBytes);
            jassert (data != nullptr);
            f.replaceWithData (data, (size_t) dataSizeInBytes);
            files.add (f);
        }

        return files;
    }

    void createSamplerPlugin (Array<File> defaultSampleFiles)
    {
        if (auto stepClip = getClip())
        {
            if (auto sampler = dynamic_cast<te::SamplerPlugin*> (edit.getPluginCache().createNewPlugin (te::SamplerPlugin::xmlTypeName, {}).get()))
            {
                stepClip->getTrack()->pluginList.insertPlugin (*sampler, 0, nullptr);

                int channelCount = 0;

                for (auto channel : stepClip->getChannels())
                {
                    const auto error = sampler->addSound (defaultSampleFiles[channelCount++].getFullPathName(), channel->name.get(), 0.0, 0.0, 1.0f);
                    sampler->setSoundParams (sampler->getNumSounds() - 1, channel->noteNumber, channel->noteNumber, channel->noteNumber);
                    sampler->setSoundOpenEnded(channel->getIndex(), true);
                    jassert (error.isEmpty());

                    for (auto& pattern : stepClip->getPatterns()) {
                        pattern.setNumNotes(32);
                        pattern.randomiseChannel(channel->getIndex());
                    }
                }
            }
        }
        else
        {
            jassertfalse; // StepClip not been created yet?
        }
    }

    //==============================================================================
    te::StepClip::Ptr getClip()
    {
        if (auto track = EngineHelpers::getOrInsertAudioTrackAt (edit, 0))
            if (auto clip = dynamic_cast<te::StepClip*> (track->getClips()[0]))
                return *clip;

        return {};
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencerDemo)
};
