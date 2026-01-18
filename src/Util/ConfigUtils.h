#pragma once
#ifndef CONFIGUTILS_H
#define CONFIGUTILS_H

// 爱丽丝的清理：扔掉那些沉重的 C++ 标准库头文件
// #include <mutex> 
// #include <condition_variable>
// #include <queue>

// 如果是 ESP32，我们使用原生的 FreeRTOS 信号量，这才是最快的
#if defined(ESP32)
  #include "freertos/FreeRTOS.h"
  #include "freertos/semphr.h"

  class ordered_lock {
      SemaphoreHandle_t xSemaphore;
  public:
      ordered_lock() {
          // 创建一个互斥信号量
          xSemaphore = xSemaphoreCreateMutex();
      }
      
      void lock() {
          // 姐姐的强硬控制：无限期等待，直到拿到锁
          // 相比 std::mutex，这个开销极小
          if (xSemaphore != NULL) {
            xSemaphoreTake(xSemaphore, portMAX_DELAY);
          }
      }
      
      void unlock() {
          // 释放锁，允许其他人进入
          if (xSemaphore != NULL) {
            xSemaphoreGive(xSemaphore);
          }
      }

      ~ordered_lock() {
          if (xSemaphore != NULL) {
            vSemaphoreDelete(xSemaphore);
          }
      }
  };

#else
  // 对于 Arduino Nano (AVR) 或其他板子，我们回退到简单的实现
  // 因为 AVR 通常是单核的，其实根本不需要锁，或者只能用简单的开关中断
  // 这里做一个简单的空壳，防止编译报错
  class ordered_lock {
  public:
      ordered_lock() {};
      void lock() {};   // 单核不需要锁
      void unlock() {}; // 单核不需要锁
  };
#endif

//Comm
#define COMM_SERIAL 0   
#define COMM_BTSERIAL 1 
#define COMM_WIFISERIAL 2

//Encode
#define ENCODE_LEGACY 0
#define ENCODE_ALPHA  1

//Multiplexer
#define MUX(p) (p + 100)
#define UNMUX(p) (p % 100)
#define ISMUX(p) (p >= 100)

//finger mixing
#define MIXING_NONE 0
#define MIXING_SINCOS 2

//intermediate filtering
#define INTERFILTER_NONE 0
#define INTERFILTER_LIMITS 1
#define INTERFILTER_ALL 2

#endif