/// Stage (source)
/// (c) 2018 Jani Nykänen

#include "stage.h"

#include "../engine/graphics.h"
#include "../lib/tmxc.h"

#include "objects.h"

#include "../global.h"

#include "math.h"
#include "stdlib.h"

// Default map size in tiles
#define DEFAULT_MAP_SIZE 16*12

// Bitmaps
static BITMAP* bmpSky;
static BITMAP* bmpClouds;
static BITMAP* bmpTiles;

// Map
static TILEMAP* mapMain;
// Collision map
static int colMap[DEFAULT_MAP_SIZE];
// Layer data
static int layerData[DEFAULT_MAP_SIZE];

// Cloud position
static float cloudPos;
// Lava position
static float lavaPos;
// Shake timer
static float shakeTimer;


// Is the tile in (x+dx,y+dy) same as in (x,y)
static bool is_same_tile(TILEMAP* t, int id, int x, int y, int dx, int dy)
{
    if(x+dx < 0 || y +dy < 0 || x+dx >= t->width || y+dy >= t->height)
        return true;

    return id == layerData[(y+dy)*t->width + x + dx];
}


// Draw a piece of tile
static void draw_tile_piece(int tx, int ty, int x, int y)
{
    draw_bitmap_region(bmpTiles,tx*8,ty*8,8,8,x,y,0);
}


// Draw soil tile
static void draw_tile_soil(TILEMAP* t, int x, int y)
{
    POINT t11, t12, t21, t22;

    bool leftGreen = false;
    bool rightGreen = false;

    t11 = point(2,0);
    t12 = point(0,1);
    t21 = point(3,0);
    t22 = point(1,1);

    // Bottom tile is different
    if(!is_same_tile(t,1,x,y,0,1))
    {
        t12 = point(10,0);
        t22 = point(11,0);
    }

    // Right tile is different
    if(!is_same_tile(t,1,x,y,1,0))
    {
        t21 = point(5,0);
        t22 = point(5,1);

        // Bottom
        if(!is_same_tile(t,1,x,y,0,1))
        {
            t22 = point(3,1);
        }
    }

    // Left tile is different
    if(!is_same_tile(t,1,x,y,-1,0))
    {
        t11 = point(4,0);
        t12 = point(4,1);

        // Bottom
        if(!is_same_tile(t,1,x,y,0,1))
        {
            t12 = point(2,1);
        }
    }

    // Upper tile is different
    if(!is_same_tile(t,1,x,y,0,-1))
    {
        t11 = point(0,0);
        t21 = point(1,0);

        // Right
        if(!is_same_tile(t,1,x,y,1,0))
        {
            t21 = point(9,1);
            rightGreen = true;
        }

        // Left
        if(!is_same_tile(t,1,x,y,-1,0))
        {
            t11 = point(8,1);
            leftGreen = true;
        }
    }

    // Bottom-right corner
    if(!is_same_tile(t,1,x,y,1,1) && is_same_tile(t,1,x,y,1,0) 
        && is_same_tile(t,1,x,y,0,1))
    {
        t22 = point(8,0);
    }

    // Bottom-left corner
    if(!is_same_tile(t,1,x,y,-1,1) && is_same_tile(t,1,x,y,-1,0) 
        && is_same_tile(t,1,x,y,0,1))
    {
        t12 = point(9,0);
    }

    // Top-right corner
    if(!is_same_tile(t,1,x,y,1,-1) && is_same_tile(t,1,x,y,1,0) 
        && is_same_tile(t,1,x,y,0,-1))
    {
        t21 = point(6,1);
    }

    // Top-left corner
    if(!is_same_tile(t,1,x,y,-1,-1) && is_same_tile(t,1,x,y,-1,0) 
        && is_same_tile(t,1,x,y,0,-1))
    {
        t11 = point(7,1);
    }

    // Draw tile pieces
    draw_tile_piece(t11.x,t11.y,x*16,y*16);
    draw_tile_piece(t21.x,t21.y,x*16 + 8,y*16);
    draw_tile_piece(t12.x,t12.y,x*16,y*16 + 8);
    draw_tile_piece(t22.x,t22.y,x*16 + 8,y*16 + 8);

    if(leftGreen)
        draw_tile_piece(6,0,x*16 - 8,y*16);
    if(rightGreen)
        draw_tile_piece(7,0,x*16 +16,y*16);
}


