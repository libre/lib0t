/*The MIT License (MIT)

Copyright (c) 2019 by Saïd Deraoui, http://github.com/libre/libdot/
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://github.com/libre/libdot/
*/

#include <ArduinoJson.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include "DHT.h"
#include "images.h"
#include <NTPClient.h>


SSD1306Wire display(0x3c, 0, 2);   								// ADDRESS, SDA, SCL 
WiFiUDP ntpUDP;													// Define NTP Client for get Time by wifi				
NTPClient timeClient(ntpUDP, "time.nist.gov", 0, 900000); 		// Define NTP Client for get Time UDP Packets	
RCSwitch mySwitch = RCSwitch();								// Define RCSwitch Lib for manage RF 433Mhz. 

// Variables to save date and time
//
String formattedDate;								
String dayStamp;
String timeStamp;
String formattedDateH;
String formattedDateM;
String formattedDateS;
String EpochDate;
// Led connexion status
Ticker ticker;

// Builtin ESP For Connected or not connected. 
//
#ifndef LED_BUILTIN
#define LED_BUILTIN 13 								// ESP32 DOES NOT DEFINE LED_BUILTIN
#endif
int LED = LED_BUILTIN;

// Pin Erase Config wifi Switch 
//
#define TRIGGER_PIN 14 								// Define Pin Reste Trigger

// Pin and Sensor DHT Define
//
#define DHTPIN 13 									// Define Pin DHT
#define DHTTYPE DHT21   							// DHT 22  (AM2302), AM2321, DHT21 for AM2301
DHT dht(DHTPIN, DHTTYPE); 							// Initialize the temperature/ humidity sensor

// Wireless 433 Config 
//
const char rf_mode = 24; 							// Initial mode 24 bits for send code 433Mhz
const char rf_pin = 4; 								// Define Pin Transmeter 433Mhz

// Define Time slideshow. 
// 
#define DEMO_DURATION 10000
typedef void (*Demo)(void);
int demoMode = 0;
int counter = 1;
int localHum = 0;
int localTemp = 0;

// Array of arguments GET URI HTTP
//
char *led[] = {
// String Request Chacon, Otio, Volet Lucie
  "/CHACON/A1/ON", "/CHACON/A1/OFF", "/CHACON/A2/ON", "/CHACON/A2/OFF", "/CHACON/A3/ON", "/CHACON/A3/OFF", "/CHACON/A4/ON", "/CHACON/A4/OFF", "/CHACON/B1/ON", "/CHACON/B1/OFF", "/CHACON/B2/ON", "/CHACON/B2/OFF", "/CHACON/B3/ON", "/CHACON/B3/OFF", "/CHACON/B4/ON", "/CHACON/B4/OFF", "/CHACON/C1/ON", "/CHACON/C1/OFF", "/CHACON/C2/ON", "/CHACON/C2/OFF", "/CHACON/C3/ON", "/CHACON/C3/OFF", "/CHACON/C4/ON", "/CHACON/C4/OFF", "/CHACON/D1/ON", "/CHACON/D1/OFF", "/CHACON/D2/ON", "/CHACON/D2/OFF", "/CHACON/D3/ON", "/CHACON/D3/OFF", "/CHACON/D4/ON", "/CHACON/D4/OFF", "/OTIO/A1/ON", "/OTIO/A1/OFF", "/OTIO/A2/ON", "/OTIO/A2/OFF", "/OTIO/A3/ON", "/OTIO/A3/OFF", "/OTIO/A4/ON", "/OTIO/A4/OFF", "/OTIO/B1/ON", "/OTIO/B1/OFF", "/OTIO/B2/ON", "/OTIO/B2/OFF", "/OTIO/B3/ON", "/OTIO/B3/OFF", "/OTIO/B4/ON", "/OTIO/B4/OFF", "/OTIO/C1/ON", "/OTIO/C1/OFF", "/OTIO/C2/ON", "/OTIO/C2/OFF", "/OTIO/C3/ON", "/OTIO/C3/OFF", "/OTIO/C4/ON", "/OTIO/C4/OFF", "/OTIO/D1/ON", "/OTIO/D1/OFF", "/OTIO/D2/ON", "/OTIO/D2/OFF", "/OTIO/D3/ON", "/OTIO/D3/OFF", "/OTIO/D4/ON", "/OTIO/D4/OFF", "/VOLET/LUCIE/UP", "/VOLET/LUCIE/STOP", "/VOLET/LUCIE/DOWN", "/GETINFO"};
