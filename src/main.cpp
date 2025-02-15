#include <Arduino.h>
#include <WebServer.h>
#include "main.h"
#include "pin.h"
#include "logic.h"
#include "tds.h"

void main_init()
{
    Serial.begin(115200);
    Serial.print("FIRMWARE_VERSION:");
    Serial.println(FIRMWARE_VERSION);
    Serial.println(String("Client ID:") + MQTT_ClientID.c_str()); // 打印客户端ID
}

// 启动时的代码，只执行一次
void setup()
{
    main_init(); // 初始化
    pin_init();  // 初始化引脚
    wifi_init(); // 初始化WiFi并连接
}

// 循环执行的代码
void loop()
{

    handleTouch(); // 检测触摸事件
    Ap_mode();     // 监测是否处于AP模式
    mqtt_loop();   // MQTT循环
    // tds_Value();   // 获取TDS值
}