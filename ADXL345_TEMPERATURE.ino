#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ThingSpeak.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "arduinoFFT.h"
#include "arduino_secrets.h"  // SSID, PASS, ThingSpeak ve Telegram bilgileri burada

// ------------ WiFi Ayarları ------------
WiFiClient wifiClient;
WiFiClientSecure secureClient;
// WiFi Ayarları
const char* ssid = "Redmi Note 9";
const char* password = "Nzrslm00.";

// ------------ Telegram Ayarları ------------

#define BOTtoken "7829233263:AAHS3hvcatPetWM22udVg9m3Akj9bXDjMFg" //Token obtained from BotFather
#define CHAT_ID "1667123081"
UniversalTelegramBot bot(BOTtoken, secureClient);

// ------------ ThingSpeak Ayarları ------------
unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;

// ------------ DS18B20 Ayarları ------------
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
float currentTemperature = 0.0;
bool overheatAlertSent = false;

// ------------ ADXL345 Ayarları ------------
#define SAMPLE_COUNT 64
#define SAMPLE_INTERVAL_MS 0.5
#define SAMPLING_FREQUENCY 2000.0
#define RMS_THRESHOLD 4.5
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
double vReal[SAMPLE_COUNT];
double vImag[SAMPLE_COUNT];
ArduinoFFT FFT = ArduinoFFT(vReal, vImag, SAMPLE_COUNT, SAMPLING_FREQUENCY);
float rms = 0.0;
bool vibrationAlertSent = false;

// ------------ Zamanlayıcılar ------------
unsigned long lastThingSpeakTime = 0;
const unsigned long thingSpeakDelay = 20000;
unsigned long lastTelegramCheck = 0;
const int telegramDelay = 1000;

void setup() {
  Serial.begin(115200);
  
  // DS18B20 başlat
  sensors.begin();

  // ADXL345 başlat
  if (!accel.begin()) {
    Serial.println("ADXL345 bulunamadı!");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_8_G);
  accel.setDataRate(ADXL345_DATARATE_200_HZ);

  // WiFi bağlan
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi Bağlandı. IP: " + WiFi.localIP().toString());

  // HTTPS sertifika ve ThingSpeak başlat
  secureClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  ThingSpeak.begin(wifiClient);
}

void loop() {
  // Telegram komutlarını dinle
  if (millis() - lastTelegramCheck > telegramDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTelegramCheck = millis();
  }

  // Her 20 saniyede sıcaklık ve titreşim ölçümü yap
  if (millis() - lastThingSpeakTime > thingSpeakDelay) {
    measureTemperature();
    measureVibration();
    sendToThingSpeak();
    lastThingSpeakTime = millis();
  }
}

// ------------ Sıcaklık Ölçümü ------------
void measureTemperature() {
  sensors.requestTemperatures();
  currentTemperature = sensors.getTempCByIndex(0);

  if (currentTemperature == DEVICE_DISCONNECTED_C) {
    Serial.println("❌ Sıcaklık sensörü hatası.");
    return;
  }

  Serial.print("🌡️ Sıcaklık: ");
  Serial.println(currentTemperature);

  if (currentTemperature > 70.0 && !overheatAlertSent) {
    bot.sendMessage(CHAT_ID, "🚨 UYARI: Sıcaklık 70°C'nin üzerine çıktı!\nAnlık: " + String(currentTemperature) + "°C", "");
    overheatAlertSent = true;
  } else if (currentTemperature <= 70.0) {
    overheatAlertSent = false;
  }
}

// ------------ Titreşim Ölçümü ve FFT Analizi ------------
void measureVibration() {
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sensors_event_t event;
    accel.getEvent(&event);

    float correctedX = (event.acceleration.x - 0.59) * 0.9580;
    float correctedY = (event.acceleration.y - 0.31) * 0.9655;
    float correctedZ = (event.acceleration.z + 0.905) * 0.9844;

    double accMag = sqrt(correctedX * correctedX +
                         correctedY * correctedY +
                         correctedZ * correctedZ);
    double dynamicAcc = accMag - 9.8;

    vReal[i] = dynamicAcc;
    vImag[i] = 0;
    delay(SAMPLE_INTERVAL_MS);
  }

  FFT.windowing(vReal, SAMPLE_COUNT, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLE_COUNT, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLE_COUNT);

  double sumSq = 0;
  int count = 0;
  for (int i = 1; i < SAMPLE_COUNT / 2; i++) {
    double freq = (i * SAMPLING_FREQUENCY) / SAMPLE_COUNT;
    if (freq >= 10 && freq <= 1000) {
      sumSq += vReal[i] * vReal[i];
      count++;
    }
  }

  rms = sqrt(sumSq / count) - 0.5;
  Serial.print("📈 RMS Titreşim: ");
  Serial.println(rms, 4);

  if (rms > RMS_THRESHOLD && !vibrationAlertSent) {
    bot.sendMessage(CHAT_ID, "⚠️ UYARI: Titreşim değeri sınırı aştı!\nRMS: " + String(rms, 2) + " mm/s", "");
    vibrationAlertSent = true;
  } else if (rms <= RMS_THRESHOLD) {
    vibrationAlertSent = false;
  }
}

// ------------ ThingSpeak'e Veri Gönderme ------------
void sendToThingSpeak() {
  ThingSpeak.setField(1, currentTemperature);
  ThingSpeak.setField(2, rms);
  int response = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (response == 200) {
    Serial.println("📤 ThingSpeak güncellendi.");
  } else {
    Serial.print("ThingSpeak hatası: ");
    Serial.println(response);
  }
}

// ------------ Telegram Komutları ------------
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "❌ Yetkisiz kullanıcı.", "");
      continue;
    }

    if (text == "/start") {
      String msg = "👋 Merhaba, " + from_name + ".\n";
      msg += "/state - Anlık sıcaklık ve RMS titreşim bilgisini gösterir.";
      bot.sendMessage(chat_id, msg, "");
    }

    if (text == "/state") {
      String msg = "🌡️ Sıcaklık: " + String(currentTemperature) + "°C\n";
      msg += "📈 RMS Titreşim: " + String(rms, 2) + " mm/s";
      bot.sendMessage(chat_id, msg, "");
    }
  }
}