// Array of frequency send
// Correspondance URI Array
// led[1] = freq[1] ..... led[3] = freq[3]
// 
int freq[] = {
// Chacon 433Mhz Otio Luci 
  1381717, 1381716, 1394005, 1394004, 1397077, 1397076, 1397845, 1397844, 4527445, 4527444, 4539733, 4539732, 4542805, 4542804, 4543573, 4543572, 5313877, 5313876, 5326165, 5326164, 5329237, 5329236, 5330005, 5330004, 5510485, 5510484, 5522773, 5522772, 5525845, 5525844, 5526613, 5526612, 919844896, 1054062624, 248756256, 30652448, 651409440, 785627168, 382973984, 517191712, 970176544, 903067680, 97761312, 231979040, 701741088, 634632224, 433305632, 366196768, 819181600, 953399328, 148092960, 80984096, 550746144, 684963872, 282310688, 416528416, 1020508192, 852736032, 47429664, 181647392, 752072736, 584300576, 483637280, 315865120, 4232320, 4232322, 4232324, 4299999
};

// Serveurs Web 
// Service Mode. (for json)
//
WiFiServer server(8181);

// Global Wifi Parms
// Get parm wifi and Custom field. 
//
WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

// Function usual Led Connected or not. 
//
void tick() {
  //toggle state
  digitalWrite(LED, !digitalRead(LED));     // set pin to the opposite state
}

//Function gets called when WiFiManager enters configuration mode
//
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// Function Save parms for AP Mode.
// 
void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}

// Reste Config Wifi Button 
// Erase Config and Init AP Setup.
//
void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
   if (!wm.autoConnect()) {
     Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    // ESP.restart();
     //  add a custom input field
      int customFieldLength = 40;
      new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
      //test custom html input type(checkbox)
      new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
      //test custom html(radio)
      // const char* custom_radio_str = "<br/><label for='customfieldid'>Active 433Hz</label><input type='radio' name='customfieldid' value='1' checked> Chacon<br><input type='radio' name='customfieldid' value='2'> Otio<br><input type='radio' name='customfieldid' value='3'> Rotary Switch";
      // new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
      wm.addParameter(&custom_field);
      wm.setSaveParamsCallback(saveParamCallback);
      // custom menu via array or vector
   //      menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
      const char* menu[] = {"wifi","info","param","sep","restart","exit"}; 
      wm.setMenu(menu,6);
      // std::vector<const char *> menu = {"wifi","wifinoscan","info","param","sep","restart","exit"};
      // wm.setMenu(menu);
      // set dark theme
      wm.setClass("invert");
      // set static ip
      wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
      wm.setShowStaticFields(false); // force show static ip fields
      wm.setShowDnsFields(false);    // force show dns field always
      wm.setConnectTimeout(20); // how long to try to connect for before continuing
      // wm.setConfigPortalTimeout(3600); // auto close configportal after n seconds
      // wm.setCaptivePortalEnable(false); // disable captive portal redirection
      wm.setAPClientCheck(true); // avoid timeout if client connected to softap 
      //wifi scan settings
      //wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
      wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
      wm.setShowInfoErase(true);      // do not show erase button on info page
      wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
      wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
      bool res;
      // res = wm.autoConnect(); // auto generated AP name from chipid
      res = wm.autoConnect("SETUPME"); // anonymous ap
      // res = wm.autoConnect("AUTOWF_LB","12345678"); // password protected ap
      delay(1000);
   } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}


// Parsing String Getdata 
//
String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
  value = wm.server->arg(name);
  }
  return value;
}

// Function Get Temp./Humidity from DHT
//
void getDHT() {
  float tempIni = localTemp;
  float humIni = localHum;
  localTemp = dht.readTemperature();
  delay(100);
  localHum = dht.readHumidity();
  // localTemp = localTemp + 10;
  if (isnan(localHum) || isnan(localTemp))   // Check if any reads failed and exit early (to try again).
  {
    Serial.println("Failed to read from DHT sensor!");
    localTemp = tempIni;
    localHum = humIni;
    return;
  }
}

// Draw State Info to Oled LCD. 
//
void drawState() {
  // Font Demo1
  String LocalIP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
  // create more fonts at http://oleddisplay.squix.ch/
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0,  "My IP" + String(LocalIP));
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 17, " Wifi: Connected");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 32, "LibD0t v1.5");
}

// Draw State Temp/humidity to Oled LCD. 
//
void drawDHT() {
  int x=0;
  int y=0;
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0 + x, 5 + y, "Hum");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(43 + x, y, "INDOOR");
  display.setFont(ArialMT_Plain_24);
  String hum = String(localHum) + "%";
  display.drawString(0 + x, 15 + y, hum);
  int humWidth = display.getStringWidth(hum);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(95 + x, 5 + y, "Temp");
  display.setFont(ArialMT_Plain_24);
  String temp = String(localTemp) + "°C";
  display.drawString(70 + x, 15 + y, temp);
  int tempWidth = display.getStringWidth(temp);
}

