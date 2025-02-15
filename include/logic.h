// EEPROM配置
#define EEPROM_SIZE 256 // EEPROM大小
#define SSID_ADDR 0     // SSID地址
#define PASS_ADDR 32    // 密码地址

// 触摸引脚配置
#define TOUCH_PIN 4         // 使用GPIO 4作为触摸引脚
#define TOUCH_THRESHOLD 20  // 触摸阈值20
#define TOUCH_DURATION 5000 // 触摸持续时间5秒

// 函数声明
String generateUniqueClientID();
void startAPMode();
void connectToWiFi(String ssid, String password);
String readEEPROM(int addr);
void writeEEPROM(int addr, String data);
void handleTouch();
void clearWiFiConfig();
void Ap_mode();
void wifi_init();
void reconnect_mqtt();
void callback(char *topic, byte *payload, unsigned int length);
void sendSensorData();
void mqtt_loop();

extern String MQTT_ClientID;
extern WebServer server;