#include <Arduino.h>

#include "HomeService.h"

HomeService homeService;

void setup() { homeService.begin(); }

void loop() { homeService.loop(); }
