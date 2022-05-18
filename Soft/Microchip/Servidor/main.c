/*======================================================================================================
||   Proyecto: Casandra v1.0                                                                          ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                    ||
||   Fecha: 07/2014                                                                                   ||
||   Compilador: PCWHD v5.025 (www.ccsinfo.com)                                                       ||
||   Fuente: http://                                                                                  ||
||                                                                                                    ||
|| Firmware del modulo servidor (display, teclado, sensor de humedad y temperatura) correspondiente   ||
|| al sistema domotico CASANDRA. Control general con uso intensivo de EEPROM. Comunicacion sobre par  ||
|| trenzado usando el transceptor SN75176.                                                            ||
||                                                                                                    ||
======================================================================================================*/

#include <18f4620.h>

//#byte TMR1H = 0xFCF // Le pongo nombre al registro alto de timer 1
//#byte TMR1L = 0xFCE // Le pongo nombre al registro bajo de timer 1
#device ADC=10
#fuses XT, MCLR, WDT1024 // Opciones de configuración
//#fuses XT, NOIESO, NOFCMEN, NOPUT, NOPROTECT, NOPBADEN, NOLVP, NOBROWNOUT, MCLR, WDT1024, NOLPT1OSC // Opciones de configuración

#use delay(clock=4000000)  // Reloj de 4MHz
#use rs232(baud=2400, xmit=PIN_C6, rcv=PIN_C7, enable=PIN_D4, ERRORS) // Comunicacion serial

//#include "DHT11.h"
#include "FLEX_LCD_164.c"
#include "DS1307.c"
//#include <string.h>
//#include <internal_eeprom.c> 


///////////////////// Variables y Funciones para DHT11  ////////////////////////////////
unsigned char values[5];   // Arreglo de valores (Solo nos sirve 0-temp- y 2-humedad-)
void DHT11_init();         // Iniciador estabilizador
unsigned char get_byte();  // Funcion que arma cada byte
unsigned char get_data();  // Funcion que asigna los valores a los diferentes bytes
unsigned char state = 0;   // Estado del sensor
////////////////////////////////////////////////////////////////////////////////////////


////// Variables del DS1307 /////
int sec;
int min;
int hrs;
int day;
int month;
int year;
int dow;
/////////////////////////////////


///////////////// Definiciones de Pines /////////////////////
#define DHT11_pin PIN_B5     // Sensor DHT11
#define LCDLED PIN_D6        // Luz de pantalla led
#define ARRIBA PIN_B3        // Teclado Cancelar
#define ABAJO  PIN_B2        // Teclado Ok
#define OOKK   PIN_B1        // Teclado Abajo
#define CANCEL PIN_B0        // Teclado Arriba
#define FA_R 0               // Fase R
#define FA_S 1               // Fase S
#define FA_T 2               // Fase T
#define TempIn values[2]     // Temperatura
#define Hum values[0]        // Humedad

#define ONLCDLED output_low(PIN_D6)
#define OFFLCDLED  output_high(PIN_D6)
#define LIMPIARLCD lcd_putc("\f")

//////////////// Definicion de los sensores y Luces protagonistas /////////////////////////////
                    //////////////// SENSORES ////////////////
#define SSPasCuar     50 // Sensor del pasillo de los cuartos
#define SSEstar       60 // Sensor del estar
#define SSHall        59 // Sensor del hall
#define SSLiving      62 // Sensor del living
#define SSCocina      61 // Sensor de la cocina
#define SSPasEnt      73 // Sensor del pasillo de entrada
#define SSGaleria     91 // Sensor de la galeria
#define SSParrilla    92 // Sensor de la galeria
#define SSGarage      72 // Sensor del garage

                     ///////// LUCES DE SERVICIOS /////////////
#define LLPasCuar1    48 // Luz1 del pasillo de los cuartos
#define LLPasCuar2    47 // Luz2 del pasillo de los cuartos
#define LLEstar       66 // Luz del estar
#define LLHall        58 // Luz del hall
#define LLLiving1     70 // Luz del living 1
//#define LLLiving2     Luz58 // Luz del living 2
#define LLCocina      63 // Luz de la cocina
#define LLPasEnt      76 // Luz del pasillo de entrada
#define LLGaleria1    82 // Luz de la galeria 1
#define LLGaleria2    83 // Luz de la galeria 2
#define LLGarage      75 // Luz del garage

       ///////// DEFINICION DE LOS VENTILADORES /////////////
#define VentPaMa      55 // Ventilador de papa y mama 23 + 32(despl)
#define VentGal       84 // Ventilador de papa y mama 52 + 32(despl)


///////////////// Definiciones de posiciones de memoria EEPROM /////////////////
#define EstAlarma   2        // Estado de la alarma
#define EstCasandra 3        // Estado de casandra
#define TpoSil      4        // Tiempo de Alarma Silenciosa (en segundos de 1 a 180)
#define TpoSal      5        // Tiempo para salir (en segundos de 1 a 180)
#define Password    6 // y 7 // Puntero de password (OJO!!! es solo el puntero, ocupa 2 bytes de memoria, 6 y 7)
#define G1On        8        // Horario de encendido del grupo automatico 1
#define G2On        9        // Horario de encendido del grupo automatico 2
#define G3On       10        // Horario de encendido del grupo automatico 2
#define G4On       11        // Horario de encendido del grupo automatico 2
#define G1Off      12        // Horario de apagado del grupo automatico 1
#define G2Off      13        // Horario de apagado del grupo automatico 2
#define G3Off      14        // Horario de apagado del grupo automatico 2
#define G4Off      15        // Horario de apagado del grupo automatico 2
#define G1Dias     16        // Luces integrantes del grupo 1
#define G2Dias     17        // Luces integrantes del grupo 2
#define G3Dias     18        // Luces integrantes del grupo 2
#define G4Dias     19        // Luces integrantes del grupo 2
#define Umbra      20        // Nivel umbral de la fotocelda

#define Luz1 33   // Posiciones de memoria para la intensidad de cada luz
#define Luz2 34
#define Luz3 35
#define Luz4 36
#define Luz5 37
#define Luz6 38
#define Luz7 39
#define Luz8 40
#define Luz9 41
#define Luz10 42
#define Luz11 43
#define Luz12 44
#define Luz13 45
#define Luz14 46
#define Luz15 47
#define Luz16 48
#define Luz17 49
#define Luz18 50
#define Luz19 51
#define Luz20 52
#define Luz21 53
#define Luz22 54
#define Luz23 55
#define Luz24 56
#define Luz25 57
#define Luz26 58
#define Luz27 59
#define Luz28 60
#define Luz29 61
#define Luz30 62
#define Luz31 63
#define Luz32 64
#define Luz33 65
#define Luz34 66
#define Luz35 67
#define Luz36 68
#define Luz37 69
#define Luz38 70
#define Luz39 71
#define Luz40 72
#define Luz41 73
#define Luz42 74
#define Luz43 75
#define Luz44 76
#define Luz45 77
#define Luz46 78
#define Luz47 79
#define Luz48 80
#define Luz49 81
#define Luz50 82
#define Luz51 83
#define Luz52 84
#define Luz53 85
#define Luz54 86
#define Luz55 87
#define Luz56 88
#define Luz57 89
#define Luz58 90
#define Luz59 91
#define Luz60 92
#define Luz61 93

//// Posiciones de los registros de maximo y minimo de Temperatura y Radiacion solar /////
#define TempMaxLun 100        // Temperatura maxima del lunes
#define TempMaxMar 105        // Temperatura maxima del martes
#define TempMaxMie 110        // Temperatura maxima del miercoles
#define TempMaxJue 115        // Temperatura maxima del jueves
#define TempMaxVie 120        // Temperatura maxima del viernes
#define TempMaxSab 125        // Temperatura maxima del sabado
#define TempMaxDom 130        // Temperatura maxima del domingo

#define TempMinLun 135        // Temperatura minima del lunes
#define TempMinMar 140        // Temperatura minima del martes
#define TempMinMie 145        // Temperatura minima del miercoles
#define TempMinJue 150        // Temperatura minima del jueves
#define TempMinVie 155        // Temperatura minima del viernes
#define TempMinSab 160        // Temperatura minima del sabado
#define TempMinDom 165        // Temperatura minima del domingo

#define SolLun 170            // Sol maximo del lunes
#define SolMar 175            // Sol maximo del martes
#define SolMie 180            // Sol maximo del miercoles
#define SolJue 185            // Sol maximo del jueves
#define SolVie 190            // Sol maximo del viernes
#define SolSab 195            // Sol maximo del sabado
#define SolDom 200            // Sol maximo del domingo

#define GrLuz1 233   // Posiciones de memoria para el byte de pertenencia de Grupos de cada GrLuz
#define GrLuz2 234
#define GrLuz3 235
#define GrLuz4 236
#define GrLuz5 237
#define GrLuz6 238
#define GrLuz7 239
#define GrLuz8 240
#define GrLuz9 241
#define GrLuz10 242
#define GrLuz11 243
#define GrLuz12 244
#define GrLuz13 245
#define GrLuz14 246
#define GrLuz15 247
#define GrLuz16 248
#define GrLuz17 249
#define GrLuz18 250
#define GrLuz19 251
#define GrLuz20 252
#define GrLuz21 253
#define GrLuz22 254
#define GrLuz23 255
#define GrLuz24 256
#define GrLuz25 257
#define GrLuz26 258
#define GrLuz27 259
#define GrLuz28 260
#define GrLuz29 261
#define GrLuz30 262
#define GrLuz31 263
#define GrLuz32 264
#define GrLuz33 265
#define GrLuz34 266
#define GrLuz35 267
#define GrLuz36 268
#define GrLuz37 269
#define GrLuz38 270
#define GrLuz39 271
#define GrLuz40 272
#define GrLuz41 273
#define GrLuz42 274
#define GrLuz43 275
#define GrLuz44 276
#define GrLuz45 277
#define GrLuz46 278
#define GrLuz47 279
#define GrLuz48 280
#define GrLuz49 281
#define GrLuz50 282
#define GrLuz51 283
#define GrLuz52 284
#define GrLuz53 285
#define GrLuz54 286
#define GrLuz55 287
#define GrLuz56 288
#define GrLuz57 289
#define GrLuz58 290
#define GrLuz59 291
#define GrLuz60 292

///////////////// Funciones Generales ////////////////
void inicio();     // Configuracion inicial
//void BusData();    // Bus de datos con algo para meter en el comando
void Comunica();   // Funcion de lectura de puerto serial y armado del comando
void Teclado();    // Leemos el teclado
void Pantalla();   // Actualizamos la pantalla
void Tiempo();     // Funcion para actualizar el tiempo
void Ambiente();   // Funcion para cargar variables ambientales
void AcercaDe();   // Acerca de...
void Holding();    // Pantalla de hold
void ZzZ();        // Casandra durmiendo
void ActAlarma();  // Activador de alarma
void LuzOnOff();   // Enciende o apaga la luz
void LuzGrupo();   // Enciende o apaga grupo de luces
void LuzInt();     // Configuracion de la intensidad de cada luz
void Ccasandra();  // Configuracion del nivel de automatico
void ConfAlarma(); // Configuracion de la alarma
void ADias();      // Funcion de seleccion de dias en grupos automaticos
void AHs();        // Funcion de horarios de grupos automaticos
void Aluces();     // Funcion de seleccion de luces para los grupos automaticos
int  Autenticar(); // Chequeo de contraseña, devuelve 1 si es correcta y 0 si no
void CClave();     // Funcion para cambiar la clave
void CHs();        // Funcion para cambiar la hora
void CReset();     // Resetea todas las variables del sistema
void ConfAlarma(); // Configuracion de alarma
void Fotocel();    // Configuracion del umbral de la fotocelula
void VerTemp();    // Donde vemos las temperaturas
void VerTens();    // Donde vemos las tensiones del sistema
void DosVeinte();  // Captura ADC para la tension trifasica
void Defini();     // Definiciones de Teclas y consumos
void Automaticos();// Funcion que analiza y dispara automaticos
//void Pasillos();   // Funcion que analiza y dispara los pasillos
void SensoresLuz();// Funcion de anlisis de sensores para servicios
void SensoresAl(); // Funcion de anlisis de sensores para alarma
void AAlarma();    // Funcion de gestion e interpretacion de alarma
void LuzLED();     // Controla cuando la luz led se prende o se apaga
void ActuaLuz();   // Funcion que actualiza las luces en los modulos segun el valor del servidor
void CuantaLuz();  // Funcion que muestra cantidad de luces prendidas
void BusData(int C1,int C2); // Funcion que manda comandos de bus por 3
void Prender(int NLuz); // Funcion que recibe parametros y prende cierta luz
void Apagar(int NLuz);  // Funcion que recibe parametros y prende cierta luz
void CorteLuz();   // Funcion que acondiciona y actualiza valores de los modulos luego de corte de luz

/////////////// Variables globales ///////////////
int Menu;          // Posicion dentro del menu
int TempMax[4];    // Temperatura maxima del dia - numero de dia - hora - minuto (el "dow" se interpreta por la posicion de memoria EEPROM)
int TempMin[4];    // Temperatura minima del dia - numero de dia - hora - minuto (el "dow" se interpreta por la posicion de memoria EEPROM)
int SolMax[4];     // Maximo nivel de radiacion de sol del dia - dia - hora - minuto (el "dow" se interpreta por la posicion de memoria EEPROM)
short Rebote;      // Antirebote de teclado
int Comando[4];    // Carga de EVENTO y VALOR
int i=0;           // Contador puntero para armar el comando recibido por el bus
char Dia[4];       // Cadena del dia
char Mes[4];       // Cadena del Mes
int TempOut;       // Temperatura remota (afuera)
int Sol;           // Intensidad del sol (remota, afuera)
int SolAux;        // Auxiliar para recibir y promediar el sol
int EstadoAl;      // Estado de la alarma
int ConfCas;       // El bit 0 es servicios, el 1 es el automatico1, el 2 el automatico2, el 3 el automatico3, el 4 el automatico4 y el 5 es el modulo web
int Umbral;        // Nivel seleccionado de fotocelda
int Tiempo1;       // Tiempo del timer1
int IntLuz[99];    // Arreglo que guarda los niveles de intensidad de cada luz
int LGrupo[99];    // Arreglo que avisa que luz forma parte de los dos grupos automaticos
//int LLuces[99];    // Arreglo que avisa que luz forma parte de los diferentes tipos de luces
//int LTecla[99];    // Arreglo que avisa que es: Tecla, sensor interno, o externo
int Passwd[2];     // Contraseña en RAM
int TpoSilencioso; // Tiempo de silencio antes de sirena
int TpoSalida;     // Tiempo de gracia post activacion para salir
int TpoSeguro;     // Variable de seguimiento del tiempo seguro para salir
int GOn[5];        // Hora de encendido de los automaticos
int GOff[5];       // Hora de apagado de los automaticos
int Gdias[5];      // Grupo de dias de los automaticos
int Vbata;         // Tension de la bateria
int FaseR;         // Tension en fase R
int FaseS;         // Tension en fase S
int FaseT;         // Tension en fase T
short SPasCuar;    // Sensor del pasillo de los cuartos
short SEstar;      // Sensor del estar
short SHall;       // Sensor del hall
short SLiving;     // Sensor del living
short SCocina;     // Sensor de la cocina
short SPasEnt;     // Sensor del pasillo de entrada
short SGaleria;    // Sensor de la galeria
short SParrilla;   // Sensor de la galeria frente a la parrilla
short SGarage;     // Sensor del garage
int AAseguu;       // Auxiliares para activacion de alarma
int AAdesact;      // Auxiliares para activacion de alarma
int UltIn[25];     // Ultimas entradas
int UltBus[13];    // Ultimos valores de bus de datos
int CuentaPanico;  // Contador de cambios de teclas para panico
int CuentaLuz;     // Contador de luces encendidas
int CuentaSens;    // Contador de luces encendidas
short zZzZ;        // Bandera de sueño
int RawBus[23];    // Valores del bus sin distincion
short EsNoche;     // Bandera para saber cuando amanece y cuando atardece


//////////////////////// EMPEZAMOS! :D //////////////////////////

void main(){

inicio();

if (input(ARRIBA)==0 && input(ABAJO)==0 ) { // && input(OOKK)==0 && input(CANCEL)==0){
long elimina;
for (elimina=0;elimina<1024;elimina++){
write_eeprom(elimina,0x00);
delay_ms(1);
lcd_gotoxy(1,2);
printf(lcd_putc, "Eliminacion rustica ");
lcd_gotoxy(1,3);
printf(lcd_putc, "    de datos...     ");
}
}

while(1){

restart_wdt(); // reiniciamos el perro

if (input(ARRIBA)==0 && input(ABAJO)==0 && input(OOKK)==0 && input(CANCEL)==0){
EstadoAl = 7;
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
delay_ms(500);
Menu = 1;
}

Teclado();   // Escaneo de teclado
Pantalla();  // Actualizaciones de Pantalla
AAlarma();   // Analisis de Alarma
LuzLED();    // Luz de display
CuantaLuz(); // Cuenta luces y sensores

if ((Menu == 121) || (Menu == 122) || (Menu == 123)) ActAlarma();
if (Menu == 124) ConfAlarma();
if (Menu == 131) LuzOnOff();
if (Menu == 132) LuzGrupo();
if (Menu == 133) LuzInt();
if (Menu ==  41) Ccasandra();
if (Menu == 151) ADias();
if (Menu == 152) AHs();
if (Menu == 153) Aluces();
if (Menu == 161) CClave();
if (Menu == 162) CHs();
if (Menu == 163) Fotocel();
if (Menu == 164) CReset();
if (Menu == 165) AcercaDe();

//OFFLCDLED;
} //while
} // Main

void LuzLED() {

if (menu == 0) OFFLCDLED;
else ONLCDLED;

}

void BusData(int C1,int C2) {
//   printf("%c%c%c%c", 250, C1, C2, C1+C2); 
   delay_ms(5);
   printf("%c%c%c%c", 250, C1, C2, C1+C2); 
//   delay_ms(3);
//   printf("%c%c%c%c", 250, C1, C2, C1+C2); 
   delay_ms(5);
}

void Prender(int NLuz) {
      if (IntLuz[NLuz] < 99){ // estaba apagado
         IntLuz[NLuz] = IntLuz[NLuz] + 100; // prendo en el arreglo
//         printf("%c%c%c%c", 250, NLuz, 90, 90+NLuz); // Mando el encendido
         BusData(NLuz,90); //
         write_eeprom(NLuz,IntLuz[NLuz]);
         delay_ms(3);
      } // si estaba apagado..
}

void Apagar(int NLuz) {
      if (IntLuz[NLuz] > 99){ // estaba prendido
         IntLuz[NLuz] = IntLuz[NLuz] - 100; // apago en el arreglo
//         printf("%c%c%c%c", 250, NLuz, 80, 80+NLuz); // Mando el apagado
         BusData(NLuz,80);
         write_eeprom(NLuz,IntLuz[NLuz]);
         delay_ms(3);
      } // si estaba prendido..
}

