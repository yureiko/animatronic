#include "servo_funcs.h"
#include "pwm_funcs.h"
#include <string.h>
#include <stdlib.h>
#include "communication_protocol_definitions.h"

#define NUM_SERVOS 8

#define SERVO_MIN_POSITION 0                    // 0 degrees
#define SERVO_MAX_POSITION 180.0                 // 180 degrees

#define SERVO_MIN_DUTY_CYCLE_PERIOD_MS 0.5      // 0.5ms
#define SERVO_MAX_DUTY_CYCLE_PERIOD_MS 2.4      // 2.4ms

#define MSGQUEUE_OBJECTS      5 

typedef enum
{
  SERVO_EYES_X = 0,
  SERVO_EYES_Y,
  SERVO_EYELIDS_S,
  SERVO_EYELIDS_I,
  SERVO_EYEBROWS_L,
  SERVO_EYEBROWS_R,
  SERVO_MOUTH,
  SERVO_EARS
}servo_names_t;

typedef struct
{
  uint8_t id;
  char name[16];
  uint32_t pwm_id;
  float default_position;
}servo_struct_t;

/******************************************************************************
* Local prototypes
******************************************************************************/
osThreadId_t servo_main_thread_id;
osMessageQueueId_t servo_main_thread_msg;

float servo_deg_to_duty_cycle(float pos);
void servo_set_position(servo_struct_t *servo, float pos);
servo_struct_t *servo_initialize(uint8_t id, char *name, uint32_t pwm_id, float default_position);

void servo_main_task(void *arg);

void servos_thread_init()
{
    servo_main_thread_id = osThreadNew(servo_main_task, NULL, NULL);
    servo_main_thread_msg = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(servo_message_t), NULL);
}

void servo_set_position(servo_struct_t *servo, float pos)
{
  PWM_set_duty(servo->pwm_id, servo_deg_to_duty_cycle(pos));
}

servo_struct_t* servo_initialize(uint8_t id, char *name, uint32_t pwm_id, float default_position)
{
  servo_struct_t *new_servo = NULL;
  
  new_servo = malloc(sizeof(servo_struct_t));
  
  new_servo->id = id;
  strcpy(new_servo->name, name);
  new_servo->pwm_id = pwm_id;
  new_servo->default_position = default_position;

  servo_set_position(new_servo, default_position);

  return new_servo;
}

float servo_deg_to_duty_cycle(float pos)
{
  float duty;
  static float period_ms;
  
  period_ms = SERVO_MIN_DUTY_CYCLE_PERIOD_MS + ((pos - SERVO_MIN_POSITION)  * 
                      ((float)(SERVO_MAX_DUTY_CYCLE_PERIOD_MS - SERVO_MIN_DUTY_CYCLE_PERIOD_MS)) /
                      ((float)(SERVO_MAX_POSITION - SERVO_MIN_POSITION)));

  duty = (period_ms/10.f) * PWM_FREQUENCY;

  return duty;
}

void servo_main_task(void *arg)
{
  servo_message_t servo_message;

  static servo_struct_t *servo_list[NUM_SERVOS];

  // Eyes
  servo_list[SERVO_EYES_X] = servo_initialize(SERVO_EYES_X, "eyes_x", PWM_OUT_0, 90.f);
  servo_list[SERVO_EYES_Y] = servo_initialize(SERVO_EYES_Y, "eyes_y", PWM_OUT_1, 90.f);

  // Eyelids
  servo_list[SERVO_EYELIDS_S] = servo_initialize(SERVO_EYELIDS_S, "eyelids_s", PWM_OUT_2, 90.f);
  servo_list[SERVO_EYELIDS_I] = servo_initialize(SERVO_EYELIDS_I, "eyelids_i", PWM_OUT_3, 90.f);

  // Eyebrows
  servo_list[SERVO_EYEBROWS_L] = servo_initialize(SERVO_EYEBROWS_L, "eyebrows_l", PWM_OUT_4, 90.f);
  servo_list[SERVO_EYEBROWS_R] = servo_initialize(SERVO_EYEBROWS_R, "eyebrows_i", PWM_OUT_5, 90.f);

  // Mouth
  servo_list[SERVO_MOUTH] = servo_initialize(SERVO_MOUTH, "mouth", PWM_OUT_6, 90.f);

  // Ears
  servo_list[SERVO_EARS] = servo_initialize(SERVO_EARS, "ears", PWM_OUT_7, 90.f);

  osStatus_t status;

  while(1)
  {
    status = osMessageQueueGet(servo_main_thread_msg, &servo_message, NULL, osWaitForever);

    if (status == osOK) 
    {
      uint8_t *data  = servo_message.data;
      float value;
      
      switch(data[0])
      {
        case ID_SET_SERVO_POSITION:
          if(data[1] < NUM_SERVOS)
          {
            value = (float) ((0x000000FF & data[2] | 0x0000FF00 & data[3] << 8))/100.0;
            servo_set_position(servo_list[data[1]], value);
          }
        break;

        case 0x01:
        break;

        default:
        break;
      }
    }
  }
}
