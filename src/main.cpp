#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <CSE7766.h>
#include <CAT9554.h>
#include <Wire.h>

#include <EEPROM.h> //导入Flash库文件

#define CAT9554_SDA_PIN 3
#define CAT9554_SCL_PIN 12
#define CAT9554_IRQ_PIN 4

#define CSE7766_RX_PIN 13
#define CSE7766_BAUDRATE 4800

#define LED_PIN 0       // 指示灯
#define LOGO_LED_PIN 14 // Logo指示灯

#define KEY_0_PIN 16 // 总开关
#define KEY_1_PIN 0  // 开关1
#define KEY_2_PIN 1  // 开关2
#define KEY_3_PIN 2  // 开关3

#define REL_0_PIN 7 // 总继电器
#define REL_1_PIN 6 // 继电器1
#define REL_2_PIN 5 // 继电器2
#define REL_3_PIN 4 // 继电器3

String ssid;
String psw;

CSE7766 myCSE7766;
const int httpPort = 80;
String deviceName = "斐讯DC1插排";
String version = "1.1";
ESP8266WebServer server(httpPort);
const char* serverIndex = "<h1>更新固件：</h1><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='开始上传升级'></form>";
// 开关的当前状态
bool logLedStatus = true;
bool wifiLedStatus = true;
// 插口
bool plugin4Status = true;
bool plugin5Status = true;
bool plugin6Status = true;
// 总开关
bool plugin7Status = true;

struct config_type
{
  char stassid[32];//定义配网得到的WIFI名长度(最大32字节)
  char stapsw[64];//定义配网得到的WIFI密码长度(最大64字节)
};

config_type config;//声明定义内容

void saveConfig()//保存函数
{
 EEPROM.begin(1024);//向系统申请1024kb ROM
 //开始写入
 uint8_t *p = (uint8_t*)(&config);
  for (int i = 0; i < sizeof(config); i++)
  {
    EEPROM.write(i, *(p + i)); //在闪存内模拟写入
  }
  EEPROM.commit();//执行写入ROM
}

void loadConfig()//读取函数
{
  EEPROM.begin(1024);
  uint8_t *p = (uint8_t*)(&config);
  for (int i = 0; i < sizeof(config); i++)
  {
    *(p + i) = EEPROM.read(i);
  }
  EEPROM.commit();
  ssid = config.stassid;
  psw = config.stapsw;
}

void smartConfig()//配网函数
{
  // 等待配网
  WiFi.beginSmartConfig();
  while(1)
    {
      digitalWrite(LED_PIN, LOW);  //加个LED快闪，确认配网是否成功！成功就不闪了。
      delay(200);                    
      digitalWrite(LED_PIN, HIGH);   
      delay(200);
      if (WiFi.smartConfigDone())
      {
        strcpy(config.stassid,WiFi.SSID().c_str());//名称复制
        strcpy(config.stapsw,WiFi.psk().c_str());//密码复制
        saveConfig();//调用保存函数
        WiFi.setAutoConnect(true);  // 设置自动连接
        break;
      }
    }
}

