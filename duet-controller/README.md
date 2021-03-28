# Duet Controller

Controlling a Duet 2 WiFi board with an ESP 32 via HTTP calls.

credentials.h must look like this:

```
const char* WIFI_SSID = "my-ssid";
const char* WIFI_PASSWD = "my-password"; 

String DUET_URL = "http://my.duet.printer";
String DUET_PASSWD = "duet-password";
```
