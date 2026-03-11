#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <inttypes.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include <hd44780.h>
#include <string.h>
#include "esp_adc/adc_oneshot.h"
#include <math.h>


#define LOOP_DELAY_MS           10      // Loop sampling time (ms)
#define DEBOUNCE_TIME           40      // Debounce time (ms)
#define NROWS                   4       // Number of keypad rows
#define NCOLS                   4       // Number of keypad columns

#define ACTIVE                  0       // Keypad active state (0 = low, 1 = high)

#define NOPRESS                 '\0'    // NOPRESS character

#define DELAY_MS        10                  // Loop delay (ms)
#define NUM_SAMPLES     1000
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_N          LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_MOTOR_1            (4)
#define LEDC_MOTOR_2            (12)        
#define LEDC_FREQUENCY          (50)
#define GREEN_LED               GPIO_NUM_9
#define RED_LED                 GPIO_NUM_11
#define BUZZER_GPIO             GPIO_NUM_5
#define Channel_1 ADC2_CHANNEL_9   // GPIO13
#define Channel_2 ADC2_CHANNEL_3   // GPIO14
#define Channel_3 ADC2_CHANNEL_2   // GPIO20
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define BITWIDTH        ADC_BITWIDTH_12
adc_oneshot_unit_handle_t adc2_handle;   
int comb1;
int comb2;
int comb3;

int potentiometers[] =    {GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_20};       // Pin numbers for the potentiometers

int row_pins[] = {GPIO_NUM_3, GPIO_NUM_8, GPIO_NUM_18, GPIO_NUM_17};     // Pin numbers for rows
int col_pins[] = {GPIO_NUM_16, GPIO_NUM_15, GPIO_NUM_7, GPIO_NUM_6};   // Pin numbers for columns

char keypad_array[NROWS][NCOLS] = {   // Keypad layout
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

hd44780_t lcd = { // declares the values of all the ports on the LCD screen
    .write_cb = NULL,
    .font = HD44780_FONT_5X8,
    .lines = 2,
    .pins = {
        .rs = GPIO_NUM_38,
        .e  = GPIO_NUM_37,
        .d4 = GPIO_NUM_36,
        .d5 = GPIO_NUM_35,
        .d6 = GPIO_NUM_48,
        .d7 = GPIO_NUM_47,
        .bl = HD44780_NOT_USED
    }
};

// initializes all the functions above so they can be run below
static void ADC_Config(void);
static void ledc_init(void);
void lcd_test(void *pvParameters);
char scan_keypad(void);
void open_door(void);
void close_door(void);
void declare_pins(void);
static void ledc_init(void);

typedef enum{ // defines the states for the safe
    OPEN,
    CLOSED,
    OPENING,
    CLOSING
} State_t;
State_t openState = CLOSED; // sets the initial state of openState


void app_main(void)
{
    ADC_Config(); // configures the potentiometers
    ledc_init(); // configures the motor
    ESP_ERROR_CHECK(hd44780_init(&lcd));  // configures the LCD screen
    declare_pins(); // configures all the remaining pins on the board
    int timer = 0; // variables
    char passcode[7] = "3C3218";
    char code[7] = "";
    char prevKey = NOPRESS;
    int attemptsFailed = 0;
    char code_str[16];
    hd44780_gotoxy(&lcd, 0, 0);
    snprintf(code_str,  17, "Passcode:");
    hd44780_puts(&lcd, code_str);
    gpio_set_level(RED_LED, 1);
    gpio_set_level(GREEN_LED, 0);
    while(1){
    bool code1 = false; // variables for determining the correctness of the dials
    bool code2 = false;
    bool code3 = false; 
        switch(openState) {
            case CLOSED: 
                int LEDC_DUTY = 614; // starting duty cycle
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_N, LEDC_DUTY);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_N);
                adc_oneshot_read
                (adc2_handle, Channel_1, &comb1);
                adc_oneshot_read
                (adc2_handle, Channel_2, &comb2);
                adc_oneshot_read
                (adc2_handle, Channel_3, &comb3);
                if (comb1 < 75){ // checking the correct values of the dials
                    code1 = true;
                }
                if ((comb2 < 2050) && (comb2 > 1800)){
                    code2 = true;
                }
                if (comb3 > 3975){
                    code3 = true;
                }
                if(timer > 0){ // stalls the user when the timer is set to above 0
                    hd44780_clear(&lcd);
                    hd44780_gotoxy(&lcd, 0, 0);
                    char time[15]; // had to declare like this so it could print
                    snprintf(time,  22, "Time left: %d", timer);
                    hd44780_puts(&lcd, time);
                    timer--;
                    vTaskDelay(1000/ portTICK_PERIOD_MS);
                    if(timer == 0){
                        hd44780_clear(&lcd);
                        hd44780_gotoxy(&lcd, 0, 0);
                        snprintf(code_str,  17, "Passcode: %s", code);
                        hd44780_puts(&lcd, code_str);
                    }
                    gpio_set_level(BUZZER_GPIO, 0);
                    if(timer != 0){
                        hd44780_clear(&lcd);
                    }
                }
                if(timer == 0){ // allows the user to input a passcode
                    char currentKey = scan_keypad();
                    if(currentKey != NOPRESS && currentKey != prevKey && currentKey != '#'){
                        int len = strlen(code);
                        if(len < 6){
                            code[len] = currentKey;
                            code[len + 1] = '\0';
                        }
                        if(currentKey == '*'){
                            code[0] = '\0';
                        }
                        prevKey = currentKey;
                        hd44780_clear(&lcd);
                        hd44780_gotoxy(&lcd, 0, 0);
                        snprintf(code_str,  17, "Passcode: %s", code);
                        hd44780_puts(&lcd, code_str); //prints the code to the LCD
                        printf("%s \n", code_str); //prints the code to the terminal
                    }
                    else if(currentKey == NOPRESS){ // used so the user holding a button down doesnt repeat the button being pressed
                        prevKey = NOPRESS;
                    }
                    else if(currentKey == '#'){ //checks for enter key
                        prevKey = '#'; 
                        bool codeequal=true;
                        if(strlen(code) == strlen(passcode)){ //checks if the code is equal and also the length of the code
                            for(int i = 0; i < strlen(code); i++){
                                if(code[i] != passcode[i]){
                                    codeequal = false;
                                }
                            }
                        }
                        else{ // if code isnt same length it immediately wrong
                            codeequal = false;
                        }
                        if(codeequal && code1 && code2 && code3){ //opens the lock if the code is correct
                            openState = OPENING;
                            printf("the safe is open \n");
                            code[0] = '\0';
                        }
                        else{ // if the code is incorrect then it returns the code
                            attemptsFailed++;
                            printf("%d, %d, %d\n", comb1, comb2, comb3);
                            printf("incorrect code \n");
                            if(attemptsFailed < 3){
                                timer = 1;
                                gpio_set_level(BUZZER_GPIO, 1);
                            }
                            else{
                                timer = 60* pow(2, (attemptsFailed-2));
                                gpio_set_level(BUZZER_GPIO, 1);
                            }
                        }
                    }
                    vTaskDelay(25/ portTICK_PERIOD_MS);
                }
                break;
            case OPEN: // waits for the close button to be pressed and then enters the closing phase
                char currentKey = scan_keypad();
                if(currentKey == '#'){
                    if(prevKey != '#'){
                        openState = CLOSING;
                    }
                }
                else{
                    prevKey = currentKey;
                }
                vTaskDelay(25/ portTICK_PERIOD_MS);
                break;
            case OPENING: //opens the lock and turns on the green LED 
                hd44780_clear(&lcd);
                hd44780_gotoxy(&lcd, 0, 0);
                hd44780_puts(&lcd, "OPENING");
                open_door();
                vTaskDelay(500/ portTICK_PERIOD_MS);
                hd44780_clear(&lcd);
                hd44780_gotoxy(&lcd, 0, 0);
                hd44780_puts(&lcd, "OPEN");
                openState = OPEN;
                break;
            case CLOSING: //closes the lock and turns on the red LED, as well as resetting the attempt counter 
                hd44780_clear(&lcd);
                hd44780_gotoxy(&lcd, 0, 0);
                hd44780_puts(&lcd, "CLOSING");
                close_door();
                vTaskDelay(500/ portTICK_PERIOD_MS);
                hd44780_gotoxy(&lcd, 0, 0);
                attemptsFailed = 0;
                snprintf(code_str,  17, "Passcode:");
                hd44780_puts(&lcd, code_str);
                openState = CLOSED;
                break;
        }
    }
}



