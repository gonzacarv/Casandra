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
#fuses XT, NOWDT, NOPROTECT, NOLVP, PUT//, NOBROWNOUT // Opciones de configuraci�n
#use delay(clock=4000000)  //Reloj de 4MHz

#byte TMR1H = 0x0F // Le pongo nombre al registro alto de timer 1
#byte TMR1L = 0x0E // Le pongo nombre al registro bajo de timer 1

////////// Tipo de entradas y salidas (predefinidas con TRIS) ///////////

#use fast_io(a)
#use fast_io(b)
#use fast_io(c)
#use fast_io(d)

#use rs232(baud=9600, xmit=PIN_C6, rcv=PIN_C7, enable=PIN_C5) // Comunicacion serial

//////////////////////////////Corresponde a MODULO NUMERO 1//////////////////////////////////////////
#define LUZ0 81 // Para modulo 5
#define LUZ1 82 // Para modulo 5
#define LUZ2 83 // Para modulo 5
#define LUZ3 84 // Para modulo 5
#define LUZ4 85 // Para modulo 5
#define LUZ5 86 // Para modulo 5
#define LUZ6 87 // Para modulo 5
#define LUZ7 88 // Para modulo 5
#define LUZ8 89 // Para modulo 5
#define LUZ9 90 // Para modulo 5
#define LUZ10 91 // Para modulo 5
#define LUZ11 92 // Para modulo 5
#define TODOS 100  // Para TODOS

//#define RESTA 33 // Para modulo 1
//#define RESTA 45 // Para modulo 2
//#define RESTA 57 // Para modulo 3
//#define RESTA 69 // Para modulo 4
#define RESTA 81 // Para modulo 5

////////// Nombres para las entradas ////////
#define IN1 PIN_D1
#define IN2 PIN_D0
#define IN3 PIN_C3
#define IN4 PIN_C2
#define IN5 PIN_C1
#define IN6 PIN_C0
#define IN7 PIN_D6
#define IN8 PIN_D5
#define IN9 PIN_D4
#define IN10 PIN_C4
#define IN11 PIN_D3
#define IN12 PIN_D2

////////// Nombres para las salidas ////////
#define OUT1 PIN_B1
#define OUT2 PIN_B2
#define OUT3 PIN_B3 
#define OUT4 PIN_B4
#define OUT5 PIN_B5
#define OUT6 PIN_B6
#define OUT7 PIN_B7
#define OUT8 PIN_E1
#define OUT9 PIN_E2
#define OUT10 PIN_A2
#define OUT11 PIN_E0
#define OUT12 PIN_A5

////////// Nombres para entradas analogicas del ADC ////////
#define ANA1 PIN_A0
#define ANA2 PIN_A1
#define ANA3 PIN_A3

////////// Otros nombres ///////////////
#define LED PIN_D7 // Piloto led del modulo
#define LEDON output_low(LED)     /// Comando rapido para el encendido y 
#define LEDOFF output_high(LED)   /// apagado del piloto LED.
#define CruX PIN_B0 // Entrada de cruce por cero

////////// Encabezado de funciones ///////////////
void Cruce(); // Funcion de cruce por 0 que reinicia el timer, se ejecuta por interrupcion de B0
void Comunica(); // Funcion de lectura de puerto serial y armado del comando, se ejecuta despues de que BusData tiene el comando lleno
void Cambio(); // Cambio en la configuracion de alguna luz (incluida la variacion de tiempos cortos, debe ser periodica)
void Entradas(); // Funcion de lectura de las entradas (periodica)
void Analogicos(); // Funcion de toma de valores analogicos
void Accion(); // Funcion de Salida de luces (solo se ejecuta por interrupcion del timer2)
void BusData(); // LLego algo al bus de datos (solo se ejecuta por interrupcion de byte esperando en buffer)
void HolaMundo(); // Funcion de inicio cargamos variables y configuraciones


////////// Variables Globales ///////////////
int Comando[4]; // Arreglo que contiene el comando una vez armado
int LuzIntensidad[12]; // Arreglo de intensidad por defecto de luces (se guarda la config)
int LuzAccion[12]; // Arreglo de intensidad instantanea de luces
int LuzEstado[12]; // Arreglo de Estado de luces
short TeclaEstado[12]; // Arreglo de Estado de las teclas
short Teclas; // Bandera de bloqueo de teclas
long Retraso; // Contador de retraso de cambio de estado en las telcas de luz para evitar ruido electromagnetico
int i=0; // Contador puntero para armar el comando recibido por el bus
int j=0; // Contador en interrupcion de cruce por cero para tiempos largos


