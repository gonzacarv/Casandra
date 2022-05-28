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

#define SwAbrir    05   // Switch abrir porton D1
#define SwCerrar   04   // Switch cerrar porton D2
#define LuzCamino  00   // Luz camino D3
#define Timbre     14   // Timbre D5
#define SensPuerta 12   // Sensor Puerta D6
#define SensPorton 13   // Sensor Porton D7
#define MSG_BUFFER_SIZE  (50)

const char* ssid = "CasandraEXT";
const char* password = "crallova";
const char* mqtt_server = "192.168.0.58"; 
String clientId = "Mosquito-PORTON";
const char* Topico = "Casandra/Porton/#"; // Solo subscripto al topico del Porton con comodin aguas abajo
//int EstadoPuerta = 12;  // Digital pin D6
//int EstadoPorton = 13;  // Digital pin D7
//int EstadoTimbre = 14;  // Digital pin D5
bool EstadoPuerta_old = false;
bool EstadoPorton_old = false;
bool EstadoTimbre_old = false;
unsigned long lastMsg = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  int Reseteo = 0;
  // Inicia conexion WiFi
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("MosquitoPorton");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ++Reseteo;
    if (Reseteo == 100)  ESP.reset(); 
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

  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "SwAbrir")) && (atoi(Pload) == 1) ) {
    digitalWrite(SwAbrir,HIGH);
    delay(500);
    client.publish("Casandra/Porton/SwAbrir", "0");
    digitalWrite(SwAbrir,LOW);    
  }

  if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "SwCerrar")) && (atoi(Pload) == 1) ) {
    digitalWrite(SwCerrar,HIGH);
    delay(500);
    client.publish("Casandra/Porton/SwCerrar", "0");
    digitalWrite(SwCerrar,LOW);    
  }

 if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "LuzCamino")) && (atoi(Pload) == 1) ) {
  digitalWrite(LuzCamino,HIGH);
 }
 if ( (!strcmp(PenUlTopic, "Porton")) && (!strcmp(UlTopic, "LuzCamino")) && (atoi(Pload) == 0) ) {
  digitalWrite(LuzCamino,LOW);
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
    if (client.connect(clientId.c_str(),"mqttuser","MQTTpass")) {
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
  pinMode(Timbre, INPUT);
  pinMode(SensPuerta, INPUT);
  pinMode(SensPorton, INPUT);
  pinMode(SwAbrir, OUTPUT);
  pinMode(SwCerrar, OUTPUT);
  pinMode(LuzCamino, OUTPUT);
  Serial.begin(2400);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  digitalWrite(SwAbrir,LOW);
  digitalWrite(SwCerrar,LOW);
  digitalWrite(LuzCamino,LOW);
}
  
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // ciclado cada 1 segundo
  int Tim = 0;
  
  if (now - lastMsg > 500) {
  
    if (Tim == 0) {
    if (digitalRead(Timbre) != EstadoTimbre_old){ //Son distintos, guardamos el nuevo en el viejo
      Tim = 5; // Esperamos 2 segundos y medio
      EstadoTimbre_old = digitalRead(Timbre);
      if (EstadoTimbre_old) client.publish("Casandra/Porton/Timbre", "1");
      else client.publish("Casandra/Porton/Timbre", "0");
    }
    } else --Tim;

    if (digitalRead(SensPuerta) != EstadoPuerta_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPuerta_old = digitalRead(SensPuerta);
      if (EstadoPuerta_old) client.publish("Casandra/Porton/SensorPuerta", "1");
      else client.publish("Casandra/Porton/SensorPuerta", "0");
    }

    if (digitalRead(SensPorton) != EstadoPorton_old){ //Son distintos, guardamos el nuevo en el viejo
      EstadoPorton_old = digitalRead(SensPorton);
      if (EstadoPorton_old) client.publish("Casandra/Porton/SensorPorton", "1");
      else client.publish("Casandra/Porton/SensorPorton", "0");
    }

  } // Loop cada 1 segundo

//Bloque de loop sin espera

}
