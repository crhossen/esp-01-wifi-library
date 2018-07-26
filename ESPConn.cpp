#include "Arduino.h"
#include <SoftwareSerial.h>
#include "ESPConn.h"

const char* READY_TERMINATORS[] = { "WIFI DISCONNECT", "WIFI CONNECTED" };

ESPConn::ESPConn(int rxPin, int txPin) :
  esp(SoftwareSerial(rxPin, txPin))
{
  _rxPin = rxPin;
  _txPin = txPin;
  _debug = true;
}

ESPConn::ESPConn(int rxPin, int txPin, bool debug) :
  esp(SoftwareSerial(rxPin, txPin))
{
  _rxPin = rxPin;
  _txPin = txPin;
  _debug = debug;
}

bool ESPConn::setupSerial() {
  // Setup ESP connection
  pinMode(_rxPin, INPUT);
  pinMode(_txPin, OUTPUT);
  esp.begin(9600);
  delay(1000);
  
  return true;
}

int ESPConn::sendReset() {
  // Send AT command
  esp.println("AT+RST");
  readUntilLine("ready");
  int resetStatus = readUntilLineOptions(READY_TERMINATORS, 2);
  if (resetStatus == 0) {
    if (_debug) Serial.println("ESPConn - Looks not connected.");
    return 0;
  } 
  // if it is connected wait for IP
  else if (resetStatus == 1) {
    if (_debug) Serial.println("ESPConn - Looks already connected.");
    readUntilLine("WIFI GOT IP");
    return 1;
  }
}

String ESPConn::readLine() {
  String str = esp.readStringUntil('\r');
  if (esp.available() > 0) esp.read();
  str.trim();
  return str;
}

bool ESPConn::connectToAP(String ssid, String pass) {
  esp.println("AT+CWMODE_CUR=3"); //station mode
  readUntilOKorERROR();
  esp.print("AT+CWJAP=\"");
  esp.print(ssid); esp.print("\",\""); esp.print(pass); esp.println('"');
  return readUntilOKorERROR();
}

String ESPConn::getIPAddr() {
  esp.println("AT+CIPSTA_CUR?");

  // first part of response in quotes is the ip
  esp.readStringUntil('"');
  String ip = esp.readStringUntil('"');
  if (_debug) Serial.print(ip);
  readUntilOKorERROR();
  return ip;
}

bool ESPConn::readUntilLine(String text) {
  String line;
  int emptyLineCount = 0;
  do {
    line = readLine();
    if (_debug)  Serial.println(line);
    if (line.equals("")) {
      ++emptyLineCount;
    } else {
      emptyLineCount = 0;
    }
  } while (!line.equals(text) && emptyLineCount < 10);
  if (emptyLineCount >= 10) pipeSerial();
  return true;
}

int ESPConn::readUntilLineOptions(const char* texts[], int length) {
  String line;
  int foundTextIdx = -1;
  do {
    line = readLine();
    if (_debug) Serial.println(line);
    for (int i = 0; i < length; i++) {
      if (line.equals(texts[i])) {
        foundTextIdx = i;
      }
    }
  } while (foundTextIdx < 0);
  return true;
}

bool ESPConn::readUntilOKorERROR() {
  String line;
  do {
    line = readLine();
    if (_debug) Serial.println(line);
  } while (!line.equals("OK") && !line.equals("ERROR"));
  return line.equals("OK");
}

void ESPConn::pipeSerial() {
  Serial.println("*** PIPING SERIAL TO ESP NOW ***");  
  while (true) {
    if (Serial.available()) {
      esp.write(Serial.read());
    }
    if (esp.available()) {
      Serial.write(esp.read());
    }
  }
}