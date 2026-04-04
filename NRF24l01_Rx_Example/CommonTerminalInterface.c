#include <stdint.h>
#include <stdbool.h>
#include "CommonTerminalInterface.h"
#include "uart0.h"
#include "strings.h"

void getsUart0(USER_DATA *data)
{
    uint32_t count = 0;
    while (1)
    {

        volatile char ch = getcUart0();
        if ((ch == 8 || ch == 127) & count > 0)
        {
            count--;
        }

        if (ch == 13)
        {
            data->buffer[count] = '\0';
            return;
        }

        if (ch >= 32)
        {
            data->buffer[count] = ch;
            count++;
            if (count == MAX_CHARS)
            {
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

void parseFields(USER_DATA *data)
{
    data->fieldCount = 0;
    uint8_t i = 0;
    uint8_t j = 0;

    if (data->buffer[i] >= 48 && data->buffer[i] <= 57)
    {
        data->fieldType[j] = 'n';
        data->fieldCount++;
        data->fieldPosition[j] = i;
        j++;
    }
    else if (data->buffer[i] >= 65 && data->buffer[i] <= 90
            || data->buffer[i] >= 97 && data->buffer[i] <= 122)
    {
        data->fieldType[j] = 'a';
        data->fieldCount++;
        data->fieldPosition[j] = i;
        j++;
    }

    i = 1;
    while (data->buffer[i] != '\0')
    {

        if ((data->buffer[i - 1] >= 32 && data->buffer[i - 1] < 48)
                || (data->buffer[i - 1] > 57 && data->buffer[i - 1] < 65)
                || (data->buffer[i - 1] > 90 && data->buffer[i - 1] < 97)
                || (data->buffer[i - 1] > 122))
        {
            if (data->buffer[i] >= 48 && data->buffer[i] <= 57)
            {
                data->buffer[i - 1] = '\0';
                data->fieldType[j] = 'n';
                data->fieldCount++;
                data->fieldPosition[j] = i;
                j++;
            }
            else if (data->buffer[i] >= 65 && data->buffer[i] <= 90
                    || data->buffer[i] >= 97 && data->buffer[i] <= 122)
            {
                data->buffer[i - 1] = '\0';
                data->fieldType[j] = 'a';
                data->fieldCount++;
                data->fieldPosition[j] = i;
                j++;
            }

        }
        i++;
    }
}

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber <= data->fieldCount)
    {
        return &data->buffer[data->fieldPosition[fieldNumber]];
    }
    else
    {
        return '\0';
    }
}

uint32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if ((fieldNumber <= data->fieldCount)
            && (data->buffer[data->fieldPosition[fieldNumber]] >= 48
                    && data->buffer[data->fieldPosition[fieldNumber]] <= 57))
    {
        return data->buffer[data->fieldPosition[fieldNumber]];
    }
    else
    {
        return 0;
    }
}

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    uint8_t i;
    for (i = 0; i < findLen(strCommand); i++)
    {
        if (data->buffer[i] != strCommand[i])
        {
            return 0;
        }
    }
    if (data->fieldCount - 1 < minArguments)
    {
        return 0;
    }
    return 1;
}
