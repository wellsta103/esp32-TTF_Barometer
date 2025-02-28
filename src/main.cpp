#include <Arduino.h>

/*
Title: ESP32 Nautical Barometer Gold
License: CC BY-SA 4.0
https://creativecommons.org/licenses/by-sa/4.0/

Source code repository: https://github.com/DebinixTeam/esp32-nautical-barometer-gold
Actual ESP32, BME280 sensor, and 'adpter board' compatibility:
https://github.com/DebinixTeam/esp32-nautical-barometer-gold/blob/master/hw-compatibility-list.md

Tested BME280 sensor: Pomoroni I2C BME280 sensor
Display: SPI 2.8" TFT SPI 240x320 v1.2 (SKU:MSP2807) www.lcdwiki.com
Code Editor: Arduino IDE 1.8.15
Default serial baudrate: 9600
Other settings:
  IDE->Tools 'Upload Speed: 921 600'
  IDE->Tools 'Boards: 'ESP32 Dev Module (30pin)', 'TinyPICO v1' (20pin),
             'Sparkfun ESP32 Thing Plus* (12pin+16pin)', DOIT ESP32 DEVKIT V1(36pin)'.

Breadboard Wiring (IO aka GPIO)
-------------------------------
IO#Name I2C-Wiring ESP32 <--> Pomoroni BME280 I2C

IO21/SDA     -> BME280 SDA
IO22/SCL     -> BME280 SCL

IO#Name SPI-Wiring ESP32 <--> Display 2.8" SPI

IO27/CS      -> TFT_SPI #3/CS
IO4/RST      -> TFT_SPI #4/RESET
IO26/DC      -> TFT_SPI #5/DC
IO23/MOSI    -> TFT_SPI #6/MOSI
IO18/SCK     -> TFT_SPI #7/SCK

*) Change in 'TFT_eSPI/User_Setup.h' for 'Sparkfun ESP32 Thing Plus'
(non-standard Arduino SPI) as:
IO18/MOSI    -> TFT_SPI #6/MOSI
IO5/SCK      -> TFT_SPI #7/SCK

IO#Name Power Wiring ESP32 (3V3) <--> TFT_SPI & BME280

3V3         -> TFT_SPI #1/VCC
3V3         -> TFT_SPI #8/LED
3V3         -> BME280 VCC/VIN/3V3

GND         -> TFT_SPI #2/GND
GND         -> BME280 GND

  ##################################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE TFT_eSPI LIBRARY ######
  ######          FULL DESCRIPTION OF THE PROJECT AT INSTRUCTABLES.COM        ######
  ##################################################################################
*/

//===========================================
// Includes
//===========================================

#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <TFT_eSPI.h> // Display hardware-specific library
#include <SPI.h>
#include <ESP32Time.h>

// #define BME280
#define BMP280

#ifdef BME280
#include <BME280.h>
BME280_Class BME280;
#else
#include <Adafruit_BMP280.h>
Adafruit_BMP280 BME280;
#endif

//===========================================
// Defines
//===========================================

#define GFXFF 1
#define FF18 &FreeSans12pt7b
#define CF_OL24 &Orbitron_Light_24
#define TFT_GREY 0x5AEB

#define MINPRESSURE 997    // hPa/mbar
#define MAXPRESSURE 1036   // hPa/mbar
#define MAXHOURTIMESLOT 11 // Number of possible one hour time slots

#define FAHRENHEIT 1 // '0' for temperature in degree Celsius, '1' for degree Fahrenheit
#define HEIGHT 162   // Height in Meters

//===========================================
// Debug code, set MYDEBUG to 1
//===========================================

#define MYDEBUG 0

#if MYDEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

//===========================================
// External prototype declarations
//===========================================

void setup_humidity_meter(void);
void update_humidity_needle(int value, int tempvalue, byte ms_delay, int16_t p_min, int16_t p_max);
void setup_pressure_scales(const char *label, int x, int y);
void update_pressure_arrows(void);
char *pressure_diff_to_1013(int value);

int16_t *update_pressure_array(int16_t pressure_now);
int16_t *map_pressure_values(int16_t *pressure_array);
int16_t is_outside_range(int16_t *pressure_array);

void debug_sensor_bme280(int32_t temp, int32_t humidity, int32_t pressure, int16_t rtc_minute, int16_t rtc_second);

//===========================================
// Global instantiation
//===========================================

