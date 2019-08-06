/***********************************************************************************
 *  LIBRERIAS INCLUIDAS
 **********************************************************************************/
#include "main.h"
#include <msp430g2553.h>
#include <stdlib.h>
#include <string.h>

/***********************************************************************************
 *  DEFINICIONES
 **********************************************************************************/
#define FREQ                                                               (125U)
#define MINIMUM_RATE                                                       (5U)
#define MAXIMUM_RATE                                                       (FREQ-5)
#define MAXIMUM_ARRAY_DATA                                                 (2U)
#define ASCII_MIN_NUMBER                                                   (47U)
#define ASCII_MAX_NUMBER                                                   (58U)

/* Tipo de dato que almacena la informacion. */
typedef struct data{
    char               cDatoEntrada[2];
    unsigned int       u8CuentaDeDatos;
}T_DATA;

/***********************************************************************************
 *  DECLARACION DE FUNCIONES LOCALES
 **********************************************************************************/
static void Init_Watchdog   (void);
static void Init_Clock      (void);
static void Init_Port       (void);
static void Init_UART       (void);
static void Init_TimerA0    (void);
static void ProcesarDatos   (void);

/***********************************************************************************
 *  VARIABLES GLOBABLES AL MODULO
 **********************************************************************************/
static T_DATA  DatosDeEntrada;
static boolean RxFlag;
static char    RxBuffer;

/**********************************************************************************
*   FUNCION:       main()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
void main(void) {
    /* Inicializacion del watchdog.                 */
    Init_Watchdog();
    /* Inicializar reloj.                           */
    Init_Clock();
    /* Inicializar puertos de salida.               */
    Init_Port();
    /* Inicializar UART.                            */
    Init_UART();
    /* Inicializar Timer A0.                        */
    Init_TimerA0();
    /* Inicializar variables globales al modulo.    */
    memset(&DatosDeEntrada, 0U, sizeof(T_DATA));
    RxFlag                = FALSE;
    /* Habilitar interrupciones por UART.           */
    IE2 |= UCA0RXIE;
    _BIS_SR(GIE);
    /* Loop infinito. */
    for(;;)
    {
        /* Espera la interrupcion. */
        if(TRUE == RxFlag)
        {
            /* Se recibio algo. Procesar buffer. */
            ProcesarDatos();
            /* Limpiar bandera de recepcion.     */
            RxFlag = FALSE;
        }
    }
}

/**********************************************************************************
*   FUNCION:       Init_Watchdog()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void Init_Watchdog(void)
{
    /* Detener el watchdog. */
    WDTCTL = WDTPW | WDTHOLD;
}

/**********************************************************************************
*   FUNCION:       Init_Clock()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void Init_Clock(void)
{
    /* Configurar registros del reloj. */
    DCOCTL      = CALDCO_1MHZ;
    BCSCTL1     = CALBC1_1MHZ;
    BCSCTL2     = SELM0 + DIVS_0;
    BCSCTL3     = LFXT1S_2;
}

/**********************************************************************************
*   FUNCION:       Init_Port()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void Init_Port(void)
{
    /* Configuracion de la terminal 6 del puerto uno. */
    P1DIR       =  BIT6;
    P1SEL       =  BIT6;
    P1OUT      &= ~BIT6;
    /* Configuracion de las terminales de salida para comunicaciones UART.    *
     * Rx -> P1.1                                                             *
     * Tx -> P1.2                                                             */
    P1SEL      |= BIT1 + BIT2 ;
    P1SEL2      = BIT1 + BIT2;
    P1DIR      |= BIT1;
}

/**********************************************************************************
*   FUNCION:       Init_UART()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void Init_UART(void)
{
    /* Configuracion de UART para usar como reloj fuente el SMCLK.   *
     * Baudrate: 9600 MHz.                                           */
    UCA0CTL0    = 0x00U;
    UCA0CTL1   |= UCSSEL_2;
    UCA0BR0     = 104U;
    UCA0BR1     = 0U;
    UCA0MCTL    = UCBRS_1;
    UCA0CTL1   &= ~UCSWRST;
}

