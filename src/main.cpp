#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <SD.h>
#include <FS.h>
#include <Audio.h>
#include <esp_wifi.h> // Wi-Fi güç ayarları için gerekli
#include "soc/soc.h"  // Brownout ayarları için
#include "soc/rtc_cntl_reg.h" // Brownout ayarları için
#include "config.h"
#include "ui.h"

// --- NESNELER ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft); // Sprite nesnesi (Flicker-free çizim için)
Adafruit_AHTX0 aht;
Audio audio;
SPIClass sdSPI(HSPI); // SD Kart için özel SPI nesnesi

// --- DEĞİŞKENLER ---
float localTemp = 0.0;
float localHum = 0.0;
float istTemp = 0.0;
String istDesc = "--";
long youtubeSubs = 0;
String youtubeName = "--";

unsigned long lastWeatherUpdate = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastYoutubeUpdate = 0;

// Ekran Yönetimi
int currentPage = 0; // 0: Hava Durumu, 1: Saat, 2: Timer, 3: Müzik, 4: YouTube
uint16_t touchStartX = 0;
uint16_t touchStartY = 0;
uint16_t touchLastX = 0;
uint16_t touchLastY = 0;
bool isTouching = false;
unsigned long lastTouchTime = 0; // Debounce için
bool isScreenOn = true; // Ekran durumu

// Timer Değişkenleri
int timerHour = 0;
int timerMin = 5;
int timerSec = 0;
bool timerActive = false;
bool alarmActive = false;
unsigned long lastTimerTick = 0;
bool ledcAttached = false; // Alarm sesi için pin durumu

// Müzik Değişkenleri
std::vector<String> playlist;
int currentSongIndex = 0;
bool isPlaying = false;
int musicVolume = 4; // Cızırtıyı önlemek için sesi düşük başlatıyoruz (Sonra arttırılabilir)

// --- FONKSİYONLAR ---
void getWeatherData();
void updateLocalSensor();
void checkTouch();
void handleTimerLogic();
void loadSongs();
int getBatteryLevel();
void getYouTubeData();

