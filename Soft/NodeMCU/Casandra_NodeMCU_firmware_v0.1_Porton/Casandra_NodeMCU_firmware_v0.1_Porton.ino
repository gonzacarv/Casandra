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

#define SwAbrir    05   // Switch abrir porton D1
#define SwCerrar   04   // Switch cerrar porton D2
#define LuzCamino  00   // Luz camino D3
#define Timbre     14   // Timbre D5
#define SensPuerta 12   // Sensor Puerta D6
#define SensPorton 13   // Sensor Porton D7
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-PORTON";
const char* mqtt_server = "192.168.0.100";
String clientId;
String ClienteID = "Mosquito-PORTON";
const char* Topico = "Casandra/Porton/#"; // Solo subscripto al topico del Porton con comodin aguas abajo
//int EstadoPuerta = 12;  // Digital pin D6
//int EstadoPorton = 13;  // Digital pin D7
//int EstadoTimbre = 14;  // Digital pin D5
bool EstadoPuerta_old = false;
bool EstadoPorton_old = false;
bool EstadoTimbre_old = false;
unsigned long Loop1 = 0;
unsigned long Loop2 = 0;
unsigned long Loop3 = 0;
int Tim = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
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

  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "BtnAbrir")) ) {
    digitalWrite(SwAbrir, HIGH);
    delay(500);
    digitalWrite(SwAbrir, LOW);
  }

  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "BtnCerrar")) ) {
    digitalWrite(SwCerrar, HIGH);
    delay(500);
    digitalWrite(SwCerrar, LOW);
  }

  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "LuzCamino")) && (atoi(Pload) == 1) ) {
    digitalWrite(LuzCamino, HIGH);
  }
  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "LuzCamino")) && (atoi(Pload) == 0) ) {
    digitalWrite(LuzCamino, LOW);
  }
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
  pinMode(Timbre, INPUT);
  pinMode(SensPuerta, INPUT);
  pinMode(SensPorton, INPUT);
  pinMode(SwAbrir, OUTPUT);
  pinMode(SwCerrar, OUTPUT);
  pinMode(LuzCamino, OUTPUT);
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
  digitalWrite(SwAbrir, LOW);
  digitalWrite(SwCerrar, LOW);
  digitalWrite(LuzCamino, LOW);
}

void loop() {

ArduinoOTA.handle();
client.loop();
unsigned long now = millis(); 
  
  if (now - Loop1 > 100) {

    if (Tim == 0) {
      if (digitalRead(Timbre) != EstadoTimbre_old) { //Son distintos, guardamos el nuevo en el viejo
        Tim = 25; // Esperamos 2 segundos y medio
        EstadoTimbre_old = digitalRead(Timbre);
        if (EstadoTimbre_old) client.publish("Casandra/Porton/Timbre", "1");
        else client.publish("Casandra/Porton/Timbre", "0");
      }
    } else Tim = Tim - 1;

    if (digitalRead(SensPuerta) != EstadoPuerta_old) { //Son distintos, guardamos el nuevo en el viejo
      EstadoPuerta_old = digitalRead(SensPuerta);
      if (EstadoPuerta_old) client.publish("Casandra/Porton/SensorPuerta", "0");
      else client.publish("Casandra/Porton/SensorPuerta", "1");
    }

    if (digitalRead(SensPorton) != EstadoPorton_old) { //Son distintos, guardamos el nuevo en el viejo
      EstadoPorton_old = digitalRead(SensPorton);
      if (EstadoPorton_old) client.publish("Casandra/Porton/SensorPorton", "0");
      else client.publish("Casandra/Porton/SensorPorton", "1");
    }
    Loop1 = now;
  } // Loop cada 100 milisegundos

if (now - Loop2 > 5000) {
    if (!client.loop()) MQTTConect();
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/Porton/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Porton/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/Porton/Mosquito","Reconect");
     }
    }
    Loop2 = now;
}

  //Bloque de loop sin espera

}
