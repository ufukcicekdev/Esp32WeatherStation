#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>

// Global Sprite nesnesine erişim (main.cpp içinde tanımlı)
extern TFT_eSprite spr;

void drawWeatherScreen(float localTemp, float localHum, float istTemp, String istDesc, int batLevel);
void drawClockScreen(int batLevel);
void drawTimerScreen(int hour, int min, int sec, bool isActive, bool isAlarm, int batLevel);
void drawMusicScreen(const std::vector<String>& playlist, int currentIndex, bool isPlaying, int volume, int batLevel);
void drawStartup(int progress);

#endif