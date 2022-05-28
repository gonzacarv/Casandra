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
#include <PubSubClient.h>
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
const char* ssid = "CasandraEXT";
const char* password = "crallova";
const char* mqtt_server = "192.168.0.58"; 
String clientId = "Mosquito-CALDERA";
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
unsigned long lastMsg = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(oneWireBus);
DallasTemperature SensTemp(&oneWire);

void setup_wifi() {
  delay(10);
  int Reseteo = 0;
  // Inicia conexion WiFi
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("MosquitoCaldera");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ++Reseteo;
    if (Reseteo == 100)  ESP.reset(); 
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

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
//  pinMode(EstadoPIR1, INPUT);
  pinMode(Lluvia, INPUT);
//  pinMode(EstadoPIR2, INPUT);
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT);
//  pinMode(Rele3, OUTPUT);
  pinMode(Termostato, OUTPUT);
  Serial.begin(2400);
  SensTemp.begin();
  delay(10);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  digitalWrite(Termostato,HIGH);
  digitalWrite(Rele1,HIGH);
  digitalWrite(Rele2,HIGH);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 10000) {
    lastMsg = now;
    char buffer[4];
    SensTemp.requestTemperatures(); 
    int TempC = (int) SensTemp.getTempCByIndex(0);
    if (TempC != TempC_old){
      TempC_old = TempC;
      if (TempC < 100) sprintf(buffer, "%d", TempC);
      client.publish("Casandra/Caldera/Temperatura", buffer);
    }
    float LuzTemporal; 
    LuzIntens = analogRead(analogInPin);
    if (LuzIntens >= 200) LuzTemporal = ((LuzIntens * 0.11) + 28);
    if (LuzIntens < 200) LuzTemporal = (LuzIntens * 0.25);
    if (((int) LuzTemporal) > 100) LuzTemporal =  100;
    sprintf(buffer, "%d", (int) LuzTemporal);
    client.publish("Casandra/Caldera/LuzSolar", buffer);

    if (digitalRead(Lluvia) != Lluvia_old){ //Son distintos, guardamos el nuevo en el viejo
      Lluvia_old = digitalRead(Lluvia);
      if (Lluvia_old) client.publish("Casandra/Caldera/Lluvia", "1");
      else client.publish("Casandra/Caldera/Lluvia", "0");
    }

  } // Loop cada 10 segundos

//Bloque de loop sin espera

}
