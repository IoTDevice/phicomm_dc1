#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <CSE7766.h>
#include <CAT9554.h>
#include <Wire.h>

#define SDA_PIN      3
#define SCL_PIN      12

CSE7766 myCSE7766;
const int httpPort = 80;
String deviceName = "斐讯DC1插排";
String version = "1.1";
ESP8266WebServer server(httpPort);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
// 看你的继电器是连接那个io，默认gpio0
const int logLed = 14;
const int wifiLed = 0;
// 插口
const int plugin4 = 4;
const int plugin5 = 5;
const int plugin6 = 6;
// 总开关
const int plugin7 = 7;
// 开关的当前状态
bool logLedStatus = true;
bool wifiLedStatus = true;
// 插口
bool plugin4Status = true;
bool plugin5Status = true;
bool plugin6Status = true;
// 总开关
bool plugin7Status = true;

// digitalWrite(led1, on);

// web服务器的根目录
void handleRoot() {
  server.send(200, "text/html", "<h1>this is index page from esp8266!</h1>");
}
// 操作LED开关状态的API
void handleSwitchStatusChange(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="on")
    {
      // 开启
      if (server.arg(i)=="logLed")
      {
        digitalWrite(logLed, LOW);
        logLedStatus = true;
      }else if (server.arg(i)=="wifiLed")
      {
        digitalWrite(wifiLed, LOW);
        wifiLedStatus = true;
      }else if (server.arg(i)=="plugin4")
      {
        CAT9554.digitalWrite(plugin4, HIGH);
        plugin4Status = true;
      }else if (server.arg(i)=="plugin5")
      {
        CAT9554.digitalWrite(plugin5, HIGH);
        plugin5Status = true;
      }else if (server.arg(i)=="plugin6")
      {
        CAT9554.digitalWrite(plugin6, HIGH);
        plugin6Status = true;
      }else if (server.arg(i)=="plugin7")
      {
        CAT9554.digitalWrite(plugin7, HIGH);
        plugin7Status = true;
      }
    }else if (server.argName(i)=="off"){
      // 关闭
      if (server.arg(i)=="logLed")
      {
        digitalWrite(logLed, HIGH);
        logLedStatus = false;
      }else if (server.arg(i)=="wifiLed")
      {
        digitalWrite(wifiLed, HIGH);
        wifiLedStatus = false;
      }else if (server.arg(i)=="plugin4")
      {
        CAT9554.digitalWrite(plugin4, LOW);
        plugin4Status = false;
      }else if (server.arg(i)=="plugin5")
      {
        CAT9554.digitalWrite(plugin5, LOW);
        plugin5Status = false;
      }else if (server.arg(i)=="plugin6")
      {
        CAT9554.digitalWrite(plugin6, LOW);
        plugin6Status = false;
      }else if (server.arg(i)=="plugin7")
      {
        CAT9554.digitalWrite(plugin7, LOW);
        plugin7Status = false;
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
// 电力芯片cse7766
void handleCSE7766(){
  double value = 0;
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="type")
    {
      if (server.arg(i)=="getVoltage"){
        value = myCSE7766.getVoltage();
      }else if (server.arg(i)=="getCurrent"){
        value = myCSE7766.getCurrent();
      }else if (server.arg(i)=="getActivePower"){
        value = myCSE7766.getActivePower();
      }else if (server.arg(i)=="getApparentPower"){
        value = myCSE7766.getApparentPower();
      }else if (server.arg(i)=="getReactivePower"){
        value = myCSE7766.getReactivePower();
      }else if (server.arg(i)=="getPowerFactor"){
        value = myCSE7766.getPowerFactor();
      }else if (server.arg(i)=="getEnergy"){
        value = myCSE7766.getEnergy();
      }
    }
  }
  String message = "{\"code\":0,\"value\":"+String(value)+",\"message\":\"success\"}";
  server.send(200, "application/json", message);
}
// 当前的LED开关状态API
void handleCurrentLEDStatus(){
  String message;
  message = "{\"logLed\":"+String(logLedStatus)+
  ",\"wifiLed\":"+String(wifiLedStatus)+
  ",\"plugin4\":"+String(plugin4Status)+
  ",\"plugin5\":"+String(plugin5Status)+
  ",\"plugin6\":"+String(plugin6Status)+
  ",\"plugin7\":"+String(plugin7Status)+
  ",\"Voltage\":"+String(myCSE7766.getVoltage())+
  ",\"Current\":"+String(myCSE7766.getCurrent())+
  ",\"ActivePower\":"+String(myCSE7766.getActivePower())+
  ",\"ApparentPower\":"+String(myCSE7766.getApparentPower())+
  ",\"ReactivePower\":"+String(myCSE7766.getReactivePower())+
  ",\"PowerFactor\":"+String(myCSE7766.getPowerFactor())+
  ",\"Energy\":"+String(myCSE7766.getEnergy())+
  ",\"code\":0,\"message\":\"success\"}";
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
  // 电力芯片
  myCSE7766.setRX(13);
  myCSE7766.begin();
  // IO扩展
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(20000);
  CAT9554.begin();
  // CAT9554.begin(SDA_PIN, SCL_PIN);
  // 按钮检测，总开关GPIO16，其他插孔:
  CAT9554.pinMode(0, INPUT);
  CAT9554.pinMode(1, INPUT);
  CAT9554.pinMode(2, INPUT);

  CAT9554.pinMode(4, OUTPUT);
  CAT9554.digitalWrite(4, HIGH);
  CAT9554.pinMode(5, OUTPUT);
  CAT9554.digitalWrite(5, HIGH);
  CAT9554.pinMode(6, OUTPUT);
  CAT9554.digitalWrite(6, HIGH);
  CAT9554.pinMode(7, OUTPUT);
  CAT9554.digitalWrite(7, HIGH);
    // 开关状态初始化为开
  pinMode(logLed, OUTPUT);
  digitalWrite(logLed, LOW);
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  // Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // 选取一种连接路由器的方式 
  // WiFi.begin(ssid, password);
  WiFi.beginSmartConfig();

  // Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(wifiLed, HIGH);
    delay(500);
    digitalWrite(wifiLed, LOW);
    // Serial.print(".");
  }
  // Serial.println("");
  // Serial.print("Connected to ");
  // Serial.println(ssid);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  if (MDNS.begin("phicomm-dc1-"+String(ESP.getFlashChipId()))) {
    // Serial.println("MDNS responder started");
  }

  MDNS.addService("iotdevice", "tcp", httpPort);
  MDNS.addServiceTxt("iotdevice", "tcp", "name", deviceName);
  MDNS.addServiceTxt("iotdevice", "tcp", "model", "com.iotserv.devices.phicomm_dc1");
  MDNS.addServiceTxt("iotdevice", "tcp", "mac", WiFi.macAddress());
  MDNS.addServiceTxt("iotdevice", "tcp", "id", ESP.getSketchMD5());
  MDNS.addServiceTxt("iotdevice", "tcp", "ui-support", "web,native");
  MDNS.addServiceTxt("iotdevice", "tcp", "ui-first", "native");
  MDNS.addServiceTxt("iotdevice", "tcp", "author", "Farry");
  MDNS.addServiceTxt("iotdevice", "tcp", "email", "newfarry@126.com");
  MDNS.addServiceTxt("iotdevice", "tcp", "home-page", "https://github.com/iotdevice");
  MDNS.addServiceTxt("iotdevice", "tcp", "firmware-respository", "https://github.com/iotdevice/phicomm_dc1");
  MDNS.addServiceTxt("iotdevice", "tcp", "firmware-version", version);

  server.on("/", handleRoot);
  server.on("/switch", handleSwitchStatusChange);
  server.on("/rename", handleDeviceRename);
  server.on("/status", handleCurrentLEDStatus);

  server.on("/cse7766", handleCSE7766);

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "{\"code\":1,\"message\":\"fail\"}" : "{\"code\":0,\"message\":\"success\"}");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        
      } else {
        
      }
    }
    yield();
  });

  server.on("/ota", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.println("HTTP server started");
  MDNS.addService("iotdevice", "tcp", httpPort);
}

void loop(void){
  MDNS.update();
  server.handleClient();
  myCSE7766.handle();
  if(!WiFi.isConnected){
    WiFi.reconnect();
    delay(500);
  }
}
