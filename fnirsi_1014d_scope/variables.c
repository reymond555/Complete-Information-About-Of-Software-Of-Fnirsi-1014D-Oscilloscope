//----------------------------------------------------------------------------------------------------------------------------------

#include "variables.h"

//----------------------------------------------------------------------------------------------------------------------------------
//Timer data
//----------------------------------------------------------------------------------------------------------------------------------

volatile uint32 timer0ticks;

//----------------------------------------------------------------------------------------------------------------------------------
//State machine data
//----------------------------------------------------------------------------------------------------------------------------------

int16 menuitem = 0;

int8  speedvalue = 10;
int8  setvalue   = 1;

uint32 previoustimerticks = 0;

uint8 lastreceivedcommand = 0;
uint8 toprocesscommand = 0;

uint8 navigationstate = 0;
uint8 fileviewstate = 0;
uint8 buttondialstate = 0;

uint8 enablesampling = 1;
uint8 enabletracedisplay = 1;

int8 *sliderdata = 0;

uint8  onoffhighlighteditem = 0;
uint8 *onoffdata = 0;

uint8 measurementslot = -1;

PCHANNELSETTINGS currentsettings = 0;

//----------------------------------------------------------------------------------------------------------------------------------
//Display data
//----------------------------------------------------------------------------------------------------------------------------------

//This first buffer is defined as 32 bits to be able to write it to file
uint32 maindisplaybuffer[SCREEN_SIZE / 2];

uint16 displaybuffer1[SCREEN_SIZE];
uint16 displaybuffer2[SCREEN_SIZE];

uint16 gradientbuffer[SCREEN_HEIGHT];

//Global buffer for formating text in
char globaldisplaytext[50];

//----------------------------------------------------------------------------------------------------------------------------------
//Scope data
//----------------------------------------------------------------------------------------------------------------------------------

FATFS fs;

SCOPESETTINGS scopesettings;

CHANNELSETTINGS calibrationsettings;

SCOPESETTINGS savedscopesettings1;
SCOPESETTINGS savedscopesettings2;

uint32 channel1tracebuffer[UINT32_SAMPLE_BUFFER_SIZE];

DISPLAYPOINTS channel1pointsbuffer[730];      //Buffer to store the x,y positions of the trace on the display

uint32 channel2tracebuffer[UINT32_SAMPLE_BUFFER_SIZE];

DISPLAYPOINTS channel2pointsbuffer[730];      //Buffer to store the x,y positions of the trace on the display

uint16 thumbnailtracedata[730];

uint16 settingsworkbuffer[256];               //Used for loading from and writing the settings to the SD card

//New variables for trace displaying

double disp_xpos_per_sample;
double disp_sample_step;
double disp_xrange;

int32 disp_first_sample;

int32 disp_trigger_index;            //Trigger point in the sample buffers

int32 disp_xstart;
int32 disp_xend;

int32 trigger_position_min;
int32 trigger_position_max;

//----------------------------------------------------------------------------------------------------------------------------------
//Data for picture and waveform view mode
//----------------------------------------------------------------------------------------------------------------------------------

FIL     viewfp;                         //Since files are not opened concurrent using a global file pointer
DIR     viewdir;
FILINFO viewfileinfo;
  
char viewfilename[32];              //The original code uses a large buffer to create all the needed file names in. Here the file name is created when needed

uint8 viewactive;                   //Not in the original code. Used to avoid copying settings to the flash

uint8 viewtype = VIEW_TYPE_PICTURE;
uint8 viewselectmode;

int16 viewpage;                     //Current page
int16 viewpages;                    //Number of pages to scroll through

uint8 viewitemsonpage;              //Max 16 or less when fewer items available


int16 viewcurrentindex;             //Used for selecting previous or next item when viewing an item

int16 viewavailableitems;           //Total available items for the current view mode

uint8 viewitemselected[VIEW_ITEMS_PER_PAGE];                 //Flags to signal if an item is selected or not

THUMBNAILDATA viewthumbnaildata[VIEW_MAX_ITEMS];

uint16 viewfilenumberdata[VIEW_MAX_ITEMS];

uint8 viewbitmapheader[PICTURE_HEADER_SIZE];

uint32 viewfilesetupdata[VIEW_NUMBER_OF_SETTINGS];

//----------------------------------------------------------------------------------------------------------------------------------
//Calibration data
//----------------------------------------------------------------------------------------------------------------------------------

uint32 samplerateindex;

//Average data for calibration calculations
uint32 samplerateaverage[2][6];

