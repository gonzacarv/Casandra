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

#include <16F877A.h> // N4
#fuses XT, WDT, NOPROTECT, NOLVP, PUT//, NOBROWNOUT // Opciones de configuraci�n
#use delay(clock=4000000)  //Reloj de 4MHz

#byte TMR1H = 0x0F // Le pongo nombre al registro alto de timer 1
#byte TMR1L = 0x0E // Le pongo nombre al registro bajo de timer 1

////////// Tipo de entradas y salidas (predefinidas con TRIS) ///////////

#use fast_io(a)
#use fast_io(b)
#use fast_io(c)
#use fast_io(d)

#use rs232(baud=2400, xmit=PIN_C6, rcv=PIN_C7, enable=PIN_C5, ERRORS) // Comunicacion serial

//////////////////////////////Corresponde a MODULO NUMERO 1//////////////////////////////////////////
#define LUZ0 69 // Para modulo 4
#define LUZ1 70 // Para modulo 4
#define LUZ2 71 // Para modulo 4
#define LUZ3 72 // Para modulo 4
#define LUZ4 73 // Para modulo 4
#define LUZ5 74 // Para modulo 4
#define LUZ6 75 // Para modulo 4
#define LUZ7 76 // Para modulo 4
#define LUZ8 77 // Para modulo 4
#define LUZ9 78 // Para modulo 4
#define LUZ10 79 // Para modulo 4
#define LUZ11 80 // Para modulo 4
#define TODOS 100  // Para TODOS

//#define RESTA 33 // Para modulo 1
//#define RESTA 45 // Para modulo 2
//#define RESTA 57 // Para modulo 3
#define RESTA 69 // Para modulo 4
//#define RESTA 81 // Para modulo 5

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
void BusDatax(); // LLego algo al bus de datos (solo se ejecuta por interrupcion de byte esperando en buffer)
void HolaMundo(); // Funcion de inicio cargamos variables y configuraciones
void BusData(int C1,int C2); // Hablamos por el bus con mejor repeticion


////////// Variables Globales ///////////////
int Comando[4]; // Arreglo que contiene el comando una vez armado
int LuzIntensidad[12]; // Arreglo de intensidad por defecto de luces (se guarda la config)
int LuzAccion[12]; // Arreglo de intensidad instantanea de luces
int LuzAccion2[12]; // Arreglo de intensidad instantanea de luces 2
int LuzEstado[12]; // Arreglo de Estado de luces
short TeclaEstado[12]; // Arreglo de Estado de las teclas
short Teclas; // Bandera de bloqueo de teclas
long Retraso; // Contador de retraso de cambio de estado en las telcas de luz para evitar ruido electromagnetico
int i=0; // Contador puntero para armar el comando recibido por el bus
int j=0; // Contador en interrupcion de cruce por cero para tiempos largos


