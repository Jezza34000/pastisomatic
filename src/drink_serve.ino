#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "ihm.h"

WebServer server(80);

// Load preferences
Preferences preferences;

// Screen declaration
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100

int lastSteadyState[NUM_BUTTONS] = {HIGH,HIGH,HIGH,HIGH};
int currentState[NUM_BUTTONS];
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};

int alcPumpSpeedMinPWM;
int alcPumpSpeedMaxPWM;
int waterPumpSpeedMinPWM;
int waterPumpSpeedMaxPWM;

int onlyWaterSpeed;
int onlyAlcSpeed;

int waterStartDuration;
int alcStartDuration;
int alcoholPUMPfreq;
int waterPUMPfreq;


void set_pump_pwm(int motor_channel, int speed) {
  // Set L298N PWM pump
  // pumpPin (int) : EN A/B pin
  // speed (int) : Value from 0 to 255

  if (motor_channel == WATER_PUMP && speed > 0) {
    valve_tank(true);
  } else {
    valve_tank(false);
  }

  Serial.println("Set pump PWM " + String(motor_channel) + " at : " + String(speed));
  ledcWrite(motor_channel, speed);
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
      fadeAnimation(waterStartDuration);
      set_pump_pwm(WATER_PUMP, 0);
      set_pump_pwm(ALC_PUMP, 255);
      display_message(6);
      fadeAnimation(alcStartDuration);
      set_pump_pwm(ALC_PUMP, 0);
      display_message(0);
      break;

    case 2:
      // Button 2 BACK-RIGHT (Add water)
      Serial.println("-> Add water");
      led_control(led_2);
      display_message(2);
      set_pump_pwm(WATER_PUMP, onlyWaterSpeed);
      break;

    case 3:
      // Button 3 FRONT-LEFT (Add pastis)
      Serial.println("-> Add pastis");
      led_control(led_3);
      display_message(3);
      set_pump_pwm(ALC_PUMP, onlyAlcSpeed);
      break;

    case 4:
      // Button 3 FRONT-RIGHT (Mix pastis+eau)
      Serial.println("-> Serve");
      led_control(led_4);
      display_message(4);
      set_pump_pwm(ALC_PUMP, map(drink_intensity, 0, 100, alcPumpSpeedMinPWM, alcPumpSpeedMaxPWM));
      set_pump_pwm(WATER_PUMP, map(drink_intensity, 0, 100, waterPumpSpeedMinPWM, waterPumpSpeedMaxPWM));
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

  WiFi.softAP("PastisOMatic", NULL);
  display.println("- Wifi OK");
  display.display();

  // Ouvrir l'espace de stockage de préférences avec l'identifiant "settings"
  preferences.begin("settings", false); // false indique de ne pas supprimer les préférences existantes

  alcPumpSpeedMinPWM = preferences.getInt("APS_MIN_PWM", ALC_PUMP_SPEED_MIN_PWM);
  alcPumpSpeedMaxPWM = preferences.getInt("APS_MAX_PWM", ALC_PUMP_SPEED_MAX_PWM);
  waterPumpSpeedMinPWM = preferences.getInt("WPS_MIN_PWM", WATER_PUMP_SPEED_MIN_PWM);
  waterPumpSpeedMaxPWM = preferences.getInt("WPS_MAX_PWM", WATER_PUMP_SPEED_MAX_PWM);

  onlyWaterSpeed = preferences.getInt("ONLY_WTR_SPEED", ONLY_WATER_SPEED);
  onlyAlcSpeed = preferences.getInt("ONLY_ALC_SPEED", ONLY_ALC_SPEED);

  waterStartDuration = preferences.getInt("W_STRT_DUR", WATER_START_DURATION);
  alcStartDuration = preferences.getInt("A_STRT_DUR", ALC_START_DURATION);

  alcoholPUMPfreq = preferences.getInt("ALC_PWM_FREQ", ALC_PUMP_PWM_FREQ);
  waterPUMPfreq = preferences.getInt("WATER_PWM_FREQ", WATER_PUMP_PWM_FREQ);

  display.println("- Param loaded");
  display.display();

  preferences.end();
  
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

  // Water pumpu PWM
  ledcSetup(WATER_PUMP, waterPUMPfreq, WATER_PUMP_PWM_RESOLUTION);
  ledcAttachPin(WATER_PUMP_PIN, WATER_PUMP);

  // Alcohol pumpu PWM
  ledcSetup(ALC_PUMP, alcoholPUMPfreq, ALC_PUMP_PWM_RESOLUTION);
  ledcAttachPin(ALC_PUMP_PIN, ALC_PUMP);

  pinMode(TANK_VALVE_RELAY, OUTPUT);

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

  display.println("- Pin configured");
  display.display();

  // Routes pour les pages web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", HTTP_POST, handleReboot);

  // Démarrer le serveur
  server.begin();

  display.println("- Webserver started");
  display.display();

  start_animation();
  Serial.println("Init done!");
}

