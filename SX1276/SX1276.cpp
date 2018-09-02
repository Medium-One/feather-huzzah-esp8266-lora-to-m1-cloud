#include "SX1276.h"
#include <SPI.h>

/*
 Functions
 Returns: Integer that determines if there has been any error
   state = 2  --> The command has not been executed
   state = 1  --> There has been an error while executing the command
   state = 0  --> The command has been executed with no errors
   state = -1 --> Forbidden command for this protocol
*/


SX1276::SX1276()
{
    // Initialize class variables
    _bandwidth = BW_125;
    _codingRate = CR_5;
    _spreadingFactor = SF_7;
    _channel = CH_12_900;
    _power = 15;
    _packetNumber = 0;
    _reception = CORRECT_PACKET;
    _retries = 0;
    _my_netkey[0] = 0x00;
    _my_netkey[1] = 0x00;
    _maxRetries = 3;
    packet_sent.retry = _retries;
};

/*
 Function: Sets the module ON.
*/
uint8_t SX1276::ON()
{
    uint8_t state = 2;

    // Powering the module
    pinMode(SX1276_SS,OUTPUT);
    digitalWrite(SX1276_SS,HIGH);
    delay(100);

    //Configure the MISO, MOSI, CS, SPCR.
    SPI.begin();
	
    //Set Most significant bit first
    SPI.setBitOrder(MSBFIRST);

    // for the MEGA, set to 2MHz
	SPI.setClockDivider(SPI_CLOCK_DIV8);

	SPI.setDataMode(SPI_MODE0);

    delay(100);
	
	packet_sent.type = PKT_TYPE_DATA;

    pinMode(SX1276_RST,OUTPUT);
    digitalWrite(SX1276_RST,HIGH);
    delay(100);
    digitalWrite(SX1276_RST,LOW);
    delay(100);

	
	digitalWrite(SX1276_RST, LOW);
	delay(100);
	digitalWrite(SX1276_RST, HIGH);
	delay(100);
	uint8_t version = readRegister(REG_VERSION);
	if (version == 0x12) {
		Serial.println(F("SX1276 detected, starting"));
	} else {
		Serial.println(F("Unrecognized transceiver"));
	}
	
    setMaxCurrent(0x1B);

    // set LoRa mode
    byte st0;
    uint8_t retry=0;

    do {
        delay(200);
        writeRegister(REG_OP_MODE, FSK_SLEEP_MODE);    // Sleep mode (mandatory to set LoRa mode)
        writeRegister(REG_OP_MODE, LORA_SLEEP_MODE);    // LoRa sleep mode
        writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
        delay(50+retry*10);
        st0 = readRegister(REG_OP_MODE);
        Serial.println(F("..."));

        if ((retry % 2)==0)
            if (retry==20)
                retry=0;
            else
                retry++;

    } while (st0!=LORA_STANDBY_MODE);	// LoRa standby mode

    if( st0 == LORA_STANDBY_MODE)
    { 
		state = 0;
    } else {
		state = 1;
		Serial.println(F("** There has been an error while setting LoRa **"));
	}
    return state;
}

/*
 Function: Sets the module OFF.
*/
void SX1276::OFF()
{
    SPI.end();

    // Powering the module
    pinMode(SX1276_SS,OUTPUT);
    digitalWrite(SX1276_SS,LOW);
}

/*
 Function: Reads the indicated register.
*/
byte SX1276::readRegister(byte address)
{
    byte value = 0x00;

    digitalWrite(SX1276_SS,LOW);
    bitClear(address, 7);		// Bit 7 cleared to write in registers
    SPI.transfer(address);
    value = SPI.transfer(0x00);
    digitalWrite(SX1276_SS,HIGH);
    return value;
}

/*
 Function: Writes on the indicated register.
*/
void SX1276::writeRegister(byte address, byte data)
{
    digitalWrite(SX1276_SS,LOW);
    bitSet(address, 7);			// Bit 7 set to read from registers
    SPI.transfer(address);
    SPI.transfer(data);
    digitalWrite(SX1276_SS,HIGH);
}

/*
 Function: Clears the interruption flags
*/
void SX1276::clearFlags()
{
    byte st0;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby mode to write in registers
    writeRegister(REG_IRQ_FLAGS, 0xFF);	// LoRa mode flags register
    writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
}

