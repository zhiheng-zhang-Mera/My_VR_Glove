#include "Main.h"
#include "Communication/SerialCommunication.h"
#include "Communication/BTSerialCommunication.h"
#include "Encoding/AlphaEncoding.h"
#include "Encoding/LegacyEncoding.h"
#include "Util/DataStructs.h"

// Ensure Arduino.h is included for platform definitions.
// 确保包含 Arduino.h 以获取平台定义。
#include <Arduino.h> 

#define ALWAYS_CALIBRATING CALIBRATION_LOOPS == -1
#define CALIB_OVERRIDE false

// 检查错误配置
#if USING_CALIB_PIN && COMMUNICATION == COMM_SERIAL && PIN_CALIB == 0 && !CALIB_OVERRIDE
  #error "You can't set your calibration pin to 0 over usb. You can calibrate with the BOOT button when using bluetooth only."
#endif

Main::Main() {
  // Constructor
}

// ---------------------------------------------------------
// SETUP: Initialization Routine / 初始化流程
// ---------------------------------------------------------
void Main::setup() {
  pinMode(32, INPUT_PULLUP);
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, HIGH); 

  // 初始化通讯方式
  #if COMMUNICATION == COMM_SERIAL
    comm = new SerialCommunication();
  #elif COMMUNICATION == COMM_BTSERIAL
    comm = new BTSerialCommunication();
  #elif COMMUNICATION == COMM_WIFISERIAL
    comm = new WIFISerialCommunication();
  #else
    #error "Communication not set."
  #endif 

  // 初始化编码方式
  #if ENCODING == ENCODE_ALPHA
    encoding = new AlphaEncoding();
  #elif ENCODING == ENCODE_LEGACY
    encoding = new LegacyEncoding();
  #else
    #error "Encoding not set."
  #endif

  // 启动通讯
  comm->start();
  input.setupInputs();
  haptics.setupServoHaptics(); // Ensure servo frequency is initialized / 确保舵机频率已初始化

  // [System] Boot Sequence
  // Wait for Serial to stabilize and print status.
  // [系统] 启动序列
  // 等待串口稳定并打印状态。
  delay(100); 
  Serial.println("\n[Alice Firmware] System Online. Ready for commands.");
  Serial.println("[Alice Firmware] Waiting for data...");
  delay(10000); 

  // [Multi-threading] Launch Input Task on Secondary Core (if available)
  // [多线程] 在辅助核心上启动输入采集任务（如果可用）
  #if defined(ESP32_DUAL_CORE_SET)
    xTaskCreatePinnedToCore(
      Main::getInputsWrapper, 
      "Get_Inputs", 
      10000, 
      this, 
      tskIDLE_PRIORITY, 
      &Task1, 
      0); 
  #endif
}

// ---------------------------------------------------------
// LOOP: Main Running Logic / 主运行逻辑
// ---------------------------------------------------------
void Main::loop() {
  
  if (comm->isOpen()){

    // --- 1. Input gathering / 输入采集
    #if USING_CALIB_PIN
    data.calib = input.getButton(PIN_CALIB) != INVERT_CALIB;
    if (data.calib) loops = 0;
    #else
    data.calib = false;
    #endif
    
    if (loops < CALIBRATION_LOOPS || ALWAYS_CALIBRATING){
      calibrate = true;
      loops++;
    } else {
      calibrate = false;
    }

    #if !defined(ESP32_DUAL_CORE_SET)
      input.getFingerPositions(calibrate, data.calib, fingerPos);
    #endif

    // read button input / 读取按键
    data.joyClick = input.getButton(PIN_JOY_BTN) != INVERT_JOY;
    #if TRIGGER_GESTURE
      data.triggerButton = gesture.triggerGesture(fingerPos);
    #else
      data.triggerButton = input.getButton(PIN_TRIG_BTN) != INVERT_TRIGGER;
    #endif
    data.aButton = input.getButton(PIN_A_BTN) != INVERT_A;
    data.bButton = input.getButton(PIN_B_BTN) != INVERT_B;
    #if GRAB_GESTURE
      data.grab = gesture.grabGesture(fingerPos);
    #else
      data.grab = input.getButton(PIN_GRAB_BTN) != INVERT_GRAB;
    #endif
    #if PINCH_GESTURE
      data.pinch = gesture.pinchGesture(fingerPos);
    #else
      data.pinch = input.getButton(PIN_PNCH_BTN) != INVERT_PINCH;
    #endif
    data.menu = input.getButton(PIN_MENU_BTN) != INVERT_MENU;

    // Copy finger data / 复制手指数据（处理锁）
    int fingerPosCopy[10];
    {
      #if defined(ESP32_DUAL_CORE_SET)
      fingerPosLock->lock();
      #endif
      for (int i = 0; i < 10; i++){
        fingerPosCopy[i] = fingerPos[i];
      }
      #if defined(ESP32_DUAL_CORE_SET)
      fingerPosLock->unlock();
      #endif
    }
    memcpy(data.fingers, fingerPosCopy, 5 * sizeof(int));
    data.joyX = input.getJoyX();
    data.joyY = input.getJoyY();
    
    // --- 2. Send Data to Host / 发送数据给主机 ---
    static char encodedString[128] = {0};
    encoding->encode(data, encodedString);
    comm->output(encodedString);

    // --- 3. Fast Receive Logic / 极速接收逻辑
    #if USING_FORCE_FEEDBACK
      static char received[128]; 
      
      // Non-blocking read check / 非阻塞读取检查
      if (comm->readData(received)){
        
        // Use strlen (C) instead of String (C++) for performance.
        // 使用 strlen (C语言) 替代 String (C++) 以提升性能。
        if(strlen(received) >= 2) { 
           
           DecodedData recievedData = encoding->decodeData(received);
           haptics.writeServoHaptics(recievedData.servoValues); 
           
           if (recievedData.fields.specialCommandReceived){
               // Use strcmp for fast string comparison.
               // 使用 strcmp 进行快速字符串比较。
              if (strcmp(recievedData.command, "ClearData") == 0) {
                  // Serial.println("Command: ClearData"); // Uncomment for debugging / 调试时可取消注释
                  input.clearFlags();
              }
              #if FLEXION_MIXING == MIXING_SINCOS
              else if (strcmp(recievedData.command, "SaveInter") == 0) {
                  input.saveIntermediate();
              }
              #endif
              else if (strcmp(recievedData.command, "SaveTravel") == 0) {
                  input.saveTravel();
              }
           }
        }
      }
    #endif

      // [Loop Timing] Minimized Delay for Maximum Polling Rate
      // [循环时序] 最小化延迟以获取最大轮询率
    #if defined(ESP32)
      // Set to 0 in AdvancedConfig.h for max speed 
      // 在 AdvancedConfig.h 设为 0 以全速运行
      vTaskDelay(LOOP_TIME);
    #else
      delay(LOOP_TIME);
    #endif
  }
}

#if defined(ESP32_DUAL_CORE_SET)
void Main::getInputs(){
    for(;;){
      {
        fingerPosLock->lock();
        input.getFingerPositions(calibrate, data.calib, fingerPos);
        fingerPosLock->unlock();
      }
      threadLoops++;
      if (threadLoops%100 == 0) vTaskDelay(1);
      // delayMicroseconds(1);
    }           
}
void Main::getInputsWrapper(void* _this) {
  static_cast<Main*>(_this)->getInputs();
}
#endif


Main lucidGlovesApp;

void setup() {
  lucidGlovesApp.setup();
}


void loop() {
  lucidGlovesApp.loop();
}