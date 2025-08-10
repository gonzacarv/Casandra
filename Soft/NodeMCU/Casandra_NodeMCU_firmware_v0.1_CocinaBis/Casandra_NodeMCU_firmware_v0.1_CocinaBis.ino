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

#define LuzEntrada 5          // Rele de la luz de entrada D1 --
#define LEDMesada 4           // Salida PWM para LED D2 --
#define TeclaLED1 14          // SW LED1 D5 
#define TeclaLED2 12          // SW LED2 D6
#define TeclaPorton 13        // Pulsador porton D7
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-COCINABIS";
const char* mqtt_server = "192.168.0.100"; 
String ClienteID = "Mosquito-COCINABIS";
String clientId;
const char* Topico = "Casandra/CocinaBis/#"; // Solo subscripto al topico de Caldera con comodin aguas abajo

bool EstadoTeclaPorton;
bool EstadoTeclaLED1;
bool EstadoTeclaLED2;
bool LuzEntradaEstado;
bool LEDMesadaEstado;
bool LEDMesadaEstado_old;
float LEDMesadaIntensidad; 
int LEDMesadaIntReal; 
int LEDMode;
int VelociLED;
float Respiracion = 0;
int DimReal;
bool Flashh;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
float dimer; // nivel del dimer
unsigned long Loop1 = 0;
unsigned long Loop2 = 0;
unsigned long Loop3 = 0;
unsigned long LoopNormal = 0;
unsigned long LoopRespira = 0;
unsigned long LoopFlash = 0;
unsigned long now; 
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];

char buffer[4];
WiFiClient espClient;
PubSubClient client(espClient);

void LEDSet(void){

  switch(LEDMode){
    case 1: // Modo normal
    if (LEDMesadaEstado) {
      if ((now - LoopNormal) > 4) { // 
      LoopNormal = now;
      if (LEDMesadaIntReal < DimReal) --DimReal;
      if (LEDMesadaIntReal > DimReal) ++DimReal;
      analogWrite(LEDMesada, (int) (255 - DimReal));
      }
    } else analogWrite(LEDMesada, 255);
      //
      break;

    case 2: // Modo Respira

    if (LEDMesadaEstado) {
      if ((now - LoopRespira) > (VelociLED*2)) { // 
        
        LoopRespira = now;
        ++Respiracion;
        if (Respiracion >= 160) Respiracion = 0;

        if (Respiracion < 60){
          int var = (255 * dimer) * (Respiracion / 60);
          analogWrite(LEDMesada, var);
        }
        if ((Respiracion >= 60) && (Respiracion < 100)){
          analogWrite(LEDMesada, 255);
      }
        
        if ((Respiracion >= 100) && (Respiracion < 160)){
          int var = (255 * dimer) * (1 - ((Respiracion - 100) / 60));
          analogWrite(LEDMesada, var);
      }

      }
    } else analogWrite(LEDMesada, 255);
      
      //
      break;
    
    case 3: // Modo Flash

    if (LEDMesadaEstado) {
      if (now - LoopFlash > (VelociLED*10)) { // 
        LoopFlash = now;
        if (Flashh) {
          analogWrite(LEDMesada, (int) (255 - LEDMesadaIntensidad));
          Flashh = false;
        } else {
            analogWrite(LEDMesada, 255);
            Flashh = true;
          }
      }
    } else analogWrite(LEDMesada, 255);
      
      //
      break;

    default: // Modo normal si nada coincide
      LEDMode = 1;      
      //
      break;
  } //switch

}

void LEDToggle(void){
    if (LEDMesadaEstado) {
      LEDMesadaEstado = false; 
      client.publish("Casandra/CocinaBis/LEDMesadaEst","0");
      } else {
      LEDMesadaEstado = true;
      client.publish("Casandra/CocinaBis/LEDMesadaEst","1");
      }
    }