void loop() {
  // Reading potentiometer value
  drink_intensity = map(analogRead(POTENTIOMETER), 0, 4095, 100, 0);
  display_intensity_bar(drink_intensity);
  handlebuttons();
  server.handleClient();

  // Sleep
  delay(10);
}

void handleRoot() {
  String webpage = "<!DOCTYPE html>";
  webpage += "<html><head><title>Paramètres Pastis-O-matic</title>";
  webpage += "<style>#saveButton {font-size: 20px;}</style>";
  webpage += "</head><body>";
  webpage += "<h1>Parametres Pastis-O-matic</h1>";
  webpage += "<form action='/save' method='POST'>";
  
  webpage += "<hr><h2>Alcohol pump:</h2>";
  webpage += "<label for='alcPumpSpeedMinPWM'>Min :</label>";
  webpage += "<output id='alcPumpMinValue'>" + String(alcPumpSpeedMinPWM) + "</output>";
  webpage += "<input type='range' id='alcPumpSpeedMinPWM' name='alcPumpSpeedMinPWM' min='0' max='255' value='" + String(alcPumpSpeedMinPWM) + "' oninput='alcPumpMinValue.value = this.value'><br>";
  webpage += "<label for='alcPumpSpeedMaxPWM'>Max :</label>";
  webpage += "<output id='alcPumpMaxValue'>" + String(alcPumpSpeedMaxPWM) + "</output>";
  webpage += "<input type='range' id='alcPumpSpeedMaxPWM' name='alcPumpSpeedMaxPWM' min='0' max='255' value='" + String(alcPumpSpeedMaxPWM) + "' oninput='alcPumpMaxValue.value = this.value'><br>";
  webpage += "<label for='alcoholPUMPfreq'>Frequency :</label>";
  webpage += "<output id='alcPumpFrequencyValue'>" + String(alcoholPUMPfreq) + "</output>";
  webpage += "<input type='range' id='alcoholPUMPfreq' name='alcoholPUMPfreq' min='10' max='10000' step='10' value='" + String(alcoholPUMPfreq) + "' oninput='alcPumpFrequencyValue.value = this.value'> (reboot required)<br>";
  webpage += "<label for='onlyAlcSpeed'>Speed single func :</label>";
  webpage += "<output id='onlyAlcSpeedValue'>" + String(onlyAlcSpeed) + "</output>";
  webpage += "<input type='range' id='onlyAlcSpeed' name='onlyAlcSpeed' min='0' max='255' step='10' value='" + String(onlyAlcSpeed) + "' oninput='onlyAlcSpeedValue.value = this.value'><br>";

  webpage += "<hr><h2>Water pump:</h2>";
  webpage += "<label for='waterPumpSpeedMinPWM'>Min :</label>";
  webpage += "<output id='waterPumpMinValue'>" + String(waterPumpSpeedMinPWM) + "</output>";
  webpage += "<input type='range' id='waterPumpSpeedMinPWM' name='waterPumpSpeedMinPWM' min='0' max='255' value='" + String(waterPumpSpeedMinPWM) + "' oninput='waterPumpMinValue.value = this.value'><br>";
  webpage += "<label for='waterPumpSpeedMaxPWM'>Max :</label>";
  webpage += "<output id='waterPumpMaxValue'>" + String(waterPumpSpeedMaxPWM) + "</output>";
  webpage += "<input type='range' id='waterPumpSpeedMaxPWM' name='waterPumpSpeedMaxPWM' min='0' max='255' value='" + String(waterPumpSpeedMaxPWM) + "' oninput='waterPumpMaxValue.value = this.value'><br>";
  webpage += "<label for='waterPUMPfreq'>Frequency :</label>";
  webpage += "<output id='waterPumpFrequencyValue'>" + String(waterPUMPfreq) + "</output>";
  webpage += "<input type='range' id='waterPUMPfreq' name='waterPUMPfreq' min='10' max='10000' step='10' value='" + String(waterPUMPfreq) + "' oninput='waterPumpFrequencyValue.value = this.value'> (reboot required)<br>";
  webpage += "<label for='onlyWaterSpeed'>Speed single func :</label>";
  webpage += "<output id='onlyWaterSpeedValue'>" + String(onlyWaterSpeed) + "</output>";
  webpage += "<input type='range' id='onlyWaterSpeed' name='onlyWaterSpeed' min='0' max='255' step='10' value='" + String(onlyWaterSpeed) + "' oninput='onlyWaterSpeedValue.value = this.value'><br>";
  
  webpage += "<hr><h2>Start/Clean:</h2>";
  webpage += "<label for='waterStartDuration'>Water time :</label>";
  webpage += "<output id='waterStartDurationValue'>" + String(waterStartDuration) + "</output>";
  webpage += "<input type='range' id='waterStartDuration' name='waterStartDuration' min='0' max='10000' step='500' value='" + String(waterStartDuration) + "' oninput='waterStartDurationValue.value = this.value'><br>";
  webpage += "<label for='alcStartDuration'>Alcohol time :</label>";
  webpage += "<output id='alcStartDurationValue'>" + String(alcStartDuration) + "</output>";
  webpage += "<input type='range' id='alcStartDuration' name='alcStartDuration' min='0' max='10000' step='500' value='" + String(alcStartDuration) + "' oninput='alcStartDurationValue.value = this.value'><br>";

  webpage += "<hr><input id='saveButton' type='submit' value='Save'>";
  webpage += "<hr></form>";
  webpage += "<form action='/reboot' method='POST'>";
  webpage += "<input type='submit' value='Reboot'>";
  webpage += "</form><hr></body></html>";

  server.send(200, "text/html", webpage);
}

