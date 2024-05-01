#include <Wire.h>
#include <Preferences.h>
#include "config.h"
#include "ihm.h"
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// Load preferences
Preferences preferences;

// Screen declaration
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100

// Slow PWM
unsigned long previousMillis = 0;
bool pinState = false;
bool soft_pwm_ison = false;
float dutyCycle = 0;
int hard_pwm_value = 0;


// Ajoutez cette variable globale
bool lastButtonState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH};

int alcPumpSpeedMinPWM;
int alcPumpSpeedMaxPWM;
int waterPumpSpeedMinPWM;
int waterPumpSpeedMaxPWM;

int onlyWaterSpeed;
int onlyAlcSpeed;

int alcoholSoftMinPWM;
int alcoholSoftMaxPWM;

int waterStartDuration;
int alcStartDuration;
int alcoholPUMPfreq;
int waterPUMPfreq;

bool buttonPressed = false;
static bool noButtonWasPressed = false;

volatile int pumpSpeed = 0;
volatile bool pumpState = false;

IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,254);
IPAddress subnet(255,255,255,0);

hw_timer_t * timer = NULL;


struct Button {
    const uint8_t PIN;
    bool pressed;
};

Button buttons[] = {
    {BUTTON_1, false},
    {BUTTON_2, false},
    {BUTTON_3, false},
    {BUTTON_4, false}
};

void IRAM_ATTR updateButtonState(Button &button) {
    ets_printf("Interrup on button n°%d state is=%d\n", button.PIN, (digitalRead(button.PIN)));
    button.pressed = !static_cast<bool>(digitalRead(button.PIN));
}

void IRAM_ATTR fnc_btn1_start_clean() {
    updateButtonState(buttons[0]);
}

void IRAM_ATTR fnc_btn2_add_water() {
    updateButtonState(buttons[1]);
}

void IRAM_ATTR fnc_btn3_add_pastis() {
    updateButtonState(buttons[2]);
}

void IRAM_ATTR fnc_btn4_serve() {
    updateButtonState(buttons[3]);
}

void IRAM_ATTR onTimer() {
  if (pumpSpeed > 0 && pumpState) {
    ledcWrite(ALC_PUMP, pumpSpeed);
  } else {
    ledcWrite(ALC_PUMP, 0);
  }
  pumpState = !pumpState;
}

void set_pump_pwm_timer(int value) {
  pumpSpeed = value;
  if (value > 0) {
    timerAlarmEnable(timer);
  } else {
    timerAlarmDisable(timer);
    ledcWrite(ALC_PUMP, 0);
    pumpState = false;
  }
}

