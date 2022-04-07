/******************************************************************************
 *
 * Autorzy:
 *    Piotr Kiełek (Lider)
 *    Szymon Ruciński
 *    Maria Wichrowska
 * Opis:
 *    Gra Refleks z następującymi funkcjonalnościami:
 *    LCD
 *    GPIO
 *    LEDMatrix
 *    Timer
 *    IRQ
 *    Motor
 *    SPI
 *    DAC
 *    PWM
 *****************************************************************************/


#include "../pre_emptive_os/api/osapi.h"
#include "../pre_emptive_os/api/general.h"
#include <printf_P.h>
#include <ea_init.h>
#include <lpc2xxx.h>
#include <consol.h>

#define PROC1_STACK_SIZE 2048
#define PROC2_STACK_SIZE 2048
#define PROC3_STACK_SIZE 2048
#define PROC4_STACK_SIZE 2048
#define INIT_STACK_SIZE  400

static tU8 proc1Stack[PROC1_STACK_SIZE];
static tU8 proc2Stack[PROC2_STACK_SIZE];
static tU8 proc3Stack[PROC3_STACK_SIZE];
static tU8 proc4Stack[PROC4_STACK_SIZE];
static tU8 initStack[INIT_STACK_SIZE];
static tU8 pid1;
static tU8 pid2;
static tU8 pid3;
static tU8 pid4;


static void proc1(void* arg);
static void proc2(void* arg);
static void proc3(void* arg);
static void proc4(void* arg);
static void initProc(void* arg);

void testLedMatrix(void);
void testMotor(void);
void testRGB(void);
void testI2C(void);

volatile tU32 msClock = 0;
extern char startupSound[];


/*****************************************************************************
 *
 * Opis:
 *    Funkcja uruchamiana przy starcie programu
 *
 ****************************************************************************/
int
main(void)
{
  tU8 error;
  tU8 pid;

  //immediately turn off buzzer (if connected)
  IODIR0 |= 0x00000080;
  IOSET0  = 0x00000080;
  
  osInit();
  osCreateProcess(initProc, initStack, INIT_STACK_SIZE, &pid, 1, NULL, &error);
  osStartProcess(pid, &error);
  
  osStart();
  return 0;
}

/*****************************************************************************
 *
 * Opis:
 *    Funkcja, posiadająca dwa zadania:
 *    1) Inicjalizuje DAC, przy pomocy którego następnie odtwarzana jest 
 *    czołówka z Star Wars: New Hope (zapisana w startupSound.c w tablicy)
 *    2) Tworzy procesy dla pozostałych funkcjonalności programu
 *    Proces 2 - Matryca LED, w której toczy się właściwa rozgrywka
 *    Proces 3 - Stepmotor, zsynchronizowany z rozgrywką
 *    Proces 4 - Dioda RGB skonfigurowana przy pomocy PWM, zsynchronizowana 
 *    z rozgrywką
 *
 * Params:
 *    [in] arg - This parameter is not used in this application. 
 *
 ****************************************************************************/
static void
proc1(void* arg)
{
  tU8  error;

  {
    tU32 cnt = 0;
    tU32 i;
    
	  IODIR |= 0x00000380;
	  IOCLR  = 0x00000380;

    //
    //Initialize DAC: AOUT = P0.25
    //
    PINSEL1 &= ~0x000C0000;
    PINSEL1 |=  0x00080000;

    cnt = 0;
    while(cnt++ < 0x17000)
    {
      tS32 val;
      
      val = startupSound[cnt] - 128;
      val = val * 2;
      if (val > 127)  {
        val = 127;
      }
      else if (val < -127) {
        val = -127;
      } 
      DACR = ((val+128) << 8) |  //actual value to output
             (1 << 16);         //BIAS = 1, 2.5uS settling time

      //delay 125 us = 850 for 8kHz, 600 for 11 kHz
      for(i=0; i<520; i++) {
        asm volatile (" nop");
      }
    }

  }
    osCreateProcess(proc2, proc2Stack, PROC2_STACK_SIZE, &pid2, 3, NULL, &error);
    osStartProcess(pid2, &error);
    osCreateProcess(proc3, proc3Stack, PROC3_STACK_SIZE, &pid3, 3, NULL, &error);
    osStartProcess(pid3, &error);
    osCreateProcess(proc4, proc4Stack, PROC4_STACK_SIZE, &pid4, 3, NULL, &error);
    osStartProcess(pid4, &error);
    IOPIN &= ~0x001f0000;
    for(;;)
    {
    }
 }

/*****************************************************************************
 *
 * Opis:
 *    A process entry function 
 *
 * Params:
 *    [in] arg - This parameter is not used in this application. 
 *
 ****************************************************************************/
static void
proc2(void* arg)
{
	testLedMatrix();
}

/*****************************************************************************
 *
 * Opis:
 *    A process entry function 
 *
 * Params:
 *    [in] arg - This parameter is not used in this application. 
 *
 ****************************************************************************/
static void
proc3(void* arg)
{
	for(;;)
	{
		testMotor();
	}
}
/*****************************************************************************
 *
 * Opis:
 *    A process entry function
 *
 * Params:
 *    [in] arg - This parameter is not used in this application.
 *
 ****************************************************************************/
static void
proc4(void* arg)
{
	testRGB();
}

/*****************************************************************************
 *
 * Opis:
 *    The entry function for the initialization process. 
 *
 * Params:
 *    [in] arg - This parameter is not used in this application. 
 *
 ****************************************************************************/
static void
initProc(void* arg)
{
  tU8 error;

  eaInit();
  osCreateProcess(proc1, proc1Stack, PROC1_STACK_SIZE, &pid1, 3, NULL, &error);
  osStartProcess(pid1, &error);

  osDeleteProcess();
}

/*****************************************************************************
 *
 * Opis:
 *    The timer tick entry function that is called once every timer tick
 *    interrupt in the RTOS. Observe that any processing in this
 *    function must be kept as short as possible since this function
 *    execute in interrupt context.
 *
 * Params:
 *    [in] elapsedTime - The number of elapsed milliseconds since last call.
 *
 ****************************************************************************/
void
appTick(tU32 elapsedTime)
{
  msClock += elapsedTime;
}
