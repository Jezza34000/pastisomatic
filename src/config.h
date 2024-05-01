#ifndef CONFIG_H
#define CONFIG_H

// Screen configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// IHM config
#define NUM_BUTTONS 4
#define DEBOUNCE_TIME 20

// IHM pins
#define BUTTON_1 35
#define BUTTON_2 34
#define BUTTON_3 25
#define BUTTON_4 27

#define NUM_LEDS 4
#define LED_1 32
#define LED_2 33
#define LED_3 26
#define LED_4 14

#define POTENTIOMETER 36

// L298N Pin PWM pumps
#define WATER_PUMP_PIN 2 // ENA
#define WATER_PUMP_IN1 0
#define WATER_PUMP_IN2 4

#define ALC_PUMP_PIN 5 // ENB
#define ALC_PUMP_IN3 16
#define ALC_PUMP_IN4 17

// PWM Led
#define LEDC_CHANNEL 0
#define POTENTIOMETER_LED 23
#define LED_PWM_FREQ 5000   // PWM Frequency (Hz)
#define LED_PWM_RESOLUTION 8 // PWM Resolution (bits)

// PWM Alcohol pump
#define ALC_PUMP 8
#define ALC_PUMP_PWM_FREQ 10   // PWM Frequency (Hz)
#define ALC_PUMP_PWM_RESOLUTION 8 // PWM Resolution (bits)

// PWM Water pump
#define WATER_PUMP 10
#define WATER_PUMP_PWM_FREQ 5000   // PWM Frequency (Hz)
#define WATER_PUMP_PWM_RESOLUTION 8 // PWM Resolution (bits)

// Water tank valve
#define TANK_VALVE_RELAY 19

/*
Water/Alcool PWM adjustment constants
Alcool/Water pump 24V MAX
Alimentation 22V (L298N voltage drop 2V) 
MAX @100% PWM => 20V

Alcohol minimum speed => 95
Water minimum speed => 105

PWM (0-255)	Sortie (en volts)
76	    6.0 V
102	    8.0 V
127	    9.8 V
153	    12.0 V
178	    14.0 V
204	    16.0 V
229	    18.0 V
255	    20.0 V
*/
#define ALC_PUMP_SPEED_MIN_PWM 10
#define ALC_PUMP_SPEED_MAX_PWM 230
#define WATER_PUMP_SPEED_MIN_PWM 255
#define WATER_PUMP_SPEED_MAX_PWM 255

#define ALC_SOFT_PWM_MIN_DUTY 0
#define ALC_SOFT_PWM_MAX_DUTY 100

// Throttle speed single use function
#define ONLY_WATER_SPEED 240
#define ONLY_ALC_SPEED 100 

// Start duration
#define WATER_START_DURATION 3000
#define ALC_START_DURATION 3000

#endif // CONFIG_H
