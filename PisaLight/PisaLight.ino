/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */
 //NODEMCU 1.0
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define RUNNING_MODE  1   
#include "FastLED.h"
#include <ESP8266WiFi.h>

#define NUM_LEDS 3
#define DATA_PIN 5
#define C_TIME 30
// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB led_cols_temp;
CRGB led_cols_wmain;
CRGB led_cols_comp;
const char* ssid     = "YG_iptime";
const char* password = "IPTIME1212";

const char* host = "192.168.0.12";
const char* D_NAME = "[[NMCU]]";
const int port = 9898;
unsigned long reconnect_timer=0;
unsigned long r_start_time=0;
unsigned long synk_time=0;
int led_pin[] = {13,14,4};
bool light_on = false;
bool light_on_old=false;
unsigned long light_start = 0;
unsigned long blink_light_start = 0;
unsigned char count = 0;
unsigned char l_step=0;
bool on_or_off = 1;

typedef struct wheather_info
{
    char weather_main[10];
    float temp;
    int compare_temp;
}wheather_info;

void TurnOffLed(void);
wheather_info getWheatherData(char* buffer);

void turnOnLed(CRGB col, CRGB* Leds, int* b_led_pin);
void turnOnBulbLed(int* b_led_pin, bool* on_off);
unsigned char lightOnOffControl(unsigned char count, unsigned char next_count, unsigned long _delay);
CRGB convertTempToLedColor(float temp);
CRGB convertWMainToLedColor(char* wheather_main);
CRGB convertTempChgToLedColor(int compare_temp);
WiFiClient client;
 