void main(){

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


void BusData(int C1,int C2) {
int Retrasox;
Retrasox = 1;
   while (Retrasox != 255){
   ++Retrasox;
   restart_wdt(); // Reiniciamos el perro
   }
   Retrasox = 1;
   printf("%c%c%c%c", 250, C1, C2, C1+C2); 
   while (Retrasox != 255){
   ++Retrasox;
   restart_wdt(); // Reiniciamos el perro
   }
   Retrasox = 1;/*
   printf("%c%c%c%c", 250, C1, C2, C1+C2);
   while (Retraso != 255){
   ++Retraso;
   restart_wdt(); // Reiniciamos el perro
   }
   Retraso = 1;
   printf("%c%c%c%c", 250, C1, C2, C1+C2); 
   while (Retraso != 255){
   ++Retraso;
   restart_wdt(); // Reiniciamos el perro
   }
   Retraso = 1;
   printf("%c%c%c%c", 250, C1, C2, C1+C2); */
}


void Comunica(){
i=0;
if (( (Comando[0]) + (Comando[1]) ) == Comando[2]){ // Prueba de checksum
if ( (((Comando[0])-RESTA) >= 0)&&(((Comando[0])-RESTA) <= 11) ) {  // Solo Luces individuales
      LEDON;
      if (Comando[1] < 40){
      LuzIntensidad[(Comando[0]-RESTA)]=Comando[1];
      } else {
      LuzEstado[(Comando[0]-RESTA)]=Comando[1];
      }
}// If

if ((Comando[0]) == TODOS){ // Para todas las luces
int Cuenta;
LEDON;
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
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[0]=input(IN1); // Gurdamos el Cambio
LEDON;
if (LuzEstado[0] != 90) {
BusData(LUZ0,90);
//printf("%c%c%c%c", 250, LUZ0, 90, LUZ0 + 90); //PRENDO
LuzEstado[0]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ0,80);
//printf("%c%c%c%c", 250, LUZ0, 80, LUZ0 + 80); //APAGO
LuzEstado[0]=80;
} // si no
} // If retraso
} // While
Retraso = 0;

while ( (input(IN2)) ^ TeclaEstado[1] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[1]=input(IN2); // Gurdamos el Cambio
LEDON;
if (LuzEstado[1] != 90) {
BusData(LUZ1,90);
//printf("%c%c%c%c", 250, LUZ1, 90, LUZ1 + 90); //PRENDO
LuzEstado[1]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ1,80);
//printf("%c%c%c%c", 250, LUZ1, 80, LUZ1 + 80); //APAGO
LuzEstado[1]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN3)) ^ TeclaEstado[2] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[2]=input(IN3); // Gurdamos el Cambio
LEDON;
if (LuzEstado[2] != 90) {
BusData(LUZ2,90);
//printf("%c%c%c%c", 250, LUZ2, 90, LUZ2 + 90); //PRENDO
LuzEstado[2]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ2,80);
//printf("%c%c%c%c", 250, LUZ2, 80, LUZ2 + 80); //APAGO
LuzEstado[2]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN4)) ^ TeclaEstado[3] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[3]=input(IN4); // Gurdamos el Cambio
LEDON;
if (input(IN4)) {
BusData(LUZ3,90);
//printf("%c%c%c%c", 250, LUZ3, 90, LUZ3 + 90); //PRENDO
//LuzEstado[3]=90;  Sensor del garage
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ3,80);
//printf("%c%c%c%c", 250, LUZ3, 80, LUZ3 + 80); //APAGO
//LuzEstado[3]=80; Sensor del Garage
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN5)) ^ TeclaEstado[4] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[4]=input(IN5); // Gurdamos el Cambio
LEDON;
if (input(IN5)) {
BusData(LUZ4,90);
//printf("%c%c%c%c", 250, LUZ4, 90, LUZ4 + 90); //PRENDO
//LuzEstado[4]=90; //Pasillo lavadero
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ4,80);
//printf("%c%c%c%c", 250, LUZ4, 80, LUZ4 + 80); //APAGO
//LuzEstado[4]=80; //Pasillo lavadero
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN6)) ^ TeclaEstado[5] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[5]=input(IN6); // Gurdamos el Cambio
LEDON;
if (LuzEstado[5] != 90) {
BusData(LUZ5,90);
//printf("%c%c%c%c", 250, LUZ5, 90, LUZ5 + 90); //PRENDO
LuzEstado[5]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ5,80);
//printf("%c%c%c%c", 250, LUZ5, 80, LUZ5 + 80); //APAGO
LuzEstado[5]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN7)) ^ TeclaEstado[6] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[6]=input(IN7); // Gurdamos el Cambio
LEDON;
if (LuzEstado[6] != 90) {
BusData(LUZ6,90);
//printf("%c%c%c%c", 250, LUZ6, 90, LUZ6 + 90); //PRENDO
LuzEstado[6]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ6,80);
//printf("%c%c%c%c", 250, LUZ6, 80, LUZ6 + 80); //APAGO
LuzEstado[6]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN8)) ^ TeclaEstado[7] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[7]=input(IN8); // Gurdamos el Cambio
LEDON;
if (LuzEstado[7] != 90) {
BusData(LUZ7,90);
//printf("%c%c%c%c", 250, LUZ7, 90, LUZ7 + 90); //PRENDO
LuzEstado[7]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ7,80);
//printf("%c%c%c%c", 250, LUZ7, 80, LUZ7 + 80); //APAGO
LuzEstado[7]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN9)) ^ TeclaEstado[8] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[8]=input(IN9); // Gurdamos el Cambio
LEDON;
if (LuzEstado[8] != 90) {
BusData(LUZ8,90);
//printf("%c%c%c%c", 250, LUZ8, 90, LUZ8 + 90); //PRENDO
LuzEstado[8]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ8,80);
//printf("%c%c%c%c", 250, LUZ8, 80, LUZ8 + 80); //APAGO
LuzEstado[8]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN10)) ^ TeclaEstado[9] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[9]=input(IN10); // Gurdamos el Cambio
LEDON;
if (LuzEstado[9] != 90) {
BusData(LUZ9,90);
//printf("%c%c%c%c", 250, LUZ9, 90, LUZ9 + 90); //PRENDO
LuzEstado[9]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ9,80);
//printf("%c%c%c%c", 250, LUZ9, 80, LUZ9 + 80); //APAGO
LuzEstado[9]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN11)) ^ TeclaEstado[10] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[10]=input(IN11); // Gurdamos el Cambio
LEDON;
if (LuzEstado[10] != 90) {
BusData(LUZ10,90);
//printf("%c%c%c%c", 250, LUZ10, 90, LUZ10 + 90); //PRENDO
LuzEstado[10]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ10,80);
//printf("%c%c%c%c", 250, LUZ10, 80, LUZ10 + 80); //APAGO
LuzEstado[10]=80;
} //Else
} // If
} // While
Retraso = 0;

