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

// PWM Led
#define PWM_FREQ 5000   // PWM Frequency (Hz)
#define PWM_RESOLUTION 8 // PWM Resolution (bits)

#define LEDC_CHANNEL 0        // Définir le canal LEDC à utiliser
#define POTENTIOMETER_LED 23

// L298N Pin PWM pumps
#define WATER_PUMP 2 // ENA
#define WATER_PUMP_IN1 0
#define WATER_PUMP_IN2 4

#define ALC_PUMP 5 // ENB
#define ALC_PUMP_IN3 16
#define ALC_PUMP_IN4 17

// Water tank valve
#define TANK_VALVE_RELAY 19

/*
Water/Alcool PWM adjustment constants
Alcool pump 24V MAX
Water pump 18V MAX
Alimentation 24V (L298N voltage drop 2V) 
MAX @100% PWM => 22V

PWM 	Tension (V)
0       0
25      2,1
50      4,3
75      6,5
100     8,6
125     10,8
150     13
175     15,2
200     17,3
225     19,5
250     21,7
255     22
*/
#define ALC_PUMP_SPEED_MIN_PWM 50
#define ALC_PUMP_SPEED_MAX_PWM 255
#define WATER_PUMP_SPEED_MIN_PWM 125
#define WATER_PUMP_SPEED_MAX_PWM 200

// Throttle speed single use function
#define ONLY_WATER_SPEED 150
#define ONLY_ALC_SPEED 150

// Start duration
#define WATER_START_DURATION 3000
#define ALC_START_DURATION 3000

#endif // CONFIG_H