//Single ADC bit dc offset step per input sensitivity setting
uint32 sampleratedcoffsetstep[2][6];

//----------------------------------------------------------------------------------------------------------------------------------
//Predefined data
//----------------------------------------------------------------------------------------------------------------------------------

//Instead of a double time per division value using the 1 / "time per division" frequency value

//For calculations on time per div and sampling rate settings translation tables are used
const uint32 frequency_per_div[24] =
{
         5,        10,        20,
        50,       100,       200,
       500,      1000,      2000,
      5000,     10000,     20000,
     50000,    100000,    200000,
    500000,   1000000,   2000000,
   5000000,  10000000,  20000000,
  50000000, 100000000, 200000000
};

const uint32 sample_rate[18] =
{
  200000000,
  100000000,
   50000000,
   20000000,
   10000000,
    5000000,
    2000000,
    1000000,
     500000,
     200000,
     100000,
      50000,
      20000,
      10000,
       5000,
       2000,
       1000,
        500
};

//----------------------------------------------------------------------------------------------------------------------------------
//Translation tables for getting the sample rate belonging to a time per div setting and vise versa

const uint8 time_per_div_sample_rate[24] =
{
  17, 16, 15,     //200ms/div, 100ms/div and 50ms/div  use the 2KSa/s setting
  14, 13, 12,     //20ms/div, 10ms/div, 5ms/div        use their respective settings
  11, 10,  9,     //2ms/div, 1ms/div, 500us/div        use their respective settings
   8,  7,  6,     //200us/div, 100us/div, 50us/div     use their respective settings
   5,  4,  3,     //20us/div, 10us/div, 5us/div        use their respective settings
   2,  1,  0,     //2us/div, 1us/div, 500ns/div        use their respective settings
   0,  0,  0,     //200ns/div, 100ns/div, 50ns/div     use the 200MSa/s setting
   0,  0,  0      //20ns/div, 10ns/div, 5ns/div        use the 200MSa/s setting
};

const uint8 sample_rate_time_per_div[18] =
{
  17,       //200MSa/s ==> 500ns/div
  16,       //100MSa/s ==>   1us/div
  15,       // 50MSa/s ==>   2us/div
  14,       // 20MSa/s ==>   5us/div
  13,       // 10MSa/s ==>  10us/div
  12,       //  5MSa/s ==>  20us/div
  11,       //  2MSa/s ==>  50us/div
  10,       //  1MSa/s ==> 100us/div
   9,       //500KSa/s ==> 200us/div
   8,       //200KSa/s ==> 500us/div
   7,       //100KSa/s ==>   1ms/div
   6,       // 50KSa/s ==>   2ms/div
   5,       // 20KSa/s ==>   5ms/div
   4,       // 10KSa/s ==>  10ms/div
   3,       //  5KSa/s ==>  20ms/div
   2,       //  2KSa/s ==>  50ms/div
   1,       //  1KSa/s ==> 100ms/div
   0        // 500Sa/s ==> 200ms/div
};