// web服务器的根目录
void handleRoot() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
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
        digitalWrite(LOGO_LED_PIN, LOW);
        logLedStatus = true;
      }else if (server.arg(i)=="wifiLed")
      {
        digitalWrite(LED_PIN, LOW);
        wifiLedStatus = true;
      }else if (server.arg(i)=="plugin4")
      {
        CAT9554.digitalWrite(REL_3_PIN, HIGH);
        plugin4Status = true;
      }else if (server.arg(i)=="plugin5")
      {
        CAT9554.digitalWrite(REL_2_PIN, HIGH);
        plugin5Status = true;
      }else if (server.arg(i)=="plugin6")
      {
        CAT9554.digitalWrite(REL_1_PIN, HIGH);
        plugin6Status = true;
      }else if (server.arg(i)=="plugin7")
      {
        CAT9554.digitalWrite(REL_0_PIN, HIGH);
        plugin7Status = true;
      }
    }else if (server.argName(i)=="off"){
      // 关闭
      if (server.arg(i)=="logLed")
      {
        digitalWrite(LOGO_LED_PIN, HIGH);
        logLedStatus = false;
      }else if (server.arg(i)=="wifiLed")
      {
        digitalWrite(LED_PIN, HIGH);
        wifiLedStatus = false;
      }else if (server.arg(i)=="plugin4")
      {
        CAT9554.digitalWrite(REL_3_PIN, LOW);
        plugin4Status = false;
      }else if (server.arg(i)=="plugin5")
      {
        CAT9554.digitalWrite(REL_2_PIN, LOW);
        plugin5Status = false;
      }else if (server.arg(i)=="plugin6")
      {
        CAT9554.digitalWrite(REL_1_PIN, LOW);
        plugin6Status = false;
      }else if (server.arg(i)=="plugin7")
      {
        CAT9554.digitalWrite(REL_0_PIN, LOW);
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
  myCSE7766.setRX(CSE7766_RX_PIN);
  myCSE7766.begin();
  // IO扩展CAT9554
  Wire.begin(CAT9554_SDA_PIN, CAT9554_SCL_PIN);
  Wire.setClock(4800);
  CAT9554.begin();
  // CAT9554.begin(SDA_PIN, SCL_PIN);
  // 按钮检测，总开关GPIO16，其他插孔:
  CAT9554.pinMode(KEY_0_PIN, INPUT);
  CAT9554.pinMode(KEY_1_PIN, INPUT);
  CAT9554.pinMode(KEY_2_PIN, INPUT);
// 初始化全开
  CAT9554.pinMode(REL_3_PIN, OUTPUT);
  CAT9554.digitalWrite(REL_3_PIN, HIGH);
  CAT9554.pinMode(REL_2_PIN, OUTPUT);
  CAT9554.digitalWrite(REL_2_PIN, HIGH);
  CAT9554.pinMode(REL_1_PIN, OUTPUT);
  CAT9554.digitalWrite(REL_1_PIN, HIGH);
  CAT9554.pinMode(REL_0_PIN, OUTPUT);
  CAT9554.digitalWrite(REL_0_PIN, HIGH);
    // 开关状态初始化为开
  pinMode(LOGO_LED_PIN, OUTPUT);
  digitalWrite(LOGO_LED_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(500);
  // 选取一种连接路由器的方式 
  // WiFi.begin(ssid, password);
  ESP.wdtEnable(10000);//设定看门狗
  loadConfig();//读取ROM是否包含密码
  //判断ROM是否有密码
  if(ssid!=0&&psw!=0){
    WiFi.begin(ssid,psw);//如果有密码则自动连接
    while(WiFi.status()!= WL_CONNECTED){
      if(digitalRead(KEY_0_PIN)== LOW){
        smartConfig();//如果配网按钮被按下则停止当前连接开始配网
        break;//跳出所有循环进入主程序
      }
      digitalWrite(LED_PIN, LOW);  //加个LED慢闪，确认联网是否成功！成功就不闪了。
      delay(1000);                    
      digitalWrite(LED_PIN, HIGH);   
      delay(1000);
    }
  }else{
    smartConfig();//如果ROM没有密码则自动进入配网模式
  }

  if (MDNS.begin("phicomm-dc1-"+String(ESP.getFlashChipId()))) {
    // Serial.println("MDNS responder started");
  }

  MDNS.addService("iotdevice", "tcp", httpPort);
  MDNS.addServiceTxt("iotdevice", "tcp", "name", deviceName);
  MDNS.addServiceTxt("iotdevice", "tcp", "model", "com.iotserv.devices.phicomm_dc1");
  MDNS.addServiceTxt("iotdevice", "tcp", "mac", WiFi.macAddress());
  MDNS.addServiceTxt("iotdevice", "tcp", "id", ESP.getSketchMD5());
  // MDNS.addServiceTxt("iotdevice", "tcp", "ui-support", "web,native");
  // MDNS.addServiceTxt("iotdevice", "tcp", "ui-first", "native");
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
 ESP.wdtFeed();//先喂狗释放资源
 if(digitalRead(KEY_0_PIN)== 0)delay(2000);if(digitalRead(KEY_0_PIN)==0)smartConfig();//如果配网按钮被按下则停止所有任务开始重新配网
  ESP.wdtFeed();//再喂狗释放资源

  MDNS.update();
  server.handleClient();
  myCSE7766.handle();
  if(WiFi.status() != WL_CONNECTED){
    WiFi.reconnect();
    delay(500);
  }
}
