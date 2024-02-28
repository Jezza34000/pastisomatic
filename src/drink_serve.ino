#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Screen declaration
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100
int lastDisplayedIntensity = -1;
int lastSteadyState[NUM_BUTTONS] = {HIGH,HIGH,HIGH,HIGH};
int currentState[NUM_BUTTONS];
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};

void set_pump_pwm(int pumpPin, int speed) {
  // Set L298N PWM pump
  // pumpPin (int) : EN A/B pin
  // speed (int) : Value from 0 to 255
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

void ihm_update(int led_pin, bool action_type) {
  // LED actions
  Serial.println("Set LED n" + String(led_pin) + " to state : " + String(action_type));
  if (action_type == true) {
    ledcWrite(LEDC_CHANNEL, 0);
    digitalWrite(led_pin, HIGH);
  } else {
    digitalWrite(led_pin, LOW);
  }
}

void display_message(int intensity) {
  String message;

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Service...");
  display.println(message);


  // From 0 to 100
  if (intensity == 0) {
    message = "L'eau c'est la vie !";
  } else if (intensity >= 1 && intensity <= 10) {
    message = "Tu as de la fièvre ?";
  } else if (intensity > 10 && intensity <= 25) {
    message = "Pastis d'enfant ça";
  } else if (intensity > 25 && intensity <= 45) {
    message = "Mouais";
  } else if (intensity > 45 && intensity <= 65) {
    message = "Parfait";
  } else if (intensity > 65 && intensity <= 80) {
    message = "Il est bien là";
  } else if (intensity > 80 && intensity <= 90) {
    message = "Flanby";
  } else if (intensity > 90 && intensity < 100) {
    message = "Tu as une cuillère?";
  } else if (intensity == 100) {
    message = "Plus fort... y'a pas !";
  }
  // Special message
  if (intensity == 1111) {
    message = "Y'a rien ici !";
  } else if (intensity == 2222) {
    message = "Petit joueur va !";
  } else if (intensity == 3333) {
    message = "La prochaine fois tourne plus le dosage vers la droite !";
  } else if (intensity == 9999) {
    message = "Tchin-Tchin !";
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
      valve_tank(false);
      ihm_update(LED_1, false);
      ihm_update(LED_2, false);
      ihm_update(LED_3, false);
      ihm_update(LED_4, false);
      ledcWrite(LEDC_CHANNEL, 255);
      break;
    case 1:
      // Button 1 BACK-LEFT (undefined)
      Serial.println("-> undefined");
      ihm_update(LED_1, true);
      display_message(1111);
      break;
    case 2:
      // Button 2 BACK-RIGHT (Add water)
      Serial.println("-> Add water");
      ihm_update(LED_2, true);
      valve_tank(true);
      display_message(2222);
      set_pump_pwm(WATER_PUMP, ONLY_WATER_SPEED);
      break;
    case 3:
      // Button 3 FRONT-LEFT (Add pastis)
      Serial.println("-> Add pastis");
      ihm_update(LED_3, true);
      display_message(3333);
      set_pump_pwm(ALC_PUMP, ONLY_ALC_SPEED);
      break;
    case 4:
      // Button 3 FRONT-RIGHT (Mix pastis+eau)
      Serial.println("-> Serve");
      ihm_update(LED_4, true);
      valve_tank(true);
      display_message(drink_intensity);
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
/*
    // Button breathing effect
    while(currentState[i] == LOW) {
      // increase the LED brightness
      for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle += 5){   
        ledcWrite(LEDC_CHANNEL, dutyCycle);
        delay(10);
      }

      currentState[i] = digitalRead(buttonPins[i]);
      // decrease the LED brightness
      for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle -= 5){
        ledcWrite(LEDC_CHANNEL, dutyCycle);   
        delay(10);
      }
      currentState[i] = digitalRead(buttonPins[i]);
    }
  }
*/



void setup() {
  // Init debug output
  Serial.begin(115200);
  Serial.println("Pastis-O-matic initializing...");
  
  // ESP32 Hardware init pins
  // Buttons
  pinMode(BUTTON_1, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_2, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  
  // Led ON/OFF
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);

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

  // Boot state init
  // Turn OFF all button's LED
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  digitalWrite(LED_4, LOW);
  // Turn ON potentiometer Led
  digitalWrite(POTENTIOMETER_LED, HIGH);
  
  // Configure L298N rotation (clockwise)
  // Pump does not allow anti-clockwise
  digitalWrite(WATER_PUMP_IN1, HIGH);
  digitalWrite(WATER_PUMP_IN2, LOW); 
  digitalWrite(ALC_PUMP_IN3, LOW);
  digitalWrite(ALC_PUMP_IN4, HIGH);

  // Open the valve to prime the pump
  // it's not self-priming
  valve_tank(true);

  // LCD I2C Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  display.clearDisplay();
  display.setTextSize(1);           
  display.setTextColor(SSD1306_WHITE);      
  display.setCursor(0,0); 
  display.println("Pastis-o-matic");
  display.display();

  // Start animation fade-in for drink intensity led
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle ++){   
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    delay(2);
  }

  Serial.println("Init done!");
}

void loop() {
  // Reading potentiometer value
  drink_intensity = map(analogRead(POTENTIOMETER), 0, 4095, 100, 0);

  // Check if the drink intensity has changed 
  if (drink_intensity != lastDisplayedIntensity) {
    Serial.println("Drink intensity = " + String(drink_intensity));

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 0);
    display.print("Dosage");

    int barHeight = 30; // Progressbar height
    int barWidth = map(drink_intensity, 0, 100, 0, SCREEN_WIDTH - 2);
    int borderRadius = 5; // Radius of rounded corners
    display.drawRoundRect(0, (SCREEN_HEIGHT - barHeight) / 2, SCREEN_WIDTH - 2, barHeight, borderRadius, SSD1306_WHITE);

    // Fill-up progessbar with drink intensity value
    display.fillRoundRect(1, (SCREEN_HEIGHT - barHeight) / 2 + 1, barWidth, barHeight - 2, borderRadius, SSD1306_WHITE); 
    display.display();

    // Update the last displayed intensity
    lastDisplayedIntensity = drink_intensity;
  }

  handlebuttons();

  // Sleep
  delay(10);
}