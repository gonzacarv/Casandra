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

#define Rele1 5          // Rele1  D1
#define Sirena 16        // Sirena D0
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-HALL";
const char* mqtt_server = "192.168.0.100";
String clientId;
String ClienteID = "Mosquito-HALL";
const char* Topico = "Casandra/Hall/#"; // Solo subscripto al topico de Caldera con comodin aguas abajo
unsigned long Loop1 = 0;
unsigned long Loop2 = 0;
unsigned long Loop3 = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
int EstadoSirena = 0;
int Cuenta = 0;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
int CuentaErrorMQTT = 0;

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

  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sodios") && (atoi(Pload) == 1)) {
    digitalWrite(Rele1, HIGH);
    //Serial.print("   Sodios Prendidos   ");
  }
  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sodios") && (atoi(Pload) == 0)) {
    digitalWrite(Rele1, LOW);
    //Serial.print("   Sodios Apagados   ");
  }

  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sirena")) EstadoSirena = atoi(Pload);
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
  pinMode(Rele1, OUTPUT);
  pinMode(Sirena, OUTPUT);
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
  digitalWrite(Rele1, LOW);
  digitalWrite(Sirena, LOW);
  EstadoSirena = 0;
}


void loop() {

ArduinoOTA.handle();
client.loop();
unsigned long now = millis(); 

if (now - Loop1 > 5000) {
    if (!client.loop()) MQTTConect();
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/Hall/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Hall/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/Hall/Mosquito","Reconect");
     }
    }
    Loop1 = now;
}

if (now - Loop2 > 500) { // Cada un segundo
  Cuenta = Cuenta + 1;
  if (Cuenta == 4) Cuenta = 0;

    switch (EstadoSirena) {
      case 0:
        digitalWrite(Sirena, LOW);
        break;

      case 1:
        digitalWrite(Sirena, HIGH);
        break;

      case 2:
        if (Cuenta == 0) digitalWrite(Sirena, HIGH);
        else digitalWrite(Sirena, LOW);
        break;

      case 3:
        if ((Cuenta == 0) || (Cuenta == 2)) digitalWrite(Sirena, HIGH);
        else digitalWrite(Sirena, LOW);
        break;

      case 4:
        if ((Cuenta == 0) || (Cuenta == 1)) digitalWrite(Sirena, HIGH);
        else digitalWrite(Sirena, LOW);
        break;

      case 5:
        if (Cuenta == 0) digitalWrite(Sirena, LOW);
        else digitalWrite(Sirena, HIGH);
        break;

      default:
        digitalWrite(Sirena, LOW);
        break;
    }
    Loop2 = now;
  } // Loop cada 500 milisegundos

  //Bloque de loop sin espera

}
