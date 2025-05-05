#include "WiFi.h"
#include "AdafruitIO_WiFi.h"
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include "RTClib.h"
#include "DHT.h" // Include DHT library
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
#include <Wire.h>
#include <U8g2lib.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ---------- Replace with your WiFi and Adafruit IO credentials ----------
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define IO_USERNAME "your_adafruit_io_username"
#define IO_KEY "your_adafruit_io_key"

// ---------------- ESP32 Pins & DHT11 Setup ----------------
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ----------- Adafruit IO Feed Setup --------------
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *temperature_feed = io.feed("temperature");
AdafruitIO_Feed *humidity_feed = io.feed("humidity");
String dataMessage;
String timeStamp;
String temperatureString = "";
String humidityString = "";

void setup() {
  Serial.begin(115200);
  Wire.begin();
  u8g2.begin();
  dht.begin();


  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr); // choose a suitable font
  u8g2.drawStr(3, 15, "Initializing");
  u8g2.sendBuffer();



  if (!SD.begin(D2)) {
    Serial.println("SD Card Initialization Failed!");
    u8g2.drawStr(3, 30, "SD Failed");
    u8g2.sendBuffer();
  } else {
    Serial.println(F("Card initialised... file access enabled..."));
    u8g2.drawStr(3, 30, "SD Enabled");
    u8g2.sendBuffer();
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    u8g2.drawStr(3, 45, "RTC Error");
    u8g2.sendBuffer();
  }
  u8g2.drawStr(3, 45, "RTC Enabled");
  u8g2.sendBuffer();
  //Uncomment to adjust the date and time. Comment it again after uploading in the node.
  //rtc.adjust(DateTime(2024, 10, 27, 10, 0, 0));


  Serial.print("Connecting to Adafruit IO...");
  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to Adafruit IO!");

  delay(500);

  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void loop() {
  io.run();

  // Read temperature and humidity from DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    u8g2.clearBuffer();
    u8g2.setCursor(0, 20);
    u8g2.print("DHT Error");
    u8g2.sendBuffer();
    delay(2000); // Wait and try again
    return;
  }
  readTime();

  temperatureString = String(t, 2);
  humidityString = String(h, 2);

  Serial.print("Temperature: ");
  Serial.print(temperatureString);
  Serial.print(" *C\t");
  Serial.print("Humidity: ");
  Serial.print(humidityString);
  Serial.println(" %");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setCursor(0, 20);
  u8g2.print("Temp: ");
  u8g2.print(temperatureString);
  u8g2.print(" C");
  u8g2.setCursor(0, 40);
  u8g2.print("Hum: ");
  u8g2.print(humidityString);
  u8g2.print(" %");
  u8g2.sendBuffer();

  // Send data to Adafruit IO
  temperature_feed->save(t);
  humidity_feed->save(h);
  logSDCard();
  delay(2000);
}

void readTime() {
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  timeStamp = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + "," + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ",";
  const char* ts = timeStamp.c_str();

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, ts);  // Adjusted position for time on the smaller display
  u8g2.sendBuffer();
}

void logSDCard() {
  dataMessage = timeStamp + "," + temperatureString + "," + humidityString + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/dhtdata.txt", dataMessage.c_str());
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(80, 40, "SD write");
  u8g2.sendBuffer();
  delay(1000);
  u8g2.clearBuffer();
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
    delay(1000);
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
