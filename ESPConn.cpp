#include "Arduino.h"
#include <SoftwareSerial.h>
#include "ESPConn.h"

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_WRITE_BYTES(x, y) Serial.write(x, y)
  #define DEBUG_WRITE(x) Serial.write(x)
#else
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_WRITE_BYTES(x, y)
  #define DEBUG_WRITE(x)
#endif

const char* READY_TERMINATORS[] = { "WIFI DISCONNECT", "WIFI CONNECTED" };
const char* SEND_TERMINATORS[] = { "SEND OK", "SEND FAIL", "ERROR" };

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

int ESPConn::sendReset() {
  // Send AT command
  esp.println("AT+RST");
  readUntilLine("ready");
  int resetStatus = readUntilLineOptions(READY_TERMINATORS, 2);
  if (resetStatus == 0) {
    DEBUG_PRINTLN("ESPConn - Looks not connected.");
    return 0;
  }
  // if it is connected wait for IP
  else if (resetStatus == 1) {
    DEBUG_PRINTLN("ESPConn - Looks already connected.");
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
  DEBUG_PRINT(ip);
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
      DEBUG_PRINT(dumpData);
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
    DEBUG_PRINTLN(line);
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
    DEBUG_PRINTLN(line);
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
    DEBUG_PRINTLN(line);
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
  DEBUG_PRINTLN("Waiting for TCP connections");
  while (true) {
    while (!esp.available()) {}

    // check if connection status message.
    if (esp.peek() != '+') {
      String str = readLine();
      if (str.endsWith("CONNECT")) {
        DEBUG_PRINTLN(str);
        DEBUG_PRINTLN("NEW CONN");
      } else if (str.endsWith("CLOSED")) {
        DEBUG_PRINTLN(str);
        DEBUG_PRINTLN("CONN CLOSED");
      } else if (str.equals("")) {
        // eat a carriage return if it exists
        if (esp.peek() == '\n') {
          esp.read();
        }
      } else {
        DEBUG_PRINT("DUNNO:"); DEBUG_PRINTLN(str);
      }
    } else {
      DEBUG_PRINTLN("looks like we have new data");
      String str;
      if (esp.peek() == '+') {
        esp.read();

        int linkID, length;
        str = esp.readStringUntil(',');
        DEBUG_PRINT(str);
        if (str.equals("IPD")) {
          str = esp.readStringUntil(',');
          DEBUG_PRINT(str);
          linkID = str.toInt();
          str = esp.readStringUntil(':');
          DEBUG_PRINT(str);
          length = str.toInt();
          if (esp.peek() == ':') { esp.read(); }

          byte data[length];
          int readBytes = esp.readBytes(data, length);
          DEBUG_PRINT("Actually read ");
          DEBUG_PRINTLN(readBytes);
          DEBUG_PRINT(" content: ");
          DEBUG_WRITE_BYTES(data, readBytes);
          DEBUG_PRINTLN();
          dataReceivedCallback(linkID, data, length);
        }
      }
    }
  }
}

bool ESPConn::openSendCloseTCP(String host, int port, String str) {
  // include room for null terminator
  byte data[str.length() + 1];
  str.getBytes(data, str.length() + 1);
  return openSendCloseTCP(host, port, data, str.length());
}

bool ESPConn::openSendCloseTCP(String host, int port, byte data[], int length) {
  esp.print("AT+CIPSTART=\"TCP\",\"");
  esp.print(host); esp.print("\","); esp.println(4321);
  if (!readUntilOKorERROR()) {
    return false;
  }

  DEBUG_PRINT("Sending: ");
  DEBUG_WRITE_BYTES(data, length); DEBUG_PRINTLN();

  esp.print("AT+CIPSEND="); esp.println(length);
  if (!readUntilOKorERROR()) {
    return false;
  }
  esp.readStringUntil('>');
  esp.write(data, length);
  int sendResult = readUntilLineOptions(SEND_TERMINATORS, 3);

  esp.println("AT+CIPCLOSE");
  readUntilOKorERROR();

  return (sendResult == 1);
}
