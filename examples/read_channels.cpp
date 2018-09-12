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
    uint16_t readings[16];
    adc.read_channels( readings, 16 );

    for( size_t i=0; i<16; ++i )
    {
        Serial.printf("ch%02d = %d\n", i, readings[i] );
    }

    delay(5000);
}