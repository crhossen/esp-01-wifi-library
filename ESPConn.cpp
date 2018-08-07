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

bool ESPConn::enableMDNS(String hostname, String service, int port) {
  esp.print("AT+MDNS=1,\"");
  esp.print(hostname);
  esp.print("\",\"");
  esp.print(service);
  esp.print("\",");
  esp.println(port);
  return readUntilOKorERROR();
}

bool ESPConn::listenTCP(int port, TCPDataReceived dataReceivedCallback) {
  esp.println("AT+CIPMUX=1");
  if (readUntilOKorERROR()) {
    esp.print("AT+CIPSERVER=1,"); esp.println(port);
    readUntilOKorERROR();
    if (esp.available()) {
      char dumpData[esp.available()];
      esp.readBytes(dumpData, esp.available());
      if (_debug) Serial.print(dumpData);
    }
    listenForTCPData(dataReceivedCallback);
    pipeSerial();
  } else {
    return false;
  }
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

void ESPConn::listenForTCPData(TCPDataReceived dataReceivedCallback) {
  //pipeSerial();
  if (_debug) Serial.println("Waiting for TCP connections");
  while (true) {
    while (!esp.available()) {}

    // check if connection status message.
    if (esp.peek() != '+') {
      String str = readLine();
      if (str.endsWith("CONNECT")) {
        if (_debug) {
          Serial.println(str);
          Serial.println("NEW CONN");
        }
      } else if (str.endsWith("CLOSED")) {
        if (_debug) {
          Serial.println(str);
          Serial.println("CONN CLOSED");
        }
      } else if (str.equals("")) {
        // eat a carriage return if it exists
        if (esp.peek() == '\n') {
          esp.read();
        }
      } else {
        if (_debug) {
          Serial.print("DUNNO:"); Serial.println(str);
        }
      }
    } else {
      Serial.println("looks like we have new data");
      String str;
      Serial.write(esp.read());
      int linkID, length;
      if (_debug) Serial.println(str);
      str = esp.readStringUntil(',');
      Serial.print(str);
      if (str.equals("IPD")) {
        str = esp.readStringUntil(',');
        Serial.print(str);
        linkID = str.toInt();
        str = esp.readStringUntil(':');
        Serial.print(str);
        length = str.toInt();
        if (esp.peek() == ':') { esp.read(); }

        byte data[length];
        int readBytes = esp.readBytes(data, length);
        Serial.print("Actually read ");
        Serial.println(readBytes);
        Serial.print(" content: ");
        Serial.write(data, readBytes);
        Serial.println();
        dataReceivedCallback(linkID, data, length);
      }
    }
  }
}

bool ESPConn::openSendCloseTCP(String host, int port, String str) {
  byte data[str.length()];
  str.getBytes(data, str.length());
  openSendCloseTCP(host, port, data, str.length());

  return true;
}

bool ESPConn::openSendCloseTCP(String host, int port, byte data[], int length) {
  pipeSerial();

  return true;
}
