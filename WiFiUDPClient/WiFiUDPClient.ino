/*
 *  This sketch sends random data over UDP on a ESP32 device
 *
 */
#include <WiFi.h>
#include <WiFiUdp.h>

// WiFi network name and password:
const char * networkName = "K7MDL";
const char * networkPswd = "Artemis2!a";

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
const char * udpAddress = "192.168.2.19";
const int udpPort = 50002;
const int udpPort_Control = 50001;
const char login_str[] = "b'\x22\xFE\xAC\xE0\x03\xFD'";

const char connect_msg[] = "b'\x10\x00\x00\x00\x03\x00\x00\x00\x52\xE8\x00\x00\x00\x00\x00\x00'";

const char msg[] = "b'\xFE\xFE\xAC\xE0\x03\xFD'";
   

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;

void setup(){
    // Initilize hardware serial:
    Serial.begin(115200);
    
    //Connect to the WiFi network
    connectToWiFi(networkName, networkPswd);

    if(connected){
      //Send a packet to log in
      udp.beginPacket(udpAddress,udpPort_Control);
      //char username[] = { "K7MDL" };
      //char userpwd[] = { "k7mdl123" };
      //udp.printf("%s",username);    
      //udp.printf("%s",userpwd);    
      //udp.write(b'\xFE\xFE\xAC\xE0\x03\xFD');
      udp.print(connect_msg);
      udp.endPacket();
      Serial.printf("Login Done");
    }
}

void loop(){
  //only send data when connected
  if(connected){
    //Send a packet
    udp.beginPacket(udpAddress,udpPort);
    //udp.print("FEFEACE005008049440100FD");
    char data[] = { 0xFE, 0xFE, 0xAC, 0xE0, 0x03, 0xFD, 0x00 };

    udp.printf("%s",data);    

    //udp.write(b'\xFE\xFE\xAC\xE0\x03\xFD');
    udp.endPacket();
    Serial.printf("Seconds since boot: %lu\n", millis()/1000);
    enet_read();
  }
  //Wait for 1 second
  delay(1000);
}


uint8_t enet_read(void)
{
  //  experiment with this -->   udp.listen( true );           // and wait for incoming message

    if (!connected)   // skip if no enet connection
         return 0;
     
    uint8_t rx_count = 0;
    uint8_t Serial_USB_BUFFER_SIZE = 254;
    int count = 0; 
    uint8_t pSdata1[254];
    uint8_t *pSdata2 = pSdata1;
    uint8_t rx_buffer[254];
    // if there's data available, read a packet
    count = udp.parsePacket();      
    rx_buffer[0] = _NULL;
    if (count > 0)
    {
        udp.read(rx_buffer, Serial_USB_BUFFER_SIZE);
        rx_buffer[count] = '\0';
        rx_count = count;          
        Serial.println(rx_count);
        Serial.println((char *) rx_buffer);
        
        // initially p1 = p2.  parser will move p1 up to p2 and when they are equal, buffer is empty, parser will reset p1 and p2 back to start of sData         
        memcpy(pSdata2, rx_buffer, rx_count+1);   // append the new buffer data to current end marked by pointer 2        
        pSdata2 += rx_count;                      // Update the end pointer position. The function processing chars will update the p1 and p2 pointer             
        rx_count = pSdata2 - pSdata1;             // update count for total unread chars. 
        Serial.println(rx_count);  
    }
    rx_buffer[0] = '\0';
    return rx_count;
}
void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          udp.begin(WiFi.localIP(),udpPort);
          connected = true;
          break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}
