/*========================================================================================================
||   Proyecto: Casandra v3.0                                                                            ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                      ||
||   Fecha: 01/2022                                                                                     ||
||   Compilador: Arduino v1.8.16 (http://arduino.cc)                                                    ||
||   Librerias: ESP8266WiFi (NodeMCU) | PubSubClient (Arduino Client for MQTT)                          ||
||                                                                                                      ||
|| Firmware de los modulos MQTT basados en NodeMCU v1.0. El modulo se comunica mediante protocolo       ||
|| MQTT al broker indicado (con credenciales). Interpreta los datos publicados en el servidor Mosquito  ||
|| y los recodifica al formato de comunicacion del sistema domotico Casandra v1.x y v2.x (250, BYTE1,   ||
|| BYTE2, CHECKSUM). Los datos se ingresan a los controladores de Casandra basados en MCU Microchip     ||
|| 16F877A, directamente al puerto UART en modalidad full duplex.                                       ||
||                                                                                                      ||
=========================================================================================================*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <string>
#include "DHT.h"

#define DHT1PIN D4  // D4
#define DHT2PIN D3  // D3
#define Rele1 4     // Rele1 D2
#define Rele2 5     // Rele2 D1
#define DHTTYPE DHT11
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-COCINA";
const char* mqtt_server = "192.168.0.58"; 
String clientId = "Mosquito-COCINA";
const char* Topico = "Casandra/Cocina/#"; // Solo subscripto al topico de Cocina con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int h1_old = 0; 
int t1_old = 0; 
int h2_old = 0; 
int t2_old = 0; 
int EstadoPIR1 = 15;  // Digital pin D8
int EstadoPIR2 = 13;  // Digital pin D7
int EstadoPIR3 = 12;  // Digital pin D6
int EstadoPIR4 = 14;  // Digital pin D5
//int EstadoPIR5 = 4;   // Digital pin D2
//int EstadoPIR6 = 5;   // Digital pin D1
bool EstadoPIR1_old = false;
bool EstadoPIR2_old = false;
bool EstadoPIR3_old = false;
bool EstadoPIR4_old = false;
//bool EstadoPIR5_old = false;
//bool EstadoPIR6_old = false;
unsigned long lastMsg = 0;
int Trein = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];


DHT dht1(DHT1PIN, DHTTYPE);
DHT dht2(DHT2PIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  const char s[2] = "/";
  char *UlTopic;
  char *PenUlTopic;
  char *Fin;
  char Pload[6];

  memcpy(Pload, payload, length);
  Pload[length] = '\0';

  Fin = strtok(topic, s);
  while (Fin != NULL) {
    PenUlTopic = UlTopic;
    UlTopic = Fin;
    Fin = strtok(NULL, s);
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0)) Hablador(atoi(UlTopic)+32+24, 80);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1)) Hablador(atoi(UlTopic)+32+24, 90);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 2)) Hablador(atoi(UlTopic)+32+24, 120);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 3)) Hablador(atoi(UlTopic)+32+24, 121);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 4)) Hablador(atoi(UlTopic)+32+24, 122);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 5)) Hablador(atoi(UlTopic)+32+24, 123);
  if (!strcmp(PenUlTopic, "LuzIntensidad")){
    float dimer;
    dimer = ( (atoi(Pload) * (-0.27) ) + 27);
    Hablador(atoi(UlTopic)+32+24, int(dimer));
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0) && (atoi(UlTopic) == 25)) {
    digitalWrite(Rele1,HIGH);  //apagar rele 1
    Hablador(80, 80); //avisamos al pic que subio la tecla 24 (corresponde a 80 en la red RS485)
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1) && (atoi(UlTopic) == 25)) {
    digitalWrite(Rele1,LOW);   //prender rele 1
    Hablador(80, 90); //avisamos al pic que bajo la tecla 24 (corresponde a 80 en la red RS485)
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0) && (atoi(UlTopic) == 26)) {
    digitalWrite(Rele2,HIGH);  //apagar rele 2
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1) && (atoi(UlTopic) == 26)) {
    digitalWrite(Rele2,LOW);   //prender rele 2
  }
}

/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y){  // La funcion encargada de hablar en el bus
         Serial.write(250);
         Serial.write(x); 
         Serial.write(y);
         Serial.write(x + y);
         delay(25);
}
void reconnect() {
  int Reseteo = 0;
  // Loopea hasta reconectar
  while (!client.connected()) {
    Serial.print("Intentando conectar a broker MQTT..");
    // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"mqttuser","MQTTpass")) {
      Serial.println("¡Conectado! :D");
      client.subscribe(Topico);
    } else {
      Serial.print("Falla de conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      ++Reseteo;
      if (Reseteo == 10) ESP.reset(); 
      delay(5000);
    }
  }
}

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  pinMode(EstadoPIR1, INPUT);
  pinMode(EstadoPIR2, INPUT);
  pinMode(EstadoPIR3, INPUT);
  pinMode(EstadoPIR4, INPUT);
  //pinMode(EstadoPIR5, INPUT);
  //pinMode(EstadoPIR6, INPUT);
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT);
  Serial.begin(2400);
  dht1.begin();
  delay(10);
  dht2.begin();
  delay(10);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  Serial.begin(2400);
  //wm.resetSettings();

    bool res;
    res = wm.autoConnect(MosqID); // password protected ap
    if(!res) {
        Serial.println("Error al conectar WiFi");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("Conectado! ");
    }

   ArduinoOTA.setHostname(MosqID);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //Hablador(100, 80); // Iniciamos apagando todo
  digitalWrite(Rele1,HIGH);  //apagar rele 1
  digitalWrite(Rele2,HIGH);  //apagar rele 1
  Hablador(67, 80); // Apagamos "Luz exterior de la entrada" por incontrolable
  Hablador(72, 80); // Apagamos "ventilador del estar" por incontrolable
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

/////////////////////////////////////////////// Recepcion de datos y actualizacion MQTT //////////////////////////////
if (Serial.available() > 0) {
  
    buff[ii] = Serial.read();
    if (buff[ii] == 250) ii=0;
      else ++ii;
      if (ii==3) {
        if (( (buff[0]) + (buff[1]) ) == buff[2]){ // Prueba de checksum
          int Payl;
          if (buff[1] == 80) Payl = 0;
          if (buff[1] == 90) Payl = 1;
          if ((buff[0]) < (10+32+24)) {
            snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/LuzEstado/0%d", (buff[0]-(32+24)));
            if ((buff[0]-(32+24)) == 3) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/01");
            if ((buff[0]-(32+24)) == 4) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/02");
            if ((buff[0]-(32+24)) == 5) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/03");
            if ((buff[0]-(32+24)) == 6) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/04");
            snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
          }
          if ((buff[0]) >= (10+32+24)) {
            snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/LuzEstado/%d", (buff[0]-(32+24)));
            if ((buff[0]-(32+24)) == 17) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/05");
            if ((buff[0]-(32+24)) == 18) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/SensorMov/06");
            if ((buff[0]-(32+24)) == 24) { // Si es entrada 24 (luz comedor a rele)
              snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cocina/LuzEstado/25");
              if (Payl) digitalWrite(Rele1,HIGH);  //prender rele 1
              else digitalWrite(Rele1,LOW);        // apagar rele 1
            }
            snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
          }
        client.publish(Topicc, Argu);
        } // Prueba de Checksum
        ii=0;
      } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 1000) {
    ++Trein;
    if (Trein >= 31) Trein = 0;
    lastMsg = now;
    char buffer[4];
    int h1 = (int) dht1.readHumidity();   // Leemos la humedad
    if ((h1 != h1_old) && (h1<100) && (h1>0)){
      h1_old = h1;
      sprintf(buffer, "%d", h1);
      client.publish("Casandra/Cocina/Humedad/01", buffer);
    }
    int t1 = (int) dht1.readTemperature(); // Leemos la temperatura
    if ((t1 != t1_old) && (t1<50) && (t1>0)){
      t1_old = t1;
      sprintf(buffer, "%d", t1);
      client.publish("Casandra/Cocina/Temperatura/01", buffer);
    }
  //publicamos ambos datos
    if (Trein == 5)  {
      sprintf(buffer, "%d", h1_old);
      client.publish("Casandra/Cocina/Humedad/01", buffer);
    }
    if (Trein == 20) {
      sprintf(buffer, "%d", t1_old);
      client.publish("Casandra/Cocina/Temperatura/01", buffer);
    }

      int h2 = (int) dht2.readHumidity();   // Leemos la humedad
    if ((h2 != h2_old) && (h2<100) && (h2>0)){
      h2_old = h2;
      sprintf(buffer, "%d", h2);
      client.publish("Casandra/Cocina/Humedad/02", buffer);
    }
    int t2 = (int) dht2.readTemperature(); // Leemos la temperatura
    if ((t2 != t2_old) && (t2<50) && (t2>0)){
      t2_old = t2;
      sprintf(buffer, "%d", t2);
      client.publish("Casandra/Cocina/Temperatura/02", buffer);
    }
  //publicamos ambos datos
    if (Trein == 10) {
      sprintf(buffer, "%d", h2_old);
      client.publish("Casandra/Cocina/Humedad/02", buffer);
    }
    if (Trein == 25) {
      sprintf(buffer, "%d", t2_old);
      client.publish("Casandra/Cocina/Temperatura/02", buffer);
    }
    
  } // Loop cada 30 segundos

    if (digitalRead(EstadoPIR1) != EstadoPIR1_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPIR1_old = digitalRead(EstadoPIR1);
      if (EstadoPIR1_old) client.publish("Casandra/Cocina/SensorMov/01", "1");
        else client.publish("Casandra/Cocina/SensorMov/01", "0");
    }

        if (digitalRead(EstadoPIR2) != EstadoPIR2_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPIR2_old = digitalRead(EstadoPIR2);
      if (EstadoPIR2_old) client.publish("Casandra/Cocina/SensorMov/02", "1");
        else client.publish("Casandra/Cocina/SensorMov/02", "0");
    }

        if (digitalRead(EstadoPIR3) != EstadoPIR3_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPIR3_old = digitalRead(EstadoPIR3);
      if (EstadoPIR3_old) client.publish("Casandra/Cocina/SensorMov/03", "1");
        else client.publish("Casandra/Cocina/SensorMov/03", "0");
    }

        if (digitalRead(EstadoPIR4) != EstadoPIR4_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPIR4_old = digitalRead(EstadoPIR4);
      if (EstadoPIR4_old) client.publish("Casandra/Cocina/SensorMov/04", "1");
        else client.publish("Casandra/Cocina/SensorMov/04", "0");
    }

}
