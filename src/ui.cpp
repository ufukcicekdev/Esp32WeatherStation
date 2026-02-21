#include "ui.h"
#include "config.h"
#include <time.h>

// --- YARDIMCI FONKSİYON: Basit Hava Durumu İkonu Çiz ---
void drawWeatherIcon(int x, int y, String desc) {
  desc.toLowerCase();
  
  // Güneşli / Açık
  if (desc.indexOf("acik") >= 0 || desc.indexOf("gunes") >= 0) {
    spr.fillCircle(x, y, 20, TFT_YELLOW);
    spr.drawCircle(x, y, 23, TFT_ORANGE);
    // Işınlar
    for (int i = 0; i < 8; i++) {
      float angle = i * 45 * 0.0174533;
      int x1 = x + cos(angle) * 28;
      int y1 = y + sin(angle) * 28;
      int x2 = x + cos(angle) * 35;
      int y2 = y + sin(angle) * 35;
      spr.drawLine(x1, y1, x2, y2, TFT_YELLOW);
    }
  }
  // Yağmurlu
  else if (desc.indexOf("yagmur") >= 0) {
    // Bulut
    spr.fillCircle(x - 10, y, 12, TFT_LIGHTGREY);
    spr.fillCircle(x + 10, y, 12, TFT_LIGHTGREY);
    spr.fillCircle(x, y - 8, 14, TFT_WHITE);
    // Yağmur damlaları
    spr.drawLine(x - 5, y + 15, x - 10, y + 25, TFT_CYAN);
    spr.drawLine(x, y + 15, x - 5, y + 25, TFT_CYAN);
    spr.drawLine(x + 5, y + 15, x, y + 25, TFT_CYAN);
  }
  // Bulutlu (Varsayılan)
  else {
    spr.fillCircle(x - 12, y + 2, 14, TFT_LIGHTGREY);
    spr.fillCircle(x + 12, y + 2, 14, TFT_LIGHTGREY);
    spr.fillCircle(x, y - 8, 18, TFT_WHITE);
  }
}

// --- YARDIMCI FONKSİYON: Batarya İkonu Çiz ---
void drawBatteryIcon(int level) {
  // Sağ üst köşe koordinatları
  int x = 280;
  int y = 10;
  
  // Yüzde Metni (İkonun soluna ekle)
  spr.setTextDatum(MR_DATUM);  // Metni sağdan hizala (Middle Right)
  spr.setTextColor(TFT_WHITE); // Arka plan şeffaf olsun (Header rengini bozmaz)
  spr.drawString(String(level) + "%", x - 5, y + 7, 2);
  spr.setTextDatum(MC_DATUM);  // Hizalamayı merkeze geri al

  // Pil Gövdesi
  spr.drawRoundRect(x, y, 30, 15, 3, TFT_WHITE);
  spr.fillRect(x + 30, y + 4, 3, 7, TFT_WHITE); // Pil ucu

  // Doluluk Rengi
  uint16_t color = TFT_GREEN;
  if (level < 20) color = TFT_RED;
  else if (level < 50) color = TFT_YELLOW;

  // Doluluk Barı (Maksimum genişlik 26px)
  int w = map(level, 0, 100, 0, 26);
  if (w > 0) spr.fillRoundRect(x + 2, y + 2, w, 11, 2, color);
}

