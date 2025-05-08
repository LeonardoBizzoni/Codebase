global CB_Robot cb = {
  .motors = {
    {
      .pin_fw = CB_GPIO_Motor_Left_Forward,
      .pin_bw = CB_GPIO_Motor_Left_Backward,
      .direction = CB_Direction_None,
    },
    {
      .pin_fw = CB_GPIO_Motor_Right_Forward,
      .pin_bw = CB_GPIO_Motor_Right_Backward,
      .direction = CB_Direction_None,
    },
  },
  .encoders = {
    {
      .pin_a = CB_GPIO_Encoder_Left_A,
      .pin_b = CB_GPIO_Encoder_Left_B,
      .last_gpio = -1,
      .callback = {
        .timeout = 50,
        .trigger = CB_Encoder_Edge_Both,
        .channel_a = cb_encoder_isr_chanA,
        .channel_b = cb_encoder_isr_chanB,
      },
    },
    {
      .pin_a = CB_GPIO_Encoder_Right_A,
      .pin_b = CB_GPIO_Encoder_Right_B,
      .last_gpio = -1,
      .callback = {
        .timeout = 50,
        .trigger = CB_Encoder_Edge_Both,
        .channel_a = cb_encoder_isr_chanA,
        .channel_b = cb_encoder_isr_chanB,
      },
    },
  }
};

fn void cb_init(void) {
  Assert(gpioInitialise() >= 0);

  gpioSetMode(cb.motor.left.pin_fw, PI_OUTPUT);
  gpioSetPWMrange(cb.motor.left.pin_fw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb.motor.left.pin_fw, PWM_FREQ);
  gpioSetMode(cb.motor.left.pin_bw, PI_OUTPUT);
  gpioSetPWMrange(cb.motor.left.pin_bw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb.motor.left.pin_bw, PWM_FREQ);
  gpioSetMode(cb.motor.right.pin_fw, PI_OUTPUT);
  gpioSetPWMrange(cb.motor.right.pin_fw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb.motor.right.pin_fw, PWM_FREQ);
  gpioSetMode(cb.motor.right.pin_bw, PI_OUTPUT);
  gpioSetPWMrange(cb.motor.right.pin_bw, MAX_DUTY_CYCLE);
  gpioSetPWMfrequency(cb.motor.right.pin_bw, PWM_FREQ);

  gpioSetMode(cb.encoder.left.pin_a, PI_INPUT);
  gpioSetPullUpDown(cb.encoder.left.pin_a, PI_PUD_UP);
  gpioSetMode(cb.encoder.left.pin_b, PI_INPUT);
  gpioSetPullUpDown(cb.encoder.left.pin_b, PI_PUD_UP);
  gpioSetMode(cb.encoder.right.pin_a, PI_INPUT);
  gpioSetPullUpDown(cb.encoder.right.pin_a, PI_PUD_UP);
  gpioSetMode(cb.encoder.right.pin_b, PI_INPUT);
  gpioSetPullUpDown(cb.encoder.right.pin_b, PI_PUD_UP);
}

fn void cb_deinit(void) {
  gpioWrite(cb.motor.left.pin_fw, 0);
  gpioWrite(cb.motor.left.pin_bw, 0);
  gpioWrite(cb.motor.right.pin_fw, 0);
  gpioWrite(cb.motor.right.pin_bw, 0);
  cb.motor.left.direction = CB_Direction_None;
  cb.motor.right.direction = CB_Direction_None;

  gpioSetISRFunc(cb.encoder.left.pin_a, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb.encoder.left.pin_b, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb.encoder.right.pin_a, EITHER_EDGE, 0, 0);
  gpioSetISRFunc(cb.encoder.right.pin_b, EITHER_EDGE, 0, 0);
  cb.encoder.left.direction = CB_Direction_None;
  cb.encoder.right.direction = CB_Direction_None;

  gpioTerminate();
}

fn void cb_motor_move(CB_Motor *motor, CB_Direction dir, f32 duty_cycle) {
  Assert((dir == CB_Direction_Forward ||
          dir == CB_Direction_Backward ||
          dir == CB_Direction_None) &&
         (duty_cycle >= 0.f && duty_cycle <= 1.f) && motor);

  i32 pwm = MAX_DUTY_CYCLE * duty_cycle;
  switch ((motor->direction = dir)) {
  case CB_Direction_None: {
    gpioPWM(motor->pin_fw, 0);
    gpioWrite(motor->pin_bw, 0);
  } break;
  case CB_Direction_Forward: {
    gpioPWM(motor->pin_fw, pwm);
    gpioWrite(motor->pin_bw, 0);
  } break;
  case CB_Direction_Backward: {
    gpioWrite(motor->pin_fw, 0);
    gpioPWM(motor->pin_bw, pwm);
  } break;
  }
}

fn void cb_encoder_bind(CB_Encoder *encoder) {
  cb_encoder_bind_channel(encoder, CB_Encoder_Channel_A);
  cb_encoder_bind_channel(encoder, CB_Encoder_Channel_B);
}

fn void cb_encoder_bind_channel(CB_Encoder *encoder, CB_Encoder_Channel chan) {
  Assert((chan == CB_Encoder_Channel_A ||
          chan == CB_Encoder_Channel_B) && encoder);
  gpioSetISRFuncEx((chan == CB_Encoder_Channel_A
                    ? encoder->pin_a
                    : encoder->pin_b),
                   (encoder->callback.trigger == CB_Encoder_Edge_Rising
                    ? RISING_EDGE
                    : (encoder->callback.trigger == CB_Encoder_Edge_Falling
                       ? FALLING_EDGE : EITHER_EDGE)),
                   encoder->callback.timeout,
                   (gpioISRFuncEx_t)encoder->callback.channels[chan], encoder);
}

fn void cb_encoder_unbind(CB_Encoder *encoder) {
  cb_encoder_unbind_channel(encoder, CB_Encoder_Channel_A);
  cb_encoder_unbind_channel(encoder, CB_Encoder_Channel_B);
}

fn void cb_encoder_unbind_channel(CB_Encoder *encoder, CB_Encoder_Channel chan) {
  Assert((chan == CB_Encoder_Channel_A ||
          chan == CB_Encoder_Channel_B) && encoder);
  gpioSetISRFunc((chan == CB_Encoder_Channel_A
                  ? encoder->pin_a : encoder->pin_b),
                 EITHER_EDGE, 0, 0);
}

fn void cb_encoder_isr_chanA(CB_GPIO gpio, CB_Encoder_Level level,
                             u32 ticks, CB_Encoder *enc) {
  if (!enc || gpio == enc->last_gpio) { return; }
  enc->last_gpio = gpio;
  enc->level_a = level;
  if(level ^ enc->level_b) {
    enc->direction = CB_Direction_Forward;
    enc->ticks += CB_Direction_Forward;
  }
}

fn void cb_encoder_isr_chanB(CB_GPIO gpio, CB_Encoder_Level level,
                             u32 ticks, CB_Encoder *enc) {
  if (!enc || gpio == enc->last_gpio) { return; }
  enc->last_gpio = gpio;
  enc->level_b = level;
  if(level ^ enc->level_a) {
    enc->direction = CB_Direction_Backward;
    enc->ticks += CB_Direction_Backward;
  }
}