// UI.h içinde tanımlı olması gereken fonksiyon (Header'ı değiştiremediğimiz için burada extern ediyoruz)
extern void drawYouTubeScreen(long subCount, String name, int batLevel);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Brownout (Düşük voltaj) reset korumasını kapat
  setCpuFrequencyMhz(240); // İşlemciyi maksimum hıza zorla (Ses kalitesi için)
  Serial.begin(115200);

  lastTouchTime = millis(); // Zamanlayıcıyı başlat

  // 0. SPI Çakışmasını Önleme (Tüm CS Pinlerini Pasif/HIGH Yap)
  // Bu adım çok kritiktir, cihazlar birbirini susturur.
  pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH);    // Ekran
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH); // Dokunmatik
  pinMode(PIN_SD_CS, OUTPUT); digitalWrite(PIN_SD_CS, HIGH); // SD Kart

  // 1. SD Kartı Özel SPI Nesnesi ile Başlat
  // Bu yöntem, TFT kütüphanesinin SPI ayarlarından etkilenmemesini sağlar.
  // Fabrika verilerine göre SD Kart pinleri: SCK=18, MISO=19, MOSI=23
  sdSPI.begin(18, 19, 23, PIN_SD_CS); 
  bool sdInited = false;
  if (!SD.begin(PIN_SD_CS, sdSPI, 16000000)) { // 16MHz en kararlı hızdır, cızırtıyı azaltır
    Serial.println("SD Kart Bulunamadi!");
  } else {
    Serial.println("SD Kart Baslatildi.");
    sdInited = true;
  }

  // 3. Donanım Pinlerini Ayarla
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  
  // Amplifikatörü Aç (Sessiz moddan çıkar)
  pinMode(PIN_AMP_EN, OUTPUT);
  digitalWrite(PIN_AMP_EN, HIGH);

  // 4. Ekranı ve Sprite'ı Başlat
  tft.init();
  tft.setRotation(3); // Yatay mod (Landscape)
  tft.fillScreen(TFT_BLACK);
  
  spr.setColorDepth(COLOR_DEPTH); 
  spr.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  // --- AÇILIŞ ANİMASYONU BAŞLIYOR ---
  drawStartup(0); // %0
  
  // Şarkıları Yükle (Eğer SD başarılıysa)
  if (sdInited) {
    loadSongs(); // Şarkıları yükle
  }
  drawStartup(20); // %20

  // Audio Başlatma (Dahili DAC Kullanımı)
  // setPinout(BCLK, LRC, DOUT) -> BCLK=0, LRC=0, DOUT=PIN_SPEAKER (Dahili DAC Modu)
  audio.setPinout(0, 0, PIN_SPEAKER);
  audio.setVolume(musicVolume);

  // Dokunmatik Kalibrasyonu (Basit manuel kalibrasyon verisi)
  // Eğer dokunmatik ters çalışırsa bu değerleri değiştirmeniz gerekebilir.
  uint16_t calData[5] = { 300, 3500, 300, 3500, 1 };
  tft.setTouch(calData);

  // NOT: LEDC (Alarm) kurulumunu burada YAPMIYORUZ.
  // Çünkü Audio kütüphanesi ile çakışır. Sadece alarm çalarken aktif edeceğiz.
  
  // AHT10 Başlatma (Config'den pinleri al)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); 
  if (!aht.begin(&Wire)) {
    Serial.println("AHT10 bulunamadi! Kablolari kontrol edin.");
  } else {
    Serial.println("AHT10 bulundu.");
  }
  drawStartup(60); // %60

  // WiFi Başlatma
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  esp_wifi_set_ps(WIFI_PS_NONE); // Wi-Fi güç tasarrufunu donanım seviyesinde kapat (En etkili yöntem)
  
  // WiFi bağlanırken animasyon oynat (Bar dolup boşalsın veya ilerlesin)
  int wifiAnim = 60;
  int wifiTimeout = 0; // Zaman aşımı sayacı
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 300) { // Yaklaşık 15 saniye bekle
    delay(50);
    wifiAnim++;
    if (wifiAnim > 90) wifiAnim = 60; // 60-90 arası gidip gel
    drawStartup(wifiAnim);
    wifiTimeout++;
  }
  drawStartup(95); // %95 (Bağlandı)
  
  // Zaman Ayarı
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);

  // İlk verileri al
  updateLocalSensor();
  getWeatherData();
  getYouTubeData();
  
  drawStartup(100); // %100
  delay(500); // Kullanıcı tam doluluğu görsün

  drawWeatherScreen(localTemp, localHum, istTemp, istDesc, getBatteryLevel());
}

void loop() {
  // Dokunmatik Kontrolü (İşlemciyi yormamak için sınırlandırıldı)
  // Müzik çalarken dokunmatiği çok sık kontrol etmek sesi bozabilir.
  // Müzik modunda (Sayfa 3) kontrol aralığını 200ms'ye çıkarıyoruz, diğerlerinde 50ms.
  static unsigned long lastTouchCheck = 0;
  int touchInterval = (currentPage == 3) ? 100 : 50; // 200ms çok yavaş, 100ms'ye düşürdük

  if (millis() - lastTouchCheck > touchInterval) { 
    lastTouchCheck = millis();
    checkTouch();
  }

  // Batarya Kritik Seviye Kontrolü
  static unsigned long lastBatCheck = 0;
  if (millis() - lastBatCheck > 10000) { // 10 saniyede bir kontrol et
    lastBatCheck = millis();
    int bat = getBatteryLevel();
    // Eğer pil %0 ise (veya voltaj çok düşükse) cihazı korumaya al
    if (bat == 0) {
       tft.fillScreen(TFT_BLACK);
       tft.setTextColor(TFT_RED, TFT_BLACK);
       tft.setTextDatum(MC_DATUM);
       tft.drawString("PIL BITTI!", 160, 120, 4);
       delay(3000);
       digitalWrite(PIN_LCD_BL, LOW); // Ekran ışığını kapat
       esp_deep_sleep_start(); // Cihazı derin uykuya al (Reset döngüsünü önler)
    }
  }

  // Ekran Zaman Aşımı Kontrolü (Otomatik Karartma)
  if (isScreenOn && (millis() - lastTouchTime > SCREEN_TIMEOUT)) {
    digitalWrite(PIN_LCD_BL, LOW); // Işığı Kapat
    isScreenOn = false;
  }

  // Zamanlayıcılar
  unsigned long currentMillis = millis();

  // Sensör Güncelleme
  // Müzik çalmıyorsa güncelle (Cızırtıyı önlemek için)
  if (currentPage != 3 && currentMillis - lastSensorUpdate >= INTERVAL_SENSOR) {
    lastSensorUpdate = currentMillis;
    updateLocalSensor();
    if (currentPage == 0) drawWeatherScreen(localTemp, localHum, istTemp, istDesc, getBatteryLevel());
  }

  // API Güncelleme
  if (currentPage != 3 && currentMillis - lastWeatherUpdate >= INTERVAL_WEATHER) {
    lastWeatherUpdate = currentMillis;
    getWeatherData();
    if (currentPage == 0) drawWeatherScreen(localTemp, localHum, istTemp, istDesc, getBatteryLevel());
  }

  // Saat Ekranı Güncellemesi (Her saniye)
  if (currentPage == 1) {
    static unsigned long lastClockUpdate = 0;
    if (millis() - lastClockUpdate > 1000) {
      lastClockUpdate = millis();
      drawClockScreen(getBatteryLevel());
    }
  }

  // YouTube Güncelleme
  if (currentPage != 3 && currentMillis - lastYoutubeUpdate >= INTERVAL_YOUTUBE) {
    lastYoutubeUpdate = currentMillis;
    getYouTubeData();
    if (currentPage == 4) drawYouTubeScreen(youtubeSubs, youtubeName, getBatteryLevel());
  }

  // Timer Mantığı
  if (currentPage == 2) {
    handleTimerLogic();
  }

  // Audio Döngüsü
  if (currentPage == 3) {
    audio.loop();
  }
}