ESP32Time rtc;
TFT_eSPI tft = TFT_eSPI();

int16_t pressure_array[MAXHOURTIMESLOT] = {0};
// BME280_Class BME280;
uint16_t osx = 120, osy = 120; // Saved x & y coords
uint16_t last_hour = 0;
int old_analog = -999;  // Value last displayed
int old_digital = -999; // Value last displayed
int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = {-1, -1, -1, -1, -1, -1};
int d = 0;
int16_t do_update_flag = 1; // Initially true for 'now' reading

int32_t pressure_max = 5, pressure_min = 200000;

//===========================================
// In file prototypes
//===========================================

int16_t one_minute_done(void);
int16_t one_hour_done(void);

#include "humidity-scale.h"
#include "pressure-data.h"
#include "pressure-scale.h"
#include "debug.h"

// #########################################################################
// ######                           SETUP                             ######
// #########################################################################

void setup(void)
{

    // RTC as EPOCH date/time like 1st Jan 1970 00:00:00,
    // only minute and hour transition 0 -> 1 is used
    rtc.setTime(0, 0, 0, 1, 1, 1970);

    Serial.begin(115200);
    while (!Serial)
        ;

    // while (!BME280.begin(I2C_STANDARD_MODE)) {
#ifdef BME280
    while (!BME280.begin(I2C_FAST_MODE_PLUS_MODE))
    {
        Serial.print(F("-  Unable to find BME280 on I2C. Press reset, or check connections, and try again in 5 seconds.\n"));
        delay(5000);
    }
    BME280.mode(NormalMode);
    BME280.setOversampling(TemperatureSensor, Oversample16);
    BME280.setOversampling(HumiditySensor, Oversample16);
    BME280.setOversampling(PressureSensor, Oversample16);
    BME280.iirFilter(IIR16);
    // BME280.setIIRFilter(IIR4);
    // BME280.setGas(0, 0);        // 0,0 means no heated gas measurements
    BME280.inactiveTime(inactive1000ms);

    debug(F("- Setting 16x oversampling for all sensors\n"));
    debug(F("- Setting IIR filter to a value of 4 samples\n"));
    debug(F("\nTemp [C] Humid [RH%] Press [hPa/mbar] [min  sec]"));
    debug(F("\n================================================\n"));
#else
    while (!BME280.begin(BMP280_ADDRESS_ALT))
    {
        BME280.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                           Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                           Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                           Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                           Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
    }
#endif

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    do_update_flag = 1; // Initially set to true to get first reading

    setup_humidity_meter(); // Draw the main analogue meter

    // Draw six pressure indicators
    byte d = 40;
    setup_pressure_scales("-10h", 0, 160);
    setup_pressure_scales("-8h", 1 * d, 160);
    setup_pressure_scales("-6h", 2 * d, 160);
    setup_pressure_scales("-3h", 3 * d, 160);
    setup_pressure_scales("-1h", 4 * d, 160);
    setup_pressure_scales("Now", 5 * d, 160);

    debug(F("Setup done"));
}

// #########################################################################
// ######                           LOOP                              ######
// #########################################################################