void ActuaLuz() { //Recorre todos los arreglos con las intensidades y estados de las luces y los ejecuta (hay que llamarla)

int kk;

for (kk=32;kk<94;kk++) {
restart_wdt();
  if (IntLuz[kk] > 99){ // La luz está prendida
//      printf("%c%c%c%c", 250, kk, IntLuz[kk]-100, (IntLuz[kk]-100)+kk); // Mando la intensidad
      BusData(kk,IntLuz[kk]-100);
      //delay_ms(3);
//      printf("%c%c%c%c", 250, kk, 90, 90+kk); // Mando el encendido
      BusData(kk,90);
      //delay_ms(3);
    } else { // si esta apagada
//      printf("%c%c%c%c", 250, kk, 80, 80+kk); // Mando el apagado
      BusData(kk,80);
      //delay_ms(3);
//      printf("%c%c%c%c", 250, kk, IntLuz[kk], IntLuz[kk]+kk); // Mando la intensidad
      BusData(kk,IntLuz[kk]);
      //delay_ms(3);
    } // else (esta apagada)
} // for...
} //fn

void CuantaLuz() { //Recorre todos los arreglos con las intensidades y estados de las luces y los ejecuta (hay que llamarla)
int kk;
CuentaLuz = 0;
for (kk=32;kk<94;kk++) {
   if ((kk==33) || (kk==37) || (kk==54) || (kk==55) || (kk==56) || (kk==57) || (kk==65) || (kk==67) || (kk==69) || (kk==71) || (kk==74) || (kk==78) || (kk==79) || (kk==80) || (kk==81) || (kk==86) || (kk==87) || (kk==89) || (kk==90)){
   } else if (IntLuz[kk] > 99) ++CuentaLuz;  // Solamente considero las luces que estan conectadas
} // for...
} //fn


void AAlarma() {  // Funcion de analisis y disparo de alarma (corre sola en main)

if ((EstadoAl == 2) || (EstadoAl == 3) || (EstadoAl == 4) || (EstadoAl == 5) || (EstadoAl == 6) || (EstadoAl == 7)) { // Si la alarma en cualquier estado activado
if (Menu > 1){
if (Autenticar()) { //autentica ok
//printf("%c%c%c%c", 250, 155, 0, 155);    // Apagamos sirena
BusData(155,0);
//delay_ms(5);
restart_wdt();
//printf("%c%c%c%c", 250, 100, 141, 241);  // Habilitamos teclas
BusData(100,141);
//delay_ms(5);
restart_wdt();
/*
printf("%c%c%c%c", 250, 155, 0, 155);    // Apagamos sirena
BusData(155,0);
delay_ms(5);
restart_wdt();
printf("%c%c%c%c", 250, 100, 141, 241);  // Habilitamos teclas
BusData(155,0);
delay_ms(5);
restart_wdt();
*/
ActuaLuz(); // Actualizamos luces segun RAM
EstadoAl = 1; // desactivada
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
Menu = 1;
} else { // no autentica
Menu = 1;
} // no autentica
} // Menu mas que 1
} // Alarma activada

if (EstadoAl == 5) { // disparada pero en silencio
if (sec != AAseguu) { // paso un segundo
AAseguu = sec;
if (TpoSilencioso > 0) {
if (TpoSilencioso == read_eeprom(TpoSil)) {
//printf("%c%c%c%c", 250, 100, 140, 240); // Disparamos alarma sin sonido
BusData(100,140);
//printf("%c%c%c%c", 250, VentPaMa, 80, VentPaMa+80); // Apago ventilador papa mama
BusData(VentPaMa,80);
//printf("%c%c%c%c", 250, VentGal, 80, VentGal+80); // Apago ventilador galeria
BusData(VentGal,80);
}
TpoSilencioso = TpoSilencioso - 1;
}
else { // llegamos a tiempo silencioso 0
EstadoAl = 6; // comienza a sonar
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
TpoSilencioso = read_eeprom(TpoSil);  // Leo y recupero el tiempo silencioso original
} // tiempo silencioso es 0
} // paso un segundo
} // disparada pero en silencio

if (EstadoAl == 6) { // disparada con sonido intermitente
if (sec != AAseguu) { // paso un segundo
AAseguu = sec;
if ((AAseguu % 4) == 0) BusData(155,2); // printf("%c%c%c%c", 250, 155, 1, 156); // Disparamos un segundo intermitente cada 4
AAdesact = AAdesact + 1;
if (AAdesact == 25) { // Pasaron 45 segundos de intermitente
AAdesact = 0; // Reiniciamos el desactivador
EstadoAl = 7; // Comienza disparo total
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
} // terminan 45 segundos tranqilos
} // paso sec
} // estado al 6

if (EstadoAl == 7) { // disparada con sonido total = PANICO
if (sec != AAseguu) { // paso un segundo
AAseguu = sec;
//printf("%c%c%c%c", 250, 155, 100, 255); // Disparamos total
BusData(155,100);
//delay_ms(5);
//printf("%c%c%c%c", 250, 100, 140, 240); // Disparamos alarma de luces
BusData(100,140);
//printf("%c%c%c%c", 250, VentPaMa, 80, VentPaMa+80); // Apago ventilador papa mama
BusData(VentPaMa,80);
//printf("%c%c%c%c", 250, VentGal, 80, VentGal+80); // Apago ventilador galeria
BusData(VentGal,80);
AAdesact = AAdesact + 1;
if (AAdesact == 240) { // Pasaron 4 minutos de alarma
EstadoAl = 2; // Reactivamos total
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
} // terminan 45 segundos tranqilos
} // paso sec
} // estado al 7

} //fn



