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

bool debu = false;
const char* MosqID = "Mosquito-CUARTOS";
const char* mqtt_server = "192.168.0.100";
String ClienteID = "Mosquito-CUARTOS";
String clientId;
const char* Topico = "Casandra/Cuartos/#"; // Solo subscripto al topico de cuartos con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int h_old = 0;
int t_old = 0;
int EstadoPIR1 = 12;  // Digital pin D6
int EstadoPIR2 = 13;  // Digital pin D7
bool EstadoPIR1_old = false;
bool EstadoPIR2_old = false;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool ResetModulo1;          // True cuando se resetea el modulo PIC
bool ResetModulo2;          // True cuando se resetea el modulo PIC
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
if (!ReconectMosq){
  Serial.write(250);
  Serial.write(x);
  Serial.write(y);
  Serial.write(x + y);
  delay(25);
}
}

void reconnect() {
  ReconectMosq = true;
  int Reseteo = 0;
  // Loopea hasta reconectar
  while (!client.connected()) {
    if (debu) Serial.print("Intentando conectar a broker MQTT..");
    // Create a random client ID
    clientId = ClienteID + String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "mqttuser", "MQTTpass")) {
      if (debu) Serial.println("¡Conectado! :D");
      client.subscribe(Topico);
    } else {
      if (debu) Serial.print("Falla de conexion, rc=");
      if (debu) Serial.print(client.state());
      if (debu) Serial.println(" intentando de nuevo en 5 segundos");
      ++Reseteo;
      if (Reseteo == 60) {
        if (debu) Serial.println("=== Reset fisico por no poder reconectar ===");
        ESP.reset();
      }
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(2400);
  ResetMosq = true; // Estamos iniciando desde un reset
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
//  WiFi.setPhyMode(WIFI_PHY_MODE_11B); // Mas confiable, mas consumo, mas lento
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
//  WiFi.setPhyMode(WIFI_PHY_MODE_11N); // Menos confiable, menos consumo, mas rapido
  pinMode(EstadoPIR1, INPUT);
  pinMode(EstadoPIR2, INPUT);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  //wm.resetSettings();
  dht.begin();
  delay(10);

  bool res;
  res = wm.autoConnect(MosqID); // password protected ap
  if (!res) {
    if (debu) Serial.println("Error al conectar WiFi");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    if (debu) Serial.println("Conectado! ");
  }

  ArduinoOTA.setHostname(MosqID);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    if (debu) Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    if (debu) Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (debu) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (debu) Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      if (debu) Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      if (debu) Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      if (debu) Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      if (debu) Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      if (debu) Serial.println("End Failed");
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

  while (Serial.available() > 0) {
    buff[ii] = Serial.read();
    if (buff[ii] == 250) ii = 0;
    else ++ii;
    if (ii == 3) {
      if (( (buff[0]) + (buff[1]) ) == buff[2]) { // Prueba de checksum
        if (buff[0] == 184) {
          if (buff[1] == 49) {
            ResetModulo1 = true;
            //client.publish("Casandra/Cuartos/Modulo/1","off");
          }
          if (buff[1] == 50) {
            ResetModulo2 = true;
            //client.publish("Casandra/Cuartos/Modulo/2","off");
          }
        } 
      } // Prueba de Checksum
      ii = 0;
    } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
    reconnect();
  }
  client.loop();

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
        if (buff[0] == 184) {
          if (buff[1] == 49) ResetModulo1 = true;
          if (buff[1] == 50) ResetModulo2 = true;
        } else client.publish(Topicc, Argu);
      } // Prueba de Checksum
      ii = 0;
    } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  unsigned long now = millis(); // ciclado cada 1 segundo
  if (now - lastMsg > 1000) {
    ++Trein;
    if (Trein >= 31) Trein = 0;

    if (Trein == 15) {
      if (ResetModulo1) {
        ResetModulo1 = false;
        client.publish("Casandra/Cuartos/Modulo/1","off");
      }
      else client.publish("Casandra/Cuartos/Modulo/1","on");
    }
    if (Trein == 16) {
     if (ResetModulo2) {
      ResetModulo2 = false;
      client.publish("Casandra/Cuartos/Modulo/2","off");
     }
      else client.publish("Casandra/Cuartos/Modulo/2","on");
    }

    if (Trein == 5) {
     if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Cuartos/Mosquito","Reset");
     }
    }

    if (Trein == 10) {
     if (ReconectMosq) {
      ReconectMosq = false;
      client.publish("Casandra/Cuartos/Mosquito","Reconect");
     }
    }

    if (Trein == 27) {
     if (!(ReconectMosq) && !(ResetMosq)){
      client.publish("Casandra/Cuartos/Mosquito","Ejecutando");
     }
    }

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
    if (Trein == 5) {
      sprintf(buffer, "%d", h_old);
      client.publish("Casandra/Cuartos/Humedad/01", buffer);
    }
    if (Trein == 20) {
      sprintf(buffer, "%d", t_old);
      client.publish("Casandra/Cuartos/Temperatura/01", buffer);
    }
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
