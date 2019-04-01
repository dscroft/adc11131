#ifndef ADC11131_H
#define ADC11131_H

#include <SPI.h>

namespace ADC11131
{
    uint32_t default_clock = 29000000;

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

            SPI.transfer16( reset );

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

            digitalWriteFast( cs, LOW );
            
            info = SPI.transfer16( message );
            
            digitalWriteFast( cs, HIGH );

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

        /* read multiple channels
            will read size channels into the buffer starting at channel 0 */
        template<size_t size=16>
        bool read_all( uint16_t* buffer )
        {
            bool success = true;
            const ADCMessage primemessage( size-1, ADC11131::ScanControl::STD_EXT );
            const ADCMessage message( size-1, ADC11131::ScanControl::NA );  
    
            SPI.beginTransaction( settings );

            digitalWriteFast( cs, LOW );
            SPI.transfer16( primemessage );
            digitalWriteFast( cs, HIGH );

            for( size_t i=0; i<size; ++i )
            {
                digitalWriteFast( cs, LOW );
                buffer[i] = SPI.transfer16( message );
                digitalWriteFast( cs, HIGH );
            }

            SPI.endTransaction();

            for( size_t i=0; i<size; ++i )
            {
                if( (buffer[i] & 0xF000) >> 12 != i ) success = false;              
                buffer[i] &= 0x0FFF;
            }

            return success;
        }
    };
};

#endif