/**********************************************************************************
*   FUNCION:       Init_TimerA0()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void Init_TimerA0(void)
{
    TA0CTL      = TACLR;
    TA0R        = 0U;
    TA0CCR0     = FREQ;
    TA0CCR1     = MAXIMUM_RATE;
    TA0CCTL0    = CCIE;
    TA0CCTL1    = OUTMOD_2;
    TA0CTL      = TASSEL_2 + MC_1 + ID_3;
}

/**********************************************************************************
*   FUNCION:       ProcesarDatos()
*   DESCRIPCION:   .
*   INPUT(S):      None.
*   OUTPUT(S):     None.
*   RETURN VALUE:  None.
***********************************************************************************/
static void ProcesarDatos(void)
{
    /* Declaracion de variables locales. */
    unsigned int Valor_PWM;
    /* Revisar el contenido del buffer.                                                              *
     * La secuencia de reception tiene dos pasos: 1) Recibir el valor del PWM. 2) Procesar el valor. *
     * Para procesar el valor el dato obtenido debe ser un '\n'                                      */
    if('\n' != RxBuffer)
    {
        /* Validar dato en buffer */
        if((MAXIMUM_ARRAY_DATA > DatosDeEntrada.u8CuentaDeDatos) &&
           (ASCII_MIN_NUMBER   < RxBuffer)                       &&
           (ASCII_MAX_NUMBER   > RxBuffer))
        {
            /* Agregar dato al arreglo. */
            DatosDeEntrada.cDatoEntrada[DatosDeEntrada.u8CuentaDeDatos] = RxBuffer;
            /* Incrementar cuenta de elementos recibidos. */
            DatosDeEntrada.u8CuentaDeDatos++;
        }
        else
        {
            /* Si el dato no es valido, inicializar estructura. */
            memset(&DatosDeEntrada, 0U, sizeof(T_DATA));
        }
    }
    else
    {
        /* El dato recibido indica procesar el valor.  *
         * Validar cuenta y datos.                     */
        if((MAXIMUM_ARRAY_DATA >= DatosDeEntrada.u8CuentaDeDatos) && ('\0' != DatosDeEntrada.cDatoEntrada[0U]))
        {
            /* Convertir el dato recibido en cuentas del timer (duty cycle). */
            Valor_PWM = (unsigned int)((atoi(DatosDeEntrada.cDatoEntrada))*128U)/100U;
            /* Verificar limites. */
            if(Valor_PWM < MINIMUM_RATE)
            {
                /* El valor es menor al minimo permitido, actualizar señal de control con el minimo permitido. */
                Valor_PWM = MINIMUM_RATE;
            }
            else if(Valor_PWM > MAXIMUM_RATE)
            {
                /* El valor es mayor al minimo permitido, actualizar señal de control con el mayor permitido. */
                Valor_PWM = MAXIMUM_RATE;
            }
            else
            {
                /* No hacer nada, evita un error de compilacion. */
            }
            /* Actualizar valor del PWM en el registro de cuenta y comparacion del timer A0. */
            TA0CCR1 = Valor_PWM;
            /* Iniciar estructura de datos de entrada. */
            memset(&DatosDeEntrada, 0U, sizeof(T_DATA));
        }
    }
}

/* todo revisar si es necesaria esta interrupcion. */
#pragma vector=TIMER0_A0_VECTOR
   __interrupt void Timer0_A0 (void)
    {
       // Clean interruption flag
       TA0CCTL0 &= ~CCIFG;
    }

#pragma vector=USCIAB0RX_VECTOR
    __interrupt void USCI0RX_ISR(void)
    {
        /* Esperar a que el buffer este libre. */
        while (!(IFG2&UCA0TXIFG));
        /* No sobre escribir en el buffer si la variable no ah sido liberada. */
        if(TRUE != RxFlag)
        {
            /* Recuperar el dato recibido.      */
            RxBuffer = UCA0RXBUF;
            /* Actualizar estado de la bandera. */
            RxFlag   = TRUE;
        }
    }