/*
 Function: Sets the bandwidth, coding rate and spreading factor of the LoRa modulation.
*/
int8_t SX1276::setMode(uint8_t mode)
{
    int8_t state = 2;
    byte st0;
    byte config1 = 0x00;
    byte config2 = 0x00;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode

    switch (mode)
    {
		// mode 1 (better reach, medium time on air)
		case 1:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_125);      // BW = 125 KHz
			break;

		// mode 2 (medium reach, less time on air)
		case 2:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_250);      // BW = 250 KHz
			break;

		// mode 3 (worst reach, less time on air)
		case 3:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_10);       // SF = 10
			setBW(BW_125);      // BW = 125 KHz
			break;

		// mode 4 (better reach, low time on air)
		case 4:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_12);       // SF = 12
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 5 (better reach, medium time on air)
		case 5:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_10);       // SF = 10
			setBW(BW_250);      // BW = 250 KHz
			break;

		// mode 6 (better reach, worst time-on-air)
		case 6:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_11);       // SF = 11
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 7 (medium-high reach, medium-low time-on-air)
		case 7:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_9);        // SF = 9
			setBW(BW_250);      // BW = 250 KHz
			break;

			// mode 8 (medium reach, medium time-on-air)
		case 8:     
			setCR(CR_5);        // CR = 4/5
			setSF(SF_9);        // SF = 9
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 9 (medium-low reach, medium-high time-on-air)
		case 9:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_8);        // SF = 8
			setBW(BW_500);      // BW = 500 KHz
			break;

		// mode 10 (worst reach, less time_on_air)
		case 10:
			setCR(CR_5);        // CR = 4/5
			setSF(SF_7);        // SF = 7
			setBW(BW_500);      // BW = 500 KHz
			break;
		default:    state = -1; // The indicated mode doesn't exist
    };

    if( state != -1 )	// if state = -1, don't change its value
	state = 1;
	config1 = readRegister(REG_MODEM_CONFIG1);
	switch (mode)
	{   
	// mode 1: BW = 125 KHz, CR = 4/5, SF = 12.
	case 1:

		if( (config1 >> 1) == 0x39 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;


		// mode 2: BW = 250 KHz, CR = 4/5, SF = 12.
	case 2:

		if( (config1 >> 1) == 0x41 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;

		// mode 3: BW = 125 KHz, CR = 4/5, SF = 10.
	case 3:

		if( (config1 >> 1) == 0x39 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_10 )
			{
				state = 0;
			}
		}
		break;

		// mode 4: BW = 500 KHz, CR = 4/5, SF = 12.
	case 4:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_12 )
			{
				state = 0;
			}
		}
		break;

		// mode 5: BW = 250 KHz, CR = 4/5, SF = 10.
	case 5:

		if( (config1 >> 1) == 0x41 )
			state=0;
		
		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_10 )
			{
				state = 0;
			}
		}
		break;

		// mode 6: BW = 500 KHz, CR = 4/5, SF = 11.
	case 6:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_11 )
			{
				state = 0;
			}
		}
		break;

		// mode 7: BW = 250 KHz, CR = 4/5, SF = 9.
	case 7:

		if( (config1 >> 1) == 0x41 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_9 )
			{
				state = 0;
			}
		}
		break;

		// mode 8: BW = 500 KHz, CR = 4/5, SF = 9.
	case 8:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_9 )
			{
				state = 0;
			}
		}
		break;

		// mode 9: BW = 500 KHz, CR = 4/5, SF = 8.
	case 9:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_8 )
			{
				state = 0;
			}
		}
		break;

		// mode 10: BW = 500 KHz, CR = 4/5, SF = 7.
	case 10:

		if( (config1 >> 1) == 0x49 )
			state=0;

		if( state==0) {
			state = 1;
			config2 = readRegister(REG_MODEM_CONFIG2);

			if( (config2 >> 4) == SF_7 )
			{
				state = 0;
			}
		}
		break;
	}
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if SF is a valid value.
*/
boolean	SX1276::isSF(uint8_t spr)
{
    // Checking available values for _spreadingFactor
    switch(spr)
    {
    case SF_7:
    case SF_8:
    case SF_9:
    case SF_10:
    case SF_11:
    case SF_12:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Gets the SF within the module is configured.
*/
int8_t	SX1276::getSF()
{
    int8_t state = 2;
    byte config2;

	// take out bits 7-4 from REG_MODEM_CONFIG2 indicates _spreadingFactor
	config2 = (readRegister(REG_MODEM_CONFIG2)) >> 4;
	_spreadingFactor = config2;
	state = 1;

	if( (config2 == _spreadingFactor) && isSF(_spreadingFactor) )
	{
		state = 0;
	}
    return state;
}

/*
 Function: Sets the indicated SF in the module.
*/
uint8_t	SX1276::setSF(uint8_t spr)
{
    byte st0;
    int8_t state = 2;
    byte config1;
    byte config2;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
	config2 = (readRegister(REG_MODEM_CONFIG2));	// Save config2 to modify SF value (bits 7-4)
	switch(spr)
	{
	case SF_7: 	config2 = config2 & B01111111;	// clears bits 7 from REG_MODEM_CONFIG2
		config2 = config2 | B01110000;	// sets bits 6, 5 & 4
		break;
	case SF_8: 	config2 = config2 & B10001111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10000000;	// sets bit 7 from REG_MODEM_CONFIG2
		break;
	case SF_9: 	config2 = config2 & B10011111;	// clears bits 6, 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10010000;	// sets bits 7 & 4 from REG_MODEM_CONFIG2
		break;
	case SF_10:	config2 = config2 & B10101111;	// clears bits 6 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B10100000;	// sets bits 7 & 5 from REG_MODEM_CONFIG2
		break;
	case SF_11:	config2 = config2 & B10111111;	// clears bit 6 from REG_MODEM_CONFIG2
		config2 = config2 | B10110000;	// sets bits 7, 5 & 4 from REG_MODEM_CONFIG2
		getBW();

		if( _bandwidth == BW_125)
		{ // LowDataRateOptimize (Mandatory with SF_11 if BW_125)
			config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the LowDataRateOptimize
			config1 = config1 | B00000001;
			writeRegister(REG_MODEM_CONFIG1,config1);
		}
		break;
	case SF_12: config2 = config2 & B11001111;	// clears bits 5 & 4 from REG_MODEM_CONFIG2
		config2 = config2 | B11000000;	// sets bits 7 & 6 from REG_MODEM_CONFIG2
		if( _bandwidth == BW_125)
		{ // LowDataRateOptimize (Mandatory with SF_12 if BW_125)
			byte config3=readRegister(REG_MODEM_CONFIG3);
			config3 = config3 | B00001000;
			writeRegister(REG_MODEM_CONFIG3,config3);
		}
		break;
	}

	// Turn on header
	config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the header bit
	config1 = config1 & B11111110;              // clears bit 0 from config1 = headerON
	writeRegister(REG_MODEM_CONFIG1,config1);	// Update config1
	
	
	// LoRa detection Optimize: 0x03 --> SF7 to SF12
	writeRegister(REG_DETECT_OPTIMIZE, 0x03);

	// LoRa detection threshold: 0x0A --> SF7 to SF12
	writeRegister(REG_DETECTION_THRESHOLD, 0x0A);

	uint8_t config3 = (readRegister(REG_MODEM_CONFIG3));
	config3=config3 | B00000100;
	writeRegister(REG_MODEM_CONFIG3, config3);

	// here we write the new SF
	writeRegister(REG_MODEM_CONFIG2, config2);		// Update config2

	delay(100);

	byte configAgc;
	uint8_t theLDRBit;

	config1 = (readRegister(REG_MODEM_CONFIG3));	// Save config1 to check update
	config2 = (readRegister(REG_MODEM_CONFIG2));

	// LowDataRateOptimize is in REG_MODEM_CONFIG3
	// AgcAutoOn is in REG_MODEM_CONFIG3
	configAgc=config1;
	theLDRBit=3;
	

	switch(spr)
	{
	case SF_7:	if(		((config2 >> 4) == 0x07)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_8:	if(		((config2 >> 4) == 0x08)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_9:	if(		((config2 >> 4) == 0x09)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_10:	if(		((config2 >> 4) == 0x0A)
						&& (bitRead(configAgc, 2) == 1))
		{
			state = 0;
		}
		break;
	case SF_11:	if(		((config2 >> 4) == 0x0B)
						&& (bitRead(configAgc, 2) == 1)
						&& (bitRead(config1, theLDRBit) == 1))
		{
			state = 0;
		}
		break;
	case SF_12:	if(		((config2 >> 4) == 0x0C)
						&& (bitRead(configAgc, 2) == 1)
						&& (bitRead(config1, theLDRBit) == 1))
		{
			state = 0;
		}
		break;
	default:	state = 1;
	}

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);

    if( isSF(spr) )
    { // Checking available value for _spreadingFactor
        state = 0;
        _spreadingFactor = spr;
    }
    return state;
}

/*
 Function: Checks if BW is a valid value.
*/
boolean	SX1276::isBW(uint16_t band)
{
    // Checking available values for _bandwidth
	switch(band)
	{
	case BW_7_8:
	case BW_10_4:
	case BW_15_6:
	case BW_20_8:
	case BW_31_25:
	case BW_41_7:
	case BW_62_5:
	case BW_125:
	case BW_250:
	case BW_500:
		return true;
		break;

	default:
		return false;
	}
}

/*
 Function: Gets the BW within the module is configured.
*/
int8_t	SX1276::getBW()
{
    uint8_t state = 2;
    byte config1;

	config1 = (readRegister(REG_MODEM_CONFIG1)) >> 4;
	
	_bandwidth = config1;

	if( (config1 == _bandwidth) && isBW(_bandwidth) )
	{
		state = 0;
	}
	else
	{
		state = 1;
	}
    return state;
}

/*
 Function: Sets the indicated BW in the module.
*/
int8_t	SX1276::setBW(uint16_t band)
{
    byte st0;
    int8_t state = 2;
    byte config1;

    if(!isBW(band) )
    {
        state = 1;
        return state;
    }

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
	
    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// LoRa standby mode
    config1 = (readRegister(REG_MODEM_CONFIG1));	// Save config1 to modify only the BW

	config1 = config1 & B00001111;	// clears bits 7 - 4 from REG_MODEM_CONFIG1
	switch(band)
	{
	case BW_125:
		// 0111
		config1 = config1 | B01110000;
		getSF();
		if( _spreadingFactor == 11 || _spreadingFactor == 12)
		{ // LowDataRateOptimize (Mandatory with BW_125 if SF_11 or SF_12)
			byte config3=readRegister(REG_MODEM_CONFIG3);
			config3 = config3 | B00001000;
			writeRegister(REG_MODEM_CONFIG3,config3);
		}
		break;
	case BW_250:
		// 1000
		config1 = config1 | B10000000;
		break;
	case BW_500:
		// 1001
		config1 = config1 | B10010000;
		break;
	}

    writeRegister(REG_MODEM_CONFIG1,config1);		// Update config1

    delay(100);

    config1 = (readRegister(REG_MODEM_CONFIG1));

	switch(band)
	{
	case BW_125: if( (config1 >> 4) == BW_125 )
		{
			state = 0;

			byte config3 = (readRegister(REG_MODEM_CONFIG3));

			if( _spreadingFactor == 11 )
			{
				if( bitRead(config3, 3) == 1 )
				{ // LowDataRateOptimize
					state = 0;
				}
				else
				{
					state = 1;
				}
			}
			if( _spreadingFactor == 12 )
			{
				if( bitRead(config3, 3) == 1 )
				{ // LowDataRateOptimize
					state = 0;
				}
				else
				{
					state = 1;
				}
			}
		}
		break;
	case BW_250: if( (config1 >> 4) == BW_250 )
		{
			state = 0;
		}
		break;
	case BW_500: if( (config1 >> 4) == BW_500 )
		{
			state = 0;
		}
		break;
	}

    if(state==0)
    {
        _bandwidth = band;
    }
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if CR is a valid value.
*/
boolean	SX1276::isCR(uint8_t cod)
{
    // Checking available values for _codingRate
    switch(cod)
    {
    case CR_5:
    case CR_6:
    case CR_7:
    case CR_8:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Sets the indicated CR in the module.
*/
int8_t	SX1276::setCR(uint8_t cod)
{
    byte st0;
    int8_t state = 2;
    byte config1;

    st0 = readRegister(REG_OP_MODE);		// Save the previous status

    writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);		// Set Standby mode to write in registers

    config1 = readRegister(REG_MODEM_CONFIG1);	// Save config1 to modify only the CR
	config1 = config1 & B11110001;	// clears bits 3 - 1 from REG_MODEM_CONFIG1
	switch(cod)
	{
	case CR_5:
		config1 = config1 | B00000010;
		break;
	case CR_6:
		config1 = config1 | B00000100;
		break;
	case CR_7:
		config1 = config1 | B00000110;
		break;
	case CR_8:
		config1 = config1 | B00001000;
		break;
	}

    writeRegister(REG_MODEM_CONFIG1, config1);		// Update config1

    delay(100);

    config1 = readRegister(REG_MODEM_CONFIG1);

    uint8_t nshift=1;

    switch(cod)
    {
    case CR_5: if( ((config1 >> nshift) & B0000111) == 0x01 )
        {
            state = 0;
        }
        break;
    case CR_6: if( ((config1 >> nshift) & B0000111) == 0x02 )
        {
            state = 0;
        }
        break;
    case CR_7: if( ((config1 >> nshift) & B0000111) == 0x03 )
        {
            state = 0;
        }
        break;
    case CR_8: if( ((config1 >> nshift) & B0000111) == 0x04 )
        {
            state = 0;
        }
        break;
    }


    if( isCR(cod) )
    {
        _codingRate = cod;
    }
    else
    {
        state = 1;
    }
    writeRegister(REG_OP_MODE,st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Checks if channel is a valid value.
*/
boolean	SX1276::isChannel(uint32_t ch)
{
    // Checking available values for _channel
    switch(ch)
    {
    case CH_10_868:
    case CH_11_868:
    case CH_12_868:
    case CH_13_868:
    case CH_14_868:
    case CH_15_868:
    case CH_16_868:
    case CH_17_868:
    case CH_18_868:
    case CH_00_900:
    case CH_01_900:
    case CH_02_900:
    case CH_03_900:
    case CH_04_900:
    case CH_05_900:
    case CH_06_900:
    case CH_07_900:
    case CH_08_900:
    case CH_09_900:
    case CH_10_900:
    case CH_11_900:
    case CH_12_900:
        return true;
        break;

    default:
        return false;
    }
}

/*
 Function: Sets the indicated channel in the module.
*/
int8_t SX1276::setChannel(uint32_t ch)
{
    byte st0;
    int8_t state = 2;
    unsigned int freq3;
    unsigned int freq2;
    uint8_t freq1;
    uint32_t freq;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
    
    freq3 = ((ch >> 16) & 0x0FF);		// frequency channel MSB
    freq2 = ((ch >> 8) & 0x0FF);		// frequency channel MIB
    freq1 = (ch & 0xFF);				// frequency channel LSB

    writeRegister(REG_FRF_MSB, freq3);
    writeRegister(REG_FRF_MID, freq2);
    writeRegister(REG_FRF_LSB, freq1);

    delay(100);

    // storing MSB in freq channel value
    freq3 = (readRegister(REG_FRF_MSB));
    freq = (freq3 << 8) & 0xFFFFFF;

    // storing MID in freq channel value
    freq2 = (readRegister(REG_FRF_MID));
    freq = (freq << 8) + ((freq2 << 8) & 0xFFFFFF);

    // storing LSB in freq channel value
    freq = freq + ((readRegister(REG_FRF_LSB)) & 0xFFFFFF);

    if( freq == ch )
    {
        state = 0;
        _channel = ch;
    }
    else
    {
        state = 1;
    }

    if(!isChannel(ch) )
    {
        state = -1;
    }

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Sets the signal power indicated in the module.
*/
int8_t SX1276::setPower(char p)
{
    byte st0;
    int8_t state = 2;
    byte value = 0x00;
	byte RegPaDacReg=0x4D;

    st0 = readRegister(REG_OP_MODE);	  // Save the previous status
	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);
    
    switch (p)
    {
    // L = Low. On SX1272/76: PA0 on RFO setting
    // H = High. On SX1272/76: PA0 on RFO setting
    // M = MAX. On SX1272/76: PA0 on RFO setting
    // X = extreme. On SX1272/76: PA1&PA2 PA_BOOST setting + 20dBm settings

    case 'X':
    case 'M':  value = 0x0F;
        // SX1272/76: 14dBm
        break;
	case 'L':  value = 0x03;
        // SX1272/76: 2dBm
        break;

    case 'H':  value = 0x07;
        // SX1272/76: 6dBm
        break;

    default:   state = -1;
        break;
    }

    // 100mA
    setMaxCurrent(0x0B);

    if (p=='X') {
        // normally value = 0x0F;
        // we set the PA_BOOST pin
        value = value | B10000000;
        // and then set the high output power config with register REG_PA_DAC
        writeRegister(RegPaDacReg, 0x87);
        // set RegOcp for OcpOn and OcpTrim
        // 150mA
        setMaxCurrent(0x12);
    }
    else {
        // disable high power output in all other cases
        writeRegister(RegPaDacReg, 0x84);
    }

	// set MaxPower to 7 -> Pmax=10.8+0.6*MaxPower [dBm] = 15
	value = value | B01110000;

	// then Pout = Pmax-(15-_power[3:0]) if  PaSelect=0 (RFO pin for +14dBm)
	// so L=3dBm; H=7dBm; M=15dBm (but should be limited to 14dBm by RFO pin)

	// and Pout = 17-(15-_power[3:0]) if  PaSelect=1 (PA_BOOST pin for +14dBm)
	// when p=='X' for 20dBm, value is 0x0F and RegPaDacReg=0x87 so 20dBm is enabled

	writeRegister(REG_PA_CONFIG, value);

    _power=value;

    value = readRegister(REG_PA_CONFIG);

    if( value == _power )
    {
        state = 0;
    }
    else
    {
        state = 1;
    }

    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    delay(100);
    return state;
}

/*
 Function: Sets the packet length in the module.
*/
int8_t SX1276::setPacketLength()
{
    uint16_t length;

	length = _payloadlength + OFFSET_PAYLOADLENGTH;

    return setPacketLength(length);
}

/*
 Function: Sets the packet length in the module.
*/
int8_t SX1276::setPacketLength(uint8_t l)
{
    byte st0;
    byte value = 0x00;
    int8_t state = 2;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
    packet_sent.length = l;

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // Set LoRa Standby mode to write in registers
	writeRegister(REG_PAYLOAD_LENGTH_LORA, packet_sent.length);
	// Storing payload length in LoRa mode
	value = readRegister(REG_PAYLOAD_LENGTH_LORA);
    
    if( packet_sent.length == value )
    {
        state = 0;
    }
    else
    {
        state = 1;
    }

    writeRegister(REG_OP_MODE, st0);
    return state;
}


/*
 Function: Sets the node address in the module.
*/
int8_t SX1276::setNodeAddress(uint8_t addr)
{
    byte st0;
    byte value;
    uint8_t state = 2;

    if( addr > 255 )
    {
        state = -1;
    }
    else
    {
        // Saving node address
        _nodeAddress = addr;
        st0 = readRegister(REG_OP_MODE);	  // Save the previous status

		writeRegister(REG_OP_MODE, LORA_STANDBY_FSK_REGS_MODE);
        
        // Storing node and broadcast address
        writeRegister(REG_NODE_ADRS, addr);
        writeRegister(REG_BROADCAST_ADRS, BROADCAST_0);

        value = readRegister(REG_NODE_ADRS);
        writeRegister(REG_OP_MODE, st0);		// Getting back to previous status

        if( value == _nodeAddress )
        {
            state = 0;
        }
        else
        {
            state = 1;
        }
    }
    return state;
}

/*
 Function: Gets the SNR value in LoRa mode.
 */
int8_t SX1276::getSNR()
{	// getSNR exists only in LoRa mode
    int8_t state = 0;
    byte value;

	value = readRegister(REG_PKT_SNR_VALUE);
	if( value & 0x80 ) // The SNR sign bit is 1
	{
		// Invert and divide by 4
		value = ( ( ~value + 1 ) & 0xFF ) >> 2;
		_SNR = -value;
	}
	else
	{
		// Divide by 4
		_SNR = ( value & 0xFF ) >> 2;
	}
    return state;
}

/*
 Function: Gets the RSSI of the last packet received in LoRa mode.
 */
int16_t SX1276::getRSSIpacket()
{	// RSSIpacket only exists in LoRa
    int8_t state = 2;

	state = getSNR();
	if( state == 0 )
	{
		// added by C. Pham
		_RSSIpacket = readRegister(REG_PKT_RSSI_VALUE);

		if( _SNR < 0 )
		{
			_RSSIpacket = -OFFSET_RSSI + (double)_RSSIpacket + (double)_SNR*0.25;
			state = 0;
		}
		else
		{
			_RSSIpacket = -OFFSET_RSSI + (double)_RSSIpacket;
			//end
			state = 0;
		}
	}
    return state;
}



/*
 Function: Limits the current supply of the power amplifier, protecting battery chemistries.
*/
int8_t SX1276::setMaxCurrent(uint8_t rate)
{
    int8_t state = 2;
    byte st0;

    // Maximum rate value = 0x1B, because maximum current supply = 240 mA
    if (rate > 0x1B)
    {
        state = -1;
    }
    else
    {
        // Enable Over Current Protection
        rate |= B00100000;

        st0 = readRegister(REG_OP_MODE);	// Save the previous status
		writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Set LoRa Standby mode to write in registers
        writeRegister(REG_OCP, rate);		// Modifying maximum current supply
        writeRegister(REG_OP_MODE, st0);		// Getting back to previous status
        state = 0;
    }
    return state;
}


/*
 Function: It truncs the payload length if it is greater than 0xFF.
*/
uint8_t SX1276::truncPayload(uint16_t length16)
{
    uint8_t state = 0;

    if( length16 > MAX_PAYLOAD )
    {
        _payloadlength = MAX_PAYLOAD;
    }
    else
    {
        _payloadlength = (length16 & 0xFF);
    }
    return state;
}


/*
 Function: Configures the module to receive information.
*/
uint8_t SX1276::receive()
{
    uint8_t state = 1;

    // Initializing packet_received struct
    memset( &packet_received, 0x00, sizeof(packet_received) );

    // Set LowPnTxPllOff
    writeRegister(REG_PA_RAMP, 0x08);

    writeRegister(REG_LNA, LNA_MAX_GAIN);
    writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
    
    if (_spreadingFactor == SF_10 || _spreadingFactor == SF_11 || _spreadingFactor == SF_12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    
    writeRegister(REG_FIFO_RX_BYTE_ADDR, 0x00); // Setting current value of reception buffer pointer
	state = setPacketLength(MAX_LENGTH);	// With MAX_LENGTH gets all packets with length < MAX_LENGTH
	writeRegister(REG_OP_MODE, LORA_RX_MODE);  	  // LORA mode - Rx
    return state;
}



/*
 Function: Configures the module to receive information.
*/
uint8_t SX1276::receivePacketTimeout(uint16_t wait)
{
    uint8_t state = 2;
    uint8_t state_f = 2;

    state = receive();
    if( state == 0 )
    {
        if( availableData(wait) )
        {
            // If packet received, getPacket
            state_f = getPacket();
        }
        else
        {
            state_f = 1;
        }
    }
    else
    {
        state_f = state;
    }
    return state_f;
}

/*
 Function: If a packet is received, checks its destination.
*/
boolean	SX1276::availableData(uint16_t wait)
{
    byte value;
    byte header = 0;
    boolean forme = false;
    boolean	_hreceived = false;
    unsigned long previous;

    previous = millis();
   
	value = readRegister(REG_IRQ_FLAGS);
	// Wait to Valid Header interrupt
	while( (bitRead(value, 4) == 0) && (millis() - previous < (unsigned long)wait) )
	{
		yield();
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	} // end while (millis)

	if( bitRead(value, 4) == 1 )
	{ // header received
		_hreceived = true;

		// actually, need to wait until 3 bytes have been received
		while( (header < 3) && (millis() - previous < (unsigned long)wait) )
		{ // Waiting to read first payload bytes from packet
			yield();
			header = readRegister(REG_FIFO_RX_BYTE_ADDR);
			// Condition to avoid an overflow (DO NOT REMOVE)
			if( millis() < previous )
			{
				previous = millis();
			}
		}

		if( header != 0 )
		{ // Reading first byte of the received packet
			_the_net_key_0 = readRegister(REG_FIFO);
			_the_net_key_1 = readRegister(REG_FIFO);
			_destination = readRegister(REG_FIFO);
		}
	}
	else
	{
		forme = false;
		_hreceived = false;
	}
    // We use _hreceived because we need to ensure that _destination value is correctly
    // updated and is not the _destination value from the previously packet
    if( _hreceived == true )
    { // Checking destination
        forme=true;

		if (_the_net_key_0!=_my_netkey[0] || _the_net_key_1!=_my_netkey[1]) {
			forme=false;
		}
		else
		{
		}
			
        if( forme && ((_destination == _nodeAddress) || (_destination == BROADCAST_0)) )
		{ 
            forme = true;
        }
        else
        {
            forme = false;
            writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Setting standby LoRa mode
        }
    }
    return forme;
}

/*
 Function: It gets and stores a packet if it is received.
*/
int8_t SX1276::getPacket()
{
    return getPacket(MAX_TIMEOUT);
}

/*
 Function: It gets and stores a packet if it is received before ending 'wait' time.
*/
int8_t SX1276::getPacket(uint16_t wait)
{
    uint8_t state = 2;
    byte value = 0x00;
    unsigned long previous;
    boolean p_received = false;

    previous = millis();
	value = readRegister(REG_IRQ_FLAGS);
	// Wait until the packet is received (RxDone flag) or the timeout expires
	while( (bitRead(value, 6) == 0) && (millis() - previous < (unsigned long)wait) )
	{
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	} // end while (millis)

	if( (bitRead(value, 6) == 1) && (bitRead(value, 5) == 0) )
	{ // packet received & CRC correct
		p_received = true;	// packet correctly received
		_reception = CORRECT_PACKET;
	}
	else
	{
		if( bitRead(value, 5) != 0 )
		{ // CRC incorrect
			_reception = INCORRECT_PACKET;
			state = 3;
		}
	}
    if( p_received == true )
    {
        // Store the packet
		writeRegister(REG_FIFO_ADDR_PTR, 0x00);  	// Setting address pointer in FIFO data buffer

		packet_received.netkey[0]=readRegister(REG_FIFO);
		packet_received.netkey[1]=readRegister(REG_FIFO);
		packet_received.dst = readRegister(REG_FIFO);	// Storing first byte of the received packet		
		packet_received.type = readRegister(REG_FIFO);		// Reading second byte of the received packet
		packet_received.src = readRegister(REG_FIFO);		// Reading second byte of the received packet
		packet_received.packnum = readRegister(REG_FIFO);	// Reading third byte of the received packet
        packet_received.length = readRegister(REG_RX_NB_BYTES);
		_payloadlength = packet_received.length - OFFSET_PAYLOADLENGTH;

        if( packet_received.length > (MAX_LENGTH + 1) )
        {
        }
        else
        {
            for(unsigned int i = 0; i < _payloadlength; i++)
            {
                packet_received.data[i] = readRegister(REG_FIFO); // Storing payload
            }
            state = 0;
        }
    }
    else
    {
        state = 1;
        if( (_reception == INCORRECT_PACKET) && (_retries < _maxRetries) )
        {
            _retries++;
        }
    }
	writeRegister(REG_FIFO_ADDR_PTR, 0x00);  // Setting address pointer in FIFO data buffer
    
	clearFlags();	// Initializing flags
    if( wait > MAX_WAIT )
    {
        state = -1;
    }

    return state;
}

/*
 Function: It sets the packet destination.
*/
int8_t SX1276::setDestination(uint8_t dest)
{
    int8_t state = 0;
    _destination = dest; // Storing destination in a global variable
    packet_sent.dst = dest;	 // Setting destination in packet structure
    packet_sent.src = _nodeAddress; // Setting source in packet structure
    packet_sent.packnum = _packetNumber;	// Setting packet number in packet structure
    _packetNumber++;
    return state;
}

/*
 Function: It sets the network key
*/
void SX1276::setNetworkKey(uint8_t key0, uint8_t key1)
{
	_my_netkey[0] = key0;
    _my_netkey[1] = key1;
}

/*
 Function: It sets the timeout according to the configured mode.
*/
uint8_t SX1276::setTimeout()
{
    uint8_t state = 0;
    uint16_t delay;

	switch(_spreadingFactor)
	{	// Choosing Spreading Factor
	case SF_7:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 408;
				break;
			case CR_6: _sendTime = 438;
				break;
			case CR_7: _sendTime = 468;
				break;
			case CR_8: _sendTime = 497;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 325;
				break;
			case CR_6: _sendTime = 339;
				break;
			case CR_7: _sendTime = 355;
				break;
			case CR_8: _sendTime = 368;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 282;
				break;
			case CR_6: _sendTime = 290;
				break;
			case CR_7: _sendTime = 296;
				break;
			case CR_8: _sendTime = 305;
				break;
			}
			break;
		}
		break;

	case SF_8:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 537;
				break;
			case CR_6: _sendTime = 588;
				break;
			case CR_7: _sendTime = 640;
				break;
			case CR_8: _sendTime = 691;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 388;
				break;
			case CR_6: _sendTime = 415;
				break;
			case CR_7: _sendTime = 440;
				break;
			case CR_8: _sendTime = 466;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 315;
				break;
			case CR_6: _sendTime = 326;
				break;
			case CR_7: _sendTime = 340;
				break;
			case CR_8: _sendTime = 352;
				break;
			}
			break;
		}
		break;

	case SF_9:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 774;
				break;
			case CR_6: _sendTime = 864;
				break;
			case CR_7: _sendTime = 954;
				break;
			case CR_8: _sendTime = 1044;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 506;
				break;
			case CR_6: _sendTime = 552;
				break;
			case CR_7: _sendTime = 596;
				break;
			case CR_8: _sendTime = 642;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 374;
				break;
			case CR_6: _sendTime = 396;
				break;
			case CR_7: _sendTime = 418;
				break;
			case CR_8: _sendTime = 441;
				break;
			}
			break;
		}
		break;

	case SF_10:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1226;
				break;
			case CR_6: _sendTime = 1388;
				break;
			case CR_7: _sendTime = 1552;
				break;
			case CR_8: _sendTime = 1716;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 732;
				break;
			case CR_6: _sendTime = 815;
				break;
			case CR_7: _sendTime = 896;
				break;
			case CR_8: _sendTime = 977;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 486;
				break;
			case CR_6: _sendTime = 527;
				break;
			case CR_7: _sendTime = 567;
				break;
			case CR_8: _sendTime = 608;
				break;
			}
			break;
		}
		break;

	case SF_11:	switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 2375;
				break;
			case CR_6: _sendTime = 2735;
				break;
			case CR_7: _sendTime = 3095;
				break;
			case CR_8: _sendTime = 3456;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1144;
				break;
			case CR_6: _sendTime = 1291;
				break;
			case CR_7: _sendTime = 1437;
				break;
			case CR_8: _sendTime = 1586;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 691;
				break;
			case CR_6: _sendTime = 766;
				break;
			case CR_7: _sendTime = 838;
				break;
			case CR_8: _sendTime = 912;
				break;
			}
			break;
		}
		break;

	case SF_12: switch(_bandwidth)
		{	// Choosing bandwidth
		case BW_125:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 4180;
				break;
			case CR_6: _sendTime = 4836;
				break;
			case CR_7: _sendTime = 5491;
				break;
			case CR_8: _sendTime = 6146;
				break;
			}
			break;
		case BW_250:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1965;
				break;
			case CR_6: _sendTime = 2244;
				break;
			case CR_7: _sendTime = 2521;
				break;
			case CR_8: _sendTime = 2800;
				break;
			}
			break;
		case BW_500:
			switch(_codingRate)
			{	// Choosing coding rate
			case CR_5: _sendTime = 1102;
				break;
			case CR_6: _sendTime = 1241;
				break;
			case CR_7: _sendTime = 1381;
				break;
			case CR_8: _sendTime = 1520;
				break;
			}
			break;
		}
		break;
	default: _sendTime = MAX_TIMEOUT;
	}
    delay = ((0.1*_sendTime) + 1);
   _sendTime = (uint16_t) ((_sendTime * 1.2) + (rand()%delay));
    return state;
}

