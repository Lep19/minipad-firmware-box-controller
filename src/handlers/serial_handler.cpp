#include <Arduino.h>
#include "handlers/serial_handler.hpp"
#include "handlers/keypad_handler.hpp"
#include "helpers/string_helper.hpp"
#include "definitions.hpp"
extern "C"
{
#include "pico/bootrom.h"
}

// Define a handy macro for printing with a newline character at the end.
#define print(fmt, ...) Serial.printf(fmt "\n", __VA_ARGS__)

void SerialHandler::handleSerialInput(String *inputStr)
{
    // Convert the string into a character array for further parsing and make it lowercase.
    char input[(*inputStr).length() + 1];
    (*inputStr).toCharArray(input, (*inputStr).length() + 1);
    StringHelper::toLower(input);

    // Parse the command as the first argument, separated by whitespaces.
    char command[1024];
    StringHelper::getArgumentAt(input, ' ', 0, command);

    // Get a pointer pointing to the start of all parameters for the command and parse them.
    char *parameters = input + strlen(command) + 1;
    char arg0[1024];
    StringHelper::getArgumentAt(parameters, ' ', 0, arg0);

    // Handle the global commands and pass their expected required parameters.
    if (isEqual(command, "boot"))
        boot();
    else if (isEqual(command, "save"))
        save();
    else if (isEqual(command, "get"))
        get();
    else if (isEqual(command, "name"))
        name(parameters);
    else if (isEqual(command, "out"))
        out(isTrue(arg0));
#if DEBUG
    else if (isEqual(command, "echo"))
        echo(parameters);
#endif

    // Handle hall effect key specific commands by checking if the command starts with "key".
    if (strstr(command, "key") == command)
    {
        // Split the command into the key string and the setting name.
        char keyStr[1024];
        char setting[1024];
        StringHelper::getArgumentAt(command, '.', 0, keyStr);
        StringHelper::getArgumentAt(command, '.', 1, setting);

        // By default, apply this command to all hall effect keys.
        HEKey *keys = ConfigController.config.heKeys;

        // If an index is specified ("keyX"), replace that keys array with just that key.
        if (strlen(keyStr) > 3)
        {
            // Get the index and check if it's in the valid range.
            uint8_t keyIndex = atoi(keyStr + 3) - 1;
            if (keyIndex >= HE_KEYS)
                return;

            // Replace the array with that single key.
            keys = &ConfigController.config.heKeys[keyIndex];
        }

        // Apply the command to all targetted hall effect keys.
        for (uint8_t i = 0; i < (strlen(keyStr) > 3 ? 1 : HE_KEYS); i++)
        {
            // Get the key from the pointer array.
            HEKey &key = keys[i];

            // Handle the settings.
            if (isEqual(setting, "rt"))
                key_rt(key, isTrue(arg0));
            else if (isEqual(setting, "crt"))
                key_crt(key, isTrue(arg0));
            else if (isEqual(setting, "rtus"))
                key_rtus(key, atoi(arg0));
            else if (isEqual(setting, "rtds"))
                key_rtds(key, atoi(arg0));
            else if (isEqual(setting, "lh"))
                key_lh(key, atoi(arg0));
            else if (isEqual(setting, "uh"))
                key_uh(key, atoi(arg0));
            else if (isEqual(setting, "char"))
                key_char(key, atoi(arg0));
            else if (isEqual(setting, "rest"))
                key_rest(key, atoi(arg0));
            else if (isEqual(setting, "down"))
                key_down(key, atoi(arg0));
            else if (isEqual(setting, "hid"))
                key_hid(key, isTrue(arg0));
        }
    }

    // Handle digital key specific commands by checking if the command starts with "dkey".
    if (strstr(command, "dkey") == command)
    {
        // Split the command into the key string and the setting name.
        char keyStr[1024];
        char setting[1024];
        StringHelper::getArgumentAt(command, '.', 0, keyStr);
        StringHelper::getArgumentAt(command, '.', 1, setting);

        // By default, apply this command to all digital keys.
        DigitalKey *keys = ConfigController.config.digitalKeys;

        // If an index is specified ("dkeyX"), replace that keys array with just that key.
        if (strlen(keyStr) > 4)
        {
            // Get the index and check if it's in the valid range.
            uint8_t keyIndex = atoi(keyStr + 4) - 1;
            if (keyIndex >= DIGITAL_KEYS)
                return;

            // Replace the array with that single digital key.
            keys = &ConfigController.config.digitalKeys[keyIndex];
        }

        // Apply the command to all targetted digital keys.
        for (uint8_t i = 0; i < (strlen(keyStr) > 4 ? 1 : DIGITAL_KEYS); i++)
        {
            // Get the key from the pointer array.
            DigitalKey &key = keys[i];

            // Handle the settings.
            if (isEqual(setting, "char"))
                dkey_char(key, atoi(arg0));
            else if (isEqual(setting, "hid"))
                dkey_hid(key, isTrue(arg0));
        }
    }
}

void SerialHandler::boot()
{
    // Set the RP2040 into bootloader mode.
    reset_usb_boot(0, 0);
}

void SerialHandler::save()
{
    // Save the configuration managed by the config controller.
    ConfigController.saveConfig();
}