void set_pump_pwm(int motor_channel, int speed) {
  // Set L298N PWM pump
  // pumpPin (int) : EN A/B pin
  // speed (int) : Value from 0 to 255

  if (motor_channel == WATER_PUMP){
    if (speed > 0) {
      valve_tank(true);
    } else {
      valve_tank(false);
    }
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

void process_actions(int btn_number) {
  // Actions process
  // Pump & Valve
  Serial.println(">>> Action :");

  switch(btn_number) {
    case 0:
      // Arrêt
      Serial.println("-> Stop");
      set_soft_pump_pwm(false, 0, 0);
      set_pump_pwm(WATER_PUMP, 0);
      set_pump_pwm(ALC_PUMP, 0); 
      set_pump_pwm_timer(0); 
      led_control(allOff);
      ledcWrite(LEDC_CHANNEL, 255);
      display_message(1);
      break;

    case BUTTON_1:
      // Button 1 BACK-LEFT (Start&Clean)
      Serial.println("-> Start&Clean");
      soft_pwm_ison = true;
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

    case BUTTON_2:
      // Button 2 BACK-RIGHT (Add water)
      Serial.println("-> Add water");
      led_control(led_2);
      display_message(2);
      set_pump_pwm(WATER_PUMP, onlyWaterSpeed);
      break;

    case BUTTON_3:
      // Button 3 FRONT-LEFT (Add pastis)
      Serial.println("-> Add pastis");
      led_control(led_3);
      display_message(3);
      //set_soft_pump_pwm(true, drink_intensity, ONLY_ALC_SPEED);
      set_pump_pwm(ALC_PUMP, onlyAlcSpeed); 
      break;

    case BUTTON_4:
      // Button 4 FRONT-RIGHT (Mix pastis+eau)
      Serial.println("-> Serve");
      soft_pwm_ison = true;
      led_control(led_4);
      display_message(4);
      set_soft_pump_pwm(true, drink_intensity, map(drink_intensity, 0, 100, alcPumpSpeedMinPWM, alcPumpSpeedMaxPWM));
      set_pump_pwm(WATER_PUMP, map(drink_intensity, 0, 100, waterPumpSpeedMinPWM, waterPumpSpeedMaxPWM));
      break;

    default:
      Serial.println("processAction not found :" + String(btn_number));
      break;
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

  // Wifi
/*
  NOT WORKING

  WiFi.disconnect(true);             // that no old information is stored  
  WiFi.mode(WIFI_OFF);               // switch WiFi off  
  delay(1000);                       // short wait to ensure WIFI_OFF  
  WiFi.persistent(false);            // avoid that WiFi-parameters will be stored in persistent memory    
  WiFi.softAPConfig(local_ip, gateway, subnet);   // configure AP with fixed IP-address   
  delay(1000);     
  WiFi.mode(WIFI_AP);                    //Access Point Only  
  delay(1000);  
  WiFi.persistent(false);                // avoid that congifuration is written to persistent memory  
  WiFi.softAP("PastisOMatic", NULL);   // Start ESP as Access-Point  


  display.println("- Wifi init...");
  display.display();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("PastisOMatic", NULL);
  delay(2000); 
  WiFi.setTxPower(WIFI_POWER_5dBm);
  WiFi.softAPConfig(local_ip, gateway, subnet);


  delay(500);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  display.println("- Wifi OK");
  display.display();

*/
  
  // Wifi
  // Connect to your wi-fi modem
  display.println("- Wifi connecting...");
  display.display();
  WiFi.begin("Fbx-2.4", "fbxkey-FLHJOk8nUARi9t5TGVdlGKCC4TY49eXZfoBjf1Q31vrVdKK20ShaahgO");

  unsigned long startTime = millis();
  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 30000) {
      Serial.println("Failed to connect to WiFi after 30 seconds");
      break;
    }
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.println("- Wifi connected OK !");
    display.display();
    Serial.println("");
    Serial.println("WiFi connected successfully");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());  //Show ESP32 IP on serial
  } else {
    display.println("- Wifi connection failed");
    display.display();
    Serial.println("WiFi connection failed");
  }

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

  alcoholSoftMinPWM = preferences.getInt("SFT_PW_MIN", ALC_SOFT_PWM_MIN_DUTY);
  alcoholSoftMaxPWM = preferences.getInt("SFT_PW_MAX", ALC_SOFT_PWM_MAX_DUTY);

  display.println("- Param loaded");
  display.display();

  preferences.end();
  
  // ESP32 Hardware init pins
  // Buttons
  pinMode(BUTTON_1, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_2, INPUT); // Physical 10K pull-up resistor
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);

  // Interrupts buttons
  attachInterrupt(BUTTON_1, fnc_btn1_start_clean, CHANGE);
  attachInterrupt(BUTTON_2, fnc_btn2_add_water, CHANGE);
  attachInterrupt(BUTTON_3, fnc_btn3_add_pastis, CHANGE);
  attachInterrupt(BUTTON_4, fnc_btn4_serve, CHANGE);
  
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

  // Timer  

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 500000, true); // 0.5 second

  // Routes pour les pages web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", HTTP_GET, handleReboot);

  // Démarrer le serveur
  server.begin();

  display.println("- Webserver started");
  display.display();

  start_animation();
  Serial.println("Init done!");
}

void soft_pwm() {
  unsigned long currentMillis = millis();
  if (!soft_pwm_ison) {
    return;
  }

  if (pinState == false && currentMillis - previousMillis >= (1 - dutyCycle) * 1000) {
    previousMillis = currentMillis;
    pinState = !pinState;
    set_pump_pwm(ALC_PUMP, hard_pwm_value);
  } else if (pinState == true && currentMillis - previousMillis >= dutyCycle * 1000) {
    previousMillis = currentMillis;
    pinState = !pinState;
    set_pump_pwm(ALC_PUMP, 0);
  }
}

void set_soft_pump_pwm(bool state, int soft_pwm, int hard_pwm) {
  if (state == true) {
    soft_pwm_ison = true;
    hard_pwm_value = hard_pwm;
    dutyCycle = (float)map(soft_pwm, 0, 100, alcoholSoftMinPWM, alcoholSoftMaxPWM) / 100;
    Serial.println("dutyCycle : " + String(dutyCycle) );
  } else {
    soft_pwm_ison = false;
    dutyCycle = 0;
    hard_pwm_value = 0;
  }
  
}

void loop() {
  // Reading potentiometer value
  drink_intensity = map(analogRead(POTENTIOMETER), 0, 4095, 100, 0);
  display_intensity_bar(drink_intensity);
  server.handleClient();

  // Check buttons state
  for (int i = 0; i < sizeof(buttons) / sizeof(Button); i++) {
    bool currentButtonState = buttons[i].pressed;

    if (currentButtonState != lastButtonState[i]) {
      if (currentButtonState) {
        // Button is pressed
        process_actions(buttons[i].PIN);
        Serial.println("Pressed !!");
      } else {
        // Button is released
        process_actions(0);
        Serial.println("NOT pressed");
      }
    }
    lastButtonState[i] = currentButtonState;
  }
  soft_pwm();
}


