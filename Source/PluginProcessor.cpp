/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TAPDelayAudioProcessor::TAPDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

TAPDelayAudioProcessor::~TAPDelayAudioProcessor()
{
}

//==============================================================================
const juce::String TAPDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TAPDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TAPDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TAPDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TAPDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TAPDelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TAPDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TAPDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TAPDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void TAPDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TAPDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mDelayBuffer.setSize (getTotalNumInputChannels(), 2.0 * (samplesPerBlock + sampleRate), false, true);
}

void TAPDelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TAPDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TAPDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // Declaring these for readability
        const int bufferLength = buffer.getNumSamples();
        const int delayBufferLength = mDelayBuffer.getNumSamples();

        const float* bufferData = buffer.getReadPointer (channel);
        const float* delayBufferData = mDelayBuffer.getReadPointer (channel);

        // Copy data from main buffer to delay buffer - this is a bit fiddly because the buffers are different lengths
        // This if alone won't fill the buffer because buffer is smaller than mDelayBuffer 
        if (delayBufferLength > bufferLength + mWritePosition)
        {
            mDelayBuffer.copyFromWithRamp (channel, mWritePosition, bufferData, bufferLength, 0.8, 0.8);
        }
        // So we have to catch the rest of them - look at TAP delay pt 1 tutorial for explanation of this
        else 
        {
            const int bufferRemaining = delayBufferLength - mWritePosition; // This is the number of values left to move after the if above ^

            mDelayBuffer.copyFromWithRamp (channel, mWritePosition, bufferData + bufferRemaining, bufferRemaining, 0.8, 0.8);
            mDelayBuffer.copyFromWithRamp (channel, 0, bufferData + bufferRemaining, bufferLength - bufferRemaining, 0.8, 0.8); // Wrap to start of buffer
        }

        mWritePosition += bufferLength; // When buffer has been processed, move write position to the next value so it becomes e.g. 513 not 0 again
        mWritePosition %= delayBufferLength; // Look below for explanation
        /*
        This has the effect of wrapping the value back around to 0.
        So when delayBufferLength gets to its maximum value, mWritePosition will become the same number as delayBufferLength
        So modulo divides mWritePosition by delayBufferLength which is the same as dividing it by itself.
        Dividing by itself = 1 with remainder 0.
        So mWritePosition becomes 0.
        */
    }
}

//==============================================================================
bool TAPDelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TAPDelayAudioProcessor::createEditor()
{
    return new TAPDelayAudioProcessorEditor (*this);
}

//==============================================================================
void TAPDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TAPDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TAPDelayAudioProcessor();
}
