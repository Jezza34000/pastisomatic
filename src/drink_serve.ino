#include <Wire.h>
#include <Preferences.h>
#include "config.h"
#include "ihm.h"
#include "BluetoothSerial.h"

// Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
String message = "";

// Screen declaration
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global var
int drink_intensity = 0; // % from 0 to 100
volatile int pumpSpeed = 0;

// Software PWM
unsigned long previousMillis = 0;
bool pinState = false;
bool soft_pwm_ison = false;
float dutyCycle = 0;
int hard_pwm_value = 0;

// Load preferences
Preferences preferences;

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

// Button state
bool lastButtonState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH};
bool buttonPressed = false;
static bool noButtonWasPressed = false;

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

// Interrupts functions

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
      led_control(allOff);
      ledcWrite(LEDC_CHANNEL, 255);
      display_message(1);
      break;

    case BUTTON_1:
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
      set_soft_pump_pwm(true, drink_intensity, map(drink_intensity, 0, 100, alcPumpSpeedMinPWM, alcPumpSpeedMaxPWM));
      break;

    case BUTTON_4:
      // Button 4 FRONT-RIGHT (Mix pastis+eau)
      Serial.println("-> Serve");
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

  // Preferences init & load
  preferences.begin("settings", false);

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

  // Water pump PWM
  ledcSetup(WATER_PUMP, waterPUMPfreq, WATER_PUMP_PWM_RESOLUTION);
  ledcAttachPin(WATER_PUMP_PIN, WATER_PUMP);

  // Alcohol pump PWM
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

  start_animation();
  Serial.println("Init done!");

  // Read the state of the buttons
  bool button1State = digitalRead(BUTTON_2);
  bool button2State = digitalRead(BUTTON_4);

  // Check if both buttons are pressed
  if (button1State == LOW && button2State == LOW) {
    // Bluetooth init
    SerialBT.begin("PastisOmatic"); //Bluetooth device name
    display.println("-> Bluetooth OK");
    display.display();
    delay(1000);
  }


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
    // Serial.println("soft_pwm=" + String(soft_pwm) + " hard_pwm=" + String(hard_pwm) + " dutyCycle=" + String(dutyCycle) );
  } else {
    soft_pwm_ison = false;
    dutyCycle = 0;
    hard_pwm_value = 0;
  }
  
}  

void bluetooth_gui() {
  if (SerialBT.available()){
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      message += String(incomingChar);
    } else {
      process_command(message);
      message = "";
    }
  }
  ;
}

void split_string(const String& str, String& part1, String& part2, String& part3) {
    int space1 = str.indexOf(' ');
    int space2 = str.indexOf(' ', space1 + 1);

    part1 = str.substring(0, space1);
    part2 = str.substring(space1 + 1, space2);
    part3 = str.substring(space2 + 1);
}


