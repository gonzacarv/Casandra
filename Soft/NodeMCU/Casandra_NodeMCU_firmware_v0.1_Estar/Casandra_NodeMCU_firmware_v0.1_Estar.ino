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
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>
#include <string>
#include "DHT.h"

#define RR D5       // Red   D5
#define GG D6       // Green D6
#define BB D7       // Blue  D7
#define IRAire D2   // IR del aire
#define IRTele D1   // IR de Tele-Deco
//#define IRInput D1  // Entrada IR
#define DHTPIN D3   // DHT22
#define MSG_BUFFER_SIZE  (50)

IRSamsungAc AA(4);     // Set the GPIO used for sending messages.
IRsend IRTVDeco(IRTele);

const char* MosqID = "Mosquito-ESTAR";
const char* mqtt_server = "192.168.0.58";
String clientId = "Mosquito-ESTAR";
const char* Topico = "Casandra/Estar/#"; // Solo subscripto al topico de Galeria con comodin aguas abajo
int ii = 0; // Contador de bus
int buff[3]; // Lo que llega
int h1_old = 0;
int t1_old = 0;
float Rojo; // variable numerica intermedia float que graba los valores RGB
float Verde;
float Azul;
float dimer; // nivel del dimer
int RojoDim; // variable numerica que graba los valores antes de pegarlos en el puero de salida
int VerdeDim;
int AzulDim;
unsigned long lastMsg = 0;
int Trein = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
DHT dht(DHTPIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);

void RGBSet(void){
    RojoDim = dimer * Rojo;
    VerdeDim = dimer * Verde;
    AzulDim = dimer * Azul;
    analogWrite(RR, RojoDim);
    analogWrite(GG, VerdeDim);
    analogWrite(BB, AzulDim);
}

