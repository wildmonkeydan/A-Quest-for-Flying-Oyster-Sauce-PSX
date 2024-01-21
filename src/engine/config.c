/**
 * Configuration
 * (source file)
 *
 * @author Jani Nykänen
 * @version 1.0.0
 */

#include "config.h"

#include "SDL2/SDL.h"

#include "../lib/parseword.h"
#include "error.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

// Key
static char* key;
// Value
static char* value;


// Read config
int read_config(CONFIG* c, const char* path)
{
    //Overriding this as it's a fixed platform, we don't need configs

    strcpy(c->title, "A Quest for Flying Oyster Sauce");
    
    c->winWidth = 320; // Standard PS1 Resolution
    c->winHeight = 240;

    c->canvasWidth = 256;
    c->canvasHeight = 192;

    c->fps = 60; // Setting to NTSC for now, will add a region check

    c->fullscreen = true;

    return 0;
}