void setup() {
    Serial.begin(115200);
    delay(10);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    pinMode(13, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(14, OUTPUT);
    // We start by connecting to a WiFi network

#if RUNNING_MODE == 1
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    if (!client.connect(host, port)) {
        Serial.println("connection failed");
        return;
    }
    synk_time = (unsigned long)C_TIME*1000 + 1000;
    delay(500);
    client.println(D_NAME);
#endif 
}

void loop() {

    char buffer[16] = {0};
    char strbuffer[16] ={0};
    int i=0;
    int len = 0;
    wheather_info w_info;
    memset(&w_info,0,sizeof(w_info));
#if RUNNING_MODE == 0
    String strs;
    int fade;
    

    if (Serial.available())
    {
        strs = Serial.readString();
    }
    
#endif 

#if RUNNING_MODE == 1
    while (client.available()){
        buffer[len] = client.read();
        len++;
    }

    reconnect_timer = millis() - r_start_time;
    //if(connected_state == true)

    if(len > 0)
    {
        Serial.println(buffer);
        if(buffer[0] == 0xe1 && buffer[1] == 0x1)
        {
            if(client.connected())
            {
                synk_time = (unsigned long)C_TIME*1000 + 1000;
                sprintf(strbuffer,"\xe1\x02");
                client.println(strbuffer);
                Serial.println("Session Check");
                r_start_time = millis();
                return;
            }
        }
    }

    if(reconnect_timer >= (unsigned long)synk_time*2)
    {
        r_start_time = millis();
        if(client.connected())
        {
            sprintf(strbuffer,"ShutDown");
            client.println(strbuffer);
            Serial.println("Shut Down");
            client.stop();
        }
        else
        {
            if (client.connect(host, port)) 
            {
                synk_time = 0xFFFFFF;
                delay(500);
                client.println(D_NAME);
                Serial.print("Send Name :");
                Serial.println(D_NAME);
            }
        }
    }
#endif

    // by touch or light
#if RUNNING_MODE == 1
    else if(buffer[0] == '<')
    {
        // 아래 함수 유효성 검증 필요함 .. 

        w_info = getWheatherData(buffer);

        Serial.printf( "Wheather Main : %s, Temp : %d, Compare_temp : %d\n",\
                        w_info.weather_main,(int)w_info.temp, w_info.compare_temp);
        
        light_start = millis();
        light_on = true;
        
        led_cols_temp = convertTempToLedColor(w_info.temp);
        led_cols_wmain = convertWMainToLedColor(w_info.weather_main);
        led_cols_comp = convertTempChgToLedColor(w_info.compare_temp);
        count = 5;
    }
#else
    if(strs[0] == '<')
    {
        strs.toCharArray(strbuffer,16);
        // 아래 함수 유효성 검증 필요함 .. 

        w_info = getWheatherData(strbuffer);

        Serial.printf( "Wheather Main : %s, Temp : %d, Compare_temp : %d\n",\
                        w_info.weather_main,(int)w_info.temp, w_info.compare_temp);
        
        light_start = millis();
        light_on = true;
        
        led_cols_temp = convertTempToLedColor(w_info.temp);
        led_cols_wmain = convertWMainToLedColor(w_info.weather_main);
        led_cols_comp = convertTempChgToLedColor(w_info.compare_temp);
        count = 5;
    
    }
#endif

    if (light_on == true)
    {
        if(l_step == 0)
        {
            if (led_cols_wmain == (CRGB)CRGB::Black)
            {
                l_step++;
                count = 0;
                return;
            }
            if (on_or_off)
                turnOnLed(led_cols_wmain,leds,led_pin);
            else
                turnOnLed(CRGB::Black,leds,led_pin);
            count = lightOnOffControl(count,0,500);
        }
        else if (l_step == 1)
        {
            if(led_cols_temp == (CRGB)CRGB::Black)
            {
                l_step++;
                return;
            }
            if (on_or_off)
                turnOnLed(led_cols_temp,leds,led_pin);
            else
                turnOnLed(CRGB::Black,leds,led_pin);
                
            count = lightOnOffControl(count,20,5000);
        }
        else if(l_step == 2)
        {
            if(led_cols_comp == (CRGB)CRGB::Black)
            {
                l_step++;
                return;
            }
            if (on_or_off)
                turnOnLed(led_cols_comp,leds,led_pin);
            else
                turnOnLed(CRGB::Black,leds,led_pin);
            count = lightOnOffControl(count,0,100); 
        }
        else
        {
            l_step =0;
            count = 0;
            on_or_off =1;
            light_on = false;
            TurnOffLed();
        }
          
    }
}

unsigned char lightOnOffControl(unsigned char count, unsigned char next_count, unsigned long _delay)
{
    if(light_on_old == false)
    {
        FastLED.show();
        light_on_old = true;
    }
    
    if (millis() - light_start > _delay)
    {
        Serial.println(count);
        light_on_old = false;
        if(count == 0)
        {
            on_or_off = 1;
            l_step++;
            count = next_count;
            return count;
        }
        if (count > 0)
          count--;
        on_or_off = !on_or_off;
        light_start = millis();
    }
    return count;
}

wheather_info getWheatherData(char* buffer)
{
    char* pch;
    char* next_pch;
    char* last_pch;
    char tmpStr[10];
    wheather_info w_info;
    strcpy(w_info.weather_main,"null");
    w_info.temp = 0xff;
    w_info.compare_temp = 0xff;
    memset(tmpStr,0,sizeof(tmpStr));
    
    if((last_pch = strchr(buffer,'>'))!=NULL)
    {
         pch = strchr(buffer,'/');
         if (pch == NULL)
            return w_info;
            
         memcpy(tmpStr,&buffer[1],pch-buffer-1);
         memcpy(w_info.weather_main, tmpStr, sizeof(tmpStr));

         if ((next_pch = strchr(pch+1,'/')) ==NULL)
         {
            memset(tmpStr,0,sizeof(10));
            memcpy(tmpStr,pch+1,last_pch-pch-1);
            w_info.temp = atof(tmpStr);
            w_info.compare_temp = 0xff;
         }
         else
         {
            memset(tmpStr,0,sizeof(10));
            memcpy(tmpStr,pch+1,next_pch-pch-1);
            w_info.temp = atof(tmpStr); 

            memset(tmpStr,0,sizeof(10));
            memcpy(tmpStr,next_pch+1,last_pch-next_pch-1);
            w_info.compare_temp = atoi(tmpStr);
            
         }

    }
    return w_info;
}

void turnOnLed(CRGB col, CRGB* Leds, int* b_led_pin)
{
    int i =0;
    
    if (col == (CRGB)CRGB::Fuchsia)
    {
        bool on_off[3] = {LOW,HIGH,HIGH};
        turnOnBulbLed(b_led_pin,on_off);
    }
    
    else if (col == (CRGB)CRGB::Blue)
    { 
        bool on_off[3] = {LOW,LOW,HIGH};
        turnOnBulbLed(b_led_pin,on_off);
    }
    else if (col == (CRGB)CRGB::Aqua)
    {
        bool on_off[3] = {HIGH,LOW,HIGH};
        turnOnBulbLed(b_led_pin,on_off);
    }
    else if (col == (CRGB)CRGB::Green)
    {
        bool on_off[3] = {HIGH,LOW,LOW};
        turnOnBulbLed(b_led_pin,on_off);
    }
    else if (col == (CRGB)CRGB::Yellow)
    {
        bool on_off[3] = {HIGH,HIGH,LOW};
        turnOnBulbLed(b_led_pin,on_off);
    }
    else if (col == (CRGB)CRGB::Red)
    {
        bool on_off[3] = {LOW,HIGH,LOW};
        turnOnBulbLed(b_led_pin,on_off);
    }
    else if (col == (CRGB)CRGB::Black || \
             col == (CRGB)CRGB::Gray  )
    {
        bool on_off[3] = {LOW,LOW,LOW};
        turnOnBulbLed(b_led_pin,on_off); 
    }
    else if (col == (CRGB)CRGB::White)
    {
        bool on_off[3] = {HIGH,HIGH,HIGH};
        turnOnBulbLed(b_led_pin,on_off);   
    }
        
    for(i = 0; i < 3; i++)
        Leds[i] = col;
}

void turnOnBulbLed(int* b_led_pin, bool* on_off)
{
    digitalWrite(b_led_pin[0],on_off[0]);
    digitalWrite(b_led_pin[1],on_off[1]);
    digitalWrite(b_led_pin[2],on_off[2]);
}

CRGB convertTempToLedColor(float temp)
{
    if (temp <= 5)
        return CRGB::Fuchsia;
    else if (temp > 5 && temp <= 9 )
        return CRGB::Blue;
    else if (temp > 9 && temp <= 15)
        return CRGB::Aqua;
    else if (temp > 15 && temp <= 20)
        return CRGB::Green;
    else if (temp > 20 && temp <=25)
        return CRGB::Yellow;
    else if (temp > 25)
        return CRGB::Red;
    else if (temp == 0xFF)
        return CRGB::Black;
    
}

CRGB convertWMainToLedColor(char* wheather_main)
{
    if (!strcmp(wheather_main, "Rain"))
      return CRGB::Blue;
    else if(!strcmp(wheather_main, "Cloud"))
      return CRGB::Gray;
    else if(!strcmp(wheather_main, "Snow"))
      return CRGB::White;
    else
      return CRGB::Black;

}

CRGB convertTempChgToLedColor(int compare_temp)
{
    if (compare_temp == 0)
        return CRGB::Blue;
    else if (compare_temp == 1)
        return CRGB::Red;
    else
        return CRGB::Black;


}

void TurnOffLed(void)
{
    int i=0;
    for(i=0; i<3; i++)
        digitalWrite(led_pin[i],LOW);
    
    for(i=0; i<3; i++)
        leds[i] = CRGB::Black;
    
    FastLED.show();    
}

int freeRam(void) {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}



