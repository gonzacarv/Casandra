/*=====================================================================================================
||   Proyecto: Casandra v1.1                                                                         ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                   ||
||   Fecha: 11/2015                                                                                  ||
||   Compilador: Arduino v1.6.5 (http://arduino.cc)                                                  ||
||   Libreria: Ethercard-master (https://github.com/jcw/ethercard)                                   ||
||                                                                                                   ||
|| Firmware del modulo Web (webserver) para el control del sistema domotico CASANDRA. El modulo      ||
|| recibe comandos desde LAN con el siguiente formato: http://192.168.0.117/AAAXXXBBBYYY (IP fija    ||
|| con opcion de DHCP), donde XXX e YYY corresponden cada una a un BYTE (0-255) que será reenviado   ||
|| al bus de CASANDRA en el formato correspondiente (250,BYTE1,BYTE2,CHECKSUM), mediante el          ||
|| el transductor SN75176 al par trensado                                                            ||
||                                                                                                   ||
======================================================================================================*/

#include <EtherCard.h>

// MacAdress configurable
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
// Direccion IP configurable (ver libreria para habilitar DHCP
static byte myip[] = { 192,168,0,117 };
// Puerta de enlace configurable
static byte gwip[] = { 192,168,0,1 };

byte Ethernet::buffer[500]; // Buffer de recepcion tcp/ip de ethernet

const char page[] PROGMEM =
"HTTP/1.0 503 Service Unavailable\r\n"
"Content-Type: text/html\r\n"
"Retry-After: 600\r\n"
"\r\n"
"<html>"
  "<head><title>"
    "-= CASANDRA v2.0a =-"
  "</title></head>"
  "<body>"
    "<h3>El modulo se encuentra en linea</h3>"
    "<p><em>"
      "El modulo para aplicacion Android/Internet se encuentra operativo<br />"
      "Desarrollo por Gon y Nico Carvallo (gonzacarv@gmail.com)."
    "</em></p>"
  "</body>"
"</html>"
;

char* statusLabel;
char* buttonLabel;

void setup(){
  Serial.begin(2400); // Baudios del bus de Casandra
  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip, gwip); // IP estatica
  pinMode(6, OUTPUT); // LED Piloto 
  pinMode(7, OUTPUT); // Habilitador para hablar en el bus 
}

 /////////////////////////////////// VARIABLES GLOBALES///////////////////////////////////
  
  int C1 = 0; // Comando 1 en formato integer
  int C2 = 0; // Comando 2 en formato integer
  boolean Trabajando = true;  // Variable de habilitacion de funcionamiento
  int ii = 0; // Contador de bus
  int buff[3]; // Lo que llega
  int Destello = 0; // Control del led Piloto
  int Destello2 = 0; // Control del led Piloto
    
