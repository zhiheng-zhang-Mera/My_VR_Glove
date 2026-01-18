##include "SerialCommunication.h"

SerialCommunication::SerialCommunication() {
    m_isOpen = false;
    m_bufferIndex = 0; // Initialize buffer index / 初始化缓冲区索引
}

bool SerialCommunication::isOpen() {
    return m_isOpen;
}

void SerialCommunication::start() {
    //Serial.setTimeout(1000000);
    Serial.begin(SERIAL_BAUD_RATE);
    m_isOpen = true;
}

void SerialCommunication::output(char* data) {
    Serial.print(data);
    Serial.flush();
}

// [Optimization] Non-blocking Fast Read
// [优化] 非阻塞式极速读取
bool SerialCommunication::readData(char* input) {
    // Consume data from the Serial buffer until it's empty or command is complete.
    // 只要串口缓冲区有数据，就持续读取，直到缓冲区空或命令结束。
    while (Serial.available() > 0) {
        char rc = Serial.read();

        // Newline indicates end of command packet.
        // 换行符表示命令包结束。
        if (rc == '\n') {
            m_buffer[m_bufferIndex] = '\0'; // Null-terminate the string / 添加字符串结束符
            strcpy(input, m_buffer);        // Copy full packet to output / 复制完整数据包
            m_bufferIndex = 0;              // Reset buffer for next packet / 重置缓冲区
            return true;                    // Signal: New data available / 信号：新数据就绪
        }
        // Accumulate characters if packet is incomplete.
        // 如果数据包未结束，累积字符。
        else {
            if (m_bufferIndex < bufferSize - 1) {
                m_buffer[m_bufferIndex] = rc;
                m_bufferIndex++;
            }
            // Overflow protection: discard excess data to prevent crash.
            // 溢出保护：丢弃多余数据以防止崩溃。
        }
    }
    // Return false if no full command was parsed in this cycle (non-blocking).
    // 如果本次循环未解析出完整命令，返回 false（非阻塞）。
    return false;
}