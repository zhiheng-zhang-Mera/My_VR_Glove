#include "Haptics.h"

#if USING_FORCE_FEEDBACK

void Haptics::setupServoHaptics() {
#if defined(ESP32)
    // [Hardware] Allocate ESP32 PWM Hardware Timers.
    // [硬件] 分配 ESP32 的 PWM 硬件定时器。
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    // [Overclocking] Set Servo Frequency to 250Hz.
    // Standard servos operate at 50Hz. High frequency reduces latency but increases heat/current.
    // WARNING: Reduce to 50Hz or 100Hz if servos overheat or whine.
    // [超频] 设置舵机频率为 250Hz。
    // 标准舵机工作在 50Hz。高频能降低延迟，但会增加发热和电流。
    // 警告：如果舵机过热或发出啸叫，请降回 50Hz 或 100Hz。
    pinkyServo.setPeriodHertz(250);
    ringServo.setPeriodHertz(250);
    middleServo.setPeriodHertz(250);
    indexServo.setPeriodHertz(250);
    thumbServo.setPeriodHertz(250);
#endif

    pinkyServo.attach(PIN_PINKY_MOTOR);
    ringServo.attach(PIN_RING_MOTOR);
    middleServo.attach(PIN_MIDDLE_MOTOR);
    indexServo.attach(PIN_INDEX_MOTOR);
    thumbServo.attach(PIN_THUMB_MOTOR);
}

void Haptics::scaleLimits(int* hapticLimits, float* scaledLimits) {
    for (int i = 0; i < 5; i++) {
#if FLIP_FORCE_FEEDBACK
        scaledLimits[i] = hapticLimits[i] / 1000.0f * 180.0f;
#else
        scaledLimits[i] = 180.0f - hapticLimits[i] / 1000.0f * 180.0f;
#endif
    }
}

void Haptics::dynScaleLimits(int* hapticLimits, float* scaledLimits) {
    for (int i = 0; i < sizeof(hapticLimits); i++) {
        scaledLimits[i] = hapticLimits[i] / 1000.0f * 180.0f;
    }
}

void Haptics::writeServoHaptics(int* hapticLimits) {
    // ---------------------------------------------------------
    // "Soft Fuse" / Power Limiter Algorithm
    // “软保险丝” / 功率限制算法
    // ---------------------------------------------------------

    // 1. Set Total Power Budget.
    //    USB limit recommended: 2500-3000 (Approx 2.5 servos at full stall torque).
    //    Decrease this value if ESP32 experiences brownouts (restarts).
    // 1. 设定总功率预算。
    //    USB 供电建议值：2500-3000（约等于同时全速驱动 2.5 个舵机）。
    //    如果 ESP32 出现掉电重启，请减小此数值。
    const int MAX_TOTAL_FORCE = 3000;

    // 2. Calculate Current Requested Load.
    // 2. 计算当前请求的总负载。
    long currentTotal = 0;
    for (int i = 0; i < 5; i++) {
        // Only count positive force requests. 
        // Assuming hapticLimits is 0-1000 representing DAC/Force value.
        // 仅统计正向力度请求。
        // 假设 hapticLimits 为 0-1000 的 DAC/力度值。
        if (hapticLimits[i] >= 0) {
            currentTotal += hapticLimits[i];
        }
    }

    // 3. Check and Calculate Scaling Factor.
    // 3. 检查并计算缩放比例。
    float scaler = 1.0f;
    if (currentTotal > MAX_TOTAL_FORCE) {
        // e.g., Request 5000, Limit 2500 -> Scaler = 0.5
        // 例如：请求 5000，限制 2500 -> 缩放系数 = 0.5
        scaler = (float)MAX_TOTAL_FORCE / (float)currentTotal;

        // Debug: Serial.println("Power Limit Triggered! Scaling down...");
    }

    // 4. Apply Scaling and Write to Servos.
    // 4. 应用缩放并写入舵机。
    float scaledLimits[5];
    int safeHapticLimits[5];

    // Apply scaling to the raw force values first for accuracy.
    // 为了精度，先对原始力度值应用缩放。
    for (int i = 0; i < 5; i++) {
        if (hapticLimits[i] >= 0) {
            safeHapticLimits[i] = (int)(hapticLimits[i] * scaler);
        }
        else {
            safeHapticLimits[i] = hapticLimits[i]; // Keep invalid values like -1 / 保持 -1 等无效值
        }
    }

    // Convert safe force values to angles.
    // 将安全力度值转换为角度。
    scaleLimits(safeHapticLimits, scaledLimits);

    if (hapticLimits[0] >= 0) thumbServo.write(scaledLimits[0]);
    if (hapticLimits[1] >= 0) indexServo.write(scaledLimits[1]);
    if (hapticLimits[2] >= 0) middleServo.write(scaledLimits[2]);
    if (hapticLimits[3] >= 0) ringServo.write(scaledLimits[3]);
    if (hapticLimits[4] >= 0) pinkyServo.write(scaledLimits[4]);
}

#endif