// Draw vine
static void draw_vine(TILEMAP* t, int x, int y)
{
    int sx1 = 96;
    int sx2 = 96;
    int sy1 = 0;
    int sy2 = 8;

    // Above
    if(!is_same_tile(t,2,x,y,0,-1) && !is_same_tile(t,1,x,y,0,-1))
    {
        sx1 += 16;
    }

    // Below
    if(!is_same_tile(t,2,x,y,0,1) && !is_same_tile(t,1,x,y,0,1))
    {
        sx2 += 16;
    }

    // Upper part
    draw_bitmap_region(bmpTiles,sx1,sy1,16,8,x*16,y*16,0);
    // Bottom part
    draw_bitmap_region(bmpTiles,sx2,sy2,16,8,x*16,y*16 + 8,0);
}


// Draw spikes
static void draw_spikes(TILEMAP* t, int x, int y)
{
    draw_bitmap_region(bmpTiles,144,0,16,8,x*16,y*16,0);

    // Left
    if(!is_same_tile(t,4,x,y,-1,0) && !is_same_tile(t,1,x,y,-1,0))
    {
        draw_bitmap_region(bmpTiles,144,8,8,8,x*16,y*16 + 8,0);
    }
    else
    {
        if(is_same_tile(t,4,x,y,-1,0))
            draw_bitmap_region(bmpTiles,144,8,8,8,x*16,y*16 + 8,0);
        else
            draw_bitmap_region(bmpTiles,160,0,8,8,x*16,y*16 + 8,0);
    }

    // Right
    if(!is_same_tile(t,4,x,y,1,0) && !is_same_tile(t,1,x,y,1,0))
    {
        draw_bitmap_region(bmpTiles,152,8,8,8,x*16 + 8,y*16 + 8,0);
    }
    else
    {
        draw_bitmap_region(bmpTiles,168,8,8,8,x*16+8,y*16 + 8,0);
    }
}


// Draw lava
static void draw_lava(TILEMAP* t, int x, int y)
{
    int i = 0;

    int lpos = (int)round(lavaPos) % 16;
    int lposy = (int)round(sin(lavaPos / 2.0f) * 1.0f) +1;

    draw_bitmap_region(bmpTiles,128,8,16,8,x*16, y*16+8, 0);
    if(!is_same_tile(t,3,x,y,0,-1))
    {
        for(; i < 2; ++ i)
        {
            draw_bitmap_region(bmpTiles,128,0,16,8,x*16 + lpos + i*16, y*16 + lposy, 0);
        }
    }
    else
    {
        draw_bitmap_region(bmpTiles,128,8,16,8,x*16, y*16, 0);
    }
}