/*
 Function: It sets an uint8_t array payload packet in a packet struct.
*/
uint8_t SX1276::setPayload(uint8_t *payload)
{
    uint8_t state = 1;
    
	for(unsigned int i = 0; i < _payloadlength; i++)
    {
        packet_sent.data[i] = payload[i];	// Storing payload in packet structure
    }
    // set length with the actual counter value
    state = setPacketLength();	// Setting packet length in packet structure
    return state;
}

/*
 Function: It sets a packet struct in FIFO in order to sent it.
*/
uint8_t SX1276::setPacket(uint8_t dest, uint8_t *payload)
{
    int8_t state = 2;
    byte st0;

    st0 = readRegister(REG_OP_MODE);	// Save the previous status
    clearFlags();	// Initializing flags

	writeRegister(REG_OP_MODE, LORA_STANDBY_MODE);	// Stdby LoRa mode to write in FIFO
    
    _reception = CORRECT_PACKET;	// Updating incorrect value to send a packet (old or new)
    if( _retries == 0 )
    { // Sending new packet
        state = setDestination(dest);	// Setting destination in packet structure
        packet_sent.retry = _retries;
        if( state == 0 )
        {
            state = setPayload(payload);
        }
    }
    else
    {
        if( _retries == 1 )
        {
            packet_sent.length++;
        }
        state = setPacketLength();
        packet_sent.retry = _retries;
    }

    packet_sent.type |= PKT_TYPE_DATA;

    writeRegister(REG_FIFO_ADDR_PTR, 0x80);  // Setting address pointer in FIFO data buffer
    if( state == 0 )
    {
        state = 1;
        // Writing packet to send in FIFO
        packet_sent.netkey[0]=_my_netkey[0];
        packet_sent.netkey[1]=_my_netkey[1];
        writeRegister(REG_FIFO, packet_sent.netkey[0]);
        writeRegister(REG_FIFO, packet_sent.netkey[1]);

		writeRegister(REG_FIFO, packet_sent.dst); 		// Writing the destination in FIFO
		writeRegister(REG_FIFO, packet_sent.type); 		// Writing the packet type in FIFO
		writeRegister(REG_FIFO, packet_sent.src);		// Writing the source in FIFO
		writeRegister(REG_FIFO, packet_sent.packnum);	// Writing the packet number in FIFO

        for(unsigned int i = 0; i < _payloadlength; i++)
        {
            writeRegister(REG_FIFO, packet_sent.data[i]);  // Writing the payload in FIFO
        }
        state = 0;
    }
    writeRegister(REG_OP_MODE, st0);	// Getting back to previous status
    return state;
}


