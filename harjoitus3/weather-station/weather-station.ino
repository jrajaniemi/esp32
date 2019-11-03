/*********
 * Original from Rui Santos
 * Complete project details at https://randomnerdtutorials.com  
 * Modifield by Jussi Rajaniemi 3.11.2019
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include "secrets.h" // get SSID and Password from secrets.h file

// Create bme280 object
Adafruit_BME280 bme;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Pin variables
int ledPin = 2;
int photoresistorPin = 36;
int pirPin = 27;

void setup()
{
  // Setup serial connection to 115200 speed
  Serial.begin(115200);

  // Setting pin-modes
  pinMode(ledPin, OUTPUT);
  pinMode(photoresistorPin, INPUT);
  pinMode(pirPin, INPUT);

  // setup bme280 to address 0x76 
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start HTTP-server
  server.begin();
}

void loop()
{
  // create Wifi-client and set it to listen incoming clients
  WiFiClient client = server.available();

  // If new client connect!
  if (client)
  { 
    // show a light when new client found
    digitalWrite(ledPin, LOW);
    delay(150);
    digitalWrite(ledPin, HIGH);

    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    
    // While client is connected
    while (client.connected()) { 
      if (client.available()) {                         
        
        // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n') { 
          // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Calculate the lux from photoresistor 5516
            int ADC = analogRead(photoresistorPin);       // read analog value from pin36
            float Vout = (ADC * 0.0008056640625);         // calculate the voltage
            int lux = 500 / (10.0 * (3.3 - Vout) / Vout); // calculate the lux value

            // Check PIR Sensor state. 1 = true, 0 = false
            int pirState = digitalRead(pirPin);
            
            // Display the HTML finnish web page with sensor values
            client.println("<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\" /><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" /><meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\" /><link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\"/>");
            client.println("<title>ESP Weather station</title></head><body><div class=\"container\"><div class=\"row\"><div class=\"col-12 text-center\"><h1 class=\"display-4\">ESP Mittaukset</h1></div></div>");
            client.println("<div class=\"row mx-auto\" style=\"max-width:350px;\"><div class=\"col-6\">Lämpötila</div><div class=\"col-6\">");
            client.println(bme.readTemperature());
            client.println(" <sup>&deg;</sup>C</div>");
            client.println("<div class=\"col-6\">Kosteus</div><div class=\"col-6\">");
            client.println(bme.readHumidity());
            client.println(" %</div><div class=\"col-6\">Paine</div><div class=\"col-6\">");
            client.println(bme.readPressure() / 100.0F);
            client.println(" mbar</div>");
            client.println("<div class=\"col-6\">Valoisuus</div><div class=\"col-6\">");
            client.println(lux);
            client.println(" lux</div>");
            client.println("<div class=\"col-6\">Paikalla?</div><div class=\"col-6\">");
            if (pirState)
              client.println("KYLLÄ");
            else
              client.println("EI");
            client.println("</div></div>");
            client.println("<div class=\"row mt-5\"><div class=\"col-12 text-center\"><a class=\"btn btn-primary btn-lg\" href=\"http://");
            client.println(WiFi.localIP());
            client.println("\">UPDATE</a></div></div></div></body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