const uint8 viable_time_per_div[18][24] =
{
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

//----------------------------------------------------------------------------------------------------------------------------------

const char *time_div_texts[24] =
{
  "200ms", "100ms",  "50ms",
   "20ms",  "10ms",   "5ms",
    "2ms",   "1ms", "500us",
  "200us", "100us",  "50us",
   "20us",  "10us",   "5us",
    "2us",   "1us", "500ns",
  "200ns", "100ns",  "50ns",
   "20ns",  "10ns",   "5ns"
};

const int8 time_div_text_x_offsets[24] =
{
  13, 13, 20, 20,
  20, 27, 27, 27,
  17, 17, 17, 24,
  24, 24, 31, 31,
  31, 17, 17, 17,
  24, 24, 24, 31,
};

//----------------------------------------------------------------------------------------------------------------------------------

const char *acquisition_speed_texts[18] =
{
  "200MSa/s",
  "100MSa/s",
   "50MSa/s",
   "20MSa/s",
   "10MSa/s",
    "5MSa/s",
    "2MSa/s",
    "1MSa/s",
  "500KSa/s",
  "200KSa/s",
  "100KSa/s",
   "50KSa/s",
   "20KSa/s",
   "10KSa/s",
    "5KSa/s",
    "2KSa/s",
    "1KSa/s",
   "500Sa/s",
};

const int8 acquisition_speed_text_x_offsets[18] =
{
  16, 16, 23, 23,
  23, 30, 30, 30,
  19, 19, 19, 26,
  26, 26, 33, 33,
  33, 26
};

//----------------------------------------------------------------------------------------------------------------------------------

const char *volt_div_texts[3][7] =
{
  { "5V", "2.5V", "1V", "500mV", "200mV", "100mV", "50mV" },
  { "50V", "25V", "10V", "5V", "2V", "1V", "500mV" },
  { "500V", "250V", "100V", "50V", "20V", "10V", "5V" }
};

//----------------------------------------------------------------------------------------------------------------------------------
//HW means done in hardware
//SW means done in software
//                                       HW       HW       HW       HW       HW       HW        SW
//                                       5V     2.5V       1V    500mV    200mV    100mV      50mV
//const int32 signal_adjusters[7] = { 7258042, 7341950, 7551720, 7551720, 7719536, 7719536, 15439072 };
const int32 signal_adjusters[7] = { 3629021, 3670975, 3775860, 3775860, 3859768, 3859768, 7719536 };

const uint32 sample_rate_settings[18] =
{
         0,     //200MSa/s
         1,     //100MSa/s
         3,     // 50MSa/s
         9,     // 20MSa/s
        19,     // 10MSa/s
        39,     //  5MSa/s
        99,     //  2MSa/s
       199,     //  1MSa/s
       399,     //500KSa/s
       999,     //200KSa/s
      1999,     //100KSa/s
      3999,     // 50KSa/s
      9999,     // 20KSa/s
     19999,     // 10KSa/s
     39999,     //  5KSa/s
     99999,     //  2KSa/s
    199999,     //  1KSa/s
    399999,     // 500Sa/s
};

//For the conversion of the period time calculated in the sample acquisition loop a sample rate based factor is needed.
//Based on a minimum of 2.5 periods on the screen the conversion yields a time per division number, that needs to be matched
//with the nearest possible setting. The input is scaled up with a factor 1048576, so an extra division is done on it.
//The 15204352 is the factor times the number of screen divisions. (1048576 * 14.5)
//To get the result in nanoseconds for matching the division time a multiplication with 1000000000 is done
const float sample_time_converters[18] =
{
  (0.000000005 * 2500000000.0) / 15204352.0,
  (0.000000010 * 2500000000.0) / 15204352.0,
  (0.000000020 * 2500000000.0) / 15204352.0,
  (0.000000050 * 2500000000.0) / 15204352.0,
  (0.000000100 * 2500000000.0) / 15204352.0,
  (0.000000200 * 2500000000.0) / 15204352.0,
  (0.000000500 * 2500000000.0) / 15204352.0,
  (0.000001000 * 2500000000.0) / 15204352.0,
  (0.000002000 * 2500000000.0) / 15204352.0,
  (0.000005000 * 2500000000.0) / 15204352.0,
  (0.000010000 * 2500000000.0) / 15204352.0,
  (0.000020000 * 2500000000.0) / 15204352.0,
  (0.000050000 * 2500000000.0) / 15204352.0,
  (0.000100000 * 2500000000.0) / 15204352.0,
  (0.000200000 * 2500000000.0) / 15204352.0,
  (0.000500000 * 2500000000.0) / 15204352.0,
  (0.001000000 * 2500000000.0) / 15204352.0,
  (0.002000000 * 2500000000.0) / 15204352.0,
};

//Division time in nanoseconds for matching the needed time per div setting in auto set
const uint32 time_per_div_matching[24] =
{
  200000000,
  100000000,
   50000000,
   20000000,
   10000000,
    5000000,
    2000000,
    1000000,
     500000,
     200000,
     100000,
      50000,
      20000,
      10000,
       5000,
       2000,
       1000,
        500,
        200,
        100,
         50,
         20,
         10,
          5,
};

const uint32 samplerate_for_autosetup[4] =
{
  0, 6, 12, 16
};

const uint32 timebase_settings[24] =
{
    1800,   //200ms/div
    1800,   //100ms/div
    1800,   // 50ms/div
    1800,   // 20ms/div (Was 800, but that does not yield enough samples)
    1800,   // 10ms/div
    2500,   //  5ms/div
    2500,   //  2ms/div
    2500,   //  1ms/div
    2500,   //500us/div
    2500,   //200us/div
    3000,   //100us/div
    6096,   // 50us/div
   14288,   // 20us/div
   22480,   // 10us/div
   22480,   //  5us/div
   83920,   //  2us/div
  206800,   //  1us/div
  411600,   //500ns/div
  411600,   //200ns/div
  411600,   //100ns/div
  411600,   // 50ns/div
  411600,   // 20ns/div
  411100,   // 10ns/div
  411100,   //  5ns/div
};

//Table used for converting horizontal screen pixels to time based on the time per div setting
const SCREENTIMECALCDATA screen_time_calc_data[24] =
{
  {    400, 3, 3 },         //200ms/div
  {    200, 3, 3 },         //100ms/div
  {    100, 3, 3 },         // 50ms/div
  {  40000, 2, 4 },         // 20ms/div
  {  20000, 2, 4 },         // 10ms/div
  {  10000, 2, 4 },         //  5ms/div
  {   4000, 2, 4 },         //  2ms/div
  {   2000, 2, 4 },         //  1ms/div
  {   1000, 2, 4 },         //500us/div
  {    400, 2, 4 },         //200us/div
  {    200, 2, 4 },         //100us/div
  {    100, 2, 4 },         // 50us/div
  {  40000, 1, 5 },         // 20us/div
  {  20000, 1, 5 },         // 10us/div
  {  10000, 1, 5 },         //  5us/div
  {   4000, 1, 5 },         //  2us/div
  {   2000, 1, 5 },         //  1us/div
  {   1000, 1, 5 },         //500ns/div
  {    400, 1, 5 },         //200ns/div
  {    200, 1, 5 },         //100ns/div
  {    100, 1, 5 },         // 50ns/div
  {  40000, 0, 6 },         // 20ns/div
  {  20000, 0, 6 },         // 10ns/div
  {  10000, 0, 6 }          //  5ns/div
};

//Table used for converting vertical screen pixels to voltage based on the volt per div setting(s)
const VOLTCALCDATA volt_calc_data[3][7] =
{
  { {  10000, 3 },  {  5000, 3 }, {  2000, 3 }, {  1000, 3 }, {  400, 3 }, {  200, 3 }, {  100, 3 } },
  { { 100000, 3 },  { 50000, 3 }, { 20000, 3 }, { 10000, 3 }, { 4000, 3 }, { 2000, 3 }, { 1000, 3 } },
  { {   1000, 4 },  {   500, 4 }, {   200, 4 }, {   100, 4 }, {   40, 4 }, {   20, 4 }, {   10, 4 } }
};

//Table for converting on samples based period time to frequency
const FREQCALCDATA freq_calc_data[18] =
{
  {   20000000, 5 },     //200MSa/s
  {   10000000, 5 },     //100MSa/s
  {    5000000, 5 },     // 50MSa/s
  { 2000000000, 4 },     // 20MSa/s
  { 1000000000, 4 },     // 10MSa/s
  {  500000000, 4 },     //  5MSa/s
  {  200000000, 4 },     //  2MSa/s
  {  100000000, 4 },     //  1MSa/s
  {   50000000, 4 },     //500KSa/s
  {   20000000, 4 },     //200KSa/s
  {   10000000, 4 },     //100KSa/s
  {    5000000, 4 },     // 50KSa/s
  { 2000000000, 3 },     // 20KSa/s
  { 1000000000, 3 },     // 10KSa/s
  {  500000000, 3 },     //  5KSa/s
  {  200000000, 3 },     //  2KSa/s
  {  100000000, 3 },     //  1KSa/s
  {   50000000, 3 }      // 500Sa/s
};

//Table used for converting on samples based time to actual time
const TIMECALCDATA time_calc_data[18] =
{
  {      500,  1 },
  {     1000,  1 },
  {     2000,  1 },
  {     5000,  1 },
  {    10000,  1 },
  {    20000,  1 },
  {    50000,  1 },
  {      100,  2 },
  {      200,  2 },
  {      500,  2 },
  {     1000,  2 },
  {     2000,  2 },
  {     5000,  2 },
  {    10000,  2 },
  {    20000,  2 },
  {    50000,  2 },
  {      100,  3 },
  {      200,  3 }
};

//Table for the different magnitudes for displaying time, frequency and voltage
const char *magnitude_scaler[8] = { "p", "n", "u", "m", "", "k", "M", "G"};

//Table for scaling between two volt per division settings. Used when looking at a waveform capture when the voltage per division setting is changed. (Stop mode or waveform view mode)
const int32 vertical_scaling_factors[7][7] =
{
  //Sample volt per div                                                         Display volt per div
  // 5V/div  2.5V/div   1V/div  500mV/div  200mV/div  100mV/div  50mV/div
  //     /1        /2       /5        /10        /25        /50      /100
  {   10000,     5000,    2000,      1000,       400,       200,      100 },  //5V/div
  
  //     *2        /1     /2.5         /5      /12.5        /25       /50
  {   20000,    10000,    4000,      2000,       800,       400,      200 },  //2.5V/div
  
  //     *5      *2.5       /1         /2         /5        /10       /20
  {   50000,    25000,   10000,      5000,      2000,      1000,      500 },  //1V/div
  
  //    *10        *5       *2         /1       /2.5         /5       /10
  {  100000,    50000,   20000,     10000,      4000,      2000,     1000 },  //500mV/div
  
  //    *25     *12.5       *5        *2.5        /1         /2        /4
  {  250000,   125000,   50000,      25000,    10000,      5000,     2500 },  //200mV/div
  
  //    *50       *25      *10          *5        *2         /1        /2
  {  500000,   250000,  100000,      50000,    20000,     10000,     5000 },  //100mV/div
  
  //   *100       *50      *20         *10        *4         *2        /1
  { 1000000,   500000,  200000,     100000,    40000,     20000,    10000 },  //50mV/div
};

//----------------------------------------------------------------------------------------------------------------------------------

const char *measurement_names[12] =
{
  "Vmax",
  "Vmin",
  "Vavg",
  "Vrms",
  "Vpp",
  "Vp",
  "Freg",
  "Cycle",
  "Time+",
  "Time-",
  "Duty+",
  "Duty-"
};

//----------------------------------------------------------------------------------------------------------------------------------

const PATHINFO view_file_path[2] = 
{
  { "\\pictures\\",  10 },
  { "\\waveforms\\", 11 }
};

const char view_file_extension[2][5] =
{
  { ".bmp" },
  { ".wav" }
};

const char *thumbnail_file_names[2] =
{
  "\\pictures\\bmp_thumbnails.sys",
  "\\waveforms\\wav_thumbnails.sys"
};

//----------------------------------------------------------------------------------------------------------------------------------

//Setup the bitmap header
//Consist of basic bitmap header followed by a DIB header (BITMAPINFOHEADER + BITMAPV3INFOHEADER)
//The bitmap height is using a negative value for reversing the top to bottom lines. This allows for just writing the frame buffer to the file
const uint8 bmpheader[PICTURE_HEADER_SIZE] =
{
  //Header identifier
  'B', 'M',

  //Size of the file in bytes
   PICTURE_FILE_SIZE        & 0xFF,
  (PICTURE_FILE_SIZE >>  8) & 0xFF,
  (PICTURE_FILE_SIZE >> 16) & 0xFF,
  (PICTURE_FILE_SIZE >> 24) & 0xFF,

  //Reserved
  0, 0, 0, 0,

  //Offset to the pixel array
   PICTURE_HEADER_SIZE        & 0xFF,
  (PICTURE_HEADER_SIZE >>  8) & 0xFF,
  (PICTURE_HEADER_SIZE >> 16) & 0xFF,
  (PICTURE_HEADER_SIZE >> 24) & 0xFF,

  //Size of DIB header (BITMAPV3INFOHEADER seems to not be recognized on Windows XP and older Ubuntu and maybe other linux variants)
  56, 0, 0, 0,

  //Bitmap width in pixels
   800        & 0xFF,
  (800 >>  8) & 0xFF,
  (800 >> 16) & 0xFF,
  (800 >> 24) & 0xFF,

  //Bitmap height in pixels
   -480        & 0xFF,
  (-480 >>  8) & 0xFF,
  (-480 >> 16) & 0xFF,
  (-480 >> 24) & 0xFF,

  //Number of color planes
  1, 0,

  //Number of bits per pixel
  16, 0,

  //Compression method (BI_BITFIELDS)
  3, 0, 0, 0,

  //Pixel array size
   PICTURE_DATA_SIZE        & 0xFF,
  (PICTURE_DATA_SIZE >>  8) & 0xFF,
  (PICTURE_DATA_SIZE >> 16) & 0xFF,
  (PICTURE_DATA_SIZE >> 24) & 0xFF,

  //Horizontal resolution
  0, 0, 0, 0,

  //Vertical resolution
  0, 0, 0, 0,

  //Number of colors in the pallete
  0, 0, 0, 0,

  //Number of colors important
  0, 0, 0, 0,

  //Mask fields for BI_BITFIELDS compression
  //Red mask 0x0000F800
  0, 0xF8, 0, 0,

  //Green mask 0x000007E0
  0xE0, 7, 0, 0,

  //Blue mask 0x0000001F
  0x1F, 0, 0, 0,

  //Alpha mask 0x00000000
  0, 0, 0, 0,
};

//----------------------------------------------------------------------------------------------------------------------------------
