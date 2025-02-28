
#if MYDEBUG == 1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MINPRESSURE 997  // hPa/mbar
#define MAXPRESSURE 1036 // hPa/mbar
#define MAXHOURTIMESLOT 11

int16_t *update_pressure_array(int16_t pressure_now);
int16_t *map_pressure_values(int16_t *pressure_array);
int16_t is_outside_range(int16_t *pressure_array);
#endif

//====================================================
// update_pressure_array: Returns an updated pressure
// reading in an MAXHOURTIMESLOT long array.
//====================================================
int16_t *update_pressure_array(int16_t pressure_now)
{
#if MYDEBUG == 1
// static int16_t pressure_data[11] = {1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018};    // All values in [1008, 1018]
// static int16_t pressure_data[11] = {1008, 1009, 1007, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018};    // 1007 is outside range - Under pressure
// static int16_t pressure_data[11] = {1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1019, 1018};    // 1019 is outside range - Over pressure
// static int16_t pressure_data[11] = {1008, 1009, 1010, 1002, 1012, 1013, 1014, 1020, 1016, 1017, 1018};    // 1002 and 1020 are outside range, total range exceeds 10 mb
// static int16_t pressure_data[11] = {1007, 1007, 1005, 1011, 1012, 1013, 1014, 1020, 1018, 1018, 1018};    // Multiple various values in and out [1008, 1018]
// static int16_t pressure_data[11] = {1013, 1013, 1013, 1013, 1013, 1013, 1013, 1013, 1013, 1013, 1013};
#endif

    // Default pressure array at power start up
    // static int16_t pressure_data[MAXHOURTIMESLOT] = {MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE, MINPRESSURE};
    // static int16_t pressure_data[11] = {999, 1002, 1005, 1008, 1011, 1014, 1017, 1020, 1023, 1026, 1029};
    static int16_t pressure_data[11] = {999, 1003, 1006, 1010, 1014, 1018, 1021, 1025, 1029, 1032, 1036};

    // Initial setup, shift out all previous pressure elements by one hour and add a new 'Now' pressure (at 0h) in pressure_data[0] position
    for (int8_t i = MAXHOURTIMESLOT - 1; i > 0; i--)
    {
        pressure_data[i] = pressure_data[i - 1];
    }
    pressure_data[0] = pressure_now;

    return pressure_data;
}
int16_t fun1(int16_t input)
{
    static int16_t range_min = MINPRESSURE;
    static int16_t range_max = MAXPRESSURE;
    static int16_t range_len = 100;
    static float range_delta = (range_max - range_min) / float(range_len);
    int16_t output;

    Serial.printf("TAW6 %f\n", range_delta);
    // output = 100 - ((input - range_min) / range_delta);
    output = ((input - range_min) / range_delta);
    Serial.printf("TAW7 %d  %d\n", input, output);
    // output = output + 10;
    // output = 50;

    return output;
}
//====================================================
// map_pressure_values: Map actual pressure values in the
// a pressure range [1003,1036] 'mbar' to fit scale.
//====================================================
int16_t *map_pressure_values(int16_t *pressure_array)
{

    static int16_t meter_data[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int8_t over_pressure = 0;
    int8_t under_pressure = 0; // False
    int16_t my_r;

    for (int8_t i = 0; i < MAXHOURTIMESLOT; i++)
    {

        my_r = fun1(pressure_array[i]);
        meter_data[i] = my_r;
        Serial.printf("TAW4 %d  %d\n", pressure_array[i], my_r);
    }

    return meter_data;
}

//====================================================
// is_outside_range: Verify if all pressure values
// will fit on the meter scale (returns 0), or if any
// single value is either to high or to low, and then
// teturns 999 or -999 respectively. Returns 0 if
// values span is greater than 2023 - 1003 = 20 'mbar'.
//====================================================

int16_t is_outside_range(int16_t *pressure_array)
{

    int8_t over_pressure_flag = 0;  // false
    int8_t under_pressure_flag = 0; // false

    // Return the outside pressure element value as -999 or 999 or 0 for either,
    // all elements in range, or find elements with over- and under-pressure at the same time.
    for (int8_t i = 0; i < MAXHOURTIMESLOT; i++)
    {

        if (pressure_array[i] > MAXPRESSURE)
        {
            over_pressure_flag = 1;
        }
        if (pressure_array[i] < MINPRESSURE)
        {
            under_pressure_flag = 1;
        }
    }

    if (over_pressure_flag && under_pressure_flag)
    {
#if MYDEBUG == 1
        printf("\nis_outside_range(Detected both an over- and under pressure)");
#endif
        // NOP, cannot do anything if pressure values exceeds available range span
        return 0;
    }
    else if (over_pressure_flag || under_pressure_flag)
    {
        if (over_pressure_flag)
        {
#if MYDEBUG == 1
            printf("\nis_outside_range(Detected over pressure value)");
#endif
            return 999;
        }
        if (under_pressure_flag)
        {
#if MYDEBUG == 1
            printf("\nis_outside_range(Detected under pressure value)");
#endif
            return -999;
        }
    }

    return 0; // All values are mappable within pressure range
}