void checkTouch() {
  // Dokunmatik için global SPI ayarlarının geçerli olduğundan emin olmak gerekebilir
  // Ancak TFT_eSPI bunu genellikle kendi içinde halleder.

  uint16_t x, y;
  // Hassasiyeti arttırmak için threshold değerini 1000 yapıyoruz (Daha hafif dokunuşları algılar)
  bool pressed = tft.getTouch(&x, &y, 250); // Hassasiyet arttırıldı (Daha hafif dokunuşlar için)

  if (pressed) {
    // Rotation 3 Düzeltmesi: Ekran 180 derece döndüğü için koordinatları ters çevir
    // (0,0) noktası değiştiği için X ve Y eksenlerini tersliyoruz.
    x = SCREEN_WIDTH - x;
    y = SCREEN_HEIGHT - y;

    lastTouchTime = millis(); // Dokunma algılandı, süreyi sıfırla

    if (!isScreenOn) {
      digitalWrite(PIN_LCD_BL, HIGH); // Işığı Aç
      isScreenOn = true;
      return; // Sadece uyandır, bu dokunuşla işlem yapma (tıklamayı yut)
    }

    if (!isTouching) {
      // Dokunma yeni başladı
      touchStartX = x;
      touchStartY = y;
      isTouching = true;
    }
    // Parmağın son konumunu sürekli takip et
    touchLastX = x;
    touchLastY = y;
  } else {
    if (isTouching) {
      isTouching = false;

      // Kaydırma (Swipe) Algılama
      int deltaX = touchLastX - touchStartX;
      int deltaY = touchLastY - touchStartY;
      
      // Kaydırma eşiğini 40 piksele çıkardık (Tıklarken yanlışlıkla kaydırmayı önlemek için)
      // Y ekseni toleransını 100'e çıkardık (Çapraz kaydırmaları da kabul etsin)
      if (abs(deltaX) > 40 && abs(deltaY) < 100) {
        // Yatay Kaydırma
        // Not: Dokunmatik panelin X ekseni ters (320 -> 0) olduğu için mantığı buna göre kuruyoruz.
        // deltaX > 0 : Parmağı sağdan sola çekme (Next)
        // deltaX < 0 : Parmağı soldan sağa çekme (Prev)
        
        int prevPage = currentPage;

        if (deltaX > 0) {
          // İleri (Next)
          currentPage++;
          if (currentPage > 4) currentPage = 0;
        } else {
          // Geri (Prev)
          currentPage--;
          if (currentPage < 0) currentPage = 4;
        }

        // Sayfa değiştiğinde Alarmı ve LEDC'yi kesinlikle kapat (Pin çakışmasını önle)
        if (alarmActive || ledcAttached) {
            alarmActive = false;
            ledcWrite(0, 0);
            ledcDetachPin(PIN_SPEAKER);
            ledcAttached = false;
        }

        // Müzik Modu Geçişlerinde WiFi Yönetimi (Cızırtıyı önlemek için)
        if (currentPage == 3 && prevPage != 3) {
            // Müziğe girildi: WiFi kapat (DAC üzerindeki paraziti keser)
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        } 
        else if (prevPage == 3 && currentPage != 3) {
            // Müzikten çıkıldı: WiFi aç
            WiFi.mode(WIFI_STA);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            esp_wifi_set_ps(WIFI_PS_NONE);
        }
        
        // Sayfayı Çiz
        tft.fillScreen(TFT_BLACK);
        if (currentPage == 0) drawWeatherScreen(localTemp, localHum, istTemp, istDesc, getBatteryLevel());
        else if (currentPage == 1) drawClockScreen(getBatteryLevel());
        else if (currentPage == 2) drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
        else if (currentPage == 3) drawMusicScreen(playlist, currentSongIndex, isPlaying, musicVolume, getBatteryLevel());
        else if (currentPage == 4) drawYouTubeScreen(youtubeSubs, youtubeName, getBatteryLevel());
      }
      // Eğer hareket azsa bu bir Tıklamadır (Tap)
      else {
        // Hava Durumu (0), Saat (1) veya YouTube (4) ekranındaysak dokunarak geçiş yap
        if (currentPage == 0 || currentPage == 1 || currentPage == 4) {
             currentPage++;
             if (currentPage > 4) currentPage = 0;
             
             // Sayfayı Çiz
             tft.fillScreen(TFT_BLACK);
             if (currentPage == 0) drawWeatherScreen(localTemp, localHum, istTemp, istDesc, getBatteryLevel());
             else if (currentPage == 1) drawClockScreen(getBatteryLevel());
             else if (currentPage == 2) drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
             else if (currentPage == 3) drawMusicScreen(playlist, currentSongIndex, isPlaying, musicVolume, getBatteryLevel());
             else if (currentPage == 4) drawYouTubeScreen(youtubeSubs, youtubeName, getBatteryLevel());
        }
        // Timer Ekranı (2) Kontrolleri
        else if (currentPage == 2) {
          // --- TIMER EKRANI BUTONLARI ---
          if (alarmActive) {
           alarmActive = false;
           ledcWrite(0, 0); // Sesi kes
           ledcDetachPin(PIN_SPEAKER); // Pini serbest bırak
           ledcAttached = false;
           drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
          }
          else {
           // Başlat/Durdur (En Alt Kısım, Y > 180)
           if (touchLastY > 180) {
             timerActive = !timerActive;
             drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
           }
           // Ayar Butonları (Orta Kısım, Y: 110 - 170)
           else if (!timerActive && touchLastY > 110 && touchLastY < 170) {
             // Eksi (Sol taraf)
             if (touchLastX > 220) { // X ekseni ters: Sol taraf yüksek değer
               timerMin--;
               if (timerMin < 0) {
                 if (timerHour > 0) {
                   timerHour--;
                   timerMin = 59;
                 } else {
                   timerMin = 0;
                 }
               }
             }
             // Artı (Sağ taraf)
             else if (touchLastX < 100) { // X ekseni ters: Sağ taraf düşük değer
               timerMin++;
               if (timerMin > 59) {
                 timerMin = 0;
                 timerHour++;
               }
             }
             drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
           }
          }
        } 
        else if (currentPage == 3) {
          // --- MÜZİK EKRANI BUTONLARI ---
          if (playlist.size() == 0) return;

          // Oynat / Duraklat (Orta) - Alanı DAHA DA genişlettik (Iskalamayı önlemek için)
          // Y: 130-230, X: 110-210 (Merkez 160)
          if (touchLastY > 130 && touchLastY < 230 && touchLastX > 110 && touchLastX < 210) {
            if (isPlaying) {
              audio.pauseResume();
              isPlaying = false;
            } else {
              if (audio.isRunning()) {
                 audio.pauseResume();
              } else {
                 // Audio kütüphanesi SD nesnesini kullanır, SD nesnesi de sdSPI'yı kullanır.
                 audio.connecttoFS(SD, playlist[currentSongIndex].c_str());
              }
              isPlaying = true;
            }
          }
          // Önceki Şarkı (Sol) - Alanı genişlettik
          else if (touchLastY > 130 && touchLastY < 230 && touchLastX < 110) { // X ters: Sağ taraf
             // X ekseni ters olduğu için: X < 100 aslında SAĞ taraf (İleri)
             currentSongIndex++;
             if (currentSongIndex >= playlist.size()) currentSongIndex = 0;
             audio.connecttoFS(SD, playlist[currentSongIndex].c_str());
             isPlaying = true;
          }
          // Sonraki Şarkı (Sağ Alt, Y: 165-235, X > 220)
          // Sonraki Şarkı (Sağ) - Alanı genişlettik
          else if (touchLastY > 130 && touchLastY < 230 && touchLastX > 210) { // X ters: Sol taraf
             // X ekseni ters olduğu için: X > 220 aslında SOL taraf (Geri)
             currentSongIndex--;
             if (currentSongIndex < 0) currentSongIndex = playlist.size() - 1;
             audio.connecttoFS(SD, playlist[currentSongIndex].c_str());
             isPlaying = true;
          }
          // Liste Üzerinden Şarkı Seçimi (Üst Kısım, Y: 50 - 150)
          else if (touchLastY > 50 && touchLastY < 150) {
             // Hangi satıra tıklandığını hesapla
             int clickedRow = (touchLastY - 50) / 25;
             
             // Ekrandaki listenin başlangıç indeksini bul
             int startIdx = 0;
             if (currentSongIndex > 2) startIdx = currentSongIndex - 2;
             if (startIdx + 5 > playlist.size()) startIdx = playlist.size() - 5;
             if (startIdx < 0) startIdx = 0;
             
             int targetIdx = startIdx + clickedRow;
             if (targetIdx < playlist.size()) {
               currentSongIndex = targetIdx;
               audio.connecttoFS(SD, playlist[currentSongIndex].c_str());
               isPlaying = true;
             }
          }
          // Ses Kontrol (En Alt Kısım, Y > 210)
          else if (touchLastY > 210) {
             if (touchLastX > 220) { // X ters: Sol taraf (Ekranda Sol - Ses Azalt)
                if (musicVolume > 0) musicVolume--;
                audio.setVolume(musicVolume);
             } 
             else if (touchLastX < 100) { // X ters: Sağ taraf (Ekranda Sağ - Ses Arttır)
                if (musicVolume < 21) musicVolume++;
                audio.setVolume(musicVolume);
             }
             drawMusicScreen(playlist, currentSongIndex, isPlaying, musicVolume, getBatteryLevel());
          }
          
          drawMusicScreen(playlist, currentSongIndex, isPlaying, musicVolume, getBatteryLevel());
        }
      }
      
    }
  }
}