// Draw an "other kind of" solid object, like lock
static void draw_other_solid(TILEMAP* t,int id, int x, int y, int dx, int dy)
{
    POINT t11, t12, t21, t22;

    t11 = point(6+dx,dy);
    t12 = point(6+dx,dy+1);
    t21 = point(6+dx+1,dy);
    t22 = point(6+dx+1,dy+1);

    // Free directions
    bool bottom = (!is_same_tile(t,1,x,y,0,1) && !is_same_tile(t,id,x,y,0,1));
    bool top = (!is_same_tile(t,1,x,y,0,-1) && !is_same_tile(t,id,x,y,0,-1));
    bool left = (!is_same_tile(t,1,x,y,-1,0) && !is_same_tile(t,id,x,y,-1,0));
    bool right = (!is_same_tile(t,1,x,y,1,0) && !is_same_tile(t,id,x,y,1,0));

    // Bottom
    if(bottom)
    {
        if(left)
            t12 = point(dx,dy+1);
        else
            t12 = point(dx+4,dy+1);

        if(right)
            t22 = point(dx+1,dy+1);
        else
            t22 = point(dx+5,dy+1);
    }
    else
    {
        if(left)
            t12 = point(dx+2,dy+1);

        if(right)
            t22 = point(dx+3,dy+1);
    }

    // Top
    if(top)
    {
        if(left)
            t11 = point(dx,dy);
        else
            t11 = point(dx+4,dy);

        if(right)
            t21 = point(dx+1,dy);
        else
            t21 = point(dx+5,dy);
    }
    else
    {
        if(left)
            t11 = point(dx+2,dy);

        if(right)
            t21 = point(dx+3,dy);
    }

    // Draw tile pieces
    draw_tile_piece(t11.x,t11.y,x*16,y*16);
    draw_tile_piece(t21.x,t21.y,x*16 + 8,y*16);
    draw_tile_piece(t12.x,t12.y,x*16,y*16 + 8);
    draw_tile_piece(t22.x,t22.y,x*16 + 8,y*16 + 8);
}


// Draw map
static void draw_map(TILEMAP* t)
{
    int x = 0;
    int y = 0;
    int id = 0;

    // Draw only lava
    for(y=0; y < t->height; ++ y)
    {
        for(x=0; x < t->width; ++ x)
        {
            id = layerData[y*t->width + x];
            if(id != 3) continue;
            draw_lava(t, x,y);
        }
    }

    // Draw the rest of tiles
    for(y=0; y < t->height; ++ y)
    {
        for(x=0; x < t->width; ++ x)
        {
            id = layerData[y*t->width + x];
            if(id == 0) continue;

            if(id == 1)
            {
                draw_tile_soil(t, x,y);
            }
            else if(id == 2)
            {
                draw_vine(t, x,y);
            }
            else if(id == 4)
            {
                draw_spikes(t,x,y);
            }
            else if(id == 5)
            {
                draw_other_solid(t,5,x,y,0,2);
            }
            else if(id == 6)
            {
                draw_other_solid(t,6,x,y,22,0);
            }
            else if(id == 17)
            {
                draw_other_solid(t,17,x,y,8,2);
            }
            else if(id == 18)
            {
                draw_bitmap_region(bmpTiles,128,16,16,16,x*16,y*16,0);
            }
        }
    }
}


// Draw stage background
static void draw_background()
{
    int i = 0;

    draw_bitmap(bmpSky,0,0,0);
    for(; i < 2; ++ i)
    {
        draw_bitmap(bmpClouds,(int)round(cloudPos + i * 256),192 - bmpClouds->h,0);
    }
}


// Parse map and create objects and define collision map
static void parse_map(TILEMAP* t, bool colOnly)
{
    int x = 0;
    int y = 0;
    int id = 0;

    // Draw only lava
    for(y=0; y < t->height; ++ y)
    {
        for(x=0; x < t->width; ++ x)
        {
            id = layerData[y*t->width + x];
            if( ( (id >= 6  && id < 16) || id == 19) && !colOnly)
            {
                obj_add(id,x,y);
            }
            else if(id > 0)
            {
                colMap[y*t->width + x] = id;
            }
        }
    }
}


// Reset stage
void stage_reset(bool soft)
{
    // Set variables to their default values
    cloudPos = 0.0f;
    lavaPos = 0.0f;
    shakeTimer = 0.0f;

    // Clear collision map & copy layer data
    int i = 0;
    for(; i < mapMain->width*mapMain->height; ++ i)
    {
        layerData[i] = mapMain->layers[0] [i];
        colMap[i] = 0;
    }

    // Create objects
    parse_map(mapMain,soft);
}


