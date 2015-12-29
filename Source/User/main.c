/*
    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************
*/

/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the standard demo application tasks.
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Fast Interrupt Test" - A high frequency periodic interrupt is generated
 * using a free running timer to demonstrate the use of the
 * configKERNEL_INTERRUPT_PRIORITY configuration constant.  The interrupt
 * service routine measures the number of processor clocks that occur between
 * each interrupt - and in so doing measures the jitter in the interrupt timing.
 * The maximum measured jitter time is latched in the ulMaxJitter variable, and
 * displayed on the LCD by the 'Check' task as described below.  The
 * fast interrupt is configured and handled in the timertest.c source file.
 *
 *
 * "Check" task -  This only executes every five seconds but has the highest
 * priority so is guaranteed to get processor time.  Its main function is to
 * check that all the standard demo tasks are still operational.  Should any
 * unexpected behaviour within a demo task be discovered the 'check' task will
 * write an error to the LCD (via the LCD task).  If all the demo tasks are
 * executing with their expected behaviour then the check task writes PASS
 * along with the max jitter time to the LCD (again via the LCD task), as
 * described above.
 *
 */

/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Library includes. */
#include "stm32f10x_it.h"

/* Demo app includes. */
#include "BlockQ.h"
#include "death.h"
#include "integer.h"
#include "blocktim.h"
#include "partest.h"
#include "semtest.h"
#include "PollQ.h"
#include "flash.h"
#include "comtest2.h"
#include "stm32f10x_usart.h"
#include "serial.h"

/* Task priorities. */
#define mainQUEUE_POLL_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( tskIDLE_PRIORITY + 3 )
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 3 )
#define mainFLASH_TASK_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainCOM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainINTEGER_TASK_PRIORITY           ( tskIDLE_PRIORITY )


/* The check task uses the sprintf function so requires a little more stack. */
#define mainCHECK_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE + 50 )

/* Dimensions the buffer into which the jitter time is written. */
#define mainMAX_MSG_LEN						25

/* The time between cycles of the 'check' task. */
#define mainCHECK_DELAY						( ( TickType_t ) 5000 / portTICK_PERIOD_MS )

/* The number of nano seconds between each processor clock. */
#define mainNS_PER_CLOCK ( ( unsigned long ) ( ( 1.0 / ( double ) configCPU_CLOCK_HZ ) * 1000000000.0 ) )

/* Baud rate used by the comtest tasks. */
#define mainCOM_TEST_BAUD_RATE		( 115200 )

/* The LED used by the comtest tasks. See the comtest.c file for more
information. */
#define mainCOM_TEST_LED			( 3 )

/*-----------------------------------------------------------*/

/*
 * Configure the clocks, GPIO and other peripherals as required by the demo.
 */
static void prvSetupHardware( void );

/*
 * Retargets the C library printf function to the USART.
 */
int fputc( int ch, FILE *f );

/*
 * Checks the status of all the demo tasks then prints a message to the
 * display.  The message will be either PASS - and include in brackets the
 * maximum measured jitter time (as described at the to of the file), or a
 * message that describes which of the standard demo tasks an error has been
 * discovered in.
 *
 * Messages are not written directly to the terminal, but passed to vLCDTask
 * via a queue.
 */
static void vCheckTask( void *pvParameters );
static void vLEDTask( void *pvParameters );
/*
 * Configures the timers and interrupts for the fast interrupt test as
 * described at the top of this file.
 */
extern void vSetupTimerTest( void );
extern signed portBASE_TYPE xSerialPutChar( xComPortHandle, signed char , TickType_t);

/*-----------------------------------------------------------*/

/* The queue used to send messages to the LCD task. */
QueueHandle_t xLCDQueue;

/*-----------------------------------------------------------*/

int main( void )
{
#ifdef DEBUG
    debug();
#endif

    prvSetupHardware();
    vAltStartComTestTasks( mainCOM_TEST_PRIORITY, mainCOM_TEST_BAUD_RATE, mainCOM_TEST_LED );

    xTaskCreate( vLEDTask, "led_test", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
    xTaskCreate( vCheckTask, "Check", mainCHECK_TASK_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );

    /* The suicide tasks must be created last as they need to know how many
    tasks were running prior to their creation in order to ascertain whether
    or not the correct/expected number of tasks are running at any given time. */
    vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );

    /* Configure the timers used by the fast interrupt timer test. */
    vSetupTimerTest();

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* Will only get here if there was not enough heap space to create the
    idle task. */
    return 0;
}
/*-----------------------test-------------------------------*/
void vLEDTask(void * pvParameters)
{
    volatile unsigned long ul;
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    for(;;)
    {
        //	DEBUG_PRINT(message);
        printf("helooooo");
        GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
        vTaskDelay(100);
        GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
        vTaskDelay(100);
    }
}


