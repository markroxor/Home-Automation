/*--------------------------------------------------------------------------------------------------------------------------
  Pin 0 = (Serial RX)
  Pin 1 = (Serial TX)
  Pin 2 =
  Pin 3 = Light 1
  Pin 4 = (Used by Ethernet Shield)
  Pin 5 = Light 2
  Pin 6 = Fan
  Pin 7 = Light 1
  Pin 8 = Light 2
  Pin 9 = Fan
  Pin 10 =(Used by Ethernet Shield)
  Pin 11 =(Used by Ethernet Shield)
  Pin 12 =(Used by Ethernet Shield)
  Pin 13 =(Used by Ethernet Shield)
  
  Pin 14 =(Used by Ethernet Shield)
  Pin 15 =(Used by Ethernet Shield)
  Pin 16 =Vcc
  Pin 17 =DHT11 with pullup
  Pin 18 =
  Pin 19 =GND
---------------------------------------------------------------------------------------------------------------------------*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <DHT11.h>
DHT11 dht11(17);
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0x00, 0x1C, 0xB3, 0x09, 0x85, 0x15 };
IPAddress ip(192, 168, 0, 20); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
int LED_state[6] = {0}; // stores the states of the LEDs
unsigned long previous_millis = 0;        // will store last time LED was updated
unsigned long delay_millis = 2000;           // delay_millis at which to blink (milliseconds)
unsigned long current_millis = 0;
float temp=0 ,humi=0;

void setup()
{
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    pinMode(16,OUTPUT);
    pinMode(19,OUTPUT);
    digitalWrite(16,HIGH);
    digitalWrite(19,LOW);
    
    Serial.begin(115200);       // for debugging
    
    // initialize SD card
    if (!SD.begin(4)) {
        Serial.println("SD init failed!");
        return;    // init failed
    }
    Serial.println("SD init success");
    // check for index.htm file
    
    
    // LEDs
    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
}

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetLEDs();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    
                    else if (StrContains(HTTP_req, "GET / ")) {
                      
                        Serial.println("\nPage Request\n");
                        
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        
                        webFile = SD.open("index.htm");        // open web page file
                        while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                        }
                        webFile.close();
                    }
                                 
                    else if (StrContainsPart(HTTP_req, "GET /e.jpg")) {
                      
                            Serial.println("\n\nBG request\n");
                            
                            client.println("HTTP/1.1 200 OK");
                            client.println("Connection: keep-alive");
                            client.println();
                            
                            webFile = SD.open("e.jpg");
                            while(webFile.available()) {
                            client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                    }
                    
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
    int err;    
    current_millis = millis();
    
    if(current_millis - previous_millis > delay_millis) 
    {
     
            previous_millis = current_millis;             // save the last time you blinked the LED
            
            if((err=dht11.read(humi, temp))!=0)
            {
              Serial.print("DHT11 error :");
              Serial.println(err);
            }
            else
            {
              Serial.print("\t\tTemperature=");
              Serial.println(temp);
              Serial.print("\t\tHumidity=");
              Serial.println(humi);
            }
    }
}

// checks if received HTTP request is switching on/off LEDs
// also saves the state of the LEDs
void SetLEDs(void)
{
    // LED 1 (pin 3)
    if (StrContains(HTTP_req, "LED1=1")) {
        LED_state[0] = 1;  // save LED state
        digitalWrite(3, HIGH);
    }
    else if (StrContains(HTTP_req, "LED1=0")) {
        LED_state[0] = 0;  // save LED state
        digitalWrite(3, LOW);
    }
    // LED 2 (pin 5)
    if (StrContains(HTTP_req, "LED2=1")) {
        LED_state[1] = 1;  // save LED state
        digitalWrite(5, HIGH);
    }
    else if (StrContains(HTTP_req, "LED2=0")) {
        LED_state[1] = 0;  // save LED state
        digitalWrite(5, LOW);
    }
    // LED 3 (pin 6)
    if (StrContains(HTTP_req, "LED3=1")) {
        LED_state[2] = 1;  // save LED state
        analogWrite(6, 51);
    }
    else if (StrContains(HTTP_req, "LED3=2")) {
        LED_state[2] = 2;  // save LED state
        analogWrite(6, 102);
    }
    else if (StrContains(HTTP_req, "LED3=3")) {
        LED_state[2] = 3;  // save LED state
        analogWrite(6, 153);
    }
    else if (StrContains(HTTP_req, "LED3=4")) {
        LED_state[2] = 4;  // save LED state
        analogWrite(6, 204);
    }
    else if (StrContains(HTTP_req, "LED3=5")) {
        LED_state[2] = 5;  // save LED state
        digitalWrite(6, HIGH);
    }
    else if (StrContains(HTTP_req, "LED3=0")) {
        LED_state[2] = 0;  // save LED state
        digitalWrite(6, LOW);
    }
    // LED 4 (pin 7)
    if (StrContains(HTTP_req, "LED4=1")) {
        LED_state[3] = 1;  // save LED state
        digitalWrite(7, HIGH);
    }
    else if (StrContains(HTTP_req, "LED4=0")) {
        LED_state[3] = 0;  // save LED state
        digitalWrite(7, LOW);
    }
    // LED 5 (pin 8)
    if (StrContains(HTTP_req, "LED5=1")) {
        LED_state[4] = 1;  // save LED state
        digitalWrite(8, HIGH);
    }
    else if (StrContains(HTTP_req, "LED5=0")) {
        LED_state[4] = 0;  // save LED state
        digitalWrite(8, LOW);
    }
    // LED 6 (pin 9)
    if (StrContains(HTTP_req, "LED6=1")) {
        LED_state[5] = 1;  // save LED state
        analogWrite(9, 51);
    }
    else if (StrContains(HTTP_req, "LED6=2")) {
        LED_state[5] = 2;  // save LED state
        analogWrite(9, 102);
    }
    else if (StrContains(HTTP_req, "LED6=3")) {
        LED_state[5] = 3;  // save LED state
        analogWrite(9, 153);
    }
    else if (StrContains(HTTP_req, "LED6=4")) {
        LED_state[5] = 4;  // save LED state
        analogWrite(9, 204);
    }
    else if (StrContains(HTTP_req, "LED6=5")) {
        LED_state[5] = 5;  // save LED state
        digitalWrite(9, HIGH);
    }
    else if (StrContains(HTTP_req, "LED6=0")) {
        LED_state[5] = 0;  // save LED state
        digitalWrite(9, LOW);
    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{    
    cl.print("<?xml version = \"1.0\" ?>");
    
    cl.print("<W>");
    
    cl.print("<n>");   
    cl.print((int)temp);
    cl.print("</n>");
    
    cl.print("<g>");
    cl.print((int)humi);
    cl.print("</g>");
       
    
    cl.print("<A>");          // LED1
    cl.print(LED_state[0]);
    cl.print("</A>");
    
    cl.print("<B>");          // LED2
    cl.print(LED_state[1]);
    cl.print("</B>");
    
    cl.print("<C>");          // LED4
    cl.print(LED_state[3]);
    cl.print("</C>");
    
    cl.print("<D>");          // LED5
    cl.print(LED_state[4]);
    cl.print("</D>");
    
    cl.print("<E>");          // LED3
    cl.print(LED_state[2]);
    cl.print("</E>");
    
    
    cl.print("<F>");          // LED6
    cl.print(LED_state[5]);
    cl.print("</F>");
   
     
    cl.print("</W>");
    /*
    */
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
char StrContainsPart(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
