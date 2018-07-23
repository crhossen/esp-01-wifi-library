#ifndef ESPConn_h
#define ESPConn_h

#include <SoftwareSerial.h>

class ESPConn {
  public:
    ESPConn(int rxPin, int txPin);
    bool setupSerial();
    bool sendReset();
    bool connectToAP(String ssid, String pass);
  
  private:
    int _rxPin;
    int _txPin;
    SoftwareSerial esp;
    String readLine();
    bool readUntilLine(String text, bool debug);
    bool readUntilOKorERROR(bool debug);

};

#endif
