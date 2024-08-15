#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#include <86box/86box.h>
#include <86box/device.h>
#include <86box/plat.h>
#include <86box/gameport.h>

#include "../deps/mister/groovymister_wrapper.h"

int joysticks_present;
joystick_t joystick_state[MAX_JOYSTICKS];

plat_joystick_t plat_joystick_state[MAX_PLAT_JOYSTICKS];
static SDL_Joystick *sdl_joy[MAX_PLAT_JOYSTICKS];

void joystick_init(void)
{
        int c;
        
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
        joysticks_present = SDL_NumJoysticks();

        memset(sdl_joy, 0, sizeof(sdl_joy));
        for (c = 0; c < joysticks_present; c++)
        {
                sdl_joy[c] = SDL_JoystickOpen(c);
                
                if (sdl_joy[c])
                {
                        int d;
                        
                        pclog("Opened Joystick %i\n", c);
                        pclog(" Name: %s\n", SDL_JoystickName(sdl_joy[c]));
                        pclog(" Number of Axes: %d\n", SDL_JoystickNumAxes(sdl_joy[c]));
                        pclog(" Number of Buttons: %d\n", SDL_JoystickNumButtons(sdl_joy[c]));
                        pclog(" Number of Hats: %d\n", SDL_JoystickNumHats(sdl_joy[c]));
                        
                        strncpy(plat_joystick_state[c].name, SDL_JoystickNameForIndex(c), 64);
                        plat_joystick_state[c].nr_axes = SDL_JoystickNumAxes(sdl_joy[c]);
                        plat_joystick_state[c].nr_buttons = SDL_JoystickNumButtons(sdl_joy[c]);
                        plat_joystick_state[c].nr_povs = SDL_JoystickNumHats(sdl_joy[c]);
                        
                        for (d = 0; d < MIN(plat_joystick_state[c].nr_axes, 8); d++)
                        {
                                sprintf(plat_joystick_state[c].axis[d].name, "Axis %i", d);
                                plat_joystick_state[c].axis[d].id = d;
                        }
                        for (d = 0; d < MIN(plat_joystick_state[c].nr_buttons, 8); d++)
                        {
                                sprintf(plat_joystick_state[c].button[d].name, "Button %i", d);
                                plat_joystick_state[c].button[d].id = d;
                        }
                        for (d = 0; d < MIN(plat_joystick_state[c].nr_povs, 4); d++)
                        {
                                sprintf(plat_joystick_state[c].pov[d].name, "POV %i", d);
                                plat_joystick_state[c].pov[d].id = d;
                        }
                }
        }
        
        if (vid_mister)
        {        	
        	pclog("Opened Joystick %i\n", c);
        	pclog(" Name: %s\n", "MiSTer 1");
                pclog(" Number of Axes: %d\n", 4);
                pclog(" Number of Buttons: %d\n", 10);
                pclog(" Number of Hats: %d\n", 1);
                
                c++;
        	pclog("Opened Joystick %i\n", c);
        	pclog(" Name: %s\n", "MiSTer 2");
                pclog(" Number of Axes: %d\n", 4);
                pclog(" Number of Buttons: %d\n", 10);
                pclog(" Number of Hats: %d\n", 1);                              
        }
}
void joystick_close(void)
{
        int c;
        
        for (c = 0; c < joysticks_present; c++)
        {
                if (sdl_joy[c])
                        SDL_JoystickClose(sdl_joy[c]);
        }
}

