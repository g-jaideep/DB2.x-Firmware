#include "assert.h"
#include <Arduino.h>
#include "ads1256.h"
#include "ads1256_regmap.h"
#include <SPI.h>
#include <stdint.h>

static const uint8_t RESET   = 6;
static const uint8_t DRDY    = 5;
static const uint8_t CS      = 10;

static const uint32_t CLKSPEED = 1000000;   // 1 MHz 
static const uint8_t CHIP_ID = 3;

static void wait_DRDY()
{
    while(digitalRead(DRDY));
}

static void send_command(uint8_t cmd)
{
    SPI.beginTransaction(SPISettings(CLKSPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS, LOW);
    SPI.transfer(cmd);
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
}

static void write_register(uint8_t reg, uint8_t data)
{
    SPI.beginTransaction(SPISettings(CLKSPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS, LOW);
    SPI.transfer(WREG | reg);   // 1st command byte (register address)
    delayMicroseconds(10);
    SPI.transfer(data);         // 2nd command byte (no.registers to read == 1)
    delayMicroseconds(10);
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
}

static uint8_t read_register(uint8_t reg)
{
    SPI.beginTransaction(SPISettings(CLKSPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS, LOW);
    SPI.transfer(RREG | reg);   // 1st command byte (register address)
    SPI.transfer(0x00);         // 2nd command byte (no.registers to read == 1)
    delayMicroseconds(10);      // t6 delay @datasheet Fig1. Serial Interface Timing
    uint8_t rx_data = SPI.transfer(NOP);
    delayMicroseconds(2);      // t11 delay
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
    return rx_data;
}

void ADS1256::begin()
{
    // Initialize pins
    pinMode(RESET, OUTPUT);
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);     // De-assert CS line
    digitalWrite(RESET, HIGH);  // De-assert RESET line
    
    reset();
    assert(read_id() == CHIP_ID);

    send_command(SDATAC);       // stop continuous reading mode
}

void ADS1256::reset()
{
    digitalWrite(RESET, LOW);
    delayMicroseconds(100);     // t16 @datasheet Fig3. RESET and SYNC/PDWN Timing     
    digitalWrite(RESET, HIGH);
    delay(30);                  // TODO - how to know exact timing?
}

int ADS1256::read_id()
{
    uint8_t id = read_register(STATUS) >> 4;
    return id;
}

long ADS1256::read_channel()
{
    while(digitalRead(DRDY));       // Wait for nDRDY
    send_command(RDATA);
    delayMicroseconds(10);          // t6 @datasheet RDATA command
    uint8_t high=0, mid=0, low=0;
    high = SPI.transfer(NOP);
    mid = SPI.transfer(NOP);
    low = SPI.transfer(NOP);
    uint32_t value = ((uint32_t)high << 16) + ((uint32_t)mid << 8) + ((uint32_t)low);
    return value;
}