void SerialHandler::get()
{
    // Output all global settings.
    print("GET version=%s%s", FIRMWARE_VERSION, DEBUG ? "-dev" : "");
    print("GET hkeys=%s", HE_KEYS);
    print("GET dkeys=%s", DIGITAL_KEYS);
    print("GET name=%s", ConfigController.config.name);
    print("GET htol=%d", HYSTERESIS_TOLERANCE);
    print("GET rtol=%d", RAPID_TRIGGER_TOLERANCE);
    print("GET trdt=%d", TRAVEL_DISTANCE_IN_0_01MM);

    // Output all key-specific settings.
    for (const HEKey &key : ConfigController.config.heKeys)
    {
        // Format the base for all lines being written.
        print("GET key%d.rt=%d", key.index + 1, key.rapidTrigger);
        print("GET key%d.crt=%d", key.index + 1, key.continuousRapidTrigger);
        print("GET key%d.rtus=%d", key.index + 1, key.rapidTriggerUpSensitivity);
        print("GET key%d.rtds=%d", key.index + 1, key.rapidTriggerDownSensitivity);
        print("GET key%d.lh=%d", key.index + 1, key.lowerHysteresis);
        print("GET key%d.uh=%d", key.index + 1, key.upperHysteresis);
        print("GET key%d.key=%d", key.index + 1, key.keyChar);
        print("GET key%d.rest=%d", key.index + 1, key.restPosition);
        print("GET key%d.down=%d", key.index + 1, key.downPosition);
        print("GET key%d.hid=%d", key.index + 1, key.hidEnabled);
    }

    // Print this line to signalize the end of printing the settings to the listener.
    Serial.println("GET END");
}

void SerialHandler::name(char *name)
{
    // Get the length of the name and check if it's within the 1-128 characters boundary.
    size_t length = strlen(name);
    if (length >= 1 && length <= 128)
        strcpy(ConfigController.config.name, name);
}

void SerialHandler::out(bool state)
{
    // Set the calibration mode field of the keypad handler to the specified state.
    KeypadHandler.outputMode = state;
}

void SerialHandler::echo(char *input)
{
    // Output the same input. This command is used for debugging purposes and only available in said environemnts.
    Serial.println(input);
}

void SerialHandler::key_rt(HEKey &key, bool state)
{
    // Set the rapid trigger config value to the specified state.
    key.rapidTrigger = state;
}

void SerialHandler::key_crt(HEKey &key, bool state)
{
    // Set the continuous rapid trigger config value to the specified state.
    key.continuousRapidTrigger = state;
}

void SerialHandler::key_rtus(HEKey &key, uint16_t value)
{
    // Check if the specified value is within the tolerance-TRAVEL_DISTANCE_IN_0_01MM boundary.
    if (value >= RAPID_TRIGGER_TOLERANCE && value <= TRAVEL_DISTANCE_IN_0_01MM)
        // Set the rapid trigger up sensitivity config value to the specified state.
        key.rapidTriggerUpSensitivity = value;
}

void SerialHandler::key_rtds(HEKey &key, uint16_t value)
{
    // Check if the specified value is within the tolerance-TRAVEL_DISTANCE_IN_0_01MM boundary.
    if (value >= RAPID_TRIGGER_TOLERANCE && value <= TRAVEL_DISTANCE_IN_0_01MM)
        // Set the rapid trigger down sensitivity config value to the specified state.
        key.rapidTriggerDownSensitivity = value;
}

void SerialHandler::key_lh(HEKey &key, uint16_t value)
{
    // Check if the specified value is at least the hysteresis tolerance away from the upper hysteresis.
    if (key.upperHysteresis - value >= HYSTERESIS_TOLERANCE)
        // Set the lower hysteresis config value to the specified state.
        key.lowerHysteresis = value;
}

void SerialHandler::key_uh(HEKey &key, uint16_t value)
{
    // Check if the specified value is at least the hysteresis tolerance away from the lower hysteresis.
    // Also make sure the upper hysteresis is at least said tolerance away from TRAVEL_DISTANCE_IN_0_01MM
    // to make sure the value can be reached and the key does not get stuck in an eternal pressed state.
    if (value - key.lowerHysteresis >= HYSTERESIS_TOLERANCE && TRAVEL_DISTANCE_IN_0_01MM - value >= HYSTERESIS_TOLERANCE)
        // Set the upper hysteresis config value to the specified state.
        key.upperHysteresis = value;
}

void SerialHandler::key_char(HEKey &key, uint8_t keyChar)
{
    // Check if the specified key is a letter with a byte value between 97 and 122.
    if (keyChar >= 97 && keyChar <= 122)
        // Set the key config value of the specified key to the specified state.
        key.keyChar = keyChar;
}

void SerialHandler::key_rest(HEKey &key, uint16_t value)
{
    // Check whether the specified value is bigger than the down position and smaller or equal to the maximum analog value.
    if (value > key.downPosition && value <= pow(2, ANALOG_RESOLUTION) - 1)
        // Set the rest position config value of the specified key to the specified state.
        key.restPosition = value;
}

void SerialHandler::key_down(HEKey &key, uint16_t value)
{
    // Check whether the specified value is smaller than the rest position.
    if (value < key.restPosition)
        // Set the down position config value of the specified key to the specified state.
        key.downPosition = value;
}

void SerialHandler::key_hid(HEKey &key, bool state)
{
    // Set the hid config value of the specified key to the specified state.
    key.hidEnabled = state;
}

bool SerialHandler::isTrue(char *str)
{
    return isEqual(str, "1") || isEqual(str, "true");
}

bool SerialHandler::isEqual(char *str1, const char *str2)
{
    // Compare the specified strings and return whether they are equal or not.
    return strcmp(str1, str2) == 0;
}