void RGBOff(void){
    //printf( "//////////// RGB OFF ///////////\n\n");
    analogWrite(RR, 0);
    analogWrite(GG, 0);
    analogWrite(BB, 0);
}

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

  if ( (!strcmp(PenUlTopic, "Estar")) && (!strcmp(UlTopic, "LuzRGB")) ) { // funcion que extrae los colores RGB del la luz
    const char separador[2] = ",";
    char *RGBs;
    RGBs = strtok(Pload, separador);
    int cuentaRGB = 0;
    while( RGBs != NULL) {
      if (cuentaRGB == 0) Rojo  = atof(RGBs);
      if (cuentaRGB == 1) Verde = atof(RGBs);
      if (cuentaRGB == 2) Azul  = atof(RGBs);
      ++cuentaRGB;
      if (cuentaRGB == 3) cuentaRGB = 0;
      RGBs = strtok(NULL, separador);
    }
    //printf( "\n\n//////////// SET solo cuando el topic es LuzRGB ///////////\n");
    RGBSet();
  }

  if (!strcmp(UlTopic, "LuzIntensidad")) {
    dimer = (atof(Pload) / 100);
    //printf( "\n\n//////////// SET solo cuando el topic es LuzIntensidad ///////////\n");
    RGBSet();
  }

  if ((!strcmp(UlTopic, "LuzEstado")) ){ 
    //printf( "\n\n//////////// SET u OFF solo cuando el topic es LuzEstado ///////////\n");
    if (atoi(Pload) == 0) RGBOff();
    if (atoi(Pload) == 1) RGBSet();
  }

    if (!strcmp(PenUlTopic, "CRemoto")){  /////////////////////////// Bloque de control IR TV y deco //////////////
      if (!strcmp(UlTopic, "BtnOnTV"))        IRTVDeco.sendNEC(0x20DF10EF, 32); 
      if (!strcmp(UlTopic, "BtnOnDeco"))      IRTVDeco.sendNEC(0x10110EF, 32);
      if (!strcmp(UlTopic, "BtnCanalUpDeco")) IRTVDeco.sendNEC(0x101926D, 32);
      if (!strcmp(UlTopic, "BtnCanalDoDeco")) IRTVDeco.sendNEC(0x10112ED, 32);
      if (!strcmp(UlTopic, "BtnVolUpDeco"))   IRTVDeco.sendNEC(0x101906F, 32); 
      if (!strcmp(UlTopic, "BtnVolDoDeco"))   IRTVDeco.sendNEC(0x10150AF, 32); 
      if (!strcmp(UlTopic, "DecoBtn0")) IRTVDeco.sendNEC(0x1017887, 32); 
      if (!strcmp(UlTopic, "DecoBtn1")) IRTVDeco.sendNEC(0x101C03F, 32);
      if (!strcmp(UlTopic, "DecoBtn2")) IRTVDeco.sendNEC(0x101C837, 32); 
      if (!strcmp(UlTopic, "DecoBtn3")) IRTVDeco.sendNEC(0x101D827, 32); 
      if (!strcmp(UlTopic, "DecoBtn4")) IRTVDeco.sendNEC(0x10120DF, 32); 
      if (!strcmp(UlTopic, "DecoBtn5")) IRTVDeco.sendNEC(0x10128D7, 32); 
      if (!strcmp(UlTopic, "DecoBtn6")) IRTVDeco.sendNEC(0x10138C7, 32); 
      if (!strcmp(UlTopic, "DecoBtn7")) IRTVDeco.sendNEC(0x101A05F, 32); 
      if (!strcmp(UlTopic, "DecoBtn8")) IRTVDeco.sendNEC(0x101A857, 32); 
      if (!strcmp(UlTopic, "DecoBtn9")) IRTVDeco.sendNEC(0x101B847, 32); // Verdadero boton "9" = 0x101B847
    }
    if (!strcmp(PenUlTopic, "AA")){
      if (!strcmp(UlTopic, "Modo")){ //[“auto”, “off”, “cool”, “heat”, “dry”, “fan_only”]
        if (!strcmp(Pload, "auto")) {
          AA.on();
          AA.setMode(kSamsungAcAuto);
          AA.send();
          Serial.print("Modo es AUTO \n");
        }
        if (!strcmp(Pload, "off")) {
          AA.off();
          AA.send();
          Serial.print("Modo es APAGADO \n");
        }
        if (!strcmp(Pload, "cool")) {
          AA.on();
          AA.setMode(kSamsungAcCool);
          AA.send();
          Serial.print("Modo es FRIO \n");
        }
        if (!strcmp(Pload, "heat")) {
          AA.on();
          AA.setMode(kSamsungAcHeat);
          AA.send();
          Serial.print("Modo es CALOR \n");
        }
        if (!strcmp(Pload, "dry")) {
          AA.on();
          AA.setMode(kSamsungAcDry);
          AA.send();
          Serial.print("Modo es DESHUMIDIFICAR \n");
        }
        if (!strcmp(Pload, "fan_only")) {
          AA.on();
          AA.setMode(kSamsungAcFan);
          AA.send();
          Serial.print("Modo es SOLO VENTILADOR \n");
        }
      }
      if (!strcmp(UlTopic, "Fan")){ //[“auto”, “low”, “medium”, “high”]
        if (!strcmp(Pload, "auto")) {
          AA.setFan(kSamsungAcFanAuto);
          AA.send();
          Serial.print("Fan esta en modo AUTO \n");
        }
        if (!strcmp(Pload, "low")) {
          AA.setFan(kSamsungAcFanLow);
          AA.send();
          Serial.print("Fan esta en modo BAJO \n");
        }
        if (!strcmp(Pload, "medium")) {
          AA.setFan(kSamsungAcFanMed);
          AA.send();
          Serial.print("Fan esta en modo MEDIO \n");
        }
        if (!strcmp(Pload, "high")) {
          AA.setFan(kSamsungAcFanHigh);
          AA.send();
          Serial.print("Fan esta en modo ALTO \n");
        }
      }
      if (!strcmp(UlTopic, "Swing")){ //[“on”, “off”]
        if (!strcmp(Pload, "on")) {
          AA.setSwing(true);
          AA.send();
          Serial.print("Swing PRENDIDO \n");
        }
        if (!strcmp(Pload, "off")) {
          AA.setSwing(false);
          AA.send();
          Serial.print("Swing APAGADO \n");
        }
      }
      if (!strcmp(UlTopic, "TempObj")){ // valor numerico de la temperatura
        AA.setTemp(atoi(Pload));
        AA.send();
        Serial.println("La temperatura obj es: ");
        Serial.println(atoi(Pload));
        Serial.println("\n");
        // decodificamos la temperatura objetivo
      }
    }
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
  dht.begin();
  delay(10);
  AA.begin();
  delay(10);
  IRTVDeco.begin();
  delay(10);
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
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 1000) {
    ++Trein;
    if (Trein >= 31) Trein = 0;
    lastMsg = now;
    char buffer[4];
    int h1 = (int) dht.readHumidity();   // Leemos la humedad
    if ((h1 != h1_old) && (h1 < 100) && (h1 > 0)) {
      h1_old = h1;
      sprintf(buffer, "%d", h1);
      client.publish("Casandra/Estar/Humedad", buffer);
    }
    int t1 = (int) dht.readTemperature(); // Leemos la temperatura
    if ((t1 != t1_old) && (t1 < 50) && (t1 > 0)) {
      t1_old = t1;
      sprintf(buffer, "%d", t1);
      client.publish("Casandra/Estar/Temperatura", buffer);
    }
    //publicamos ambos datos
    if (Trein == 5) {
      sprintf(buffer, "%d", h1_old);
      client.publish("Casandra/Estar/Humedad", buffer);
    }
    if (Trein == 20) {
      sprintf(buffer, "%d", t1_old);
      client.publish("Casandra/Estar/Temperatura", buffer);
    }

  } // Loop cada 30 segundos

  // Bloque de loop sin espera

}
