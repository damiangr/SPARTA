/*
 ==============================================================================
 
 This file is part of SPARTA; a suite of spatial audio plug-ins.
 Copyright (c) 2018 - Leo McCormack.
 
 SPARTA is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SPARTA is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SPARTA.  If not, see <http://www.gnu.org/licenses/>.
 
 ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor() :
    AudioProcessor(BusesProperties()
        .withInput("Input", AudioChannelSet::discreteChannels(64), true)
        .withOutput("Output", AudioChannelSet::discreteChannels(2), true))
{
	decorrelator_create(&hDecor);
    startTimer(TIMER_PROCESSING_RELATED, 40); 
}

PluginProcessor::~PluginProcessor()
{
	decorrelator_destroy(&hDecor);
}

void PluginProcessor::setParameter (int index, float newValue)
{
    switch (index) {
        case k_nChannels:  decorrelator_setNumberOfChannels(hDecor, (int)(newValue*(float)(MAX_NUM_CHANNELS-1) + 1.5f));
         
        default: break;
    }
}

void PluginProcessor::setCurrentProgram (int /*index*/)
{
}

float PluginProcessor::getParameter (int index)
{
    switch (index) {
        case k_nChannels:   return (float)(decorrelator_getNumberOfChannels(hDecor)-1)/(float)(MAX_NUM_CHANNELS-1);
        default: return 0.0f;
    }
}

int PluginProcessor::getNumParameters()
{
	return k_NumOfParameters;
}

const String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

const String PluginProcessor::getParameterName (int index)
{
    switch (index) {
        case k_nChannels:            return "order";
       
        default: return "NULL";
    }
}

const String PluginProcessor::getParameterText(int index)
{
    switch (index) {
        case k_nChannels: return String(decorrelator_getNumberOfChannels(hDecor));
        default: return "NULL";
    }
}

const String PluginProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String PluginProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 0;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

const String PluginProcessor::getProgramName (int /*index*/)
{
    return String();
}


bool PluginProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool PluginProcessor::isOutputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& /*layouts*/) const
{
    return true;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

void PluginProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    nHostBlockSize = samplesPerBlock;
    nNumInputs =  getTotalNumInputChannels();
    nNumOutputs = getTotalNumOutputChannels();
    nSampleRate = (int)(sampleRate + 0.5);

	decorrelator_init(hDecor, nSampleRate);
    AudioProcessor::setLatencySamples(decorrelator_getProcessingDelay());
}

void PluginProcessor::releaseResources()
{
}

void PluginProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{
    int nCurrentBlockSize = nHostBlockSize = buffer.getNumSamples();
    nNumInputs = jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    nNumOutputs = jmin(getTotalNumOutputChannels(), buffer.getNumChannels());
    float** bufferData = buffer.getArrayOfWritePointers();
    float* pFrameData[MAX_NUM_CHANNELS];
    int frameSize = decorrelator_getFrameSize();

    if((nCurrentBlockSize % frameSize == 0)){ /* divisible by frame size */
        for (int frame = 0; frame < nCurrentBlockSize/frameSize; frame++) {
            for (int ch = 0; ch < buffer.getNumChannels(); ch++)
                pFrameData[ch] = &bufferData[ch][frame*frameSize];

            /* perform processing */
            decorrelator_process(hDecor, pFrameData, pFrameData, nNumInputs, nNumOutputs, frameSize);
        }
    }
    else
        buffer.clear();
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; 
}

AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (this);
}

//==============================================================================
void PluginProcessor::getStateInformation (MemoryBlock& destData)
{
    XmlElement xml("DECORRELATORPLUGINSETTINGS");
    
    xml.setAttribute("NCHANNELS", decorrelator_getNumberOfChannels(hDecor));
    copyXmlToBinary(xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        if (xmlState->hasTagName("DECORRELATORPLUGINSETTINGS")) {
            
            if(xmlState->hasAttribute("NCHANNELS"))
                decorrelator_setNumberOfChannels(hDecor, xmlState->getIntAttribute("NCHANNELS", 2));
            
            decorrelator_refreshParams(hDecor);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