void loop()
{

    int32_t temp = 0, humidity = 0, pressure = 0, gas = 0;
    char bufpres[20] = ""; // sprintf text buffer
    float fpres;
    uint8_t dpres;
    int16_t array_cnt = 0;
    double Pressure2, Pressure2plus;
    double myT, myP;

    delay(5000); // Wait 5 s before acquiring the new BME280 environmental data

    // Throw away first reading, I2C/BME280 garbage
    if (do_update_flag == 1)
    {
#ifdef BME280
        BME280.getSensorData(temp, humidity, pressure);
#else
        temp = BME280.readTemperature();
        pressure = BME280.readPressure() - 200.0; // -2.0 mb Correction
        humidity = 12.3;
#endif
        delay(1000);
    }
#ifdef BME280
    BME280.getSensorData(temp, humidity, pressure); // Get real data, ignore gas value
    if (pressure < 1000)
    {
        delay(1000);
        BME280.getSensorData(temp, humidity, pressure); // Get real data, ignore gas value
        if (pressure < 1000)
        {
            Serial.printf("Retry Succeeded");
        }
        else
        {
            Serial.printf("Retry Failed");
        }
    }
#else
    temp = BME280.readTemperature();
    pressure = BME280.readPressure() - 200.0; // -2.0 mb Correction
    humidity = 12.3 * 100.0;
#endif
    myP = double(pressure) / 100.0;
    myT = double(temp) / 100.0;

    // Adjust pressure back to SeaLevel Pressure based on current elevation, Height in meters

    Pressure2 = myP;
    Pressure2plus = -Pressure2 + Pressure2 / pow((1.0 - (0.0065 * double(HEIGHT) / (myT + 0.0065 * double(HEIGHT) + 273.15))), 5.25588);
    Serial.printf("TAW: %.3f %.3f %.3f", Pressure2, Pressure2plus, Pressure2 + Pressure2plus);
    Pressure2 = Pressure2 + Pressure2plus;
    pressure = int(Pressure2 * 100.0);
    Serial.printf(" %.3d ", pressure);

    fpres = Pressure2;

    debug_sensor_bme280(temp, humidity, pressure, rtc.getMinute(), rtc.getSecond());

//
// Check if it's time to update values, once every hour (every minute for MYDEBUG)
//
#if MYDEBUG == 1
    if (one_minute_done() || do_update_flag)
    {                                     // Shift barometric scale pointers every minute in DEBUG mode
        rtc.setTime(0, 0, 0, 1, 1, 1970); // Ensure that we only does a barometric scale pointers one per minute update.
#else
    if (one_hour_done() || do_update_flag)
    {                                     // Shift barometric scale pointers once in an hour in REAL mode
        rtc.setTime(0, 0, 0, 1, 1, 1970); // Ensure that we only does a barometric scale pointers one per hour update.
#endif

        int16_t *p_pressure;    // Raw BME280 sensor pressure values (hPa aka mbar)
        int16_t *p_metervalues; // Pressure values mapped in the range [0,100], to fit meter scale

        p_pressure = update_pressure_array(fpres);       // First 'Now'-pressure is added to the pressure array
        p_metervalues = map_pressure_values(p_pressure); // Retrieve mapped values for meter usage

        // You can select different time slots than default, up to (MAXHOURTIMESLOT-1)
        value[0] = p_metervalues[10]; // -10 hour ago pressure
        value[1] = p_metervalues[8];  // -8 hour ago pressure
        value[2] = p_metervalues[6];  // -6 hour ago pressure
        value[3] = p_metervalues[3];  // -3 hour ago pressure
        value[4] = p_metervalues[1];  // -1 hour ago pressure
        value[5] = p_metervalues[0];  //  Now-pressure

        //
        // Draw the six analog scales with the barometric history
        //
        update_pressure_arrows();
    } // end-if

    if (pressure > pressure_max)
        pressure_max = pressure;
    if (pressure < pressure_min)
        pressure_min = pressure;
    //
    // Draw the humidity needle, top of screen, including the temperature reading
    //
    update_humidity_needle((int8_t)(humidity / 100), (int8_t)(temp / 100), 0,
                           (int16_t)(pressure_min / 100), (int16_t)(pressure_max / 100));

    Serial.printf("TAW2: %4d, %4d\n", pressure_min, pressure_max);

    //
    // Print pressure value with two decimals
    //
    tft.setTextPadding(tft.width());
    tft.setTextDatum(TL_DATUM);
    if (fpres > MAXPRESSURE)
    {
        sprintf(bufpres, "++ %8.2f mb", fpres); // Indicating now-value is outside MAXPRESSURE range
    }
    else if (fpres < MINPRESSURE)
    {
        sprintf(bufpres, "-- %8.2f mb", fpres); // Indicating now-value is outside MINPRESSURE range
    }
    else
    {
        sprintf(bufpres, "  %8.2f mb", fpres); // Pressure hPascals=mbar
    }

    Serial.println(bufpres);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont(CF_OL24);                // Select the font
    tft.drawString(bufpres, 15, 128, GFXFF); // Print the mb value

    // Reset text padding to zero (default)
    tft.setTextPadding(0);

    do_update_flag = 0; // No need for flag after initial first BME280 reading
}

//====================================================
// one_minute_done: Returns 'true'/1, on the new
// minute shift, otherwise returns 'false'/0.
//====================================================
int16_t one_minute_done(void)
{

    if (rtc.getMinute() > 0)
    {
        return 1;
    }
    return 0;
}
//====================================================
// one_hour_done: Returns 'true'/1, on the new
// hour shift, otherwise returns 'false'/0.
//====================================================
int16_t one_hour_done(void)
{

    if (rtc.getHour() > 0)
    {
        return 1;
    }
    return 0;
}