void SensoresLuz(){ // funcion que analiza y dispara el encendido y apagado de los pasillos y servicios
if (bit_test(ConfCas,0)) { // si configuracion casandra tiene activado los servicios (pasillos)

//if (Sol <= Umbral) { // si estamos bajo el umbral de sol

if (IntLuz[LLPasCuar2] < 40) { // La luz esta apagada, controla casandra
if (SPasCuar) { if (Sol <= (Umbral + 20)) { BusData(LLPasCuar2,96); }}//printf("%c%c%c%c", 250, LLPasCuar2, 102, LLPasCuar2+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLPasCuar2,80); //printf("%c%c%c%c", 250, LLPasCuar2, 80, LLPasCuar2+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if (IntLuz[LLPasCuar1] < 40) { // La luz esta apagada, controla casandra
if (SPasCuar) { if (Sol <= (Umbral + 20)) { BusData(LLPasCuar1,96); }} // printf("%c%c%c%c", 250, LLPasCuar1, 102, LLPasCuar1+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLPasCuar1,80); //printf("%c%c%c%c", 250, LLPasCuar1, 80, LLPasCuar1+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((IntLuz[LLEstar]) < 40) { // La luz esta apagada, controla casandra
if (SEstar) { if (Sol <= (Umbral + 10)) { BusData(LLEstar,95); }} //printf("%c%c%c%c", 250, LLEstar, 95, LLEstar+95); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLEstar,80); //printf("%c%c%c%c", 250, LLEstar, 80, LLEstar+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((IntLuz[LLHall]) < 40) { // La luz esta apagada, controla casandra
if (SHall) { if (Sol <= (Umbral + 10)) { BusData(LLHall,102); }} //printf("%c%c%c%c", 250, LLHall, 102, LLHall+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLHall,80); //printf("%c%c%c%c", 250, LLHall, 80, LLHall+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((CuentaLuz - CuentaSens) < 4){ //solo funciona si hay menos de 4 luces prendidas
if ((IntLuz[LLLiving1]) < 40) { // La luz esta apagada, controla casandra
if (SLiving) { if (Sol <= (Umbral + 10)) { BusData(LLLiving1,95); }} //printf("%c%c%c%c", 250, LLLiving1, 102, LLLiving1+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLLiving1,80); //printf("%c%c%c%c", 250, LLLiving1, 80, LLLiving1+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada
} // luces del sensor

if ((IntLuz[LLCocina]) < 40) { // La luz esta apagada, controla casandra
if (SCocina) { if (Sol <= (Umbral + 10)) { BusData(LLCocina,95); }} //printf("%c%c%c%c", 250, LLCocina, 95, LLCocina+95); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLCocina,80); //printf("%c%c%c%c", 250, LLCocina, 80, LLCocina+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((IntLuz[LLPasEnt]) < 40) { // La luz esta apagada, controla casandra
if (SPasEnt) { if (Sol <= (Umbral + 10)) { BusData(LLPasEnt,102); }} //printf("%c%c%c%c", 250, LLPasEnt, 102, LLPasEnt+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLPasEnt,80); //printf("%c%c%c%c", 250, LLPasEnt, 80, LLPasEnt+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((IntLuz[LLGaleria1]) < 40) { // La luz esta apagada, controla casandra
if (SGaleria) { if (Sol <= (Umbral + 10)) { BusData(LLGaleria1,102); }} //printf("%c%c%c%c", 250, LLGaleria1, 102, LLGaleria1+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLGaleria1,80); //printf("%c%c%c%c", 250, LLGaleria1, 80, LLGaleria1+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada
if ((IntLuz[LLGaleria2]) < 40) { // La luz esta apagada, controla casandra
if (SParrilla) { if (Sol <= (Umbral + 10)) { BusData(LLGaleria2,102); }} //printf("%c%c%c%c", 250, LLGaleria2, 102, LLGaleria2+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLGaleria2,80); //printf("%c%c%c%c", 250, LLGaleria2, 80, LLGaleria2+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

if ((IntLuz[LLGarage]) < 40) { // La luz esta apagada, controla casandra
if (SGarage) { if (Sol <= (Umbral + 10)) { BusData(LLGarage,95); }} //printf("%c%c%c%c", 250, LLGarage, 102, LLGarage+102); // Prendemos en la red // esta activado el sensor de los pasillos de los cuartos
else BusData(LLGarage,80); //printf("%c%c%c%c", 250, LLGarage, 80, LLGarage+80); // Apagamos en la red // esta activado el sensor de los pasillos de los cuartos
} // esta apagada

//} // si el sol es menor a umbral

} // confcas
} // fn

void SensoresAl(){  // Funcion que maneja criterios de disparo de alamra segun sensores

int SumCuartos;
if (SPasCuar == true) SumCuartos = 1;
else SumCuartos = 0;

int SumEstar;
if (SEstar == true) SumEstar = 1;
else SumEstar = 0;

int SumHall;
if (SHall == true) SumHall = 1;
else SumHall = 0;

int SumLiving;
if (SLiving == true) SumLiving = 1;
else SumLiving = 0;

int SumCocina;
if (SCocina == true) SumCocina = 1;
else SumCocina = 0;

int SumPasEnt;
if (SPasEnt == true) SumPasEnt = 1;
else SumPasEnt = 0;

int SumGaleria;
if (SGaleria == true) SumGaleria = 1;
else SumGaleria = 0;

int SumParrilla;
if (SParrilla == true) SumParrilla = 1;
else SumParrilla = 0;

int SumGarage;
if (SGarage == true) SumGarage = 1;
else SumGarage = 0;

if (TpoSeguro == 0){ // 30 segundos mas despues de activacion

if (EstadoAl == 2) { // Alarma total activa (dispara con 3 sensores cualquiera)
if ((SumCuartos + SumEstar + SumHall + SumLiving + SumCocina + SumPasEnt + SumGaleria + SumParrilla + SumGarage) > 2){
EstadoAl = 5;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
}
} //Al2

if (EstadoAl == 3) { // Alarma CuartosOK (dispara con 3 sensores cualquiera fuera de cuartos)
if ((SumEstar + SumHall + SumLiving + SumCocina + SumPasEnt + SumGaleria + SumParrilla + SumGarage) > 2){
EstadoAl = 5;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
}
} //Al3

if (EstadoAl == 4) { // Alarma CasaOK (dispara con 2 sensores cualquiera de afuera)
if ((SumGaleria + SumParrilla + SumGarage) > 1){
EstadoAl = 5;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
}
} //Al4

} // ya termiano el tiempo seguro

} //fn



void Automaticos(){ // funcion que analiza y dispara el encendido y apagado de los grupos automaticos

int kk;
int dowfalso;
int HsAux;
int MinAux;
int Miti;

if (bit_test(ConfCas,1)) { // si configuracion casandra tiene activado grupo auto 1

if (bit_test(Gdias[1], dow)) {  // El grupo 1 esta activo el dia de hoy
////// ENCENDIDO
if (GOn[1] == 48) {  // si se prende por fotocelda
//if ((hrs > 12) && (hrs < 24)){
if ((Umbral == Sol) && (EsNoche == false)){
//if (((Umbral - 1) == Sol) || ((Umbral - 2) == Sol) || ((Umbral - 3) == Sol)) { // si vale 1 menos que umbral, o 2 o 3 menos disparamos encendido
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4))Prender (kk); // si pertenece al grupo la prendo
      } // for...
EsNoche = true;
}
//} // horario
} // Prende por fotocelda

if (GOn[1] < 48) { // por horario comienza
HsAux = (hrs*2)+(GOn[1]%2);
MinAux = ((GOn[1]%2)*30);

if ((GOn[1] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4))Prender(kk); // si pertenece al grupo la prendo 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G1 activo hoy

Miti = GOff[1]/2;
if (( (Miti >= 0) && (Miti < 6) ) || (Miti == 24)) {
dowfalso = dow + 1;
if (dowfalso == 8) dowfalso = 1;
}
else dowfalso = dow;

if (bit_test(Gdias[1], dowfalso)) {  // El grupo 1 esta activo el dia de hoy o ayer, dado que es para apagado
////// APAGADO
if (GOff[1] == 48) {  // si se apaga por fotocelda
//if ((hrs > 0) && (hrs < 12)){
if ((Umbral == Sol) && (EsNoche == true)){
//if (((Umbral + 1) == Sol) || ((Umbral + 2) == Sol) || ((Umbral + 3) == Sol)) { // si vale 1 mas que umbral, o 2 o 3 mas disparamos apagado
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4))Apagar(kk); // si pertenece al grupo la apago 
      } // for...
EsNoche = false;      
}
//} // horario fotocelda
} // fotocelda 
if (GOff[1] < 48) { // por horarios
HsAux = (hrs*2)+(GOff[1]%2);
MinAux = ((GOff[1]%2)*30);
if ((GOff[1] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4))Apagar(kk); // si pertenece al grupo la apago 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G1 activo hoy
} // si confcas tiene activado auto1 ////////////////////////////////////////////////////////////////////////////



if (bit_test(ConfCas,2)) { // si configuracion casandra tiene activado grupo auto 2

if (bit_test(Gdias[2], dow)) {  // El grupo 2 esta activo el dia de hoy
////// ENCENDIDO
if (GOn[2] == 48) {  // si se prende por fotocelda
//if ((hrs > 12) && (hrs < 24)){
if ((Umbral == Sol) && (EsNoche == false)){
//if (((Umbral - 1) == Sol) || ((Umbral - 2) == Sol) || ((Umbral - 3) == Sol)) { // si vale 1 menos que umbral, o 2 o 3 menos disparamos encendido
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5))Prender(kk); // si pertenece al grupo la prendo 
      } // for...
EsNoche = true;
}
//} // hs ftcel
} 
if (GOn[2] < 48) {  // si se prende por horario
HsAux = (hrs*2)+(GOn[2]%2);
//if (GOn[2]%2) MinAux = 30;
//else MinAux = 0;
MinAux = ((GOn[2]%2)*30);
if ((GOn[2] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5)) Prender(kk); // si pertenece al grupo la prendo 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G2 activo hoy

Miti = GOff[2]/2;
if (( (Miti >= 0) && (Miti < 6) ) || (Miti == 24)) {
dowfalso = dow + 1;
if (dowfalso == 8) dowfalso = 1;
}
else dowfalso = dow;

if (bit_test(Gdias[2], dowfalso)) {  // El grupo 2 esta activo el dia de hoy o ayer, dado que es para apagado
////// APAGADO
if (GOff[2] == 48) {  // si se apaga por fotocelda
//if ((hrs > 0) && (hrs < 12)){
if ((Umbral == Sol) && (EsNoche == true)){
//if (((Umbral + 1) == Sol) || ((Umbral + 2) == Sol) || ((Umbral + 3) == Sol)) { // si vale 1 mas que umbral, o 2 o 3 mas disparamos apagado
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
EsNoche = false;
}
//} // hs ftcel
} 
if (GOff[2] < 48) {  // si se apaga por horario
HsAux = (hrs*2)+(GOff[2]%2);
MinAux = ((GOff[2]%2)*30);
if ((GOff[2] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G2 activo hoy
} // si confcas tiene activado auto2 ///////////////////////////////////////////


if (bit_test(ConfCas,3)) { // si configuracion casandra tiene activado grupo auto 3

if (bit_test(Gdias[3], dow)) {  // El grupo 3 esta activo el dia de hoy
////// ENCENDIDO
if (GOn[3] == 48) {  // si se prende por fotocelda
//if ((hrs > 12) && (hrs < 24)){
if ((Umbral == Sol) && (EsNoche == false)){
//if (((Umbral - 1) == Sol) || ((Umbral - 2) == Sol) || ((Umbral - 3) == Sol)) { // si vale 1 menos que umbral, o 2 o 3 menos disparamos encendido
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Prender(kk); // si pertenece al grupo la prendo 
      } // for...
EsNoche = true;
}
//} // hs ftcel
} 
if (GOn[3] < 48) {
HsAux = (hrs*2)+(GOn[3]%2);
MinAux = ((GOn[3]%2)*30);
if ((GOn[3] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Prender(kk); // si pertenece al grupo la prendo 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G1 activo hoy

Miti = GOff[3]/2;
if (( (Miti >= 0) && (Miti < 6) ) || (Miti == 24)) {
dowfalso = dow + 1;
if (dowfalso == 8) dowfalso = 1;
}
else dowfalso = dow;

if (bit_test(Gdias[3], dowfalso)) {  // El grupo 1 esta activo el dia de hoy o ayer, dado que es para apagado
////// APAGADO
if (GOff[3] == 48) {  // si se apaga por fotocelda
//if ((hrs > 0) && (hrs < 12)){
if ((Umbral == Sol) && (EsNoche == true)){
//if (((Umbral + 1) == Sol) || ((Umbral + 2) == Sol) || ((Umbral + 3) == Sol)) { // si vale 1 mas que umbral, o 2 o 3 mas disparamos apagado
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
EsNoche = false;
}
//} // hs ftcel
}
if (GOff[3] < 48) {  // si se apaga por fotocelda
HsAux = (hrs*2)+(GOff[3]%2);
MinAux = ((GOff[3]%2)*30);
if ((GOff[3] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G3 activo hoy
} // si confcas tiene activado auto3 ////////////////////////////////////////////////////////////////////////////

if (bit_test(ConfCas,4)) { // si configuracion casandra tiene activado grupo auto 2

if (bit_test(Gdias[4], dow)) {  // El grupo 2 esta activo el dia de hoy
////// ENCENDIDO
if (GOn[4] == 48) {  // si se prende por fotocelda
//if ((hrs > 12) && (hrs < 24)){
if ((Umbral == Sol) && (EsNoche == false)){
//if (((Umbral - 1) == Sol) || ((Umbral - 2) == Sol) || ((Umbral - 3) == Sol)) { // si vale 1 menos que umbral, o 2 o 3 menos disparamos encendido
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Prender(kk); // si pertenece al grupo la prendo 
      } // for...
EsNoche = true;
}
//} // hs ftcel
} 
if (GOn[4] < 48) {  // si se prende por horario
HsAux = (hrs*2)+(GOn[4]%2);
MinAux = ((GOn[4]%2)*30);
if ((GOn[4] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Prender(kk); // si pertenece al grupo la prendo 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G4 activo hoy

Miti = GOff[4]/2;
if (( (Miti >= 0) && (Miti < 6) ) || (Miti == 24)) {
dowfalso = dow + 1;
if (dowfalso == 8) dowfalso = 1;
}
else dowfalso = dow;

if (bit_test(Gdias[4], dowfalso)) {  // El grupo 2 esta activo el dia de hoy o ayer, dado que es para apagado
////// APAGADO
if (GOff[4] == 48) {  // si se apaga por fotocelda
//if ((hrs > 0) && (hrs < 12)){
if ((Umbral == Sol) && (EsNoche == true)){
//if (((Umbral + 1) == Sol) || ((Umbral + 2) == Sol) || ((Umbral + 3) == Sol)) { // si vale 1 mas que umbral, o 2 o 3 mas disparamos apagado
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
EsNoche = false;
}
//} //hs fcel
} 
if (GOff[4] < 48) {  // si se apaga por fotocelda
HsAux = (hrs*2)+(GOff[4]%2);
MinAux = ((GOff[4]%2)*30);
if ((GOff[4] == HsAux) && (min == MinAux) && (sec == 00)){ // prendemos por horario
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Apagar(kk); // si pertenece al grupo la apago 
      } // for...
} // prendemos por hs
} // fin por horarios
} // G4 activo hoy
} // si confcas tiene activado auto4 ///////////////////////////////////////////

} // fn



void VerTemp() {   // visor de temperaturas historicas de la semana

LIMPIARLCD;
restart_wdt();
int dia = 1;
int Tmax[4];  // 0 es dow, 1 es grados, 2 es hora y 3 es minuto
int Tmin[4];  // 0 es dow, 1 es grados, 2 es hora y 3 es minuto
int Solc[4];  // 0 es dow, 1 es radiacion, 2 es hora y 3 es minuto
char DDow[8]; // Leyenda de lo que estamos visualizando

int falsodow;

while (dia != 0){ // 
restart_wdt();

falsodow = dow - (dia - 1);
if ((falsodow == 0) || (falsodow > 7)) falsodow = falsodow + 7;

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
dia = 0;
Rebote = true;
} // si cancel
}

if (Rebote == false){
if (input(ARRIBA) == 0){ // si arriba
Rebote = true;
++dia;
if (dia == 8) dia = 7;
} // si abajo
} // si rebote

if (Rebote == false){
if (input(ABAJO) == 0){ // si abajo
Rebote = true;
--dia;
} // si abajo
} // si rebote

lcd_gotoxy(1,1);
printf(lcd_putc, "== Max/Min de%s", DDow);
//printf(lcd_putc, "= falsodow : %u", falsodow);
lcd_gotoxy(1,2);
printf(lcd_putc, "Maxima: %2d%cC (%02d:%02d)", Tmax[1], 210, Tmax[2], Tmax[3]);
lcd_gotoxy(1,3);
printf(lcd_putc, "Minima: %2d%cC (%02d:%02d)", Tmin[1], 210, Tmin[2], Tmin[3]);
lcd_gotoxy(1,4);
printf(lcd_putc, "Sol max:%3u  (%02d:%02d)", Solc[1], Solc[2], Solc[3]);

long puntero;
puntero = (long) ((falsodow * 5) + 95);
Tmax[1] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 97);
Tmax[2] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 98);
Tmax[3] = read_eeprom(puntero);

puntero = (long) ((falsodow * 5) + 130);
Tmin[1] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 132);
Tmin[2] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 133);
Tmin[3] = read_eeprom(puntero);

puntero = (long) ((falsodow * 5) + 165);
Solc[1] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 167);
Solc[2] = read_eeprom(puntero);
puntero = (long) ((falsodow * 5) + 168);
Solc[3] = read_eeprom(puntero);

switch (falsodow){
   case 1: // Hoy
   DDow = " Lun ==";
   break;

   case 2: // Ayer
   DDow = " Mar ==";
   break;

   case 3: // 
   DDow = " Mier =";
   break;

   case 4: // 
   DDow = " Jue ==";
   break;

   case 5: //
   DDow = " Vie ==";
   break;

   case 6: //
   DDow = " Sab ==";
   break;

   case 7: //
   DDow = " Dom ==";
   break;
} // switch

if (falsodow == dow) DDow = " Hoy ==";

} // while
while (input(ABAJO)==0){
Rebote = true;
Menu = 1;
}
//delay_ms(50);
} // fn


void DosVeinte(){  // Toma de lectura analogica de la tension 220

restart_wdt();
float Rr;
float Ss;
float Tt;

long Tensiones[3];

set_adc_channel(FA_R);
Tensiones[0] = read_adc();
Rr = (float) (Tensiones[0]/3.41);
FaseR = Rr;

set_adc_channel(FA_S);
Tensiones[1] = read_adc();
Ss = (float) (Tensiones[1]/3.41);
FaseS = Ss;

set_adc_channel(FA_T);
Tensiones[2] = read_adc();
Tt = (float) (Tensiones[2]/3.41);
FaseT = Tt;

}//fn


void VerTens() { // funcion de muestra de tensiones
LIMPIARLCD;
restart_wdt();

int Nido = 1; // Cuan anidados estamos dentro de la funcion (o sea, en que pantalla)
while (Nido != 0){ // 
restart_wdt();
if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
Nido = 0;
Rebote = true;
LIMPIARLCD;
} // si cancel
} // si rebote
if (Rebote == false){
if (input(ARRIBA) == 0){ // si arriba
Rebote = true;
--Nido;
LIMPIARLCD;
} // si arriba
} // si rebote
if (Rebote == false){
if (input(ABAJO) == 0){ // si abajo
Rebote = true;
++Nido;
if (Nido == 5) Nido = 4; // Tope de pantallas
LIMPIARLCD;
} // si abajo
} // si rebote

if (Nido == 1) {
restart_wdt();
lcd_gotoxy(1,1);
printf(lcd_putc, "=== Tensiones(V) ===");
lcd_gotoxy(1,2);
printf(lcd_putc, " (R) // (S) // (T)  ");
lcd_gotoxy(1,3);
printf(lcd_putc, " %uv ", FaseR);
lcd_gotoxy(8,3);
printf(lcd_putc, " %uv ", FaseS);
lcd_gotoxy(15,3);
printf(lcd_putc, " %uv ", FaseT);
lcd_gotoxy(1,4);
printf(lcd_putc, "   Bateria: %3.1wv   ", Vbata);
}

if (Nido == 2) {
restart_wdt();
lcd_gotoxy(1,1);
printf(lcd_putc, "=Valores de Entrada=");
lcd_gotoxy(1,2);
printf(lcd_putc, "(%02d:%d)%02d:%d %02d:%d %02d:%d", UltIn[1], UltIn[2], UltIn[3], UltIn[4], UltIn[5], UltIn[6], UltIn[7], UltIn[8]);
lcd_gotoxy(1,3);
printf(lcd_putc, " %02d:%d %02d:%d %02d:%d %02d:%d", UltIn[9], UltIn[10], UltIn[11], UltIn[12], UltIn[13], UltIn[14], UltIn[15], UltIn[16]);
lcd_gotoxy(1,4);
printf(lcd_putc, " %02d:%d %02d:%d %02d:%d %02d:%d", UltIn[17], UltIn[18], UltIn[19], UltIn[20], UltIn[21], UltIn[22], UltIn[23], UltIn[24]);
}

if (Nido == 3) {
restart_wdt();
lcd_gotoxy(1,1);
printf(lcd_putc, "== Valores de Bus ==");
lcd_gotoxy(1,2);
printf(lcd_putc, "(%03U:%03U)  (%03U:%03U)", UltBus[1], UltBus[2], UltBus[3], UltBus[4]);
lcd_gotoxy(1,3);
printf(lcd_putc, "(%03U:%03U)  (%03U:%03U)", UltBus[5], UltBus[6], UltBus[7], UltBus[8]);
lcd_gotoxy(1,4);
printf(lcd_putc, "(%03U:%03U)  (%03U:%03U)", UltBus[9], UltBus[10], UltBus[11], UltBus[12]);
}

if (Nido == 4) {
restart_wdt();
lcd_gotoxy(1,1);
printf(lcd_putc, "-%03U-%03U-%03U-%03U-%03U", RawBus[0], RawBus[1], RawBus[2], RawBus[3], RawBus[4]);
lcd_gotoxy(1,2);
printf(lcd_putc, "-%03U-%03U-%03U-%03U-%03U", RawBus[5], RawBus[6], RawBus[7], RawBus[8], RawBus[9]);
lcd_gotoxy(1,3);
printf(lcd_putc, "-%03U-%03U-%03U-%03U-%03U", RawBus[10], RawBus[11], RawBus[12], RawBus[13], RawBus[14]);
lcd_gotoxy(1,4);
printf(lcd_putc, "-%03U-%03U-%03U-%03U-%03U", RawBus[15], RawBus[16], RawBus[17], RawBus[18], RawBus[19]);
}

} /// While nido no sea cero
} // fn

void Fotocel() { // Configuracion del Umbral de la fotocelda

LIMPIARLCD;
restart_wdt();
int submenu = 0;
int umbrr;
umbrr = Umbral;

while (submenu < 1){ // hasta llenar todo

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ // si ok, solo sumo
submenu = 1;
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
submenu = 2;
Rebote = true;
} // si cancel
}

if (Rebote == false){
if (input(ABAJO) == 0){ // si abajo
Rebote = true;
umbrr = umbrr + 5;
if (umbrr > 100) umbrr = 100;
} // si abajo
} // si rebote

if (Rebote == false){
if (input(ARRIBA) == 0){ // si arriba
Rebote = true;
umbrr = umbrr - 5;
if (umbrr > 100) umbrr = 0;
} // si abajo
} // si rebote

lcd_gotoxy(1,1);
printf(lcd_putc, "= Conf. Fotocelula =");
lcd_gotoxy(1,2);
printf(lcd_putc, "Seleccione el nivel ");
lcd_gotoxy(1,3);
printf(lcd_putc, "del umbral de Sol:  ");
lcd_gotoxy(1,4);
//if (submenu == 0){
if (bit_test(tiempo1,0)) printf(lcd_putc, "      -~%3u%%        ", umbrr);
else printf(lcd_putc, "      -~   %%        ");
//}
restart_wdt();
} // while

if (submenu == 1) { // salimos con OK
Umbral = umbrr;
write_eeprom(Umbra,Umbral);
Menu = 1;
}
if (submenu == 2) { // salimos con CANCEL
//Umbral = umbrr;
Menu = 63;
}
} // fn

void ALuces(){ // Que luces perteneces a Auto 1, Auto 2, Auto 3 y Auto 4 en LGrupo[] son bits 4, 5, 6 y 7

LIMPIARLCD;
int submenu = 33;
int grupo = 0;
char Gr1[3];
char Gr2[3];
char Gr3[3];
char Gr4[3];

while (submenu < 95){ // 

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ 
++grupo;
if (grupo == 5) grupo = 0;
rebote = true;
}//si ok
}//rebote

if (Rebote == false){
if (input(CANCEL) == 0){ 
--grupo;
if (grupo == 255) {
Menu = 53;
submenu = 100;
}
rebote = true;
}// si cancel
}//rebote

if (Rebote == false){
if (input(ARRIBA) == 0) { // apretamos arriba, entonces resto numero de luz

if (grupo == 0) {
--submenu;
if (submenu == 32) submenu = 93;
long puntero;
puntero = (long) submenu + 200;
LGrupo[submenu] = read_eeprom(puntero);
}

if (grupo == 1) {
if (bit_test(LGrupo[submenu],4)) {
bit_clear(LGrupo[submenu],4);
//Gr1= "NO";
} else {
bit_set(LGrupo[submenu],4);
//Gr1= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 2) {
if (bit_test(LGrupo[submenu],5)) {
bit_clear(LGrupo[submenu],5);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],5);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 3) {
if (bit_test(LGrupo[submenu],6)) {
bit_clear(LGrupo[submenu],6);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],6);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 4) {
if (bit_test(LGrupo[submenu],7)) {
bit_clear(LGrupo[submenu],7);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],7);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

rebote = true;
} // si arriba
}//rebote

if (Rebote == false){
if (input(ABAJO) == 0) { // apretamos arriba, entonces resto numero de luz

if (grupo == 0) {
++submenu;
if (submenu == 94) submenu = 33;
long puntero;
puntero = (long) submenu + 200;
LGrupo[submenu] = read_eeprom(puntero);
}

if (grupo == 1) {
if (bit_test(LGrupo[submenu],4)) {
bit_clear(LGrupo[submenu],4);
//Gr1= "NO";
} else {
bit_set(LGrupo[submenu],4);
//Gr1= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 2) {
if (bit_test(LGrupo[submenu],5)) {
bit_clear(LGrupo[submenu],5);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],5);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 3) {
if (bit_test(LGrupo[submenu],6)) {
bit_clear(LGrupo[submenu],6);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],6);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

if (grupo == 4) {
if (bit_test(LGrupo[submenu],7)) {
bit_clear(LGrupo[submenu],7);
//Gr2= "NO";
} else {
bit_set(LGrupo[submenu],7);
//Gr2= "SI";
}
long puntero;
puntero = (long) submenu+200;
//Defini();
write_eeprom(puntero,LGrupo[submenu]);
}

rebote = true;
} // si arriba
}//rebote

/////////////////////// LCD ///////////////////////
if (bit_test(LGrupo[submenu],4)) Gr1 = "SI";
else Gr1 = "NO";
if (bit_test(LGrupo[submenu],5)) Gr2 = "SI";
else Gr2 = "NO";
if (bit_test(LGrupo[submenu],6)) Gr3 = "SI";
else Gr3 = "NO";
if (bit_test(LGrupo[submenu],7)) Gr4 = "SI";
else Gr4 = "NO";

lcd_gotoxy(1,1);
printf(lcd_putc, "= Luces Automatic. =");

lcd_gotoxy(1,2);
if (grupo == 0){ // si grupo 0
if (bit_test(tiempo1,0)) printf(lcd_putc, "Luz #%02u pertenece a:",(submenu-32));
else printf(lcd_putc, "Luz     pertenece a:");
} else printf(lcd_putc, "Luz #%02u pertenece a:",(submenu-32));


lcd_gotoxy(1,3);
if (grupo == 1){ // si grupo 1
if (bit_test(tiempo1,0)) printf(lcd_putc, "GA1: -%s- ",Gr1);
else printf(lcd_putc, "GA1: -  - ");
} else printf(lcd_putc, "GA1: -%s- ",Gr1);

lcd_gotoxy(11,3);
if (grupo == 2){ // si grupo 2
if (bit_test(tiempo1,0)) printf(lcd_putc, "GA2: -%s- ",Gr2);
else printf(lcd_putc, "GA2: -  - ");
} else printf(lcd_putc, "GA2: -%s- ",Gr2);

lcd_gotoxy(1,4);
if (grupo == 3){ // si grupo 3
if (bit_test(tiempo1,0)) printf(lcd_putc, "GA3: -%s- ",Gr3);
else printf(lcd_putc, "GA3: -  - ");
} else printf(lcd_putc, "GA3: -%s- ",Gr3);

lcd_gotoxy(11,4);
if (grupo == 4){ // si grupo 4
if (bit_test(tiempo1,0)) printf(lcd_putc, "GA4: -%s- ",Gr4);
else printf(lcd_putc, "GA4: -  - ");
} else printf(lcd_putc, "GA4: -%s- ",Gr4);

restart_wdt();

} // while
} // fn


void AHs(){  // horario de los grupos automaticos

LIMPIARLCD;
int submenu = 1;
int grupo = 1;
//GOn[1] = GOn[2] = GOn[3] = GOn[4] = 0;
//GOff[1] = GOff[2] = GOff[3] = GOff[4] = 0;

while (submenu < 5){ // 

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ 
++submenu;
if (submenu == 4) submenu = 1;
rebote = true;
}//si ok
}//rebote

if (Rebote == false){
if (input(CANCEL) == 0){ 
--submenu;
if (submenu == 0){ 
Menu = 52;
submenu = 100;
}
rebote = true;
}//si ok
}//rebote

if (Rebote == false){
if (input(ARRIBA) == 0) { // apretamos arriba

   if (submenu == 1) { // elegir grupo
      --grupo;
      if (grupo == 0) grupo = 1;
   } 
   
   if (submenu == 2) { // hora de ON
     --GOn[grupo];
     if (GOn[grupo] > 49) GOn[grupo] = 49;
     write_eeprom(7+grupo,GOn[grupo]);
     delay_ms(2);
   }
   
   if (submenu == 3) { // hora de OFF
     --GOff[grupo];
     if (GOff[grupo] > 49) GOff[grupo] = 49;
     write_eeprom(11+grupo,GOff[grupo]);
     delay_ms(2);
   }
rebote = true;
} // si arriba
}//rebote

if (Rebote == false){
if (input(ABAJO) == 0) { // apretamos arriba

   if (submenu == 1) { // elegir grupo
      ++grupo;
      if (grupo == 5) grupo = 4;
   } 
   
   if (submenu == 2) { // hora de ON
     ++GOn[grupo];
     if (GOn[grupo] > 49) GOn[grupo] = 0;
     write_eeprom(7+grupo,GOn[grupo]);
     delay_ms(2);
   }
   
   if (submenu == 3) { // hora de OFF
     ++GOff[grupo];
     if (GOff[grupo] > 49) GOff[grupo] = 0;
     write_eeprom(11+grupo,GOff[grupo]);
     delay_ms(2);
   }
rebote = true;
} // si abajo
}//rebote


