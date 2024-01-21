/// Main file (source)
/// (c) 2018 Jani Nykänen

#include "app.h"

#include <stdio.h>
#include <stdbool.h>

#include "controls.h"
#include "graphics.h"
#include "assets.h"
#include "music.h"
#include "sample.h"

#include "stdlib.h"
#include "math.h"
#include "stdio.h"

// Is application app_running
static bool isRunning;
// Is full screen
static bool isFullscreen;

// Double buffer variables
static DB	db[2];
static int	db_active = 0;
static char* db_nextpri;

// (Timer) old ticks
static int oldTicks;
// New ticks
static int newTicks;
// (Timer) delta time
static int deltaTime;

// Canvas pos
static SDL_Point canvasPos;
// Canvas size
static SDL_Point canvasSize;

// Current scene
static SCENE currentScene;
// Previous scene
static SCENE prevScene;
// Global scene
static SCENE globalScene;
// Scenes
static SCENE scenes[16];
// Scene count
static Uint8 sceneCount;


// Configuration
static CONFIG config;

// Joystick
static uint8_t joy[2][34];
static uint8_t prevJoy[2][34];

// Heap
static unsigned char* ramAddr;

// Calculate canvas size and position on screen
static void app_calc_canvas_prop(int winWidth, int winHeight)
{
    // If aspect ratio is bigger or equal to the ratio of the canvas
    if((float)winWidth/(float)winHeight >= (float)config.canvasWidth/ (float)config.canvasHeight )
    {
        canvasSize.y = winHeight;
        canvasSize.x = (int) ( (float)winHeight / (float) config.canvasHeight  * config.canvasWidth);

        canvasPos.x = winWidth/2 - canvasSize.x/2;
        canvasPos.y = 0;
    }
    else
    {
        canvasSize.x = winWidth;
        canvasSize.y =  (int) ( (float)canvasSize.x / (float) config.canvasWidth  * config.canvasHeight );

        canvasPos.y = winHeight/2 - canvasSize.y/2;
        canvasPos.x = 0;
    }
}


// Initialize SDL
static int app_init_SDL()
{   
    // Init

    ResetGraph(0);

    int windowWidth = config.winWidth;
    int windowHeight = config.winHeight;

    // Set display and draw environment areas
    // (display and draw areas must be separate, otherwise hello flicker)
    SetDefDispEnv(&db[0].disp, 0, 0, windowWidth, windowHeight);
    SetDefDrawEnv(&db[0].draw, 0, 256, windowWidth, windowHeight);

    // Enable draw area clear and dither processing
    setRGB0(&db[0].draw, 0, 0, 0);
    db[0].draw.isbg = 1;
    db[0].draw.dtd = 1;


    // Define the second set of display/draw environments
    SetDefDispEnv(&db[1].disp, 0, 256, windowWidth, windowHeight);
    SetDefDrawEnv(&db[1].draw, 0, 0, windowWidth, windowHeight);

    setRGB0(&db[1].draw, 0, 0, 0);
    db[1].draw.isbg = 1;
    db[1].draw.dtd = 1;

    SetDispMask(1);

    // Create window

    isFullscreen = false;
    if(config.fullscreen)
        app_toggle_fullscreen();

    // Apply the drawing environment of the first double buffer
    PutDispEnv(&db[0].disp);
    PutDrawEnv(&db[0].draw);

    // Clear both ordering tables to make sure they are clean at the start
    ClearOTagR((uint32_t*)db[0].ot, OT_LEN);
    ClearOTagR((uint32_t*)db[1].ot, OT_LEN);

    // Set primitive pointer address
    db_nextpri = db[0].p;

    CdInit();

    EnterCriticalSection();
    InitHeap((u_long*)ramAddr, 1200000);
    ExitCriticalSection();

    SpuInit();

    // Master volume should be in range 0x0000 - 0x3fff
    SpuSetCommonMasterVolume(0x3fff, 0x3fff);
    // Cd volume should be in range 0x0000 - 0x7fff
    SpuSetCommonCDVolume(0x2fff, 0x2fff);
    // Set transfer mode 
    SpuSetTransferMode(SPU_TRANSFER_BY_DMA);

    // Open joystick
    // Init BIOS pad driver and set pad buffers (buffers are updated
    // automatically on every V-Blank)
    InitPAD(&joy[0][0], 34, &joy[1][0], 34);

    // Start pad
    StartPAD();

    // Don't make pad driver acknowledge V-Blank IRQ (recommended)
    ChangeClearPAD(0);

    return 0;
}


