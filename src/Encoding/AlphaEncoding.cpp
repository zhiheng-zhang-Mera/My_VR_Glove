#include "AlphaEncoding.h"
#include <Arduino.h>

void AlphaEncoding::encode(OutboundData data, char* stringToEncode) {
    // [Optimization] Compact trigger value calculation logic.
    // [优化] 更紧凑的扳机键数值计算逻辑，减少分支指令。
    int trigger = (data.fingers[1] > ANALOG_MAX / 2) ? (data.fingers[1] - ANALOG_MAX / 2) * 2 : 0;

    char splayString[40] = ""; // Increased buffer size to prevent stack overflow. / 稍微增加缓冲区大小以防止栈溢出。
#if USING_SPLAY
    // [Performance Note] Splay data formatting. Kept as sprintf for now, could be optimized further if needed.
    // [性能注记] 侧展(Splay)数据格式化。暂时保留 sprintf，如有必要可进一步优化。
    sprintf(splayString, "(AB)%d(BB)%d(CB)%d(DB)%d(EB)%d",
        data.splay[0], data.splay[1], data.splay[2], data.splay[3], data.splay[4]
    );
#endif

    // [Transmission] Use snprintf for safe formatting. While slightly slower, the TX bottleneck is usually at the receiver side.
    // [传输] 使用 snprintf 进行安全格式化。虽然速度略慢，但发送端的瓶颈通常在于接收端的处理能力。
    snprintf(stringToEncode, 128, "A%dB%dC%dD%dE%dF%dG%dP%d%s%s%s%s%s%s%s%s%s\n",
        data.fingers[0], data.fingers[1], data.fingers[2], data.fingers[3], data.fingers[4],
        data.joyX, data.joyY, trigger, data.joyClick ? "H" : "",
        data.triggerButton ? "I" : "", data.aButton ? "J" : "", data.bButton ? "K" : "", data.grab ? "L" : "", data.pinch ? "M" : "", data.menu ? "N" : "", data.calib ? "O" : "",
        splayString);
}

// [Core Logic] High-Performance Decoding Algorithm
// [核心逻辑] 高性能极速解码算法
DecodedData AlphaEncoding::decodeData(char* stringToDecode) {
    DecodedData decodedData = {};

    // 1. Quick check for special commands.
    //    Only scan for commands if the 'Z' key character is present to save CPU cycles.
    // 1. 快速检查特殊命令。
    //    仅当字符串中存在 'Z' 字符时才扫描具体命令，以节省 CPU 周期。
    if (strchr(stringToDecode, 'Z') != NULL) {
        for (int i = 0; i < NUM_SPECIAL_COMMANDS; i++) {
            if (strstr(stringToDecode, SPECIAL_COMMANDS[i]) != NULL) {
                decodedData.command = SPECIAL_COMMANDS[i];
                decodedData.fields.specialCommandReceived = true;
                return decodedData;
            }
        }
    }

    // 2. Single Pass Parsing (Linear Scan)
    //    Instead of using expensive function calls like getArgument, we iterate using a raw pointer.
    // 2. 单次线性扫描解析
    //    直接使用原始指针遍历，不再使用开销较大的 getArgument 函数调用。
    char* ptr = stringToDecode;

    while (*ptr) {
        char key = *ptr; // Get current character key / 获取当前键值

        // Check for uppercase keys (A-Z) / 检查大写字母键
        if (key >= 'A' && key <= 'Z') {

            // Parse Finger Data (A=0, B=1, C=2, D=3, E=4)
            // 解析手指数据
            if (key >= 'A' && key < 'A' + NUM_FINGERS) {
                int fingerIndex = key - 'A';

                // [Optimization] strtol parses the number and updates the pointer to the end of the number.
                // This allows us to skip the parsed digits and jump directly to the next key.
                // [优化] strtol 解析数字并自动将指针更新到数字末尾。
                // 这允许我们跳过已解析的数字，直接跳转到下一个键值。
                char* nextPtr;
                long val = strtol(ptr + 1, &nextPtr, 10);

                if (ptr + 1 != nextPtr) { // Validation: Ensure a number was actually read / 验证：确保确实读取到了数字
                    decodedData.servoValues[fingerIndex] = (int)val;
                    decodedData.fields.servoValuesReceived[fingerIndex] = true;
                    ptr = nextPtr; // Advance main pointer / 移动主指针
                    continue; // Skip the ptr++ at the end of loop / 跳过循环末尾的自增
                }
            }
        }
        // Advance pointer if key not recognized or parsing complete for this block
        // 如果键未识别或当前块解析完成，指针后移
        ptr++;
    }

    return decodedData;
}

// [Deprecated] Legacy function kept for header compatibility.
// [废弃] 为保持头文件兼容性而保留的旧函数。
int AlphaEncoding::getArgument(char* stringToDecode, char command) {
    return -1;
}