/////////////////////// LCD ///////////////////////
lcd_gotoxy(1,1);
printf(lcd_putc, "=====  Horario =====");
lcd_gotoxy(1,2);
if (submenu == 1) { // estamos en grupo
if (grupo == 1){ // si grupo 1
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (1)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 1
if (grupo == 2){ // si grupo 2
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (2)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 2
if (grupo == 3){ // si grupo 3
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (3)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 3
if (grupo == 4){ // si grupo 4
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (4)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 4
} // grupo
else {
if (grupo == 1) printf(lcd_putc, "  Grupo:  AUTO (1)  ");
if (grupo == 2) printf(lcd_putc, "  Grupo:  AUTO (2)  ");
if (grupo == 3) printf(lcd_putc, "  Grupo:  AUTO (3)  ");
if (grupo == 4) printf(lcd_putc, "  Grupo:  AUTO (4)  ");
}

lcd_gotoxy(1,3);
if (submenu == 2) { // Configurando el horario de encendido
if (bit_test(tiempo1,0)) {
if (GOn[grupo] == 48) printf(lcd_putc, " Encendido: Fotocel ");
if (GOn[grupo] == 49) printf(lcd_putc, " Encendido: ---     ");
if ((GOn[grupo] >= 0) && (GOn[grupo] <= 47)) printf(lcd_putc, " Encendido: %02u:%02uhs   ",(GOn[grupo]/2),(GOn[grupo]%2)*30);
} else printf(lcd_putc, " Encendido:         ");
} else {
if (GOn[grupo] == 48) printf(lcd_putc, " Encendido: Fotocel ");
if (GOn[grupo] == 49) printf(lcd_putc, " Encendido: ---     ");
if ((GOn[grupo] >= 0) && (GOn[grupo] <= 47)) printf(lcd_putc, " Encendido: %02u:%02uhs   ",(GOn[grupo]/2),(GOn[grupo]%2)*30);
}

lcd_gotoxy(1,4);
if (submenu == 3) { // Configurando el horario de apagado
if (bit_test(tiempo1,0)) {
if (GOff[grupo] == 48) printf(lcd_putc, " Apagado: Fotocelda ");
if (GOff[grupo] == 49) printf(lcd_putc, " Apagado: ---       ");
if ((GOff[grupo] >= 0) && (GOff[grupo] <= 47)) printf(lcd_putc, " Apagado: %02u:%02uhs   ",(GOff[grupo]/2),(GOff[grupo]%2)*30);
} else printf(lcd_putc, " Apagado:           ");
} else {
if (GOff[grupo] == 48) printf(lcd_putc, " Apagado: Fotocelda ");
if (GOff[grupo] == 49) printf(lcd_putc, " Apagado: ---       ");
if ((GOff[grupo] >= 0) && (GOff[grupo] <= 47)) printf(lcd_putc, " Apagado: %02u:%02uhs   ",(GOff[grupo]/2),(GOff[grupo]%2)*30);
}
restart_wdt();
} // while
} // fn


void ADias(){

LIMPIARLCD;
char Lun[4];
char Mar[4];
char Mie[5];
char Jue[4];
char Vie[4];
char Sab[4];
char Dom[4];

int submenu = 0;
int grupo = 1;

while (submenu < 9){ // hasta llenar todas las luces

if (grupo == 1) {
if (bit_test(Gdias[1],1)){ // Lunes grupo 1 verdadero
Lun = "Lun";
} else { // Lunes es falso
Lun = " X ";
} // Lunes falso
if (bit_test(Gdias[1],2)){ // Martes grupo 1 verdadero
Mar = "Mar";
} else { // Lunes es falso
Mar = " X ";
} // Lunes falso
if (bit_test(Gdias[1],3)){ // Miercoles grupo 1 verdadero
Mie = "Mier";
} else { // Lunes es falso
Mie = " X  ";
} // Lunes falso
if (bit_test(Gdias[1],4)){ // Jueves grupo 1 verdadero
Jue = "Jue";
} else { // Lunes es falso
Jue = " X ";
} // Lunes falso
if (bit_test(Gdias[1],5)){ // Viernes grupo 1 verdadero
Vie = "Vie";
} else { // Lunes es falso
Vie = " X ";
} // Lunes falso
if (bit_test(Gdias[1],6)){ // Sabado grupo 1 verdadero
Sab = "Sab";
} else { // Lunes es falso
Sab = " X ";
} // Lunes falso
if (bit_test(Gdias[1],7)){ // Domingo grupo 1 verdadero
Dom = "Dom";
} else { // Lunes es falso
Dom = " X ";
} // Lunes falso
} // grupo es 1

if (grupo == 2) {
if (bit_test(Gdias[2],1)){ // Lunes grupo 1 verdadero
Lun = "Lun";
} else { // Lunes es falso
Lun = " X ";
} // Lunes falso
if (bit_test(Gdias[2],2)){ // Martes grupo 1 verdadero
Mar = "Mar";
} else { // Lunes es falso
Mar = " X ";
} // Lunes falso
if (bit_test(Gdias[2],3)){ // Miercoles grupo 1 verdadero
Mie = "Mier";
} else { // Lunes es falso
Mie = " X  ";
} // Lunes falso
if (bit_test(Gdias[2],4)){ // Jueves grupo 1 verdadero
Jue = "Jue";
} else { // Lunes es falso
Jue = " X ";
} // Lunes falso
if (bit_test(Gdias[2],5)){ // Viernes grupo 1 verdadero
Vie = "Vie";
} else { // Lunes es falso
Vie = " X ";
} // Lunes falso
if (bit_test(Gdias[2],6)){ // Sabado grupo 1 verdadero
Sab = "Sab";
} else { // Lunes es falso
Sab = " X ";
} // Lunes falso
if (bit_test(Gdias[2],7)){ // Domingo grupo 1 verdadero
Dom = "Dom";
} else { // Lunes es falso
Dom = " X ";
} // Lunes falso
} // grupo es 2

if (grupo == 3) {
if (bit_test(Gdias[3],1)){ // Lunes grupo 1 verdadero
Lun = "Lun";
} else { // Lunes es falso
Lun = " X ";
} // Lunes falso
if (bit_test(Gdias[3],2)){ // Martes grupo 1 verdadero
Mar = "Mar";
} else { // Lunes es falso
Mar = " X ";
} // Lunes falso
if (bit_test(Gdias[3],3)){ // Miercoles grupo 1 verdadero
Mie = "Mier";
} else { // Lunes es falso
Mie = " X  ";
} // Lunes falso
if (bit_test(Gdias[3],4)){ // Jueves grupo 1 verdadero
Jue = "Jue";
} else { // Lunes es falso
Jue = " X ";
} // Lunes falso
if (bit_test(Gdias[3],5)){ // Viernes grupo 1 verdadero
Vie = "Vie";
} else { // Lunes es falso
Vie = " X ";
} // Lunes falso
if (bit_test(Gdias[3],6)){ // Sabado grupo 1 verdadero
Sab = "Sab";
} else { // Lunes es falso
Sab = " X ";
} // Lunes falso
if (bit_test(Gdias[3],7)){ // Domingo grupo 1 verdadero
Dom = "Dom";
} else { // Lunes es falso
Dom = " X ";
} // Lunes falso
} // grupo es 3

if (grupo == 4) {
if (bit_test(Gdias[4],1)){ // Lunes grupo 1 verdadero
Lun = "Lun";
} else { // Lunes es falso
Lun = " X ";
} // Lunes falso
if (bit_test(Gdias[4],2)){ // Martes grupo 1 verdadero
Mar = "Mar";
} else { // Lunes es falso
Mar = " X ";
} // Lunes falso
if (bit_test(Gdias[4],3)){ // Miercoles grupo 1 verdadero
Mie = "Mier";
} else { // Lunes es falso
Mie = " X  ";
} // Lunes falso
if (bit_test(Gdias[4],4)){ // Jueves grupo 1 verdadero
Jue = "Jue";
} else { // Lunes es falso
Jue = " X ";
} // Lunes falso
if (bit_test(Gdias[4],5)){ // Viernes grupo 1 verdadero
Vie = "Vie";
} else { // Lunes es falso
Vie = " X ";
} // Lunes falso
if (bit_test(Gdias[4],6)){ // Sabado grupo 1 verdadero
Sab = "Sab";
} else { // Lunes es falso
Sab = " X ";
} // Lunes falso
if (bit_test(Gdias[4],7)){ // Domingo grupo 1 verdadero
Dom = "Dom";
} else { // Lunes es falso
Dom = " X ";
} // Lunes falso
} // grupo es 4

if (bit_test(tiempo1,0)) {
switch (submenu){
   case 1:
      Lun = "   ";
   break;
   
   case 2:
      Mar = "   ";
   break;
   
   case 3:
      Mie = "    ";
   break;
   
   case 4:
      Jue = "   ";
   break;
   
   case 5:
      Vie = "   ";
   break;
   
   case 6:
      Sab = "   ";
   break;
   
   case 7:
      Dom = "   ";
   break;
} //sw
} // bit test

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ 
++submenu;
if (submenu == 8){ 
// Grabamos en eeprom
write_eeprom(G1Dias,Gdias[1]);
delay_ms(3);
write_eeprom(G2Dias,Gdias[2]);
delay_ms(3);
write_eeprom(G3Dias,Gdias[3]);
delay_ms(3);
write_eeprom(G4Dias,Gdias[4]);
delay_ms(3);
Menu = 51;
submenu = 100;
}
rebote = true;
}//si ok
}//rebote

if (Rebote == false){
if (input(CANCEL) == 0){ 
--submenu;
if (submenu == 255){ 
Menu = 51;
submenu = 100;
}
rebote = true;
}//si ok
}//rebote

if (Rebote == false){
if (input(ARRIBA) == 0){ // apretamos arriba o abajo

   if (submenu == 0) {
      --grupo;
      if (grupo == 0) grupo = 1;
   } else { // submenu entre 1 y 7

      if (grupo == 1) { // estamos en el grupo 1
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // estamos en grupo 1
 
      if (grupo == 2){
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // si grupo 2
      
      if (grupo == 3) { // estamos en el grupo 1
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // estamos en grupo 1
 
      if (grupo == 4){
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // si grupo 2
   } // submenu entre 1 y 7   

rebote = true;
} // si arriba o abajo

if (input(ABAJO) == 0){ // apretamos arriba o abajo

   if (submenu == 0) {
      ++grupo;
      if (grupo == 5) grupo = 4;
   } else { // submenu entre 1 y 7

      if (grupo == 1) { // estamos en el grupo 1
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // estamos en grupo 1
 
      if (grupo == 2){
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // si grupo 2
      
      if (grupo == 3) { // estamos en el grupo 1
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // estamos en grupo 1
 
      if (grupo == 4){
      if (bit_test(Gdias[grupo],submenu)){ // el dia es verdadero
      bit_clear(Gdias[grupo],submenu);
      } // si chequeo 
      else { // el dia es falso
      bit_set(Gdias[grupo],submenu);
      }
      } // si grupo 2
   } // submenu entre 1 y 7   

rebote = true;
} // si arriba o abajo
}//rebote

/////////////////////// LCD ///////////////////////
lcd_gotoxy(1,1);
printf(lcd_putc, "= Seleccionar dias =");
lcd_gotoxy(1,2);
if (submenu == 0) { // estamos en grupo
if (grupo == 1){ // si grupo 1
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (1)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 1
if (grupo == 2){ // si grupo 2
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (2)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 2
if (grupo == 3){ // si grupo 3
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (3)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 1
if (grupo == 4){ // si grupo 4
if (bit_test(tiempo1,0)) printf(lcd_putc, "  Grupo:  AUTO (4)  ");
else printf(lcd_putc, "  Grupo:            ");
} // si el grupo es 2
} // grupo
else { // Estamos en los dias de la semana
if (grupo == 1) printf(lcd_putc, "  Grupo:  AUTO (1)  ");
if (grupo == 2) printf(lcd_putc, "  Grupo:  AUTO (2)  ");
if (grupo == 3) printf(lcd_putc, "  Grupo:  AUTO (3)  ");
if (grupo == 4) printf(lcd_putc, "  Grupo:  AUTO (4)  ");
}
lcd_gotoxy(1,3);
printf(lcd_putc, "%s-%s-%s-%s-%s",Lun,Mar,Mie,Jue,Vie);
lcd_gotoxy(1,4);
printf(lcd_putc, "      %s-%s       ",Sab,Dom);
restart_wdt();
} // while
} // fn



void LuzGrupo(){

LIMPIARLCD;
int submenu = 1;

int kk;
//Defini();
while (submenu < 50){ // hasta llenar todas las luces

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ 
switch (submenu){
   
   case 11:  // Grupo APAGAR TODOS
      for (kk=32;kk<94;kk++) {
      if (IntLuz[kk] > 99){ // estaba apagado
         IntLuz[kk] = IntLuz[kk] - 100; // prendo en el arreglo
         write_eeprom(kk,IntLuz[kk]);
         delay_ms(3);
      } // si estaba apagado..
      } // for...
      BusData(100,80); //printf("%c%c%c%c", 250, 100, 80, 180); // Apagamos todo en la red
      BusData(93,80); //printf("%c%c%c%c", 250, 100, 80, 180); // Apagamos todo en la red
   break;

   case 12:  // Grupo APAGAR Perimetro Jardin 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],1)) Apagar(kk);
      } // for...
   break;

   case 13: // Grupo APAGAR JARDIN 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],2)) Apagar(kk);
      } // for...
   break;

   case 14: // Grupo APAGAR GARAGE 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],3)) Apagar(kk);
      } // for...
   break;

   case 15: // Grupo APAGAR VESTIDOR 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4)) Apagar(kk);
      } // for...
   break;

   case 16: // Grupo APAGAR FUERA CUARTOS 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5)) Apagar(kk);
      } // for...
   break;

   case 17: // Grupo APAGAR AUTO1 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Apagar(kk);
      } // for...
   break;

   case 18: // Grupo APAGAR AUTO2 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Apagar(kk);
      } // for...
   break;


   case 21:  // Grupo PRENDER TODOS
      for (kk=32;kk<94;kk++) {
      if (IntLuz[kk] < 99){ // estaba apagado
         IntLuz[kk] = IntLuz[kk] + 100; // prendo en el arreglo
         write_eeprom(kk,IntLuz[kk]);
         delay_ms(3);
      } // si estaba apagado..
      } // for...
      BusData(100,90); //printf("%c%c%c%c", 250, 100, 90, 190); // Apagamos todo en la red
      BusData(VentPaMa,80); //printf("%c%c%c%c", 250, VentPaMa, 80, VentPaMa+80); // Apago ventilador papa mama
      IntLuz[VentPaMa] = IntLuz[VentPaMa] - 100; // prendo en el arreglo
      write_eeprom(VentPaMa,IntLuz[VentPaMa]);
      delay_ms(3);
      BusData(VentGal,80); //printf("%c%c%c%c", 250, VentGal, 80, VentGal+80); // Apago ventilador galeria
      IntLuz[VentGal] = IntLuz[VentGal] - 100; // prendo en el arreglo
      write_eeprom(VentGal,IntLuz[VentGal]);
      delay_ms(3);
   break;

   case 22:  // Grupo PRENDER EXTERIOR 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],1)) Prender(kk);
      } // for...
   break;

   case 23: // Grupo PRENDER JARDIN 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],2)) Prender(kk);
      } // for...
   break;

   case 24: // Grupo PREDNER GARAGE 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],3)) Prender(kk);
      } // for...
   break;

   case 25: // Grupo PRENDER VESTIDOR 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],4)) Prender(kk);
      } // for...
   break;

   case 26: // Grupo APAGAR FUERA CUARTOS 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],5)) Prender(kk);
      } // for...
   break;

   case 27: // Grupo APAGAR AUTO1 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],6)) Prender(kk);
      } // for...
   break;

   case 28: // Grupo APAGAR AUTO2 
      for (kk=32;kk<94;kk++) {
      if (bit_test(LGrupo[kk],7)) Prender(kk);
      } // for...
   break;
} // switch
if (submenu < 10) submenu = submenu + 10;
Rebote = true;
} // si ok
} //rebote falso

if (Rebote == false){
if (input(CANCEL) == 0){ // salgo al toque con cancel
if (submenu < 10) {
Menu = 32;
submenu = 100;
} 
if (submenu > 10) submenu = submenu - 10;
if (submenu > 10) submenu = submenu - 10;
Rebote = true;
} // si cancel
}


if (Rebote == false){
if (input(ARRIBA) == 0){ // aprieto arriba
if (submenu < 10) { // es menor que 10
submenu = submenu - 1;
if (submenu == 0) submenu = 1;
} else { // submenu es mayor que 10
if (submenu > 20){
submenu = submenu - 10;
} else submenu = submenu + 10;
}
Rebote = true;
} // si Arriba
} // rebote falso



if (Rebote == false){
if (input(ABAJO) == 0){ // aprieto abajo
if (submenu < 10) { // es menor que 10
submenu = submenu + 1;
if (submenu == 9) submenu = 8;
} else { // submenu es mayor que 10
if (submenu > 20){
submenu = submenu - 10;
} else submenu = submenu + 10;
}
Rebote = true;
} // si Arriba
} // rebote falso

////////////////////////////////// GRAFICA ////////////////////////////////////////
lcd_gotoxy(1,1);
printf(lcd_putc, "== Control Grupal ==");
lcd_gotoxy(1,2);
if ((submenu == 11) || (submenu == 21)) printf(lcd_putc, "   %cGrupo? Todas!   ",143);
if ((submenu == 12) || (submenu == 22)) printf(lcd_putc, "   %cGrupo? Perim Jar",143);
if ((submenu == 13) || (submenu == 23)) printf(lcd_putc, "   %cGrupo? Perim Fre",143);
if ((submenu == 14) || (submenu == 24)) printf(lcd_putc, "   %cGrupo? Jardin   ",143);
if ((submenu == 15) || (submenu == 25)) printf(lcd_putc, "   %cGrupo? Auto (1) ",143);
if ((submenu == 16) || (submenu == 26)) printf(lcd_putc, "   %cGrupo? Auto (2) ",143);
if ((submenu == 17) || (submenu == 27)) printf(lcd_putc, "   %cGrupo? Auto (3) ",143);
if ((submenu == 18) || (submenu == 28)) printf(lcd_putc, "   %cGrupo? Auto (4) ",143);
if (submenu == 1) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Todas!   ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 2) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Perim Jar",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 3) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Perim Fre",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 4) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Jardin   ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 5) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Auto (1) ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 6) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Auto (2) ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 7) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Auto (3) ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
if (submenu == 8) {
if (bit_test(tiempo1,0)) printf(lcd_putc, " ~ %cGrupo? Auto (4) ",143);
else printf(lcd_putc, " ~ %cGrupo?          ",143);
}
lcd_gotoxy(1,3);
if ((submenu > 10) && (submenu < 20)) printf(lcd_putc, " ~ ");
else printf(lcd_putc, "   ");
lcd_gotoxy(4,3);
printf(lcd_putc, "Apagar           ");

lcd_gotoxy(1,4);
if (submenu > 20) printf(lcd_putc, " ~ ");
else printf(lcd_putc, "   ");
lcd_gotoxy(4,4);
printf(lcd_putc, "Encender         ");
restart_wdt();
} // dentro de while

} // fn


void LuzInt(){  // Configura la intensidad de las luces

LIMPIARLCD;
int submenu = 33;
short editar = false;
//short ON[99];

while (submenu < 94){ // hasta llenar todas las luces

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ 
if (editar == false) editar = true;
else editar = false;
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // salgo al toque con cancel
if (editar == true) editar = false;
else {
Menu = 33;
submenu = 95;
}
Rebote = true;
} // si cancel
}

if (Rebote == false){
if (input(ABAJO) == 0){ // si abajo
if (editar){ // editar habilitado
if ((IntLuz[submenu] != 27) && (IntLuz[submenu] != 127)) IntLuz[submenu] = IntLuz[submenu] + 3;
if (IntLuz[submenu] > 99) BusData(submenu,IntLuz[submenu]-100); //printf("%c%c%c%c", 250, submenu, IntLuz[submenu]-100, submenu + IntLuz[submenu]-100); // enviamos la intensidad sin el 100
if (IntLuz[submenu] < 99) BusData(submenu,IntLuz[submenu]); //printf("%c%c%c%c", 250, submenu, IntLuz[submenu], submenu + IntLuz[submenu]); // enviamos la intensidad apagada
write_eeprom(submenu,IntLuz[submenu]);
} else { // editar no habilitado
++submenu;
if (submenu == 94) submenu = 33;
}
Rebote = true;
} // si Abajo
} // si rebote


