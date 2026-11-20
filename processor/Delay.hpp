#pragma once
#include <JuceHeader.h>
#include "BiquadFilter.hpp"

class Delay {
public:
    Delay() = default;
    ~Delay() = default;

    auto prepareToPlay(double sampleRate, double maxDelaySeconds = 8.0) -> void {
        this->sampleRate = sampleRate;

        this->bufferSize = 2 * static_cast<int>(std::ceil(maxDelaySeconds * sampleRate));
        this->delayBufferL.setSize(1, this->bufferSize);
        this->delayBufferR.setSize(1, this->bufferSize);

        this->highpassFilterL.prepareToPlay(sampleRate);
        this->highpassFilterR.prepareToPlay(sampleRate);
        this->lowpassFilterL.prepareToPlay(sampleRate);
        this->lowpassFilterR.prepareToPlay(sampleRate);

        this->highpassFilterL.setType("highpass");
        this->highpassFilterR.setType("highpass");
        this->lowpassFilterL.setType("lowpass");
        this->lowpassFilterR.setType("lowpass");

        this->reset();
        this->updateDelaySamples();
        this->updateFilters();
    }

    auto reset() -> void {
        this->writePos = 0;
        this->reversePosL = 0;
        this->reversePosR = 0;
        this->delayBufferL.clear();
        this->delayBufferR.clear();

        this->highpassFilterL.reset();
        this->highpassFilterR.reset();
        this->lowpassFilterL.reset();
        this->lowpassFilterR.reset();
    }

    auto setBPM(double bpm) -> void {
        this->bpm = bpm;
        this->updateDelaySamples();
    }

    auto setDelayTimeL(const String& delayTimeL) -> void {
        this->delayTimeL = delayTimeL;
        this->updateDelaySamples();
    }

    auto setDelayTimeR(const String& delayTimeR) -> void {
        this->delayTimeR = delayTimeR;
        this->updateDelaySamples();
    }

    auto setShiftTime(const String& shiftTime) -> void {
        this->shiftTime = shiftTime;
        this->updateDelaySamples();
    }

    auto setFeedback(float feedback) -> void {
        this->feedback = jlimit(0.0f, 0.99f, feedback);
    }

    auto setStereoWidth(float stereoWidth) -> void {
        this->stereoWidth = jlimit(0.0f, 2.0f, stereoWidth);
    }

    auto setPingPong(bool pingPong) -> void {
        this->pingPong = pingPong;
    }

    auto setReverse(bool reverse) -> void {
        if (this->reverse != reverse) {
            this->reversePosL = 0;
            this->reversePosR = 0;
        }
        this->reverse = reverse;
    }

    auto setHighcutFreq(float highcutFreq) -> void {
        this->highcutFreq = highcutFreq;
        updateFilters();
    }

    auto setLowcutFreq(float lowcutFreq) -> void {
        this->lowcutFreq = lowcutFreq;
        updateFilters();
    }

    auto getBeats(const String& rate) const -> double {
        if (rate == "1/1") return 4.0;
        if (rate == "1/2") return 2.0;
        if (rate == "1/3") return 4.0 / 3.0;
        if (rate == "1/4") return 1.0;
        if (rate == "1/6") return 2.0 / 3.0;
        if (rate == "1/8") return 0.5;
        if (rate == "1/12") return 1.0 / 3.0;
        if (rate == "1/16") return 0.25;
        if (rate == "1/24") return 1.0 / 6.0;
        if (rate == "1/32") return 0.125;
        if (rate == "0") return 0;
        return 1.0;
    }

    auto updateDelaySamples() -> void {
        const double secondsPerBeat = 60.0 / jmax(1.0, bpm);

        this->delaySamplesL = jlimit(1, this->bufferSize / 2 - 1,
            static_cast<int>(secondsPerBeat * getBeats(this->delayTimeL) * this->sampleRate));

        this->delaySamplesR = jlimit(1, this->bufferSize / 2 - 1,
            static_cast<int>(secondsPerBeat * getBeats(this->delayTimeR) * this->sampleRate));

        this->shiftSamples = jlimit(0, this->bufferSize / 2 - 1,
            static_cast<int>(secondsPerBeat * getBeats(this->shiftTime) * this->sampleRate));

        this->alternatePingSamples = this->shiftSamples + std::max(this->delaySamplesL, this->delaySamplesR);
    }

