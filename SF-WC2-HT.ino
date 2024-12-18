#include <Wire.h>
#include <EEPROM.h>

#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <EEPROM-Storage.h>
#include <at24c32.h>
#include <ArtronShop_SHT45.h>

///////////////////////////////////////////////////////////////////////////////////
//-----------------------------------LCD Module----------------------------------//
///////////////////////////////////////////////////////////////////////////////////
//****** Configuration *******//
#define LCD_COLS 16
#define LCD_ROWS 2
#define LCD_CHARSIZE LCD_5x8DOTS

//********* Declare **********//
LiquidCrystal_I2C LCD(0x27, LCD_COLS, LCD_ROWS);

//***** Custom Character *****//
//0
uint8_t degreeSymbol[] = {
    0x07,
    0x05,
    0x07,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};
//1
uint8_t upDownSymbol[] = {
    0x04,
    0x0A,
    0x11,
    0x00,
    0x00,
    0x11,
    0x0A,
    0x04
};
//2
uint8_t upSymbol[] = {
    0x04,
    0x0A,
    0x11,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};
//3
uint8_t downSymbol[] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x11,
    0x0A,
    0x04
};

///////////////////////////////////////////////////////////////////////////////////
//----------------------------------DS3231 Module--------------------------------//
///////////////////////////////////////////////////////////////////////////////////
//********* Declare **********//
DS3231 RTC;
bool century = false;
bool h12hflag = false;
bool pmflag = false;

///////////////////////////////////////////////////////////////////////////////////
//-----------------------------------SHT45 Module--------------------------------//
///////////////////////////////////////////////////////////////////////////////////
ArtronShop_SHT45 sht45(&Wire, 0x44);

///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------Button------------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//** Define count of all buttons **//
#define BUTTON_NUM 5

//********** Define Pin **********//
#define SETTING_BUTTON 8
#define UP_BUTTON 9
#define DOWN_BUTTON 10
#define ENTER_BUTTON 11
#define BACK_BUTTON 12

uint8_t buttons[BUTTON_NUM] = {
    SETTING_BUTTON,
    UP_BUTTON,
    DOWN_BUTTON,
    ENTER_BUTTON,
    BACK_BUTTON
};

//******** Declear variables for button state ********//
bool btLastButtonState = true;
int btActive = -1;
int btHold = 0;

//-------- Adjust for Hold button time ---------//
int btAcHold = 20;      //fine-june
//----------------------------------------------//

///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------Display-----------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//********** Define Welcome Display **********//
#define DISP_WELCOME_L1 "Welcome"
#define DISP_WELCOME_L2 "SF-WC2"

//-------- Adjust for Welcom Display ---------//
int welcomeDelay = 1000;    //Time for show welcome message (second)
//--------------------------------------------//

//*********** Declear Display State ***********//

//Main
enum DISPLAY_STATE{
    DS_MAIN,
    DS_ADJUST_WT,
    DS_DATALOG,
    DS_SETTING
};

//Adjust Water Time
enum DISPLAY_ADJUST_WATER_TIME{
    WT_MAIN,
    WT_SETTING      //Link DISPLAY_ADJ_WT_SETTING
};

enum DISPLAY_ADJ_WT_SETTING{
    WT_ST_MAIN,
    WT_ST_SUB
};

enum DISPLAY_ADJ_WT_SETTING_SUB{
    WT_ST_SUB_EDIT,
    WT_ST_SUB_DELETE,
    WT_ST_SUB_ADD
};

//Setting
enum DISPLAY_SETTING{
    ST_MAIN,
    ST_MENU
};

enum DISPLAY_ST_MENU{
    ST_MENU_TIME,
    ST_MENU_MAIN_DISPLAY,
    ST_MENU_BACKLIGHT,
    ST_MENU_TIMEOUT_MENU,    //Include AdjustWaterTime
    ST_MENU_TIMEOUT_MANUAL_WT,
    ST_MENU_RECORD_FREQUENCY,
    ST_MENU_RESET
};
//Define message for Setting Menu//
#define DISP_ST_MENU_TIME "Time Setting"
#define DISP_ST_MENU_MAIN_DISPLAY "Main Dispaly"
#define DISP_ST_MENU_BACKLIGHT "BackLight Time"
#define DISP_ST_MENU_TIMEOUT_MENU "Menu Timeout"
#define DISP_ST_MENU_TIMEOUT_MANUAL_WT "Limit Manual"
#define DISP_ST_MENU_RECORD_FREQUENCY "Record Time"
#define DISP_ST_MENU_RESET "Reset"

//Main Display
enum DISPLAY_ST_MAIN_DISPLAY{
    ST_MD_MAIN_MENU,
    ST_MD_SUB_MENU
};
enum DISPLAY_ST_MD{
    MD_STYLE,   //Main Diaplay Style
    MD_CLASSIC, //Switch Display Time
    MD_HT,      //Update HT Time
    MD_DUAL     //Switch Display Time
};

//Setting Reset Menu
enum DISPLAY_ST_RESET{
    ST_RS_MAIN,
    ST_RS_MENU
};
enum DISPLAY_RS_SETTING{
    RS_ADJUST_WT,
    RS_DEFAULT
};
//Define message for Reset Menu//
#define DISP_ST_RS_ADJUST_WT "WaterTime"
#define DISP_ST_RS_DEFAULT "DefaultFactory"

//Blink State
enum DISPLAY_BLINK{
    DAY,
    MONTH,
    YEAR,
    HOUR,
    MINUTE,
    PERIOD_TIME,
    YES,
    NO
};

//*********** Define Display State ***********//

//Main
enum DISPLAY_STATE dispStateCurrent = DS_MAIN;

//AdjustWaterTime
enum DISPLAY_ADJUST_WATER_TIME dispAdjustWTCurrent = WT_MAIN;
enum DISPLAY_ADJ_WT_SETTING dispAdjWTSetingCurrent = WT_ST_MAIN;
uint8_t dispAdjWTSettingSubIndex = WT_ST_SUB_EDIT;
uint8_t dispAdjustWTIndex = 0;

//Setting
enum DISPLAY_SETTING dispSettingCurrent = ST_MAIN;
int dispSTMenuIndex = 0;
//Reset
enum DISPLAY_ST_RESET dispSTResetCurrent = ST_RS_MAIN;
int dispSTResetIndex = 0;       //DISPLAY_RS_SETTING

//Main Display
enum DISPLAY_ST_MAIN_DISPLAY dispMainDisplayCurrent = ST_MD_MAIN_MENU;
int dispMainDisplayIndex = 0;

//Blink
enum DISPLAY_BLINK dispBlinkSelect = HOUR;

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------Main Display---------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//******** Define count of Main Display ********//
#define NUM_MAINDISPLAY_STYLE_OPTION 3

//******** Define Default Main Display ********//
#define MAINDISPLAY_STYLE_DEFAULT 2
#define MAINDISPLAY_CLASSIC_SWITCH_DEFAULT 3
#define MAINDISPLAY_HT_UPDATE_DEFAULT 3
#define MAINDISPLAY_DUAL_SWITCH_DEFAULT 5

///////////// Declear variables for Main Display (Common) /////////////
enum DISPLAY_MAIN{
    MN_CONTROLLER,
    MN_HT
};
uint8_t mainDisplayIndex = MN_CONTROLLER;

enum DISPLAY_MAIN_STATE{
    MN_STT_CONTROLLER_ONLY,
    MN_STT_HT_ONLY,
    MN_STT_CONTROLLER_HT
};
uint8_t mainDisplayOptionStyle[3] = {
    MN_STT_CONTROLLER_ONLY,
    MN_STT_HT_ONLY,
    MN_STT_CONTROLLER_HT
};
uint8_t mainDisplayStyle;
int mainDisplayOptionStyleIndex = 0;

uint8_t mainDisplayOptionTimeSetting;   //Temporary for Time Setting

//******** Action variables ********//
uint32_t mainDisplayAct;
float mainDisplayTemp;
float mainDisplayRH;

///////////// Declear variables for Main Display (Classic) /////////////

const uint8_t MAX_MD_CLASSIC = 10;

enum DISPLAY_CLASSIC_HT{
    TEMP_IN,
    TEMP_OUT,
    RH
};
uint8_t dispMeasurementIndex = TEMP_IN;
uint8_t mainDisplayClassicSwitchTime;  //Sec

///////////// Declear variables for Main Display (HT Only) /////////////

const int8_t MAX_MD_HT = 10;

uint8_t mainDisplayHTUpdateTime;  //Sec

///////////// Declear variables for Main Display (Dual) /////////////

const int8_t MAX_MD_DUAL = 10;

uint8_t mainDisplayDualSwitchTime;    //Sec

//-----------------------------------------------------------------//

//*********** Declear variables for Adjust Water Time ***********//
struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int periodTime;
} StTime;

bool newAdjustWaterState = false;

//-------- Adjust for Adjust Water Time  ---------//
#define DELAY_ERROR_DISP 1300       //Error Message Dipsplay Time (MilLiSec)

//*********** Declear variables for Blink ***********//
bool blinkState = false;
int blinkCount = 0;

//-------- Adjust for Blink time ---------//
int blinkTime = 10;    //fine-june

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------Solenoid Drive--------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//**************** Define Pin ****************//
#define SOLENOID_PIN 13

//******** Declear variables ********//
bool solenoid_state = false;

///////////////////////////////////////////////////////////////////////////////////
//-------------------------------Water Time by Manual----------------------------//
///////////////////////////////////////////////////////////////////////////////////

//******** Define count of Water Time ********//
#define WATER_TIME_NUM 16

//******** Declear variables ********//
int wtStartTime[WATER_TIME_NUM];    //Minute unit
int wtPeriodTime[WATER_TIME_NUM];   //Second unit
int onTimePeriod;                   //PeriodTime for Water now

///////////////////////////////////////////////////////////////////////////////////
//-----------------------------Water Time by Temp & RH---------------------------//
///////////////////////////////////////////////////////////////////////////////////

//Define RH control **Feature: defind by user**
uint8_t wrhLower = 80;
uint8_t wrhHigher = 90;
uint8_t wrhCheckFrequency = 1;  //Minute unit 1
uint8_t wrhTime = 1;            //Minute unit 3
uint8_t wrhDelayCheck = 1;    //Minute unit 5
uint8_t wrhMaxRepeat = 10;      //10

//Declear variables for operate
bool wrhActive = false;
bool wrhActiveCheck = false;
float wrhRHPoint;
uint32_t wrhCheckTime;
uint32_t wrhOffTime;
uint32_t wrhActiveCheckTime;
uint8_t wrhRepeat = 0;

///////////////////////////////////////////////////////////////////////////////////
//------------------------------------BackLight----------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//******** Define count of BackLight Option ********//
#define NUM_BACKLIGHT_OPTION 8

//******** Define Default Factory value ********//
#define BACKLIGHT_DEFAULT 30

//******** Declear variables ********//
uint8_t backLightTime;                  //BackLight Time Current

uint8_t backLightOptionTime[NUM_BACKLIGHT_OPTION] = {      //For BackLignt Setting
    0, 1, 3, 5, 10, 15, 30, 60
};
int backLightOptionIndex = 0;

bool backLightState = true;