if (Rebote == false){
if (input(ARRIBA) == 0){ // si arriba
if (editar){ // editar habilitado
if ((IntLuz[submenu] != 0 ) && (IntLuz[submenu] != 100 )) IntLuz[submenu] = IntLuz[submenu] - 3;
if (IntLuz[submenu] > 99) BusData(submenu,IntLuz[submenu]-100); //printf("%c%c%c%c", 250, submenu, IntLuz[submenu]-100, submenu + IntLuz[submenu]-100); // enviamos la intensidad sin el 100
if (IntLuz[submenu] < 99) BusData(submenu,IntLuz[submenu]); //printf("%c%c%c%c", 250, submenu, IntLuz[submenu], submenu + IntLuz[submenu]); // enviamos la intensidad apagada
write_eeprom(submenu,IntLuz[submenu]);
} else { // editar no habilitado
--submenu;
if (submenu == 32) submenu = 93;
}
Rebote = true;
} // si Abajo
} // si rebote

lcd_gotoxy(1,1);
printf(lcd_putc, "= Control de luces =");
lcd_gotoxy(1,2);
printf(lcd_putc, "   Configurar la    ");
lcd_gotoxy(1,3);
if (!editar){ // editar falso
if (bit_test(tiempo1,0)){
printf(lcd_putc, " intensidad Luz #%02u ",(submenu-32));
} else printf(lcd_putc, " intensidad Luz     ");
}else { // editar verdadero
printf(lcd_putc, " intensidad Luz #%02u ",(submenu-32));
}
lcd_gotoxy(1,4);
if (editar) { // si editar prendido
if (bit_test(tiempo1,0)){
if (IntLuz[submenu] > 80){ // La luz esta prendida
printf(lcd_putc, "         %u%%        ",( ( 30 - (IntLuz[submenu]-100) ) / 3 ) * 10);
} else { // Luz apagada
printf(lcd_putc, "         %u%%        ",( ( 30 - (IntLuz[submenu]) ) / 3 ) * 10);
}
} else { // si bit si
printf(lcd_putc, "                    ");
}
} else {  // Editar apagado
if (IntLuz[submenu] > 80){ // La luz esta prendida
printf(lcd_putc, "         %u%%        ",( ( 30 - (IntLuz[submenu]-100) ) / 3 ) * 10);
} else { // luz apagada
printf(lcd_putc, "         %u%%        ",( ( 30 - (IntLuz[submenu]) ) / 3 ) * 10);
}
} // else editar (apagado)
} // dentro de while
} // fn


void LuzOnOff(){ // prender y apagar las luces individualmente
LIMPIARLCD;
restart_wdt();
int submenu = 33;

while (submenu < 94){ // hasta llenar todas las luces

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ // si ok, solo sumo
if (IntLuz[submenu] > 80){ // La luz esta prendida
Apagar(submenu);
} else { // La luz esta apagada
Prender(submenu);
}
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // salgo al toque con cancel
Rebote = true;
Menu = 31;
submenu = 95;
} // si cancel
}

if (Rebote == false){
if (input(ABAJO) == 0){ // si abajo
Rebote = true;
++submenu;
if (submenu == 94) submenu = 33;
} // si Abajo
} // si rebote

if (Rebote == false){
if (input(ARRIBA) == 0){ // si abajo
Rebote = true;
--submenu;
if (submenu == 32) submenu = 93;
} // si Abajo
} // si rebote


lcd_gotoxy(1,1);
printf(lcd_putc, "= Control de luces =");
lcd_gotoxy(1,2);
printf(lcd_putc, " Estado de Luz #%02u: ",(submenu-32));
lcd_gotoxy(1,3);
if (IntLuz[submenu] > 80){ // La luz esta prendida
if (bit_test(tiempo1,0)) printf(lcd_putc, "     ENCENDIDA      ");
else printf(lcd_putc, "                    ");
lcd_gotoxy(1,4);
printf(lcd_putc, "  Intensidad: %u%%  ",( ( 30 - (IntLuz[submenu]-100) ) / 3 ) * 10);
} else {
if (bit_test(tiempo1,0)) printf(lcd_putc, "      APAGADA       ");
else printf(lcd_putc, "                    ");
lcd_gotoxy(1,4);
printf(lcd_putc, "  Intensidad: %u%%  ",( ( 30 - (IntLuz[submenu]) ) / 3 ) * 10);
}

} // dentro de while
} // fn


void ActAlarma(){

LIMPIARLCD;
restart_wdt();

int submenu = 1;
int segu; // Auxiliar para contar los segundos
int tpoo; // tiempo para mostrar

while (submenu < 3){ // hasta llenar todo

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ // si ok, solo sumo
if (submenu == 1) ++submenu;
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
if (submenu == 2) ActuaLuz();
if ((submenu == 1) || (submenu == 2)) --submenu;
Rebote = true;
if (submenu == 0) {
submenu=10;
Menu = 21; // para salir con el cancel o el error
}
} // si cancel
}

if (submenu == 1) {
lcd_gotoxy(1,2);
printf(lcd_putc, "   %cDesea activar  ",143);
lcd_gotoxy(1,3);
if (Menu == 121) printf(lcd_putc, "   alarma Total ?   ");
if (Menu == 122) printf(lcd_putc, "alarma de CuartosOK?");
if (Menu == 123) printf(lcd_putc, " alarma en CasaOK?  ");
tpoo = (TpoSalida + 1); // tiempo para mostrar
LuzLED();
} // cuando submenu es 1 (antes de aceptar)

if (submenu == 2) {
if (bit_test(Tiempo1,0)) ONLCDLED;
else OFFLCDLED;
TpoSeguro = 60; // regraba el valor 60 hasta que sale del contador
//EstadoAl = 2;
lcd_gotoxy(1,2);
printf(lcd_putc, "  Armado de alarma  ");
lcd_gotoxy(1,3);
printf(lcd_putc, "      en: %3u       ",tpoo);

/*
if (tpoo == TpoSalida) { //////// Ejecutamos una vez toda la mugre de red

//if ((Menu == 121) || (Menu == 122) || (Menu == 123)) { // TOTAL
if (tpoo > 10) printf("%c%c%c%c", 250, LLHall, 121, LLHall+121); // Todas oscilan durante activacion de alarma total
if (tpoo < 10) printf("%c%c%c%c", 250, LLHall, 122, LLHall+122); // Todas oscilan durante activacion de alarma total
//}
} ////////////////// Mugre de red
*/

if (sec != segu){

if (Menu == 121) { // Total
if (tpoo == TpoSalida) {
BusData(LLHall,121); //printf("%c%c%c%c", 250, 100, 121, 221); // Todas oscilan durante activacion de alarma total
//BusData(VentPaMa,80); //printf("%c%c%c%c", 250, VentPaMa, 80, VentPaMa+80); // Apago ventilador papa mama
//BusData(VentGal,80);  //printf("%c%c%c%c", 250, VentGal, 80, VentGal+80); // Apago ventilador galeria
}
if (tpoo == 10) {
BusData(LLHall,122); //printf("%c%c%c%c", 250, 100, 122, 222); // Todas oscilan durante activacion de alarma total
//BusData(VentPaMa,80); //printf("%c%c%c%c", 250, VentPaMa, 80, VentPaMa+80); // Apago ventilador papa mama
//BusData(VentGal,80); //printf("%c%c%c%c", 250, VentGal, 80, VentGal+80); // Apago ventilador galeria
}
} else { // Demas casos
if (tpoo == TpoSalida) BusData(LLHall,121);  //printf("%c%c%c%c", 250, LLHall, 121, LLHall+121); // Todas oscilan durante activacion de alarma total
if (tpoo == 10) BusData(LLHall,122);  //printf("%c%c%c%c", 250, LLHall, 122, LLHall+122); // Todas oscilan durante activacion de alarma total
} //Else
segu = sec;
--tpoo;
}
} // cuando submenu es 2 (Alarma ya aceptada)


if (tpoo == 0) submenu = 3;
restart_wdt();
} // dentro de while

if (submenu == 3){ // si es 3 es porque se activo, sino seria 10
LuzLED();
if (Menu == 121) { // TOTAL
EstadoAl = 2;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
ActuaLuz();   // Funcion que actualiza las luces en los modulos segun el valor del servidor
restart_wdt();
Menu = 0;
}

if (Menu == 122) { // Nocturna
EstadoAl = 3;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
ActuaLuz();   // Funcion que actualiza las luces en los modulos segun el valor del servidor
restart_wdt();
Menu = 0;
}

if (Menu == 123) { // Exterior
EstadoAl = 4;
write_eeprom(EstAlarma,EstadoAl);
delay_ms(3);
ActuaLuz();   // Funcion que actualiza las luces en los modulos segun el valor del servidor
restart_wdt();
Menu = 0;
}
} // submenu 3

} // Fn

void ConfAlarma(){

LIMPIARLCD;
restart_wdt();

int submenu = 1;
int TiemSal;
int TiemSil;
int tpoo;
int diff;
short prueba;
char test[6];

TiemSal = TpoSalida;
TiemSil = TpoSilencioso;
prueba = false;


while (submenu < 4){ // hasta llenar todo


if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ // si ok, solo sumo
++submenu;
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
--submenu;
Rebote = true;
if (submenu == 0) {
submenu=10;
Menu = 24; // para salir con el cancel o el error
}
} // si cancel
}

if (Rebote == false){
if (input(ABAJO) == 0){ // si arriba
Rebote = true;

   if (submenu == 1) { // arriba dentro de dia de la semana
         ++TiemSal;
         if (TiemSal == 181) TiemSal = 1;
   } // arriba dentro de mes

   if (submenu == 2) { // arriba dentro de mes
         ++TiemSil;
         if (TiemSil == 181)  TiemSil= 1;
   } // arriba dentro de mes

   if (submenu == 3) { // arriba dentro de dia
         prueba=True;
         tpoo = sec;
         test="ON";
         printf("%c%c%c%c", 250, 155, 3, 158); // Prendo la sirena por 3 segundos
         }
} // si Abajo
} // si rebote

if (Rebote == false){
if (input(ARRIBA) == 0){ // si abajo
Rebote = true;

   if (submenu == 1) { // arriba dentro de dia de la semana
         --TiemSal;
         if (TiemSal == 255) TiemSal = 180;
   } // arriba dentro de mes

   if (submenu == 2) { // arriba dentro de mes
         --TiemSil;
         if (TiemSil == 255)  TiemSil= 180;
   } // arriba dentro de mes

   if (submenu == 3) { // arriba dentro de dia
         prueba=True;
         tpoo = sec;
         test="ON";
         printf("%c%c%c%c", 250, 155, 3, 158); // Prendo la sirena por 3 segundos
         }
}
}


lcd_gotoxy(1,1);
printf(lcd_putc, "=== Conf. Alarma ===");
lcd_gotoxy(1,2);
if (submenu == 1){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Tpo Salida: %3us", TiemSal);
else printf(lcd_putc, "Tpo Salida:      ");
} else{
lcd_gotoxy(1,2);
printf(lcd_putc, "Tpo Salida: %3us  ", TiemSal);
}

lcd_gotoxy(1,3);
if (submenu == 2){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Tpo Silencio: %3us",TiemSil);
else printf(lcd_putc, "Tpo Silencio:       ");
} else{
lcd_gotoxy(1,3);
printf(lcd_putc, "Tpo Silencio: %3us",TiemSil);
}

lcd_gotoxy(1,4);
if (submenu == 3){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Prueba sirena: %s",test);
else printf(lcd_putc, "Prueba sirena:        ");
} else {
lcd_gotoxy(1,4);
 printf(lcd_putc, "Prueba sirena: %s",test);
}

restart_wdt();

diff = sec - tpoo;

if (diff > 2) prueba = false;
if (prueba == false) test = "OFF  ";

} // dentro de while

if (submenu == 4) {
TpoSilencioso = TiemSil;  // Cargamos en RAM
TpoSalida = TiemSal;  // Cargamos en RAM

write_eeprom(TpoSil,TpoSilencioso); // Guardamos el valor 
write_eeprom(TpoSal,TpoSalida); // Guardamos el valor 
Menu = 1;
}
} // fn


int Autenticar(){

LIMPIARLCD;
restart_wdt();

int PassCheck[2];

PassCheck[0] = PassCheck[1] = 0;

int submenu = 1;

while (submenu < 7){
restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == False) {
if (input(OOKK) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_clear(PassCheck[0],0);
   bit_set(PassCheck[0],1);
   //++submenu;
   break;

   case 2:
   bit_clear(PassCheck[0],2);
   bit_set(PassCheck[0],3);
   //++submenu;
   break;

   case 3:
   bit_clear(PassCheck[0],4);
   bit_set(PassCheck[0],5);
   //++submenu;
   break;

   case 4:
   bit_clear(PassCheck[0],6);
   bit_set(PassCheck[0],7);
   //++submenu;
   break;
   
   case 5:
   bit_clear(PassCheck[1],0);
   bit_set(PassCheck[1],1);
   //++submenu;
   break;

   case 6:
   bit_clear(PassCheck[1],2);
   bit_set(PassCheck[1],3);
   //++submenu;
   break;
}
++submenu;
} // si OK
} // rebote falso

if (Rebote == False) {
if (input(CANCEL) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_set(PassCheck[0],0);
   bit_set(PassCheck[0],1);
   //++submenu;
   break;

   case 2:
   bit_set(PassCheck[0],2);
   bit_set(PassCheck[0],3);
   //++submenu;
   break;

   case 3:
   bit_set(PassCheck[0],4);
   bit_set(PassCheck[0],5);
   //++submenu;
   break;

   case 4:
   bit_set(PassCheck[0],6);
   bit_set(PassCheck[0],7);
   //++submenu;
   break;

   case 5:
   bit_set(PassCheck[1],0);
   bit_set(PassCheck[1],1);
   //++submenu;
   break;

   case 6:
   bit_set(PassCheck[1],2);
   bit_set(PassCheck[1],3);
   //++submenu;
   break;
} // switch
++submenu;
} // si CANCEL
} // rebote falso


if (Rebote == False) {
if (input(ARRIBA) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_clear(PassCheck[0],0);
   bit_clear(PassCheck[0],1);
   //++submenu;
   break;

   case 2:
   bit_clear(PassCheck[0],2);
   bit_clear(PassCheck[0],3);
   //++submenu;
   break;

   case 3:
   bit_clear(PassCheck[0],4);
   bit_clear(PassCheck[0],5);
   //++submenu;
   break;

   case 4:
   bit_clear(PassCheck[0],6);
   bit_clear(PassCheck[0],7);
   //++submenu;
   break;

   case 5:
   bit_clear(PassCheck[1],0);
   bit_clear(PassCheck[1],1);
   //++submenu;
   break;

   case 6:
   bit_clear(PassCheck[1],2);
   bit_clear(PassCheck[1],3);
   //++submenu;
   break;

} // switch
++submenu;
} // si ARRIBA
} // rebote falso


if (Rebote == False) {
if (input(ABAJO) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_set(PassCheck[0],0);
   bit_clear(PassCheck[0],1);
   //++submenu;
   break;

   case 2:
   bit_set(PassCheck[0],2);
   bit_clear(PassCheck[0],3);
   //++submenu;
   break;

   case 3:
   bit_set(PassCheck[0],4);
   bit_clear(PassCheck[0],5);
   //++submenu;
   break;

   case 4:
   bit_set(PassCheck[0],6);
   bit_clear(PassCheck[0],7);
   //++submenu;
   break;

   case 5:
   bit_set(PassCheck[1],0);
   bit_clear(PassCheck[1],1);
   //++submenu;
   break;

   case 6:
   bit_set(PassCheck[1],2);
   bit_clear(PassCheck[1],3);
   //++submenu;
   break;

} // switch
++submenu;
} // si CANCEL
} // rebote falso

//lcd_gotoxy(1,1);
//printf(lcd_putc, "== Cambiar Clave ===");
lcd_gotoxy(1,2);
printf(lcd_putc, " Ingrese la clave:  ");

switch (submenu){
case 1:
lcd_gotoxy(7,3);
printf(lcd_putc, " %c_____",199);
break;
case 2:
lcd_gotoxy(7,3);
printf(lcd_putc, " *%c____",199);
break;
case 3:
lcd_gotoxy(7,3);
printf(lcd_putc, " **%c___",199);
break;
case 4:
lcd_gotoxy(7,3);
printf(lcd_putc, " ***%c__",199);
break;
case 5:
lcd_gotoxy(7,3);
printf(lcd_putc, " ****%c_",199);
break;
case 6:
lcd_gotoxy(7,3);
printf(lcd_putc, " *****%c",199);
break;

} // Sw subm

restart_wdt();


} // while -- todo lleno cuando salgo de aqui

if ((PassCheck[0] == Passwd[0]) && (PassCheck[1] == Passwd[1]))  { // contraseña comprobada ok
int espera = Tiempo1 + 5;
LIMPIARLCD;
lcd_gotoxy(1,2);
printf(lcd_putc, "   Clave correcta   ");
lcd_gotoxy(1,3);
printf(lcd_putc, "    Bienvenido :)   ");
restart_wdt();
while (espera != Tiempo1) restart_wdt();
return 1;
} else { // contraseña no comprobada ok
int espera = Tiempo1 + 5;
LIMPIARLCD;
lcd_gotoxy(1,2);
lcd_gotoxy(1,2);
printf(lcd_putc, "  Clave incorrecta  ");
lcd_gotoxy(1,3);
printf(lcd_putc, " Acceso denegado :( ");
restart_wdt();
while (espera != Tiempo1) restart_wdt();
//delay_ms(1000);
Menu = 1;
return 0;
}
} // fn