void main(){

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

HolaMundo(); // Comenzamos la ejecucion preguntando configuracion y como nos apagamos la ultima vez

/* //Para que funcione hay que deshabilitar la funcion de barrio de la salida
while (1){
LEDON;
delay_ms(500);
LEDOFF;
delay_ms(500);
}*/

while (1){ /////////////// LOOP PRINCIPAL DE MAIN ///////////////////////////////

if (Teclas == True) Entradas(); // Leemos entradas
Cambio(); // Cambios periodicos
restart_wdt(); // Reiniciamos el perro
LEDOFF;
} // while 1
} // main


void Comunica(){
restart_wdt(); // Reiniciamos el perro
i=0;
if (( (Comando[0]) + (Comando[1]) ) == Comando[2]){ // Prueba de checksum
if (((Comando[0])-RESTA) < 13) {  // Solo Luces individuales
      if (Comando[1] < 40){
      LuzIntensidad[(Comando[0]-RESTA)]=Comando[1];
      } else {
      LuzEstado[(Comando[0]-RESTA)]=Comando[1];
      }
}// If

if ((Comando[0]) == TODOS){ // Para todas las luces
int Cuenta;
   for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
      if (Comando[1] < 40) {
      LuzIntensidad[Cuenta]=Comando[1];
      } else {
      LuzEstado[Cuenta]=Comando[1];
      }
} //For
} // If
} // if checksum
} //fn


