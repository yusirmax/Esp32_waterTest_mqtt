#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "logic.h"
#include "web.h"

// 处理根路径请求
void handleRoot()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>"; // 设置字符编码为UTF-8
    html += "<title>ESP32配网页面</title>";
    html += "<style>"
            "body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }"
            "form { display: flex; flex-direction: column; }"
            "input { margin: 15px 0; }"
            "</style>";
    html += "<script>"
            "function copyToClipboard() {"
            "  var copyText = document.getElementById('deviceID');"
            "  copyText.select();"
            "  copyText.setSelectionRange(0, 99999);"
            "  document.execCommand('copy');"
            "  alert('设备ID已复制: ' + copyText.value);"
            "}"
            "</script>";
    html += "</head><body>";
    html += "<form action='/config' method='POST'>";
    html += "<h1>ESP32配网页面</h1>";
    html += "设备ID: <input type='text' id='deviceID' value='" + MQTT_ClientID + "' readonly><br>";
    html += "<button type='button' onclick='copyToClipboard()'>复制设备ID</button><br>";
    html += "WiFi SSID: <input type='text' name='ssid'><br>";
    html += "WiFi密码: <input type='password' name='password'><br>";
    html += "<input type='submit' value='连接'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

// 处理配置请求
void handleConfig()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (ssid.length() > 0 && password.length() > 0)
    {
        // 保存WiFi信息到EEPROM
        writeEEPROM(SSID_ADDR, ssid);
        writeEEPROM(PASS_ADDR, password);
        EEPROM.commit();

        // 尝试连接WiFi
        connectToWiFi(ssid, password);

        // 返回成功页面
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>"; // 设置字符编码为UTF-8
        html += "<title>配置成功</title>";
        html += "</head><body>";
        html += "<h1>配置成功！</h1>";
        html += "<p>设备正在尝试连接WiFi...可以关闭此页面了</p>";
        html += "<script>"
                "function copyToClipboard() {"
                "  var copyText = document.getElementById('deviceID');"
                "  copyText.select();"
                "  copyText.setSelectionRange(0, 99999);"
                "  document.execCommand('copy');"
                "  alert('设备ID已复制: ' + copyText.value);"
                "}"
                "</script>";
        html += "</head><body>";
        html += "<form action='/config' method='POST'>";
        html += "<h2>请复制此设备id粘贴到客户端</h2>";
        html += "设备ID: <input type='text' id='deviceID' value='" + MQTT_ClientID + "' readonly><br>";
        html += "<button type='button' onclick='copyToClipboard()'>复制设备ID</button><br>";
        html += "</body></html>";
        server.send(200, "text/html", html);
    }
    else
    {
        // 返回错误页面
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>"; // 设置字符编码为UTF-8
        html += "<title>错误</title>";
        html += "</head><body>";
        html += "<h1>错误：SSID和密码不能为空！</h1>";
        html += "</body></html>";
        server.send(400, "text/html", html);
    }
}