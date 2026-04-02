/*
 * CTI.c
 *
 *  Created on: Mar 25, 2025
 *      Author: jrsie
 */

#include "CTI.h"


void getsUart0(USER_DATA *data)
{
    uint8_t index = 0;
    char symbol;
    uint8_t flag = 0;

    //Run until flag is true
    while (flag != 1)
    {
        symbol = getcUart0();
        //Check to see if the symbol is a backspace
        if (symbol == 8 || symbol == 127)
        {
            if (index > 0)
            {
                index--;
            }
        }
        //Check to see if the symbol is a return
        else if (symbol == 10 || symbol == 13)
        {
            data->buffer[index] = '\0';
            flag = 1;
        }
        //Ignore unprintable symbols < 32
        //Save all other symbols
        else if (symbol >= 32)
        {
            data->buffer[index] = symbol;
            index++;
            //Check to see if we filled the buffer
            if (index == MAX_CHARS)
            {
                flag = 1;
            }
        }

        data->buffer[index] = '\0';
    }
}

void parseFields(USER_DATA *data)
{

    uint8_t i, fieldNum = 0;
    bool newField = true;
    char symbol;
    for (i = 0; data->buffer[i] != 0; i++)
    {
        symbol = data->buffer[i];
        // Check for alpha characters a-z
        if(symbol >= 'a' && symbol <= 'z')
        {
            // Check to see if a trasition has passed and that the field number
            // has not passed the maximum fields allowed
            if (newField && fieldNum < MAX_FIELDS)
            {
                // There was a new field, record type and position and increment counter
                data->fieldType[fieldNum] = 'a';
                data->fieldPosition[fieldNum] = i;
                fieldNum++;
                newField = false;
            }
        }
        // Check for alpha characters A-Z
        else if ((symbol >= 'A') && (symbol <= 'Z'))
        {
            if (newField && fieldNum < MAX_FIELDS)
            {
                data->fieldType[fieldNum] = 'a';
                data->fieldPosition[fieldNum] = i;
                fieldNum++;
                newField = false;
            }
        }
        // Check for numeric characters 0-9
        // Check for numeric characters 0-9, decimal point, or minus sign
        else if ((symbol >= '0' && symbol <= '9') || symbol == '.' || symbol == '-')
        {
            if (newField && fieldNum < MAX_FIELDS)
            {
                data->fieldType[fieldNum] = 'n';
                data->fieldPosition[fieldNum] = i;
                fieldNum++;
                newField = false;
            }
        }

        // It is the delimiter, set newField to true and set to NULL
        else
        {
            newField = true;
            data->buffer[i] = '\0';
        }
    }
    //Set the number of fields to count
    data->fieldCount =  fieldNum;
}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    // Past the field of MAX_FIELDS
    if(fieldNumber >= MAX_FIELDS || fieldNumber > data->fieldCount)
    {
        return "\0";
    }
    else
    {
        return &data->buffer[data->fieldPosition[fieldNumber]];
    }
}

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
        // Past the field of MAX_FIELDS
       if(fieldNumber >= MAX_FIELDS || fieldNumber > data->fieldCount
          || data->fieldType[fieldNumber] != 'n')
       {
           return 0;
       }
       else
       {
           char string[MAX_CHARS + 1];
           uint8_t index = data->fieldPosition[fieldNumber];
           uint8_t i = 0;
           // Copy characters from the buffer to the local string
           while ((data->buffer[index] != '\0') && (i < MAX_CHARS))
           {
               string[i] = data->buffer[index];
               i++;
               index++;
           }
           string[i] = '\0';  // null-terminate

           int value = 0;
           i = 0;
           // Build the integer value from the copied string
           while (string[i] != '\0')
           {
               // Convert ASCII digit to integer and build the value:
                value = value * 10 + (string[i] - '0');
                i++;
           }
           return value;
       }
}

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    // Check argument count (excluding command itself)
    if ((data->fieldCount - 1) < minArguments)
        return false;

    // Compare command string exactly
    char *cmd = &data->buffer[data->fieldPosition[0]];

    if (strcmp(cmd, strCommand) == 0)
        return true;

    return false;
}


float getFieldFloat(USER_DATA *data, uint8_t fieldNumber)
{
    // Bounds and type checking
    if (fieldNumber >= MAX_FIELDS || fieldNumber > data->fieldCount)
        return 0.0f;

    // Allow numeric fields that may include a decimal point
    // (parseFields marks numeric fields as 'n')
    if (data->fieldType[fieldNumber] != 'n')
        return 0.0f;

    char *str = &data->buffer[data->fieldPosition[fieldNumber]];

    float value = 0.0f;
    float fraction = 0.0f;
    float divisor = 1.0f;
    bool negative = false;
    bool decimalSeen = false;

    // Handle optional sign
    if (*str == '-')
    {
        negative = true;
        str++;
    }

    // Parse characters
    while (*str != '\0')
    {
        if (*str == '.')
        {
            decimalSeen = true;
        }
        else if (*str >= '0' && *str <= '9')
        {
            if (!decimalSeen)
            {
                value = value * 10.0f + (*str - '0');
            }
            else
            {
                divisor *= 10.0f;
                fraction += (*str - '0') / divisor;
            }
        }
        else
        {
            // Invalid character
            return 0.0f;
        }
        str++;
    }

    value += fraction;

    if (negative)
        value = -value;

    return value;
}


