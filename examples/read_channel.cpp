#include <Arduino.h>
#include <SPI.h>
#include <adc11131.h>

#define SS 34

ADC11131::ADC adc( SS );

void setup() 
{
    while( !Serial );
    Serial.begin(9600);

    SPI.begin();

    adc.begin();
}

void loop() 
{
    Serial.printf("ch%02d = %d\n", 9, adc.read_channel(9) );

    delay(5000);
}