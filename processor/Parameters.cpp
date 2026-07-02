#include "Parameters.h"
#include "Functions.hpp"

template<typename T>
static auto castParameter(const AudioProcessorValueTreeState& tree, 
    const ParameterID* id, T*& dest) -> void {
    dest = dynamic_cast<T*>(tree.getParameter(id->getParamID()));
    jassert(dest != nullptr);
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterFloat* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterBool* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = static_cast<bool>(paramObj->getDefaultValue());
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterChoice* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = static_cast<T>(paramObj->getDefaultValue());
}

ParameterIDs Parameters::paramIDs = ParameterIDs::loadFromJSON();

Parameters::Parameters(AudioProcessorValueTreeState& tree) : tree(tree) {
    using FloatPair = std::pair<AudioParameterFloat*&, const ParameterID*>;
    using BoolPair = std::pair<AudioParameterBool*&, const ParameterID*>;
    using ChoicePair = std::pair<AudioParameterChoice*&, const ParameterID*>;

    auto floatParameters = std::vector<FloatPair>{
        {mixParam, &paramIDs.mix},
        {feedbackParam, &paramIDs.feedback},
        {widthParam, &paramIDs.width},
        {lowcutParam, &paramIDs.lowcut},
        {highcutParam, &paramIDs.highcut}
    };

    auto boolParameters = std::vector<BoolPair>{
        {pingPongParam, &paramIDs.pingPong}
    };

    auto choiceParameters = std::vector<ChoicePair>{
        {delayLParam, &paramIDs.delayL},
        {delayRParam, &paramIDs.delayR},
        {shiftParam, &paramIDs.shift}
    };

    for (const auto& [param, paramID] : floatParameters) {
        castParameter(tree, paramID, param);
    }

    for (const auto& [param, paramID] : boolParameters) {
        castParameter(tree, paramID, param);
    }

    for (auto& [param, paramID] : choiceParameters) {
        castParameter(tree, paramID, param);
    }
}

auto Parameters::createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout {
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.mix, "Mix", NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 0.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.delayL, "Delay Time L", StringArray{"1/32", "1/24", "1/16", "1/12", "1/8", "1/6", "1/4", "1/3", "1/2", "1/1"}, 6
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.delayR, "Delay Time R", StringArray{"1/32", "1/24", "1/16", "1/12", "1/8", "1/6", "1/4", "1/3", "1/2", "1/1"}, 6
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.shift, "Shift", StringArray{"0", "1/16", "1/12", "1/8", "1/6", "1/4", "1/3", "1/2", "1/1"}, 0
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.feedback, "Feedback", NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 0.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.width, "Width", NormalisableRange<float>{0.0f, 2.0f, 0.01f}, 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    layout.add(std::make_unique<AudioParameterBool>(
        paramIDs.pingPong, "Ping Pong", false
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.lowcut, "Lowcut Freq", NormalisableRange<float>{200.0f, 20000.0f, 1.0f, 0.3f}, 200.0f, 
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayHz)
        .withValueFromStringFunction(Functions::parseHz)
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.highcut, "Highcut Freq", NormalisableRange<float>{200.0f, 20000.0f, 1.0f, 0.3f}, 20000.0f, 
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayHz)
        .withValueFromStringFunction(Functions::parseHz)
    ));

    return layout;
}

auto Parameters::getDefaultParameter(const Array<var>& args,
    WebBrowserComponent::NativeFunctionCompletion completion) -> void {

    auto paramID = args[0].toString();
    auto* param = this->tree.getParameter(paramID);
    float defaultValue = param->convertFrom0to1(param->getDefaultValue());

    completion(defaultValue);
}

auto Parameters::prepareToPlay(double sampleRate, int blockSize) noexcept -> void {
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;

    double duration = 0.001;

    auto smoothers = std::vector{
        &mixSmoother,
        &feedbackSmoother,
        &widthSmoother
    };

    for (const auto& smoother : smoothers) {
        smoother->reset(this->sampleRate, duration);
    }
}

auto Parameters::reset() noexcept -> void {
    auto paramFloats = std::vector{
        std::pair{mixParam, &mix},
        std::pair{feedbackParam, &feedback},
        std::pair{widthParam, &width},
        std::pair{lowcutParam, &lowcut},
        std::pair{highcutParam, &highcut}
    };

    auto paramChoices = std::vector{
        std::pair{delayLParam, &delayL},
        std::pair{delayRParam, &delayR},
        std::pair{shiftParam, &shift}
    };

    auto paramBools = std::vector{
        std::pair{pingPongParam, &pingPong}
    };

    for (auto& [param, value] : paramFloats) {
        resetParameter(tree, param, value);
    }

    for (auto& [param, value] : paramChoices) {
        resetParameter(tree, param, value);
    }

    for (auto& [param, value] : paramBools) {
        resetParameter(tree, param, value);
    }
    
    auto smoothers = std::vector{
        std::pair{mixParam, &mixSmoother},
        std::pair{feedbackParam, &feedbackSmoother},
        std::pair{widthParam, &widthSmoother},
        std::pair{lowcutParam, &lowcutSmoother},
        std::pair{highcutParam, &highcutSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setCurrentAndTargetValue(param->get());
    }
}

auto Parameters::setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void {
    this->bpm = bpm;
    this->ppq = ppq;
    this->timeSignature = timeSignature;

    if (ppq > 0.0) {
        this->ppq = ppq;
        this->internalPPQ = ppq;
    } else {
        double ppqPerSample = (this->bpm / 60.0) / this->sampleRate;
        this->internalPPQ += ppqPerSample * this->blockSize; 
        this->ppq = this->internalPPQ;
    }
}

auto Parameters::blockUpdate() noexcept -> void {
    auto smoothers = std::vector{
        std::pair{mixParam, &mixSmoother},
        std::pair{feedbackParam, &feedbackSmoother},
        std::pair{widthParam, &widthSmoother},
        std::pair{lowcutParam, &lowcutSmoother},
        std::pair{highcutParam, &highcutSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setTargetValue(param->get());
    }

    this->delayL = this->delayLParam->getCurrentChoiceName();
    this->delayR = this->delayRParam->getCurrentChoiceName();
    this->shift = this->shiftParam->getCurrentChoiceName();
    this->pingPong = this->pingPongParam->get();
}

auto Parameters::update() noexcept -> void {
    this->mix = this->mixSmoother.getNextValue();

    this->feedback = this->feedbackSmoother.getNextValue();
    this->width = this->widthSmoother.getNextValue();
    
    this->lowcut = this->lowcutSmoother.getNextValue();
    this->highcut = this->highcutSmoother.getNextValue();
}