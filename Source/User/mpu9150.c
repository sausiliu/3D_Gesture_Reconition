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
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "invensense.h"
#include "invensense_adv.h"
#include "eMPL_outputs.h"
#include "mltypes.h"
#include "mpu.h"
//#include "log.h"
#include "packet.h"

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
    } else
        printf("initialize successful\n\r");

    result = inv_init_mpl();
    if (result) {
        printf("Could not initialize MPL.\n");
    } else
        printf("initialize MPL success");
    /* Compute 6-axis and 9-axis quaternions. */
		//--------------------------------------------------------------------
		/*Program Size: Code=33004 RO-data=500 RW-data=204 ZI-data=19548  */
		//--------------------------------------------------------------------
		
    inv_enable_quaternion();/*code:3.48k	100Byte*/
		inv_enable_9x_sensor_fusion();/*code:4.44k	496Bytes*/

		    /* The MPL expects compass data at a constant rate (matching the rate
     * passed to inv_set_compass_sample_rate). If this is an issue for your
     * application, call this function, and the MPL will depend on the
     * timestamps passed to inv_build_compass instead.
     *
     * inv_9x_fusion_use_timestamps(1);
     */

    /* This function has been deprecated.
     * inv_enable_no_gyro_fusion();
     */

    /* Update gyro biases when not in motion.
     * WARNING: These algorithms are mutually exclusive.
     */
//    inv_enable_fast_nomot();/*35668  2.6k  672Byte*/
    /* inv_enable_motion_no_motion(); */
    /* inv_set_no_motion_time(1000); */

    /* Update gyro biases when temperature changes. */
//    inv_enable_gyro_tc();/*34220	1.18k*/
    /* This algorithm updates the accel biases when in motion. A more accurate
     * bias measurement can be made when running the self-test (see case 't' in
     * handle_input), but this algorithm can be enabled if the self-test can't
     * be executed in your application.
     *
     * inv_enable_in_use_auto_calibration();
     */
#ifdef COMPASS_ENABLED
    /* Compass calibration algorithms. */
//    inv_enable_vector_compass_cal();
//    inv_enable_magnetic_disturbance();
#endif
    /* If you need to estimate your heading before the compass is calibrated,
     * enable this algorithm. It becomes useless after a good figure-eight is
     * detected, so we'll just leave it out to save memory.
     * inv_enable_heading_from_gyro();
     */
}


