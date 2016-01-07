/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Library includes. */
#include "stm32f10x_it.h"

/* Demo program include files. */
#include "serial.h"
#include "comtest.h"
#include "partest.h"
#include "i2c.h"
#include "mpu9150.h"
#include "inv_mpu.h"

/*-----------------------------------------------------------*/
/* The queue used to send messages to the uart task. */

void MPU9150Init( void )
{
 int result;
    unsigned char accel_fsr,  new_temp = 0;
    unsigned short gyro_rate, gyro_fsr;
    struct int_param_s int_param;

#ifdef COMPASS_ENABLED
    unsigned char new_compass = 0;
    unsigned short compass_fsr;
#endif
 
		
	//initialize the i2c2
	I2cMaster_Init();
  result = mpu_init(&int_param);
  if (result) {
			printf("Could not initialize gyro.\n\r");
		//      MPL_LOGE("Could not initialize gyro.\n");
  }else
		printf("initialize successful\n\r");
}

