#pragma once
#include "ICommunication.h"
#include "Config.h"
#include <Arduino.h>

class SerialCommunication : public ICommunication {
private:
    bool m_isOpen;
    // 植入私有缓冲区，用来暂存碎片数据
    static const int bufferSize = 128;
    char m_buffer[bufferSize];
    int m_bufferIndex;

public:
    SerialCommunication();

    bool isOpen() override;

    void start() override;

    void output(char* data) override;

    bool readData(char* input) override;
};