/*-----------------------------------------------------------*/

static void vCheckTask( void *pvParameters )
{
    TickType_t xLastExecutionTime;
//    xLCDMessage xMessage;
    static signed char cPassMessage[ mainMAX_MSG_LEN ];
    extern unsigned short usMaxJitter;

    xLastExecutionTime = xTaskGetTickCount();
//   xMessage.pcMessage = cPassMessage;

    for( ;; )
    {
        /* Perform this check every mainCHECK_DELAY milliseconds. */
        vTaskDelayUntil( &xLastExecutionTime, mainCHECK_DELAY );

        /* Has an error been found in any task? */

        if( xAreBlockingQueuesStillRunning() != pdTRUE )
        {
            printf("ERROR IN BLOCK Q\n");
        }
        else if( xAreBlockTimeTestTasksStillRunning() != pdTRUE )
        {
            printf("ERROR IN BLOCK TIME\n");
        }
        else if( xAreSemaphoreTasksStillRunning() != pdTRUE )
        {
            printf("ERROR IN SEMAPHORE\n");
        }
        else if( xArePollingQueuesStillRunning() != pdTRUE )
        {
            printf("ERROR IN POLL Q\n");
        }
        else if( xIsCreateTaskStillRunning() != pdTRUE )
        {
            printf("ERROR IN CREATE\n");
        }
        else if( xAreIntegerMathsTaskStillRunning() != pdTRUE )
        {
            printf("ERROR IN MATH\n");
        }
        else if( xAreComTestTasksStillRunning() != pdTRUE )
        {
            printf("ERROR IN COM TEST\n");
        }
        else
        {
            sprintf( ( char * ) cPassMessage, "PASS [%uns]\n", ( ( unsigned long ) usMaxJitter ) * mainNS_PER_CLOCK );
        }

    }
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    /* Start with the clocks in their expected state. */
    RCC_DeInit();

    /* Enable HSE (high speed external clock). */
    RCC_HSEConfig( RCC_HSE_ON );

    /* Wait till HSE is ready. */
    while( RCC_GetFlagStatus( RCC_FLAG_HSERDY ) == RESET )
    {
    }

    /* 2 wait states required on the flash. */
    *( ( unsigned long * ) 0x40022000 ) = 0x02;

    /* HCLK = SYSCLK */
    RCC_HCLKConfig( RCC_SYSCLK_Div1 );

    /* PCLK2 = HCLK */
    RCC_PCLK2Config( RCC_HCLK_Div1 );

    /* PCLK1 = HCLK/2 */
    RCC_PCLK1Config( RCC_HCLK_Div2 );

    /* PLLCLK = 8MHz * 9 = 72 MHz. */
    RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_9 );

    /* Enable PLL. */
    RCC_PLLCmd( ENABLE );

    /* Wait till PLL is ready. */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    /* Select PLL as system clock source. */
    RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );

    /* Wait till PLL is used as system clock source. */
    while( RCC_GetSYSCLKSource() != 0x08 )
    {
    }

    /* Enable GPIOA, GPIOB, GPIOC, GPIOD, GPIOE and AFIO clocks */
    RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOC
                            | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE );

    /* SPI2 Periph clock enable */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );


    /* Set the Vector Table base address at 0x08000000 */
    NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

    NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );

    /* Configure HCLK clock as SysTick clock source. */
    SysTick_CLKSourceConfig( SysTick_CLKSource_HCLK );

    vParTestInitialise();
}


/*-----------------------------------------------------------*/
int fputc( int ch, FILE *f )
{

    xSerialPutChar(NULL, ch, 0);
    return ch;
}

/*-----------------------------------------------------------*/

#ifdef  DEBUG
/* Keep the linker happy. */
void assert_failed( unsigned char* pcFile, unsigned long ulLine )
{
    for( ;; )
    {
    }
}
#endif
