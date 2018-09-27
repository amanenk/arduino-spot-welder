#include <Arduino.h>

//for 4 digit indicator
#include <TM1637Display.h>

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
#define CLK 5
#define DIO 6

//buttons settings
#define OK_BUTTON 2
#define FORWARD_BUTTON 4
#define BACKWARD_BUTTON 3

//pin to control relay
#define RELAY_PIN A0

//welding timer
Timer t;

// Create buttons
PushButton ok_button = PushButton(OK_BUTTON, ENABLE_INTERNAL_PULLUP);
PushButton forward_button = PushButton(FORWARD_BUTTON, ENABLE_INTERNAL_PULLUP);
PushButton backward_button = PushButton(BACKWARD_BUTTON, ENABLE_INTERNAL_PULLUP);

TM1637Display display(CLK, DIO);

#define MAX_PULSE_MS 1000
uint16_t pulse_width = 0;

bool welding_active = false;
bool after_welding_idle = false;

#define IDLE_LENGTH 200

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
    welding_active = false;
    display.showNumberDec(pulse_width, true, 4, 0);
}

void startIdle()
{
    after_welding_idle = true;
    if (pulse_width <= IDLE_LENGTH * 2)
    {
        t.after(IDLE_LENGTH, stopIdle);
    }
    else
    {
        t.after(pulse_width / 2, stopIdle);
    }
}

void stopIdle()
{
    DEBUG_PRINTLN("you can weld now");
    after_welding_idle = false;
}

void startWelding()
{
    if (!welding_active)
    {
        welding_active = true;
        DEBUG_PRINTLN("turn ON welding pin");
        t.pulseImmediate(RELAY_PIN, pulse_width, LOW); // 10 seconds
        t.after(pulse_width, stopWelding);
        uint8_t data[] = {SEG_G, SEG_G, SEG_G, SEG_G};
        display.setSegments(data);
    }
    else
    {
        DEBUG_PRINTLN("weleding is currently active");
    }
}

void onFirePressed(Button &btn)
{
    DEBUG_PRINTLN("OK pressed");
    if (!after_welding_idle)
    {
        startWelding();
    }
    else
    {
        DEBUG_PRINTLN("Welding not allowed now the device is in dile");
    }
}

//buttons callback
void onButtonPressed(Button &btn)
{
    if (btn.is(forward_button))
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
                if (pulse_width <= 50)
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
    //show symbols on disllay
    display.showNumberDec(pulse_width, true, 4, 0);
}

void setup()
{
#ifdef SERIAL_LOG
    Serial.begin(9600);
#endif
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    //setup and set relay pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);

    //set callback to buttons
    ok_button.onPress(onFirePressed);
    forward_button.onPress(onButtonPressed);
    backward_button.onPress(onButtonPressed);

    uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
    display.setBrightness(0x0f);
    // All segments on
    display.setSegments(data);
    delay(100);

    //read eeprom
    readEEPROM();

    //show symbols on disllay
    display.showNumberDec(pulse_width, true, 4, 0);
}

void loop()
{
    t.update();
    ok_button.update();
    forward_button.update();
    backward_button.update();

    if (pulse_width > MAX_PULSE_MS)
    {
        pulse_width = 0;
    }
}
