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
const char* mqtt_server = "192.168.0.100";
String clientId;
String ClienteID = "Mosquito-ESTAR";
const char* Topico = "Casandra/Estar/#"; // Solo subscripto al topico de Galeria con comodin aguas abajo
int ii = 0;    // Contador de bus
int buff[3];   // Lo que llega
int h1_old = 0;
int t1_old = 0;
float Rojo;    // variable numerica intermedia float que graba los valores RGB
float Verde;
float Azul;
float dimer;   // nivel del dimer
int RojoDim;   // variable numerica que graba los valores antes de pegarlos en el puero de salida
int VerdeDim;
int AzulDim;
int LEDMode;
int VelociLED = 30;
int Dosis = 0;
unsigned long Loop1 = 0;
unsigned long Loop2 = 0;
unsigned long LoopFlash = 0;
unsigned long LoopLluvia = 0;
float CuentaOla = 0;
bool Flashh;
bool ResetMosq;      // True cuando se reinicia el mosquito
bool ReconectMosq;   // True cuando se debe reconectar a MQTT
bool Ejecutando = false;
int CuentaErrorMQTT = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
unsigned long now;
bool RGBEstado;
DHT dht(DHTPIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);

void RGBSet(void){
  if (RGBEstado) {
    switch(LEDMode){
      case 1: // Modo normal
        RojoDim = dimer * Rojo;
        VerdeDim = dimer * Verde;
        AzulDim = dimer * Azul;
        RGBOn(RojoDim, VerdeDim, AzulDim);
        //
      break;

      case 2: // Modo OlaRGB
       if (now - LoopFlash > (VelociLED*2)) { // 
        
        LoopFlash = now;
        ++CuentaOla;
        if (CuentaOla >= 360) CuentaOla = 0;
        
        if ((CuentaOla >= 0) && (CuentaOla < 60)){
          RojoDim = dimer * 255;
          VerdeDim = dimer * (4.25 * CuentaOla);
          AzulDim = 0;
        }
        if ((CuentaOla >= 60) && (CuentaOla < 120)){
          RojoDim = dimer * (4.25 * (60 - (CuentaOla - 60)));
          VerdeDim = dimer * 255;
          AzulDim = 0;
        }
        if ((CuentaOla >= 120) && (CuentaOla < 180)){
          RojoDim = 0;
          VerdeDim = dimer * 255;
          AzulDim = dimer * (4.25 * (CuentaOla - 120));
        }
        if ((CuentaOla >= 180) && (CuentaOla < 240)){
          RojoDim = 0;
          VerdeDim = dimer * (4.25 * (60 - (CuentaOla - 180)));
          AzulDim = dimer * 255;
        }
        if ((CuentaOla >= 240) && (CuentaOla < 300)){
          RojoDim = dimer * (4.25 * (CuentaOla - 240));
          VerdeDim = 0;
          AzulDim = dimer * 255;
        }
        if ((CuentaOla >= 300) && (CuentaOla < 360)){
          RojoDim = dimer * 255;
          VerdeDim = 0;
          AzulDim = dimer * (4.25 * (60 - (CuentaOla - 300)));
        }
        RGBOn(RojoDim, VerdeDim, AzulDim);
      }
        //
      break;

      case 3: // Modo FlashRGB
  
      if (now - LoopFlash > (VelociLED*3)) { // 
        LoopFlash = now;
        if (Flashh) {
          RojoDim = dimer * (random(2)*255);
          VerdeDim = dimer * (random(2)*255);
          AzulDim = dimer * (random(2)*255);
          if ((RojoDim == 0) && (VerdeDim == 0) && (AzulDim == 0)) {RojoDim = (dimer * 255);VerdeDim = (dimer * 255);AzulDim = (dimer * 255);}
          RGBOn(RojoDim, VerdeDim, AzulDim);
          Flashh = false;
        } else {
            RGBOff();
            Flashh = true;
          }
      }

        //
      break;

      case 4: // Modo LluviaRGB
  
      if (now - LoopLluvia > (VelociLED*6)) { // 
        LoopLluvia = now;
          RojoDim = dimer * random(255);
          VerdeDim = dimer * random(255);
          AzulDim = dimer * random(255);
          if ((RojoDim == 0) && (VerdeDim == 0) && (AzulDim == 0)) {RojoDim = (dimer * 255);VerdeDim = (dimer * 255);AzulDim = (dimer * 255);}
          RGBOn(RojoDim, VerdeDim, AzulDim);
          Flashh = false;
      }

        //
      break;

      case 5: // Modo Respira
       if (now - LoopFlash > (VelociLED*2)) { // 
        
        LoopFlash = now;
        ++CuentaOla;
        if (CuentaOla >= 120) CuentaOla = 0;
        
        if (CuentaOla < 60){
          RojoDim = dimer * Rojo * (CuentaOla / 60);
          VerdeDim = dimer * Verde * (CuentaOla / 60);
          AzulDim = dimer * Azul * (CuentaOla / 60);
        }
        if ((CuentaOla >= 60) && (CuentaOla < 120)){
          RojoDim = dimer * Rojo * (1 - ((CuentaOla - 60) / 60));
          VerdeDim = dimer * Verde * (1 - ((CuentaOla - 60) / 60));
          AzulDim = dimer * Azul * (1 - ((CuentaOla - 60) / 60));
        }
        RGBOn(RojoDim, VerdeDim, AzulDim);
      }
  
        //
      break;

      case 6: // Modo Flash

      if (now - LoopFlash > (VelociLED*3)) { // 
        LoopFlash = now;
        if (Flashh) {
          RojoDim = dimer * Rojo;
          VerdeDim = dimer * Verde;
          AzulDim = dimer * Azul;
          RGBOn(RojoDim, VerdeDim, AzulDim);
          Flashh = false;
        } else {
            RGBOff();
            Flashh = true;
          }
      }

        //
      break;

      default: //
        LEDMode = 1;  
        //
      break;
    }
  } else {
      RGBOff();
    }
}

