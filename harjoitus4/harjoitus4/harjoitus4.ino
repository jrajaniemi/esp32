/***********
 * Original from Asksensors
 * https://github.com/asksensors/AskSensors-ESP32-API/blob/master/http_get.ino
 * Modifield by Jussi Rajaniemi 3.11.2019
 */
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include "secrets.h" // get SSID and Password from secrets.h file which is ignored to github

// MultiWifi object
WiFiMulti WiFiMulti;

// Global HTTP-client object and httpsensors.com settings
HTTPClient http;
const char *host = "api.asksensors.com"; 
const int httpPort = 80; 

// Global variables of sensors
int humidity = 0;
float temperature = 0;
int pressure = 0;
int light = 0;

// ISR volatile state variable, last state and counter
volatile int state;
int lastState = 0;
int stateCounter = 0;
int counter = 0;

// Pin variables
int ledPin = 2;
int photoresistorPin = 36;
int pirPin = 27;

// Create BME280 object 
Adafruit_BME280 bme; 

// send sensor 1 data method to Asksensor.com
void sendSensor1Values() {
  // Create a URL for the request
  String url = "https://api.asksensors.com/write/";
  url += apiKeyIn1;
  url += "?module1=";
  url += humidity;
  url += "&module2=";
  url += temperature;
  url += "&module3=";
  url += pressure;
  url += "&module4=";
  url += light;

  // Open connect to specific URL
  http.begin(url); 

  // Send GET-request and save HTTP-response code
  int httpCode = http.GET();

  // if code on OK, get payload and print them to Serial-port
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Get " + httpCode);
    Serial.println("Return " + payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  // Close connection 
  http.end();
}

void sendSensor2Values(int i) {
  Serial.println("Motion detected");

  // Create Wifi client object
  WiFiClient client;

  // Check Asksernsor connection
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed (ISR)");
  } else {
    // Create a URL for the request
    String url = "https://api.asksensors.com/write/";
    url += apiKeyIn2;
    url += "?module3=";
    url +=  i;
    url += "&module4=";
    url +=  i;

    // Open connect to specific URL
    http.begin(url); //Specify the URL
  
    // Send GET-request and save HTTP-response code
    int httpCode = http.GET();

    // if code on OK, get payload and print them to Serial-port
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
    }
    // Close HTTP-client connection 
    http.end(); 
  }
  // Close Wifi-client connection
  client.stop();
  // HTTP-request excecution time is about 1 second, so increase counter value
  counter++;
}


void IRAM_ATTR ISRcallback() {
  // Save PIR-sensor value to state variable
  state = digitalRead(pirPin);
}

void setup() {
  // Open serial connect to speed 115200
  Serial.begin(115200);

  // Set pin-modes
  pinMode(ledPin, OUTPUT);
  pinMode(photoresistorPin, INPUT);
  pinMode(pirPin, INPUT);

  // Create interrupt service routine for PIR polling
  attachInterrupt(digitalPinToInterrupt(pirPin), ISRcallback, RISING);


  // Setting BME280-sensor for reading
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }

  // Connect to Wifi access point (2 choices)
  Serial.print("Connecting to ");
  // connecting to the WiFi network
  WiFiMulti.addAP(ssid1, password1);
  WiFiMulti.addAP(ssid2, password2);
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  // Show IP-address of client
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  // ISR status polling speed is 1 sec and weather sensors send data to network 50 times per day
  // so delay must been 1 hour 2 min 8 second. (3600 * 24) / 50 = 1728 sec.
  if(counter >= 1728) {
    // Calculate the lux from photoresistor 5516 
    int ADC = analogRead(photoresistorPin);       // read analog value from pin36
    float Vout = (ADC * 0.0008056640625);         // calculate the voltage
    light = 500 / (10.0 * (3.3 - Vout) / Vout);   // calculate the lux value
  
    // Read temperature value to variable
    temperature = bme.readTemperature();
  
    // Read humidity value to variable
    humidity = bme.readHumidity();
  
    // Read pressure value to variable
    pressure = bme.readPressure() / 100.0F;
  
    // Create Wifi-client object
    WiFiClient client;

    // Check Asksernsor connection
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    } else {
      // Send weather data to network
      Serial.println("Send sensor 1 values");
      sendSensor1Values();
    }

    // Close connection
    client.stop();

    // Set counter to 1, because HTTP-request excecution time is about 1 second
    counter = 1;    
  }

  /* Debuging
  Serial.print(state);
  Serial.print(":");
  Serial.println(lastState);
  */

  // If PIR-state cheange, send data to network
  if(state != lastState) {
    sendSensor2Values(state);
    lastState = state;
  } 

  // Delay and increase counter value
  delay(1000);
  counter++;
  
}
