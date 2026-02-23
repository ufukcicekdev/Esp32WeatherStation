#ifndef CONFIG_H
#define CONFIG_H

// --- WIFI AYARLARI ---
#define WIFI_SSID       "your_ssid"
#define WIFI_PASSWORD   "your_password"

// --- YOUTUBE AYARLARI ---
#define YOUTUBE_API_KEY "your_api_key" // Google Cloud Console'dan alınan API Key (AIza...)
#define YOUTUBE_CHANNEL_ID "your_channel_id" // YouTube Kanal ID'si (UC...)
#define INTERVAL_YOUTUBE 600000 // 10 Dakika

// --- OPENWEATHERMAP AYARLARI ---
#define API_KEY         "your_token"
#define CITY            "Istanbul"
#define COUNTRY_CODE    "TR"
#define WEATHER_LANG    "tr" // Türkçe yanıt için

// --- PIN TANIMLAMALARI (ESP32 & CrowPanel) ---
#define PIN_LCD_BL      27
#define PIN_I2C_SDA     22
#define PIN_I2C_SCL     21
#define PIN_SPEAKER     26 // Alarmın çalıştığı pin (26)
#define PIN_AMP_EN      4  // Amplifikatör Güç Pini (Sesi açmak için gerekli)
#define PIN_SD_CS       5  // SD Kart CS Pini
#define PIN_BAT_VOLT    32   // Batarya Voltaj Pini

// --- ZAMAN AYARLARI ---
#define NTP_SERVER      "pool.ntp.org"
#define GMT_OFFSET_SEC  10800 // UTC+3 (İstanbul)
#define DAYLIGHT_OFFSET 0

// --- GÜNCELLEME ARALIKLARI (Milisaniye) ---
#define INTERVAL_WEATHER 600000 // 10 Dakika
#define INTERVAL_SENSOR  5000   // 5 Saniye

// --- EKRAN AYARLARI ---
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define COLOR_DEPTH     8       // 8-bit renk (RAM tasarrufu için)
#define SCREEN_TIMEOUT  30000   // 30 Saniye (Ekran zaman aşımı)

#endif