#include "Arduino.h"
#include <SoftwareSerial.h>
#include "ESPConn.h"

ESPConn::ESPConn(int rxPin, int txPin) :
  esp(SoftwareSerial(rxPin, txPin))
{
  _rxPin = rxPin;
  _txPin = txPin;
}

bool ESPConn::setupSerial() {
  // Setup ESP connection
  pinMode(_rxPin, INPUT);
  pinMode(_txPin, OUTPUT);
  esp.begin(9600);
  delay(1000);
  
  return true;
}

bool ESPConn::sendReset() {
  // Send AT command
  esp.println("AT+RST");
  readUntilLine("ready", true);
  readUntilLine("WIFI DISCONNECT", true);
  return true;
}

String ESPConn::readLine() {
  String str = esp.readStringUntil('\r');
  esp.readStringUntil('\n');
  return str;
}

bool ESPConn::connectToAP(String ssid, String pass) {
  esp.println("AT+CWMODE_CUR=3"); //station mode
  readUntilOKorERROR(true);
  esp.print("AT+CWJAP=");
  esp.print(ssid); esp.print(','); esp.println(pass);
  return readUntilOKorERROR(true);
}

bool ESPConn::readUntilLine(String text, bool debug) {
  String line;
  do {
    line = readLine();
    if (debug) Serial.println(line);
  } while (!line.equals(text));
  return true;
}

bool ESPConn::readUntilOKorERROR(bool debug) {
  String line;
  do {
    line = readLine();
    if (debug) Serial.println(line);
  } while (!line.equals("OK") && !line.equals("ERROR"));
  return line.equals("OK");
}