// Draw Time from NTP to Oled LCD. 
//
void drawdigitalClockFrame() {
  while(!timeClient.update()) {
  timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedTime();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  //Serial.print("DATE: ");
  //Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  //Serial.print("HOUR: ");
  //Serial.println(timeStamp); 
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(60, 32, dayStamp);
}



// Function send code to Transmeter 433Mhz.
// 
void rf_send(unsigned long rf_code) {
    mySwitch.send( rf_code, rf_mode);
}

// Setup Global starting device. 
//
void setup() {
  pinMode(TRIGGER_PIN, INPUT);
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  // Initialising the UI will init the display too.
  display.init();
  // clear the display
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);  
  int progress = (counter / 5) % 100;
  // draw the progress bar
  display.drawProgressBar(0, 32, 120, 10, progress);
  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 15, String(progress) + "%");
  delay(10); 
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);  
  display.drawXbm(34, 15, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits); 
  delay(10); 
  // draw the current demo method
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  // wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
   if (!wm.autoConnect()) {
     Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    // ESP.restart();
     //  add a custom input field
      int customFieldLength = 40;
      new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
      //test custom html input type(checkbox)
      new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
      //test custom html(radio)
      // const char* custom_radio_str = "<br/><label for='customfieldid'>Active 433Hz</label><input type='radio' name='customfieldid' value='1' checked> Chacon<br><input type='radio' name='customfieldid' value='2'> Otio<br><input type='radio' name='customfieldid' value='3'> Rotary Switch";
      // new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
      wm.addParameter(&custom_field);
      wm.setSaveParamsCallback(saveParamCallback);
      // custom menu via array or vector
   //      menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
      const char* menu[] = {"wifi","info","param","sep","restart","exit"}; 
      wm.setMenu(menu,6);
      // std::vector<const char *> menu = {"wifi","wifinoscan","info","param","sep","restart","exit"};
      // wm.setMenu(menu);
      // set dark theme
      wm.setClass("invert");
      // set static ip
      wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
      wm.setShowStaticFields(false); // force show static ip fields
      wm.setShowDnsFields(false);    // force show dns field always
      wm.setConnectTimeout(20); // how long to try to connect for before continuing
      // wm.setConfigPortalTimeout(3600); // auto close configportal after n seconds
      // wm.setCaptivePortalEnable(false); // disable captive portal redirection
      wm.setAPClientCheck(true); // avoid timeout if client connected to softap 
      //wifi scan settings
      //wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
      wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
      wm.setShowInfoErase(true);      // do not show erase button on info page
      wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
      wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
      bool res;
      // res = wm.autoConnect(); // auto generated AP name from chipid
      res = wm.autoConnect("SETUPME"); // anonymous ap
      // res = wm.autoConnect("AUTOWF_LB","12345678"); // password protected ap
      delay(1000);
   }
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.printf(" ESP8266 Chip id = %08X\n", ESP.getChipId());
  Serial.printf(" DevKey Request Order is = %08X\n", ESP.getChipId());
  ticker.detach();
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200);
  //keep LED on
  digitalWrite(LED, LOW);
  // Création du serveur
  // Démarrage du serveur
  server.begin();
  // Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(4);
  // Optional set protocol (default is 1, will work for most outlets)
  mySwitch.setProtocol(1);
  // Optional set pulse length.
  mySwitch.setPulseLength(422);
  // Optional set number of transmission repetitions.
  mySwitch.setRepeatTransmit(15);  
  // clear the display
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  while(!timeClient.update()) {
	timeClient.forceUpdate();
  }
  EpochDate = timeClient.getEpochTime();
}

// Init Slide Oled Screen. 
// 
Demo demos[] = {drawdigitalClockFrame, drawState, drawDHT};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