void Entradas() {

while ( (input(IN1)) ^ TeclaEstado[0] ){  // Si cambia la tecla
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[0]=input(IN1); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[0]) == 90) || ((LuzEstado[0]) == 102))) {
printf("%c%c%c%c", 250, LUZ0, 90, LUZ0 + 90); //PRENDO
LuzEstado[0]=90;
} else {
printf("%c%c%c%c", 250, LUZ0, 80, LUZ0 + 80); //APAGO
LuzEstado[0]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN2)) ^ TeclaEstado[1] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[1]=input(IN2); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[1]) == 90) || ((LuzEstado[1]) == 102))) {
printf("%c%c%c%c", 250, LUZ1, 90, LUZ1 + 90); //PRENDO
LuzEstado[1]=90;
} else {
printf("%c%c%c%c", 250, LUZ1, 80, LUZ1 + 80); //APAGO
LuzEstado[1]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN3)) ^ TeclaEstado[2] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[2]=input(IN3); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[2]) == 90) || ((LuzEstado[2]) == 102))) {
printf("%c%c%c%c", 250, LUZ2, 90, LUZ2 + 90); //PRENDO
LuzEstado[2]=90;
} else {
printf("%c%c%c%c", 250, LUZ2, 80, LUZ2 + 80); //APAGO
LuzEstado[2]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN4)) ^ TeclaEstado[3] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[3]=input(IN4); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[3]) == 90) || ((LuzEstado[3]) == 102))) {
printf("%c%c%c%c", 250, LUZ3, 90, LUZ3 + 90); //PRENDO
LuzEstado[3]=90;
} else {
printf("%c%c%c%c", 250, LUZ3, 80, LUZ3 + 80); //APAGO
LuzEstado[3]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN5)) ^ TeclaEstado[4] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[4]=input(IN5); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[4]) == 90) || ((LuzEstado[4]) == 102))) {
printf("%c%c%c%c", 250, LUZ4, 90, LUZ4 + 90); //PRENDO
LuzEstado[4]=90;
} else {
printf("%c%c%c%c", 250, LUZ4, 80, LUZ4 + 80); //APAGO
LuzEstado[4]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN6)) ^ TeclaEstado[5] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[5]=input(IN6); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[5]) == 90) || ((LuzEstado[5]) == 102))) {
printf("%c%c%c%c", 250, LUZ5, 90, LUZ5 + 90); //PRENDO
LuzEstado[5]=90;
} else {
printf("%c%c%c%c", 250, LUZ5, 80, LUZ5 + 80); //APAGO
LuzEstado[5]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN7)) ^ TeclaEstado[6] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[6]=input(IN7); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[6]) == 90) || ((LuzEstado[6]) == 102))) {
printf("%c%c%c%c", 250, LUZ6, 90, LUZ6 + 90); //PRENDO
LuzEstado[6]=90;
} else {
printf("%c%c%c%c", 250, LUZ6, 80, LUZ6 + 80); //APAGO
LuzEstado[6]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN8)) ^ TeclaEstado[7] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[7]=input(IN8); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[7]) == 90) || ((LuzEstado[7]) == 102))) {
printf("%c%c%c%c", 250, LUZ7, 90, LUZ7 + 90); //PRENDO
LuzEstado[7]=90;
} else {
printf("%c%c%c%c", 250, LUZ7, 80, LUZ7 + 80); //APAGO
LuzEstado[7]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN9)) ^ TeclaEstado[8] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[8]=input(IN9); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[8]) == 90) || ((LuzEstado[8]) == 102))) {
printf("%c%c%c%c", 250, LUZ8, 90, LUZ8 + 90); //PRENDO
LuzEstado[8]=90;
} else {
printf("%c%c%c%c", 250, LUZ8, 80, LUZ8 + 80); //APAGO
LuzEstado[8]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN10)) ^ TeclaEstado[9] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[9]=input(IN10); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[9]) == 90) || ((LuzEstado[9]) == 102))) {
printf("%c%c%c%c", 250, LUZ9, 90, LUZ9 + 90); //PRENDO
LuzEstado[9]=90;
} else {
printf("%c%c%c%c", 250, LUZ9, 80, LUZ9 + 80); //APAGO
LuzEstado[9]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN11)) ^ TeclaEstado[10] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[10]=input(IN11); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[10]) == 90) || ((LuzEstado[7]) == 102))) {
printf("%c%c%c%c", 250, LUZ10, 90, LUZ10 + 90); //PRENDO
LuzEstado[10]=90;
} else {
printf("%c%c%c%c", 250, LUZ10, 80, LUZ10 + 80); //APAGO
LuzEstado[10]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN12)) ^ TeclaEstado[11] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
restart_wdt(); // Reiniciamos el perro
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[11]=input(IN12); // Gurdamos el Cambio
LEDON;
if (!(((LuzEstado[11]) == 90) || ((LuzEstado[11]) == 102))) {
printf("%c%c%c%c", 250, LUZ11, 90, LUZ11 + 90); //PRENDO
LuzEstado[11]=90;
} else {
printf("%c%c%c%c", 250, LUZ11, 80, LUZ11 + 80); //APAGO
LuzEstado[11]=80;
} //Else
} // If
} // While
Retraso = 0;

} //fn