void drawWeatherScreen(float localTemp, float localHum, float istTemp, String istDesc, int batLevel) {
  // Arka planı temizle (Sprite üzerinde)
  spr.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x1002); // Çok koyu lacivert arka plan

  // Üst Bar (Header)
  spr.fillRect(0, 0, SCREEN_WIDTH, 40, 0xFCA0); // Canlı Turuncu
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_BLACK, 0xFCA0);
  spr.drawString("HAVA DURUMU", 160, 20, 4); // Fontu büyüttük

  // Batarya İkonu
  drawBatteryIcon(batLevel);

  // --- SOL KART (ODA) ---
  // Kart Arkaplanı
  spr.fillRoundRect(10, 55, 145, 175, 15, 0x2124); // Koyu Gri
  spr.drawRoundRect(10, 55, 145, 175, 15, TFT_SILVER); // Çerçeve
  
  spr.setTextColor(TFT_GREEN, 0x2124);
  spr.drawString("ODA", 82, 80, 4);

  spr.setTextColor(TFT_WHITE, 0x2124);
  spr.setTextFont(6); // Daha büyük, dijital görünümlü font
  String localTempStr = String(localTemp, 1);
  spr.drawString(localTempStr, 82, 130); // Sıcaklık Sayısı
  
  // Derece Simgesi ve 'C' (Sayı genişliğine göre hizala)
  int w1 = spr.textWidth(localTempStr);
  spr.setTextFont(4);
  spr.drawString("C", 82 + (w1 / 2) + 12, 130); 
  spr.drawCircle(82 + (w1 / 2) + 4, 122, 3, TFT_WHITE); // Derece yuvarlağı

  spr.setTextFont(2); // Küçük Font
  spr.setTextColor(TFT_YELLOW, 0x2124);
  spr.drawString("Nem: %" + String(localHum, 0), 82, 180);

  // --- SAĞ KART (DIŞARI) ---
  spr.fillRoundRect(165, 55, 145, 175, 15, 0x0010); // Çok koyu mavi
  spr.drawRoundRect(165, 55, 145, 175, 15, TFT_CYAN); // Çerçeve
  
  spr.setTextColor(TFT_CYAN, 0x0010);
  spr.drawString("ISTANBUL", 237, 80, 4);

  // İkon Çizimi (Sıcaklığın üstüne)
  drawWeatherIcon(237, 120, istDesc);

  spr.setTextColor(TFT_WHITE, 0x0010);
  spr.setTextFont(6);
  String istTempStr = String(istTemp, 1);
  spr.drawString(istTempStr, 237, 170);

  // Derece Simgesi ve 'C'
  int w2 = spr.textWidth(istTempStr);
  spr.setTextFont(4);
  spr.drawString("C", 237 + (w2 / 2) + 12, 170);
  spr.drawCircle(237 + (w2 / 2) + 4, 162, 3, TFT_WHITE);
  
  // Açıklama (En alta)
  spr.setTextFont(2);
  spr.setTextColor(TFT_SILVER, 0x0010);
  spr.drawString(istDesc, 237, 205);

  // Sprite'ı ekrana bas
  spr.pushSprite(0, 0);
}

void drawClockScreen(int batLevel) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    spr.fillScreen(TFT_BLACK);
    spr.setCursor(0,0);
    spr.println("Saat alinamadi");
    spr.pushSprite(0,0);
    return;
  }

  // Sprite temizle
  spr.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK); // Tam siyah arka plan

  spr.setTextDatum(MC_DATUM);
  
  // Batarya İkonu
  drawBatteryIcon(batLevel);
  
  // Türkçe Gün ve Ay İsimleri
  static const char* gunler[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
  static const char* aylar[] = {"Ocak", "Subat", "Mart", "Nisan", "Mayis", "Haziran", "Temmuz", "Agustos", "Eylul", "Ekim", "Kasim", "Aralik"};

  char strH[5];
  char strM[5];
  strftime(strH, 5, "%H", &timeinfo);
  strftime(strM, 5, "%M", &timeinfo);
  
  char timeSec[10];
  strftime(timeSec, 10, "%S", &timeinfo);

  String dateStr = String(timeinfo.tm_mday) + " " + String(aylar[timeinfo.tm_mon]) + " " + String(timeinfo.tm_year + 1900);
  String dayStr = String(gunler[timeinfo.tm_wday]);

  // --- MODERN SAAT TASARIMI ---
  
  // Saat Kutusu (Sol)
  spr.fillRoundRect(20, 40, 130, 120, 10, 0x2124); // Koyu Gri
  spr.setTextColor(TFT_WHITE, 0x2124);
  // 2.4 inç ekrana sığması için Font 6 (Büyük Rakamlar) kullanıyoruz, scale yok
  spr.drawString(strH, 85, 100, 6); 

  // İki nokta üst üste (Yanıp sönme efekti için saniyeye bakabiliriz ama sabit kalsın şimdilik)
  spr.fillCircle(160, 80, 5, TFT_ORANGE);
  spr.fillCircle(160, 120, 5, TFT_ORANGE);

  // Dakika Kutusu (Sağ)
  spr.fillRoundRect(170, 40, 130, 120, 10, 0x2124);
  spr.setTextColor(TFT_CYAN, 0x2124);
  spr.drawString(strM, 235, 100, 6);

  // Saniye Çubuğu (Altta ilerleyen bir bar)
  int secWidth = map(timeinfo.tm_sec, 0, 60, 0, 320);
  spr.fillRect(0, 170, secWidth, 5, TFT_GREEN);

  // Tarih (En Alt)
  spr.setTextColor(TFT_SILVER, TFT_BLACK);
  // Font 4 uzun tarihlerde 320px'e sığmayabilir, Font 2 daha güvenli
  spr.drawString(dayStr + ", " + dateStr, 160, 210, 2);
  
  spr.pushSprite(0, 0);
}

