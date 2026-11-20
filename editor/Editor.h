#pragma once
#pragma clang diagnostic ignored "-Wshadow-field"
#include <JuceHeader.h>
#include "Processor.h"
#include "EventEmitter.hpp"

class Editor : public AudioProcessorEditor, public EventEmitter::Listener {
public:
    Editor(Processor& p);
    ~Editor() override;
    
    auto resized() -> void override;

    auto getResource(const String& url) -> std::optional<WebBrowserComponent::Resource>;
    auto webviewOptions() -> WebBrowserComponent::Options;
    auto getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte>;

    auto handleEvent(const String& name, const var& payload) -> void override;
    auto handleThemeChange(const String& theme) -> void;
        
private:
    Processor& processor;
    ComponentBoundsConstrainer constrainer;

    WebSliderRelay mixRelay {Parameters::paramIDs.mix.getParamID()};
    WebSliderParameterAttachment mixAttachment {*this->processor.parameters.mixParam, mixRelay, nullptr};

    WebComboBoxRelay delayLRelay {Parameters::paramIDs.delayL.getParamID()};
    WebComboBoxParameterAttachment delayLAttachment {*this->processor.parameters.delayLParam, delayLRelay, nullptr};

    WebComboBoxRelay delayRRelay {Parameters::paramIDs.delayR.getParamID()};
    WebComboBoxParameterAttachment delayRAttachment {*this->processor.parameters.delayRParam, delayRRelay, nullptr};

    WebComboBoxRelay shiftRelay {Parameters::paramIDs.shift.getParamID()};
    WebComboBoxParameterAttachment shiftAttachment {*this->processor.parameters.shiftParam, shiftRelay, nullptr};

    WebSliderRelay feedbackRelay {Parameters::paramIDs.feedback.getParamID()};
    WebSliderParameterAttachment feedbackAttachment {*this->processor.parameters.feedbackParam, feedbackRelay, nullptr};

    WebSliderRelay widthRelay {Parameters::paramIDs.width.getParamID()};
    WebSliderParameterAttachment widthAttachment {*this->processor.parameters.widthParam, widthRelay, nullptr};

    WebToggleButtonRelay pingPongRelay {Parameters::paramIDs.pingPong.getParamID()};
    WebToggleButtonParameterAttachment pingPongAttachment {*this->processor.parameters.pingPongParam, pingPongRelay, nullptr};

    WebToggleButtonRelay reverseRelay {Parameters::paramIDs.reverse.getParamID()};
    WebToggleButtonParameterAttachment reverseAttachment {*this->processor.parameters.reverseParam, reverseRelay, nullptr};
    
    WebSliderRelay lowcutRelay {Parameters::paramIDs.lowcut.getParamID()};
    WebSliderParameterAttachment lowcutAttachment {*this->processor.parameters.lowcutParam, lowcutRelay, nullptr};

    WebSliderRelay highcutRelay {Parameters::paramIDs.highcut.getParamID()};
    WebSliderParameterAttachment highcutAttachment {*this->processor.parameters.highcutParam, highcutRelay, nullptr};

    WebBrowserComponent webview;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};