// Initialize stage
void stage_init(ASSET_PACK* ass)
{
    // Get assets
    bmpSky = (BITMAP*)get_asset(ass,"sky1");
    bmpClouds = (BITMAP*)get_asset(ass,"clouds1");
    bmpTiles = (BITMAP*)get_asset(ass,"tiles1");
    mapMain = (TILEMAP*)get_asset(ass,"testMap");

    // Set variables to their default values
    cloudPos = 0.0f;
    lavaPos = 0.0f;

    // Clear collision map & copy layer data
    stage_reset(false);
}


// Update stage
void stage_update(float tm)
{
    const float CLOUD_SPEED = 0.5f;
    const float LAVA_SPEED = 0.125f;

    // Update cloud position
    cloudPos -= CLOUD_SPEED * tm;
    if(cloudPos <= -bmpClouds->w)
    {
        cloudPos += bmpClouds->w;
    }

    // Update lava position
    lavaPos -= LAVA_SPEED * tm;
    if(lavaPos <= -M_PI*2 * 16.0f)
    {
        lavaPos += M_PI*2 * 16.0f;
    }

    // Update shake timer
    if(shakeTimer > 0.0f)
    {
        shakeTimer -= 1.0f * tm;
    }
}


// Draw stage
void stage_draw()
{
    if(shakeTimer > 0.0f)
    {
        int shakex = rand() % 7 - 3;
        int shakey = rand() % 7 - 3;

        translate(shakex,shakey);
    }
    else
    {
        translate(0,0);
    }

    draw_background();
    draw_map(mapMain);

    translate(0,0);
}


// Get collision map
int* stage_get_collision_map()
{
    return colMap;
}


// Get current map dimensions
POINT stage_get_map_size()
{
    return point(mapMain->width,mapMain->height);
}


// Is the tile in x,y solid
bool stage_is_solid(int x, int y)
{
    if(x < 0 || y < 0 || x >= mapMain->width || y >= mapMain->height)
        return true;

    int id = colMap[y * mapMain->width + x];

    return (id == 1 || (id >= 4 && id <= 6) || id == 17);
}


// Is the tile in x,y vine
bool stage_is_vine(int x, int y)
{
    if(x < 0 || y < 0 || x >= mapMain->width || y >= mapMain->height)
        return false;

    return layerData [y * mapMain->width + x] == 2;
}


// Set collision tile value
void stage_set_collision_tile(int x, int y, int id)
{
    if(x < 0 || y < 0 || x >= mapMain->width || y >= mapMain->height)
        return;

    colMap[y * mapMain->width + x] = id;
}


// Set tile
void stage_set_tile(int x, int y, int id)
{
    layerData [y*mapMain->width + x] = id;
}


// Is lava
bool stage_is_lava(int x, int y)
{
    if(x < 0 || y < 0 || x >= mapMain->width || y >= mapMain->height)
        return false;

    return layerData[y * mapMain->width + x] == 3;
}


// Is harmful
int stage_is_harmful(int x, int y)
{
    if(x < 0 || y < 0 || x >= mapMain->width || y >= mapMain->height)
        return false;

    int id = layerData[y * mapMain->width + x];
    int idy = colMap[ (y+1) * mapMain->width + x];
    if (id == 3 || idy == 4)
    {
        return id == 3 ? 2 : 1;   
    }
    return 0;
}


// Set shake timer value
void stage_set_shake_timer(float s)
{
    shakeTimer = s;
}


// Set stage name
void stage_set_main_stage(const char* name)
{
    ASSET_PACK* ass = get_global_assets();
    mapMain = (TILEMAP*)get_asset(ass,name);
}


/// Toggle purple blocks
void stage_toggle_purple_blocks()
{
    int i = 0;
    int id = 0;
    for(; i < mapMain->width*mapMain->height; ++ i)
    {
        id = layerData[i];
        if(id == 18)
        {
            layerData[i] = 17;
            colMap[i] = 1;
        }
        else if(id == 17)
        {
            layerData[i] = 18;
            colMap[i] = 0;
        }
    }
}