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

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <string>

#define Bomba 14 // Salida relé de la bomba   D5
//#define DHTPIN D3   // DHT22
#define MSG_BUFFER_SIZE  (50)

const char* MosqID = "Mosquito-HUERTA";
const char* mqtt_server = "192.168.0.100";
const int analogInPin = A0;
String clientId;
String ClienteID = "Mosquito-HUERTA";
const char* Topico = "Casandra/Huerta/#"; // Solo subscripto al topico de Galeria con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int MQTTOn;
int MQTTOff;
int SegOn;
int AutoSegOn;
int SegOff;
int MinOff;
int AguaIntens = 0; 
bool Estado;
bool EstadoPpal;
bool HayAgua = false;
int EstadoNum;
int MaxBbaOn = 60; // Maxima cantidad de segundos que puede andar la bomba
unsigned long CadaMil = 0;
unsigned long CadaDosc = 0;
unsigned long CadaCinco = 0;
bool ResetMosq = true;      // True cuando se reinicia el mosquito
bool ReconectMosq = true;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
int CuentaErrorMQTT = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
char buffer[4];
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
  //if (!strcmp(UlTopic, "On")) { Serial.println("    ..Acaba de llegar un ON  = "); Serial.print(MQTTOn);}
  if ( (!strcmp(PenUlTopic, "Huerta")) && (!strcmp(UlTopic, "Off")) ) MQTTOff = atof(Pload);
  //if (!strcmp(UlTopic, "Off")){ Serial.println("    ..Acaba de llegar un OFF  = "); Serial.print(MQTTOff);}
  //Serial.println("    ..La suma de ambas es  = "); Serial.print(MQTTOn+MQTTOff);

if (!strcmp(UlTopic, "On")){
if (MQTTOn > 0) {
  if (MQTTOn > 1000) ;
  else { 
  if (MQTTOn > MaxBbaOn) {
    MQTTOn = MaxBbaOn;
    SegOn = MQTTOn;
  }
  else SegOn = MQTTOn;
  Estado = true;
  Serial.println("       ...Llego un MQTT On positivo, lo pasamos a SegOn que valdrá: ");
  Serial.print(SegOn);
  }
} else { 
  MQTTOn = 0;
  Estado = false;
//  Serial.println("       ...Apagada por 0 on ");
}
}

if (!strcmp(UlTopic, "Off")){
if (MQTTOff > 0) {
  SegOff = MQTTOff;
  MinOff = ((MQTTOff)*60);
  Serial.println("       ...Llego un MQTT Off positivo, lo pasamos a SegOff que valdrá: ");
  Serial.print(MinOff);
} else {
  MQTTOff = 0;
//  Serial.println("       ...Siempre prendida por 0 off ");
}
}

  if ( (!strcmp(PenUlTopic, "Huerta")) && (!strcmp(UlTopic, "BbaEstado")) ) {
    EstadoNum = atof(Pload);
    if (EstadoNum > 0) {
      EstadoPpal = true;
      //Serial.println("Prendida por Sw");
      }
  else {
    EstadoPpal = false;
    //Serial.println("Apagada por Sw");
    }
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
  pinMode(Bomba, OUTPUT);
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
  EstadoPpal = true;
  AutoSegOn = 0;
} //setup()

void loop() {

ArduinoOTA.handle();
client.loop();
unsigned long now = millis(); 

if (now - CadaMil > 1000) {  // ciclado cada 1 segundos

    if (!client.loop()) MQTTConect();

    if (Estado){ /////// Estado es verdadero: On

      if ((SegOn) > 999) {
        ++AutoSegOn;
        Serial.println("       ...Valor mayor o igual a mil... AutoSegon vale: ");
        Serial.print(AutoSegOn);
      } else {
      if ((SegOn-1) > 0) {
        --SegOn;
      } else {
        if (MQTTOff > 0) Estado = false;
        SegOn = MQTTOn;
      }
      } // else de segon menor a 1000


    } ///////////////////// Estado es verdadero

    if (!Estado){ // Estado es Falso: Off
      if (MinOff > 0) {
        --MinOff;
      } else {
        if (MQTTOn > 0) Estado = true;
        MinOff = ((MQTTOff)*60);
      }
    }

  if (EstadoPpal){
    if (Estado) {
      digitalWrite(Bomba,LOW);
      client.publish("Casandra/Huerta/Bomba","on");
//      Serial.println("    ..Escrita salida LOW");
      }
    else {
      digitalWrite(Bomba,HIGH);
      client.publish("Casandra/Huerta/Bomba","off");
//      Serial.println("    ..Escrita salida HIGH");
      }
  } else {
    digitalWrite(Bomba,HIGH);
    client.publish("Casandra/Huerta/Bomba","off");
  }
  CadaMil = now;
 } // Loop cada 1 segundo

if (now - CadaDosc > 200) {
float AguaTemporal; 
AguaIntens = analogRead(analogInPin);
sprintf(buffer, "%d", (int) AguaTemporal);
if (AguaTemporal !=  AguaIntens) {
  AguaTemporal =  AguaIntens;
  if (AguaTemporal < 500) HayAgua = false;
  else HayAgua = true;
}
  CadaDosc = now;
}

if (now - CadaCinco > 5000) {
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/Huerta/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Huerta/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/Huerta/Mosquito","Reconect");
     }
    }
    CadaCinco = now;
}

  // Bloque de loop sin espera

if (SegOn > 999) {
  if (Estado) {
    if ((!HayAgua)|| (AutoSegOn > MaxBbaOn)){
      Estado = false;
      sprintf(buffer, "%d", (int) (AutoSegOn + 1000));
      client.publish("Casandra/Huerta/On", buffer);
      Serial.println("       ...Se acabo el agua y se paso el valor 1000 mas segon");
      Serial.print(AutoSegOn);
      AutoSegOn = 0;
    }
  }
}
  
  }
