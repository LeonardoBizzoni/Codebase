global i32 gpio_pin_counter = 0;

global CB_Motor cb_motor[2] = {
  { CB_GPIO_Motor_Left_Forward, CB_GPIO_Motor_Left_Backward, CB_Direction_None },
  { CB_GPIO_Motor_Right_Forward, CB_GPIO_Motor_Right_Backward, CB_Direction_None },
};

global CB_Encoder cb_encoder[2] = {
  { CB_GPIO_Encoder_Left_A, CB_GPIO_Encoder_Left_B, -1, 0, 0, 0, 0, 0 },
  { CB_GPIO_Encoder_Right_A, CB_GPIO_Encoder_Right_B, -1, 0, 0, 0, 0, 0 },
};

fn void cb_init(void) {
  Assert(gpioInitialise() >= 0);

  gpioSetMode(cb_motor[CB_MOTOR_LEFT].pin_fw, PI_OUTPUT);
  gpioSetPWMrange(cb_motor[CB_MOTOR_LEFT].pin_fw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb_motor[CB_MOTOR_LEFT].pin_fw, PWM_FREQ);
  gpioSetMode(cb_motor[CB_MOTOR_LEFT].pin_bw, PI_OUTPUT);
  gpioSetPWMrange(cb_motor[CB_MOTOR_LEFT].pin_bw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb_motor[CB_MOTOR_LEFT].pin_bw, PWM_FREQ);
  gpioSetMode(cb_motor[CB_MOTOR_RIGHT].pin_fw, PI_OUTPUT);
  gpioSetPWMrange(cb_motor[CB_MOTOR_RIGHT].pin_fw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb_motor[CB_MOTOR_RIGHT].pin_fw, PWM_FREQ);
  gpioSetMode(cb_motor[CB_MOTOR_RIGHT].pin_bw, PI_OUTPUT);
  gpioSetPWMrange(cb_motor[CB_MOTOR_RIGHT].pin_bw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb_motor[CB_MOTOR_RIGHT].pin_bw, PWM_FREQ);

  gpioSetMode(cb_encoder[CB_ENCODER_LEFT].pin_a, PI_INPUT);
  gpioSetPullUpDown(cb_encoder[CB_ENCODER_LEFT].pin_a, PI_PUD_UP);
  gpioSetMode(cb_encoder[CB_ENCODER_LEFT].pin_b, PI_INPUT);
  gpioSetPullUpDown(cb_encoder[CB_ENCODER_LEFT].pin_b, PI_PUD_UP);
  gpioSetMode(cb_encoder[CB_ENCODER_RIGHT].pin_a, PI_INPUT);
  gpioSetPullUpDown(cb_encoder[CB_ENCODER_RIGHT].pin_a, PI_PUD_UP);
  gpioSetMode(cb_encoder[CB_ENCODER_RIGHT].pin_b, PI_INPUT);
  gpioSetPullUpDown(cb_encoder[CB_ENCODER_RIGHT].pin_b, PI_PUD_UP);
}

fn void cb_deinit(void) {
  gpioWrite(cb_motor[CB_MOTOR_LEFT].pin_fw, 0);
  gpioWrite(cb_motor[CB_MOTOR_LEFT].pin_bw, 0);
  gpioWrite(cb_motor[CB_MOTOR_RIGHT].pin_fw, 0);
  gpioWrite(cb_motor[CB_MOTOR_RIGHT].pin_bw, 0);
  cb_motor[CB_MOTOR_LEFT].direction = CB_Direction_None;
  cb_motor[CB_MOTOR_RIGHT].direction = CB_Direction_None;

  gpioSetISRFunc(cb_encoder[CB_ENCODER_LEFT].pin_a, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb_encoder[CB_ENCODER_LEFT].pin_b, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb_encoder[CB_ENCODER_RIGHT].pin_a, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb_encoder[CB_ENCODER_RIGHT].pin_b, EITHER_EDGE, 0, 0);
  cb_encoder[CB_ENCODER_LEFT].direction = CB_Direction_None;
  cb_encoder[CB_ENCODER_RIGHT].direction = CB_Direction_None;

  gpioTerminate();
}

fn void cb_motor_move(u8 left_right_motor, CB_Direction dir,
                      f32 duty_cycle, u32 at_least_millisec) {
  Assert((dir == CB_Direction_Forward ||
          dir == CB_Direction_Backward ||
          dir == CB_Direction_None) &&
         (duty_cycle >= 0.f && duty_cycle <= 1.f) &&
         (left_right_motor == CB_MOTOR_LEFT ||
          left_right_motor == CB_MOTOR_RIGHT));

  i32 pwm = MAX_DUTY_CYCLE * duty_cycle;
  switch ((cb_motor[left_right_motor].direction = dir)) {
  case CB_Direction_None: {
    gpioPWM(cb_motor[left_right_motor].pin_fw, 0);
    gpioWrite(cb_motor[left_right_motor].pin_bw, 0);
  } break;
  case CB_Direction_Forward: {
    gpioPWM(cb_motor[left_right_motor].pin_fw, pwm);
    gpioWrite(cb_motor[left_right_motor].pin_bw, 0);
  } break;
  case CB_Direction_Backward: {
    gpioWrite(cb_motor[left_right_motor].pin_fw, 0);
    gpioPWM(cb_motor[left_right_motor].pin_bw, pwm);
  } break;
  }
  os_sleep_milliseconds(at_least_millisec);
}

fn void cb_encoder_register_callback(u8 left_right_encoder,
                                     CB_Encoder_Channel chan,
                                     CB_Encoder_Edge edge,
                                     u32 timeout_millisec,
                                     CB_GPIO_ISR_Fn *func) {
  Assert((left_right_encoder == CB_ENCODER_LEFT ||
          left_right_encoder == CB_ENCODER_RIGHT) &&
         (chan == CB_Encoder_Channel_A ||
          chan == CB_Encoder_Channel_B) &&
         (edge >= 1 && edge <= 3) && func);
  gpioSetISRFuncEx((chan == CB_Encoder_Channel_A
                    ? cb_encoder[left_right_encoder].pin_a
                    : cb_encoder[left_right_encoder].pin_b),
                   (edge == CB_Encoder_Edge_Rising
                    ? RISING_EDGE
                    : (edge == CB_Encoder_Edge_Falling
                       ? FALLING_EDGE
                       : EITHER_EDGE)),
                   timeout_millisec, (gpioISRFuncEx_t)func,
                   &cb_encoder[left_right_encoder]);
}

fn void cb_encoder_unregister_callback(u8 left_right_encoder,
                                       CB_Encoder_Channel chan) {
  Assert((left_right_encoder == CB_ENCODER_LEFT ||
          left_right_encoder == CB_ENCODER_RIGHT) &&
         (chan == CB_Encoder_Channel_A ||
          chan == CB_Encoder_Channel_B));
  gpioSetISRFunc((chan == CB_Encoder_Channel_A
                  ? cb_encoder[left_right_encoder].pin_a
                  : cb_encoder[left_right_encoder].pin_b),
                 EITHER_EDGE, 0, 0);
}