void declare_pins()
{
    for (int i = 0; i < sizeof(row_pins)/sizeof(row_pins[0]); i++){
        gpio_reset_pin(row_pins[i]);
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[i], 1);
    }
    for (int i = 0; i < sizeof(col_pins)/sizeof(col_pins[0]); i++){
        gpio_reset_pin(col_pins[i]);
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_pullup_en(col_pins[i]);
    }
    gpio_reset_pin(GREEN_LED);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RED_LED);
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
}
static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 50 Hz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));


    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_N,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_MOTOR_1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}


char scan_keypad()
{
    for(int i = 0; i < sizeof(row_pins)/sizeof(row_pins[0]); i++){
        gpio_set_level(row_pins[i], 0);
        for(int k = 0; k < sizeof(col_pins)/sizeof(col_pins[0]); k++){
            if(!gpio_get_level(col_pins[k])){
                gpio_set_level(row_pins[i], 1);
                //printf("%c \n", keypad_array[i][k]);
                return keypad_array[i][k];
            }
        }
        gpio_set_level(row_pins[i], 1);
    }
    
    return '\0';
}

void open_door(){
    int LEDC_DUTY = 364;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_N, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_N);
    gpio_set_level(RED_LED, 0);
    gpio_set_level(GREEN_LED, 1);
    // these are the two you should work on
}

void close_door(){
    int LEDC_DUTY = 614;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_N, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_N);
    gpio_set_level(RED_LED, 1);
    gpio_set_level(GREEN_LED, 0);
    //
}

void ADC_Config(void) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_2,
    };
    adc_oneshot_new_unit(&init_config1, &adc2_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = BITWIDTH
    };

    adc_oneshot_config_channel(adc2_handle, Channel_1, &config);
    adc_oneshot_config_channel(adc2_handle, Channel_2, &config);
    adc_oneshot_config_channel(adc2_handle, Channel_3, &config);
}
