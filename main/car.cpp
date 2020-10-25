
#include "car.h"

#include "config.h"

/*************************************************************************************************/

#if defined(ARDUINO) // Arduino Framework

    #include "Arduino.h"

    #define _LOW (LOW)
    #define _HIGH (HIGH)
    #define _OUTPUT (OUTPUT)
    #define _INPUT (INPUT)
    #define _INPUT_PULLUP (INPUT_PULLUP)
    #define _INPUT_PULLDOWN (INPUT_PULLDOWN)

#elif defined(ESP_IDF) // ESP32 ESPIDF Framework

    #include "freertos/FreeRTOS.h"
    #include "driver/gpio.h"

    #define _LOW (0)
    #define _HIGH (1)
    #define _OUTPUT (0)
    #define _INPUT (1)
    #define _INPUT_PULLUP (2)
    #define _INPUT_PULLDOWN (3)
#endif

static void _pinMode(const uint8_t gpio, const uint8_t mode);
static void _digitalWrite(const uint8_t gpio, const uint8_t value);

/*************************************************************************************************/

Car::Car()
{
    moving_forward = false;
    moving_backward = false;
    turning_right = false;
    turning_left = false;
    setup();
}

void Car::move(const uint8_t direction)
{
    if(direction == FORWARD)
    {
        move_backward(false);
        move_forward(true);
    }
    else if(direction == BACKWARD)
    {
        move_forward(false);
        move_backward(true);
    }
    else /* if(direction == STOP) */
    {
        move_backward(false);
        move_forward(false);
    }
}

void Car::turn(const uint8_t direction)
{
    if(direction == RIGHT)
    {
        turn_left(false);
        turn_right(true);
    }
    else if(direction == LEFT)
    {
        turn_right(false);
        turn_left(true);
    }
    else /* if(direction == STOP) */
    {
        turn_left(false);
        turn_right(false);
    }
}

void Car::stop(void)
{
    move(STOP);
    turn(STOP);
}

bool Car::is_moving_forward(void)
{
    return moving_forward;
}

bool Car::is_moving_backward(void)
{
    return moving_backward;
}

bool Car::is_turning_right(void)
{
    return turning_right;
}

bool Car::is_turning_left(void)
{
    return turning_left;
}

/*************************************************************************************************/

void Car::setup(void)
{
    _pinMode(GPIO_MOTOR_FORWARD, _OUTPUT);
    _pinMode(GPIO_MOTOR_BACKWARD, _OUTPUT);
    _pinMode(GPIO_TURN_RIGHT, _OUTPUT);
    _pinMode(GPIO_TURN_LEFT, _OUTPUT);

    _digitalWrite(GPIO_MOTOR_FORWARD, _LOW);
    _digitalWrite(GPIO_MOTOR_BACKWARD, _LOW);
    _digitalWrite(GPIO_TURN_RIGHT, _LOW);
    _digitalWrite(GPIO_TURN_LEFT, _LOW);
}

void Car::move_forward(const bool yes)
{
    if(yes)
        _digitalWrite(GPIO_MOTOR_FORWARD, _HIGH);
    else
        _digitalWrite(GPIO_MOTOR_FORWARD, _LOW);
    moving_forward = yes;
}

void Car::move_backward(const bool yes)
{
    if(yes)
        _digitalWrite(GPIO_MOTOR_BACKWARD, _HIGH);
    else
        _digitalWrite(GPIO_MOTOR_BACKWARD, _LOW);
    moving_backward = yes;
}

void Car::turn_right(const bool yes)
{
    if(yes)
        _digitalWrite(GPIO_TURN_RIGHT, _HIGH);
    else
        _digitalWrite(GPIO_TURN_RIGHT, _LOW);
    turning_right = yes;
}

void Car::turn_left(const bool yes)
{
    if(yes)
        _digitalWrite(GPIO_TURN_LEFT, _HIGH);
    else
        _digitalWrite(GPIO_TURN_LEFT, _LOW);
    turning_left = yes;
}

/*************************************************************************************************/

#if defined(ARDUINO) // Arduino Framework

static void _pinMode(const uint8_t gpio, const uint8_t mode)
{
    pinMode(gpio, mode);
}

static void _digitalWrite(const uint8_t gpio, const uint8_t value)
{
    digitalWrite(gpio, value);
}

/*************************************************************************************************/

#elif defined(ESP_IDF) // ESP32 ESPIDF Framework

static void _pinMode(const uint8_t gpio, const uint8_t mode)
{
    if(mode == _INPUT)
    {
        gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_INPUT);
    }
    if(mode == _INPUT_PULLUP)
    {
        gpio_pad_pullup((gpio_num_t)gpio);
        gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_INPUT);
    }
    else if(mode == _INPUT_PULLDOWN)
    {
        gpio_pad_pulldown((gpio_num_t)gpio);
        gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_INPUT);
    }
    else if(mode == _OUTPUT)
    {
        gpio_pad_select_gpio((gpio_num_t)gpio);
        gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_OUTPUT);
    }
}

static void _digitalWrite(const uint8_t gpio, const uint8_t value)
{
    gpio_set_level((gpio_num_t)gpio, value);
}

/*************************************************************************************************/

#endif
