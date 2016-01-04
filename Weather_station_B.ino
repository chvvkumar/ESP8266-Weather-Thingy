#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include "SSD1306.h"
#include "SSD1306Ui.h"
#include "Wire.h"
#include "WundergroundClient.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "NTPClient.h"
#include "DHT.h"

// Function prototypes
bool drawFrame1(SSD1306 *, SSD1306UiState*, int, int); 
bool drawFrame2(SSD1306 *, SSD1306UiState*, int, int); 
bool drawFrame3(SSD1306 *, SSD1306UiState*, int, int); 
bool drawFrame4(SSD1306 *, SSD1306UiState*, int, int); 
bool drawFrame5(SSD1306 *, SSD1306UiState*, int, int); 


/***************************
 * Begin Settings
 **************************/
// WIFI
const char* WIFI_SSID = "---"; //Your WiFi SSID 
const char* WIFI_PWD = "----"; //Your WiFi Password

// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// Display Settings
const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = D3; // NodeMCU
const int SDC_PIN = D4; // NodeMCU
SSD1306   display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
SSD1306Ui ui     ( &display );

// DHT Settings
#define DHTPIN D2       // NodeMCU
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321

// Wunderground Settings
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "---"; //Your Wunderground API Key
const String WUNDERGROUND_COUNTRY = "IL";
const String WUNDERGROUND_CITY = "Peoria";

// NTP settings
#define UTC_OFFSET -21600 // UTC offset for Peoria is -8h

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "---"   //Adafruit IO Username
#define AIO_KEY         "---------"   //Adafruit IO API Key
/************ Global State (you don't need to change this!) ******************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);
/****************************** Feeds ***************************************/
// Setup a feed called 'Temperature' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char Temperature_feed[] PROGMEM = AIO_USERNAME "/feeds/Temperature";
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&mqtt, Temperature_feed);
const char Humidity_feed[] PROGMEM = AIO_USERNAME "/feeds/Humidity";
Adafruit_MQTT_Publish Humidity = Adafruit_MQTT_Publish(&mqtt, Humidity_feed);

/***************************
 * End Settings
 **************************/
void MQTT_connect() ;

NTPClient timeClient(UTC_OFFSET); 

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

// Initialize the temperature/ humidity sensor
DHT dht(DHTPIN, DHTTYPE);
float humidity = 0;
float temperature = 0;
uint32_t x=0;

//MQTT Connection setup routine
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
//  Serial.print("Connecting to MQTT... ");
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
//       Serial.println(mqtt.connectErrorString(ret));
//       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
//  Serial.println("MQTT Connected!");
}
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
bool (*frames[])(SSD1306 *display, SSD1306UiState* state, int x, int y) = { drawFrame1, drawFrame2, /*drawFrame3,*/ drawFrame4, drawFrame5 };
int numberOfFrames = 4;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

Ticker ticker;

