#pragma once
#include <JuceHeader.h>
#include "BinaryData.h"

class ParameterIDs {
public:
    ParameterID mix;
    ParameterID delayL;
    ParameterID delayR;
    ParameterID shift;
    ParameterID feedback;
    ParameterID width;
    ParameterID pingPong;
    ParameterID lowcut;
    ParameterID highcut;
    ParameterID reverse;

    static inline var parsedJSON;

    static auto getJSON() -> DynamicObject* {
        if (!ParameterIDs::parsedJSON) {
            auto jsonStr = String::fromUTF8(BinaryData::parameters_json, BinaryData::parameters_jsonSize);
            ParameterIDs::parsedJSON = JSON::parse(jsonStr);
        }
        return ParameterIDs::parsedJSON.getDynamicObject();
    }

    static auto loadFromJSON() -> ParameterIDs {
        ParameterIDs parameterIDs;

        auto* json = ParameterIDs::getJSON();

        auto getParameter = [&json](const String& key) -> ParameterID {
            auto value = json->getProperty(key);
            auto* obj = value.getDynamicObject();
            jassert(obj != nullptr);

            String id = obj->getProperty("id").toString();
            int version = static_cast<int>(obj->getProperty("version"));

            return ParameterID{id, version};
        };

        parameterIDs.mix = getParameter("mix");
        parameterIDs.delayL = getParameter("delayL");
        parameterIDs.delayR = getParameter("delayR");
        parameterIDs.shift = getParameter("shift");
        parameterIDs.feedback = getParameter("feedback");
        parameterIDs.width = getParameter("width");
        parameterIDs.pingPong = getParameter("pingPong");
        parameterIDs.lowcut = getParameter("lowcut");
        parameterIDs.highcut = getParameter("highcut");
        parameterIDs.reverse = getParameter("reverse");

        return parameterIDs;
    }

    static auto getStringKeys() -> std::vector<String> {
        auto* json = ParameterIDs::getJSON();
        std::vector<String> keys;
        keys.reserve(static_cast<size_t>(json->getProperties().size()));

        for (const auto& prop : json->getProperties()) {
            keys.push_back(prop.name.toString());
        }

        return keys;
    }

    static auto isStringValue(const String& key) -> bool {
        auto* json = ParameterIDs::getJSON();
        if (json->hasProperty(key)) {
            auto* paramDef = json->getProperty(key).getDynamicObject();
            if (paramDef->hasProperty("stringValue") && paramDef->getProperty("stringValue")) {
                return true;
            }
        }

        return false;
    }
};