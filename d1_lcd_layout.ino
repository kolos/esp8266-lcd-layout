#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266WebServer.h>
#include <FS.h>

#include "constants.h"

const int RS = D2, EN = D3, d4 = D5, d5 = D6, d6 = D7, d7 = D8;
LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);

ESP8266WebServer server(80);
File fsUploadFile;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  Serial.print("Connecting to WiFi");

  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DHCP_CLIENTNAME);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print(" connected: ");
  Serial.println(WiFi.localIP());

  NBNS.begin(DHCP_CLIENTNAME);
  lcd.begin(16, 2);

  SPIFFS.begin();

  server.on("/lcd", HTTP_POST, handleLcdRequest);
  server.on("/lcd_customchar", HTTP_POST, handleLcdCustomCharRequest);

  server.on("/upload_file", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.serveStatic("/", SPIFFS, "/");

  server.begin();
  Serial.println("HTTP server started");
}

void handleLcdRequest() {
  if (! server.hasArg("data")) {
    server.send(503, "text/plain", "error");
  }

  String data = server.arg("data");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(data.substring(0, 16));

  if (data.length() > 16) {
    lcd.setCursor(0, 1);
    lcd.print(data.substring(16));
  }

  server.sendHeader("Refresh", "1; url=/");
  server.send(200, "text/plain", "LCD updated, redirect in 1 secs..");
}

void handleLcdCustomCharRequest() {
  if (! server.hasArg("char")
      || ! server.hasArg("byte_0")
      || ! server.hasArg("byte_1")
      || ! server.hasArg("byte_2")
      || ! server.hasArg("byte_3")
      || ! server.hasArg("byte_4")
      || ! server.hasArg("byte_5")
      || ! server.hasArg("byte_6")
      || ! server.hasArg("byte_7")
     ) {
    server.send(503, "text/plain", "error");
  }

  Serial.print("New customchar: ");
  byte customChar[8];
  customChar[0] = server.arg("byte_0").toInt();
  customChar[1] = server.arg("byte_1").toInt();
  customChar[2] = server.arg("byte_2").toInt();
  customChar[3] = server.arg("byte_3").toInt();
  customChar[4] = server.arg("byte_4").toInt();
  customChar[5] = server.arg("byte_5").toInt();
  customChar[6] = server.arg("byte_6").toInt();
  customChar[7] = server.arg("byte_7").toInt();
  
  for (byte i = 0; i < 8; i++) {
    Serial.print("0x");
    Serial.print(customChar[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  lcd.createChar(
    server.arg("char").toInt(),
    customChar
  );

  server.sendHeader("Refresh", "1; url=/");
  server.send(200, "text/plain", "LCD CustomChars updated, redirect in 1 secs..");
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    fsUploadFile = SPIFFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      server.sendHeader("Refresh", "5; url=/");
      server.send(200, "text/plain", "File uploaded, redirect in 5 secs..");
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void loop() {
  server.handleClient();
}
