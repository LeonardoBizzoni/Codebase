#ifndef CODERBOT_CORE_H
#define CODERBOT_CORE_H

#include <pigpio.h>

#define PWM_FREQ 100
#define MAX_DUTY_CYCLE 255

typedef i8 CB_GPIO;
enum {
    /* STMicroelectronics L293DD - Four channel H-Bridge driver
     *                  +---+_+---+
     *             Vcc  |1*     20|  Vcc
     * Channel 1  GP17  |2      19|  GP23  Channel 4
     *    LF   ^   J61  |3   L  18|  J72   ^   RB
     *             GND  |4   2  17|  GND
     *             GND  |5   9  16|  GND
     *             GND  |6   3  15|  GND
     *             GND  |7   D  14|  GND
     * Channel 2   J62  |8   D  13|  J71   Channel 3
     *    LB   ^  GP18  |9      12|  GP22  ^   RF
     *                  |10     11|  Vcc
     *                  +---------+
     *
     * J6 - Left Motor Header
     *    1
     * +-+-+  1: Left Motor +/Forward
     * |.|.|  2: Left Motor -/Backwards
     * +-+-+
     * J6
     *
     * J7 - Right Motor Header
     *    1
     * +-+-+  1: Right Motor +/Forward
     * |.|.|  2: Right Motor -/Backwards
     * +-+-+
     * J7
     */
    CB_GPIO_Motor_Left_Forward = 17,    // L293DD Pin 2, Ch.1
    CB_GPIO_Motor_Left_Backward = 18,   // L293DD Pin 9, Ch.2
    CB_GPIO_Motor_Right_Forward = 23,   // L293DD Pin 12, Ch.3
    CB_GPIO_Motor_Right_Backward = 22,  // L293DD Pin 19, Ch.4

    // =========================================================================
    // Unused
    CB_GPIO_Button_Push = 16, // 11

    // servo
    CB_GPIO_Servo_1 = 19, // 9
    CB_GPIO_Servo_2 = 26, // 10

    // sonar
    CB_GPIO_Sonar_1_Trigger = 5, // 18
    CB_GPIO_Sonar_1_Echo = 27, // 7
    CB_GPIO_Sonar_2_Trigger = 5, // 18
    CB_GPIO_Sonar_2_Echo = 6, // 8
    CB_GPIO_Sonar_3_Trigger = 5, // 18
    CB_GPIO_Sonar_3_Echo = 12, // 23
    CB_GPIO_Sonar_4_Trigger = 5, // 18
    CB_GPIO_Sonar_4_Echo = 13, // 23

    // =========================================================================
    /* J11 - Left Encoder Header
     * +-+-+-+-+
     * |.|.|.|.| J11
     * +-+-+-+-+
     *  1
     *
     * 1: VCC +5V
     * 2: GND
     * 3: Channel B, IO15, Pin 10
     * 4: Channel A, IO14, Pin 8 */
    CB_GPIO_Encoder_Left_A = 14,
    CB_GPIO_Encoder_Left_B = 15,

    // =========================================================================
    /* J12 - Right Encoder Header
     * +-+-+-+-+
     * |.|.|.|.| J12
     * +-+-+-+-+
     *  1
     *
     * 1: VCC +5V
     * 2: GND
     * 3: Channel A, IO24, Pin 18
     * 4: Channel B, IO25, Pin 22 */
    CB_GPIO_Encoder_Right_A = 24,
    CB_GPIO_Encoder_Right_B = 25
};

typedef u8 CB_Direction;
enum {
  CB_Direction_None = 0,
  CB_Direction_Forward,
  CB_Direction_Backward,
};

typedef struct {
  CB_GPIO pin_fw, pin_bw;
  CB_Direction direction;
} CB_Motor;
#define CB_MOTOR_LEFT 0
#define CB_MOTOR_RIGHT 1

typedef u8 CB_Encoder_Edge;
enum {
  CB_Encoder_Edge_Rising  = (1 << 0),
  CB_Encoder_Edge_Falling = (1 << 1),
};

typedef u8 CB_Encoder_Level;
enum {
  CB_Encoder_Level_Low,
  CB_Encoder_Level_High,
  CB_Encoder_Level_NoChange,
};

typedef u8 CB_Encoder_Channel;
enum {
  CB_Encoder_Channel_A,
  CB_Encoder_Channel_B,
};

typedef struct {
  CB_GPIO pin_a, pin_b;
  CB_GPIO last_gpio;
  CB_Direction direction;
  CB_Encoder_Level level_a, level_b;
  i64 ticks, bad_ticks;
} CB_Encoder;
#define CB_ENCODER_LEFT 0
#define CB_ENCODER_RIGHT 1

typedef void (*CB_GPIO_ISR_Fn)(CB_GPIO gpio,
                               CB_Encoder_Level level,
               /* Aka ticks */ u32 microsecs_since_boot,
                               CB_Encoder *encoder);

fn void cb_init(void);
fn void cb_deinit(void);

// TODO(lb): add tracing functionality so that we can undo the
//           the movement of the motor.
fn void cb_motor_move(u8 left_right_motor, CB_Direction dir,
                      f32 duty_cycle, u32 at_least_millisec);

fn void cb_encoder_register_callback(u8 left_right_encoder,
                                     CB_Encoder_Channel chan,
                                     CB_Encoder_Edge edge,
                                     u32 timeout_millisec,
                                     CB_GPIO_ISR_Fn *func);

fn void cb_encoder_unregister_callback(u8 left_right_encoder,
                                       CB_Encoder_Channel chan);

#endif
