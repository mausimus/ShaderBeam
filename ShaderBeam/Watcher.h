/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "UI.h"

namespace ShaderBeam
{

#define CHARTS_LEN 1024

struct Chart
{
    int   m_index;
    int   m_marker;
    float m_values[CHARTS_LEN];
    float m_time;

    void Clear();
    void AddDelta();
    void AddDelta(float value);
    void SetStart(float value);
    void AddValue(float value);

    bool Read(int& index, float& value);
};

class Watcher
{
public:
    Watcher(UI& ui);

    void Start();

    void FrameSubmitted();

    void FrameReceived(float value);

    void Stop();

    Chart m_submitChart;
    Chart m_receiveChart;

private:
    UI& m_ui;

    float m_lastSnapshot { 0 };

    int m_inputFrames { 0 };
    int m_outputFrames { 0 };

    void UpdateSnapshot();
};
} // namespace ShaderBeam
