#ifndef __MAIN_H
#define __MAIN_H

#include "config.h"
#include "EmbUI.h"
#include "camera.h"
#include <arduino.h>
#include <ArduinoJson.h>
#include <string.h>

void create_parameters();
void sync_parameters();

extern EMBUICAMERA *camera;
extern EmbUI *embui;

#endif