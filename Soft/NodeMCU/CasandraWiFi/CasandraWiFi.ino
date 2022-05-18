/*=====================================================================================================
||   Proyecto: Casandra v2.1                                                                         ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                   ||
||   Fecha: 02/2020                                                                                  ||
||   Compilador: Arduino v1.8.13 (http://arduino.cc)                                                 ||
||   Libreria: ESP8266WiFi (NodeMCU)                                                                 ||
||                                                                                                   ||
|| Firmware del modulo Web (webserver) para el control del sistema domotico CASANDRA. El modulo      ||
|| recibe comandos desde LAN con el siguiente formato: http://192.168.0.117/AAAXXXBBBYYY (IP fija    ||
|| con opcion de DHCP), donde XXX e YYY corresponden cada una a un BYTE (0-255) que será reenviado   ||
|| al bus de CASANDRA en el formato correspondiente (250,BYTE1,BYTE2,CHECKSUM), mediante el          ||
|| el transductor SN75176 al par trensado RS485                                                      ||
||                                                                                                   ||
======================================================================================================*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "CasandraL2";           // SSID del AP a conectarse
const char* password = "crallova";  // pass del AP
IPAddress ip(192, 168, 0, 117);     // IP estática
IPAddress gateway(192, 168, 0, 1);  // Puerta de enlace
IPAddress subnet(255, 255, 255, 0); // Red

ESP8266WebServer server(80);

#define LEDpin D7  // Declaramos el piloto de info de estado
#define WiFiLED D6 // Declaramos el pilotode info de conexion WiFi
#define EnTx D8    // Declaramos el habilitador de transmision


void setup() {
  Serial.begin(2400); // Seteamos el puerto serial a la velocidad del bus de Casandra
  delay(100);
  pinMode(LEDpin, OUTPUT);  // Tanto los dos pilotos...
  pinMode(EnTx, OUTPUT);    // ...como el habilitador...
  pinMode(WiFiLED, OUTPUT); // ...son salidas.

  //Serial.println("Conectandose a ");
  //Serial.println(ssid);

 
  WiFi.config(ip, gateway, subnet); // Configura IP estática
  WiFi.begin(ssid, password);

  // Loopea  el LED a 500ms mientras se conecta
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  digitalWrite(WiFiLED,LOW);
  delay(500);
  digitalWrite(WiFiLED,HIGH);
  //Serial.print(".");
  }
  //Serial.println("");
  //Serial.println("Conectado con exito a WiFi!");
  //Serial.print("Mi direccion IP es: ");  Serial.println(WiFi.localIP());

  server.onNotFound(handle_NotFound); // Con esta funcion vamos a analizar los parametros de entrada del navegador
  server.begin();
  //Serial.println("Web server iniciado y listo :)");
}

 /////////////////////////////////// VARIABLES GLOBALES///////////////////////////////////
  
  int C1 = 0; // Comando 1 en formato integer
  int C2 = 0; // Comando 2 en formato integer
  boolean Trabajando = true;  // Variable de habilitacion de funcionamiento
  int ii = 0; // Contador de bus
  int buff[3]; // Lo que llega
  int Destello = 0; // Control del led Piloto
  int Destello2 = 0; // Control del led Piloto
    

void loop() {

//  Destellador(); // Destellador que nos informa el estado de funcionamiento.
  server.handleClient(); // Manejador de llegadas HTTP

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


}

void handle_OnConnect() { // Manejador de conexion a la raiz (/)
  server.send(200, "text/html", SendHTML(1,1)); // Esta es la pagina web que devuelve cuando entramos a la IP raiz
}


void handle_NotFound(){ // Manejador de todas las demas llegadas
  
  /////////////////////////////////// VARIABLES LOCALES DE ARMADO ///////////////////////////////////
  char * Cadena1; // Puntero de la cadena del comando 1
  char * Cadena2; // Puntero de la cadena del comando 2
  char Comando1[3]; // Comando 1 en formato string
  char Comando2[3]; // Comando 2 en formato string

  String CadenaHTTP = server.uri();

  //Serial.println("La cadena completa es");
  //Serial.println(CadenaHTTP);

  Cadena1 = strstr((char *)&CadenaHTTP[0], "AAA");  // Esperamos a recibir el en buffer algo que contega la clave AAA
  if(Cadena1 != 0) { // AAA recibido
      strncpy(Comando1, Cadena1 + 3, 3); // Cortamos los siguientes 3 caracteres de la cadena posterior a la clave
      Comando1[3] = '\0';
      C1 = atoi(Comando1);
        //Serial.println("El valor agarrado de A es: ");
        //Serial.println(C1);
    } else {C1 = 0;} 

  Cadena2 = strstr((char *)&CadenaHTTP[0], "BBB");  // Esperamos a recibir el en buffer algo que contega la clave BBB
  if(Cadena2 != 0) {
      strncpy(Comando2, Cadena2 + 3, 3); // Cortamos los siguientes 3 caracteres de la cadena posterior a la clave
      Comando2[3] = '\0';
      C2 = atoi(Comando2);
        //Serial.println("El valor agarrado de B es: ");
        //Serial.println(C2);
    } else {C2 = 0;} 
  
  if (C1) server.send(200, "text/html", "Comando recibido OK"); 
  else  server.send(200, "text/html", "Comando no reconocido"); 

 /////////////////////////////////// Habilito Bus y LED piloto y mando el comando ///////////////////////////////////
 if ((Trabajando == true) && (C1 != 0)) {

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
} //Fin del manejador NOT FOUND (de todos los comandos distintos a la raiz


/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y){  // La funcion encargada de hablar en el bus
         digitalWrite(EnTx,HIGH); // Llego un comando nuevo, prendemos el LED
         digitalWrite(LEDpin,HIGH);
         delay(5);
         Serial.write(250);
         Serial.write(x); 
         Serial.write(y);
         Serial.write(x + y);
         delay(25);
         digitalWrite(EnTx,LOW);
         digitalWrite(LEDpin,LOW);
         
}

String SendHTML(uint8_t led1stat,uint8_t led2stat){  // Funcion que "arma" la pagina Web en caso de acceder por navegador
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP8266 Web Server</h1>\n";
    ptr +="<h3>Using Station(STA) Mode</h3>\n";
  
   if(led1stat)
  {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
  else
  {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

  if(led2stat)
  {ptr +="<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";}
  else
  {ptr +="<p>LED2 Status: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
