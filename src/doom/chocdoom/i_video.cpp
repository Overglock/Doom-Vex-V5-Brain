// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------
#include "liblvgl/display/lv_display.h"
static const char rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "config.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_video.h"
#include "z_zone.h"

#include "tables.h"
#include "doomkeys.h"

#include <stdint.h>
#include <stdbool.h>

#include "pros/apix.h"
#include "api.h"
using namespace pros;

extern int key_nextweapon;
extern int key_prevweapon;

// Display resolution will be fetched at runtime
static int display_width = 0;
static int display_height = 0;

void update_controller(void);
void check_button(int prev, int curr, int action);

struct controller_state {
    int32_t
        a_lx,
        a_ly,
        a_rx,
        a_ry,
        d_l1,
        d_l2,
        d_r1,
        d_r2,
        d_up,
        d_dwn,
        d_lft,
        d_rig,
        d_x,
        d_b,
        d_y,
        d_a;
}; 

static struct controller_state c_state = { 0 };
static struct controller_state c_oldstate = { 0 };
static event_t event;

// Canvas for rendering to display
static lv_obj_t *canvas = NULL;
static lv_draw_buf_t *canvas_buf = NULL;

// The screen buffer; this is modified to draw things to the screen
byte *I_VideoBuffer = NULL;

// If true, game is running as a screensaver
boolean screensaver_mode = false;

// Gamma correction level to use
int usegamma = 0;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 4.0;
int mouse_threshold = 10;

int usemouse = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;

typedef struct {
    byte r;
    byte g;
    byte b;
} col_t;

// Palette converted to LVGL 9 colors
static lv_color_t rgb888_palette[256];

// Scaled framebuffer (maximum display size)
static uint8_t *scaled_buffer = NULL;
static size_t scaled_buffer_size = 0;

// run state
static bool run;

void I_InitGraphics(void)
{
    I_VideoBuffer = (byte*)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    
    // Get display resolution at runtime
    // For VEX V5, this is typically 480x272, but we query it anyway
    lv_display_t *disp = lv_display_get_default();
    if (!disp) {
        // Fallback to VEX V5 brain display dimensions
        display_width = 480;
        display_height = 272;
        printf("WARNING: No LVGL display found, using fallback 480x272\n");
    } else {
        display_width = lv_display_get_horizontal_resolution(disp);
        display_height = lv_display_get_vertical_resolution(disp);
    }
    
    // Allocate scaled framebuffer
    scaled_buffer_size = display_width * display_height;
    scaled_buffer = (uint8_t*)Z_Malloc(scaled_buffer_size, PU_STATIC, NULL);
    if (!scaled_buffer) {
        printf("ERROR: Failed to allocate scaled framebuffer!\n");
        return;
    }
    
    memset(scaled_buffer, 0, scaled_buffer_size);
    
    // Create canvas to display the framebuffer
    lv_obj_t *parent = lv_screen_active();
    canvas = lv_canvas_create(parent);
    if (!canvas) {
        printf("ERROR: Failed to create canvas!\n");
        return;
    }
    
    // Set canvas size and buffer
    lv_canvas_set_buffer(canvas, scaled_buffer, display_width, display_height, 
                         LV_COLOR_FORMAT_RGB888);
    lv_obj_set_size(canvas, display_width, display_height);
    lv_obj_set_pos(canvas, 0, 0);
}


void I_ShutdownGraphics(void)
{
    if (canvas) {
        lv_obj_delete(canvas);
        canvas = NULL;
    }
    
    if (scaled_buffer) {
        Z_Free(scaled_buffer);
        scaled_buffer = NULL;
        scaled_buffer_size = 0;
    }
    
    Z_Free(I_VideoBuffer);
}

void I_StartFrame(void) { }

void update_controller(void)
{
    c_state.a_lx  = pros::c::controller_get_analog(E_CONTROLLER_MASTER,  E_CONTROLLER_ANALOG_LEFT_X);
    c_state.a_ly  = pros::c::controller_get_analog(E_CONTROLLER_MASTER,  E_CONTROLLER_ANALOG_LEFT_Y);
    c_state.a_rx  = pros::c::controller_get_analog(E_CONTROLLER_MASTER,  E_CONTROLLER_ANALOG_RIGHT_X);
    c_state.a_ry  = pros::c::controller_get_analog(E_CONTROLLER_MASTER,  E_CONTROLLER_ANALOG_RIGHT_Y);
    c_state.d_l1  = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_L1);
    c_state.d_l2  = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_L2);
    c_state.d_r1  = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_R1);
    c_state.d_r2  = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_R2);
    c_state.d_up  = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_UP);
    c_state.d_dwn = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_DOWN);
    c_state.d_lft = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_LEFT);
    c_state.d_rig = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_RIGHT);
    c_state.d_x   = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_X);
    c_state.d_b   = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_B);
    c_state.d_y   = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_Y);
    c_state.d_a   = pros::c::controller_get_digital(E_CONTROLLER_MASTER, E_CONTROLLER_DIGITAL_A);
}

