#ifndef ESPConn_h
#define ESPConn_h

#include <SoftwareSerial.h>

typedef void (* TCPDataReceived)(int linkID, byte data[], int length);

class ESPConn {
  public:
    ESPConn(int rxPin, int txPin);
    bool setupSerial();
    int sendReset();
    bool connectToAP(String ssid, String pass);
    String getIPAddr();
    void pipeSerial();
    bool enableMDNS(String hostname, String service, int port);
    bool listenTCP(int port, TCPDataReceived dataReceivedCallback);
    //bool connectTCP(String host, int port, TCPDataReceived dataReceivedCallback);
    bool openSendCloseTCP(String host, int port, String str);
    bool openSendCloseTCP(String host, int port, byte data[], int length);
    bool setupUDP(String ip, int port);
    bool sendUDP(String str);
    bool sendUDP(byte data[], int length);
  private:
    int _rxPin;
    int _txPin;
    SoftwareSerial esp;
    String readLine();
    bool readUntilLine(String text);
    int readUntilLineOptions(const char* texts[], int length);
    bool readUntilOKorERROR();
    void listenForTCPData(TCPDataReceived dataReceivedCallback);

};

#endif
