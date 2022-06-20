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
#define DHTPIN D4
#define DHTTYPE DHT22
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-CUARTOS";
const char* mqtt_server = "192.168.0.58";
String clientId = "Mosquito-CUARTOS";
const char* Topico = "Casandra/Cuartos/#"; // Solo subscripto al topico de cuartos con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int h_old = 0;
int t_old = 0;
int EstadoPIR1 = 12;  // Digital pin D6
int EstadoPIR2 = 13;  // Digital pin D7
bool EstadoPIR1_old = false;
bool EstadoPIR2_old = false;
unsigned long lastMsg = 0;
int Trein = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
DHT dht(DHTPIN, DHTTYPE);
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
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0)) Hablador(atoi(UlTopic) + 32, 80);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1)) Hablador(atoi(UlTopic) + 32, 90);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 2)) Hablador(atoi(UlTopic) + 32, 120);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 3)) Hablador(atoi(UlTopic) + 32, 121);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 4)) Hablador(atoi(UlTopic) + 32, 122);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 5)) Hablador(atoi(UlTopic) + 32, 123);

  if (!strcmp(PenUlTopic, "LuzIntensidad")) {
    float dimer;
    dimer = ( (atoi(Pload) * (-0.27) ) + 27);
    Hablador(atoi(UlTopic) + 32, int(dimer));
  }
}

/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y) { // La funcion encargada de hablar en el bus
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
    if (client.connect(clientId.c_str(), "mqttuser", "MQTTpass")) {
      Serial.println("¡Conectado! :D");
      client.subscribe(Topico);
    } else {
      Serial.print("Falla de conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      ++Reseteo;
      if (Reseteo == 60) ESP.reset();
      delay(5000);
    }
  }
}

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  pinMode(EstadoPIR1, INPUT);
  pinMode(EstadoPIR2, INPUT);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  Serial.begin(2400);
  //wm.resetSettings();
  dht.begin();
  delay(10);

  bool res;
  res = wm.autoConnect(MosqID); // password protected ap
  if (!res) {
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
    if (buff[ii] == 250) ii = 0;
    else ++ii;
    if (ii == 3) {
      if (( (buff[0]) + (buff[1]) ) == buff[2]) { // Prueba de checksum
        int Payl;
        if (buff[1] == 80) Payl = 0;
        if (buff[1] == 90) Payl = 1;
        if ((buff[0]) < (10 + 32)) {
          snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cuartos/LuzEstado/0%d", (buff[0] - 32));
          snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
        }
        if ((buff[0]) >= (10 + 32)) {
          snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Cuartos/LuzEstado/%d", (buff[0] - 32));
          snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
        }
        client.publish(Topicc, Argu);
      } // Prueba de Checksum
      ii = 0;
    } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  unsigned long now = millis(); // ciclado cada 1 segundo
  if (now - lastMsg > 1000) {
    ++Trein;
    if (Trein >= 31) Trein = 0;
    lastMsg = now;
    char buffer[4];
    int h = (int) dht.readHumidity();   // Leemos la humedad
    if ((h != h_old) && (h < 100) && (h > 0)) {
      h_old = h;
      sprintf(buffer, "%d", h);
      client.publish("Casandra/Cuartos/Humedad/01", buffer);
    }
    int t = (int) dht.readTemperature(); // Leemos la temperatura
    if ((t != t_old) && (t < 50) && (t > 0)) {
      t_old = t;
      sprintf(buffer, "%d", t);
      client.publish("Casandra/Cuartos/Temperatura/01", buffer);
    }
    //publicamos ambos datos
    if (Trein == 5) client.publish("Casandra/Cuartos/Humedad/01", buffer);
    if (Trein == 20) client.publish("Casandra/Cuartos/Temperatura/01", buffer);
  } // Loop cada 1 segundo

  if (digitalRead(EstadoPIR1) != EstadoPIR1_old) { //Son distintos, guardamos el nuevo en el viejo
    EstadoPIR1_old = digitalRead(EstadoPIR1);
    if (EstadoPIR1_old) client.publish("Casandra/Cuartos/SensorMov/01", "1");
    else client.publish("Casandra/Cuartos/SensorMov/01", "0");
  }

  if (digitalRead(EstadoPIR2) != EstadoPIR2_old) { //Son distintos, guardamos el nuevo en el viejo
    EstadoPIR2_old = digitalRead(EstadoPIR2);
    if (EstadoPIR2_old) client.publish("Casandra/Cuartos/SensorMov/02", "1");
    else client.publish("Casandra/Cuartos/SensorMov/02", "0");
  }
}