while ( (input(IN12)) ^ TeclaEstado[11] ){  // Si cambia la tecla (Cuando aviso al bus de este cambio?)
Retraso = Retraso + 1;
if (Retraso == 755) {
Retraso = 0;
TeclaEstado[11]=input(IN12); // Gurdamos el Cambio
LEDON;
if (LuzEstado[11] != 90) {
BusData(LUZ11,90);
//printf("%c%c%c%c", 250, LUZ11, 90, LUZ11 + 90); //PRENDO
LuzEstado[11]=90;
} // si no es 90 (no esta encendida con intensidad)
else {
BusData(LUZ11,80);
//printf("%c%c%c%c", 250, LUZ11, 80, LUZ11 + 80); //APAGO
LuzEstado[11]=80;
} //Else
} // If
} // While
Retraso = 0;

} //fn


void Cambio(){
int Cuenta;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
      if (LuzEstado[Cuenta] == 80) {
         LuzAccion[Cuenta] = 95; // Apagado
         LuzAccion2[Cuenta] = 95 + 39; // Apagado
      } // If estado 80
 /*     
      if (LuzEstado[Cuenta] == 90) {
         if (LuzIntensidad[Cuenta] == 0) {
         LuzEstado[Cuenta] = 102; // Maximo
         } else {
         LuzAccion[Cuenta] = LuzIntensidad[Cuenta]; // Segun intensidad
         }
      } // If estado 90      
      
*/      
      if (LuzEstado[Cuenta] == 90) {
         LuzAccion[Cuenta] = LuzIntensidad[Cuenta]; // Segun intensidad
         LuzAccion2[Cuenta] = LuzIntensidad[Cuenta] + 39; // Segun intensidad
      } // If estado 90

      if (LuzEstado[Cuenta] == 102) {
      LuzAccion[Cuenta] = 0; // MAXIMO
      LuzAccion2[Cuenta] = 39; // MAXIMO
      }
      if (LuzEstado[Cuenta] == 95) {
      LuzAccion[Cuenta] = 17; // MEDIO
      LuzAccion2[Cuenta] = 17 + 39; // MEDIO
      }
      if (LuzEstado[Cuenta] == 96) {
      LuzAccion[Cuenta] = 23; //MINIMO
      LuzAccion2[Cuenta] = 23 + 39; //MINIMO
      }
      if ((LuzEstado[Cuenta] == 120) || (LuzEstado[Cuenta] == 70)){ // ONOFF1 mas lento de todos
         if (bit_test(j,7)) { // Encendido
         LuzEstado[Cuenta] = 120;
         LuzAccion[Cuenta] = 0;
         LuzAccion2[Cuenta] = 39;
         }
         else {
         LuzEstado[Cuenta] = 70;
         LuzAccion[Cuenta] = 95;
         LuzAccion2[Cuenta] = 95 + 39;
         }
      } // If ONOFF 1
      
      if ((LuzEstado[Cuenta] == 121) || (LuzEstado[Cuenta] == 71)){ // ONOFF2 mediano
         if (bit_test(j,6)) { // Encendido
         LuzEstado[Cuenta] = 121;
         LuzAccion[Cuenta] = 0;
         LuzAccion2[Cuenta] = 39;
         }
         else {
         LuzEstado[Cuenta] = 71;
         LuzAccion[Cuenta] = 95;
         LuzAccion2[Cuenta] = 95 + 39;
         }
         } // If ONOFF 2
      
      if ((LuzEstado[Cuenta] == 122) || (LuzEstado[Cuenta] == 72)){ // ONOFF3 rapido
         if (bit_test(j,5)) { // Encendido
         LuzEstado[Cuenta] = 122;
         LuzAccion[Cuenta] = 0;
         LuzAccion2[Cuenta] = 39;
         }
         else {
         LuzEstado[Cuenta] = 72;
         LuzAccion[Cuenta] = 95;
         LuzAccion2[Cuenta] = 95 + 39;
         }
         
      } // If ONOFF 1
      if ((LuzEstado[Cuenta] == 123) || (LuzEstado[Cuenta] == 73)){ // ONOFF4 rapidisimo
         if (bit_test(j,4)) { // Encendido
         LuzEstado[Cuenta] = 123;
         LuzAccion[Cuenta] = 0;
         LuzAccion2[Cuenta] = 39;
         }
         else {
         LuzEstado[Cuenta] = 73;
         LuzAccion[Cuenta] = 95;
         LuzAccion2[Cuenta] = 95 + 39;
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
         LuzEstado[Cuenta] = 80;
      } // If ONOFF 3 --> Alarma
} // FOR
} // fn
 