//******** Action variables ********//
uint32_t backLightAct;

///////////////////////////////////////////////////////////////////////////////////
//-----------------------------------Menu TimeOut--------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//******** Define count of Menu TimeOut Option ********//
#define NUM_MENU_TIMEOUT 5

//******** Define Default Factory value ********//
#define MENU_TIMEOUT_DEFAULT 15

//******** Declear variables ********//
uint8_t menuTimeout;

uint8_t menuTimeoutOptionTime[NUM_MENU_TIMEOUT] = {
    0, 10, 15, 30, 60
};
int menuTimeoutOptionIndex = 0;
bool menuTimeOutState = false;

//******** Action variables ********//
uint32_t menuTimeOutAct;

///////////////////////////////////////////////////////////////////////////////////
//----------------------------------Manual TimeOut-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//******** Define count of Manual TimeOut Option ********//
#define NUM_MANUAL_TIMOUT 8

//******** Define Default Factory value ********//
#define MANUAL_TIMEOUT_DEFAULT 10

//******** Declear variables ********//
uint8_t manualTimeout;

uint8_t manualTimeoutOptionTime[NUM_MANUAL_TIMOUT] = {
    0, 1, 3, 5, 10, 15, 30, 60
};
int manualTimeoutOptinIndex = 0;
bool manualTimeoutState = false;

//******** Action variables ********//
uint32_t manualTimeOutAct;

///////////////////////////////////////////////////////////////////////////////////
//-------------------------Humidity and Temperature Record-----------------------//
///////////////////////////////////////////////////////////////////////////////////
// Pointer put to Internal
// Data put to External
#define EX_EEPROM_DATA 0x57

//******** Define count of Record Frequency Option ********//
#define NUM_RECORD_FREQUENCY 5

//******** Define Default Factory value ********//
#define RECORD_FREQUENCY_DEFAULT 15

//******** Declear variables for EEPROM ********//
uint8_t htRecordFrequencyOptionTime[NUM_RECORD_FREQUENCY] = {
    1, 2, 5, 10, 15
};
int recordFrequencyOptionIndex = 0;

//******** Declear variables for Record ********//
struct DataLog{
    uint32_t dateTime;
    float temp;
    float rh;
};

uint8_t htPeriodTime;      //Minute unit

//******** Action variables ********//
byte htMinuteAct;       //Get next minute

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------DataLog Monitor-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

uint16_t dlmPointer = 0;

///////////////////////////////////////////////////////////////////////////////////
//----------------------------------Record Process-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//Record Status Refer https://github.com/Shinodari/ShinoFarm-Water-Control-SF-WC2-HT/issues/10#issuecomment-2527870628
enum RP_STATUS{
    RP_WCM_START = 10,
    RP_WCM_STOP = 11,
    RP_WCT_START = 20,
    RP_WCT_STOP = 21,
    RP_WCR_START = 30,
    RP_WCR_STOP = 31,
    RP_WCR_LOW = 32,
    RP_WCR_HIGH = 33,
    RP_WCR_SUCCESS = 34,
    RP_WCR_FAIL = 35
};

struct RecProcess{
    uint32_t timeStemp;
    int status;
};


///////////////////////////////////////////////////////////////////////////////////
//----------------------------------Internal EEPROM------------------------------//
///////////////////////////////////////////////////////////////////////////////////
//********** Define EEPROM Adress for Setting ************//
//Setting allocate 256bit (16 Values for data 8bit + checksum 8bit)
#define V1_ADR 0
#define V2_ADR V1_ADR + 1 + 1
#define V3_ADR V2_ADR + 1 + 1
#define V4_ADR V3_ADR + 1 + 1
#define V5_ADR V4_ADR + 1 + 1
#define V6_ADR V5_ADR + 1 + 1
#define V7_ADR V6_ADR + 1 + 1
#define V8_ADR V7_ADR + 1 + 1

//********** Define EEPROM Adress for Water Time ************//
//Water Time allocate 768bit for 16 Water Time
#define S1_ADR 32
#define S2_ADR S1_ADR + 2 + 1
#define S3_ADR S2_ADR + 2 + 1
#define S4_ADR S3_ADR + 2 + 1
#define S5_ADR S4_ADR + 2 + 1
#define S6_ADR S5_ADR + 2 + 1
#define S7_ADR S6_ADR + 2 + 1
#define S8_ADR S7_ADR + 2 + 1
#define S9_ADR S8_ADR + 2 + 1
#define S10_ADR S9_ADR + 2 + 1
#define S11_ADR S10_ADR + 2 + 1
#define S12_ADR S11_ADR + 2 + 1
#define S13_ADR S12_ADR + 2 + 1
#define S14_ADR S13_ADR + 2 + 1
#define S15_ADR S14_ADR + 2 + 1
#define S16_ADR S15_ADR + 2 + 1

#define P1_ADR S1_ADR + 64
#define P2_ADR P1_ADR + 2 + 1
#define P3_ADR P2_ADR + 2 + 1
#define P4_ADR P3_ADR + 2 + 1
#define P5_ADR P4_ADR + 2 + 1
#define P6_ADR P5_ADR + 2 + 1
#define P7_ADR P6_ADR + 2 + 1
#define P8_ADR P7_ADR + 2 + 1
#define P9_ADR P8_ADR + 2 + 1
#define P10_ADR P9_ADR + 2 + 1
#define P11_ADR P10_ADR + 2 + 1
#define P12_ADR P11_ADR + 2 + 1
#define P13_ADR P12_ADR + 2 + 1
#define P14_ADR P13_ADR + 2 + 1
#define P15_ADR P14_ADR + 2 + 1
#define P16_ADR P15_ADR + 2 + 1

//********** Define EEPROM Adress for Record Process ************//
#define RP_START_ADR 156
#define RP_MAX 150

//Pointer
#define RP_PTR 1021

//Time Process
#define PT_SIZE 4   //KB

//Status Process
#define PS_SIZE 1   //KB

//********** Define EEPROM Adress for Pointer of DataLog ************//
#define DL_PTR 1023

//******* Define Variable of EEPROM for Setting ********//
EEPROMStorage<uint8_t> eepSettingBackLight(V1_ADR, BACKLIGHT_DEFAULT);
EEPROMStorage<uint8_t> eepSettingTOMenu(V2_ADR, MENU_TIMEOUT_DEFAULT);
EEPROMStorage<uint8_t> eepSettingTOManualWT(V3_ADR, MANUAL_TIMEOUT_DEFAULT);
EEPROMStorage<uint8_t> eepSettingRecordFrequency(V4_ADR, RECORD_FREQUENCY_DEFAULT);
EEPROMStorage<uint8_t> eepSettingMainDisplayStyle(V5_ADR, MAINDISPLAY_STYLE_DEFAULT);
EEPROMStorage<uint8_t> eepSettingMainDisplayClassicSwitch(V6_ADR, MAINDISPLAY_CLASSIC_SWITCH_DEFAULT);
EEPROMStorage<uint8_t> eepSettingMainDisplayHTUpdate(V7_ADR, MAINDISPLAY_HT_UPDATE_DEFAULT);
EEPROMStorage<uint8_t> eepSettingMainDisplayDualSwitch(V8_ADR, MAINDISPLAY_DUAL_SWITCH_DEFAULT);