void callback(char* topic, byte* payload, unsigned int length) {
  const char s[2] = "/";
  char *UlTopic;
  char *PenUlTopic;
  char *Fin;
  char Pload[10];

  memcpy(Pload, payload, length);
  Pload[length] = '\0';

  Fin = strtok(topic, s);
  while (Fin != NULL) {
    PenUlTopic = UlTopic;
    UlTopic = Fin;
    Fin = strtok(NULL, s);
  }

  if (!strcmp(UlTopic, "LuzEntrada")) {
    if (atoi(Pload) == 0) {
      LuzEntradaEstado = false;
      digitalWrite(LuzEntrada,LOW);
    } else {
      LuzEntradaEstado = true;
      digitalWrite(LuzEntrada,HIGH);
     }
  }
  
  if (!strcmp(UlTopic, "LEDMesadaEst")) {
    if (atoi(Pload) == 0) {
      LEDMesadaEstado = false;
    } else {
     LEDMesadaEstado = true;
  }
  }
  
  if (!strcmp(UlTopic, "LEDMesadaInt")) {
    dimer = (atof(Pload) / 100); 
    LEDMesadaIntensidad = ((255 * atof(Pload)) / 100);
    LEDMesadaIntReal = LEDMesadaIntensidad;
  }
  
  if (!strcmp(UlTopic, "ModoLED")) {
    if (!strcmp(Pload, "Normal")) LEDMode = 1;
    }

  if (!strcmp(UlTopic, "ModoLED")) {
    if (!strcmp(Pload, "Respira")) LEDMode = 2;
    }

  if (!strcmp(UlTopic, "ModoLED")) {
    if (!strcmp(Pload, "Flash")) LEDMode = 3;
    }

  if (!strcmp(UlTopic, "VelocidadLED")) VelociLED = (100 - atoi(Pload));
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
      }
      delay(1000);
    }
  }

void setup() {
  Serial.begin(9600);
  pinMode(LuzEntrada, OUTPUT);
  pinMode(LEDMesada, OUTPUT);
  pinMode(TeclaLED1, INPUT);
  pinMode(TeclaLED1, INPUT);
  pinMode(TeclaPorton, INPUT);
  WiFiManager wifiManager;
  ResetMosq = true; // Estamos iniciando desde un reset
  Ejecutando = false;

  // Descomentar para resetear configuración
  //wifiManager.resetSettings();

  wifiManager.setConfigPortalTimeout(180);
  if(!wifiManager.autoConnect(MosqID)){
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
  LuzEntradaEstado = false;

  LEDMesadaEstado = false;
  LEDMesadaIntensidad = 255; 
  LEDMode = 1;
  VelociLED = 50;
}

void loop() {

ArduinoOTA.handle();
client.loop();
now = millis(); 

  if (now - Loop1 > 5000) { // 5 segundos
    Loop1 = now;
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/CocinaBis/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/CocinaBis/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/CocinaBis/Mosquito","Reconect");
     }
    }
  }

  if (now - Loop2 > 1000) {
    Loop2 = now;
    if (!client.loop()) MQTTConect();
  }

  if (now - Loop3 > 50) {
    Loop3 = now;
    if (digitalRead(TeclaPorton)!= EstadoTeclaPorton) {
      if (digitalRead(TeclaPorton)) {
        client.publish("Casandra/CocinaBis/SWPorton","1");
        Serial.println("........Prendimos Sw Porton");
      } else {
        client.publish("Casandra/CocinaBis/SWPorton","0");
        Serial.println("........Apagamos Sw Porton");
      }
      EstadoTeclaPorton = digitalRead(TeclaPorton);
    }

    if (digitalRead(TeclaLED1)!= EstadoTeclaLED1) {
      LEDToggle();
      EstadoTeclaLED1 = digitalRead(TeclaLED1);
    }

    if (digitalRead(TeclaLED2)!= EstadoTeclaLED2) {
      LEDToggle();
      EstadoTeclaLED2 = digitalRead(TeclaLED2);
    }

   if (LEDMesadaEstado != LEDMesadaEstado_old){
    if (LEDMode == 1) {
      if (LEDMesadaEstado) {
        LEDMesadaIntReal = LEDMesadaIntensidad;
      } else {
          LEDMesadaIntReal = 255;
        }
      LEDMesadaEstado_old = LEDMesadaEstado;
    }
}
}



LEDSet();
//Bloque de loop sin espera

}
