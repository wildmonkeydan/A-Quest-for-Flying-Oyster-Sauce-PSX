/// Stage (source)
/// (c) 2018 Jani Nykänen

#include "stage.h"

#include "../engine/graphics.h"
#include "../lib/tmxc.h"

#include "math.h"
#include "stdlib.h"

// Bitmaps
static BITMAP* bmpSky;
static BITMAP* bmpClouds;
static BITMAP* bmpTiles;

// Map
static TILEMAP* mapMain;

// Cloud position
static float cloudPos;

// Is the tile in (x+dx,y+dy) same as in (x,y)
static bool is_same_tile(TILEMAP* t, int l, int id, int x, int y, int dx, int dy)
{
    if(x+dx < 0 || y +dy < 0 || x+dx >= t->width || y+dy >= t->height)
        return true;

    LAYER data = t->layers[l];

    return id == data[(y+dy)*t->width + x + dx];
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
    if(!is_same_tile(t,0,1,x,y,0,1))
    {
        t12 = point(10,0);
        t22 = point(11,0);
    }

    // Right tile is different
    if(!is_same_tile(t,0,1,x,y,1,0))
    {
        t21 = point(5,0);
        t22 = point(5,1);

        // Bottom
        if(!is_same_tile(t,0,1,x,y,0,1))
        {
            t22 = point(3,1);
        }
    }

    // Left tile is different
    if(!is_same_tile(t,0,1,x,y,-1,0))
    {
        t11 = point(4,0);
        t12 = point(4,1);

        // Bottom
        if(!is_same_tile(t,0,1,x,y,0,1))
        {
            t12 = point(2,1);
        }
    }

    // Upper tile is different
    if(!is_same_tile(t,0,1,x,y,0,-1))
    {
        t11 = point(0,0);
        t21 = point(1,0);

        // Right
        if(!is_same_tile(t,0,1,x,y,1,0))
        {
            t21 = point(9,1);
            rightGreen = true;
        }

        // Left
        if(!is_same_tile(t,0,1,x,y,-1,0))
        {
            t11 = point(8,1);
            leftGreen = true;
        }
    }

    // Bottom-right corner
    if(!is_same_tile(t,0,1,x,y,1,1) && is_same_tile(t,0,1,x,y,1,0) 
        && is_same_tile(t,0,1,x,y,0,1))
    {
        t22 = point(8,0);
    }

    // Bottom-left corner
    if(!is_same_tile(t,0,1,x,y,-1,1) && is_same_tile(t,0,1,x,y,-1,0) 
        && is_same_tile(t,0,1,x,y,0,1))
    {
        t12 = point(9,0);
    }

    // Top-right corner
    if(!is_same_tile(t,0,1,x,y,1,-1) && is_same_tile(t,0,1,x,y,1,0) 
        && is_same_tile(t,0,1,x,y,0,-1))
    {
        t21 = point(6,1);
    }

    // Top-left corner
    if(!is_same_tile(t,0,1,x,y,-1,-1) && is_same_tile(t,0,1,x,y,-1,0) 
        && is_same_tile(t,0,1,x,y,0,-1))
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
    if(!is_same_tile(t,0,2,x,y,0,-1) && !is_same_tile(t,0,1,x,y,0,-1))
    {
        sx1 += 16;
    }

    // Below
    if(!is_same_tile(t,0,2,x,y,0,1) && !is_same_tile(t,0,1,x,y,0,1))
    {
        sx2 += 16;
    }

    // Upper part
    draw_bitmap_region(bmpTiles,sx1,sy1,16,8,x*16,y*16,0);
    // Bottom part
    draw_bitmap_region(bmpTiles,sx2,sy2,16,8,x*16,y*16 + 8,0);
}


// Draw map
static void draw_map(TILEMAP* t)
{
    int x = 0;
    int y = 0;
    int id = 0;
    LAYER data = t->layers[0];

    for(; y < t->height; ++ y)
    {
        for(x=0; x < t->width; ++ x)
        {
            id = data[y*t->width + x];
            if(id == 0) continue;

            if(id == 1)
            {
                draw_tile_soil(t, x,y);
            }
            else if(id == 2)
            {
                draw_vine(t, x,y);
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
}


// Update stage
void stage_update(float tm)
{
    const float CLOUD_SPEED = 0.5f;

    cloudPos -= CLOUD_SPEED * tm;
    if(cloudPos <= -bmpClouds->w)
    {
        cloudPos += bmpClouds->w;
    }
}


// Draw stage
void stage_draw()
{
    draw_background();
    draw_map(mapMain);
}