//******* Define Variable of EEPROM for Water Time ********//
EEPROMStorage<uint16_t> eepStartWT1(S1_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT2(S2_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT3(S3_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT4(S4_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT5(S5_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT6(S6_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT7(S7_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT8(S8_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT9(S9_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT10(S10_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT11(S11_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT12(S12_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT13(S13_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT14(S14_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT15(S15_ADR, 0);
EEPROMStorage<uint16_t> eepStartWT16(S16_ADR, 0);

EEPROMStorage<uint16_t> eepPeriodWT1(P1_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT2(P2_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT3(P3_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT4(P4_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT5(P5_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT6(P6_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT7(P7_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT8(P8_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT9(P9_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT10(P10_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT11(P11_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT12(P12_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT13(P13_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT14(P14_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT15(P15_ADR, 0);
EEPROMStorage<uint16_t> eepPeriodWT16(P16_ADR, 0);

//********** Define Variable of EEPROM for Pointer of DataLog ************//
EEPROMStorage<uint16_t> dlPointer(DL_PTR, 0);    //Next position for record the data log

//********** Define Variable of EEPROM for Pointer of Record Process ************//
//uint16_t rpPointer;
EEPROMStorage<uint16_t> rpPointer(RP_PTR, 0);

///////////////////////////////////////////////////////////////////////////////////
//-------------------------------External EEPROM(0x50)---------------------------//
///////////////////////////////////////////////////////////////////////////////////

//********** Define EEPROM Adress for Setting ************//
#define DL_ADR 0
const uint16_t DL_MAX = 288;

//******* Define Variable of External EEPROM for DataLog ********//
AT24C32 dlEEPROM(EX_EEPROM_DATA);

///////////////////////////////////////////////////////////////////////////////////
//---------------------------Link External by Serial Port------------------------//
///////////////////////////////////////////////////////////////////////////////////

const String LINK_GET_DATALOG = "getDatalog";
const String LINK_GET_PROCESS = "getProcess";

///////////////////////////////////////////////////////////////////////////////////
//**********************************Setep Process********************************//
///////////////////////////////////////////////////////////////////////////////////
void setup() {
    //--DS3231--//
    Wire.begin();

    //--Display Welcome--//
    LCD.begin(LCD_COLS, LCD_ROWS, LCD_CHARSIZE);
    LCD.backlight();
    LCD.setCursor(0, 0);
    LCD.print(DISP_WELCOME_L1);
    LCD.setCursor(0, 1);
    LCD.print(DISP_WELCOME_L2);
    delay(welcomeDelay);
    LCD.clear();

    /*/--SHT45--//
    while (!sht45.begin()){
        LCD.clear();
        LCD.print("SHT45 not found!");
        delay(1000);
    }/**/

    //--Button--//
    for(int pin : buttons){
        pinMode(pin, INPUT_PULLUP);
    }
    
    //--Solenoid--//
    pinMode(SOLENOID_PIN, OUTPUT);

    //--LCD--//
    LCD.createChar(0, degreeSymbol);
    LCD.createChar(1, upDownSymbol);
    LCD.createChar(2, upSymbol);
    LCD.createChar(3, downSymbol);

    //--Auto Water by RH Control--//
    wrhCheckTime = millis() + (wrhCheckFrequency * 60 * 1000L);
    //Temp//-----------------
        wrhCheckTime = 0;
    //-----------------------*/

    //--Water Time(EEPROM)--//
    eepAllWaterTimeLoad();
    
    //--Setting(EEPROM)--//
    eepAllSettingLoad();

    //--BackLight--//
    setBackLightAct();

    //--Menu TimeOut--//
    setMenuTimeOutAct();

    //--Main Display--//
    setMainDisplayAct();

    //--Record Process--//
    //getRPPointer();

    //--DataLog--//
    DataLog dl;
    dlEEPROM.get((dlPointer - 1) * sizeof(struct DataLog), dl);
    DateTime dt(dl.dateTime);
    if ((dt.day() == RTC.getDate())
      && (dt.month() == RTC.getMonth(century))
      && (dt.year() == RTC.getYear() + 2000)
      && (dt.hour() == RTC.getHour(h12hflag,pmflag))
      && (dt.minute() == RTC.getMinute())){
        if (dt.minute() < 60)
            htMinuteAct = dt.minute() + 1;
        else
            htMinuteAct = 0;
    } else {
        htMinuteAct = RTC.getMinute();
    }
    checkPointerDataLog();

    //--Serial Port--//
    Serial.begin(9600);
}

///////////////////////////////////////////////////////////////////////////////////
//***********************************Loop Process********************************//
///////////////////////////////////////////////////////////////////////////////////
void loop() {
    //--Auto WaterTime--//
    for (int i = 0; i < WATER_TIME_NUM; i++){
        int hour = wtStartTime[i] / 60;
        int minute = wtStartTime[i] % 60;
        if (hour == RTC.getHour(h12hflag,pmflag) && (minute == RTC.getMinute())){
            onTimePeriod = (RTC.getHour(h12hflag,pmflag) * 60 * 60) + (RTC.getMinute() * 60) + wtPeriodTime[i];
            if(wtPeriodTime[i] > 0){
                solenoid_state = true;
                digitalWrite(SOLENOID_PIN, solenoid_state);
                break;
            }
        }
    }
    if (solenoid_state && !manualTimeoutState && !wrhActive){
        if(((RTC.getHour(h12hflag,pmflag) * 60 * 60) + (RTC.getMinute() * 60) + RTC.getSecond()) >= onTimePeriod){
            solenoid_state = false;
            digitalWrite(SOLENOID_PIN, solenoid_state);
        }
    }

    //--Auto Water by RH Control--//
    if (!wrhActive){
        if (millis() >= wrhCheckTime){
            float rh = getRH();
            /*/Temp//
            Serial.print("Please enter the RH Lower : ");
            while (Serial.available() == 0){
                LCD.clear();
                LCD.print("Please enter in Serial!");
            }
            rh = Serial.parseFloat();
            Serial.println(rh);
            ///////*/
            if (rh <= wrhLower){
                wrhActive = true;
                wrhOffTime = millis() + (wrhTime * 60 * 1000L);
                wrhRHPoint = rh;
                //solenoid_state = true;
                digitalWrite(SOLENOID_PIN, true);
                /*Temp/Serial.println("Start..");/**/
                rpStatusSave(RP_WCR_START);
            } else {
                wrhCheckTime = millis() + (wrhCheckFrequency * 60 * 1000L);
            }
        }
    } else {
        if (!wrhActiveCheck){
            if (millis() >= wrhOffTime){
                wrhActiveCheck = true;
                wrhActiveCheckTime = millis() + (wrhDelayCheck * 60 * 1000L);
                //solenoid_state = false;
                digitalWrite(SOLENOID_PIN, false);
                /*Temp/Serial.println("Stop..");/**/
                rpStatusSave(RP_WCR_STOP);
            }
        } else {
            if (millis() >= wrhActiveCheckTime){
                float rh = getRH();
                /*/Temp//
                Serial.print("Please enter the Visual RH : ");
                while (Serial.available() == 0){
                    LCD.clear();
                    LCD.print("Please enter in Serial!");
                }
                rh = Serial.parseFloat();
                Serial.println(rh);
                ////////**/
                if (rh > wrhRHPoint && rh >= wrhHigher){
/*Ag Duplicate*/    wrhActive = false;
                    wrhCheckTime = millis() + (wrhCheckFrequency * 60 * 1000L);
                    wrhRepeat = 0;
                    rpStatusSave(RP_WCR_SUCCESS);
                } else {
                    if (wrhRepeat < wrhMaxRepeat){
                        wrhOffTime = millis() + (wrhTime * 60 * 1000L);
                        //solenoid_state = true;
                        digitalWrite(SOLENOID_PIN, true);
                        rpStatusSave(RP_WCR_START);
                        wrhRepeat++;
                    } else {
/*Ag Duplicate*/        wrhActive = false;
                        wrhCheckTime = millis() + (wrhCheckFrequency * 60 * 1000L);
                        wrhRepeat = 0;
                        rpStatusSave(RP_WCR_FAIL);
                    }
                }
                wrhActiveCheck = false;
            }
        }
    }

    //--Display--//
    switch (dispStateCurrent)
    {
    case DS_MAIN:
        dispMain();
        break;
    case DS_ADJUST_WT:
        switch (dispAdjustWTCurrent){
            case WT_MAIN:        
                dispAdjustWTMain();
                break;
            case WT_SETTING:
                dispAdjustWTSetting();
                break;
        }
        break;
    case DS_DATALOG:
        dispDataLogMoniter();
        break;
    case DS_SETTING:
        dispSetting();
    }
    
    //--Button--//
    btCheckState();

    //--BackLight--//
    if (millis() >= backLightAct && backLightTime != 0){
        backLightState = false;
        LCD.noBacklight();
    }

    //--Menu TimeOut--//
    if (millis() >= menuTimeOutAct && menuTimeout !=0 && !menuTimeOutState){
        dispStateCurrent = DS_MAIN;
        menuTimeOutState = true;
        LCD.clear();
    }

    //--Manaul Water Timeout--//
    if (millis() >= manualTimeOutAct && manualTimeoutState){
        manualTimeoutState = false;
        solenoid_state = false;
        digitalWrite(SOLENOID_PIN, solenoid_state);
    }
    
    //----DataLog--//
    byte cMinute = RTC.getMinute();
    if (cMinute == htMinuteAct){
        if (cMinute % htPeriodTime == 0){
            DataLog dl;
            int adrHead = (dlPointer * sizeof(struct DataLog));
            DateTime dt = RTClib::now();
            dl.dateTime = dt.unixtime();
            dl.temp = getTemp();
            dl.rh = getRH();
            dlEEPROM.put(adrHead, dl);
            Serial.print(">>>> DataLog:");     Serial.println(dlPointer);///////////
            dlPointer++;
            checkPointerDataLog();
        }
        if (cMinute + 1 < 60)
            htMinuteAct = cMinute + 1;
        else
            htMinuteAct = 0;
    }

    //--Link Water Controller Program--/
    if (Serial.available() > 0){
        String command = Serial.readString();/*/
        switch (command){
            case LINK_GET_DATALOG:
                LCD.clear();
                LCD.print("Sending DataLog...");
                link_sendDataLog();
                LCD.clear();
                LCD.print("Completed");
                delay(3000);
                break;
            case LINK_GET_PROCESS:
                LCD.clear();
                LCD.print("Sending Process...");
                link_sendDataLog();
                LCD.clear();
                LCD.print("Completed");
                delay(3000);
                break;
        }/**/
        if (command == LINK_GET_DATALOG){
            LCD.clear();
            LCD.print("Sending DataLog...");
            link_sendDataLog();
            LCD.clear();
            LCD.print("Completed");
            delay(3000);
        } else if (command == LINK_GET_PROCESS){
            LCD.clear();
            LCD.print("Sending Process...");
            link_sendProcess();
            LCD.clear();
            LCD.print("Completed");
            delay(3000);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------Button FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////
void btCheckState(){
    for (int pin : buttons){
        bool value = digitalRead(pin);

        if (btActive < 0){
            if (value < btLastButtonState){   //DownButton
                btActive = pin;
                btLastButtonState = value;
                //--BackLight--//
                setBackLightAct();
                LCD.backlight();
                //--MenuTimeOut--//
                setMenuTimeOutAct();
                menuTimeOutState = false;
            }
        } else {
            if (btActive == pin){
                if (value > btLastButtonState){   //UpButton
                    if (backLightState == true){
                        if (btHold < btAcHold){
                            switch (pin)
                            {
                                case SETTING_BUTTON:
                                    btSettingPress();
                                    break;
                                case UP_BUTTON:
                                    btUpPress();
                                    break;
                                case DOWN_BUTTON:
                                    btDownPress();
                                    break;
                                case ENTER_BUTTON:
                                    btEnterPress();
                                    break;
                                case BACK_BUTTON:
                                    btBackPress();
                            }
                        }
                    }
                    backLightState = true;
                    btActive = -1;
                    btHold = 0;
                } else if (value == btLastButtonState) {    //HoldButton
                    if (btHold > btAcHold){
                        switch (pin)
                        {
                            case SETTING_BUTTON:
                                btSettingHold();
                                break;
                        }
                    }
                    btHold++;
                }                
                btLastButtonState = value;
            }
        }
    }
}

void btSettingPress(){
    switch (dispStateCurrent){
        case DS_MAIN:
            LCD.clear();
            newAdjustWaterState = false;
            dispStateCurrent = DS_ADJUST_WT;
            dispAdjustWTCurrent = WT_MAIN;
            dispBlinkSelect = HOUR;
            dispAdjustWTIndex = 0;
            preWaterTime(wtStartTime[dispAdjustWTIndex]);
            break;
        case DS_ADJUST_WT:
            switch (dispAdjustWTCurrent){
                case WT_MAIN:
                    LCD.clear();
                    dispAdjustWTCurrent = WT_SETTING;
                    dispAdjWTSetingCurrent = WT_ST_MAIN;
                    dispAdjWTSettingSubIndex = WT_ST_SUB_DELETE;
                    break;
                case WT_SETTING:
                    if (dispAdjWTSetingCurrent != WT_ST_MAIN){
                        switch (dispAdjWTSettingSubIndex){
                            case WT_ST_SUB_ADD:
                            case WT_ST_SUB_EDIT:
                                blinkCount = 0;
                                blinkState = false;
                                switch (dispBlinkSelect){
                                    case HOUR:
                                        dispBlinkSelect = MINUTE;
                                        break;
                                    case MINUTE:
                                        dispBlinkSelect = PERIOD_TIME;
                                        break;
                                    case PERIOD_TIME:
                                        dispBlinkSelect = HOUR;
                                        break;
                                }
                                break;
                        }
                    }
                    break;
            }
            break;
        case DS_SETTING:
            switch (dispSTMenuIndex){
                case ST_MENU_TIME:
                    switch (dispBlinkSelect){
                        case DAY:
                            dispBlinkSelect = HOUR;
                            break;
                        case MONTH:
                            dispBlinkSelect = DAY;
                            break;
                        case YEAR:
                            dispBlinkSelect = MONTH;
                            break;
                        case HOUR:
                            dispBlinkSelect = MINUTE;
                            break;
                        case MINUTE:
                            dispBlinkSelect = YEAR;
                            break;
                    }
                    break;
                case ST_MENU_RESET:
                    if (dispSTResetCurrent = ST_RS_MENU){
                        if (dispBlinkSelect == YES)
                            dispBlinkSelect = NO;
                        else if (dispBlinkSelect == NO)
                            dispBlinkSelect = YES;
                        
                    }
                    break;
            }
            blinkCount = 0;
            blinkState = false;
            break;
    }
}

void btSettingHold(){
    switch (dispStateCurrent){
        case DS_MAIN:
            LCD.clear();
            dispStateCurrent = DS_SETTING;
            dispSettingCurrent = ST_MAIN;
            dispSTMenuIndex = 0;
            break;
    }
}

void btUpPress(){
    switch (dispStateCurrent){
        case DS_MAIN:
            LCD.clear();
            dispStateCurrent = DS_DATALOG;
            dlmPointer = 0;
            getDLMPointer();
            break;
        case DS_ADJUST_WT:
            switch (dispAdjustWTCurrent){
                case WT_MAIN:
                    LCD.clear();   
                    if (dispAdjustWTIndex > 0)
                        dispAdjustWTIndex--;  

                    newAdjustWaterState = false;
                    break;
                case WT_SETTING:
                    switch (dispAdjWTSetingCurrent){
                        case WT_ST_MAIN:
                            if (dispAdjWTSettingSubIndex > 0)
                                dispAdjWTSettingSubIndex--;
                            break;
                        case WT_ST_SUB:
                            switch (dispAdjWTSettingSubIndex){
                                case WT_ST_SUB_ADD:
                                case WT_ST_SUB_EDIT:
                                    adjustStTimeOrPeriodUp(15, 30);
                                    break;
                            }
                            break;
                    }
                    LCD.clear();
                    break;
            }
            break;
        case DS_DATALOG:
            if (dlmPointer < DL_MAX){
                dlmPointer++;
                LCD.clear();
            } else {
                dlmPointer = 0;
            }
            break;
        case DS_SETTING:
            switch (dispSettingCurrent){
                case ST_MAIN:
                    if (dispSTMenuIndex > 0){
                        dispSTMenuIndex--;
                        LCD.clear();
                    }
                    break;
                case ST_MENU:
                    switch (dispSTMenuIndex){
                        case ST_MENU_TIME:
                            adjustStTimeOrPeriodUp(1, 1);
                            break;
                        case ST_MENU_MAIN_DISPLAY:
                            switch (dispMainDisplayCurrent){
                                case ST_MD_MAIN_MENU:
                                    if (dispMainDisplayIndex > 0){
                                        dispMainDisplayIndex--;
                                        LCD.clear();
                                    }
                                    break;
                                case ST_MD_SUB_MENU:
                                    if (dispMainDisplayIndex == MD_STYLE){
                                        if (mainDisplayOptionStyleIndex > 0){
                                            mainDisplayOptionStyleIndex--;
                                            LCD.clear();
                                        }
                                    } else {
                                        uint8_t maxTemp;
                                        switch (dispMainDisplayIndex){
                                            case MD_CLASSIC:
                                                maxTemp = MAX_MD_CLASSIC;
                                                break;
                                            case MD_HT:
                                                maxTemp = MAX_MD_HT;
                                                break;
                                            case MD_DUAL:
                                                maxTemp = MAX_MD_DUAL;
                                                break;
                                        }
                                        if (mainDisplayOptionTimeSetting < maxTemp){
                                            mainDisplayOptionTimeSetting++;
                                            LCD.clear();
                                        }
                                    }
                                    break;
                            }
                            break;
                        case ST_MENU_BACKLIGHT:
                            if (backLightOptionIndex < NUM_BACKLIGHT_OPTION - 1){
                                backLightOptionIndex++;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_TIMEOUT_MENU:
                            if (menuTimeoutOptionIndex < NUM_MENU_TIMEOUT -1){
                                menuTimeoutOptionIndex++;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_TIMEOUT_MANUAL_WT:
                            if (manualTimeoutOptinIndex < NUM_MANUAL_TIMOUT -1){
                                manualTimeoutOptinIndex++;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_RECORD_FREQUENCY:
                            if (recordFrequencyOptionIndex < NUM_RECORD_FREQUENCY -1){
                                recordFrequencyOptionIndex++;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_RESET:
                            if (dispSTResetIndex > 0 && dispSTResetCurrent == ST_RS_MAIN){
                                dispSTResetIndex--;
                                LCD.clear();
                            }
                            break;
                    }
                    break;
            }
            break;
    }
}

void btDownPress(){
    switch (dispStateCurrent){
        case DS_ADJUST_WT:
            switch (dispAdjustWTCurrent){
                case WT_MAIN:
                    LCD.clear();
                    if (dispAdjustWTIndex >= WATER_TIME_NUM -1)
                        break;

                    if (!newAdjustWaterState)
                        dispAdjustWTIndex++;

                    if (dispAdjustWTIndex >= getAllActiveWaterTime()){
                        newAdjustWaterState = true;
                    }
                    break;
                case WT_SETTING:
                    switch (dispAdjWTSetingCurrent){
                        case WT_ST_MAIN:
                            if (dispAdjWTSettingSubIndex < WT_ST_SUB_ADD)
                                dispAdjWTSettingSubIndex++;
                            break;
                        case WT_ST_SUB:
                            switch (dispAdjWTSettingSubIndex){
                                case WT_ST_SUB_ADD:
                                case WT_ST_SUB_EDIT:
                                    adjustStTimeOrPeriodDown(15, 30);
                                    break;
                            }
                            break;
                    }
                    LCD.clear();
                    break;
            }
        case DS_DATALOG:
            if (dlmPointer > 0){
                dlmPointer--;
                LCD.clear();
            } else {
                dlmPointer = DL_MAX;
            }
            break;
        case DS_SETTING:
            switch (dispSettingCurrent){
                case ST_MAIN:
                    if (dispSTMenuIndex < ST_MENU_RESET){
                        dispSTMenuIndex++;
                        LCD.clear();
                    }
                    break;
                case ST_MENU:
                    switch (dispSTMenuIndex){
                        case ST_MENU_TIME:
                            adjustStTimeOrPeriodDown(1, 1);
                            break;
                        case ST_MENU_MAIN_DISPLAY:
                            switch (dispMainDisplayCurrent){
                                case ST_MD_MAIN_MENU:
                                    if (dispMainDisplayIndex < MD_DUAL){
                                        dispMainDisplayIndex++;
                                        LCD.clear();
                                    }
                                    break;
                                case ST_MD_SUB_MENU:
                                    if (dispMainDisplayIndex == MD_STYLE){
                                        if (mainDisplayOptionStyleIndex < MN_STT_CONTROLLER_HT){
                                            mainDisplayOptionStyleIndex++;
                                            LCD.clear();
                                        }
                                    } else {
                                        if (mainDisplayOptionTimeSetting > 1){
                                            mainDisplayOptionTimeSetting--;
                                            LCD.clear();
                                        }
                                    break;
                                    }
                            }
                            break;
                        case ST_MENU_BACKLIGHT:
                            if (backLightOptionIndex > 0){
                                backLightOptionIndex--;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_TIMEOUT_MENU:
                            if (menuTimeoutOptionIndex > 0){
                                menuTimeoutOptionIndex--;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_TIMEOUT_MANUAL_WT:
                            if (manualTimeoutOptinIndex > 0){
                                manualTimeoutOptinIndex--;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_RECORD_FREQUENCY:
                            if (recordFrequencyOptionIndex > 0){
                                recordFrequencyOptionIndex--;
                                LCD.clear();
                            }
                            break;
                        case ST_MENU_RESET:
                            if (dispSTResetIndex < RS_DEFAULT && dispSTResetCurrent == ST_RS_MAIN){
                                dispSTResetIndex++;
                                LCD.clear();
                            }
                            break;
                    }
                    break;
            }
            break;
    }
}

void btEnterPress(){
    switch (dispStateCurrent){
        case DS_MAIN:
            solenoid_state = !solenoid_state;
            digitalWrite(SOLENOID_PIN, solenoid_state);
            //--Munaul Water Timeout--//
            if (solenoid_state){
                setManualTimeoutAct();
                manualTimeoutState = true;
            } else {
                manualTimeoutState = false;
            }
            break;
        case DS_ADJUST_WT:
            switch (dispAdjustWTCurrent){
                case WT_MAIN:
                    if (!newAdjustWaterState){
                        dispAdjustWTCurrent = WT_SETTING;                    
                        dispAdjWTSetingCurrent = WT_ST_MAIN;
                        dispAdjWTSettingSubIndex = WT_ST_SUB_EDIT;
                    } else {
                        dispAdjustWTCurrent = WT_SETTING;
                        dispAdjWTSetingCurrent = WT_ST_SUB;
                        dispAdjWTSettingSubIndex = WT_ST_SUB_ADD;
                        preWaterTime(0);
                        prePeriodTime(0);
                    }
                    break;
                case WT_SETTING:
                    switch (dispAdjWTSetingCurrent){
                        case WT_ST_MAIN:
                            dispAdjWTSetingCurrent = WT_ST_SUB;
                            if (dispAdjWTSettingSubIndex == WT_ST_SUB_ADD){
                                if (getAllActiveWaterTime() < WATER_TIME_NUM){
                                    dispAdjustWTIndex = getAllActiveWaterTime();
                                    preWaterTime(0);
                                    prePeriodTime(0);
                                } else {
                                    LCD.clear();
                                    LCD.print("Max Water Time");
                                    delay(DELAY_ERROR_DISP);
                                    dispAdjustWTCurrent = WT_MAIN;
                                }
                            }
                            break;
                        case WT_ST_SUB:
                            switch (dispAdjWTSettingSubIndex){
                                case WT_ST_SUB_ADD:
                                case WT_ST_SUB_EDIT:
                                    if (StTime.periodTime != 0){
                                        dispAdjustWTCurrent = WT_MAIN;
                                        dispBlinkSelect = HOUR;
                                        newAdjustWaterState = false;
                                        eepWaterTimeSave();
                                    } else {
                                        LCD.clear();
                                        LCD.print("Not Saved!");
                                        delay(DELAY_ERROR_DISP);
                                    }
                                    break;
                                case WT_ST_SUB_DELETE:
                                    wtStartTime[dispAdjustWTIndex] = 0;
                                    wtPeriodTime[dispAdjustWTIndex] = 0;
                                    rmDeactiveWT();
                                    eepAllWaterTimeSave();
                                    dispAdjustWTCurrent = WT_MAIN;
                                    if (getAllActiveWaterTime() > 0 && dispAdjustWTIndex != 0)
                                        dispAdjustWTIndex--;
                                    break;
                            }
                            break;
                    }
            }
            break;
        case DS_SETTING:
            switch (dispSettingCurrent){
                case ST_MAIN:                       //Before into each setting
                    dispSettingCurrent = ST_MENU;
                    switch (dispSTMenuIndex){
                        case ST_MENU_TIME:
                            StTime.day = RTC.getDate();
                            StTime.month = RTC.getMonth(century);
                            StTime.year = RTC.getYear();
                            StTime.hour = RTC.getHour(h12hflag,pmflag);
                            StTime.minute = RTC.getMinute();
                            dispBlinkSelect = YEAR;
                            break;
                        case ST_MENU_MAIN_DISPLAY:
                            dispMainDisplayCurrent = ST_MD_MAIN_MENU;
                            dispMainDisplayIndex = 0;                            
                            break;
                        case ST_MENU_BACKLIGHT:
                            backLightOptionIndex = findIndex(backLightOptionTime, backLightTime);
                            break;
                        case ST_MENU_TIMEOUT_MENU:
                            menuTimeoutOptionIndex = findIndex(menuTimeoutOptionTime, menuTimeout);
                            break;
                        case ST_MENU_TIMEOUT_MANUAL_WT:
                            manualTimeoutOptinIndex = findIndex(manualTimeoutOptionTime, manualTimeout);
                            break;
                        case ST_MENU_RECORD_FREQUENCY:
                            recordFrequencyOptionIndex = findIndex(htRecordFrequencyOptionTime, htPeriodTime);
                            break;
                        case ST_MENU_RESET:
                            dispSTResetCurrent = ST_RS_MAIN;
                            dispSTResetIndex = 0;
                            break;
                    }
                    break;
                case ST_MENU:                       //After into each setting
                    switch (dispSTMenuIndex){
                        case ST_MENU_TIME:
                            RTC.setDate(StTime.day);
                            RTC.setMonth(StTime.month);
                            RTC.setYear(StTime.year);
                            RTC.setHour(StTime.hour);
                            RTC.setMinute(StTime.minute);
                            RTC.setSecond(0);
                            dispSettingCurrent = ST_MAIN;
                            break;
                        case ST_MENU_MAIN_DISPLAY:
                            switch (dispMainDisplayCurrent){
                                case ST_MD_MAIN_MENU:
                                    dispMainDisplayCurrent = ST_MD_SUB_MENU;
                                    switch (dispMainDisplayIndex){
                                        case MD_STYLE:
                                            mainDisplayOptionStyleIndex = findIndex(mainDisplayOptionStyle, mainDisplayStyle);
                                            break;
                                        case MD_CLASSIC:
                                            mainDisplayOptionTimeSetting = mainDisplayClassicSwitchTime;
                                            break;
                                        case MD_HT:
                                            mainDisplayOptionTimeSetting = mainDisplayHTUpdateTime;
                                            break;
                                        case MD_DUAL:
                                            mainDisplayOptionTimeSetting = mainDisplayDualSwitchTime;
                                            break;
                                    }
                                    break;
                                case ST_MD_SUB_MENU:
                                    switch (dispMainDisplayIndex){
                                        case MD_STYLE:
                                            eepMainDispalyStyleSave();
                                            break;
                                        case MD_CLASSIC:
                                            eepMainDisplayClassicSave();
                                            break;
                                        case MD_HT:
                                            eepMainDisplayHTSave();
                                            break;
                                        case MD_DUAL:
                                            eepMainDisplayDualSave();
                                            break;
                                    }
                                    dispMainDisplayCurrent = ST_MD_MAIN_MENU;
                                    break;
                            }
                            break;
                        case ST_MENU_BACKLIGHT:
                            eepBacklightSave();
                            dispSettingCurrent = ST_MAIN;
                            break;
                        case ST_MENU_TIMEOUT_MENU:
                            eepMenuTimeoutSave();
                            dispSettingCurrent = ST_MAIN;
                            break;
                        case ST_MENU_TIMEOUT_MANUAL_WT:
                            eepManualWTTimeoutSave();
                            dispSettingCurrent = ST_MAIN;
                            break;
                        case ST_MENU_RECORD_FREQUENCY:
                            eepRecordFrequencySave();
                            dispSettingCurrent = ST_MAIN;
                            break;
                        case ST_MENU_RESET:
                            switch (dispSTResetCurrent){
                                case ST_RS_MAIN:
                                    dispSTResetCurrent = ST_RS_MENU;
                                    dispBlinkSelect = NO;
                                    break;
                                case ST_RS_MENU:
                                    if (dispBlinkSelect == YES){
                                        switch (dispSTResetIndex){
                                            case RS_ADJUST_WT:
                                                eepWaterTimeReset();
                                                break;
                                            case RS_DEFAULT:
                                                eepDefaultReset();
                                                break;
                                        }
                                    }
                                    dispSettingCurrent = ST_MAIN;
                            }
                            break;
                    }
                    break;
            }
            break;
    }
    LCD.clear();
}

void btBackPress(){
    switch (dispStateCurrent){
        case DS_ADJUST_WT:
            switch (dispAdjustWTCurrent){
                case WT_MAIN:
                    dispStateCurrent = DS_MAIN;
                    break;
                case WT_SETTING:
                    switch (dispAdjWTSetingCurrent){
                        case WT_ST_MAIN:
                            dispAdjustWTCurrent = WT_MAIN;
                            break;
                        case WT_ST_SUB:
                            if (!newAdjustWaterState)
                                dispAdjWTSetingCurrent = WT_ST_MAIN;
                            else
                                dispAdjustWTCurrent = WT_MAIN;
                            break;
                    }
                    break;
            }
            dispBlinkSelect = HOUR;
            break;
        case DS_DATALOG:
            dispStateCurrent = DS_MAIN;
            break;
        case DS_SETTING:
            switch (dispSettingCurrent){
                case ST_MAIN:
                    dispStateCurrent = DS_MAIN;
                    dispBlinkSelect = HOUR;
                    break;
                case ST_MENU:
                    switch (dispSTMenuIndex){
                        case ST_MENU_MAIN_DISPLAY:
                            switch (dispMainDisplayCurrent){
                                case ST_MD_MAIN_MENU:
                                    dispSettingCurrent = ST_MAIN;
                                    break;
                                case ST_MD_SUB_MENU:
                                    dispMainDisplayCurrent = ST_MD_MAIN_MENU;
                                    break;
                            }
                            break;
                        case ST_MENU_RESET:
                            switch (dispSTResetCurrent){
                                case ST_RS_MAIN:
                                    dispSettingCurrent = ST_MAIN;
                                    break;
                                case ST_RS_MENU:
                                    dispSTResetCurrent = ST_RS_MAIN;
                                    break;
                            }
                            break;
                        default:
                            dispSettingCurrent = ST_MAIN;
                    }
                    break;
            }
    }
    LCD.clear();
}

//************ Adjust StTime or Period Function *************//
void adjustStTimeOrPeriodUp(uint8_t stepMinute, uint8_t stepSecond){
    bool end30DayOfMonth = false;
    bool end28DayOfMonth = false;
    blinkState = true;
    blinkCount = 0;
    switch (dispBlinkSelect){
        case DAY:
            StTime.day++;
            switch (StTime.month){
                case 4:
                case 6:
                case 9:
                case 11:
                    end30DayOfMonth = true;
                    break;
                case 2:
                    end28DayOfMonth = true;
                    break;
            }
            if ((!end30DayOfMonth && end28DayOfMonth && StTime.day > 28) ||
                (!end30DayOfMonth && !end28DayOfMonth && StTime.day > 30) || 
                (end30DayOfMonth && StTime.day > 31)){
                    StTime.day = 1;
            }
            break;
        case MONTH:
            StTime.month++;
            if (StTime.month > 12){
                StTime.month = 1;
            }
            break;
        case YEAR:
            StTime.year++;
            if (StTime.year > 99){
                StTime.year = 0;
            }
            break;
        case HOUR:
            StTime.hour++;
            if (StTime.hour >= 24)
                StTime.hour = 0;
            break;
        case MINUTE:
            StTime.minute += stepMinute;
            if (StTime.minute >= 60)
                StTime.minute = 0;
            break;
        case PERIOD_TIME:
            StTime.periodTime += stepSecond;
            if (StTime.periodTime >= 900)
                StTime.periodTime = 0;
            break;
    }
}

void adjustStTimeOrPeriodDown(uint8_t stepMinute, uint8_t stepSecond){
    bool end30DayOfMonth = false;
    bool end28DayOfMonth = false;
    blinkState = true;
    blinkCount = 0;
    switch (dispBlinkSelect){
        case DAY:
            StTime.day--;
            switch (StTime.month){
                case 4:
                case 6:
                case 9:
                case 11:
                    end30DayOfMonth = true;
                    break;
                case 2:
                    end28DayOfMonth = true;
                    break;
            }
            if (StTime.day <= 0){
                if (end30DayOfMonth){
                    StTime.day = 30;
                } else {
                    if (end28DayOfMonth){
                        StTime.day = 28;
                    } else {
                        StTime.day = 31;
                    }
                }
            }
            break;
        case MONTH:
            StTime.month--;
            if (StTime.month <= 0){
                StTime.month = 12;
            }
            break;
        case YEAR:
            StTime.year--;
            if (StTime.year <= 0){
                StTime.year = 99;
            }
            break;
        case HOUR:
            StTime.hour--;
            if (StTime.hour < 0)
                StTime.hour = 23;
            break;
        case MINUTE:
            StTime.minute -= stepMinute;
            if (StTime.minute < 0)
                StTime.minute = (60 - stepMinute);
            break;
        case PERIOD_TIME:
            StTime.periodTime -= stepSecond;
            if (StTime.periodTime < 0)
                StTime.periodTime = 0;
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------Display FUNCTION------------------------------//
///////////////////////////////////////////////////////////////////////////////////
void dispMain(){
    LCD.home();
    switch (mainDisplayStyle){
        case MN_STT_CONTROLLER_ONLY:
            if (millis() >= mainDisplayAct){
                if (dispMeasurementIndex < RH)
                    dispMeasurementIndex++;
                else
                    dispMeasurementIndex = 0;
                setMainDisplayAct();
                LCD.clear();
            }
            dispMainClassic();
            break;
        case MN_STT_HT_ONLY:
            if (millis() >= mainDisplayAct){
                setMainDisplayAct();
            }
            dispMainHT();
            break;
        case MN_STT_CONTROLLER_HT:
            if (millis() >= mainDisplayAct){
                if (mainDisplayIndex < MN_HT)
                    mainDisplayIndex++;
                else
                    mainDisplayIndex = 0;
                setMainDisplayAct();
                LCD.clear();
            }
            switch (mainDisplayIndex){
                case MN_CONTROLLER:
                    dispMainClassic();
                    break;
                case MN_HT:
                    dispMainHT();
                    break;
            }
            break;
    }
}

void dispMainClassic(){
    LCD.home();
    
    //Display Date
    printDate(RTC.getDate(), RTC.getMonth(century), RTC.getYear() , false);
    printSpace(2);
    //Display Time
    printAdjustHourMinute(RTC.getHour(h12hflag,pmflag), RTC.getMinute(), false);

    //Display Measurement in Main Display
    switch (dispMeasurementIndex){
        case TEMP_IN:   //Display Temp in RTC Module
            LCD.setCursor(0, 1);
            LCD.print("Board:");
            printSpace(2);
            LCD.print(RTC.getTemperature());
            LCD.print(char(0));
            LCD.print("C");
            break;
        case TEMP_OUT:
            LCD.setCursor(1, 1);
            LCD.print("Temp:");
            printSpace(2);
            LCD.print(mainDisplayTemp);
            LCD.print(char(0));
            LCD.print("C");
            break;
        case RH:
            LCD.setCursor(3, 1);
            LCD.print("RH:");
            printSpace(3);
            LCD.print(mainDisplayRH);
            LCD.print("%");
            break;
    }
}

void dispMainHT(){
    LCD.print("Temp");
    printSpace(5);
    LCD.print(mainDisplayTemp);
    LCD.print(char(0));
    LCD.print("C");
    
    LCD.setCursor(0,1);

    LCD.print("Humidity");
    printSpace(1);
    LCD.print(mainDisplayRH);
    printSpace(1);
    LCD.print("%");    
}

void dispAdjustWTMain(){
    //Get Count of All WaterTime is Active
    uint8_t allWTActive = getAllActiveWaterTime();

    //Line1 Section
    LCD.home();
    LCD.print("WaterTime");
    printNumPage(dispAdjustWTIndex + 1, allWTActive);

    //Line2 Section
    if (getAllActiveWaterTime() == 0){
        newAdjustWaterState = true;
    }

    LCD.setCursor(0, 1);
    if(!newAdjustWaterState){
        //WaterTime Section
        preWaterTime(wtStartTime[dispAdjustWTIndex]);
        LCD.print(dispFillCharFullDigit(StTime.hour, '0', 2));
        LCD.print(":");
        LCD.print(dispFillCharFullDigit(StTime.minute, '0', 2));

        //PeriodTime Section
        prePeriodTime(wtPeriodTime[dispAdjustWTIndex]);
        printSpace(1);
        LCD.print("P:");
        printPeriod(StTime.periodTime, false);
        printSpace(1);
        if (dispAdjustWTIndex <= 0)
            LCD.print(char(3));     //UpDownSymbol
        else
            LCD.print(char(1));     //UpDownSymbol
    } else {                        //Add new Water Time
        StTime.hour = 0;
        StTime.minute = 0;
        LCD.print("Add New Time   ");
        LCD.print(char(2));         //UpSymbol
    }
}

void dispAdjustWTAdjust(){
    blink();

    //Line1
    LCD.home();
    LCD.print("WaterTime");
    printSpace(2);
    printAdjustHourMinute(StTime.hour, StTime.minute, true);

    //Line2
    LCD.setCursor(0 , 1);
    printSpace(3);
    LCD.print("Period");
    printSpace(1);
    
    printPeriod(StTime.periodTime, true);
}

void dispAdjustWTSetting(){
    LCD.home();    
    switch (dispAdjWTSetingCurrent){
        case WT_ST_MAIN:
            //Line1
            LCD.print("WaterTime");
            //Line2
            LCD.setCursor(0 ,1);
            switch (dispAdjWTSettingSubIndex){
                case WT_ST_SUB_EDIT:
                    LCD.print("1.Edit");
                    break;
                case WT_ST_SUB_DELETE:
                    LCD.print("2.Delete");
                    break;
                case WT_ST_SUB_ADD:
                    LCD.print("3.Add");
                    break;
            }
            break;
        case WT_ST_SUB:
            switch (dispAdjWTSettingSubIndex){
                case WT_ST_SUB_ADD:
                case WT_ST_SUB_EDIT:
                    dispAdjustWTAdjust();
                    break;
                case WT_ST_SUB_DELETE:
                    LCD.print("Confirm..");
                    LCD.setCursor(0, 1);
                    LCD.print("Press Enter");
                    break;
            }
    }
}

void dispSetting(){
    LCD.home();
    switch (dispSettingCurrent){
        case ST_MAIN:
            //Line1
            LCD.print("Setting");
            //Line2
            LCD.setCursor(0, 1);
            switch (dispSTMenuIndex){
                case ST_MENU_TIME:
                    LCD.print(DISP_ST_MENU_TIME);
                    printSpace(3);
                    break;
                case ST_MENU_MAIN_DISPLAY:
                    LCD.print(DISP_ST_MENU_MAIN_DISPLAY);
                    printSpace(3);
                    break;
                case ST_MENU_BACKLIGHT:
                    LCD.print(DISP_ST_MENU_BACKLIGHT);
                    printSpace(1);
                    break;
                case ST_MENU_TIMEOUT_MENU:
                    LCD.print(DISP_ST_MENU_TIMEOUT_MENU);
                    printSpace(1);
                    break;
                case ST_MENU_TIMEOUT_MANUAL_WT:
                    LCD.print(DISP_ST_MENU_TIMEOUT_MANUAL_WT);
                    printSpace(3);
                    break;
                case ST_MENU_RECORD_FREQUENCY:
                    LCD.print(DISP_ST_MENU_RECORD_FREQUENCY);
                    printSpace(4);
                    break;
                case ST_MENU_RESET:
                    LCD.print(DISP_ST_MENU_RESET);
                    printSpace(10);
                    break;
            }

            if (dispSTMenuIndex == 0)
                LCD.print(char(3));
            else if (dispSTMenuIndex >= ST_MENU_RESET)
                LCD.print(char(2));
            else
                LCD.print(char(1));

            break;
        case ST_MENU:
            switch (dispSTMenuIndex){
                case ST_MENU_TIME:
                    dispTimeSetting();
                    break;
                case ST_MENU_MAIN_DISPLAY:
                    dispMainDisplay();
                    break;
                case ST_MENU_BACKLIGHT:
                    dispBackLightSetting();
                    break;
                case ST_MENU_TIMEOUT_MENU:
                    dispTimeOutMenuSetting();
                    break;
                case ST_MENU_TIMEOUT_MANUAL_WT:
                    dispManualTimeOutSetting();
                    break;
                case ST_MENU_RECORD_FREQUENCY:
                    dispRecordFrequency();
                    break;
                case ST_MENU_RESET:
                    dispResetSetting();
                    break;
            }
            break;
    }
}

void dispTimeSetting(){
    //Line1
    LCD.home();
    LCD.print(DISP_ST_MENU_TIME);

    //Line2
    LCD.setCursor(0, 1);
    blink();
    printDate(StTime.day, StTime.month, StTime.year, true);
    printSpace(3);
    printAdjustHourMinute(StTime.hour, StTime.minute, true);
}

void dispMainDisplay(){
    switch (dispMainDisplayCurrent){
        case ST_MD_MAIN_MENU:
            switch (dispMainDisplayIndex){
                case MD_STYLE:
                    LCD.print(DISP_ST_MENU_MAIN_DISPLAY);
                    LCD.setCursor(0, 1);
                    LCD.print("Style");
                    printSpace(10);
                    LCD.print(char(3));
                    break;
                case MD_CLASSIC:
                    LCD.print("Classic Style");
                    LCD.setCursor(0, 1);
                    LCD.print("Change Time");
                    printSpace(4);
                    LCD.print(char(1));
                    break;
                case MD_HT:
                    LCD.print("Temp & RH Style");
                    LCD.setCursor(0, 1);
                    LCD.print("Update Time");
                    printSpace(4);
                    LCD.print(char(1));
                    break;
                case MD_DUAL:
                    LCD.print("Dual Style");
                    LCD.setCursor(0, 1);
                    LCD.print("Change Time");
                    printSpace(4);
                    LCD.print(char(2));
                    break;
            }
            break;
        case ST_MD_SUB_MENU:
            switch (dispMainDisplayIndex){
                case MD_STYLE:
                    LCD.print("Style");
                    LCD.setCursor(0, 1);
                    switch (mainDisplayOptionStyleIndex){                        
                        case MN_STT_CONTROLLER_ONLY:
                            LCD.print("Classic Only");
                            printSpace(3);
                            LCD.print(char(3));
                            break;
                        case MN_STT_HT_ONLY:
                            LCD.print("Temp & RH Only");
                            printSpace(1);
                            LCD.print(char(1));
                            break;
                        case MN_STT_CONTROLLER_HT:
                            LCD.print("Dual Display");
                            printSpace(3);
                            LCD.print(char(2));
                            break;
                    }
                    break;
                case MD_CLASSIC:
                    LCD.print("Change Time");
                    LCD.setCursor(0, 1);
                    printOptionTime(mainDisplayOptionTimeSetting, MAX_MD_CLASSIC, "s");
                    break;
                case MD_HT:
                    LCD.print("Update Time");
                    LCD.setCursor(0, 1);
                    printOptionTime(mainDisplayOptionTimeSetting, MAX_MD_HT, "s");
                    break;
                case MD_DUAL:
                    LCD.print("Change Time");
                    LCD.setCursor(0, 1);
                    printOptionTime(mainDisplayOptionTimeSetting, MAX_MD_DUAL, "s");
                    break;
            }
            break;
    }
}

void dispBackLightSetting(){
    LCD.home();
    LCD.print(DISP_ST_MENU_BACKLIGHT);
    LCD.setCursor(0, 1);
    printOptionTime(backLightOptionTime, backLightOptionIndex, NUM_BACKLIGHT_OPTION, "m");
}

void dispTimeOutMenuSetting(){
    LCD.home();
    LCD.print(DISP_ST_MENU_TIMEOUT_MENU);
    LCD.setCursor(0, 1);
    printOptionTime(menuTimeoutOptionTime, menuTimeoutOptionIndex, NUM_MENU_TIMEOUT, "s");
}

void dispManualTimeOutSetting(){
    LCD.home();
    LCD.print(DISP_ST_MENU_TIMEOUT_MANUAL_WT);
    LCD.setCursor(0, 1);
    printOptionTime(manualTimeoutOptionTime, manualTimeoutOptinIndex, NUM_MANUAL_TIMOUT, "m");
}

void dispResetSetting(){
    switch (dispSTResetCurrent){
        case ST_RS_MAIN:
            //Line1
            LCD.home();
            LCD.print(DISP_ST_MENU_RESET);
            //Line2
            LCD.setCursor(0, 1);
            switch (dispSTResetIndex){
                case RS_ADJUST_WT:
                    LCD.print(DISP_ST_RS_ADJUST_WT);
                    printSpace(6);
                    LCD.print(char(3));
                    break;
                case RS_DEFAULT:
                    LCD.print(DISP_ST_RS_DEFAULT);
                    printSpace(1);
                    LCD.print(char(2));
                    break;
            }
            break;
        case ST_RS_MENU:
            dispReset();
            break;
    }
}

void dispRecordFrequency(){
    LCD.print(DISP_ST_MENU_RECORD_FREQUENCY);
    LCD.setCursor(0, 1);
    printOptionTime(htRecordFrequencyOptionTime, recordFrequencyOptionIndex, NUM_RECORD_FREQUENCY, "m");
}

void dispReset(){
    LCD.home();
    LCD.print("Reset");
    printSpace(1);
    switch (dispSTResetIndex){
        case RS_ADJUST_WT:
            LCD.print(DISP_ST_RS_ADJUST_WT);
            break;
        case RS_DEFAULT:
            LCD.print("Default");
            break;
    }
    LCD.print("?");
    blink();
    printSelectYesNo();
}

void dispDataLogMoniter(){
    DataLog dl;
    dlEEPROM.get(dlmPointer * sizeof(struct DataLog), dl);
    DateTime dt(dl.dateTime);
    //Line1
    LCD.home();
    LCD.print(dispFillCharFullDigit(dt.day(), '0', 2));
    LCD.print("/");
    LCD.print(dispFillCharFullDigit(dt.month(), '0', 2));
    printSpace(1);
    LCD.print(dispFillCharFullDigit(dt.hour(), '0', 2));
    LCD.print(":");
    LCD.print(dispFillCharFullDigit(dt.minute(), '0', 2));

    printSpace(1);
    LCD.print("#");
    LCD.print(dispFillCharFullDigit(dlmPointer + 1, '0', 3));
    //Line2
    LCD.setCursor(0, 1);
    LCD.print(dl.temp);
    LCD.print(char(0));
    LCD.print("C");

    printSpace(3);
    LCD.print(dl.rh);
    LCD.print("%");
}

//************ Calculator blink time *************//
void blink(){
    if (blinkCount >= blinkTime){
        blinkCount = 0;
        blinkState = !blinkState;
    } else {        
        blinkCount++;
    }
}

//************ Prepare variables for Adjust Water Time *************//

//WaterTime (minute to hour & minute)
void preWaterTime(int minute){
    StTime.hour = minute / 60;
    StTime.minute = minute % 60;
}

//PeriodTime
void prePeriodTime(int second){
    StTime.periodTime = second;
}

//************ Fill charactor function *************//
String dispFillCharFullDigit(int t, char fill, uint8_t digit){
    String s = String(t);
    while (s.length() < digit){
        s = fill + s;
    }
    return s;
}

void fillSpaceForArrow(int t, int maxDigit, int spaceForMaxDigit){
    if (t < maxDigit)
        printSpace(spaceForMaxDigit);
    else
        printSpace(spaceForMaxDigit - 1);
}

//************ find index of Option Setting *************//
int findIndex(uint8_t a[], uint8_t find){
    for (int i = 0; i < NUM_BACKLIGHT_OPTION; i++){
        if (a[i] == find){
            return i;
        }
    }
}

//************ Print common format on LCD *************//
void printSpace(uint8_t n){
    for (int i = 0; i < n; i++){
        LCD.print(" ");
    }
}

void printDate(int date, int month, int year, bool onBlink){
    if (blinkState && dispBlinkSelect == DAY || (dispBlinkSelect != DAY) || !onBlink)
        LCD.print(dispFillCharFullDigit(date, '0', 2));
    else
        printSpace(2);

    LCD.print("/");

    if (blinkState && dispBlinkSelect == MONTH || (dispBlinkSelect != MONTH) || !onBlink)
        LCD.print(dispFillCharFullDigit(month, '0', 2));
    else
        printSpace(2);

    LCD.print("/");

    if (blinkState && dispBlinkSelect == YEAR || (dispBlinkSelect != YEAR) || !onBlink)
        LCD.print(dispFillCharFullDigit(year, '0', 2));
    else
        printSpace(2);
}

void printPeriod(int period, bool onBlink){
    String minute = dispFillCharFullDigit(period / 60, ' ', 2);
    String second = dispFillCharFullDigit(period % 60, '0', 2);
    if (blinkState && dispBlinkSelect == PERIOD_TIME || dispBlinkSelect != PERIOD_TIME || !onBlink){
        LCD.print(minute);
        LCD.print(":");
        LCD.print(second);
        LCD.print("s");
    } else {
        printSpace(6);
    }
}

void printAdjustHourMinute(int hour, int minute, bool onBlink){
    if (blinkState && dispBlinkSelect == HOUR || (dispBlinkSelect != HOUR) || !onBlink){
        LCD.print(dispFillCharFullDigit(hour, '0', 2));
    } else {
        printSpace(2);
    }

    LCD.print(":");

    if ((dispBlinkSelect == MINUTE && blinkState) || (dispBlinkSelect != MINUTE) || !onBlink){
        LCD.print(dispFillCharFullDigit(minute, '0', 2));
    } else {
        printSpace(2);
    }
}

void printSelectYesNo(){
    LCD.setCursor(3, 1);
    if ((dispBlinkSelect == YES && blinkState) || dispBlinkSelect != YES){
        LCD.print("Yes");
    } else {
        printSpace(3);
    }
    printSpace(5);
    if ((dispBlinkSelect == NO && blinkState) || dispBlinkSelect != NO){
        LCD.print("No");
    } else {
        printSpace(2);
    }
}

void printOptionTime(uint8_t optionTime[], int optionTimeIndex, uint8_t optionTimeMax , String unit){
    uint8_t t = optionTime[optionTimeIndex];
    if (t != 0){
        LCD.print(t);
        LCD.print(unit);
        fillSpaceForArrow(t, 10, 13);
    } else {
        LCD.print("No Limit");
        printSpace(7);
    }
    if (optionTimeIndex == 0)
        LCD.print(char(2));
    else if (optionTimeIndex >= optionTimeMax -1)
        LCD.print(char(3));
    else
        LCD.print(char(1));
}

void printOptionTime(uint8_t time, uint8_t timeMax, String unit){
    LCD.print(time);
    LCD.print(unit);
    fillSpaceForArrow(time, timeMax, 13);
    if (time <= 1)
        LCD.print(char(2));
    else if (time >= timeMax)
        LCD.print(char(3));
    else
        LCD.print(char(1));
}

void printNumPage(uint8_t currentPage, uint8_t  allPage){
    int digitCurrentPage, digitAllPage;
    if (currentPage < 10)
        digitCurrentPage = 1;
    else 
        digitCurrentPage = 2;
    
    if (allPage < 10)
        digitAllPage = 1;
    else
        digitAllPage = 2;

    int row = LCD_COLS - (digitCurrentPage + digitAllPage +1);
    LCD.setCursor(row, 0);
    LCD.print(currentPage);
    LCD.print("/");
    LCD.print(allPage);
}

///////////////////////////////////////////////////////////////////////////////////
//---------------------------------EEPROM FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////
void eepAllWaterTimeLoad(){
    //Start Water Time
    wtStartTime[0] = eepStartWT1;
    wtStartTime[1] = eepStartWT2;
    wtStartTime[2] = eepStartWT3;
    wtStartTime[3] = eepStartWT4;
    wtStartTime[4] = eepStartWT5;
    wtStartTime[5] = eepStartWT6;
    wtStartTime[6] = eepStartWT7;
    wtStartTime[7] = eepStartWT8;
    wtStartTime[8] = eepStartWT9;
    wtStartTime[9] = eepStartWT10;
    wtStartTime[10] = eepStartWT11;
    wtStartTime[11] = eepStartWT12;
    wtStartTime[12] = eepStartWT13;
    wtStartTime[13] = eepStartWT14;
    wtStartTime[14] = eepStartWT15;
    wtStartTime[15] = eepStartWT16;
    //Period Water Time
    wtPeriodTime[0] = eepPeriodWT1;
    wtPeriodTime[1] = eepPeriodWT2;
    wtPeriodTime[2] = eepPeriodWT3;
    wtPeriodTime[3] = eepPeriodWT4;
    wtPeriodTime[4] = eepPeriodWT5;
    wtPeriodTime[5] = eepPeriodWT6;
    wtPeriodTime[6] = eepPeriodWT7;
    wtPeriodTime[7] = eepPeriodWT8;
    wtPeriodTime[8] = eepPeriodWT9;
    wtPeriodTime[9] = eepPeriodWT10;
    wtPeriodTime[10] = eepPeriodWT11;
    wtPeriodTime[11] = eepPeriodWT12;
    wtPeriodTime[12] = eepPeriodWT13;
    wtPeriodTime[13] = eepPeriodWT14;
    wtPeriodTime[14] = eepPeriodWT15;
    wtPeriodTime[15] = eepPeriodWT16;
}

void eepAllWaterTimeSave(){
    //Start Water Time
    eepStartWT1 = wtStartTime[0];
    eepStartWT2 = wtStartTime[1];
    eepStartWT3 = wtStartTime[2];
    eepStartWT4 = wtStartTime[3];
    eepStartWT5 = wtStartTime[4];
    eepStartWT6 = wtStartTime[5];
    eepStartWT7 = wtStartTime[6];
    eepStartWT8 = wtStartTime[7];
    eepStartWT9 = wtStartTime[8];
    eepStartWT10 = wtStartTime[9];
    eepStartWT11 = wtStartTime[10];
    eepStartWT12 = wtStartTime[11];
    eepStartWT13 = wtStartTime[12];
    eepStartWT14 = wtStartTime[13];
    eepStartWT15 = wtStartTime[14];
    eepStartWT16 = wtStartTime[15];
    //Period Water Time
    eepPeriodWT1 = wtPeriodTime[0];
    eepPeriodWT2 = wtPeriodTime[1];
    eepPeriodWT3 = wtPeriodTime[2];
    eepPeriodWT4 = wtPeriodTime[3];
    eepPeriodWT5 = wtPeriodTime[4];
    eepPeriodWT6 = wtPeriodTime[5];
    eepPeriodWT7 = wtPeriodTime[6];
    eepPeriodWT8 = wtPeriodTime[7];
    eepPeriodWT9 = wtPeriodTime[8];
    eepPeriodWT10 = wtPeriodTime[9];
    eepPeriodWT11 = wtPeriodTime[10];
    eepPeriodWT12 = wtPeriodTime[11];
    eepPeriodWT13 = wtPeriodTime[12];
    eepPeriodWT14 = wtPeriodTime[13];
    eepPeriodWT15 = wtPeriodTime[14];
    eepPeriodWT16 = wtPeriodTime[15];
}

void eepWaterTimeSave(){
    int wt = (StTime.hour * 60) + StTime.minute;
    wtStartTime[dispAdjustWTIndex] = wt;
    wtPeriodTime[dispAdjustWTIndex] = StTime.periodTime;

    //clear operate time for close equal zero
    for(int i = 0; i < WATER_TIME_NUM; i++){
        if(wtPeriodTime[i] == 0){
            wtStartTime[i] = 0;
        }
    }
    //remove duplicates
    for(int i = WATER_TIME_NUM -1; i > 0; i--){
        int idx = i;
        for(int j = 0; j < i; j++){
        if(wtStartTime[j] == wtStartTime[i]){
            wtStartTime[j] = 0;
            wtPeriodTime[j] = 0;
        }
        }
    }

    //romove deactive operate time between active operate time
    rmDeactiveWT();

    //sort
    spWTSort();

    //find and set position of operate time current
    for (int i = 0; i < WATER_TIME_NUM; i++){
        if (wtStartTime[i] == wt && (wtStartTime[i] != 0 && wtPeriodTime[i] != 0))
            dispAdjustWTIndex = i;
    }

    eepAllWaterTimeSave();
}

void eepAllSettingLoad(){
    backLightTime = eepSettingBackLight;
    menuTimeout = eepSettingTOMenu;
    manualTimeout = eepSettingTOManualWT;
    htPeriodTime = eepSettingRecordFrequency;
    mainDisplayStyle = eepSettingMainDisplayStyle;
    mainDisplayClassicSwitchTime = eepSettingMainDisplayClassicSwitch;
    mainDisplayHTUpdateTime = eepSettingMainDisplayHTUpdate;
    mainDisplayDualSwitchTime = eepSettingMainDisplayDualSwitch; //NoSure
}

void eepBacklightSave(){
    uint8_t backlightT = backLightOptionTime[backLightOptionIndex];
    backLightTime = backlightT;
    eepSettingBackLight = backlightT;
    
    setBackLightAct();
}

void eepMenuTimeoutSave(){
    uint8_t menuTimeoutT = menuTimeoutOptionTime[menuTimeoutOptionIndex];
    menuTimeout = menuTimeoutT;
    eepSettingTOMenu = menuTimeoutT;

    setMenuTimeOutAct();
}

void eepManualWTTimeoutSave(){
    uint8_t manualWTT = manualTimeoutOptionTime[manualTimeoutOptinIndex];
    manualTimeout = manualWTT;
    eepSettingTOManualWT = manualWTT;
}

void eepRecordFrequencySave(){
    uint8_t htRecordFrequencyT = htRecordFrequencyOptionTime[recordFrequencyOptionIndex];
    htPeriodTime = htRecordFrequencyT;
    eepSettingRecordFrequency = htRecordFrequencyT;
}

void eepMainDispalyStyleSave(){
    uint8_t mainDisplayStyleT = mainDisplayOptionStyle[mainDisplayOptionStyleIndex];
    mainDisplayStyle = mainDisplayStyleT;
    eepSettingMainDisplayStyle = mainDisplayStyleT;
}

void eepMainDisplayClassicSave(){
    mainDisplayClassicSwitchTime = mainDisplayOptionTimeSetting;
    eepSettingMainDisplayClassicSwitch = mainDisplayOptionTimeSetting;
}

void eepMainDisplayHTSave(){
    mainDisplayHTUpdateTime = mainDisplayOptionTimeSetting;
    eepSettingMainDisplayHTUpdate = mainDisplayOptionTimeSetting;
}

void eepMainDisplayDualSave(){
    mainDisplayDualSwitchTime = mainDisplayOptionTimeSetting;
    eepSettingMainDisplayDualSwitch = mainDisplayOptionTimeSetting;
}

void eepWaterTimeReset(){
    eepStartWT1 = 0;
    eepStartWT2 = 0;
    eepStartWT3 = 0;
    eepStartWT4 = 0;
    eepStartWT5 = 0;
    eepStartWT6 = 0;
    eepStartWT7 = 0;
    eepStartWT8 = 0;
    eepStartWT9 = 0;
    eepStartWT10 = 0;
    eepStartWT11 = 0;
    eepStartWT12 = 0;
    eepStartWT13 = 0;
    eepStartWT14 = 0;
    eepStartWT15 = 0;
    eepStartWT16 = 0;

    eepPeriodWT1 = 0;
    eepPeriodWT2 = 0;
    eepPeriodWT3 = 0;
    eepPeriodWT4 = 0;
    eepPeriodWT5 = 0;
    eepPeriodWT6 = 0;
    eepPeriodWT7 = 0;
    eepPeriodWT8 = 0;
    eepPeriodWT9 = 0;
    eepPeriodWT10 = 0;
    eepPeriodWT11 = 0;
    eepPeriodWT12 = 0;
    eepPeriodWT13 = 0;
    eepPeriodWT14 = 0;
    eepPeriodWT15 = 0;
    eepPeriodWT16 = 0;

    eepAllWaterTimeLoad();
}

void eepDefaultReset(){
    uint8_t backLightT = BACKLIGHT_DEFAULT;
    uint8_t menuTimeoutT = MENU_TIMEOUT_DEFAULT;
    uint8_t timeOutWT = MANUAL_TIMEOUT_DEFAULT;
    uint8_t htPeriodTimeT = RECORD_FREQUENCY_DEFAULT;
    uint8_t mainDisplayStyleT = MAINDISPLAY_STYLE_DEFAULT;
    uint8_t mainDisplayClassicSwitchTimeT = MAINDISPLAY_CLASSIC_SWITCH_DEFAULT;
    uint8_t mainDisplayUpdateHTTimeT = MAINDISPLAY_HT_UPDATE_DEFAULT;
    uint8_t mainDisplaySwitchTimeT = MAINDISPLAY_DUAL_SWITCH_DEFAULT;

    eepSettingBackLight = backLightT;
    eepSettingTOMenu = menuTimeoutT;
    eepSettingTOManualWT = timeOutWT;
    eepSettingRecordFrequency = htPeriodTimeT;
    eepSettingMainDisplayStyle = mainDisplayStyleT;
    eepSettingMainDisplayClassicSwitch = mainDisplayClassicSwitchTimeT;
    eepSettingMainDisplayHTUpdate = mainDisplayUpdateHTTimeT;
    eepSettingMainDisplayDualSwitch = mainDisplaySwitchTimeT;


    backLightTime = backLightT;
    menuTimeout = menuTimeoutT;
    manualTimeout = timeOutWT;
    htPeriodTime = htPeriodTimeT;
    mainDisplayStyle = mainDisplayStyleT;
    mainDisplayClassicSwitchTime = mainDisplayClassicSwitchTimeT;
    mainDisplayHTUpdateTime = mainDisplayUpdateHTTimeT;
    mainDisplayDualSwitchTime = mainDisplaySwitchTimeT;
}

//** romove deactive operate time between active operate for time Water Time **//
void rmDeactiveWT(){
    for(int i = 0; i < WATER_TIME_NUM; i++){
        int min_idx = i;
        if(wtStartTime[i] == 0 && wtPeriodTime[i] == 0){
            for(int j = i; j < WATER_TIME_NUM - 1; j++){
                wtStartTime[j] = wtStartTime[j + 1];
                wtPeriodTime[j] = wtPeriodTime[j +1];
            }
            wtStartTime[WATER_TIME_NUM - 1] = 0;
            wtPeriodTime[WATER_TIME_NUM - 1] = 0;
        }
    }
}

//************ Sort Start/Period Water Time *************//
void spWTSort(){
    for(int i = 0; i < WATER_TIME_NUM - 1; i++){
        int min_idx = i;
        for(int j = i + 1; j < WATER_TIME_NUM; j++){
        if(wtStartTime[j] < wtStartTime[min_idx] && wtPeriodTime[j] > 0){
            min_idx = j;
        }
        }
        int temp1 = wtStartTime[min_idx];
        int temp2 = wtPeriodTime[min_idx];
        wtStartTime[min_idx] = wtStartTime[i];
        wtPeriodTime[min_idx] = wtPeriodTime[i];
        wtStartTime[i] = temp1;
        wtPeriodTime[i] = temp2;
    }
}

///////////////////////////////////////////////////////////////////////////////////
//----------------------------Link External FUNCTION-----------------------------//
///////////////////////////////////////////////////////////////////////////////////

void link_sendDataLog(){
    for(int i = 0; i < DL_MAX; i++){
        DataLog dl;
        dlEEPROM.get(i * sizeof(struct DataLog), dl);
        DateTime dt(dl.dateTime);
        Serial.print(dt.day());
        Serial.print("/");
        Serial.print(dt.month());
        Serial.print("/");
        Serial.print(dt.year());
        Serial.print(" ");
        Serial.print(dt.hour());
        Serial.print(":");
        Serial.print(dt.minute());
        Serial.print(",");
        Serial.print(dl.temp);
        Serial.print(",");
        Serial.print(dl.rh);
        Serial.println();
    }
    Serial.println("EOF");
}

void link_sendProcess(){
    for (int i = 0; i < RP_MAX; i++){
        //RecProcess rp;
        int timeAddress =RP_START_ADR + (i * (PT_SIZE + PS_SIZE));
        int statusAddress = timeAddress + PT_SIZE;
        uint32_t timeStemp;
        uint8_t status;
        EEPROM.get(timeAddress, timeStemp);
        EEPROM.get(statusAddress, status);
        DateTime dt(timeStemp);
        Serial.print(dt.day());
        Serial.print("/");
        Serial.print(dt.month());
        Serial.print("/");
        Serial.print(dt.year());
        Serial.print(" ");
        Serial.print(dt.hour());
        Serial.print(":");
        Serial.print(dt.minute());
        Serial.print(",");
        Serial.print(status);
        Serial.println();
    }
    Serial.println("EOF");
}

///////////////////////////////////////////////////////////////////////////////////
//------------------------------BackLight FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

void setBackLightAct(){
    backLightAct = millis() + (backLightTime * 1000L * 60L);
}

///////////////////////////////////////////////////////////////////////////////////
//------------------------------BackLight FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

void setMenuTimeOutAct(){
    menuTimeOutAct = millis() + (menuTimeout * 1000L);
}

///////////////////////////////////////////////////////////////////////////////////
//---------------------------ManualWaterTime FUNCTION----------------------------//
///////////////////////////////////////////////////////////////////////////////////

void setManualTimeoutAct(){
    manualTimeOutAct = millis() + (manualTimeout * 1000L * 60L);
}

///////////////////////////////////////////////////////////////////////////////////
//--------------------------------SHT45 FUNCTION---------------------------------//
///////////////////////////////////////////////////////////////////////////////////

float getTemp(){
    if (sht45.measure()){
        return sht45.temperature();
    }
}

float getRH(){
    if (sht45.measure()){
        return sht45.humidity();
    }
}
///////////////////////////////////////////////////////////////////////////////////
//-----------------------------Main Display FUNCTION-----------------------------//
///////////////////////////////////////////////////////////////////////////////////
void setMainDisplayAct(){
    switch (mainDisplayStyle){
        case MN_STT_CONTROLLER_ONLY:
            mainDisplayAct = millis() + (mainDisplayClassicSwitchTime * 1000L);
            break;
        case MN_STT_HT_ONLY:
            mainDisplayAct = millis() + (mainDisplayHTUpdateTime * 1000L);
            break;
        case MN_STT_CONTROLLER_HT:
            mainDisplayAct = millis() + (mainDisplayDualSwitchTime * 1000L);
            break;
    }    
    mainDisplayTemp = getTemp();
    mainDisplayRH = getRH();
}

///////////////////////////////////////////////////////////////////////////////////
//--------------------------------DataLog FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

// For DataLog Monitor
void getDLMPointer(){
    LCD.print("Please wait..");
    unsigned long maxTimeStamp;
    for(int i = 0; i < DL_MAX; i++){
        DataLog dl;
        dlEEPROM.get(i * sizeof(struct DataLog), dl);
        DateTime dt(dl.dateTime);
        if (dt.unixtime() > maxTimeStamp){
            maxTimeStamp = dt.unixtime();
            dlmPointer = i;
        }
    }
}

void checkPointerDataLog(){
    if (dlPointer > DL_MAX){
        dlPointer = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////
//----------------------------Record Process FUNCTION----------------------------//
///////////////////////////////////////////////////////////////////////////////////
/*/
void getRPPointer(){
    EEPROM.get(RP_PTR, rpPointer);
    if (rpPointer > RP_MAX){
        rpPointer = 0;
        EEPROM.put(RP_PTR, (short)10);
    }
}/**/

void rpStatusSave(int status){
    uint16_t pointer = rpPointer;
    int timeAddress = RP_START_ADR + (pointer * (PT_SIZE + PS_SIZE));
    int statusAddress = timeAddress + PT_SIZE;
    EEPROM.put(timeAddress, RTClib::now().unixtime());
    EEPROM.put(statusAddress, status);
    if (pointer < RP_MAX)
        rpPointer++;
    else
        rpPointer = 0;

    Serial.print("rpPionter:");     Serial.println(rpPointer);
    Serial.print("TimeAddress:");   Serial.println(timeAddress);
    Serial.print("Time:");          Serial.println(RTClib::now().unixtime());
    Serial.print("StatusAddress:"); Serial.println(statusAddress);
    Serial.print("Status:");        Serial.println(status);
    Serial.println();
}

///////////////////////////////////////////////////////////////////////////////////
//--------------------------------COMMOND FUNCTION-------------------------------//
///////////////////////////////////////////////////////////////////////////////////

//************ Adjust Water Time *************//
uint8_t getAllActiveWaterTime(){    
    uint8_t allWTActive = 0;
    for (int i = 0; i < WATER_TIME_NUM; i++){
        if (wtStartTime[i] != 0  && wtPeriodTime !=0){
            allWTActive++;
        }
    }
    return allWTActive;
}