void drawTimerScreen(int hour, int min, int sec, bool isActive, bool isAlarm, int batLevel) {
  // Arka plan temizle
  spr.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK);

  // Başlık
  spr.fillRect(0, 0, SCREEN_WIDTH, 40, 0x4008); // Koyu Bordo
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, 0x4008);
  spr.drawString("ZAMANLAYICI", 160, 20, 4);

  // Batarya İkonu
  drawBatteryIcon(batLevel);

  // --- SÜRE GÖSTERGESİ ---
  char timeStr[12];
  sprintf(timeStr, "%02d:%02d:%02d", hour, min, sec);
  
  spr.setTextColor(isAlarm ? TFT_RED : TFT_WHITE, TFT_BLACK);
  // Font 6 yerine daha büyük görünmesi için Font 7 (7-segment) varsa kullanılabilir ama Font 6 yeterli
  spr.drawString(timeStr, 160, 70, 6); // Saati yukarı taşıdık (90 -> 70)

  // --- BUTONLAR ---
  // Eğer alarm çalıyorsa sadece DURDUR butonu göster
  if (isAlarm) {
    spr.fillRoundRect(60, 150, 200, 60, 10, TFT_RED);
    spr.setTextColor(TFT_WHITE, TFT_RED);
    spr.drawString("ALARMI DURDUR", 160, 180, 4);
  } 
  else {
    // Aktif değilse ayar butonlarını göster (+ ve -)
    if (!isActive) {
      // Eksi Butonu
      spr.fillRoundRect(30, 120, 50, 50, 5, 0x2124); // Aşağı taşıdık (80 -> 120)
      spr.setTextColor(TFT_WHITE, 0x2124);
      spr.drawString("-", 55, 145, 4);

      // Artı Butonu
      spr.fillRoundRect(240, 120, 50, 50, 5, 0x2124); // Aşağı taşıdık (80 -> 120)
      spr.setTextColor(TFT_WHITE, 0x2124);
      spr.drawString("+", 265, 145, 4);
    }

    // Başlat / Durdur Butonu
    uint16_t btnColor = isActive ? 0x8000 : 0x0400; // Aktifse Kırmızımsı, Değilse Yeşilimsi
    String btnText = isActive ? "DURDUR" : "BASLAT";
    
    spr.fillRoundRect(85, 190, 150, 40, 10, btnColor); // Aşağı taşıdık (160 -> 190)
    spr.setTextColor(TFT_WHITE, btnColor);
    spr.drawString(btnText, 160, 210, 4);
  }

  spr.pushSprite(0, 0);
}

