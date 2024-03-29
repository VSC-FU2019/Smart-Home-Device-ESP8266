/*************************************************** 
  NodeMCU 1.0
****************************************************/ 
#include <FS.h>                                                 //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>                                        // ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>                                          //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>                                   //Local WebServer used to serve the configuration portal
#include <PubSubClient.h>               
#include <stdlib.h>
#include <ArduinoJson.h>                                        //https://github.com/bblanchon/ArduinoJson

#include "WiFiManager.h" 

/************************* SET PIN DEVICE *********************************/ 
// -- CAN'T USE PIN 9 FOR OUTPUT - TRIGGER WDT TIMER                               
#define DEVICE_PIN               5                                // pin connected to the FAN. GPIO5 (D1)
#define LIGHT_PIN             4                                // pin connected to the LIGHT. GPIO4 (D2)
//#define AIR_PIN               14                               // pin connected to the AIR CONDITIONER. GPIO14 (D5)
//#define TIVI_PIN              16                               // pin connected to the TELEVISION. GPI016 (D0)
//#define DOOR_PIN              2                                // pin connected to the DOOR. GPIO02 (D4)

#define BUTTON_PIN            0                                // pin connected to the BUTTON. GPIO0 (D3)
#define LED_SUCCESS_PIN       12                               // 
#define LED_FAIL_PIN          13                               // LED SIGNAL

/************************* GLOBAL STATE *********************************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server. 
WiFiClient        client; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
PubSubClient pubsub_client(client);

//define your default values here, if there are different values in config.json, they are overwritten.
char              mqtt_server[40];
char              mqtt_port_str[6];
char              device_topic[20];

/****************************** SKETCH CODE ************************************/ 
WiFiManager       wifiManager;
bool              flag = false;
void              wifi_reset();

ICACHE_RAM_ATTR void handleInterrupt() {
    wifi_reset();
}

void setup() {
    Serial.begin(115200);
    //clean FS, for testing
    //SPIFFS.format();

    //read configuration from FS json
    Serial.println("mounting FS...");
    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
    } else {
        Serial.println("failed to mount FS");
    }
    Serial.println(device_topic);
    //end read
    delay(10);
    // pinMode
    pinMode(LIGHT_PIN, OUTPUT);  
    pinMode(DEVICE_PIN, OUTPUT);
    
    pinMode(LED_SUCCESS_PIN, OUTPUT); 
    pinMode(LED_FAIL_PIN, OUTPUT); 
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // setup default pin
    digitalWrite(LIGHT_PIN, !LOW);  
    digitalWrite(DEVICE_PIN, !LOW);
    
    digitalWrite(LED_SUCCESS_PIN, !LOW);  
    digitalWrite(LED_FAIL_PIN, !HIGH);

    // attachInterrupt
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterrupt, FALLING);

    if (flag) {
        flag = false;
    }

    //Serial.println(WiFi.SSID());
    if (!wifiManager.autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
    }

    read_config_file();

    // Connect to WiFi access point. 
    while (WiFi.status() != WL_CONNECTED) { 
        Serial.println("connect...");
        digitalWrite(LED_SUCCESS_PIN, !HIGH);  
        digitalWrite(LED_FAIL_PIN, !HIGH);  
        delay(500); 
        digitalWrite(LED_SUCCESS_PIN, !LOW);  
        digitalWrite(LED_FAIL_PIN,  !LOW); 
        Serial.println("trying to connect to wifi access point");
        delay(500);
    } 
    Serial.println("successfully connect to wifi access point");
    digitalWrite(LED_FAIL_PIN, !LOW); 
    digitalWrite(LED_SUCCESS_PIN, !HIGH);  

    uint16_t  mqtt_port;
    mqtt_port = strtol(mqtt_port_str, NULL, 0);
    pubsub_client.setServer(mqtt_server, mqtt_port);
    // Callback function to process publish / subscribe
    pubsub_client.setCallback(pubsubclient_callback);
    // call this function to reconnect to mqtt server when lost connection
    pubsubclient_reconnect();

    esp_check_reset();
}

void read_config_file() {
    if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, buf.get());
                if (error) {
                    Serial.println("failed to load json config");
                    Serial.println(error.c_str());
                } else {
                    Serial.println("\nparsed json");
                    serializeJson(doc, Serial);
                    strcpy(mqtt_server, doc["mqtt_server"]);
                    strcpy(device_topic, doc["device_topic"]);
                    strcpy(mqtt_port_str, doc["mqtt_port"]);
                }
                configFile.close();
            }
        }
}

void pubsubclient_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Topic: ");
    Serial.println(topic);
    for (int i=0; i < length; i++) {
        Serial.print((char)payload[i]);  
    }
    Serial.println();
    if (strncmp(topic, "light", 5) == 0) {
        if((char)payload[0] == 'o' && (char)payload[1] == 'n') {
            digitalWrite(LIGHT_PIN, !HIGH); 
        } else if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {
            digitalWrite(LIGHT_PIN, !LOW);
        }
    } else if (strncmp(topic, device_topic, strlen(device_topic)) == 0) {
        Serial.println("device topic found");
        if((char)payload[0] == 'o' && (char)payload[1] == 'n') {
            Serial.println("on");
            digitalWrite(DEVICE_PIN, !HIGH); 
        } else if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {
          Serial.println("off");
            digitalWrite(DEVICE_PIN, !LOW); 
        }
    }
}

void pubsubclient_reconnect() {
    // loop until reconnected
    if(!pubsub_client.connected()) {
        Serial.println("Attempting MQTT connection ...");
        if(pubsub_client.connect("ESP8266")) {
            Serial.println("connected MQTT");
            Serial.println(device_topic);
            pubsub_client.subscribe("light");
            pubsub_client.subscribe(device_topic);
        } else {
          // serial print status of client when not connect to MQTT broker
          Serial.print("failed, rc=");
          Serial.print(pubsub_client.state());
          Serial.println(" try again in 5 seconds");
          uint8_t number_try = 5;
          while(number_try -- > 0){
              digitalWrite(LED_SUCCESS_PIN, !HIGH);  
              delay(500);  
              digitalWrite(LED_SUCCESS_PIN, !LOW); 
              delay(500);
          }
      }
    }
}

void esp_check_reset() {
  if (flag) {
        Serial.println("wifi reset");
        flag = false;
      //WiFi.persistent(false);
//      WiFi.disconnect(true);
        wifiManager.resetSettings();
        delay(1000);
        ESP.reset();
    }
}

void loop() {
    esp_check_reset();
    if(!pubsub_client.connected()) {
        pubsubclient_reconnect();
    } else {
        digitalWrite(LED_SUCCESS_PIN, !HIGH);
    }
    pubsub_client.loop();
} 

void wifi_reset(){
    flag = true;
}
