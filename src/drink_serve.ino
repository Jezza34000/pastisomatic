#include <Wire.h>
#include "config.h"
#include "ihm.h"

// Screen declaration
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100

int lastSteadyState[NUM_BUTTONS] = {HIGH,HIGH,HIGH,HIGH};
int currentState[NUM_BUTTONS];
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};

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

void process_actions(int choice) {
  // Actions process
  // Pump & Valve
  Serial.println(">>> Action :");

  switch(choice) {
    case 0:
      // Arrêt
      Serial.println("-> Stop");
      //set_pump_pwm(ALC_PUMP, 0);
      set_pump_pwm(WATER_PUMP, 0);   
      led_control(allOff);
      ledcWrite(LEDC_CHANNEL, 255);
      ledcWrite(MOTORC_CHANNEL, 0);
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
      // set_pump_pwm(ALC_PUMP, map(drink_intensity, 0, 100, ALC_PUMP_SPEED_MIN_PWM, ALC_PUMP_SPEED_MAX_PWM));

      ledcWrite(MOTORC_CHANNEL, map(drink_intensity, 0, 100, 0, 255));
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(5, 10);
      display.println(map(drink_intensity, 0, 100, 0, 255));
      display.display();
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
  ledcSetup(LEDC_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttachPin(POTENTIOMETER_LED, LEDC_CHANNEL);

  // Motor PWM
  ledcSetup(MOTORC_CHANNEL, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
  ledcAttachPin(ALC_PUMP, MOTORC_CHANNEL);
  
  // L298N Pump PWM output
  pinMode(TANK_VALVE_RELAY, OUTPUT);
  // pinMode(ALC_PUMP, OUTPUT);
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

  start_animation();
  Serial.println("Init done!");
}

void loop() {
  // Reading potentiometer value
  drink_intensity = map(analogRead(POTENTIOMETER), 0, 4095, 100, 0);
  display_intensity_bar(drink_intensity);
  handlebuttons();

  // Sleep
  delay(10);
}