void HolaMundo(){ // Inicio & configuracion de variables

/////////  Inicio el WatchDog  ///////////////
setup_wdt(WDT_2304MS);

/////////  Puertos  ///////////////
set_tris_a(0b00001011);  // Puerto A como salidas, menos A0, A1 y A3 que son entradas analogicas.
set_tris_b(0b00000001);  // Puerto B, todas salidas, menos B0 que es el cruce por cero
set_tris_c(0b10011111);  // Puerto C, de 0 a 4 -> entradas; 5 y 6 para RS232 salidas; C7 es RX -> Entrada
set_tris_d(0b01111111);  // Puerto D, todas entradas menos el Piloto (D7)

int Cuenta = 0;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
Luzintensidad[Cuenta] = 0;
LuzEstado[Cuenta] = 80;  // Por defecto todas las luces apagadas
}

restart_wdt(); // Reiniciamos el perro
delay_ms(1000);
restart_wdt(); // Reiniciamos el perro

/////////  Conversor AD  ///////////////
setup_adc_ports(RA0_RA1_Ra3_ANALOG);  // Entradas analogicas.
setup_adc(ADC_CLOCK_INTERNAL);  // Barrido �Hace falta?

/////////  Temporizadores  ///////////////
setup_timer_1(T1_INTERNAL | T1_DIV_BY_1); // Contador del tiempo de cruce por cero
setup_timer_2(T2_DIV_BY_1, 200, 1); // Contador de entrada a la funcion Accion(); 185

/////////  Inicio de valores particulares por si el servidor no contesta  ///////////////