void CClave(){
if (Autenticar()){
restart_wdt();
LIMPIARLCD;

int PassCheck1[2];
int PassCheck2[2];

PassCheck1[0] = PassCheck1[1] = PassCheck2[0] = PassCheck2[1] = 0;

int submenu = 1;

while (submenu < 13){

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas


if (Rebote == False) {
if (input(OOKK) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_clear(PassCheck1[0],0);
   bit_set(PassCheck1[0],1);
   //++submenu;
   break;

case 7:
   bit_clear(PassCheck2[0],0);
   bit_set(PassCheck2[0],1);
   //++submenu;
   break;
   case 2:
   bit_clear(PassCheck1[0],2);
   bit_set(PassCheck1[0],3);
   //++submenu;
   break;

   case 8:
   bit_clear(PassCheck2[0],2);
   bit_set(PassCheck2[0],3);
   //++submenu;
   break;

   case 3:
   bit_clear(PassCheck1[0],4);
   bit_set(PassCheck1[0],5);
   //++submenu;
   break;
   case 9:
   bit_clear(PassCheck2[0],4);
   bit_set(PassCheck2[0],5);
   //++submenu;
   break;

   case 4:
   bit_clear(PassCheck1[0],6);
   bit_set(PassCheck1[0],7);
   //++submenu;
   break;
   case 10:
   bit_clear(PassCheck2[0],6);
   bit_set(PassCheck2[0],7);
   //++submenu;
   break;

   case 5:
   bit_clear(PassCheck1[1],0);
   bit_set(PassCheck1[1],1);
   //++submenu;
   break;
   case 11:
   bit_clear(PassCheck2[1],0);
   bit_set(PassCheck2[1],1);
   //++submenu;
   break;

   case 6:
   bit_clear(PassCheck1[1],2);
   bit_set(PassCheck1[1],3);
   //++submenu;
   break;
   case 12:
   bit_clear(PassCheck2[1],2);
   bit_set(PassCheck2[1],3);
   //++submenu;
   break;
}
++submenu;
} // si OK
} // rebote falso

if (Rebote == False) {
if (input(CANCEL) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_set(PassCheck1[0],0);
   bit_set(PassCheck1[0],1);
   //++submenu;
   break;
   case 7:
   bit_set(PassCheck2[0],0);
   bit_set(PassCheck2[0],1);
   //++submenu;
   break;

   case 2:
   bit_set(PassCheck1[0],2);
   bit_set(PassCheck1[0],3);
   //++submenu;
   break;
   case 8:
   bit_set(PassCheck2[0],2);
   bit_set(PassCheck2[0],3);
   //++submenu;
   break;

   case 3:
   bit_set(PassCheck1[0],4);
   bit_set(PassCheck1[0],5);
   //++submenu;
   break;
   case 9:
   bit_set(PassCheck2[0],4);
   bit_set(PassCheck2[0],5);
   //++submenu;
   break;

   case 4:
   bit_set(PassCheck1[0],6);
   bit_set(PassCheck1[0],7);
   //++submenu;
   break;
   case 10:
   bit_set(PassCheck2[0],6);
   bit_set(PassCheck2[0],7);
   //++submenu;
   break;

   case 5:
   bit_set(PassCheck1[1],0);
   bit_set(PassCheck1[1],1);
   //++submenu;
   break;
   case 11:
   bit_set(PassCheck2[1],0);
   bit_set(PassCheck2[1],1);
   //++submenu;
   break;

   case 6:
   bit_set(PassCheck1[1],2);
   bit_set(PassCheck1[1],3);
   //++submenu;
   break;
   case 12:
   bit_set(PassCheck2[1],2);
   bit_set(PassCheck2[1],3);
   //++submenu;
   break;
} // switch
++submenu;
} // si CANCEL
} // rebote falso


if (Rebote == False) {
if (input(ARRIBA) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_clear(PassCheck1[0],0);
   bit_clear(PassCheck1[0],1);
   //++submenu;
   break;
   case 7:
   bit_clear(PassCheck2[0],0);
   bit_clear(PassCheck2[0],1);
   //++submenu;
   break;

   case 2:
   bit_clear(PassCheck1[0],2);
   bit_clear(PassCheck1[0],3);
   //++submenu;
   break;
   case 8:
   bit_clear(PassCheck2[0],2);
   bit_clear(PassCheck2[0],3);
   //++submenu;
   break;

   case 3:
   bit_clear(PassCheck1[0],4);
   bit_clear(PassCheck1[0],5);
   //++submenu;
   break;
   case 9:
   bit_clear(PassCheck2[0],4);
   bit_clear(PassCheck2[0],5);
   //++submenu;
   break;

   case 4:
   bit_clear(PassCheck1[0],6);
   bit_clear(PassCheck1[0],7);
   //++submenu;
   break;
   case 10:
   bit_clear(PassCheck2[0],6);
   bit_clear(PassCheck2[0],7);
   //++submenu;
   break;

   case 5:
   bit_clear(PassCheck1[1],0);
   bit_clear(PassCheck1[1],1);
   //++submenu;
   break;
   case 11:
   bit_clear(PassCheck2[1],0);
   bit_clear(PassCheck2[1],1);
   //++submenu;
   break;

   case 6:
   bit_clear(PassCheck1[1],2);
   bit_clear(PassCheck1[1],3);
   //++submenu;
   break;
   case 12:
   bit_clear(PassCheck2[1],2);
   bit_clear(PassCheck2[1],3);
   //++submenu;
   break;
} // switch
++submenu;
} // si ARRIBA
} // rebote falso


if (Rebote == False) {
if (input(ABAJO) == 0){
Rebote = True;
switch (submenu){
   case 1:
   bit_set(PassCheck1[0],0);
   bit_clear(PassCheck1[0],1);
   //++submenu;
   break;
   case 7:
   bit_set(PassCheck2[0],0);
   bit_clear(PassCheck2[0],1);
   //++submenu;
   break;

   case 2:
   bit_set(PassCheck1[0],2);
   bit_clear(PassCheck1[0],3);
   //++submenu;
   break;
   case 8:
   bit_set(PassCheck2[0],2);
   bit_clear(PassCheck2[0],3);
   //++submenu;
   break;

   case 3:
   bit_set(PassCheck1[0],4);
   bit_clear(PassCheck1[0],5);
   //++submenu;
   break;
   case 9:
   bit_set(PassCheck2[0],4);
   bit_clear(PassCheck2[0],5);
   //++submenu;
   break;

   case 4:
   bit_set(PassCheck1[0],6);
   bit_clear(PassCheck1[0],7);
   //++submenu;
   break;
   case 10:
   bit_set(PassCheck2[0],6);
   bit_clear(PassCheck2[0],7);
   //++submenu;
   break;

   case 5:
   bit_set(PassCheck1[1],0);
   bit_clear(PassCheck1[1],1);
   //++submenu;
   break;
   case 11:
   bit_set(PassCheck2[1],0);
   bit_clear(PassCheck2[1],1);
   //++submenu;
   break;

   case 6:
   bit_set(PassCheck1[1],2);
   bit_clear(PassCheck1[1],3);
   //++submenu;
   break;
   case 12:
   bit_set(PassCheck2[1],2);
   bit_clear(PassCheck2[1],3);
   //++submenu;
   break;
} // switch
++submenu;
} // si CANCEL
} // rebote falso

lcd_gotoxy(1,1);
printf(lcd_putc, "== Cambiar Clave ===");
lcd_gotoxy(1,2);
printf(lcd_putc, "  Nueva: ");
if (submenu > 6) {
lcd_gotoxy(1,3);
printf(lcd_putc, " Repita: ");
}

switch (submenu){
case 1:
lcd_gotoxy(10,2);
printf(lcd_putc, "%c_____",199);
break;
case 2:
lcd_gotoxy(10,2);
printf(lcd_putc, "*%c____",199);
break;
case 3:
lcd_gotoxy(10,2);
printf(lcd_putc, "**%c___",199);
break;
case 4:
lcd_gotoxy(10,2);
printf(lcd_putc, "***%c__",199);
break;
case 5:
lcd_gotoxy(10,2);
printf(lcd_putc, "****%c_",199);
break;
case 6:
lcd_gotoxy(10,2);
printf(lcd_putc, "*****%c",199);
break;

case 7:
lcd_gotoxy(10,2);
printf(lcd_putc, "******");
lcd_gotoxy(10,3);
printf(lcd_putc, "%c_____",199);
break;
case 8:
lcd_gotoxy(10,3);
printf(lcd_putc, "*%c____",199);
break;
case 9:
lcd_gotoxy(10,3);
printf(lcd_putc, "**%c___",199);
break;
case 10:
lcd_gotoxy(10,3);
printf(lcd_putc, "***%c__",199);
break;
case 11:
lcd_gotoxy(10,3);
printf(lcd_putc, "****%c_",199);
break;
case 12:
lcd_gotoxy(10,3);
printf(lcd_putc, "*****%c",199);
break;
} // Sw subm

restart_wdt();


} // while -- todo lleno cuando salgo de aqui

if (submenu == 13){
if ((PassCheck1[0] == PassCheck2[0]) && (PassCheck1[1] == PassCheck2[1]))  { // contraseña comprobada ok
LIMPIARLCD;
lcd_gotoxy(1,2);
printf(lcd_putc, "  Clave modificada  ");
lcd_gotoxy(1,3);
printf(lcd_putc, "  correctamente :)  ");

Passwd[0] = PassCheck1[0];  // Contraseña a RAM
Passwd[1] = PassCheck1[1];  // Contraseña a RAM

write_eeprom(Password,Passwd[0]);  // Contraseña1 de RAM a ROM
delay_ms(10);
restart_wdt();
write_eeprom(Password + 1,Passwd[1]);  // Contraseña2 de RAM a ROM
delay_ms(1000);
Menu = 1;
} else { // contraseña no comprobada ok
LIMPIARLCD;
lcd_gotoxy(1,2);
printf(lcd_putc, "  Error: Las claves ");
lcd_gotoxy(1,3);
printf(lcd_putc, "   no coinciden :(  ");
restart_wdt();
delay_ms(1000);
Menu = 1;
}
} // if 13
} // si autenticamos
} // fn

void Ccasandra(){  // Configuracion de casandra

LIMPIARLCD;
restart_wdt();

char sServ[3];
char sAuto1[3];
char sAuto2[3];
char sAuto3[3];
char sAuto4[3];
char sWeb[3];

if (bit_test(ConfCas,0)) sServ = "SI";
else sServ = "NO";
if (bit_test(ConfCas,1)) sAuto1 = "SI";
else sAuto1 = "NO";
if (bit_test(ConfCas,2)) sAuto2 = "SI";
else sAuto2 = "NO";
if (bit_test(ConfCas,3)) sAuto3 = "SI";
else sAuto3 = "NO";
if (bit_test(ConfCas,4)) sAuto4 = "SI";
else sAuto4 = "NO";
if (bit_test(ConfCas,5)) sWeb = "SI";
else sWeb = "NO";

int submenu =1;

while (submenu < 7){

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == False) {
if (input(OOKK) == 0){
Rebote = true;
++submenu;
}
}

if (Rebote == False) {
if (input(CANCEL) == 0){
Rebote = true;
--submenu;
if (submenu == 0){
submenu = 8;
Menu = 4;
}
}
}

if (Rebote == False) {
if ((input(ABAJO) == 0) || (input(ARRIBA)) == 0) { // si arriba o abajo togleo
Rebote = true;
   if (submenu == 1) { // dentro de SERV
         if (bit_test(ConfCas,0)){
         bit_clear(ConfCas,0);
         sServ = "NO";
         }
         else {
         bit_set(ConfCas,0);
         sServ = "SI";
         }
   } // arriba dentro de mes

   if (submenu == 2) { // dentro de AUTO1
         if (bit_test(ConfCas,1)){
         bit_clear(ConfCas,1);
         sAuto1 = "NO";
         }
         else {
         bit_set(ConfCas,1);
         sAuto1 = "SI";
         }
   } // arriba dentro de mes

   if (submenu == 3) { // dentro de AUTO2
         if (bit_test(ConfCas,2)){
         bit_clear(ConfCas,2);
         sAuto2 = "NO";
         }
         else {
         bit_set(ConfCas,2);
         sAuto2 = "SI";
         }
   } // arriba dentro de mes

   if (submenu == 4) { // dentro de AUTO3
         if (bit_test(ConfCas,3)){
         bit_clear(ConfCas,3);
         sAuto3 = "NO";
         }
         else {
         bit_set(ConfCas,3);
         sAuto3 = "SI";
         }
   } // arriba dentro de mes

   if (submenu == 5) { // dentro de AUTO4
         if (bit_test(ConfCas,4)){
         bit_clear(ConfCas,4);
         sAuto4 = "NO";
         }
         else {
         bit_set(ConfCas,4);
         sAuto4 = "SI";
         }
   } // arriba dentro de mes

   if (submenu == 6) { // Dentro de Web
         if (bit_test(ConfCas,5)){
         bit_clear(ConfCas,5);
         sWeb = "NO";
         }
         else {
         bit_set(ConfCas,5);
         sWeb = "SI";
         }
   } // arriba dentro de mes

} // si arriba o abajo
} // rebote falso


lcd_gotoxy(1,1);
printf(lcd_putc, "== Conf. Casandra ==");
if ((submenu == 1) || (submenu == 2) || (submenu == 3)) {
lcd_gotoxy(1,2);
printf(lcd_putc, "  Luz Pasillos: ");
lcd_gotoxy(1,3);
printf(lcd_putc, "  Grupo Auto 1: ");
lcd_gotoxy(1,4);
printf(lcd_putc, "  Grupo Auto 2: ");

lcd_gotoxy(16,2);
if (submenu == 1) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sServ);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sServ);

lcd_gotoxy(16,3);
if (submenu == 2) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto1);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto1);

lcd_gotoxy(16,4);
if (submenu == 3) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto2);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto2);

restart_wdt();
} // primeros 3

if (submenu == 4) { // mirando el 4to
lcd_gotoxy(1,2);
printf(lcd_putc, "  Grupo Auto 1: ");
lcd_gotoxy(1,3);
printf(lcd_putc, "  Grupo Auto 2: ");
lcd_gotoxy(1,4);
printf(lcd_putc, "  Grupo Auto 3: ");

lcd_gotoxy(16,2);
if (submenu == 2) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto1);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto1);

lcd_gotoxy(16,3);
if (submenu == 3) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto2);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto2);

lcd_gotoxy(16,4);
if (submenu == 4) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto3);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto3);

restart_wdt();
} // mirando el 4

if (submenu == 5) {  // llegando al ultimo
lcd_gotoxy(1,2);
printf(lcd_putc, "  Grupo Auto 2: ");
lcd_gotoxy(1,3);
printf(lcd_putc, "  Grupo Auto 3: ");
lcd_gotoxy(1,4);
printf(lcd_putc, "  Grupo Auto 4: ");

lcd_gotoxy(16,2);
if (submenu == 3) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto2);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto2);

lcd_gotoxy(16,3);
if (submenu == 4) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto3);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto3);

lcd_gotoxy(16,4);
if (submenu == 5) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto4);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto4);

restart_wdt();
} // mirando el 5

if (submenu == 6) {  // llegando al ultimo
lcd_gotoxy(1,2);
printf(lcd_putc, "  Grupo Auto 3: ");
lcd_gotoxy(1,3);
printf(lcd_putc, "  Grupo Auto 4: ");
lcd_gotoxy(1,4);
printf(lcd_putc, " Ctrl Internet: ");

lcd_gotoxy(16,2);
if (submenu == 4) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto3);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto3);

lcd_gotoxy(16,3);
if (submenu == 5) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sAuto4);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sAuto4);

lcd_gotoxy(16,4);
if (submenu == 6) {
   if (bit_test(tiempo1,0)){
      printf(lcd_putc, " %s  ", sWeb);
   } else printf(lcd_putc, "   ");
} else printf(lcd_putc, " %s  ", sWeb);

restart_wdt();
} // mirando el Ultimo

} // while -- todo lleno cuando salgo de aqui

if (submenu == 7){
write_eeprom(EstCasandra,ConfCas);
delay_ms(5);

if (bit_test(ConfCas,5)) BusData(150,90);  //printf("%c%c%c%c", 250, 150, 90, 150 + 90); // Webserver activado
if ((bit_test(ConfCas,5)) == false) BusData(150,80);  //printf("%c%c%c%c", 250, 150, 80, 150 + 80); // Webserver Apagado

Menu = 1;
} 
} // Fn

void CReset(){

LIMPIARLCD;
short Eleccion = True;

if (Autenticar()){

while (Eleccion){

restart_wdt();

if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false) {

lcd_gotoxy(1,1);
printf(lcd_putc, "= RESETEAR SISTEMA =");
lcd_gotoxy(1,2);
printf(lcd_putc, "%cSe eliminaran todos",165);
lcd_gotoxy(1,3);
printf(lcd_putc, "los datos guardados!");
lcd_gotoxy(1,4);
printf(lcd_putc, "    %cContinuar?    ",143);

if (bit_test(Tiempo1,0)) ONLCDLED;
else OFFLCDLED;


if (input(ARRIBA)==0 || input(ABAJO)==0 || input(CANCEL)==0){
Rebote = True;
Menu = 1;
Eleccion = False;
}
 
if (input(OOKK)==0){  ///////////////////////////////// Destruir todo YA!
Rebote = True;
LIMPIARLCD; // Limpiamos la pantalla
ONLCDLED;

lcd_gotoxy(1,2);
printf(lcd_putc, " Eliminando datos...");

int aux1;
long aux2;

for (aux1=0;aux1<5;aux1++){ // llena todo desde el 0 al 4
TempMax[aux1]=0;    // Temperatura maxima de la semana - dia - hora - minuto
TempMin[aux1]=0;    // Temperatura minima de la semana - dia - hora - minuto 
SolMax[aux1]=0;     // Maximo nivel de radiacion de sol en la semana - dia - hora - minuto
Comando[aux1]=0;    // Carga de EVENTO y VALOR
}

for (aux2=0;aux2<1024;aux2++){ // llena todo desde el 0 al 255
write_eeprom(aux2,0x00);  // la llenamos con 0
delay_ms(2);
restart_wdt();
}

for (aux1=31;aux1<94;aux1++){ // llena todo desde el 32 al 94 
IntLuz[aux1] = 0; // 100%
}

EstadoAl = 1;      // Estado de la alarma
write_eeprom(EstAlarma,EstadoAl);
ConfCas=0;       // Estado del nivel de penetracion de Casandra (apagado todo)
write_eeprom(EstCasandra,ConfCas);
TpoSilencioso = 15;
write_eeprom(TpoSil,TpoSilencioso); // Guardamos el valor
TpoSalida = 15;
write_eeprom(TpoSal,TpoSalida); // Guardamos el valor


/*
port_b_pullups(true);
setup_wdt(WDT_ON);
enable_interrupts(global);
enable_interrupts(int_rda); // Interrupcion de llegada de datos
enable_interrupts(INT_TIMER1); // Interrupcion por timer 1
setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
set_timer1(0);
*/
ds1307_set_date_time(1,1,1,1,0,0,0);  // Inicio de valores para el DS1307
delay_ms(5);
Menu = 1;      // Posicion dentro del menu 0
Eleccion = False;
} // If eliminar todo confirmado
} // si rebote es falso
} // Salimos del while con eleccion
} // si autenticamos correctamente
} // Fn