void Cambio(){
restart_wdt(); // Reiniciamos el perro
int Cuenta;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
      if (LuzEstado[Cuenta] == 80) {
         LuzAccion[Cuenta] = 95; // Apagado
      } // If estado 80
      if (LuzEstado[Cuenta] == 90) {
         if (LuzIntensidad[Cuenta] == 0) {
         LuzEstado[Cuenta] = 102; // Maximo
         } else {
         LuzAccion[Cuenta] = LuzIntensidad[Cuenta]; // Segun intensidad
         }
      } // If estado 90
      if (LuzEstado[Cuenta] == 102) {
      LuzAccion[Cuenta] = 0; // MAXIMO
      }
      if (LuzEstado[Cuenta] == 95) {
      LuzAccion[Cuenta] = 17; // MEDIO
      }
      if (LuzEstado[Cuenta] == 96) {
      LuzAccion[Cuenta] = 23; //MINIMO
      }
      if ((LuzEstado[Cuenta] == 120) || (LuzEstado[Cuenta] == 70)){ // ONOFF1 mas lento de todos
         if (bit_test(j,7)) { // Encendido
         LuzEstado[Cuenta] = 120;
         LuzAccion[Cuenta] = 0;
         }
         else {
         LuzEstado[Cuenta] = 70;
         LuzAccion[Cuenta] = 95;
         }
      } // If ONOFF 1
      
      if ((LuzEstado[Cuenta] == 121) || (LuzEstado[Cuenta] == 71)){ // ONOFF2 mediano
         if (bit_test(j,6)) { // Encendido
         LuzEstado[Cuenta] = 121;
         LuzAccion[Cuenta] = 0;
         }
         else {
         LuzEstado[Cuenta] = 71;
         LuzAccion[Cuenta] = 95;
         }
         } // If ONOFF 2
      
      if ((LuzEstado[Cuenta] == 122) || (LuzEstado[Cuenta] == 72)){ // ONOFF3 rapido
         if (bit_test(j,5)) { // Encendido
         LuzEstado[Cuenta] = 122;
         LuzAccion[Cuenta] = 0;
         }
         else {
         LuzEstado[Cuenta] = 72;
         LuzAccion[Cuenta] = 95;
         }
         
      } // If ONOFF 1
      if ((LuzEstado[Cuenta] == 123) || (LuzEstado[Cuenta] == 73)){ // ONOFF4 rapidisimo
         if (bit_test(j,4)) { // Encendido
         LuzEstado[Cuenta] = 123;
         LuzAccion[Cuenta] = 0;
         }
         else {
         LuzEstado[Cuenta] = 73;
         LuzAccion[Cuenta] = 95;
         }
      } // If ONOFF 1
      
/*  OLAS PENDIENTES DE IMPLEMENTAR

      if (LuzEstado[Cuenta] == 130) ii =// OLA 1 Lentisima
         if (bit_test(j,7)) ii = 0;
         else ii = 10000

      
      if (LuzEstado[Cuenta] == 131){ // OLA 2 normal
         if (bit_test(j,7)) ii = 0;
         else ii = 10000
      } // If ONOFF 1
      
      if (LuzEstado[Cuenta] == 132){ // OLA 3 rapida
         if (bit_test(j,7)) ii = 0;
         else ii = 10000
      } // If ONOFF 1
*/

      if (LuzEstado[Cuenta] == 140){ // ALARMA ON
         Teclas = false;
         LuzEstado[Cuenta] = 122;
      } // If ONOFF 3 --> Alarma

      if (LuzEstado[Cuenta] == 141){ // ALARMA OFF
         Teclas = true;
         LuzEstado[Cuenta] = 90;
      } // If ONOFF 3 --> Alarma
} // FOR
} // fn
 
void HolaMundo(){ // Inicio & configuracion de variables

/////////  Inicio el WatchDog  ///////////////
//setup_wdt(WDT_2304MS);
//restart_wdt(); // Reiniciamos el perro

//////////// LEDs de inicio /////////////
LEDON;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(200);
restart_wdt(); // Reiniciamos el perro


/////////  Puertos  ///////////////
set_tris_a(0b00001011);  // Puerto A como salidas, menos A0, A1 y A3 que son entradas analogicas.
set_tris_b(0b00000001);  // Puerto B, todas salidas, menos B0 que es el cruce por cero
set_tris_c(0b10011111);  // Puerto C, de 0 a 4 -> entradas; 5 y 6 para RS232 salidas; C7 es RX -> Entrada
set_tris_d(0b01111111);  // Puerto D, todas entradas menos el Piloto (D7)

/////////  Conversor AD  ///////////////
setup_adc_ports(RA0_RA1_Ra3_ANALOG);  // Entradas analogicas.
setup_adc(ADC_CLOCK_INTERNAL);  // Barrido �Hace falta?

/////////  Temporizadores  ///////////////
setup_timer_1(T1_INTERNAL | T1_DIV_BY_1); // Contador del tiempo de cruce por cero
setup_timer_2(T2_DIV_BY_1, 200, 1); // Contador de entrada a la funcion Accion(); 185

/////////  Interrupciones  ///////////////
enable_interrupts(global);
//disable_interrupts(global);
enable_interrupts(int_ext); // Interrupcion para el cruce por 0
enable_interrupts(int_rda); // Interrupcion de llegada de datos
enable_interrupts(int_timer2); // frecuencia de entrada al barrido de focos

Teclas = true; // Para que se puedan usar las teclas

/////////  Inicio valores de luces por defecto por si el servidor no contesta  ///////////////

int Cuenta;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
Luzintensidad[Cuenta] = 0;
LuzEstado[Cuenta] = 80;  // Por defecto todas las luces apagadas
}