void drawMusicScreen(const std::vector<String>& playlist, int currentIndex, bool isPlaying, int volume, int batLevel) {
  // Arka plan
  spr.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK);

  // Başlık
  spr.fillRect(0, 0, SCREEN_WIDTH, 40, 0x8000); // Bordo/Kırmızı
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, 0x8000);
  spr.drawString("MUZIK CALAR", 160, 20, 4);

  // Batarya İkonu
  drawBatteryIcon(batLevel);

  // --- ŞARKI LİSTESİ (Üst Kısım) ---
  int startY = 50; // Listeyi biraz yukarı alalım
  int itemHeight = 25;
  int maxItems = 4; // Yer açmak için satır sayısını 4'e düşürelim
  
  // Listeyi kaydırma mantığı (Seçili şarkıyı ortalamaya çalış)
  int startIdx = 0;
  if (currentIndex > 2) startIdx = currentIndex - 2;
  if (startIdx + maxItems > playlist.size()) startIdx = playlist.size() - maxItems;
  if (startIdx < 0) startIdx = 0;

  if (playlist.empty()) {
    spr.setTextColor(TFT_RED, TFT_BLACK);
    spr.drawString("Sarki Bulunamadi!", 160, 100, 4);
  } else {
    for (int i = 0; i < maxItems && (startIdx + i) < playlist.size(); i++) {
      int idx = startIdx + i;
      String name = playlist[idx];
      name.replace("/MP3/", ""); // Dosya yolunu gizle
      if (name.length() > 25) name = name.substring(0, 25) + "..";

      if (idx == currentIndex) {
        // Seçili satır vurgusu
        spr.fillRoundRect(10, startY + (i * itemHeight), 300, itemHeight, 5, 0x2124);
        spr.setTextColor(TFT_GREEN, 0x2124);
        spr.drawString(">", 20, startY + (i * itemHeight) + 12); // Ok işareti
      } else {
        spr.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      spr.setTextDatum(ML_DATUM); // Sola hizala
      spr.drawString(name, 35, startY + (i * itemHeight) + 12, 2);
    }
  }
  spr.setTextDatum(MC_DATUM); // Merkeze hizalamayı geri yükle

  // --- KONTROLLER ---
  int ctrlY = 180; // Kontrol butonlarını yukarı taşıyalım
  // Geri Butonu (<<)
  spr.fillTriangle(50, ctrlY, 80, ctrlY - 20, 80, ctrlY + 20, TFT_WHITE);
  spr.fillTriangle(20, ctrlY, 50, ctrlY - 20, 50, ctrlY + 20, TFT_WHITE);

  // Oynat / Duraklat (Ortada)
  spr.fillCircle(160, ctrlY, 35, TFT_ORANGE);
  if (isPlaying) {
    // Pause ikonu (II)
    spr.fillRect(145, ctrlY - 15, 10, 30, TFT_BLACK);
    spr.fillRect(165, ctrlY - 15, 10, 30, TFT_BLACK);
  } else {
    // Play ikonu (Üçgen)
    spr.fillTriangle(150, ctrlY - 20, 150, ctrlY + 20, 180, ctrlY, TFT_BLACK);
  }

  // İleri Butonu (>>)
  spr.fillTriangle(240, ctrlY, 270, ctrlY - 20, 270, ctrlY + 20, TFT_WHITE);
  spr.fillTriangle(270, ctrlY, 300, ctrlY - 20, 300, ctrlY + 20, TFT_WHITE);

  // --- SES KONTROL (En Alt) ---
  int volY = 220;
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.drawString("Ses: " + String(volume), 160, volY, 4);

  // Ses - (Sol)
  spr.fillRoundRect(20, volY - 15, 60, 30, 5, 0x2124);
  spr.drawString("-", 50, volY, 4);

  // Ses + (Sağ)
  spr.fillRoundRect(240, volY - 15, 60, 30, 5, 0x2124);
  spr.drawString("+", 270, volY, 4);

  spr.pushSprite(0, 0);
}

void drawStartup(int progress) {
  spr.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLACK);
  
  // Animasyonlu Başlık (@withufuk)
  spr.setTextDatum(MC_DATUM);
  spr.setTextFont(4);
  
  // Hafif bir gölge efekti (Derinlik katar)
  spr.setTextColor(0x2124, TFT_BLACK); // Koyu Gri Gölge
  spr.drawString("@withufuk", 163, 103);
  
  // Ana Metin
  spr.setTextColor(TFT_CYAN, TFT_BLACK);
  spr.drawString("@withufuk", 160, 100);

  // Yükleniyor Çubuğu (Konteyner)
  spr.drawRoundRect(60, 160, 200, 15, 5, TFT_WHITE);
  
  // Yükleniyor Çubuğu (Doluluk)
  if (progress > 100) progress = 100;
  if (progress < 0) progress = 0;
  int w = map(progress, 0, 100, 0, 196); // Çubuğun iç genişliği
  if (w > 0) spr.fillRoundRect(62, 162, w, 11, 4, TFT_GREEN);

  spr.pushSprite(0, 0);
}