// Init Loop for device. 
// 
void loop() {
	while(!timeClient.update()) {
	  timeClient.forceUpdate();
	}
	// The formattedDate comes with the following format:
	// 2018-05-28T16:00:13Z
	// We need to extract date and time
	formattedDateH = timeClient.getHours();
	formattedDateM = timeClient.getMinutes();
	formattedDateS = timeClient.getSeconds();
	formattedDate = timeClient.getFormattedTime();
	// Extract date
	int splitT = formattedDate.indexOf("T");
	dayStamp = formattedDate.substring(0, splitT);
	//Serial.print("DATE: ");
	//Serial.println(dayStamp);
	// Extract time
	timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
	//Serial.print("HOUR: ");
	//Serial.println(timeStamp);
//	delay(50);	
	getDHT();
	delay(50);
	checkButton();
	// clear the display
	display.clear();
	// draw the current demo method
	demos[demoMode]();
	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(10, 128, String(millis()));
	// write the buffer to the display
	display.display();
	if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
		demoMode = (demoMode + 1)  % demoLength;
		timeSinceLastModeSwitch = millis();
	}
	counter++;
	delay(10);   
	// test si client connecté
    WiFiClient client = server.available();
    if (!client) {
		delay(2);
        return;
    }
  
    // attendre les données
    while(!client.available()){
        delay(2);
    }
  
    // lecture 1ère ligne
    String req = client.readStringUntil('\r');
    client.flush();
   if(req.indexOf("CHACON") > 0) {
		Serial.println("Request Chacon Detected");
		const char rf_mode = 24; 
		// Optional set protocol (default is 1, will work for most outlets)
		mySwitch.setProtocol(1);
		// Optional set pulse length.
		mySwitch.setPulseLength(422);
		// Optional set number of transmission repetitions.
		mySwitch.setRepeatTransmit(15);  	  
    }
   if(req.indexOf("OTIO") > 0) { 
		Serial.println("Request Otio Detected");
		const char rf_mode = 32;
		// Optional set protocol (default is 1, will work for most outlets)
		mySwitch.setProtocol(2);
		// Optional set pulse length.
		mySwitch.setPulseLength(700);
		// Optional set number of transmission repetitions.
		mySwitch.setRepeatTransmit(15);  	  
   } 
   if(req.indexOf("VOLET/LUCIE") > 0) {
		Serial.println("Request Volet Lucie");
		const char rf_mode = 24;
		// Optional set protocol (default is 1, will work for most outlets)
		mySwitch.setProtocol(1);
		// Optional set pulse length.
		mySwitch.setPulseLength(422);
		// Optional set number of transmission repetitions.
		mySwitch.setRepeatTransmit(15); 
   }
   if(req.indexOf("GETINFO") > 0) {
		Serial.println("Request Device Info");
   }   
   for (int i = 0; i < (sizeof(freq) - 1); i++){ //Boucle quiparcours le tableau des commandes
        if(req.indexOf(led[i]) != -1) {
            //rf_send(freq[i]);
            mySwitch.send( freq[i], rf_mode);
            StaticJsonDocument<200> doc;
			// Add values in the document
			//
			String hum = String(localHum) + "%";
			String temp = String(localTemp) + "°C";
			int dotit = ESP.getChipId();
			doc["libdotid"] = dotit;
			doc["version"] = "1.4";
			doc["date"] = dayStamp;
			doc["temperature"] = temp;
			doc["humidity"] = hum;
      doc["order"] = led[i];
      doc["StateOrder"] = "Send";
			// Add an array.
			//
			//JsonArray data = doc.createNestedArray("order");
			//data.add(led[i]);
			//data.add(" Order Send");          
			// Generate the minified JSON and send it to the Serial port.
			//
			serializeJson(doc, Serial);
			Serial.println();
			serializeJsonPretty(doc, Serial);
			// Write response headers
			String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: \r\nAccess-Control-Allow-Origin: *\r\n";
			client.print(s);
			client.print("\r\n");
//			client.println();
			serializeJsonPretty(doc, client);
			client.print("\r\n");
			//doc.printTo(client);
			client.stop();
			client.flush();
            return;
        }
   } 
  // Add values in the document
  //
  StaticJsonDocument<200> docerror;
  String hum = String(localHum) + "%";
  String temp = String(localTemp) + "°C";
  int dotit = ESP.getChipId();
  docerror["libdotid"] = dotit;
  docerror["version"] = "1.4";
  docerror["date"] = dayStamp;
  docerror["temperature"] = temp;
  docerror["humidity"] = hum;
  docerror["order"] = "URI Not found";
  docerror["StateOrder"] = "Error";  
  // Add an array.
  //
  //JsonArray data = docerror.createNestedArray("order");
  //data.add("Error Request Order");
  //data.add(" Order send ERROR");          
  serializeJson(docerror, Serial);
  Serial.println();
  serializeJsonPretty(docerror, Serial);
  // Write response headers
  //client.println(serializeJsonPretty(docerror));
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: \r\nAccess-Control-Allow-Origin: *\r\n";
  client.print(s);
  client.print("\r\n");
//  client.println();
  serializeJsonPretty(docerror, client);
  
  //doc.printTo(client);
  client.stop();
  client.flush();   
  delay(2000);
}