void loop(){

Destellador();

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if ((len != 0) && (pos != 0)) { // Entramos solo cuando recibimos un paquete

  /////////////////////////////////// VARIABLES ///////////////////////////////////
  char * Cadena1; // Puntero de la cadena del comando 1
  char * Cadena2; // Puntero de la cadena del comando 2
  char Comando1[3]; // Comando 1 en formato string
  char Comando2[3]; // Comando 2 en formato string

  /////////////////////////////////// Rastreamos el comando y lo convertimos a integer ///////////////////////////////////
Cadena1 = strstr((char *)Ethernet::buffer + pos, "AAA");  // Esperamos a recibir el en buffer algo que contega la clave AAA
  if(Cadena1 != 0) { // AAA recibido
      strncpy(Comando1, Cadena1 + 3, 3); // Cortamos los siguientes 3 caracteres de la cadena posterior a la clave
      Comando1[3] = '\0';
      C1 = atoi(Comando1);
    }

Cadena2 = strstr((char *)Ethernet::buffer + pos, "BBB");  // Esperamos a recibir el en buffer algo que contega la clave BBB
  if(Cadena2 != 0) {
      strncpy(Comando2, Cadena2 + 3, 3); // Cortamos los siguientes 3 caracteres de la cadena posterior a la clave
      Comando2[3] = '\0';
      C2 = atoi(Comando2);
    } 
    
    memcpy_P(ether.tcpOffset(), page, sizeof page); // Preparamos la rta
    ether.httpServerReply(sizeof page - 1); // Enviamos la rta

 /////////////////////////////////// Habilito Bus y LED piloto y mando el comando ///////////////////////////////////
 if (Trabajando) {

     if (C1 < 101) {    
        Hablador(C1,C2);
        delay(15);
     } // C1 menor que 101

     if (C1 == 101) {    
        Hablador(63,C2); // Cocina
        delay(15);
        Hablador(68,C2); // Ext Estar
        delay(15);
        Hablador(70,C2); // Living
        delay(15);
        Hablador(75,C2); // Garage
        delay(15);
        Hablador(82,C2); // Galeria Pared
        delay(15);
     } // C1 = 101
  
     if (C1 == 102) {    
        Hablador(43,C2); // Perimetro LED
        delay(15);
        Hablador(57,C2); // Perimetro entrada
        delay(15);
     } // C1 = 102
  
     if (C1 == 103) {    
        Hablador(43,C2); // Perimetro LED
        delay(15);
        Hablador(57,C2); // Perimetro entrada
        delay(15);
        Hablador(82,C2); // Galeria Pared
        delay(15);
        Hablador(68,C2); // Ext Estar
        delay(15);
        Hablador(56,C2); // Ventana Hall
        delay(15);
        Hablador(75,C2); // Garage
        delay(15);
     } // C1 = 103
  
     if (C1 == 104) {    
        Hablador(47,C2); // Pasillo estar
        delay(15);
        Hablador(48,C2); // Pasillo cuartos
        delay(15);
        Hablador(63,C2); // Cocina
        delay(15);
        Hablador(75,C2); // Garage
        delay(15);
        Hablador(58,C2); // Hall
        delay(15);
     } // C1 = 104
  
     if (C1 == 105) {    
        Hablador(82,C2); // Galeria Pared
        delay(15);
        Hablador(68,C2); // Ext Estar
        delay(15);
        Hablador(56,C2); // Ventana Hall
        delay(25);
        Hablador(93,C2); // Sodios
        delay(25);
     } // C1 = 105
  
 
 }// Trabajando
 }// Recinbimos paquete por internet
 len = 0;
 pos = 0;

/////////////////////////////////////////////// Activador o desactivador //////////////////////////////
if (Serial.available() > 0) {
    buff[ii] = Serial.read();
    if (buff[ii] == 250) ii=0;
      else ++ii;
      if (ii==3) {
        if (( (buff[0]) + (buff[1]) ) == buff[2]){ // Prueba de checksum
          if((buff[0] == 150) && (buff[1] == 90)) Trabajando = true;
          if((buff[0] == 150) && (buff[1] == 80)) Trabajando = false;
        } // Prueba de Checksum
      } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
/////////////////////////////////////////////// Activador o desactivador //////////////////////////////

} // Prog


/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y){  // La funcion encargada de hablar en el bus
         digitalWrite(6,HIGH); // Llego un comando nuevo, prendemos el LED
         digitalWrite(7,HIGH);
         delay(5);
         Serial.write(250);
         Serial.write(x);
         Serial.write(y);
         Serial.write(x + y);
         delay(25);
         digitalWrite(6,LOW);
         digitalWrite(7,LOW);
         
}

void Destellador() {  // La funcion encargada manejar el piloto
++Destello;
if (Destello == 101) {
  Destello = 0;
  ++Destello2;
} // destello
if (Destello2 == 201) Destello2 = 0;

if (Trabajando){
if (Destello2 > 20) digitalWrite(6,HIGH);
else digitalWrite(6,LOW);
} // Esta trabajando, iluminado mucho
else {
if (Destello2 > 180) digitalWrite(6,HIGH);
else digitalWrite(6,LOW);
} // No esta trabajando iluminado poco

} //fn
