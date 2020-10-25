
#ifndef CAR_H
#define CAR_H

/*************************************************************************************************/

/* C++ compiler compatibility */

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************************/

#include "inttypes.h"

/*************************************************************************************************/

#define GPIO_MOTOR_FORWARD 25
#define GPIO_MOTOR_BACKWARD 26
#define GPIO_TURN_RIGHT 27
#define GPIO_TURN_LEFT 14

#define RIGHT 0
#define LEFT 1
#define FORWARD 0
#define BACKWARD 1
#define STOP 2

/*************************************************************************************************/

class Car
{
    public:
        Car();
        void move(const uint8_t direction);
        void turn(const uint8_t direction);
        void stop(void);
        bool is_moving_forward(void);
        bool is_moving_backward(void);
        bool is_turning_right(void);
        bool is_turning_left(void);

    private:
        bool moving_forward, moving_backward, turning_right, turning_left;

        void setup(void);
        void move_forward(const bool yes);
        void move_backward(const bool yes);
        void dont_move(void);
        void turn_right(const bool yes);
        void turn_left(const bool yes);
        void dont_turn(void);
};

/*************************************************************************************************/

#ifdef __cplusplus
}
#endif  /* extern "C" */

/*************************************************************************************************/

#endif
