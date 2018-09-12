#ifndef ADC11131_H
#define ADC11131_H

#include <SPI.h>

namespace ADC11131
{
    uint32_t default_clock = 1000000;

    enum ScanControl
    {
        NA =        B0000,
        MANUAL =    B0001,
        REPEAT =    B0100,
        STD_INT =   B0011,
        STD_EXT =   B0100,
        UPPER_INT = B0101,
        UPPER_EXT = B0110,
        CUST_INT =  B0110,
        CUST_EXT =  B1000
    };

    enum Reset
    {
        NONE =      B00,
        FIFO_ONLY = B01,
        ALL =       B10         
    };

    enum PowerManagement
    {
        NORMAL =        B00,
        AUTO_SHUTDOWN = B01,
        AUTO_STANDBY =  B10
    };

    class ADCMessage
    {
        /*[ADC Mode Control] 0

        [ADC Mode SCAN3]   0      
        [ADC Mode SCAN2]   0      
        [ADC Mode SCAN1]   0      
        [ADC Mode SCAN0]   1   

        [Chnl CHSEL3]      1  
        [Chnl CHSEL2]      0
        [Chnl CHSEL1]      0
        [Chnl CHSEL0]      1

        [RESET]            0
        [RESET]            0

        [PM]               0
        [PM]               0

        [CHAN_ID]          1

        [SWCNV]            0  unsure

        [UNUSED]           0
        */   

    public:
        uint16_t message;

        void set_config( bool val )
        {
            message &= 0x8000;
            message |= uint16_t(val) << 15;
        }

        void set_scan_control( ScanControl val )
        {
            message &= 0x87FF;
            message |= uint16_t(val) << 11;
        }

        void set_channel( uint8_t val )
        {
            val = min(15,max(val,0));
            
            message &= 0xF87F;
            message |= uint16_t(val) << 7;            
        }

        void set_reset( Reset val )
        {
            message &= 0xFF9F;
            message |= uint16_t(val) << 5;
        }

        void set_power_management( PowerManagement val )
        {
            message &= 0xFFE7;
            message |= uint16_t(val) << 3;
        }

        void set_chan_id( bool val )
        {
            message &= 0xFFFB;
            message |= uint16_t(val) << 2;
        }

        void set_swcnv( bool val )
        {
            message &= 0xFFFD;
            message |= uint16_t(val) << 1;
        }

        operator uint16_t() const
        {
            return message;
        }

        ADCMessage( ScanControl sc=ScanControl::MANUAL, 
                    Reset r=Reset::NONE, 
                    PowerManagement pw=PowerManagement::NORMAL, 
                    bool chanId=true, bool swcnv=true, bool config=false ) : message(0)
        {
            set_config(config);
            set_scan_control(sc);
            set_reset(r);
            set_power_management(pw);
            set_chan_id(chanId);
            set_swcnv(swcnv);
        }

        ADCMessage( uint8_t channel,
                    ScanControl sc=ScanControl::MANUAL, 
                    Reset r=Reset::NONE, 
                    PowerManagement pw=PowerManagement::NORMAL, 
                    bool chanId=true, bool swcnv=true )
            : ADCMessage(sc, r, pw, chanId, swcnv)
        {
            set_channel( channel );
        }
    };

    class ADC
    {
    private:
        ADCMessage message;
        SPISettings settings;
        SPIClass spi;

    public:
        const uint8_t cs;

        void reset()
        {
            ADCMessage reset( ScanControl::NA, 
                        Reset::ALL,
                        PowerManagement::NORMAL,
                        false, false, true );

            SPI.beginTransaction( settings );
            digitalWrite( cs, LOW );

            uint8_t high = SPI.transfer( highByte( uint16_t(reset.message) ) );
            uint8_t low = SPI.transfer( lowByte( uint16_t(reset.message) ) );

            digitalWrite( cs, HIGH );
            delayMicroseconds(5);
            SPI.endTransaction();
        }

        ADC( uint8_t _cs, SPIClass& _spi=SPI, uint32_t clock=default_clock ) 
            : settings(clock, MSBFIRST, SPI_MODE3), spi(_spi), cs(_cs)
        {

        }

        void begin()
        {
            pinMode( cs, OUTPUT );
            digitalWrite( cs, HIGH );
            reset();
        }

        /* sends ADCMessage to read a given channel but channel reading will 
            be returned on the NEXT _read_channel call 
            message.setChannel() needs to have been set already */
        inline uint16_t _read_channel()
        {
            uint16_t info = 0;
            uint8_t* ptr = (uint8_t*)&info;

            digitalWrite( cs, LOW );

            //*(ptr+1) = SPI.transfer( highByte( uint16_t(message) ) );
            //*(ptr+0) = SPI.transfer( lowByte( uint16_t(message) ) );
            uint8_t high = SPI.transfer( highByte( uint16_t(message) ) );
            uint8_t low = SPI.transfer( lowByte( uint16_t(message) ) );

            info |= int16_t(high) << 8;
            info |= int16_t(low);
            
            digitalWrite( cs, HIGH );
            delayMicroseconds(5);

            return info;
        }

        uint16_t read_channel( uint8_t channel )
        {
            SPI.beginTransaction( settings );

            message.set_channel( channel );
           
            _read_channel();
            uint16_t info = _read_channel();

            SPI.endTransaction();

            if( (info & 0xF000) >> 12 == channel )
                return info & 0x0FFF;

            return 0;
        }

        /* not working at moment */
        uint16_t* read_channels( uint16_t* buffer, size_t size, size_t start=0 )
        {
            
            for( size_t i=0; i<size; ++i )
            {
                size_t channel = start+i;

                buffer[i] = read_channel( channel );
            }

            /*uint16_t info;

            SPI.beginTransaction( settings );

            
            for( size_t i=0; i!=size; ++i )
            {
                const size_t channel = start+i;

                Serial.printf( "i: %d\n", i );
                Serial.printf( " channel: %d\n", channel );

                // first iteration, prime the sequence
                if( i==0 )
                {
                    Serial.println(" first");
                    message.set_channel( channel );
                    _read_channel();
                }

                info = _read_channel();

                Serial.printf( " retChan: %d\n", (info&0xF000)>>12 );

                if( (info & 0xF000) >> 12 == channel )
                    buffer[i] = info & 0x0FFF;
                else
                    buffer[i] = 0;

                // last iteration, might go out of valid channel range but ADCmessage will cap it
                Serial.printf( "request: %d\n", channel+1 );
                message.set_channel(channel+1);
            }

          

            SPI.endTransaction();*/

            return buffer;
        }
    };
};

#endif