void CHs(){

LIMPIARLCD;
int mes=month;
int dia=day;
int dias=dow;
int hora=hrs;
int minuto=min;
int anio=year;
char sdia[11];
char smes[4];

restart_wdt();

int submenu=1;

Rebote = true;
//lcd_putc("\f"); // Limpiamos la pantalla

while (submenu < 6){ // hasta llenar todo


if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){
if (input(OOKK) == 0){ // si ok, solo sumo
++submenu;
Rebote = true;
} // si ok
}

if (Rebote == false){
if (input(CANCEL) == 0){ // si cancel solo resto o salgo con 10
--submenu;
Rebote = true;
if (submenu == 0) {
submenu = 10;
}
} // si cancel
}

if (Rebote == false){
if (input(ABAJO) == 0){ // si arriba
Rebote = true;

   if (submenu == 1) { // arriba dentro de dia de la semana
         ++dias;
         if (dias == 8) dias = 1;
   } // arriba dentro de mes

   if (submenu == 2) { // arriba dentro de mes
         ++mes;
         if (mes == 13) mes = 1;
   } // arriba dentro de mes

   if (submenu == 3) { // arriba dentro de dia
         ++dia;
         if ((dia >= 32) && ((mes == 1) || (mes == 3) || (mes == 5) || (mes == 7) || (mes == 8) || (mes == 10) || (mes == 12))) dia = 1;
         if ((dia >= 29) && (mes == 2)) dia = 1;
         if ((dia >= 31) && ((mes == 4) || (mes == 6) || (mes == 9) || (mes == 11))) dia = 1;
   } // arriba dentro de dia

   if (submenu == 4) { // arriba dentro de hora
         ++hora;
         if (hora == 24) hora = 0;
   } // arriba dentro de hora

   if (submenu == 5) { // arriba dentro de minuto
         ++minuto;
         if (minuto == 60) minuto = 0;
   } // arriba dentro de minuto
}
}

if (Rebote == false){
if (input(ARRIBA) == 0){ // si abajo
Rebote = true;

   if (submenu == 1) { // abajo dentro de mes
         --dias;
         if (dias == 0) dias = 7;
   } // arriba dentro de mes

   if (submenu == 2) { // abajo dentro de mes
         --mes;
         if (mes == 0) mes = 12;
   } // arriba dentro de mes

   if (submenu == 3) { // arriba dentro de dia
         --dia;
         if ((dia == 0) && ((mes == 1) || (mes == 3) || (mes == 5) || (mes == 7) || (mes == 8) || (mes == 10) || (mes == 12))) dia = 31;
         if ((dia == 0) && (mes == 2)) dia = 28;
         if ((dia == 0) && ((mes == 4) || (mes == 6) || (mes == 9) || (mes == 11))) dia = 30;
   } // arriba dentro de dia

   if (submenu == 4) { // arriba dentro de hora
         --hora;
         if (hora == 255) hora = 23;
   } // arriba dentro de hora

   if (submenu == 5) { // arriba dentro de minuto
         --minuto;
         if (minuto == 255) minuto = 59;
   } // arriba dentro de minuto

}
}

switch (dias){
   case 1:
   sdia="Lunes    ";
   break;

   case 2:
   sdia="Martes   ";
   break;

   case 3:
   sdia="Miercoles";
   break;

   case 4:
   sdia="Jueves   ";
   break;

   case 5:
   sdia="Viernes  ";
   break;

   case 6:
   sdia="Sabado   ";
   break;

   case 7:
   sdia="Domingo  ";
   break;
}

switch (mes){
   case 1:
   smes="Ene";
   break;

   case 2:
   smes="Feb";
   break;

   case 3:
   smes="Mar";
   break;

   case 4:
   smes="Abr";
   break;

   case 5:
   smes="May";
   break;

   case 6:
   smes="Jun";
   break;

   case 7:
   smes="Jul";
   break;

   case 8:
   smes="Ago";
   break;

   case 9:
   smes="Sep";
   break;

   case 10:
   smes="Oct";
   break;

   case 11:
   smes="Nov";
   break;

   case 12:
   smes="Dic";
   break;
}

lcd_gotoxy(1,1);
printf(lcd_putc, "=== Fecha y hora ===");

lcd_gotoxy(1,2);
if (submenu == 1){
if (bit_test(tiempo1,0)) printf(lcd_putc, "   Dia: %s ", sdia);
else printf(lcd_putc, "   Dia:             ");
} else{
lcd_gotoxy(1,2);
printf(lcd_putc, "   Dia: %s ", sdia);
}

lcd_gotoxy(1,3);
if (submenu == 3){
if (bit_test(tiempo1,0)) printf(lcd_putc, " Nro: %02u   ",dia);
else printf(lcd_putc, " Nro:     ");
} else
{
lcd_gotoxy(1,3);
printf(lcd_putc, " Nro: %02u ",dia);
}

lcd_gotoxy(11,3);
if (submenu == 2){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Mes: %s",smes);
else printf(lcd_putc, "Mes:    ");
} else {
lcd_gotoxy(11,3);
printf(lcd_putc, "Mes: %s",smes);
}

lcd_gotoxy(1,4);
if (submenu == 4){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Hora: %02u ",hora);
else printf(lcd_putc, "Hora:    ");
} else {
lcd_gotoxy(1,4);
printf(lcd_putc, "Hora: %02u ",hora);
}

lcd_gotoxy(10,4);
if (submenu == 5){
if (bit_test(tiempo1,0)) printf(lcd_putc, "Minutos: %02u",minuto);
else printf(lcd_putc, "Minutos:   ");
} else {
lcd_gotoxy(10,4);
printf(lcd_putc, "Minutos: %02u",minuto);
}
restart_wdt();

} // todo lleno cuando salgo de aqui

if (submenu == 10) Menu = 62;
if (submenu == 6) {
Menu = 1;
ds1307_set_date_time(dia,mes,anio,dias,hora,minuto,0);
}

}

void inicio(){  /////////////// INICIO //////////////////////
ONLCDLED;
//// Pullups para el teclado ////
port_b_pullups(true);

/////////  Inicio el WatchDog  ///////////////
setup_wdt(WDT_ON);

//// Interrupciones /////
enable_interrupts(global);
//disable_interrupts(global);
enable_interrupts(int_RDA); // Interrupcion de llegada de datos
enable_interrupts(int_TIMER1); // Interrupcion por timer 1

////// Inicio de dispositivos //////
DHT11_init();
DS1307_init();
lcd_init();
LIMPIARLCD;
lcd_gotoxy(1,2);
printf(lcd_putc, "      CASANDRA      ");
lcd_gotoxy(1,3);
printf(lcd_putc, "    Iniciando....   ");
restart_wdt();
delay_ms(50);

/////////////////////// Arrancamos el timer 1 para medir tiempos internos ////////////
setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
set_timer1(0);

////// Conversor AD //////
setup_adc(ADC_CLOCK_INTERNAL );
setup_adc_ports(AN0_TO_AN3);

////// Inicio de variables ///////////
Menu = 1;      // Posicion dentro del menu 0
Tiempo1=0; // Contador de tiempos internos larga en 0
TempOut=20;    // Temperatura remota (afuera)
Sol=50;       // Intensidad del sol (remota, afuera)
Rebote = false;    // Teclado no presionado
CuentaPanico = 0;  // Contador de cambios de teclas para panico en 0
CuentaLuz = 0; // Arrancamos con todas las luces apagadas
CuentaSens = 0; // Arrancamos con todos los sensores apagados
zZzZ = false;
//Defini();

EstadoAl = read_eeprom(EstAlarma);  // Leo el estado de alarma
ConfCas = read_eeprom(EstCasandra);

if (bit_test(ConfCas,5)) BusData(150,90);  //printf("%c%c%c%c", 250, 150, 90, 150 + 90); // Webserver activado
if ((bit_test(ConfCas,5)) == false) BusData(150,80);  //printf("%c%c%c%c", 250, 150, 80, 150 + 80); // Webserver Apagado

TpoSilencioso = read_eeprom(TpoSil);  // Leo el tiempo silencioso
TpoSalida = read_eeprom(TpoSal);  // Leo el tiempo para salir

Passwd[0] = read_eeprom(Password);  // Contraseña1 de ROM a RAM
Passwd[1] = read_eeprom(Password + 1);  // Contraseña2 de ROM a RAM

Gdias[1] = read_eeprom(G1Dias);
Gdias[2] = read_eeprom(G2Dias);
Gdias[3] = read_eeprom(G3Dias);
Gdias[4] = read_eeprom(G4Dias);
GOn[1] = read_eeprom(G1On);
GOn[2] = read_eeprom(G2On);
GOn[3] = read_eeprom(G3On);
GOn[4] = read_eeprom(G4On);
GOff[1] = read_eeprom(G1Off);
GOff[2] = read_eeprom(G2Off);
GOff[3] = read_eeprom(G3Off);
GOff[4] = read_eeprom(G4Off);
Umbral = read_eeprom(Umbra);


//Passwd[0] = 0;
//Passwd[1] = 0;
//ds1307_set_date_time(1,1,1,1,0,0,0);  // Inicio de valores para el DS1307

restart_wdt();

////////////// Recuperamos intensidades de EEPROM /////////////////
int aux1;
long puntero = 0;

for (aux1=32;aux1<94;aux1++){ // llena todo desde el 32 al 94 
restart_wdt();
IntLuz[aux1] = read_eeprom(aux1);
delay_ms(5);
if (IntLuz[aux1] > 80) { // La luz estaba encendida
BusData(aux1,(IntLuz[aux1]-100));  //printf("%c%c%c%c", 250, aux1, (IntLuz[aux1]-100), aux1+(IntLuz[aux1]-100)); // Le cargo la intensidad
//delay_ms(5);
BusData(aux1,90);  //printf("%c%c%c%c", 250, aux1, 90, aux1+90); // La enciendo
//delay_ms(5);
} else{ // La luz estaba apagada
BusData(aux1,(IntLuz[aux1]));  //printf("%c%c%c%c", 250, aux1, (IntLuz[aux1]), aux1+(IntLuz[aux1])); // Le cargo la intensidad
//delay_ms(5);
}
restart_wdt();

//  Nose porque tengo desactivado esto, habria que probarlo
puntero = (long) aux1 + 200;
LGrupo[aux1] = read_eeprom(puntero);
delay_ms(5);
bit_clear(LGrupo[aux1],1);
bit_clear(LGrupo[aux1],2);
bit_clear(LGrupo[aux1],3);
//
}

/// Definimos las luces de grupos fijos ///     ex Defini();
/// Perfimetro J (bit 1) ///
bit_set(LGrupo[32+36],1);    // 68 Exterior del Estar en red
bit_set(LGrupo[32+25],1);    // 59 Exterior del Hall en red
bit_set(LGrupo[32+50],1);    // 82 Luz Pared de la Galeria
//bit_set(LGrupo[32+],1);    // 83 Luz Frente a la Parrilla
//bit_set(LGrupo[32+],1);    // 85 Luz principal y del ventilado de la Galeria

/// Perfimetro F (bit 2) ///
bit_set(LGrupo[32+11],2);    // 43 Luz del perimetro de cuartos
bit_set(LGrupo[32+25],2);    // 59 Luz de entrada principal y cocina exterior

///    Jardin (bit 3)    ///
bit_set(LGrupo[32+61],3);    // 93 Luz exterior Jardin vapor de Sodio
bit_set(LGrupo[32+24],3);    // 56 Luz ventana del hall
//////////////////////////////////////////

ActuaLuz(); // Mas que nada para refrescar el contador
LIMPIARLCD; // Limpiamos la pantalla

if ((hrs > 12) && (hrs < 24)) EsNoche = false;
else EsNoche = true;

/*while(1){  ///////////////////// Impresion de caracteres
int kkk;

for(kkk=0;kkk<255;++kkk){
   lcd_gotoxy(1,1);
   printf(lcd_putc, "(%03u)%c (%03u)%c (%03u)%c",kkk,kkk,kkk+1,kkk+1,kkk+2,kkk+2);
   lcd_gotoxy(1,2);
   printf(lcd_putc, "(%03u)%c (%03u)%c (%03u)%c",kkk+3,kkk+3,kkk+4,kkk+4,kkk+5,kkk+5);
   lcd_gotoxy(1,3);
   printf(lcd_putc, "(%03u)%c (%03u)%c (%03u)%c",kkk+6,kkk+6,kkk+7,kkk+7,kkk+8,kkk+8);
   lcd_gotoxy(1,4);
   printf(lcd_putc, "(%03u)%c (%03u)%c (%03u)%c",kkk+9,kkk+9,kkk+10,kkk+10,kkk+11,kkk+11);
delay_ms(2000);
}
*/
}  // Fn inicio


void DHT11_init(){
output_float(DHT11_pin);
delay_ms(1000);
}

unsigned char get_byte(){
unsigned char s=0;
unsigned char value=0;

for(s=0;s<8;s+=1)
{
value<<=1;
while(!input(DHT11_pin));
delay_us(30);

if(input(DHT11_pin))
{
value|=1;
}
while(input(DHT11_pin));
}
return value;
}

unsigned char get_data(){
short chk=0;
unsigned char s=0;
unsigned char check_sum=0;

output_high(DHT11_pin);
output_low(DHT11_pin);
delay_ms(18);
output_high(DHT11_pin);
delay_us(26);

chk=input(DHT11_pin);
if(chk)
{
return 1;
}
delay_us(80);

chk=input(DHT11_pin);
if(!chk)
{
return 2;
}
delay_us(80);

for(s=0;s<=4;s+=1)
{
values[s]=get_byte();
}

output_high(DHT11_pin);

for(s=0;s<4;s+=1)
{
check_sum+=values[s];
}

if(check_sum!=values[4])
{
return 3;
}
else
{
return 0;
}
}


////////////////////////// INTERRUPCIONES //////////////////////////////
#int_RDA
void RDA_isr(){
int ii;
if (kbhit()){
Comando[i]=getc();
/*for (ii=0;ii<22;++ii){
RawBus[23-ii] = RawBus[22-ii];
}*/
RawBus[20]=RawBus[19];
RawBus[19]=RawBus[18];
RawBus[18]=RawBus[17];
RawBus[17]=RawBus[16];
RawBus[16]=RawBus[15];
RawBus[15]=RawBus[14];
RawBus[14]=RawBus[13];
RawBus[13]=RawBus[12];
RawBus[12]=RawBus[11];
RawBus[11]=RawBus[10];
RawBus[10]=RawBus[9];
RawBus[9]=RawBus[8];
RawBus[8]=RawBus[7];
RawBus[7]=RawBus[6];
RawBus[6]=RawBus[5];
RawBus[5]=RawBus[4];
RawBus[4]=RawBus[3];
RawBus[3]=RawBus[2];
RawBus[2]=RawBus[1];
RawBus[1]=RawBus[0];
RawBus[0]=Comando[i];
if (Comando[i] == 250) i=0;
else ++i;
if (i==3) Comunica();
}
}

#int_TIMER1 // Entramos aqui cada 524ms ////////////////////////////
void  TIMER1_isr(void) { //Función de interrupción por desbordamiento TMR1  
set_timer1(0); //carga del TMR1
restart_wdt();
Tiempo(); // Tomo el tiempo
//DosVeinte(); // Tomo los valores del ADC
Automaticos();
//AAlarma();
if ((bit_test(tiempo1,0) == 0) && (bit_test(tiempo1,3) == 0)) Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
++Tiempo1;
if (TpoSeguro != 0) --TpoSeguro;
///////PANICO por presion de teclas ////////////
if (bit_test(tiempo1,0) == 0) {
if (CuentaPanico > 0) --CuentaPanico;
if (CuentaPanico > 20){ //PANICO
EstadoAl = 7;
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
}
} // Cuenta panico
} //////////////////////////////// 524 ms //////////////////////////



void Comunica(){ // esta funcion ¿si escribe la EEPROM?
restart_wdt();
i=0;
if (( (Comando[0]) + (Comando[1]) ) == Comando[2]){ // Prueba de checksum

/////// Arreglo de los valores historicos del BUS ///////
UltBus[12] = UltBus[10];
UltBus[11] = UltBus[9];
UltBus[10] = UltBus[8];
UltBus[9]  = UltBus[7];
UltBus[8]  = UltBus[6];
UltBus[7]  = UltBus[5];
UltBus[6]  = UltBus[4];
UltBus[5]  = UltBus[3];
UltBus[4]  = UltBus[2];
UltBus[3]  = UltBus[1];
UltBus[2]  = Comando[1];
UltBus[1]  = Comando[0];
///////////////////////////////////////////////////////

if ((Comando[0] == 29) && (Comando[1] == 29)){ //PANICO
EstadoAl = 7;
write_eeprom(EstAlarma,EstadoAl); // Guardo en ram el estado de la alarma
}
if (Comando[0] == 165) SolAux = Comando[1];
if (Comando[0] == 163) Vbata = Comando[1] - 20; // Le resto 2,5v que es el error medido el 7/5/2016
if (Comando[0] == 160) TempOut = (Comando[1]-10);

if (SolAux < 101) { // Promediamos para descartar cambios violentos y rechazar valores alejados irreales

//if (SolAux > Sol) ++Sol;
//if (SolAux < Sol) --Sol;

//SolAux = (SolAux + Sol) / 2;
//SolAux = (SolAux + Sol) / 2;
Sol = (SolAux + Sol) / 2;
//Sol = SolAux;

}

if (Comando[0] == 184) CorteLuz();

if ((Comando[0] < 95) && (Comando[0] > 31)) { // Comando para alguna luz

/////// Arreglo de los valores historicos de Entradas ///////
int FFor;
for (FFor=24;FFor>=3;FFor--) UltIn[FFor] = UltIn[FFor - 2];
if (Comando[1] == 80) UltIn[2] = 0;
if (Comando[1] == 90) UltIn[2] = 1;
UltIn[1] = Comando[0] - 32;
///////////////////////////////////////////////////////

if (Comando[1] < 40) { // Lo que llego es una intensidad ya que es menor que 27
   if (IntLuz[Comando[0]] > 99) {
   IntLuz[Comando[0]] = Comando[1] + 100; // Le pongo la intensidad sin cambiarle el estado estando prendida
//   write_eeprom(Comando[0],IntLuz[Comando[0]]);
//   delay_ms(3);
   }
   if (IntLuz[Comando[0]] < 99) {
   IntLuz[Comando[0]] = Comando[1]; // Y tambien le pongo la intensidad sin cambiarle el estado estando apagada
//   write_eeprom(Comando[0],IntLuz[Comando[0]]);
//   delay_ms(3);
   }
} // si lo que llegó es una intensidad
if (Comando[1] == 80) { // Lo que llego es un apagado
   if (IntLuz[Comando[0]] > 99) IntLuz[Comando[0]] = IntLuz[Comando[0]] - 100; // Llego apagado y como estaba prendida la apago (-100)
//   write_eeprom(Comando[0],IntLuz[Comando[0]]);
//   delay_ms(3);
   ++CuentaPanico; // se incrementa el panico
//   if (CuentaLuz > 0) --CuentaLuz;
} // si lo que llegó es un apagado
if (Comando[1] > 81) { // Lo que llego es un estado de encendido (puede ser cualquiera)
   if (IntLuz[Comando[0]] < 99) IntLuz[Comando[0]] = IntLuz[Comando[0]] + 100; // Le pongo la intensidad sin cambiarle el estado estando prendida
//   write_eeprom(Comando[0],IntLuz[Comando[0]]);
//   delay_ms(3);
   ++CuentaPanico; // se incrementa el panico
//   ++CuentaLuz;
} // si lo que llegó es un encendido

CuentaSens = 0;
if (IntLuz[SSPasCuar] > 99) {
SPasCuar = True;
++CuentaSens;
}
else SPasCuar = False;

if (IntLuz[SSEstar] > 99) {
SEstar = True;
++CuentaSens;
}
else SEstar = False;

if (IntLuz[SSHall] > 99) {
SHall = True;
++CuentaSens;
}
else SHall = False;

if (IntLuz[SSLiving] > 99) {
SLiving = True;
++CuentaSens;
}
else SLiving = False;
if (IntLuz[SSCocina] > 99) {
SCocina = True;
++CuentaSens;
}
else SCocina = False;

if (IntLuz[SSPasEnt] > 99) {
SPasEnt = True;
++CuentaSens;
}
else SPasEnt = False;

if (IntLuz[SSGaleria] > 99) {
SGaleria = True;
++CuentaSens;
}
else SGaleria = False;

if (IntLuz[SSParrilla] > 99) {
SParrilla = True;
++CuentaSens;
}
else SParrilla = False;

if (IntLuz[SSGarage] > 99) {
SGarage = True;
++CuentaSens;
}
else SGarage = False;

write_eeprom(Comando[0],IntLuz[Comando[0]]);
delay_ms(5);

SensoresLuz(); // Analiza los sensores y banderas para prender luces de servicio
SensoresAl();  // Analiza los sensores y banderas para activar alarma
}// If comando para alguna luz
} // if checksum
} //fn