// Toggle fullscreen mode
void app_toggle_fullscreen()
{
	isFullscreen = !isFullscreen;
}


// Is full screen
bool app_is_full_screen()
{
    return isFullscreen;
}


// Initialize application
static int app_init(SCENE* arrScenes, int count, const char* assPath)
{
    // Init SDL
    if(app_init_SDL() != 0)
    {
        return 1;
    }

    // Set global renderer & init graphics
    init_graphics();
    set_global_renderer(&db);

    // Initialize audio
    init_samples();
    if(init_music() == 1)
    {
        return 1;
    }

    // Copy scenes to a scene array
    // and initialize them
    sceneCount = count;
    int i =0;
    for(; i < count; i++)
    {
        scenes[i] = arrScenes[i];
        if(scenes[i].on_init != NULL)
        {
            if(scenes[i].on_init() == 1)
            {
                return 1;
            }
        }

        if(strcmp(scenes[i].name,"global") == 0)
        {
            globalScene = scenes[i];
        }
    }

    // Make the last scene the current scene
    if(sceneCount > 0)
        currentScene = scenes[count -1];

    isRunning = true;

    return 0;
}


// Go through events
static void app_events()
{    
    PADTYPE* pad = (PADTYPE*)&joy[0][0];

    // Joy button down
    case SDL_JOYBUTTONDOWN:
        ctr_on_joy_down(event.jbutton.button);
        break;

    // Joy button up
    case SDL_JOYBUTTONUP:
        ctr_on_joy_up(event.jbutton.button);
        break;

    // Joy axis
    case SDL_JOYAXISMOTION:
    {
        int axis = 0;
        if(event.jaxis.axis == 0)
            axis = 0;
        else if(event.jaxis.axis == 1)
            axis = 1;
        else 
            break;

        float value = (float)event.jaxis.value / 32767.0f;

        ctr_on_joy_axis(axis,value);
            
        break;
    }

    // Joy hat
    case SDL_JOYHATMOTION:
    {
        int v = event.jhat.value;
        VEC2 stick = vec2(0.0f,0.0f);
        if(v == SDL_HAT_LEFTUP || v == SDL_HAT_LEFT || v == SDL_HAT_LEFTDOWN)
        {
            stick.x = -1.0f;
        }

        if(v == SDL_HAT_RIGHTUP || v == SDL_HAT_RIGHT || v == SDL_HAT_RIGHTDOWN)
        {
            stick.x = 1.0f;
        }

        if(v == SDL_HAT_LEFTUP || v == SDL_HAT_UP || v == SDL_HAT_RIGHTUP)
        {
            stick.y = -1.0f;
        }

        if(v == SDL_HAT_LEFTDOWN || v == SDL_HAT_DOWN || v == SDL_HAT_RIGHTDOWN)
        {
            stick.y = 1.0f;
        }

        ctr_on_joy_axis(0,stick.x);
        ctr_on_joy_axis(1,stick.y);

        break;
    }

    default:
        break;
    }

}   


