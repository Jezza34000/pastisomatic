#ifndef DISPLAY_FUNCTIONS_H
#define DISPLAY_FUNCTIONS_H

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Screen declaration
extern Adafruit_SH1106G display;

const int led_mapping[NUM_LEDS] = {LED_1, LED_2, LED_3, LED_4};

const bool allOff[NUM_LEDS] = {false, false, false, false};
const bool allOn[NUM_LEDS] = {true, true, true, true};
const bool led_1[NUM_LEDS] = {true, false, false, false};
const bool led_2[NUM_LEDS] = {false, true, false, false};
const bool led_3[NUM_LEDS] = {false, false, true, false};
const bool led_4[NUM_LEDS] = {false, false, false, true};

int lastDisplayedIntensity = -1;

void led_control(const bool booleanList[NUM_LEDS]) {
    for (int i = 0; i < NUM_LEDS; ++i) {
        if (booleanList[i]) {
            // Allumer la LED correspondante
            digitalWrite(led_mapping[i], HIGH);
            Serial.print("LED ");
            Serial.print(i+1);
            Serial.println(" allumée.");
        } else {
            // Éteindre la LED correspondante
            digitalWrite(led_mapping[i], LOW);
            Serial.print("LED ");
            Serial.print(i+1);
            Serial.println(" éteinte.");
        }
    }
}

void display_message(int message) {

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  switch(message) {
    case 0:
      display.setTextSize(2);
      display.setCursor(15, 25);
      display.println("Pret !");
      break;
    case 1:
      display.setTextSize(2);
      display.setCursor(15, 10);
      display.println("Tchin");
      display.setCursor(40, 35);
      display.println("Tchin !");
      break;
    case 2:
      display.setTextSize(2);
      display.setCursor(15, 10);
      display.println("Ajout...");
      display.setCursor(15, 25);
      display.setTextSize(3);
      display.println("Eau");
      break;
    case 3:
      display.setTextSize(2);
      display.setCursor(15, 10);
      display.println("Ajout...");
      display.setCursor(15, 25);
      display.setTextSize(3);
      display.println("Pastis");
      break;
    case 4:
      display.setTextSize(2);
      display.setCursor(5, 20);
      display.println("Service...");
      break;
    case 5:
      display.setTextSize(1);
      display.setCursor(5, 9);
      display.println("Amorcage pompes...");
      display.setCursor(5, 25);
      display.println("-> Eau");
      break;
    case 6:
      display.setTextSize(1);
      display.setCursor(5, 9);
      display.println("Amorcage pompes...");
      display.setCursor(5, 25);
      display.println("-> Eau");
      display.setCursor(5, 40);
      display.println("-> Pastis");
      break;
    case 7:
      display.setTextSize(3);
      display.setCursor(5, 10);
      display.println("Non...");
      display.setTextSize(2);
      display.setCursor(5, 35);
      display.println("Trop fort");
      break;
    case 8:
      display.setTextSize(3);
      display.setCursor(5, 10);
      display.println("Euh...");
      display.setTextSize(2);
      display.setCursor(5, 35);
      display.println("De l'eau ?");
      break;
  }
  display.display();
}

void start_animation(){
  led_control(allOn);
  delay(500);
  led_control(allOff);
  // Start animation fade-in for drink intensity led
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle ++){   
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    delay(5);
  }
}

void fadeAnimation(int totalDuration) {
  int fadeDuration = totalDuration / 2; // Durée de chaque phase de fondu
  int delayTime = 5; // Définir le délai fixe
  
  while (fadeDuration > 0) { // Tant qu'il reste du temps pour faire un fondu
    // Fade-in
    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle += 5) {
      ledcWrite(LEDC_CHANNEL, dutyCycle);
      delay(delayTime);
    }
    
    // Fade-out
    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle -= 5) {
      ledcWrite(LEDC_CHANNEL, dutyCycle);
      delay(delayTime);
    }
    
    fadeDuration -= 2 * delayTime * 51; // Soustraire le temps utilisé pour un fondu complet
  }
}

void display_intensity_bar(int drink_intensity) {
  // Check if the drink intensity has changed 
  if (abs(drink_intensity - lastDisplayedIntensity) >= 2) {
    Serial.println("Drink intensity = " + String(drink_intensity));

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(25, 2);
    display.print("Dosage");

    int barHeight = 25; // Hauteur de la barre de progression
    int barWidth = map(drink_intensity, 0, 100, 0, SCREEN_WIDTH - 2); // Largeur de la barre de progression en fonction de l'intensité de la boisson
    int borderRadius = 5; // Rayon des coins arrondis
    int borderWidth = 3; // Largeur de la bordure
    int offsetY = 10;

    // Dessine le contour de la barre de progression
    display.drawRoundRect(0, (SCREEN_HEIGHT - barHeight) / 2 + offsetY, SCREEN_WIDTH - 2, barHeight, borderRadius, SH110X_WHITE);
    display.fillRoundRect(borderWidth, (SCREEN_HEIGHT - barHeight) / 2 + borderWidth + offsetY, barWidth - 2 * borderWidth, barHeight - 2 * borderWidth, borderRadius - borderWidth, SH110X_WHITE); 
    display.display();

    if (drink_intensity <= 1) {
      display_message(8);
    } 
    if (drink_intensity >= 99) {
      display_message(7);
    }
    // Update the last displayed intensity
    lastDisplayedIntensity = drink_intensity;
  }
}

#endif