Cuenta = 0;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
Luzintensidad[Cuenta] = 0;
LuzEstado[Cuenta] = 80;  // Por defecto todas las luces apagadas
}
/*
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
//////////////////////// INDICADOR DE ARRANQUE ///////////////////////////
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDON;
delay_ms(RESTA);
restart_wdt(); // Reiniciamos el perro
LEDOFF;

Cuenta = 0;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
Luzintensidad[Cuenta] = 0;
LuzEstado[Cuenta] = 80;  // Por defecto todas las luces apagadas
}

/////////  Interrupciones  ///////////////
enable_interrupts(global);
//disable_interrupts(global);
enable_interrupts(int_ext); // Interrupcion para el cruce por 0
enable_interrupts(int_rda); // Interrupcion de llegada de datos
enable_interrupts(int_timer2); // frecuencia de entrada al barrido de focos

Teclas = true; // Para que se puedan usar las teclas

/////////  Inicio valores de luces por defecto por si el servidor no contesta  ///////////////

Cuenta = 0;
for (Cuenta=0;Cuenta<12;Cuenta++){ // llena todo desde el 0 al 11
Luzintensidad[Cuenta] = 0;
LuzEstado[Cuenta] = 80;  // Por defecto todas las luces apagadas
}
LEDON;
BusData(184,52);
//printf("%c%c%c%c", 250, 184, 52, 184+52); //Identificacion de inicio (49 es 1; 50 es 2; 51 es 3; 52 es 4 y 53 es 5) // Identificacion de red
//delay_ms(50);
//restart_wdt(); // Reiniciamos el perro
//BusData(184,52);
//printf("%c%c%c%c", 250, 184, 52, 184+52); //Identificacion de inicio (49 es 1; 50 es 2; 51 es 3; 52 es 4 y 53 es 5) // Identificacion de red
//delay_ms(50);
restart_wdt(); // Reiniciamos el perro
//BusData(184,52);
//printf("%c%c%c%c", 250, 184, 52, 184+52); //Identificacion de inicio (49 es 1; 50 es 2; 51 es 3; 52 es 4 y 53 es 5) // Identificacion de red
//delay_ms(50);
//restart_wdt(); // Reiniciamos el perro
}

#int_ext
void Cruce(){
set_timer1(0); // Reseteamos el timer porque cruzamos por 0
++j; // variable de tiempos largos
}

#int_rda
void BusDatax(){
//LEDON;
if (kbhit()){
Comando[i]=getc();
if (Comando[i] == 250) i=0;
else ++i;
if (i==3) Comunica(); // Tenemos un comando nuevo
}
}

#int_timer2
void Accion(){ // Funcion de actuacion de luces de alta velocidad (lo mas depurada posible)

if (TMR1H < 36){
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

if (TMR1H == 35){    
if (LuzAccion[0]>2) output_low(OUT1);
if (LuzAccion[1]>2) output_low(OUT2);
if (LuzAccion[2]>2) output_low(OUT3);
if (LuzAccion[3]>2) output_low(OUT4);
if (LuzAccion[4]>2) output_low(OUT5);
if (LuzAccion[5]>2) output_low(OUT6);
if (LuzAccion[6]>2) output_low(OUT7);
if (LuzAccion[7]>2) output_low(OUT8);
if (LuzAccion[8]>2) output_low(OUT9);
if (LuzAccion[9]>2) output_low(OUT10);
if (LuzAccion[10]>2) output_low(OUT11);
if (LuzAccion[11]>2) output_low(OUT12);
}
} else {
if (TMR1H == LuzAccion2[0]) output_high(OUT1);
if (TMR1H == LuzAccion2[1]) output_high(OUT2);
if (TMR1H == LuzAccion2[2]) output_high(OUT3);
if (TMR1H == LuzAccion2[3]) output_high(OUT4);
if (TMR1H == LuzAccion2[4]) output_high(OUT5);
if (TMR1H == LuzAccion2[5]) output_high(OUT6);
if (TMR1H == LuzAccion2[6]) output_high(OUT7);
if (TMR1H == LuzAccion2[7]) output_high(OUT8);
if (TMR1H == LuzAccion2[8]) output_high(OUT9);
if (TMR1H == LuzAccion2[9]) output_high(OUT10);
if (TMR1H == LuzAccion2[10]) output_high(OUT11);
if (TMR1H == LuzAccion2[11]) output_high(OUT12);

if (TMR1H == 74){    
if (LuzAccion[0]>2) output_low(OUT1);
if (LuzAccion[1]>2) output_low(OUT2);
if (LuzAccion[2]>2) output_low(OUT3);
if (LuzAccion[3]>2) output_low(OUT4);
if (LuzAccion[4]>2) output_low(OUT5);
if (LuzAccion[5]>2) output_low(OUT6);
if (LuzAccion[6]>2) output_low(OUT7);
if (LuzAccion[7]>2) output_low(OUT8);
if (LuzAccion[8]>2) output_low(OUT9);
if (LuzAccion[9]>2) output_low(OUT10);
if (LuzAccion[10]>2) output_low(OUT11);
if (LuzAccion[11]>2) output_low(OUT12);
}
}
} // Fn


