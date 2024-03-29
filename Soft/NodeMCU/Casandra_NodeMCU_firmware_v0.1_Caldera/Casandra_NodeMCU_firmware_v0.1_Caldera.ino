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

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <string>
#include <OneWire.h>
#include <DallasTemperature.h>

#define Rele1 0         // Rele1 D3
#define Rele2 2         // Rele2 D4
//#define Rele3 5         // Rele3 D1
#define Lluvia 13       // Lluvia D7
#define Termostato 14   // Termostato D5
#define MSG_BUFFER_SIZE  (50)

const int oneWireBus = 12;  // D6
const int analogInPin = A0;
const char* MosqID = "Mosquito-CALDERA";
const char* mqtt_server = "192.168.0.100"; 
String clientId;
String ClienteID = "Mosquito-CALDERA";
const char* Topico = "Casandra/Caldera/#"; // Solo subscripto al topico de Caldera con comodin aguas abajo
//int EstadoPIR1 = 15;  // Digital pin D8
//int Lluvia = 13;  // Digital pin D7
int LuzIntens = 0; 
int TempC_old = 0;
int Luz_old = 0;
//int EstadoPIR2 = 13;  // Digital pin D7
//bool EstadoPIR1_old = false;
bool Lluvia_old = false;
//bool EstadoPIR2_old = false;
unsigned long Loop1 = 0;
unsigned long Loop2 = 0;
unsigned long Loop3 = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
int Cuenta = 0;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
int CuentaErrorMQTT = 0;

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(oneWireBus);
DallasTemperature SensTemp(&oneWire);

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
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0)) {
    switch(atoi(UlTopic)) {
      case 1: digitalWrite(Rele1,HIGH); 
      break;
      case 2: digitalWrite(Rele2,HIGH); 
      break;
//      case 3: digitalWrite(Rele3,HIGH); 
//      break;
    }
  }
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1)) {
    switch(atoi(UlTopic)) {
      case 1: digitalWrite(Rele1,LOW); 
      break;
      case 2: digitalWrite(Rele2,LOW); 
//      break;
//      case 3: digitalWrite(Rele3,LOW); 
//      break;
    }
  }

  if ( (!strcmp(PenUlTopic, "Caldera")) && (!strcmp(UlTopic, "Termostato")) && (atoi(Pload) == 1)) digitalWrite(Termostato,LOW); 
  if ( (!strcmp(PenUlTopic, "Caldera")) && (!strcmp(UlTopic, "Termostato")) && (atoi(Pload) == 0)) digitalWrite(Termostato,HIGH); 

}

void LaOTA(){
  
  ArduinoOTA.setHostname(MosqID);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
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
}

void MQTTConect(){
    ReconectMosq = true;
    Ejecutando = false;
    if (!client.connected()) {
    Serial.print("Intentando conectar a broker MQTT..");
    clientId = ClienteID + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), "mqttuser", "MQTTpass")) {
      Serial.println("¡Conectado! :D");
      client.subscribe(Topico);
    } else {
      Serial.print("Falla de conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo...");
      ++CuentaErrorMQTT;
      if (CuentaErrorMQTT > 100) ESP.reset();
      }
      delay(1000);
    }
  }

void setup() {
  Serial.begin(9600);
  pinMode(Lluvia, INPUT);
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT);
  pinMode(Termostato, OUTPUT);
  WiFiManager wifiManager;
  ResetMosq = true; // Estamos iniciando desde un reset
  Ejecutando = false;
  
  // Descomentar para resetear configuración
  //wifiManager.resetSettings();

  wifiManager.setConfigPortalTimeout(180);
  if(!wifiManager.autoConnect("MosqID")){
    Serial.println("Fallo en la conexión (timeout)");
    ESP.reset();
    delay(1000);
  }
  Serial.println("Conectado a WiFi");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
  LaOTA();
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  digitalWrite(Termostato,HIGH);
  digitalWrite(Rele1,HIGH);
  digitalWrite(Rele2,HIGH);
}

void loop() {

ArduinoOTA.handle();
client.loop();
unsigned long now = millis(); 

  if (now - Loop1 > 1000) {
    if (!client.loop()) MQTTConect();
    ++Cuenta;
    if (Cuenta > 30) Cuenta = 0;
    char buffer[4];
    SensTemp.requestTemperatures(); 
    int TempC = (int) SensTemp.getTempCByIndex(0);
    if (TempC != TempC_old){
      TempC_old = TempC;
      if (TempC < 100) sprintf(buffer, "%d", TempC);
      client.publish("Casandra/Caldera/Temperatura", buffer);
    }
    if (Cuenta == 5) {
      sprintf(buffer, "%d", TempC_old);
      client.publish("Casandra/Caldera/Temperatura", buffer);
    }

    float LuzTemporal; 
    LuzIntens = analogRead(analogInPin);
    if (LuzIntens < 100) LuzTemporal = (LuzIntens * 0.333);
    if (LuzIntens >= 100) LuzTemporal = ((LuzIntens * 0.026) + 31);
    if (LuzIntens >= 700) LuzTemporal = ((LuzIntens * 0.157) - 60);
    if (((int) LuzTemporal) > 100) LuzTemporal =  100;
    sprintf(buffer, "%d", (int) LuzTemporal);  // Numero procesado
//    sprintf(buffer, "%d", (int) LuzIntens);  // Numero puro
    if (Cuenta == 20) client.publish("Casandra/Caldera/LuzSolar", buffer);

    if (Cuenta == 12) {
    if (digitalRead(Lluvia) != Lluvia_old){ //Son distintos, guardamos el nuevo en el viejo
      Lluvia_old = digitalRead(Lluvia);
      if (Lluvia_old) client.publish("Casandra/Caldera/Lluvia", "0");
      else client.publish("Casandra/Caldera/Lluvia", "1");
    }
    }
    Loop1 = now;
  } // Loop cada 10 segundos

if (now - Loop2 > 5000) {
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/Caldera/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Caldera/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/Caldera/Mosquito","Reconect");
     }
    }
    Loop2 = now;
}

//Bloque de loop sin espera

}
