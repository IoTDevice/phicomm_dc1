#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
const int httpPort = 80;
String deviceName = "斐讯DC1插排";
String version = "1.0";
ESP8266WebServer server(httpPort);
// 开关的状态表示
const int on = 1;
const int off = 0;
// 看你的继电器是连接那个io，默认gpio0
const int logLed = 14;
const int wifiLed = 0;
const int primarySwitch = 16;
// 开关的当前状态
int logLedStatus = on;
int wifiLedStatus = on;
int primarySwitchStatus = on;
// digitalWrite(led1, on);

// web服务器的根目录
void handleRoot() {
  server.send(200, "text/html", "<h1>this is index page from esp8266!</h1>");
}
// 操作LED开关状态的API
void handleLEDStatusChange(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="pin")
    {
      // 开启
      if (server.arg(i)=="ONlogLed")
      {
        digitalWrite(logLed, on);
        logLedStatus = on;
      }else if (server.arg(i)=="ONwifiLed")
      {
        digitalWrite(wifiLed, on);
        wifiLedStatus = on;
      }else if (server.arg(i)=="ONprimarySwitch")
      {
        digitalWrite(primarySwitch, on);
        primarySwitchStatus = on;
      }
      // 关闭
      else if (server.arg(i)=="OFFlogLed")
      {
        digitalWrite(logLed, off);
        logLedStatus = off;
      }else if (server.arg(i)=="OFFwifiLed")
      {
        digitalWrite(wifiLed, off);
        wifiLedStatus = off;
      }else if (server.arg(i)=="OFFprimarySwitch")
      {
        digitalWrite(primarySwitch, off);
        primarySwitchStatus = off;
      }
    }
  }
  server.send(200, "application/json", message);
}
// 设备改名的API
void handleDeviceRename(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="name")
    {
      deviceName = server.arg(i);
    }
  }
  server.send(200, "application/json", message);
}
// 当前的LED开关状态API
void handleCurrentLEDStatus(){
  String message;
  message = "{\"logLed\":"+String(logLedStatus)+
  ",\"wifiLed\":"+String(wifiLedStatus)+
  ",\"primarySwitch\":"+String(primarySwitchStatus)+
  ",\"code\":0,\"message\":\"success\"}";
  server.send(200, "application/json", message);
}
// 设备信息
void handleDeviceInfo(){
  String message;
  message = "{\n";
  message += "\"name\":\""+deviceName +"\",\n";
  message += "\"model\":\"com.iotserv.devices.phicomm_dc1\",\n";
  message += "\"mac\":\""+WiFi.macAddress()+"\",\n";
  message += "\"id\":\""+String(ESP.getFlashChipId())+"\",\n";
  message += "\"ui-support\":[\"web\",\"native\"],\n";
  message += "\"ui-first\":\"native\",\n";
  message += "\"author\":\"Farry\",\n";
  message += "\"email\":\"newfarry@126.com\",\n";
  message += "\"home-page\":\"https://github.com/iotdevice\",\n";
  message += "\"firmware-respository\":\"https://github.com/iotdevice/phicomm_dc1\",\n";
  message += "\"firmware-version\":\""+version+"\"\n";
  message +="}";
  server.send(200, "application/json", message);
}

// 页面或者api没有找到
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void){
  // 开关状态初始化为开
  pinMode(logLed, OUTPUT);
  digitalWrite(logLed, on);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, on);
  pinMode(primarySwitch, OUTPUT);
  digitalWrite(primarySwitch, on);

  // Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // 选取一种连接路由器的方式 
  // WiFi.begin(ssid, password);
  WiFi.beginSmartConfig();

  // Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(ssid);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    // Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/led", handleLEDStatusChange);
  server.on("/rename", handleDeviceRename);
  server.on("/status", handleCurrentLEDStatus);
  // about this device
  server.on("/info", handleDeviceInfo);
  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.println("HTTP server started");
  MDNS.addService("iotdevice", "tcp", httpPort);
}

void loop(void){
  MDNS.update();
  server.handleClient();
}