// Gestion de l'enregistrement
void handleSave() {
  if (server.args() > 0) {
    // Ouvrir l'espace de stockage de préférences avec l'identifiant "settings"
    preferences.begin("settings", false); // false indique de ne pas supprimer les préférences existantes

    // Mettre à jour les valeurs des paramètres
    alcPumpSpeedMinPWM = server.arg("alcPumpSpeedMinPWM").toInt();
    alcPumpSpeedMaxPWM = server.arg("alcPumpSpeedMaxPWM").toInt();
    waterPumpSpeedMinPWM = server.arg("waterPumpSpeedMinPWM").toInt();
    waterPumpSpeedMaxPWM = server.arg("waterPumpSpeedMaxPWM").toInt();
    alcoholPUMPfreq = server.arg("alcoholPUMPfreq").toInt();
    waterPUMPfreq = server.arg("waterPUMPfreq").toInt();
    onlyAlcSpeed = server.arg("onlyAlcSpeed").toInt();
    onlyWaterSpeed = server.arg("onlyWaterSpeed").toInt();

    waterStartDuration = server.arg("waterStartDuration").toInt();
    alcStartDuration = server.arg("alcStartDuration").toInt();

    // Enregistrer les nouvelles valeurs dans les préférences
    preferences.putInt("APS_MIN_PWM", alcPumpSpeedMinPWM);
    preferences.putInt("APS_MAX_PWM", alcPumpSpeedMaxPWM);
    preferences.putInt("WPS_MIN_PWM", waterPumpSpeedMinPWM);
    preferences.putInt("WPS_MAX_PWM", waterPumpSpeedMaxPWM);

    preferences.putInt("ALC_PWM_FREQ", alcoholPUMPfreq);
    preferences.putInt("WATER_PWM_FREQ", waterPUMPfreq);

    preferences.putInt("ONLY_ALC_SPEED", onlyAlcSpeed);
    preferences.putInt("ONLY_WTR_SPEED", onlyAlcSpeed);

    preferences.putInt("W_STRT_DUR", waterStartDuration);
    preferences.putInt("A_STRT_DUR", alcStartDuration);

    // Fermer l'espace de stockage de préférences
    preferences.end();

    // Rediriger vers la page d'accueil après l'enregistrement
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleReboot() {
  ESP.restart();
}