/*
 Function: Configures the module to transmit information.
*/
uint8_t SX1276::sendWithTimeout()
{
    setTimeout();
    return sendWithTimeout(_sendTime);
}

/*
 Function: Configures the module to transmit information.
*/
uint8_t SX1276::sendWithTimeout(uint16_t wait)
{
    uint8_t state = 2;
    byte value = 0x00;
    unsigned long previous;

    // wait to TxDone flag
    previous = millis();
    
    clearFlags();	// Initializing flags

	writeRegister(REG_OP_MODE, LORA_TX_MODE);  // LORA mode - Tx

	value = readRegister(REG_IRQ_FLAGS);
	// Wait until the packet is sent (TX Done flag) or the timeout expires
	while ((bitRead(value, 3) == 0) && (millis() - previous < wait))
	{
		value = readRegister(REG_IRQ_FLAGS);
		// Condition to avoid an overflow (DO NOT REMOVE)
		if( millis() < previous )
		{
			previous = millis();
		}
	}
	state = 1;

    if( bitRead(value, 3) == 1 )
    {
        state = 0;	// Packet successfully sent
    }
    clearFlags();		// Initializing flags
    return state;
}

/*
 Function: Configures the module to transmit information.
*/
uint8_t SX1276::sendPacketTimeout(uint8_t dest, uint8_t *payload, uint16_t length16)
{
    uint8_t state = 2;
    uint8_t state_f = 2;

    state = truncPayload(length16);
    if( state == 0 )
    {
        state_f = setPacket(dest, payload);	// Setting a packet with 'dest' destination
    }												// and writing it in FIFO.
    else
    {
        state_f = state;
    }
    if( state_f == 0 )
    {
        state_f = sendWithTimeout();	// Sending the packet
    }
    return state_f;
}

SX1276 sx1276 = SX1276();
