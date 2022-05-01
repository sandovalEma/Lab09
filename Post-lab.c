/* 
 * File:   Post-lab.c
 * Author: sando
 *
 * Created on 29 de abril de 2022, 10:52 PM
 */
// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

// ---------- CONSTANTES ---------- //
#define _XTAL_FREQ 8000000      // Frec. 8MHz
#define _tmr0_value 255         // TMR0 a 2 ms

// ---------- VARIABLES ---------- //
 
unsigned short CCPR = 0;        // Variable para ancho de pulso 1
unsigned short CCPR_2 = 0;      // Variable para ancho de pulso 2
unsigned short cont_tmr0;       // Contador de interrupciones de TMR0
unsigned short pulse_w;         // Ancho de pulso variado por POT

// ---------- PROTOTIPO DE FUNCIÓN ---------- //

void setup(void);

//--------------- INTERRUPCIONES --------------- //
void __interrupt() isr(void){
    
    if (PIR1bits.ADIF){
        if (ADCON0bits.CHS == 0){                   // Se verifica canal AN0        
            CCPR = map(ADRESH, 0, 255, 30, 64);     // Valor de ancho de pulso variable
            CCPR1L = (uint8_t)(CCPR>>2);            // Se guardan los 8 bits más significativos en CPR1L
            CCP1CONbits.DC1B = CCPR & 0b11;         // Se guardan los 2 bits menos significativos en DC1B
        }
        else if (ADCON0bits.CHS == 1){              // Se verifica canal AN1        
            CCPR_2 = map(ADRESH, 0, 255, 30, 64);   // Valor de ancho de pulso variable
            CCPR2L = (uint8_t)(CCPR_2>>2);          // Se guardan los 8 bits más significativos en CPR2L
            CCP2CONbits.DC2B0 = CCPR_2 & 0b01;      // Se guardan los 2 bits menos significativos en DC2B
            CCP2CONbits.DC2B1 = CCPR_2 & 0b10;      // Se guardan los 2 bits menos significativos en DC2B
        }
        else if (ADCON0bits.CHS == 2){              // Se verifica canal AN2        
            pulse_w = CCPR_2 = map(ADRESH, 0, 255, 1, 9);   // Valor de ancho de pulso variable
        }
        PIR1bits.ADIF = 0;                          // Limpiamos bandera ADC
    }
     if (INTCONbits.T0IF){ 
       cont_tmr0++;                     // Contador de 0.2 ms
       if (cont_tmr0 == pulse_w) {      // Se verifica si contador TMR0 igual al ancho de pulso
           PORTCbits.RC3 = 0;           // PORTC3 pasa a 0
           return;
       }
       if (cont_tmr0 == 10) {           // Se verifica período de 20 ms
        PORTCbits.RC3 = 1;              // PORTC3 pasa a 1
        cont_tmr0 = 0;                  // Reinicio de contador
       }
       TMR0 = _tmr0_value;              // Reinicio de TMR0
       INTCONbits.T0IF = 0;             // Limpieza de bandera
    }
    return;
}

// --------------- CICLO PRINCIPAL  --------------- //
void main(void) {  
    setup();                                // Llamamos a la función de configuraciones
// ---------- LOOP PRINCIPAL ---------- //                                       
    while(1)
    {
       if(ADCON0bits.GO == 0){              // Si, no hay proceso de conversión
            if(ADCON0bits.CHS == 0)
                 ADCON0bits.CHS = 1;        // Cambio a AN1
            else if(ADCON0bits.CHS == 1)
                 ADCON0bits.CHS = 2;        // Cambio a AN2
            else if(ADCON0bits.CHS == 2)
                 ADCON0bits.CHS = 0;        // Cambio a AN0
        __delay_us(40); //Sample time
        ADCON0bits.GO = 1;                  // Se inicia proceso de conversión
       }
    }
    return;
}

void setup(void){
    ANSEL = 0b00000111;             // Se configura PORTA0, PORTA1 y PORTA2 como entrada analógica
    ANSELH = 0;                     // I/O DIGITALES
      
    TRISA = 0x0011;                 // PORTA0, PORTA1 y PORTA2 como INPUT    
    PORTA = 0;                      // Limpiamos PORTA  

// --------------- CONFIGURACION DE OSCILADOR --------------- //    
    OSCCONbits.SCS = 1;             // Oscilador interno
    OSCCONbits.IRCF = 0b0111;       // 8MHz

// --------------- CONFIGURACION DEL ADC --------------- //    
    ADCON0bits.ADCS = 0b11;         // FRC
    ADCON1bits.VCFG0 = 0;           // Referencia VDD
    ADCON1bits.VCFG1 = 0;           // Referencia VSS
    ADCON0bits.CHS = 0;             // Se selecciona PORTA0/AN0
    ADCON1bits.ADFM = 0;            // Se indica que se tendrá un justificado a la izquierda
    ADCON0bits.ADON = 1;            // Se habilita el modulo ADC
    __delay_us(40);                 // Delay para sample time
    
// --------------- CONFIGURACION DE PWM --------------- //
    TRISCbits.TRISC2 = 1;           // RC2/CCP1 como salida deshabilitado
    TRISCbits.TRISC1 = 1;           // Se deshabilita salida de PWM (CCP2)
    CCP1CON = 0;                    // Se apaga CCP1
    CCP2CON = 0;                    // Se apaga CCP2
    PR2 = 155;                      // Período de 20 ms  
    
// --------------- CONFIGURACION DEL CCP --------------- //
    CCP1CONbits.P1M = 0;            // Modo single output
    CCP1CONbits.CCP1M = 0b1100;     // Modo PWM
    CCP2CONbits.CCP2M = 0b1100;     // Modo PWM
    //Servo 1
    CCPR1L = 30>>2;                     
    CCP1CONbits.DC1B = 30 & 0b11;       

    //Servo 2
    CCPR2L = 30>>2;                 //Ciclo de trabajo base pues se va a variar
    CCP2CONbits.DC2B0 = 30 & 0b01;      
    CCP2CONbits.DC2B1 = (30 & 0b10)>>1; 

// --------------- CONFIGURACION DE TIMER 2 --------------- //
    PIR1bits.TMR2IF = 0;        // Limpieza de bandera del TMR2
    T2CONbits.T2CKPS = 0b11;    // Prescaler 1:16
    T2CONbits.TMR2ON = 1;       // Se enciende TMR2
    while(!PIR1bits.TMR2IF);    // Se espera un ciclo del TMR2
    PIR1bits.TMR2IF = 0;        // Limpieza de bandera del TMR2 nuevamente
    
    TRISCbits.TRISC3 = 0;       // Se habilita salida de PWM (Con TMR0)
    TRISCbits.TRISC2 = 0;       // Se habilita salida de PWM (CCP2)
    TRISCbits.TRISC1 = 0;       // Se habilita salida de PWM (CCP2)
    
    // --------------- CONFIGURACION DE TIMER 0 --------------- //    
    OPTION_REGbits.T0CS = 0;    // Uso de reloj interno
    OPTION_REGbits.PSA = 0;     // Uso de Prescaler con TMR0
    OPTION_REGbits.PS = 0b0111; // Prescaler de 1:256 
    TMR0 = _tmr0_value;         // preset for timer register a 0.2 ms

// --------------- CONFIGURACION DE INTERRUPCIONES --------------- //    
    INTCONbits.GIE = 1;         // Habilitamos interrupciones globales
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion del ADC
    PIR1bits.ADIF = 0;          // Limpiamos la bandera del ADC
    INTCONbits.PEIE = 1;        // Hbilitamos interrupciones de periféricos
    return;
 }