void handleTimerLogic() {
  // Alarm Sesi (Bip Bip Efekti)
  if (alarmActive) {
    // Alarm başladığında LEDC'yi bağla
    if (!ledcAttached) {
      ledcSetup(0, 1000, 8);
      ledcAttachPin(PIN_SPEAKER, 0);
      ledcAttached = true;
    }
    
    if (millis() % 500 < 250) ledcWrite(0, 127); // Ses Aç
    else ledcWrite(0, 0);   // Ses Kapat
    return;
  }
  
  // Alarm bittiyse LEDC'yi serbest bırak (Müzik için)
  if (ledcAttached) {
    ledcDetachPin(PIN_SPEAKER);
    ledcAttached = false;
  }

  // Geri Sayım
  if (timerActive) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      
      if (timerSec == 0) {
        if (timerMin == 0) {
          if (timerHour == 0) {
            // SÜRE BİTTİ!
            timerActive = false;
            alarmActive = true;
            drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
          } else {
            timerHour--;
            timerMin = 59;
            timerSec = 59;
            drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
          }
        } else {
          timerMin--;
          timerSec = 59;
          drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
        }
      } else {
        timerSec--;
        drawTimerScreen(timerHour, timerMin, timerSec, timerActive, alarmActive, getBatteryLevel());
      }
    }
  }
}

