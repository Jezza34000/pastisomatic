#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Screen declaration
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100
int lastDisplayedIntensity = -1;
int lastSteadyState[NUM_BUTTONS] = {HIGH,HIGH,HIGH,HIGH};
int currentState[NUM_BUTTONS];
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};

const int led_mapping[NUM_LEDS] = {LED_1, LED_2, LED_3, LED_4};

const bool allOff[NUM_LEDS] = {false, false, false, false};
const bool allOn[NUM_LEDS] = {true, true, true, true};
const bool led_1[NUM_LEDS] = {true, false, false, false};
const bool led_2[NUM_LEDS] = {false, true, false, false};
const bool led_3[NUM_LEDS] = {false, false, true, false};
const bool led_4[NUM_LEDS] = {false, false, false, true};

void set_pump_pwm(int pumpPin, int speed) {
  // Set L298N PWM pump
  // pumpPin (int) : EN A/B pin
  // speed (int) : Value from 0 to 255

  if (pumpPin == WATER_PUMP && speed > 0) {
    valve_tank(true);
  } else {
    valve_tank(false);
  }

  Serial.println("Set pump PWM " + String(pumpPin) + " at : " + String(speed));
  analogWrite(pumpPin, speed);
}

void valve_tank(bool state) {
  // Open Close Water tank valve.
  // state (bool) : True = Opened / False = Closed
  Serial.println("Set tank's valve state : " + String(state));
  if (state == true) {
    digitalWrite(TANK_VALVE_RELAY, HIGH);
  } else {
    digitalWrite(TANK_VALVE_RELAY, LOW);
  }
}

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

void display_message(int message) {

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  switch(message) {
    case 0:
      display.setTextSize(2);
      display.setCursor(15, 25);
      display.println("Pret !");
      display.display(); 
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
      display.display();
      break;
    case 3:
      display.setTextSize(2);
      display.setCursor(15, 10);
      display.println("Ajout...");
      display.setCursor(15, 25);
      display.setTextSize(3);
      display.println("Pastis");
      display.display();
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

void process_actions(int choice) {
  // Actions process
  // Pump & Valve
  Serial.println(">>> Action :");

  switch(choice) {
    case 0:
      // Arrêt
      Serial.println("-> Stop");
      set_pump_pwm(ALC_PUMP, 0);
      set_pump_pwm(WATER_PUMP, 0);   
      led_control(allOff);
      ledcWrite(LEDC_CHANNEL, 255);
      display_message(1);
      break;

    case 1:
      // Button 1 BACK-LEFT (Start&Clean)
      Serial.println("-> Start&Clean");
      led_control(led_1);
      display_message(5);
      set_pump_pwm(WATER_PUMP, 255);
      fadeAnimation(WATER_START_DURATION);
      set_pump_pwm(WATER_PUMP, 0);
      set_pump_pwm(ALC_PUMP, 255);
      display_message(6);
      fadeAnimation(ALC_START_DURATION);
      set_pump_pwm(ALC_PUMP, 0);
      display_message(0);
      break;

    case 2:
      // Button 2 BACK-RIGHT (Add water)
      Serial.println("-> Add water");
      led_control(led_2);
      display_message(2);
      // set_pump_pwm(WATER_PUMP, ONLY_WATER_SPEED);
      set_pump_pwm(WATER_PUMP, map(drink_intensity, 0, 100, WATER_PUMP_SPEED_MIN_PWM, WATER_PUMP_SPEED_MAX_PWM));
      break;

    case 3:
      // Button 3 FRONT-LEFT (Add pastis)
      Serial.println("-> Add pastis");
      led_control(led_3);
      display_message(3);
      // set_pump_pwm(ALC_PUMP, ONLY_ALC_SPEED);
      set_pump_pwm(ALC_PUMP, map(drink_intensity, 0, 100, ALC_PUMP_SPEED_MIN_PWM, ALC_PUMP_SPEED_MAX_PWM));
      break;

    case 4:
      // Button 3 FRONT-RIGHT (Mix pastis+eau)
      Serial.println("-> Serve");
      led_control(led_4);
      display_message(4);
      set_pump_pwm(ALC_PUMP, map(drink_intensity, 0, 100, ALC_PUMP_SPEED_MIN_PWM, ALC_PUMP_SPEED_MAX_PWM));
      set_pump_pwm(WATER_PUMP, map(drink_intensity, 0, 100, WATER_PUMP_SPEED_MIN_PWM, WATER_PUMP_SPEED_MAX_PWM));
      break;

    default:
      Serial.println("processAction not found :" + String(choice));
      break;
  }
}

void handlebuttons() {
  // Handle buttons
  int buttonPins[NUM_BUTTONS] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    currentState[i] = digitalRead(buttonPins[i]);

    if (currentState[i] != lastSteadyState[i]) {
      Serial.print("currentState[i]=" + String(currentState[i]) + " lastSteadyState[i]=" + String(lastSteadyState[i]) );
      if (currentState[i] == LOW) { // LOW is button pressed
        Serial.print("Button " + String(i) + " is pressed - ");
        process_actions(i + 1);
      } else {
        Serial.print("Button " + String(i) + " released\r\n");
        process_actions(0);
      }
      lastSteadyState[i] = currentState[i];
    }
  }
}

void setup() {
  // Init debug output
  Serial.begin(115200);
  Serial.println("Pastis-O-matic initializing...");

  // LCD OLED I2C Init
  display.begin(SCREEN_ADDRESS, true);

  display.clearDisplay();
  display.setTextSize(1); 
  display.setTextColor(SH110X_WHITE);             
  display.setCursor(0,0); 
  display.println("Initializing...");
  display.display();
  
  // ESP32 Hardware init pins
  // Buttons
  pinMode(BUTTON_1, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_2, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  
  // Led ON/OFF
  for (int i = 0; i < NUM_LEDS; ++i) {
      pinMode(led_mapping[i], OUTPUT);
  }

  // Led PWM
  ledcSetup(LEDC_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(POTENTIOMETER_LED, LEDC_CHANNEL);
  
  // L298N Pump PWM output
  pinMode(TANK_VALVE_RELAY, OUTPUT);
  pinMode(ALC_PUMP, OUTPUT);
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(WATER_PUMP_IN1, OUTPUT);
  pinMode(WATER_PUMP_IN2, OUTPUT);
  pinMode(ALC_PUMP_IN3, OUTPUT);
  pinMode(ALC_PUMP_IN4, OUTPUT);
  
  // Configure L298N rotation (clockwise)
  // Pump does not allow anti-clockwise
  digitalWrite(WATER_PUMP_IN1, HIGH);
  digitalWrite(WATER_PUMP_IN2, LOW); 
  digitalWrite(ALC_PUMP_IN3, LOW);
  digitalWrite(ALC_PUMP_IN4, HIGH);

  led_control(allOn);
  delay(500);
  led_control(allOff);
  // Start animation fade-in for drink intensity led
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle ++){   
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    delay(5);
  }

  Serial.println("Init done!");
}

void loop() {
  // Reading potentiometer value
  drink_intensity = map(analogRead(POTENTIOMETER), 0, 4095, 100, 0);

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

  handlebuttons();

  // Sleep
  delay(10);
}