/*////////  Inicio de valores particulares por si el servidor no contesta  ///////////////
LuzEstado[0] = 80;
LuzEstado[1] = 80;
LuzEstado[2] = 80;
LuzEstado[3] = 80;
LuzEstado[4] = 80;
LuzEstado[5] = 80;
LuzEstado[6] = 80;
LuzEstado[7] = 80;
LuzEstado[8] = 80;
LuzEstado[9] = 80;
LuzEstado[10] = 80;
LuzEstado[11] = 80;
*/

LEDON;
printf("%c%c%c%c", 250, 250, 184, 53); //Identificacion de inicio (49 es 1; 50 es 2; 51 es 3; 52 es 4 y 53 es 5)


}

#int_ext
void Cruce(){
set_timer1(0); // Reseteamos el timer porque cruzamos por 0
++j; // variable de tiempos largos
}

#int_rda
void BusData(){
LEDON;
if (kbhit()){
Comando[i]=getc();
if (Comando[i] == 250) i=0;
else ++i;
if (i==3) Comunica(); // Tenemos un comando nuevo
}
}

#int_timer2
void Accion(){ // Funcion de actuacion de luces de alta velocidad (lo mas depurada posible)

if (TMR1H == LuzAccion[0]) output_high(OUT1);
if (TMR1H == LuzAccion[1]) output_high(OUT2);
if (TMR1H == LuzAccion[2]) output_high(OUT3);
if (TMR1H == LuzAccion[3]) output_high(OUT4);
if (TMR1H == LuzAccion[4]) output_high(OUT5);
if (TMR1H == LuzAccion[5]) output_high(OUT6);
if (TMR1H == LuzAccion[6]) output_high(OUT7);
if (TMR1H == LuzAccion[7]) output_high(OUT8);
if (TMR1H == LuzAccion[8]) output_high(OUT9);
if (TMR1H == LuzAccion[9]) output_high(OUT10);
if (TMR1H == LuzAccion[10]) output_high(OUT11);
if (TMR1H == LuzAccion[11]) output_high(OUT12);
        
if (TMR1H == LuzAccion[0] + 39) output_high(OUT1);
if (TMR1H == LuzAccion[1] + 39) output_high(OUT2);
if (TMR1H == LuzAccion[2] + 39) output_high(OUT3);
if (TMR1H == LuzAccion[3] + 39) output_high(OUT4);
if (TMR1H == LuzAccion[4] + 39) output_high(OUT5);
if (TMR1H == LuzAccion[5] + 39) output_high(OUT6);
if (TMR1H == LuzAccion[6] + 39) output_high(OUT7);
if (TMR1H == LuzAccion[7] + 39) output_high(OUT8);
if (TMR1H == LuzAccion[8] + 39) output_high(OUT9);
if (TMR1H == LuzAccion[9] + 39) output_high(OUT10);
if (TMR1H == LuzAccion[10] + 39) output_high(OUT11);
if (TMR1H == LuzAccion[11] + 39) output_high(OUT12);

if (TMR1H == 35){    
if (LuzEstado[0]<99) output_low(OUT1);  
if (LuzEstado[1]<99) output_low(OUT2);
if (LuzEstado[2]<99) output_low(OUT3);
if (LuzEstado[3]<99) output_low(OUT4);
if (LuzEstado[4]<99) output_low(OUT5);
if (LuzEstado[5]<99) output_low(OUT6);
if (LuzEstado[6]<99) output_low(OUT7);
if (LuzEstado[7]<99) output_low(OUT8);
if (LuzEstado[8]<99) output_low(OUT9);
if (LuzEstado[9]<99) output_low(OUT10);
if (LuzEstado[10]<99) output_low(OUT11);
if (LuzEstado[11]<99) output_low(OUT12);
}

if (TMR1H == 74){    
if (LuzEstado[0]<99) output_low(OUT1);
if (LuzEstado[1]<99) output_low(OUT2);
if (LuzEstado[2]<99) output_low(OUT3);
if (LuzEstado[3]<99) output_low(OUT4);
if (LuzEstado[4]<99) output_low(OUT5);
if (LuzEstado[5]<99) output_low(OUT6);
if (LuzEstado[6]<99) output_low(OUT7);
if (LuzEstado[7]<99) output_low(OUT8);
if (LuzEstado[8]<99) output_low(OUT9);
if (LuzEstado[9]<99) output_low(OUT10);
if (LuzEstado[10]<99) output_low(OUT11);
if (LuzEstado[11]<99) output_low(OUT12);
}

} // Fn

