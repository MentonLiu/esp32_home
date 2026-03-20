#include <Arduino.h>

#include "CentralProcessor.h"

CentralProcessor processor;

void setup()
{
    processor.begin();
}

void loop()
{
    processor.loop();
}
