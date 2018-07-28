#ifndef ESPConn_h
#define ESPConn_h

#include <SoftwareSerial.h>

typedef void (* TCPDataReceived)(int linkID, byte data[], int length);

class ESPConn {
  public:
    ESPConn(int rxPin, int txPin);
    ESPConn(int rxPin, int txPin, bool debug);
    bool setupSerial();
    int sendReset();
    bool connectToAP(String ssid, String pass);
    String getIPAddr();
    void pipeSerial();
    bool enableMDS(String hostname, String service, int port);
    bool listenTCP(int port, TCPDataReceived dataReceivedCallback);

  private:
    int _rxPin;
    int _txPin;
    bool _debug;
    SoftwareSerial esp;
    String readLine();
    bool readUntilLine(String text);
    int readUntilLineOptions(const char* texts[], int length);
    bool readUntilOKorERROR();
    void listenForTCPData(TCPDataReceived dataReceivedCallback);

};

#endif
