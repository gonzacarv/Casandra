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
#define DHT1PIN D2  // D2
#define DHTTYPE DHT22
#define MSG_BUFFER_SIZE  (50)

bool debu = false;
const char* MosqID = "Mosquito-GALERIA";
const char* mqtt_server = "192.168.0.100";
String ClienteID = "Mosquito-GALERIA";
String clientId;
const char* Topico = "Casandra/Galeria/#"; // Solo subscripto al topico de Galeria con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int h1_old = 0;
int t1_old = 0;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
int CuentaErrorMQTT = 0;
bool ResetModulo1;          // True cuando se resetea el modulo PIC

unsigned long lastMsg = 0;
int Trein = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
DHT dht1(DHT1PIN, DHTTYPE);
//DHT dht2(DHT2PIN, DHTTYPE);
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
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0)) Hablador(atoi(UlTopic) + 32 + 48, 80);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1)) Hablador(atoi(UlTopic) + 32 + 48, 90);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 2)) Hablador(atoi(UlTopic) + 32 + 48, 120);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 3)) Hablador(atoi(UlTopic) + 32 + 48, 121);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 4)) Hablador(atoi(UlTopic) + 32 + 48, 122);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 5)) Hablador(atoi(UlTopic) + 32 + 48, 123);
  //if (!strcmp(PenUlTopic, "LuzIntensidad")){
  //  float dimer;
  //  dimer = ( (atoi(Pload) * (-0.27) ) + 27);
  //  Hablador(atoi(UlTopic)+32+48, int(dimer));
  //}
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

/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y) { // La funcion encargada de hablar en el bus
  Serial.write(250);
  Serial.write(x);
  Serial.write(y);
  Serial.write(x + y);
  delay(25);
}


void setup() {
  Serial.begin(2400);
  dht1.begin();
  delay(10);
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
  //Hablador(100, 80); // Iniciamos apagando todo
  Hablador(86, 80); // Apagamos incontrolable "Guirnalda galeria"
  delay(10);
  Hablador(88, 80); // Apagamos incontrolable "LED Cobertizo"
}

void loop() {
  ArduinoOTA.handle();
  client.loop();
  if (!client.connected()) {

  while (Serial.available() > 0) {
    buff[ii] = Serial.read();
    if (buff[ii] == 250) ii = 0;
    else ++ii;
    if (ii == 3) {
      if (( (buff[0]) + (buff[1]) ) == buff[2]) { // Prueba de checksum
        if (buff[0] == 184) {
          if (buff[1] == 53) {
            ResetModulo1 = true;
            client.unsubscribe(Topico);
            //client.publish("Casandra/Galeria/Modulo/1","off");
          }
          if (buff[1] == 54) {
            //ResetModulo2 = true;
            //client.publish("Casandra/Galeria/Modulo/2","off");
          }
        } 
      } // Prueba de Checksum
      ii = 0;
    } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
    MQTTConect();
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
        if ((buff[0]) < (10 + 32 + 48)) {
          snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Galeria/LuzEstado/0%d", (buff[0] - (32 + 48)));
          snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
        }
        if ((buff[0]) >= (10 + 32 + 48)) {
          snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Galeria/LuzEstado/%d", (buff[0] - (32 + 48)));
          if ((buff[0] - (32 + 48)) == 11) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Galeria/SensorMov/01");
          if ((buff[0] - (32 + 48)) == 12) snprintf (Topicc, MSG_BUFFER_SIZE, "Casandra/Galeria/SensorMov/02");
          snprintf (Argu, MSG_BUFFER_SIZE, "%d", Payl);
        }
        if (buff[0] == 184) {
          if (buff[1] == 53) client.publish("Casandra/Galeria/Modulo/1","off");
        } else client.publish(Topicc, Argu);
      } // Prueba de Checksum
      ii = 0;
    } // Cuando el contador llega a 3
  } // Cuando llego algo al buffer
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 1000) {
    if (!client.loop()) MQTTConect();
    ++Trein;
    if (Trein >= 31) Trein = 0;
    if (Trein == 15) {
      if (ResetModulo1) {
        ResetModulo1 = false;
        client.subscribe(Topico);
        client.publish("Casandra/Galeria/Modulo/1","off");
      } else client.publish("Casandra/Galeria/Modulo/1","on");
    }    

    if (Trein == 5) {
     if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Galeria/Mosquito","Reset");
     }
    }

    if (Trein == 10) {
     if (ReconectMosq) {
      ReconectMosq = false;
      client.publish("Casandra/Galeria/Mosquito","Reconect");
     }
    }

    if (Trein == 27) {
     if (!(ReconectMosq) && !(ResetMosq)){
      client.publish("Casandra/Galeria/Mosquito","Ejecutando");
     }
    }


    lastMsg = now;
    char buffer[4];
    int h1 = (int) dht1.readHumidity();   // Leemos la humedad
    if ((h1 != h1_old) && (h1 < 100) && (h1 > 0)) {
      h1_old = h1;
      sprintf(buffer, "%d", h1);
      client.publish("Casandra/Galeria/Humedad/01", buffer);
    }
    int t1 = (int) dht1.readTemperature(); // Leemos la temperatura
    if ((t1 != t1_old) && (t1 < 50) && (t1 > 0)) {
      t1_old = t1;
      sprintf(buffer, "%d", t1);
      client.publish("Casandra/Galeria/Temperatura/01", buffer);
    }
    //publicamos ambos datos
    if (Trein == 5) {
      sprintf(buffer, "%d", h1_old);
      client.publish("Casandra/Galeria/Humedad/01", buffer);
    }
    if (Trein == 20) {
      sprintf(buffer, "%d", t1_old);
      client.publish("Casandra/Galeria/Temperatura/01", buffer);
    }

  } // Loop cada 30 segundos

  // Bloque de loop sin espera

}