void updateLocalSensor() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  localTemp = temp.temperature;
  localHum = humidity.relative_humidity;
}

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // lang=tr parametresi eklendi
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(CITY) + "," + String(COUNTRY_CODE) + "&units=metric&lang=" + String(WEATHER_LANG) + "&appid=" + String(API_KEY);
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      istTemp = doc["main"]["temp"];
      // 'main' yerine 'description' kullanarak daha detaylı Türkçe bilgi alıyoruz
      const char* desc = doc["weather"][0]["description"];
      istDesc = String(desc);
      
      // Türkçe karakterleri ekrana uygun hale getirme (ASCII dönüşümü)
      istDesc.replace("ç", "c");
      istDesc.replace("Ç", "C");
      istDesc.replace("ğ", "g");
      istDesc.replace("Ğ", "G");
      istDesc.replace("ı", "i");
      istDesc.replace("İ", "I");
      istDesc.replace("ö", "o");
      istDesc.replace("Ö", "O");
      istDesc.replace("ş", "s");
      istDesc.replace("Ş", "S");
      istDesc.replace("ü", "u");
      istDesc.replace("Ü", "U");
    }
    http.end();
  }
}

void getYouTubeData() {
  if (WiFi.status() == WL_CONNECTED) {
    // --- BELLEK YÖNETİMİ ---
    // HTTPS bağlantısı için yer açmak adına Sprite'ı geçici olarak siliyoruz (~76KB yer açılır)
    spr.deleteSprite();

    // --- BLOK BAŞLANGICI ---
    // Bu blok sayesinde client ve http nesneleri işleri bitince hemen hafızadan silinir.
    {
      WiFiClientSecure client;
      client.setInsecure(); 
      
      HTTPClient http;
      String url = "https://www.googleapis.com/youtube/v3/channels?part=statistics,snippet&id=" + String(YOUTUBE_CHANNEL_ID) + "&key=" + String(YOUTUBE_API_KEY);
      
      if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode > 0) {
          String payload = http.getString();
          DynamicJsonDocument doc(4096); 
          DeserializationError error = deserializeJson(doc, payload);
          if (!error) {
             if (doc.containsKey("items") && doc["items"].size() > 0) {
               youtubeSubs = doc["items"][0]["statistics"]["subscriberCount"].as<long>();
               youtubeName = doc["items"][0]["snippet"]["title"].as<String>();
             }
          }
        }
        http.end();
      }
    } // --- BLOK BİTİŞİ: client ve http burada yok edilir, RAM serbest kalır ---

    // --- BELLEK YÖNETİMİ ---
    // İşlem bitti, Sprite'ı tekrar oluşturuyoruz
    spr.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  }
}

