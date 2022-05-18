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
#include <PubSubClient.h>
#include <string> 

const char* ssid = "CasandraC2";
const char* password = "crallova";
const char* mqtt_server = "192.168.0.58";
String clientId = "Mosquito-CUARTOS";
const char* Topico = "Casandra/Cuartos/#";

#define LEDpin D7  // Declaramos el piloto de info de estado
#define WiFiLED D6 // Declaramos el pilotode info de conexion WiFi
#define EnTx D8    // Declaramos el habilitador de transmision

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // Inicia conexion WiFi
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

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
  //if (!strcmp(PenUlTopic, "LuzEstado")) printf("Es igual a LuzEstado y la variable int del final es %d y el payload es %d\n", atoi(UlTopic), atoi(Pload));
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 0)) Hablador(atoi(UlTopic)+32, 80);
  if (!strcmp(PenUlTopic, "LuzEstado") && (atoi(Pload) == 1)) Hablador(atoi(UlTopic)+32, 90);
  if (!strcmp(PenUlTopic, "LuzIntensidad")){
    float dimer;
    dimer = ( (atoi(Pload) * (-0.27) ) + 27);
    Hablador(atoi(UlTopic)+32, int(dimer));
  }

}

/////////////////////////// Funcion que habla en el bus ////////////////////////////////////////
void Hablador(int x, int y){  // La funcion encargada de hablar en el bus
         digitalWrite(EnTx,HIGH); // Llego un comando nuevo, prendemos el LED
         digitalWrite(LEDpin,HIGH);
         delay(5);
         Serial.write(250);
         Serial.write(x); 
         Serial.write(y);
         Serial.write(x + y);
         delay(25);
         digitalWrite(EnTx,LOW);
         digitalWrite(LEDpin,LOW);
         
}
void reconnect() {
  // Loopea hasta reconectar
  while (!client.connected()) {
    Serial.print("Intentando conectar a broker MQTT..");
    // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"mqttuser","MQTTpass")) {
      Serial.println("¡Conectado! :D");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(Topico);
    } else {
      Serial.print("Falla de conexion, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LEDpin, OUTPUT);  // Tanto los dos pilotos...
  pinMode(EnTx, OUTPUT);    // ...como el habilitador...
  pinMode(WiFiLED, OUTPUT); // ...son salidas.
  Serial.begin(2400);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    ++value;
    //snprintf (msg, MSG_BUFFER_SIZE, "Recuento cada 2 seg: #%ld", value);
    // Serial.print("Publish message: ");
    //Serial.println(msg);
    //client.publish("Casandra", msg);
  }
}
