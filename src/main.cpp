#include <Arduino.h>

//for lcd
#include <LiquidCrystal.h>

//for button callbacks
#include <Button.h>
#include <ButtonEventCallback.h>
#include <PushButton.h>

//timer for turning on welding
#include <Timer.h>

// for saving settings in the memory
#include <EEPROM.h>


//comment row below to turn off debig messages to serial
#define SERIAL_LOG
#if defined(SERIAL_LOG)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Module connection pins (Digital Pins)
#define RS 5
#define EN 6
#define D4 9
#define D5 10
#define D6 11
#define D7 12

//buttons settings
#define OK_BUTTON 2
#define FORWARD_BUTTON 3
#define BACKWARD_BUTTON 4

//pin to control relay
#define RELAY_PIN A0

//welding timer
Timer t;

//init display
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Create buttons
PushButton ok_button = PushButton(OK_BUTTON, ENABLE_INTERNAL_PULLUP);
PushButton forward_button = PushButton(FORWARD_BUTTON, ENABLE_INTERNAL_PULLUP);
PushButton backward_button = PushButton(BACKWARD_BUTTON, ENABLE_INTERNAL_PULLUP);


#define MAX_PULSE_MS 3000
uint16_t pulse_width = 0;

void readEEPROM()
{
    byte buffer[2] = {0};

    buffer[0] = EEPROM.read(0);
    buffer[1] = EEPROM.read(1);
    pulse_width = 0;

    pulse_width = pulse_width | (buffer[0] << 8);
    pulse_width = pulse_width | buffer[1];

    DEBUG_PRINT("eeprom read result: ");
    DEBUG_PRINTLN(pulse_width);
}

void writeEEPROM()
{
    byte buffer[2] = {0};
    buffer[0] = (pulse_width >> 8) & 0xFF;
    buffer[1] = pulse_width & 0xFF;
    EEPROM.write(0, buffer[0]);
    EEPROM.write(1, buffer[1]);
}

void stopWelding()
{
    DEBUG_PRINTLN("turn OFF welding pin");
}

void startWelding()
{
    DEBUG_PRINTLN("turn ON welding pin");
    t.pulseImmediate(RELAY_PIN, pulse_width, HIGH); // 10 seconds
    t.after(pulse_width, stopWelding);
}

//buttons callback
void onButtonPressed(Button &btn)
{
    if (btn.is(ok_button))
    {
        DEBUG_PRINTLN("OK pressed");
        startWelding();
    }
    else if (btn.is(forward_button))
    {
        DEBUG_PRINTLN("FORWARD pressed");
        if (pulse_width < MAX_PULSE_MS)
        {
            //if it close to maximum make it maximum
            if (pulse_width >= MAX_PULSE_MS - 200)
            {
                pulse_width = MAX_PULSE_MS;
            }
            else
            {
                if (pulse_width < 50)
                {
                    pulse_width += 10;
                }
                else if (pulse_width < 1000)
                {
                    pulse_width += 50;
                }
                else
                {
                    pulse_width += 200;
                }
            }
        }
    }
    else if (btn.is(backward_button))
    {
        DEBUG_PRINTLN("BACKWARD pressed");
        if (pulse_width > 0)
        {
            //if it close to minimum make it minimum
            if (pulse_width < 10)
            {
                pulse_width = 0;
            }
            else
            {
                if (pulse_width < 50)
                {
                    pulse_width -= 10;
                }
                else if (pulse_width < 1000)
                {
                    pulse_width -= 50;
                }
                else
                {
                    pulse_width -= 200;
                }
            }
        }
    }
    else
    {
        DEBUG_PRINTLN("Hmmm, no button wasp ressed");
    }

    //save value to eeprom
    writeEEPROM();

    DEBUG_PRINT("pulse width: ");
    DEBUG_PRINTLN(pulse_width);
    lcd.clear();
    lcd.print(pulse_width);
    lcd.print(" ms");
}

void setup()
{

#ifdef SERIAL_LOG
    Serial.begin(9600);
#endif

    //set callback to buttons
    ok_button.onPress(onButtonPressed);
    forward_button.onPress(onButtonPressed);
    backward_button.onPress(onButtonPressed);

    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    //setup and set relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // start lcd
    lcd.begin(16, 4);
    // Print a message to the LCD.
    lcd.print("hello, world!");
    delay(200);
    lcd.clear();

    //read eeprom
    readEEPROM();
    lcd.clear();
    lcd.print(pulse_width);
    lcd.print(" ms");
}

void loop()
{
    //handle timer
    t.update();
    //handle buttons
    ok_button.update();
    forward_button.update();
    backward_button.update();
    // // turn the LED on (HIGH is the voltage level)
    // digitalWrite(LED_BUILTIN, HIGH);
    // lcd.noBlink();

    // // wait for a second
    // delay(1000);
    // // turn the LED off by making the voltage LOW
    // digitalWrite(LED_BUILTIN, LOW);
    // lcd.blink();

    // // wait for a second
    // delay(1000);
}