void loadSongs() {
  playlist.clear();
  File root = SD.open("/MP3");
  if (!root) {
    Serial.println("MP3 klasoru acilamadi!");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String fileName = file.name();
      // Gizli dosyaları (._ ile başlayan) ve MP3 olmayanları filtrele
      if (!fileName.startsWith(".") && (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
        // Dosya adının başına klasör yolunu ekle
        playlist.push_back("/MP3/" + fileName);
        Serial.println("Bulunan sarki: " + fileName);
      }
    }
    file = root.openNextFile();
  }
  root.close();
}

int getBatteryLevel() {
  // Analog okuma (0-4095)
  // Stabilite için çoklu okuma yapıp ortalamasını alalım
  long sum = 0;
  for(int i=0; i<10; i++) {
    sum += analogRead(PIN_BAT_VOLT);
    delay(2);
  }
  int raw = sum / 10;
  
  // Voltaj hesaplama (Voltaj bölücü genellikle 1/2 oranındadır)
  // 3.3V referans, 12-bit ADC
  // V_bat = (raw / 4095.0) * 3.3 * 2 (Bölücü katsayısı)
  // LiPo Pil: 3.0V (%0) - 4.2V (%100)
  // ADC Değerleri: ~1860 (%0) - ~2600 (%100)
  
  int level = map(raw, 1860, 2600, 0, 100);
  if (level < 0) level = 0;
  if (level > 100) level = 100;
  
  return level;
}