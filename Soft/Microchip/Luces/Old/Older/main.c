
#include <16F877A.h>
#fuses XT, NOWDT, NOPROTECT, NOLVP, PUT, NOBROWNOUT // Opciones de configuración
#use delay(clock=4000000)  //Reloj de 4MHz
//#use rs232(baud=115200, xmit=PIN_B7, rcv=PIN_B5) 

void Crux();

void main(){

long Tim1;
int i;
Tim1=0;
long k;
long kk;
k=0;
kk=0;
i=0;
short Up;
Up=true;

setup_timer_1(T1_INTERNAL | T1_DIV_BY_1);
enable_interrupts(global);
//disable_interrupts(global);
enable_interrupts(int_ext);
/*
output_high(PIN_D7);
delay_ms(500);
output_low(PIN_D7);
delay_ms(500);
output_high(PIN_D7);
delay_ms(500);
*/
set_timer1(1);


output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);
output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);
output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);
output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);
output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);
output_high(PIN_D7);
output_high(PIN_B6);
delay_ms(100);
output_low(PIN_D7);
output_low(PIN_B6);
delay_ms(100);

while (1){





i++;
if (i==15){
k++;
i=0;
}
if (Up){
kk=k;
}else{
kk = 8800 - k;
}

if (k==8800){
k=0;
if (Up){Up=false;}else{Up=True;}
}

//k=8000;

Tim1 = get_timer1();

if ((Tim1 > kk) && (Tim1 < kk + 100)) {
output_high(PIN_B6);
}

if ((Tim1 > kk + 10000) && (Tim1 < kk + 10100)) {
output_high(PIN_B6);
}



output_low(PIN_B6);

}//While
}//main

#int_ext
Cruce(){
set_timer1(0);
}


