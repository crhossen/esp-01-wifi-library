#ifndef ESPConn_h
#define ESPConn_h

#include <SoftwareSerial.h>

class ESPConn {
  public:
    ESPConn(int rxPin, int txPin);
    ESPConn(int rxPin, int txPin, bool debug);
    bool setupSerial();
    int sendReset();
    bool connectToAP(String ssid, String pass);
    String getIPAddr();
    void pipeSerial();
  
  private:
    int _rxPin;
    int _txPin;
    bool _debug;
    SoftwareSerial esp;
    String readLine();
    bool readUntilLine(String text);
    int readUntilLineOptions(const char* texts[], int length);
    bool readUntilOKorERROR();
    

};

#endif