static int joystick_get_axis(int joystick_nr, int mapping)
{
        if (mapping & POV_X)
        {
                switch (plat_joystick_state[joystick_nr].p[mapping & 3])
                {
                        case SDL_HAT_LEFTUP: case SDL_HAT_LEFT: case SDL_HAT_LEFTDOWN:
                        return -32767;

                        case SDL_HAT_RIGHTUP: case SDL_HAT_RIGHT: case SDL_HAT_RIGHTDOWN:
                        return 32767;

                        default:
                        return 0;
                }
        }
        else if (mapping & POV_Y)
        {
                switch (plat_joystick_state[joystick_nr].p[mapping & 3])
                {
                        case SDL_HAT_LEFTUP: case SDL_HAT_UP: case SDL_HAT_RIGHTUP:
                        return -32767;

                        case SDL_HAT_LEFTDOWN: case SDL_HAT_DOWN: case SDL_HAT_RIGHTDOWN:
                        return 32767;

                        default:
                        return 0;
                }
        }
        else
                return plat_joystick_state[joystick_nr].a[plat_joystick_state[joystick_nr].axis[mapping].id];
}
void joystick_process(void)
{
        int c, d;

        SDL_JoystickUpdate();
        for (c = 0; c < joysticks_present; c++)
        {
                int b;
                
                plat_joystick_state[c].a[0] = SDL_JoystickGetAxis(sdl_joy[c], 0);
                plat_joystick_state[c].a[1] = SDL_JoystickGetAxis(sdl_joy[c], 1);
                plat_joystick_state[c].a[2] = SDL_JoystickGetAxis(sdl_joy[c], 2);
                plat_joystick_state[c].a[3] = SDL_JoystickGetAxis(sdl_joy[c], 3);
                plat_joystick_state[c].a[4] = SDL_JoystickGetAxis(sdl_joy[c], 4);
                plat_joystick_state[c].a[5] = SDL_JoystickGetAxis(sdl_joy[c], 5);
                
                for (b = 0; b < 16; b++)
                        plat_joystick_state[c].b[b] = SDL_JoystickGetButton(sdl_joy[c], b);

                for (b = 0; b < 4; b++)
                        plat_joystick_state[c].p[b] = SDL_JoystickGetHat(sdl_joy[c], b);
//                pclog("joystick %i - x=%i y=%i b[0]=%i b[1]=%i  %i\n", c, joystick_state[c].x, joystick_state[c].y, joystick_state[c].b[0], joystick_state[c].b[1], joysticks_present);
        }
        
        if (vid_mister)
        {        	
        	gmw_fpgaJoyInputs joyInputs;
        	gmw_pollInputs();
 		gmw_getJoyInputs(&joyInputs); 	
 		//MiSTer 1	
 		plat_joystick_state[joysticks_present].a[0] = (joyInputs.joy1LXAnalog << 8) + joyInputs.joy1LXAnalog;
 		plat_joystick_state[joysticks_present].a[1] = (joyInputs.joy1LYAnalog << 8) + joyInputs.joy1LYAnalog;
 		plat_joystick_state[joysticks_present].a[2] = (joyInputs.joy1RXAnalog << 8) + joyInputs.joy1RXAnalog;
 		plat_joystick_state[joysticks_present].a[3] = (joyInputs.joy1RYAnalog << 8) + joyInputs.joy1RYAnalog;
 		plat_joystick_state[joysticks_present].p[0] = joyInputs.joy1 & GMW_JOY_UP;
 		plat_joystick_state[joysticks_present].p[1] = joyInputs.joy1 & GMW_JOY_RIGHT;
 		plat_joystick_state[joysticks_present].p[2] = joyInputs.joy1 & GMW_JOY_DOWN;
 		plat_joystick_state[joysticks_present].p[3] = joyInputs.joy1 & GMW_JOY_LEFT;
 		for(unsigned i = 0; i < 10; i++)
		 {  	
		   switch(i)
		   {
		   	case 0: plat_joystick_state[joysticks_present].b[0] = joyInputs.joy1 & GMW_JOY_B1;
		   	case 1: plat_joystick_state[joysticks_present].b[1] = joyInputs.joy1 & GMW_JOY_B2;
		   	case 2: plat_joystick_state[joysticks_present].b[2] = joyInputs.joy1 & GMW_JOY_B3;
		   	case 3: plat_joystick_state[joysticks_present].b[3] = joyInputs.joy1 & GMW_JOY_B4;
		   	case 4: plat_joystick_state[joysticks_present].b[4] = joyInputs.joy1 & GMW_JOY_B5;
		   	case 5: plat_joystick_state[joysticks_present].b[5] = joyInputs.joy1 & GMW_JOY_B6;
		   	case 6: plat_joystick_state[joysticks_present].b[6] = joyInputs.joy1 & GMW_JOY_B7;
		   	case 7: plat_joystick_state[joysticks_present].b[7] = joyInputs.joy1 & GMW_JOY_B8;
		   	case 8: plat_joystick_state[joysticks_present].b[8] = joyInputs.joy1 & GMW_JOY_B9;   	
		   	case 9: plat_joystick_state[joysticks_present].b[9] = joyInputs.joy1 & GMW_JOY_B10;   	
		   }	   
		 }
		//MiSTer 2 
		plat_joystick_state[joysticks_present+1].a[0] = (joyInputs.joy2LXAnalog << 8) + joyInputs.joy2LXAnalog;
 		plat_joystick_state[joysticks_present+1].a[1] = (joyInputs.joy2LYAnalog << 8) + joyInputs.joy2LYAnalog;
 		plat_joystick_state[joysticks_present+1].a[2] = (joyInputs.joy2RXAnalog << 8) + joyInputs.joy2RXAnalog;
 		plat_joystick_state[joysticks_present+1].a[3] = (joyInputs.joy2RYAnalog << 8) + joyInputs.joy2RYAnalog;
 		plat_joystick_state[joysticks_present+1].p[0] = joyInputs.joy2 & GMW_JOY_UP;
 		plat_joystick_state[joysticks_present+1].p[1] = joyInputs.joy2 & GMW_JOY_RIGHT;
 		plat_joystick_state[joysticks_present+1].p[2] = joyInputs.joy2 & GMW_JOY_DOWN;
 		plat_joystick_state[joysticks_present+1].p[3] = joyInputs.joy2 & GMW_JOY_LEFT;
 		for(unsigned i = 0; i < 10; i++)
		 {  	
		   switch(i)
		   {
		   	case 0: plat_joystick_state[joysticks_present+1].b[0] = joyInputs.joy2 & GMW_JOY_B1;
		   	case 1: plat_joystick_state[joysticks_present+1].b[1] = joyInputs.joy2 & GMW_JOY_B2;
		   	case 2: plat_joystick_state[joysticks_present+1].b[2] = joyInputs.joy2 & GMW_JOY_B3;
		   	case 3: plat_joystick_state[joysticks_present+1].b[3] = joyInputs.joy2 & GMW_JOY_B4;
		   	case 4: plat_joystick_state[joysticks_present+1].b[4] = joyInputs.joy2 & GMW_JOY_B5;
		   	case 5: plat_joystick_state[joysticks_present+1].b[5] = joyInputs.joy2 & GMW_JOY_B6;
		   	case 6: plat_joystick_state[joysticks_present+1].b[6] = joyInputs.joy2 & GMW_JOY_B7;
		   	case 7: plat_joystick_state[joysticks_present+1].b[7] = joyInputs.joy2 & GMW_JOY_B8;
		   	case 8: plat_joystick_state[joysticks_present+1].b[8] = joyInputs.joy2 & GMW_JOY_B9;   	
		   	case 9: plat_joystick_state[joysticks_present+1].b[9] = joyInputs.joy2 & GMW_JOY_B10;   	
		   }	   
		 }
        }
        
        for (c = 0; c < joystick_get_max_joysticks(joystick_type); c++)
        {
                if (joystick_state[c].plat_joystick_nr)
                {
                        int joystick_nr = joystick_state[c].plat_joystick_nr - 1;
                        
                        for (d = 0; d < joystick_get_axis_count(joystick_type); d++)
                                joystick_state[c].axis[d] = joystick_get_axis(joystick_nr, joystick_state[c].axis_mapping[d]);
                        for (d = 0; d < joystick_get_button_count(joystick_type); d++)
                                joystick_state[c].button[d] = plat_joystick_state[joystick_nr].b[joystick_state[c].button_mapping[d]];
                        for (d = 0; d < joystick_get_pov_count(joystick_type); d++)
                        {
                                int x, y;
                                double angle, magnitude;

                                x = joystick_get_axis(joystick_nr, joystick_state[c].pov_mapping[d][0]);
                                y = joystick_get_axis(joystick_nr, joystick_state[c].pov_mapping[d][1]);
                                
                                angle = (atan2((double)y, (double)x) * 360.0) / (2*M_PI);
                                magnitude = sqrt((double)x*(double)x + (double)y*(double)y);
                                
                                if (magnitude < 16384)
                                        joystick_state[c].pov[d] = -1;
                                else
                                        joystick_state[c].pov[d] = ((int)angle + 90 + 360) % 360;
                        }
                }
                else
                {
                        for (d = 0; d < joystick_get_axis_count(joystick_type); d++)
                                joystick_state[c].axis[d] = 0;
                        for (d = 0; d < joystick_get_button_count(joystick_type); d++)
                                joystick_state[c].button[d] = 0;
                        for (d = 0; d < joystick_get_pov_count(joystick_type); d++)
                                joystick_state[c].pov[d] = -1;
                }
        }
}
