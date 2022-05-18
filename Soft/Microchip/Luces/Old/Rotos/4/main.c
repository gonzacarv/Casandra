/*======================================================================================================
||   Proyecto: Casandra v1.0                                                                          ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                    ||
||   Fecha: 06/2014                                                                                   ||
||   Compilador: PCWHD v5.025 (www.ccsinfo.com)                                                       ||
||   Fuente: http://                                                                                  ||
||                                                                                                    ||
|| Firmware de modulos actuadores de luces correspondientes al sistema domotico CASANDRA. Capacidad   ||
|| para actuar sobre 12 consumos y leer 12 entradas digitales + 2 analogicas. Comunicacion sobre par  ||
|| trensado usando el transceptor SN75176.                                                            ||
||                                                                                                    ||
======================================================================================================*/

#include <16F877A.h> // N1
#fuses XT, NOWDT, NOPROTECT, NOLVP, PUT//, NOBROWNOUT // Opciones de configuración
#use delay(clock=4000000)  //Reloj de 4MHz

#byte TMR1H = 0x0F // Le pongo nombre al registro alto de timer 1
#byte TMR1L = 0x0E // Le pongo nombre al registro bajo de timer 1

////////// Tipo de entradas y salidas (predefinidas con TRIS) ///////////

/*#use fast_io(a)
#use fast_io(b)
#use fast_io(c)
#use fast_io(d)
*/
//#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7, enable=PIN_C5) // Comunicacion serial

////////// Otros nombres ///////////////
#define LED PIN_D7 // Piloto led del modulo
#define LEDON output_low(LED)     /// Comando rapido para el encendido y 
#define LEDOFF output_high(LED)   /// apagado del piloto LED.
//#define CruX PIN_B0 // Entrada de cruce por cero


void main(){
while (1){
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
LEDON;
delay_ms(200);
LEDOFF;
delay_ms(200);
}
}