void RGBOn(int ERE, int GE, int BE) {
  analogWrite(RR, ERE);
  analogWrite(GG, GE);
  analogWrite(BB, BE);    
}

void RGBOff() {
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

  if ( (!strcmp(PenUlTopic, "Estar")) && (!strcmp(UlTopic, "LuzRGB")) ) { ////////// funcion que recibe extrae los colores RGB del la luz
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
  } /////////////

  if (!strcmp(UlTopic, "LuzIntensidad")) {
    dimer = (atof(Pload) / 100);
    //printf( "\n\n//////////// SET solo cuando el topic es LuzIntensidad ///////////\n");
  } /////////////

  if ((!strcmp(UlTopic, "LuzEstado")) ){
    //printf( "\n\n//////////// SET u OFF solo cuando el topic es LuzEstado ///////////\n");
    if (atoi(Pload) == 0) {
      RGBEstado = false;
    }
    if (atoi(Pload) == 1) {
      RGBEstado = true;
    }
  } /////////////

  if (!strcmp(UlTopic, "ModoLED")) {
    if (!strcmp(Pload, "Normal"))    LEDMode = 1;
    if (!strcmp(Pload, "OlaRGB"))    LEDMode = 2;
    if (!strcmp(Pload, "FlashRGB"))  LEDMode = 3;
    if (!strcmp(Pload, "LluviaRGB")) LEDMode = 4;
    if (!strcmp(Pload, "Respira"))   LEDMode = 5;
    if (!strcmp(Pload, "Flash"))     LEDMode = 6;
  } /////////////

  if (!strcmp(UlTopic, "VelocidadLED")) {
    VelociLED = (100 - atoi(Pload));
  } /////////////

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
      if (!strcmp(UlTopic, "DecoBtnOk"))    IRTVDeco.sendNEC(0x101F00F, 32); 
      if (!strcmp(UlTopic, "DecoBtnUp"))    IRTVDeco.sendNEC(0x101609F, 32); 
      if (!strcmp(UlTopic, "DecoBtnDw"))    IRTVDeco.sendNEC(0x101E01F, 32); 
      if (!strcmp(UlTopic, "DecoBtnDe"))    IRTVDeco.sendNEC(0x101E817, 32); 
      if (!strcmp(UlTopic, "DecoBtnIz"))    IRTVDeco.sendNEC(0x101708F, 32); 
      if (!strcmp(UlTopic, "DecoBtnMute"))  IRTVDeco.sendNEC(0x10158A7, 32); 
      if (!strcmp(UlTopic, "DecoBtnGuide")) IRTVDeco.sendNEC(0x1018877, 32); 
      if (!strcmp(UlTopic, "DecoBtnExit"))  IRTVDeco.sendNEC(0x101E41B, 32);
    } /////////////
    
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
    }/////////////
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
  dht.begin();
  delay(10);
  AA.begin();
  delay(10);
  IRTVDeco.begin();
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
}

void loop() {

ArduinoOTA.handle();
client.loop();
now = millis(); 

if (now - Loop1 > 5000) {
    if (!(ReconectMosq) && !(ResetMosq) && !(Ejecutando)){
      client.publish("Casandra/Estar/Mosquito","Ejecutando");
      Ejecutando = true;
    }

    if (ResetMosq) {
      ResetMosq = false;
      client.publish("Casandra/Estar/Mosquito","Reset");
    } else {
      if (ReconectMosq) {
        ReconectMosq = false;
        client.publish("Casandra/Estar/Mosquito","Reconect");
     }
    }
    Loop1 = now;
}

  if (now - Loop2 > 1000) {
    if (!client.loop()) MQTTConect();
    ++Dosis;
    if (Dosis > 30) Dosis = 0;

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
    
    if (Dosis == 15) {
      sprintf(buffer, "%d", h1_old);
      client.publish("Casandra/Estar/Humedad", buffer);
    }
    if (Dosis == 30) {
      sprintf(buffer, "%d", t1_old);
      client.publish("Casandra/Estar/Temperatura", buffer);
    }
    Loop2 = now;
  }

RGBSet();

  // Bloque de loop sin espera
}
