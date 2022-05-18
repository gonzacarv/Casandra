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

#define Rele1 5          // Rele1  D1
#define Sirena 16        // Sirena D0
#define MSG_BUFFER_SIZE  (50)

const char* ssid = "CasandraL2";
const char* password = "crallova";
const char* mqtt_server = "192.168.0.58"; 
String clientId = "Mosquito-HALL";
const char* Topico = "Casandra/Hall/#"; // Solo subscripto al topico de Caldera con comodin aguas abajo
unsigned long lastMsg = 0;
char Topicc[MSG_BUFFER_SIZE];
char Argu[MSG_BUFFER_SIZE];
int EstadoSirena = 0;
int Cuenta = 0;

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
  WiFi.hostname("MosquitoHall");
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

  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sodios") && (atoi(Pload) == 1)) {
    digitalWrite(Rele1,HIGH); 
    //Serial.print("   Sodios Prendidos   ");
}
  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sodios") && (atoi(Pload) == 0)) {
    digitalWrite(Rele1,LOW); 
    //Serial.print("   Sodios Apagados   ");
  }

  if (!strcmp(PenUlTopic, "Hall") && !strcmp(UlTopic, "Sirena")) EstadoSirena = atoi(Pload); 
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
  pinMode(Rele1, OUTPUT);
  pinMode(Sirena, OUTPUT);
  Serial.begin(2400);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  digitalWrite(Rele1,LOW);
  digitalWrite(Sirena,LOW);
  EstadoSirena = 0;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // ciclado cada 30 segundos
  if (now - lastMsg > 500) {
    lastMsg = now;
    Cuenta = Cuenta + 1;
    if (Cuenta == 4) Cuenta = 0;
    switch (EstadoSirena){
      case 0:
        digitalWrite(Sirena,LOW);
      break;

      case 1:
        digitalWrite(Sirena,HIGH);
      break;

      case 2:
        if (Cuenta == 0) digitalWrite(Sirena,HIGH);
        else digitalWrite(Sirena,LOW);
      break;

      case 3:
        if ((Cuenta == 0) || (Cuenta == 2)) digitalWrite(Sirena,HIGH);
        else digitalWrite(Sirena,LOW);
      break;

      case 4:
        if ((Cuenta == 0) || (Cuenta == 1)) digitalWrite(Sirena,HIGH);
        else digitalWrite(Sirena,LOW);
      break;

      case 5:
        if (Cuenta == 0) digitalWrite(Sirena,LOW);
        else digitalWrite(Sirena,HIGH);
      break;

      default:
        digitalWrite(Sirena,LOW);
      break;
    }

  } // Loop cada 500 milisegundos

//Bloque de loop sin espera

}
