/*=====================================================================================================
||   Proyecto: Casandra v1.1                                                                         ||
||   Autor: Gonzalo Carvallo (gonzacarv@gmail.com)                                                   ||
||   Fecha: 11/2015                                                                                  ||
||   Compilador: Arduino v1.6.5 (http://arduino.cc)                                                  ||
||   Libreria: Ethercard-master (https://github.com/jcw/ethercard)                                   ||
||                                                                                                   ||
|| Firmware del modulo Web (webserver) para el control del sistema domotico CASANDRA. El modulo      ||
|| recibe comandos desde LAN con el siguiente formato: http://192.168.0.117/AAAXXXBBBYYY (IP fija    ||
|| con opcion de DHCP), donde XXX e YYY corresponden cada una a un BYTE (0-255) que será reenviado   ||
|| al bus de CASANDRA en el formato correspondiente (250,BYTE1,BYTE2,CHECKSUM), mediante el          ||
|| el transductor SN75176 al par trensado                                                            ||
||                                                                                                   ||
======================================================================================================*/

void setup(){
  Serial.begin(2400); // Baudios del bus de Casandra
  pinMode(6, OUTPUT); // LED Piloto 
  pinMode(7, OUTPUT); // Habilitador para hablar en el bus 
}

 /////////////////////////////////// VARIABLES GLOBALES///////////////////////////////////
  
  int C1 = 0; // Comando 1 en formato integer
  int C2 = 0; // Comando 2 en formato integer
  int exC1 = 0; // Ex comando 1 en formato integer
  int exC2 = 0; // Ex comando 2 en formato integer

  
void loop(){


  /////////////////////////////////// Habilito Bus y LED piloto y mando el comando ///////////////////////////////////
         digitalWrite(6,HIGH); // Llego un comando nuevo, prendemos el LED
         digitalWrite(7,HIGH);

         Serial.write(250);
         Serial.write(70);
         Serial.write(90);
         Serial.write(70 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(70);
         Serial.write(80);
         Serial.write(70 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(70);
         Serial.write(90);
         Serial.write(70 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(70);
         Serial.write(80);
         Serial.write(70 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(70);
         Serial.write(90);
         Serial.write(70 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(70);
         Serial.write(80);
         Serial.write(70 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(70);
         Serial.write(90);
         Serial.write(70 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(70);
         Serial.write(80);
         Serial.write(70 + 80);
         delay(500);



         digitalWrite(6,LOW); // Llego un comando nuevo, prendemos el LED
         digitalWrite(7,LOW);

         Serial.write(250);
         Serial.write(58);
         Serial.write(90);
         Serial.write(58 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(58);
         Serial.write(80);
         Serial.write(58 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(58);
         Serial.write(90);
         Serial.write(58 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(58);
         Serial.write(80);
         Serial.write(58 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(58);
         Serial.write(90);
         Serial.write(58 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(58);
         Serial.write(80);
         Serial.write(58 + 80);
         delay(500);

         Serial.write(250);
         Serial.write(58);
         Serial.write(90);
         Serial.write(58 + 90);
         delay(500);
         
         Serial.write(250);
         Serial.write(58);
         Serial.write(80);
         Serial.write(58 + 80);
         delay(500);

      
} // Prog
