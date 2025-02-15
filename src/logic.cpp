#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h> // MQTT库
#include <ArduinoJson.h>
#include "main.h"
#include "logic.h"
#include "web.h"

// 全局变量
String savedSSID = "";            // 默认出厂保存的WiFiSSID
String savedPassword = "";        // 默认出厂保存的WiFi密码
unsigned long touchStartTime = 0; // 触摸开始时间
bool isTouching = false;          // 触摸状态
bool isConnected = false;         // MQTT连接状态

// WiFi配置
const char *apSSID = "ESP32_WaterTest_Config"; // AP模式的热点名称
const char *apPassword = "12345678";           // AP模式的密码

// MQTT配置
const char *mqtt_server = "113.45.152.55"; // MQTT服务器地址
// const char *mqtt_topic = "waterTest/sensor_data";
const uint16_t mqtt_port = 1883;                 // MQTT服务器端口
String MQTT_ClientID = generateUniqueClientID(); // 生成唯一的客户端ID
const char *mqtt_topic = MQTT_ClientID.c_str();  // 订阅主题为设备ID

// 生成唯一的客户端ID
String generateUniqueClientID()
{
    uint64_t chipid = ESP.getEfuseMac();               // 获取芯片ID
    String clientId = "WaterTest_";                    // 客户端ID前缀
    clientId += String((uint16_t)(chipid >> 16), HEX); // 使用芯片ID的一部分
    clientId += String((uint32_t)chipid, HEX);
    return clientId.substring(0, 16); // 客户端ID长度为16
}

// Web服务器
WebServer server(80);

// MQTT客户端
WiFiClient espClient;
PubSubClient client(espClient);

// 读取EEPROM数据
String readEEPROM(int addr)
{
    String data = "";
    for (int i = addr; i < addr + 32; i++)
    {
        char c = EEPROM.read(i);
        if (c == 0)
            break;
        data += c;
    }
    return data;
}

// 写入EEPROM数据
void writeEEPROM(int addr, String data)
{
    for (int i = 0; i < data.length(); i++)
    {
        EEPROM.write(addr + i, data[i]);
    }
    EEPROM.write(addr + data.length(), 0); // 添加字符串结束符
}

// 启动AP模式
void startAPMode()
{
    Serial.println("启动AP模式...");
    WiFi.softAP(apSSID, apPassword);
    Serial.print("AP IP地址: ");
    Serial.println(WiFi.softAPIP());
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str()); // 接入点mac地址

    // 设置Web服务器路由
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.begin();
}

// 连接到WiFi
void connectToWiFi(String ssid, String password)
{
    Serial.println("尝试连接WiFi...");
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) // 尝试连接最多20次
    {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi连接成功");
        Serial.print("IP地址: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi连接失败,进入AP模式");

        // 确保退出Wi-Fi客户端模式
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        startAPMode();
    }
}

// 清除WiFi配置
void clearWiFiConfig()
{
    Serial.println("清除WiFi配置...");
    for (int i = 0; i < EEPROM_SIZE; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}

// 处理触摸事件
void handleTouch()
{
    int touchValue = touchRead(TOUCH_PIN);

    if (touchValue < TOUCH_THRESHOLD)
    {
        if (!isTouching)
        {
            // 开始触摸
            isTouching = true;
            touchStartTime = millis();
        }
        else if (millis() - touchStartTime >= TOUCH_DURATION)
        {
            // 触摸超过5秒，清除配网信息并重启
            clearWiFiConfig();
            ESP.restart();
        }
    }
    else
    {
        // 停止触摸
        isTouching = false;
    }
}

void Ap_mode()
{
    // 如果处于AP模式，处理Web服务器请求
    if (WiFi.getMode() == WIFI_AP)
    {
        server.handleClient();
    }
}

void wifi_init()
{
    // 尝试从EEPROM读取保存的WiFi信息
    EEPROM.begin(EEPROM_SIZE);
    savedSSID = readEEPROM(SSID_ADDR);
    savedPassword = readEEPROM(PASS_ADDR);
    Serial.println("从EEPROM读取WiFi信息：");
    // 如果EEPROM中没有保存的WiFi信息，进入AP模式
    if (savedSSID == "" || savedPassword == "")
    {
        startAPMode();
    }
    else
    {
        // 尝试连接保存的WiFi信息
        Serial.printf("SSID: %s\n", savedSSID.c_str());
        connectToWiFi(savedSSID, savedPassword);
        //  初始化MQTT客户端
        client.setServer(mqtt_server, mqtt_port);
        client.setCallback(callback);
    }
}

// 断线重连功能
void reconnect_mqtt()
{
    while (!client.connected() && WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED)
    {
        Serial.print("MQTT connection...");
        if (client.connect(mqtt_topic))
        {
            client.subscribe(mqtt_topic);
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" esp32 will restart in 2 seconds to connect mqtt server");
            delay(2000);
            ESP.restart();
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    String messageTemp;
    for (unsigned int i = 0; i < length; i++)
    {
        messageTemp += (char)payload[i];
    }

    if (messageTemp.startsWith("{\"setting\":"))
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, messageTemp);
        if (!error)
        {
            JsonObject obj = doc.as<JsonObject>();
            Serial.println("Received setting JSON:");
            serializeJsonPretty(obj, Serial);
            Serial.println();

            // 从"setting"字段获取一些信息
            const char *settingValue = obj["setting"];
            Serial.printf("Setting Value: %s\n", settingValue);
            Serial.println("****************************************************");
            // 根据接收到的信息执行相应的动作
            // 这里加入业务逻辑
        }
        else
        {
            Serial.println("Failed to parse JSON");
        }
    }
}
void sendSensorData()
{
    JsonDocument doc;
    doc["sensor"] = mqtt_topic;
    doc["temperature"] = random(20, 30);
    doc["humidity"] = random(50, 70);
    doc["tds"] = random(20, 30);
    doc["ph"] = random(1, 14);
    doc["turbidity"] = random(20, 30);

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));
    Serial.printf("Sending data: %s\n", buffer);
    client.publish(mqtt_topic, buffer);
}

void mqtt_loop()
{
    if (WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && WiFi.localIP().toString() != "0.0.0.0")
    {
        if (!client.connected()) // 如果MQTT客户端未连接，则尝试连接
        {
            reconnect_mqtt();
        }
        else
        {
            client.loop();
            sendSensorData();
        }
    }
    delay(1000);
}