#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.hpp"

class Parameters {
public:
    Parameters(AudioProcessorValueTreeState& tree);
    ~Parameters() = default;

    static auto createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout;

    auto prepareToPlay(double sampleRate, int blockSize) noexcept -> void;
    auto reset() noexcept -> void;
    auto init() noexcept -> void;
    auto blockUpdate() noexcept -> void;
    auto update() noexcept -> void;
    auto setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void;

    auto getDefaultParameter(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    static ParameterIDs paramIDs;

    float mix = 0.5f;
    AudioParameterFloat* mixParam;

    String delayL = "1/4";
    AudioParameterChoice* delayLParam;

    String delayR = "1/4";
    AudioParameterChoice* delayRParam;

    String shift = "0";
    AudioParameterChoice* shiftParam;

    float feedback = 0.5f;
    AudioParameterFloat* feedbackParam;

    float width = 1.0f;
    AudioParameterFloat* widthParam;

    bool pingPong = false;
    AudioParameterBool* pingPongParam;

    bool reverse = false;
    AudioParameterBool* reverseParam;

    float lowcut = 200.0f;
    AudioParameterFloat* lowcutParam;

    float highcut = 20000.0f;
    AudioParameterFloat* highcutParam;
private:
    AudioProcessorValueTreeState& tree;

    LinearSmoothedValue<float> mixSmoother;
    LinearSmoothedValue<float> feedbackSmoother;
    LinearSmoothedValue<float> widthSmoother;
    LinearSmoothedValue<float> lowcutSmoother;
    LinearSmoothedValue<float> highcutSmoother;

    double sampleRate = 44100.0;
    int blockSize = 512;
    double bpm = 150.0;
    double ppq = 0.0;
    double internalPPQ = 0.0;
    AudioPlayHead::TimeSignature timeSignature{4, 4};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};