void CorteLuz(){   // Se está reiniciando alguno de los modulos (falla y WD o corte de luz)

disable_interrupts(int_rda); // Apagamos la interrupcion para no recibir la basura del reinicio
int kk;
for (kk=32;kk<94;kk++) {
restart_wdt();
delay_ms(50);
restart_wdt();
delay_ms(50);
restart_wdt();
delay_ms(50);
restart_wdt();
delay_ms(50);
lcd_gotoxy(1,4);
//printf(lcd_putc, " Cargando luz N: %02u  ",(kk-32));
  if (IntLuz[kk] > 99){ // La luz está prendida
//      printf("%c%c%c%c", 250, kk, IntLuz[kk]-100, (IntLuz[kk]-100)+kk); // Mando la intensidad
      BusData(kk,IntLuz[kk]-100);
//      delay_ms(10);
//      printf("%c%c%c%c", 250, kk, 90, 90+kk); // Mando el encendido
      BusData(kk,90);
//      delay_ms(10);
    } else { // si esta apagada
//      printf("%c%c%c%c", 250, kk, 80, 80+kk); // Mando el apagado
      BusData(kk,80);
//      delay_ms(10);
//      printf("%c%c%c%c", 250, kk, IntLuz[kk], IntLuz[kk]+kk); // Mando la intensidad
      BusData(kk,IntLuz[kk]);
//      delay_ms(10);
    } // else (esta apagada)
} // for...
enable_interrupts(int_rda); // Volvemos a habilitar la interrupcion para seguir con ejecucion normal
}


void Teclado(){

restart_wdt();
if (input(ARRIBA)==1 && input(ABAJO)==1 && input(OOKK)==1 && input(CANCEL)==1) Rebote = false; // Soltamos las teclas

if (Rebote == false){ // Tomamos el comando ya que el antirebote nos dice que esta ok

switch (Menu){

////////////////////////////// Comienza Menu basico ///////////////////////////
   case 0:
   if (input(ARRIBA)==0 || input(ABAJO)==0 || input(OOKK)==0){ // || input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   } 
   ZzZ();
   break;
   
   case 1:  // Resumen ppal
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 2;
   }
   if (input(CANCEL)==0){
   Menu = 0;
   Rebote = True;
   }
   if (input(ARRIBA)==0){
   Rebote = True;
   VerTemp();
   }
   if (input(ABAJO)==0){
   Rebote = True;
   VerTens();
   }
   break;
   
   case 2:  // Menu - Activar alarma
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 3;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 21;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   }
   break;
   
   case 3: // Menu - Luces
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 2;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 4;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 31;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   }
   break;
   
   case 4: // Menu - Casandra
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 3;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 5;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 41;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   }
   break;
   
   case 5: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 4;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 6;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 51;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   }
   break;
   
   case 6: // Menu - Luces
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 5;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 61;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 1;
   }
   break;
   


///////////////////////////////////////////// Comienza menu alarma /////////

   case 21: // Menu - ALARMA Total
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 22;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 121;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 2;
   }
   break;
   
   case 22: // Menu - Alarma noche suave
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 21;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 23;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 122;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 2;
   }
   break;
   
   case 23: // Menu - Alarma noche dura
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 22;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 24;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 123;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 2;
   }
   break;
   
   case 24: // Menu - Alarma conf
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 23;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 124;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 2;
   }
   break;
   

///////////////////////////////////////////// Comienza menu de Luces /////////

   case 31: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 32;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 131;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 3;
   }
   break;
   
   case 32: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 31;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 33;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 132;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 3;
   }
   break;
   
   case 33: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 32;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 133;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 3;
   }
   break;
   


/*//////////////////////////////////////////// Comienza menu de Casandra /////////

   case 41: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 42;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 141;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 4;
   }
   break;
   
   case 42: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 41;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 43;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 142;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 4;
   }
   break;
   
   case 43: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 42;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 44;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 143;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 4;
   }
   break;
   
   case 44: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 43;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 144;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 4;
   }
   break;
   
*/

///////////////////////////////////////////// Comienza menu del Automatico /////////

   case 51: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 52;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 151;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 5;
   }
   break;
   
   case 52: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 51;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 53;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 152;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 5;
   }
   break;
   
   case 53: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 52;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 153;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 5;
   }
   break;
   

///////////////////////////////////////////// Comienza menu de Configuracion /////////

   case 61: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 62;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 161;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 6;
   }
   break;
   
   case 62: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 61;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 63;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 162;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 6;
   }
   break;
   
   case 63: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 62;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 64;
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 163;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 6;
   }
   break;
   
   case 64: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 63;
   }
   if (input(ABAJO)==0){
   Rebote = True;
   Menu = 65;

   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 164;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 6;
   }
   break;

   case 65: // Menu - Grupo automatico
   if (input(ARRIBA)==0){
   Rebote = True;
   Menu = 64;
   }
   if (input(ABAJO)==0){
   }
   if (input(OOKK)==0){
   Rebote = True;
   Menu = 165;
   }
   if (input(CANCEL)==0){
   Rebote = True;
   Menu = 6;
   }
   break;
   
Pantalla(); // Actualizamos la pantalla

} // switch
} // if del antirebote
} // Fn

void Pantalla(){

restart_wdt();
switch (Menu){

   case 0:
//   output_high(LCDLED);
//   lcd_putc("\f"); // Limpiamos la pantalla, por este motivo solo la llamamos cuando es necesario
   break;

   case 1:
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "%c%c%c %02u %c%c%c  %02u:%02u:%02u", Dia[0], Dia[1], Dia[2], day, Mes[0], Mes[1], Mes[2], hrs, min, sec);
   lcd_gotoxy(1,2);
   printf(lcd_putc, " Temp: %2d%cC / %2d%cC  ", TempIn-3, 210, TempOut, 210);
   lcd_gotoxy(1,3);
   printf(lcd_putc, " HR: %02u%%   Luz: %02u" , Hum, Sol);
/*   if (Sol > 5) printf(lcd_putc, " HR: %02u%%   Sol:%3u " , Hum, Sol);
     else         printf(lcd_putc, " HR: %02u%%  Luna:%2u  ", Hum, Sol);
   */
   lcd_gotoxy(1,4);
   if (EstadoAl==1)  printf(lcd_putc, "Al: Off   Act: %02u(%01u)", CuentaLuz, CuentaSens);
   if (EstadoAl==2)  printf(lcd_putc, "Al: Total Act: %02u(%01u)", CuentaLuz, CuentaSens);
   if (EstadoAl==3)  printf(lcd_putc, "Al: Cuar  Act: %02u(%01u)", CuentaLuz, CuentaSens);
   if (EstadoAl==4)  printf(lcd_putc, "Al: Casa  Act: %02u(%01u)", CuentaLuz, CuentaSens);
   if ((EstadoAl==5)|| (EstadoAl==6)|| (EstadoAl==7)){
   if (bit_test(Tiempo1,0)) printf(lcd_putc, " PANICO   Act: %02u(%01u)", CuentaLuz, CuentaSens);
   else                     printf(lcd_putc, "          Act: %02u(%01u)", CuentaLuz, CuentaSens);
   }
   lcd_gotoxy(19,3);
   if (Sol <= Umbral){
   printf(lcd_putc,"* ");
   } else printf(lcd_putc,"  ");
   break;

   case 2:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "======= MENU =======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, " ~ (1) Alarma       ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Luces        ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Casandra     ");
   break;

   case 3:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "======= MENU =======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Alarma       ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (2) Luces        ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Casandra     ");
   break;

   case 4:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "======= MENU =======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (2) Luces        ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (3) Casandra     ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (4) Automaticos  ");
   break;

   case 5:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "======= MENU =======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (3) Casandra     ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (4) Automaticos  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (5) Configuracion");
   break;

   case 6:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "======= MENU =======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (3) Casandra     ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (4) Automaticos  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, " ~ (5) Configuracion");
   break;

   case 21:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "====== Alarma ======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "~ (1) Activar Total ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "  (2) Activar Cuart ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "  (3) Activar Casa  ");
   break;

   case 22:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "====== Alarma ======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "  (1) Activar Total ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "~ (2) Activar Cuart ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "  (3) Activar Casa  ");
   break;

   case 23:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "====== Alarma ======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "  (2) Activar Cuart ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "~ (3) Activar Casa  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "  (4) Configuracion ");
   break;

   case 24:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "====== Alarma ======");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "  (2) Activar Cuart ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "  (3) Activar Casa  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "~ (4) Configuracion ");
   break;

   case 31:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "= Control de luces =");
   lcd_gotoxy(1,2);
   printf(lcd_putc, " ~ (1) Prend/Apagar ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Grupos       ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Regulacion   ");
   break;

   case 32:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "= Control de luces =");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Prend/Apagar ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (2) Grupos       ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Regulacion   ");
   break;

   case 33:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "= Control de luces =");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Prend/Apagar ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Grupos       ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, " ~ (3) Regulacion   ");
   break;
/*
   case 41:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "== Nivel Casandra ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, " ~ (1) Serv y Autom ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Solo Serv    ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Solo Autom   ");
   break;

   case 42:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "== Nivel Casandra ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Serv y Autom ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (2) Solo Serv    ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Solo Autom   ");
   break;

   case 43:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "== Nivel Casandra ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (2) Solo Serv    ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (3) Solo Autom   ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (4) Nada :(      ");
   break;

   case 44:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "== Nivel Casandra ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (2) Solo Serv    ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (3) Solo Autom   ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, " ~ (4) Nada :(      ");
   break;
*/
   case 51:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "==== Automatico ====");
   lcd_gotoxy(1,2);
   printf(lcd_putc, " ~ (1) Elegir dias  ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Horario      ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Elegir luces ");
   break;

   case 52:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "==== Automatico ====");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Elegir dias  ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (2) Horario      ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Elegir luces ");
   break;

   case 53:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "==== Automatico ====");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Elegir dias  ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Horario      ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, " ~ (3) Elegir luces ");
   break;

   case 61:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "=== Configuracion ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, " ~ (1) Cambiar Clave");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (2) Fecha y hora ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Fotocelula   ");
   break;

   case 62:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "=== Configuracion ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (1) Cambiar Clave");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (2) Fecha y hora ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (3) Fotocelula   ");
   break;

   case 63:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "=== Configuracion ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (2) Fecha y hora ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (3) Fotocelula   ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (4) Resetear :O  ");
   break;

   case 64:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "=== Configuracion ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (3) Fotocelula   ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, " ~ (4) Resetear :O  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, "   (5) Acerca de..  ");
   break;


   case 65:
//   Tiempo(); // Tomo el tiempo
//   Ambiente(); // Tomo las variables ambientales (incluidas las remotas con cuidado de no saturar el bus)
//   output_low(LCDLED);
   lcd_gotoxy(1,1);
   printf(lcd_putc, "=== Configuracion ==");
   lcd_gotoxy(1,2);
   printf(lcd_putc, "   (3) Fotocelula   ");
   lcd_gotoxy(1,3);
   printf(lcd_putc, "   (4) Resetear :O  ");
   lcd_gotoxy(1,4);
   printf(lcd_putc, " ~ (5) Acerca de..  ");
   break;
 
}

}

void Tiempo(){
restart_wdt();
ds1307_get_date(day,month,year,dow);
delay_ms(2);

ds1307_get_time(hrs,min,sec);
delay_ms(2);

if ((hrs == 0) && (min == 0) && (sec == 0)){ // Empieza un nuevo dia, reseteamos los valores maximos y minimos
TempMax[0]=0;
TempMin[0]=255;
SolMax[0]=0;
}

switch (dow){
   case 1:
   Dia="Lun";
   break;

   case 2:
   Dia="Mar";
   break;

   case 3:
   Dia="Mie";
   break;

   case 4:
   Dia="Jue";
   break;

   case 5:
   Dia="Vie";
   break;

   case 6:
   Dia="Sab";
   break;

   case 7:
   Dia="Dom";
   break;

   default:
   Dia="??";
   break;
}

switch (month){
   case 1:
   Mes="Ene";
   break;

   case 2:
   Mes="Feb";
   break;

   case 3:
   Mes="Mar";
   break;

   case 4:
   Mes="Abr";
   break;

   case 5:
   Mes="May";
   break;

   case 6:
   Mes="Jun";
   break;

   case 7:
   Mes="Jul";
   break;

   case 8:
   Mes="Ago";
   break;

   case 9:
   Mes="Sep";
   break;

   case 10:
   Mes="Oct";
   break;

   case 11:
   Mes="Nov";
   break;

   case 12:
   Mes="Dic";
   break;
   
   default:
   Dia="??";
   break;

}

}

void Ambiente(){
restart_wdt();
state = get_data();

if (TempOut > TempMax[0]){ // tenemos una nueva temperatura maxima del dia
TempMax[0] = TempOut;
long puntero;
puntero = (long) ((dow * 5) + 95);
write_eeprom(puntero,TempMax[0]);
delay_ms(2);
TempMax[1] = day;
puntero = (long) ((dow * 5) + 96);
write_eeprom(puntero,TempMax[1]);
delay_ms(2);
TempMax[2] = hrs;
puntero = (long) ((dow * 5) + 97);
write_eeprom(puntero,TempMax[2]);
delay_ms(2);
TempMax[3] = min;
puntero = (long) ((dow * 5) + 98);
write_eeprom(puntero,TempMax[3]);
delay_ms(2);
}

if (TempOut < TempMin[0]){ // tenemos una nueva temperatura minima del dia
TempMin[0] = TempOut;
long puntero;
puntero = (long) ((dow * 5) + 130);
write_eeprom(puntero,TempMin[0]);
delay_ms(2);
TempMin[1] = day;
puntero = (long) ((dow * 5) + 131);
write_eeprom(puntero,TempMin[1]);
delay_ms(2);
TempMin[2] = hrs;
puntero = (long) ((dow * 5) + 132);
write_eeprom(puntero,TempMin[2]);
delay_ms(2);
TempMin[3] = min;
puntero = (long) ((dow * 5) + 133);
write_eeprom(puntero,TempMin[3]);
delay_ms(2);
}

if (Sol > SolMax[0]){ // tenemos una nueva radiacion solar maxima
SolMax[0] = Sol;
long puntero;
puntero = (long) ((dow * 5) + 165);
write_eeprom(puntero,SolMax[0]);
delay_ms(2);
SolMax[1] = day;
puntero = (long) ((dow * 5) + 166);
write_eeprom(puntero,SolMax[1]);
delay_ms(2);
SolMax[2] = hrs;
puntero = (long) ((dow * 5) + 167);
write_eeprom(puntero,SolMax[2]);
delay_ms(2);
SolMax[3] = min;
puntero = (long) ((dow * 5) + 168);
write_eeprom(puntero,SolMax[3]);
delay_ms(2);
}
}

void ZzZ(){
LuzLED();
restart_wdt();
lcd_gotoxy(1,1);
printf(lcd_putc,"                    ");
lcd_gotoxy(1,3);
printf(lcd_putc,"                    ");
lcd_gotoxy(1,4);
printf(lcd_putc,"                    ");

if (zZzZ == true) {
if (bit_test(Tiempo1,2)){
lcd_gotoxy(1,2);
printf(lcd_putc,"   ...zZzZzZz...    ");
zZzZ = false;
/*
Prender(33);
Prender(55);
Prender(61);
Prender(72);
Prender(91);
*/
} // si bit tiempo
} else { // ZZZ falso 
if (!bit_test(Tiempo1,2)){
lcd_gotoxy(1,2);
printf(lcd_putc,"   ...ZzZzZzZ...    ");
zZzZ = true;
/*
Prender(33);
Prender(55);
Prender(61);
Prender(72);
Prender(91);
*/
} // si bit tiempo
} // else (zzz falso)

} //fn


void AcercaDe(){ // se lleva el 5% de ROM

LIMPIARLCD;
restart_wdt();
int k=0;
while (input(ARRIBA)==1 && input(ABAJO)==1 && input(CANCEL)==1){
if (k<250) ++k;
lcd_gotoxy(1,1);
if (k==0) printf(lcd_putc,"==                 C");
if (k==5) printf(lcd_putc,"==                C ");
if (k==10) printf(lcd_putc,"==               C  ");
if (k==15) printf(lcd_putc,"==              C  A");
if (k==20) printf(lcd_putc,"==             C  A ");
if (k==25) printf(lcd_putc,"==            C  A  ");
if (k==30) printf(lcd_putc,"==           C  A  S");
if (k==35) printf(lcd_putc,"==          C  A  S ");
if (k==40) printf(lcd_putc,"==         C  A  S  ");
if (k==45) printf(lcd_putc,"==        C  A  S  A");
if (k==50) printf(lcd_putc,"==       C  A  S  A ");
if (k==55) printf(lcd_putc,"==      C  A  S  A  ");
if (k==60) printf(lcd_putc,"==     C  A  S  A  N");
if (k==65) printf(lcd_putc,"==    C  A  S  A  N ");
if (k==70) printf(lcd_putc,"==   C  A  S  A  N  ");
if (k==75) printf(lcd_putc,"==  C  A  S  A  N  D");
if (k==80) printf(lcd_putc,"== C  A  S  A  N  D ");
if (k==85) printf(lcd_putc,"== C A  S  A  N  D  ");
if (k==90) printf(lcd_putc,"== CA  S  A  N  D  R");
if (k==95) printf(lcd_putc, "== CA S  A  N  D  R ");
if (k==100) printf(lcd_putc,"== CAS  A  N  D  R  ");
if (k==105) printf(lcd_putc,"== CAS A  N  D  R  A");
if (k==110) printf(lcd_putc,"== CASA  N  D  R  A ");
if (k==115) printf(lcd_putc,"== CASA N  D  R  A  ");
if (k==120) printf(lcd_putc,"== CASAN  D  R  A   ");
if (k==125) printf(lcd_putc,"== CASAN D  R  A    ");
if (k==130) printf(lcd_putc,"== CASAND  R  A     ");
if (k==135) printf(lcd_putc,"== CASAND R  A      ");
if (k==140) printf(lcd_putc,"== CASANDR  A       ");
if (k==145) printf(lcd_putc,"== CASANDR A        ");
if (k==150) printf(lcd_putc,"== CASANDRA         ");
if (k==155) printf(lcd_putc,"=== CASANDRA       =");
if (k==160) printf(lcd_putc,"==== CASANDRA     ==");
if (k==165) printf(lcd_putc,"===== CASANDRA   ===");
if (k==170) printf(lcd_putc,"===== CASANDRA =====");
if (k==247) printf(lcd_putc,"     CASANDRA %c     ",220);

lcd_gotoxy(1,2);
printf(lcd_putc,"Vers: 2.0a  Jun-2014");
lcd_gotoxy(1,3);
printf(lcd_putc,"Gon & Nico CarvalloM");
lcd_gotoxy(1,4);
printf(lcd_putc,"~gonzacarv@gmail.com");
restart_wdt();
} // while
Menu = 1;

} // fn

