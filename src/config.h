#define SSID "Delegate"
#define PASS ""

#define CLIENTID "wemos"
#define IOTNAME "delegate"
#define IOTHOSTNAME IOTNAME ".azure-devices.net"
#define IOTPORT 8883
#define USERNAME IOTHOSTNAME "/" CLIENTID
#define SAS "SharedAccessSignature sr=&sig=&se="

#define TOPICSUB "devices/" CLIENTID "/messages/devicebound/#"
#define TOPICPUB "devices/" CLIENTID "/messages/events/"