// Update application
static void app_update(uint32_t delta)
{
    // Calculate timer multiplier
    float tm = (float)((float)delta/1000.0f) / (1.0f/60.0f);
    // Limit tm (in other words, limit minimum fps)
    if(tm > 5.0) tm = 5.0;

    // Quit
    if(get_key_state(SDL_SCANCODE_LCTRL) == DOWN &&
       get_key_state(SDL_SCANCODE_Q) == PRESSED)
    {
        isRunning = false;
        return;
    }

    // Full screen
    if( (get_key_state(SDL_SCANCODE_LALT) == DOWN &&
       get_key_state(SDL_SCANCODE_RETURN) == PRESSED) ||
       get_key_state(SDL_SCANCODE_F4) == PRESSED )
    {
        app_toggle_fullscreen();
    }

    // Update current & global scenes
    if(currentScene.on_update != NULL)
    {
        currentScene.on_update(tm);
    }
    if(globalScene.on_update != NULL)
    {
        globalScene.on_update(tm);
    }

    // Update controls
    ctr_update();
}


// Draw application
static void app_draw()
{
    // Clear to black
    clear(0,0,0);

    // Set target to the canvas texture
    SDL_SetRenderTarget(rend,canvas);

    // Draw global & current scenes
    if(currentScene.on_draw != NULL)
    {
        currentScene.on_draw();
    }
    if(globalScene.on_draw != NULL)
    {
        globalScene.on_draw();
    }

    // Set target back to the main window
    SDL_SetRenderTarget(rend,NULL);

    // Draw frame
    SDL_Rect dest = (SDL_Rect){canvasPos.x,canvasPos.y,canvasSize.x,canvasSize.y};
    SDL_RenderCopy(rend,canvas,NULL,&dest);

    // Render frame
    SDL_RenderPresent(rend);
}


// Destroy application
static void app_destroy()
{
    // Destroy scenes
    int i = 0;
    if(scenes[i].on_destroy != NULL)
    {
        scenes[i].on_destroy();
    }

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);

    SDL_JoystickClose(joy);

    
}


// Swap scene
void app_swap_scene(const char* name)
{
    int i = 0;
    for(; i < sceneCount; i++)
    {
        if(strcmp(scenes[i].name,name) == 0)
        {
            prevScene = currentScene;
            currentScene = scenes[i];
            if(currentScene.on_swap != NULL)
                currentScene.on_swap();
            return;
        }
    }

}


// Swap to previous
void app_swap_to_previous_scene()
{
    currentScene = prevScene;
}


// Terminate application
void app_terminate()
{
    isRunning = false;
}


// Run application
int app_run(SCENE* arrScenes, int count, CONFIG c)
{
    config = c;

    // Calculate frame wait value  -  Don't need this, no timing needed on PSX
    //int frame_wait = (int)round(1000.0f / config.fps);

    if(app_init(arrScenes,count,NULL) != 0) return 1;

    while(isRunning)
    {
        // Update frame
        app_events();
        app_update(16);
        app_draw();

        // Set new time
        newTicks = SDL_GetTicks();

        // Wait
        int deltaMilliseconds = (newTicks - oldTicks);
        int restTime = (int) (frame_wait-1) - (int)deltaMilliseconds;
        if (restTime > 0) 
            SDL_Delay((unsigned int) restTime);

        // Set delta time
        deltaTime = SDL_GetTicks() - oldTicks;;

    }
    app_destroy();

    return 0;
}


/// Ask if the user wants to quit
int ask_to_quit()
{
    const SDL_MessageBoxButtonData buttons[] = {
        { 0, 0, "Yes" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "No" },
        
    };
    const SDL_MessageBoxColorScheme colorScheme = {
        { 
            { 85,   85,   85 },
            {   255, 255,   255 },
            { 255, 255,   255 },
            {   170,   170, 170 },
            { 255,   255, 0 }
        }
    };

    const SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_INFORMATION,
        NULL, 
        "Quit application?", 
        "Are you sure you want to\nterminate the application?", 
        SDL_arraysize(buttons),
        buttons,
        &colorScheme
    };

    int buttonid;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) 
    {
        printf("Error displaying a message box!\n");
        return 1;
    }

    if (buttonid == 0)
    {
        isRunning = false;
        return 1;
    } 

    return 0;
}