void handleRoot() {
  String webpage = "<!DOCTYPE html>";
  webpage += "<html><head><title>Parametres Pastis-O-matic</title>";
  webpage += "<style>#saveButton {font-size: 20px;}</style>";
  webpage += "</head><body>";
  webpage += "<h1>Parametres Pastis-O-matic</h1>";
  webpage += "<form action='/save' method='POST'>";

  webpage += "<hr><h2>Alcohol pump:</h2>";

  webpage += "<label for='alcoholSoftMinPWM'>Soft PWM Min:</label>";
  webpage += "<output id='alcSoftMinPWM'>" + String(alcoholSoftMinPWM) + "</output>";
  webpage += "<input type='range' id='alcoholSoftMinPWM' name='alcoholSoftMinPWM' min='0' max='100' value='" + String(alcoholSoftMinPWM) + "' oninput='alcSoftMinPWM.value = this.value'><br>";

  webpage += "<label for='alcoholSoftMaxPWM'>Soft PWM Max:</label>";
  webpage += "<output id='alcSoftMaxPWM'>" + String(alcoholSoftMaxPWM) + "</output>";
  webpage += "<input type='range' id='alcoholSoftMaxPWM' name='alcoholSoftMaxPWM' min='0' max='100' value='" + String(alcoholSoftMaxPWM) + "' oninput='alcSoftMaxPWM.value = this.value'><br>";

  webpage += "<label for='alcPumpSpeedMinPWM'>Hard PWM Min :</label>";
  webpage += "<output id='alcPumpMinValue'>" + String(alcPumpSpeedMinPWM) + "</output>";
  webpage += "<input type='range' id='alcPumpSpeedMinPWM' name='alcPumpSpeedMinPWM' min='0' max='255' value='" + String(alcPumpSpeedMinPWM) + "' oninput='alcPumpMinValue.value = this.value'><br>";

  webpage += "<label for='alcPumpSpeedMaxPWM'>Hard PWM Max :</label>";
  webpage += "<output id='alcPumpMaxValue'>" + String(alcPumpSpeedMaxPWM) + "</output>";
  webpage += "<input type='range' id='alcPumpSpeedMaxPWM' name='alcPumpSpeedMaxPWM' min='0' max='255' value='" + String(alcPumpSpeedMaxPWM) + "' oninput='alcPumpMaxValue.value = this.value'><br>";

  webpage += "<label for='alcoholPUMPfreq'>Frequency :</label>";
  webpage += "<output id='alcPumpFrequencyValue'>" + String(alcoholPUMPfreq) + "</output>";
  webpage += "<input type='range' id='alcoholPUMPfreq' name='alcoholPUMPfreq' min='10' max='10000' step='10' value='" + String(alcoholPUMPfreq) + "' oninput='alcPumpFrequencyValue.value = this.value'> (reboot required)<br>";
  webpage += "<label for='onlyAlcSpeed'>Speed single func :</label>";
  webpage += "<output id='onlyAlcSpeedValue'>" + String(onlyAlcSpeed) + "</output>";
  webpage += "<input type='range' id='onlyAlcSpeed' name='onlyAlcSpeed' min='0' max='255' step='1' value='" + String(onlyAlcSpeed) + "' oninput='onlyAlcSpeedValue.value = this.value'><br>";

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
  webpage += "<input type='range' id='onlyWaterSpeed' name='onlyWaterSpeed' min='0' max='255' step='1' value='" + String(onlyWaterSpeed) + "' oninput='onlyWaterSpeedValue.value = this.value'><br>";
  
  webpage += "<hr><h2>Start/Clean:</h2>";
  webpage += "<label for='waterStartDuration'>Water time :</label>";
  webpage += "<output id='waterStartDurationValue'>" + String(waterStartDuration) + "</output>";
  webpage += "<input type='range' id='waterStartDuration' name='waterStartDuration' min='0' max='10000' step='500' value='" + String(waterStartDuration) + "' oninput='waterStartDurationValue.value = this.value'><br>";
  webpage += "<label for='alcStartDuration'>Alcohol time :</label>";
  webpage += "<output id='alcStartDurationValue'>" + String(alcStartDuration) + "</output>";
  webpage += "<input type='range' id='alcStartDuration' name='alcStartDuration' min='0' max='10000' step='500' value='" + String(alcStartDuration) + "' oninput='alcStartDurationValue.value = this.value'><br>";

  webpage += "<hr><input id='saveButton' type='submit' value='Save'>";
  webpage += "<hr></form>";
  webpage += "<form action='/reboot' method='GET'>";
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

    alcoholSoftMinPWM = server.arg("alcoholSoftMinPWM").toInt();
    alcoholSoftMaxPWM = server.arg("alcoholSoftMaxPWM").toInt();

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
    preferences.putInt("ONLY_WTR_SPEED", onlyWaterSpeed);

    preferences.putInt("W_STRT_DUR", waterStartDuration);
    preferences.putInt("A_STRT_DUR", alcStartDuration);

    preferences.putInt("SFT_PW_MIN", alcoholSoftMinPWM);
    preferences.putInt("SFT_PW_MAX", alcoholSoftMaxPWM);

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