void drawForecast(SSD1306 *display, int x, int y, int dayIndex) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  display->drawString(x + 20, y, day);
  
  display->setFont(Meteocons_0_21);
  display->drawString(x + 20, y + 15, wunderground.getForecastIcon(dayIndex));

  display->setFont(ArialMT_Plain_16);
  display->drawString(x + 20, y + 37, wunderground.getForecastLowTemp(dayIndex) + "/" + wunderground.getForecastHighTemp(dayIndex));
  //display.drawString(x + 20, y + 51, );
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// Time and date
bool drawFrame1(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = wunderground.getDate();
  int textWidth = display->getStringWidth(date);
  display->drawString(64 + x, y, date);
  display->setFont(ArialMT_Plain_24);
  String time = timeClient.getFormattedTime();
  textWidth = display->getStringWidth(time);
  display->drawString(64 + x, 20 + y, time);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// Big icon and temperature outside
bool drawFrame2(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setFont(ArialMT_Plain_10);
  display->drawString(60 + x, 0 + y, wunderground.getWeatherText());

  display->setFont(ArialMT_Plain_24);
  String temp = wunderground.getCurrentTemp() + "°C";
  display->drawString(60 + x, 20 + y,temp);
  int tempWidth = display->getStringWidth(temp);

  display->setFont(Meteocons_0_42);
  String weatherIcon = wunderground.getTodayIcon();
  int weatherIconWidth = display->getStringWidth(weatherIcon);
  display->drawString(32 + x - weatherIconWidth / 2, 12 + y, weatherIcon);
}

// // Humidity Pressure and Precipitation
// bool drawFrame3(SSD1306 *display, SSD1306UiState* state, int x, int y) {
//   display->setTextAlignment(TEXT_ALIGN_CENTER);
//   display->setFont(ArialMT_Plain_10);
//   display->drawString(32 + x, 0 + y, "Humidity");
//   display->drawString(96 + x, 0 + y, "Pressure");
//   display->drawString(32 + x, 28 + y, "Precipit.");

//   display->setFont(ArialMT_Plain_16);
//   display->drawString(32 + x, 10 + y, wunderground.getHumidity());
//   display->drawString(96 + x, 10 + y, wunderground.getPressure());
//   display->drawString(32 + x, 38 + y, wunderground.getPrecipitationToday());
// }


// Icons for today, tomorrow and day after
bool drawFrame4(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  drawForecast(display, x, y+1, 0);
  drawForecast(display, x + 44, y+1, 2);
  drawForecast(display, x + 88, y+1, 4);
}

// Indoor temperature and humidity
bool drawFrame5(SSD1306 *display, SSD1306UiState* state, int x, int y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64 + x, 0 + y, "Inside");
  display->setFont(ArialMT_Plain_24);
  display->drawString(74 + x, 12 + y, "  " + String(temperature) + "°C");
  display->drawString(74 + x, 31 + y, "  " + String(humidity) + "%");

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Meteocons_0_21);
  display->drawString(10 +x, 15 + y, "'");

  display->setFont(Meteocons_0_21);
  display->drawString(10 +x, 35 + y, "R");

}

// Progress bar when updating
void drawProgress(SSD1306 *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(64, 10, 128,label);
  display->drawRect(10, 48, 108, 12);
  display->fillRect(12, 50, 104 * percentage / 100 , 9);
  display->display();
}

// Called every ten minutes
void updateData(SSD1306 *display) {
  display->flipScreenVertically();
  drawProgress(display, 10, "Updating time...");
  timeClient.begin();
  drawProgress(display, 20, "Updating time...");
  drawProgress(display, 30, "Updating conditions...");
  wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  drawProgress(display, 40, "Updating conditions...");
  drawProgress(display, 50, "Updating forecasts...");
  wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  drawProgress(display, 60, "Updating forecasts...");
  drawProgress(display, 70, "Updating local temperature and humidity");
  humidity = dht.readHumidity();
  delay(200);
  drawProgress(display, 80, "Updating local temperature and humidity");
  temperature = dht.readTemperature();
  delay(200);
  drawProgress(display, 90, "Updating local temperature and humidity");
  lastUpdate = timeClient.getFormattedTime();
  Serial.println(lastUpdate);
  MQTT_connect();
  delay(200);
  Temperature.publish(temperature);
  Humidity.publish(humidity);
  Serial.print(temperature);
  Serial.print("...");
  Serial.println(humidity);
  delay(200);
  drawProgress(display, 100, "Done...");
  readyForWeatherUpdate = false;
  delay(200);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}



void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // initialize display
  display.init();
  display.clear();
  display.display();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(100);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawXbm(5, 15, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
    display.drawString(64, 00, "WiFi Weather Thingy");
    display.drawString(95, 22, "by Kumar");
    display.drawXbm(46, 50, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(60, 50, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(74, 50, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();
    delay(2000);
    counter++;
  }
  
  ui.setTargetFPS(30);

  ui.setActiveSymbole(activeSymbole);
  ui.setInactiveSymbole(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, numberOfFrames);

  // Inital UI takes care of initalising the display too.
  ui.init();

  Serial.println("");
  updateData(&display);
  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
     
}

void loop() {

  if (readyForWeatherUpdate && ui.getUiState().frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

}




