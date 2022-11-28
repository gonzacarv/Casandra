/*========================================================================================================
  ||   Proyecto: Casandra v3.0                                                                            ||
  ||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                      ||
  ||   Fecha: 11/2022                                                                                     ||
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

#define Bomba D5       // Salida relé de la bomba   D5
//#define DHTPIN D3   // DHT22
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-HUERTA";
const char* mqtt_server = "192.168.0.58";
String clientId = "Mosquito-HUERTA";
const char* Topico = "Casandra/Huerta/#"; // Solo subscripto al topico de Galeria con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int MQTTOn;
int MQTTOff;
int SegOn;
int SegOff;
bool Estado;
unsigned long lastMsg = 0;
int Trein = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) { // Funcion de lectura de lo que va llegando al topico que escuchamos
  const char s[2] = "/";
  char *UlTopic;
  char *PenUlTopic;
  char *Fin;
  char Pload[15];

  memcpy(Pload, payload, length);
  Pload[length] = '\0';

  Fin = strtok(topic, s);
  while (Fin != NULL) {
    PenUlTopic = UlTopic;
    UlTopic = Fin;
    Fin = strtok(NULL, s);
  }

  if ( (!strcmp(PenUlTopic, "Huerta")) && (!strcmp(UlTopic, "On")) ) MQTTOn = atof(Pload);
  if ( (!strcmp(PenUlTopic, "Huerta")) && (!strcmp(UlTopic, "Off")) ) MQTTOff = atof(Pload);
  SegOn = MQTTOn;
  SegOff = MQTTOff;
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
      if (Reseteo == 10) ESP.reset();
      delay(5000);
    }
  }
}

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(9600);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
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

if (MQTTOn > 0) {
  SegOn = MQTTOn;
  Estado = true;
} else {
  MQTTOn = 0;
  Estado = false;
}

if (MQTTOff > 0) {
  SegOff = MQTTOff;
} else {
  MQTTOff = 0;
}

}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 1000) {

    if (Estado){ // Estado es verdadero: On
      if (SegOn > 0) {
        --SegOn;
      } else {
        if (MQTTOff > 0) Estado = false;
        SegOn = MQTTOn;
      }
      
    } else { // Estado es falso: Off
      if (SegOff > 0) {
        --SegOff;
      } else {
        if (MQTTOn > 0) Estado = true;
        SegOff = MQTTOff;
      }
    }

    lastMsg = now;
    if (Estado) digitalWrite(Bomba,LOW);//Encendemos la bajoactiva
    else digitalWrite(Bomba,HIGH);//Apagamos la bajoactiva



  } // Loop cada 30 segundos

  // Bloque de loop sin espera

}