void process_command(String command) {
    String part1, part2, part3;
    split_string(command, part1, part2, part3);

    Serial.println("Type=" + part1);
    Serial.println("Param=" + part2);
    Serial.println("Value=" + part3);

    part1.trim();
    part2.trim();
    part3.trim();

    if (part1 == "help") {
      SerialBT.println("Available commands:");
      SerialBT.println("help - Show this help message");
      SerialBT.println("state - Show the current state of all values");
      SerialBT.println("set param xx - Set the value of a parameter");
      SerialBT.println("clear - Restore default values");
      SerialBT.println("reboot - Reboot the device");
      SerialBT.println("test alc - Test alcohol pump (with potentiometer value)");
      SerialBT.println("stop - Stop pump");
      SerialBT.println("stopbt - Stop Bluetooth");

    } else if (part1 == "state") {
      SerialBT.println("------------------------------------------------------");
      SerialBT.println("### Alcohol pump");
      SerialBT.println("1 - PWM hardware Min [25] (0-255): " + String(alcPumpSpeedMinPWM));
      SerialBT.println("2 - PWM hardware Max [150] (0-255): " + String(alcPumpSpeedMaxPWM));
      SerialBT.println("3 - PWM Frequency [10] (10-10000): " + String(alcoholPUMPfreq));
      SerialBT.println("4 - PWM software Min [5] (10-100): " + String(alcoholSoftMinPWM));
      SerialBT.println("5 - PWM software Max [50] (10-100): " + String(alcoholSoftMaxPWM));
      SerialBT.println("6 - Only func speed [45] (0-255): " + String(onlyAlcSpeed));
      SerialBT.println("------------------------------------------------------");
      SerialBT.println("### Water pump");
      SerialBT.println("7 - PWM hardware Min [255] (0-255): " + String(waterPumpSpeedMinPWM));
      SerialBT.println("8 - PWM hardware Max [255] (0-255): " + String(waterPumpSpeedMaxPWM));
      SerialBT.println("9 - PWM Frequency [5000] (10-10000): " + String(waterPUMPfreq));
      SerialBT.println("10 - Only func speed [255] (0-255): " + String(onlyWaterSpeed));
      SerialBT.println("------------------------------------------------------");
      SerialBT.println("### Start & Clean");
      SerialBT.println("11 - Water duration [2400] (1000-9000): " + String(waterStartDuration));
      SerialBT.println("12 - Alcohol duration [2800] (1000-9000): " + String(alcStartDuration));
      SerialBT.println("------------------------------------------------------");

    } else if (part1 == "clear") {
        preferences.begin("settings", false);
        preferences.clear();
        preferences.end();
        SerialBT.println("Preferences cleared !");
        delay(10);
        ESP.restart();

    } else if (part1 == "reboot") {
        SerialBT.println("Reboot...");
        delay(10);
        ESP.restart();

    } else if (part1 == "stopbt") {
        SerialBT.println("Disabling Bluetooth...");
        delay(10);
        SerialBT.end();

    } else if (part1 == "stop") {
      SerialBT.println("Stop");
      process_actions(0);

    } else if (part1 == "set" && part2 != "" && part3 != "") {
        preferences.begin("settings", false);
        if (part2 == "1") {
            alcPumpSpeedMinPWM = part3.toInt();
            preferences.putInt("APS_MIN_PWM", alcPumpSpeedMinPWM);
        } else if (part2 == "2") {
            alcPumpSpeedMaxPWM = part3.toInt();
            preferences.putInt("APS_MAX_PWM", alcPumpSpeedMaxPWM);
        } else if (part2 == "3") {
            alcoholPUMPfreq = part3.toInt();
            preferences.putInt("ALC_PWM_FREQ", alcoholPUMPfreq);
        } else if (part2 == "4") {
            alcoholSoftMinPWM = part3.toInt();
            preferences.putInt("SFT_PW_MIN", alcoholSoftMinPWM);
        } else if (part2 == "5") {
            alcoholSoftMaxPWM = part3.toInt();
            preferences.putInt("SFT_PW_MAX", alcoholSoftMaxPWM);
        } else if (part2 == "6") {
            onlyAlcSpeed = part3.toInt();
            preferences.putInt("ONLY_ALC_SPEED", onlyAlcSpeed);
        } else if (part2 == "7") {
            waterPumpSpeedMinPWM = part3.toInt();
            preferences.putInt("WPS_MIN_PWM", waterPumpSpeedMinPWM);
        } else if (part2 == "8") {
            waterPumpSpeedMaxPWM = part3.toInt();
            preferences.putInt("WPS_MAX_PWM", waterPumpSpeedMaxPWM);
        } else if (part2 == "9") {
            waterPUMPfreq = part3.toInt();
            preferences.putInt("WATER_PWM_FREQ", waterPUMPfreq);
        } else if (part2 == "10") {
            onlyWaterSpeed = part3.toInt();
            preferences.putInt("ONLY_WTR_SPEED", onlyWaterSpeed);
        } else if (part2 == "11") {
            waterStartDuration = part3.toInt();
            preferences.putInt("W_STRT_DUR", waterStartDuration);
        } else if (part2 == "12") {
            alcStartDuration = part3.toInt();
            preferences.putInt("A_STRT_DUR", alcStartDuration);
        }
        preferences.end();
        SerialBT.println("Updated OK");
    }
}

void loop() {
  // Reading potentiometer value
  int potValue = analogRead(POTENTIOMETER);
  drink_intensity = map(potValue, 0, 4095, 100, 0);
  display_intensity_bar(drink_intensity);

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
  bluetooth_gui();
}