    auto updateFilters() -> void {
        float q = 0.707f;

        this->highpassFilterL.setFrequency(this->lowcutFreq);
        this->highpassFilterR.setFrequency(this->lowcutFreq);
        this->lowpassFilterL.setFrequency(this->highcutFreq);
        this->lowpassFilterR.setFrequency(this->highcutFreq);

        this->highpassFilterL.setResonance(q);
        this->highpassFilterR.setResonance(q);
        this->lowpassFilterL.setResonance(q);
        this->lowpassFilterR.setResonance(q);
    }

    auto readDelay(const float* buffer, int length, int reverseLength, int& reversePos) -> float {
        if (!this->reverse) {
            return buffer[(this->writePos - length + this->bufferSize) % this->bufferSize];
        }

        if (reversePos >= reverseLength) reversePos = 0;
        int offset = length - reverseLength;
        int phase = (reversePos + reverseLength - offset % reverseLength) % reverseLength;

        int readPos = (this->writePos - 1 - 2 * phase - offset + this->bufferSize) % this->bufferSize;
        int fadeSamples = jmax(1, jmin(reverseLength / 2, static_cast<int>(0.005 * this->sampleRate)));
        float gain = jmin(1.0f, static_cast<float>(jmin(phase, reverseLength - 1 - phase)) / static_cast<float>(fadeSamples));

        reversePos++;
        return buffer[readPos] * gain;
    }

    auto process(float dryL, float dryR, float& wetL, float& wetR) -> void {
        auto* left = delayBufferL.getWritePointer(0);
        auto* right = delayBufferR.getWritePointer(0);

        int stereoOffset = !this->pingPong && this->delaySamplesL == this->delaySamplesR
            ? static_cast<int>(0.01 * this->sampleRate) : 0;

        int lengthL = jlimit(1, this->bufferSize / 2 - 1, this->delaySamplesL + this->shiftSamples);
        int lengthR = jlimit(1, this->bufferSize / 2 - 1, this->delaySamplesR + this->shiftSamples + stereoOffset);

        wetL = this->readDelay(left, lengthL, this->delaySamplesL, this->reversePosL);
        wetR = this->readDelay(right, lengthR, this->delaySamplesR, this->reversePosR);

        const float delayedL = this->reverse
            ? left[(this->writePos - this->delaySamplesL + this->bufferSize) % this->bufferSize] : wetL;

        const float delayedR = this->reverse
            ? right[(this->writePos - this->delaySamplesR + this->bufferSize) % this->bufferSize] : wetR;

        const float mid  = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * stereoWidth;
        wetL = mid + side;
        wetR = mid - side;

        float feedbackL = delayedL * feedback;
        float feedbackR = delayedR * feedback;

        float writeL = 0.0f;
        float writeR = 0.0f;

        if (pingPong) {
            float mono = 0.5f * (dryL + dryR);

            if (this->alternatePing) {
                writeL = mono + feedbackR;
                writeR = feedbackL;
            } else {
                writeL = feedbackR;
                writeR = mono + feedbackL;
            }
        } else {
            writeL = dryL + feedbackL;
            writeR = dryR + feedbackR;
        }
        
        writeL = highpassFilterL.processSample(writeL);
        writeL = lowpassFilterL.processSample(writeL);
        writeR = highpassFilterR.processSample(writeR);
        writeR = lowpassFilterR.processSample(writeR);

        left[writePos] = writeL;
        right[writePos] = writeR;

        this->writePos++;
        if (this->writePos >= this->bufferSize) {
            this->writePos = 0;
        }

        if (--this->alternatePingSamples <= 0) {
            this->alternatePing = !this->alternatePing;
            this->alternatePingSamples = std::max(this->delaySamplesL, this->delaySamplesR);
        }
    }

private:
    double sampleRate = 44100.0;
    double bpm = 150.0;

    String delayTimeL = "1/4";
    String delayTimeR = "1/4";
    String shiftTime = "0";
    float feedback = 0.5f;
    float stereoWidth = 1.0f;
    bool pingPong = false;
    bool reverse = false;
    float lowcutFreq = 200.0f;
    float highcutFreq = 20000.0f;

    AudioBuffer<float> delayBufferL;
    AudioBuffer<float> delayBufferR;
    int bufferSize = 0;
    int writePos = 0;
    int reversePosL = 0;
    int reversePosR = 0;

    int delaySamplesL = 1;
    int delaySamplesR = 1;
    int shiftSamples = 0;
    bool alternatePing = false;
    int alternatePingSamples = 1;

    BiquadFilter lowpassFilterL;
    BiquadFilter lowpassFilterR;

    BiquadFilter highpassFilterL;
    BiquadFilter highpassFilterR;
};