void check_button(int prev, int curr, int action)
{
    if (!prev && curr) {
        event.type = ev_keydown;
        event.data1 = action; //key code
        D_PostEvent(&event);
    } else if (prev && !curr) {
        event.type = ev_keyup;
        event.data1 = action;
        D_PostEvent(&event);
    }
}

void I_GetEvent(void)
{
    c_oldstate = c_state;
    update_controller();

    //joystick and strafe
    //right x = turn left/right
    //left y = move forward/back
    //left x = strafe left/right
    event.type = ev_joystick;
    event.data1 = 0; //bitfield of buttons
    event.data2 = c_state.a_rx; //x axis mouse (turn)
    event.data3 = -c_state.a_ly; //y axis mouse (forward/backward)
    event.data4 = c_state.a_lx; //3rd axis mouse (strafe)
    D_PostEvent(&event);

    //other buttons
    //r1 = fire
    check_button(c_oldstate.d_r1, c_state.d_r1, KEY_FIRE);
    //b = use
    check_button(c_oldstate.d_b, c_state.d_b, KEY_USE);
    //x = enter
    check_button(c_oldstate.d_x, c_state.d_x, KEY_ENTER);
    //y = escape
    check_button(c_oldstate.d_y, c_state.d_y, KEY_ESCAPE);

    //dpad is arrow keys
    check_button(c_oldstate.d_up,  c_state.d_up,  KEY_UPARROW);
    check_button(c_oldstate.d_dwn, c_state.d_dwn, KEY_DOWNARROW);
    check_button(c_oldstate.d_lft, c_state.d_lft, KEY_LEFTARROW);
    check_button(c_oldstate.d_rig, c_state.d_rig, KEY_RIGHTARROW);

    //l2 = prev weapon
    check_button(c_oldstate.d_l1, c_state.d_l2, key_prevweapon);

    //r2 = next weapon
    check_button(c_oldstate.d_r1, c_state.d_r2, key_nextweapon);
}

void I_StartTic(void) {
    I_GetEvent();
}

void I_UpdateNoBlit(void) { }

void I_FinishUpdate(void)
{
    if (!scaled_buffer || display_width <= 0 || display_height <= 0) {
        return;
    }

    const int w1 = SCREENWIDTH;
    const int h1 = SCREENHEIGHT;
    const int w2 = display_width;
    const int h2 = display_height;

    const int x_ratio = ((w1 << 16) / w2) + 1;
    const int y_ratio = ((h1 << 16) / h2) + 1;

    // Scale from I_VideoBuffer to a temp buffer (palette indices)
    uint8_t *temp_indices = (uint8_t *)Z_Malloc(scaled_buffer_size, PU_CACHE, NULL);
    
    for (int i = 0; i < h2; ++i) {
        for (int j = 0; j < w2; ++j) {
            int x2 = (j * x_ratio) >> 16;
            int y2 = (i * y_ratio) >> 16;
            temp_indices[(i * w2) + j] = I_VideoBuffer[(y2 * w1) + x2];
        }
    }

    // Convert palette indices to RGB888 colors in canvas buffer
    uint32_t *canvas_data = (uint32_t *)scaled_buffer;
    for (int i = 0; i < scaled_buffer_size; ++i) {
        uint8_t palette_idx = temp_indices[i];
        lv_color_t color = rgb888_palette[palette_idx];
        canvas_data[i] = lv_color_to_u32(color);
    }
    
    Z_Free(temp_indices);

    if (canvas) {
        lv_obj_invalidate(canvas);
    }
}



void I_ReadScreen(byte *scr)
{
    memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

void I_SetPalette(byte *palette)
{
    for (int i = 0; i < 256; ++i) {
        uint8_t r = gammatable[usegamma][palette[0]];
        uint8_t g = gammatable[usegamma][palette[1]];
        uint8_t b = gammatable[usegamma][palette[2]];

        // Create LVGL 9 color from RGB888
        rgb888_palette[i] = lv_color_make(r, g, b);

        palette += 3;
    }
}

int I_GetPaletteIndex(int r, int g, int b)
{
    int best = 0;
    int best_diff = INT_MAX;

    for (int i = 0; i < 256; ++i) {
        int diff = 
            (r - rgb888_palette[i].red)   * (r - rgb888_palette[i].red) +
            (g - rgb888_palette[i].green) * (g - rgb888_palette[i].green) +
            (b - rgb888_palette[i].blue)  * (b - rgb888_palette[i].blue);

        if (diff < best_diff) {
            best = i;
            best_diff = diff;
        }

        if (diff == 0) {
            break;
        }
    }

    return best;
}

void I_BeginRead(void) { }
void I_EndRead(void) { }
void I_SetWindowTitle(char *title) { }
void I_GraphicsCheckCommandLine(void) { }
void I_SetGrabMouseCallback(grabmouse_callback_t func) { }
void I_EnableLoadingDisk(void) { }
void I_BindVideoVariables(void) { }
void I_DisplayFPSDots(boolean dots_on) { }
void I_CheckIsScreensaver(void) { }

