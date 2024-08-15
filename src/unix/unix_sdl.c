#include <SDL.h>
#include <SDL_messagebox.h>

#include <time.h>
#include <sys/time.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/param.h>
/* This #undef is needed because a SDL include header redefines HAVE_STDARG_H. */
#undef HAVE_STDARG_H
#define HAVE_STDARG_H
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/plat.h>
#include <86box/plat_dynld.h>
#include <86box/video.h>
#include <86box/ui.h>
#include <86box/version.h>
#include <86box/unix_sdl.h>

#include "../deps/switchres/switchres_wrapper.h" //psakhis

#define RENDERER_FULL_SCREEN 1
#define RENDERER_HARDWARE    2
#define RENDERER_OPENGL      4

typedef struct sdl_blit_params {
    int x, y, w, h;
} sdl_blit_params;
extern sdl_blit_params params;
extern int             blitreq;

static SDL_Window   *sdl_win    = NULL;
static SDL_Renderer *sdl_render = NULL;
static SDL_Texture *sdl_tex    = NULL;
int                 sdl_w = SCREEN_RES_X, sdl_h = SCREEN_RES_Y;
static int          sdl_fs, sdl_flags           = -1;
static int          cur_w, cur_h;
static int          cur_wx = 0, cur_wy = 0, cur_ww = 0, cur_wh = 0;
static volatile int sdl_enabled = 1;
static SDL_mutex   *sdl_mutex   = NULL;

int                 title_set         = 0;
int                 resize_pending    = 0;
int                 resize_w          = 0;
int                 resize_h          = 0;
//double              mouse_sensitivity = 1.0;                  /* Unused. */
//double              mouse_x_error = 0.0, mouse_y_error = 0.0; /* Unused. */
static uint8_t      interpixels[17842176];

//psakhis
static unsigned char retSR; 
static double       sw_x_scale = 0;     
static double       sw_y_scale = 0;     
static int          sw_req_w = 0;     
static int          sw_req_h = 0;     
static double       sw_req_vfreq = 0;  
static SDL_TimerID  sw_timer;
static int          sw_switch = 1;

extern void RenderImGui(void);
static void
sdl_integer_scale(double *d, double *g)
{
    double ratio;

    if (*d > *g) {
        ratio = floor(*d / *g);
        *d    = *g * ratio;
    } else {
        ratio = ceil(*d / *g);
        *d    = *g / ratio;
    }
}

void sdl_reinit_texture(void);

static int
sdl_display(void)
{
   int num_displays;
   SDL_Rect dbr;

   if((num_displays = SDL_GetNumVideoDisplays()) < 0)
   {
    pclog("SDL_GetNumVideoDisplays() failed: %s\n", SDL_GetError());
    return 0;
   }	   

   if(SDL_GetDisplayBounds(vid_display, &dbr) < 0)
   {
    pclog("SDL_GetDisplayBounds() failed: %s\n", SDL_GetError());
    return 0;
   }
   
   if (vid_display) {
     SDL_SetWindowPosition(sdl_win, dbr.x, dbr.y);
     SDL_SetWindowSize(sdl_win, dbr.w, dbr.h);        		
   }
   
   return 1;	
}

static void
sdl_stretch(int *w, int *h, int *x, int *y)
{
    double hw, gw, hh, gh, dx, dy, dw, dh, gsr, hsr;
    int    real_sdl_w, real_sdl_h;

    SDL_GL_GetDrawableSize(sdl_win, &real_sdl_w, &real_sdl_h);              

    hw  = (double) real_sdl_w;
    hh  = (double) real_sdl_h;
    gw  = (double) *w;
    gh  = (double) *h;
    hsr = hw / hh; 

    switch (video_fullscreen_scale) {    
        case FULLSCR_SCALE_FULL:
        default:           
            *w = real_sdl_w;
            *h = real_sdl_h;                        
            *x = 0;            
            *y = 0;             
            break;
        case FULLSCR_SCALE_43:
        case FULLSCR_SCALE_KEEPRATIO:
            if (video_fullscreen_scale == FULLSCR_SCALE_43)
                gsr = 4.0 / 3.0;
            else
                gsr = gw / gh;
            if (gsr <= hsr) {
                dw = hh * gsr;
                dh = hh;
            } else {
                dw = hw;
                dh = hw / gsr;
            }
            dx = (hw - dw) / 2.0;
            dy = (hh - dh) / 2.0;
            *w = (int) dw;
            *h = (int) dh;
            *x = (int) dx;
            *y = (int) dy;
            break;
        case FULLSCR_SCALE_INT:
            gsr = gw / gh;
            if (gsr <= hsr) {
                dw = hh * gsr;
                dh = hh;
            } else {
                dw = hw;
                dh = hw / gsr;
            }
            sdl_integer_scale(&dw, &gw);
            sdl_integer_scale(&dh, &gh);
            dx = (hw - dw) / 2.0;
            dy = (hh - dh) / 2.0;
            *w = (int) dw;
            *h = (int) dh;
            *x = (int) dx;
            *y = (int) dy;           
            break;
    }
    
    if (!vid_mister && video_fullscreen && sw_x_scale >= 1 && sw_y_scale >= 1)
    {
    	*w = floor(0.5 + video_sw_req_w * sw_x_scale);
    	*h = floor(0.5 + video_sw_req_h * sw_y_scale);
    	*x = (real_sdl_w - *w) / 2;
    	*y = (real_sdl_h - *h) / 2;
    	//printf("%d %f %d %f %d %d\n",video_sw_req_w,sw_x_scale,video_sw_req_h,sw_y_scale, real_sdl_w, real_sdl_h);              	
    }
}

void
sdl_blit_shim(int x, int y, int w, int h, int monitor_index)
{        	       
    params.x = x;
    params.y = y;
    params.w = w;
    params.h = h;        
    if (!(!sdl_enabled || (x < 0) || (y < 0) || (w <= 0) || (h <= 0) || (w > 2048) || (h > 2048) || (buffer32 == NULL) || (sdl_render == NULL) || (sdl_tex == NULL)) || (monitor_index >= 1)) 
        video_copy(interpixels, &(buffer32->line[y][x]), h * 2048 * sizeof(uint32_t));           
    if (screenshots)
        video_screenshot((uint32_t *) interpixels, 0, 0, 2048);
    blitreq = 1;
    video_blit_complete_monitor(monitor_index);
}

void ui_window_title_real(void);

void
sdl_real_blit(SDL_Rect *r_src)
{
    SDL_Rect r_dst;
    int      ret, winx, winy;
    SDL_GL_GetDrawableSize(sdl_win, &winx, &winy); 
    SDL_RenderClear(sdl_render);

    r_dst   = *r_src;
    r_dst.x = r_dst.y = 0;   

    if (sdl_fs) {
        sdl_stretch(&r_dst.w, &r_dst.h, &r_dst.x, &r_dst.y);
    } else {
        r_dst.w *= ((float) winx / (float) r_dst.w);
        r_dst.h *= ((float) winy / (float) r_dst.h);
    }    
    ret = SDL_RenderCopy(sdl_render, sdl_tex, r_src, &r_dst);
    if (ret)
        fprintf(stderr, "SDL: unable to copy texture to renderer (%s)\n", SDL_GetError());

    SDL_RenderPresent(sdl_render);
}

Uint32 sw_timer_set(Uint32 interval, void* param)
{
        sw_switch = 1;
        SDL_RemoveTimer(sw_timer);
        return interval;
}

void
sdl_blit(int x, int y, int w, int h)
{
    SDL_Rect r_src;

    if (!sdl_enabled || (x < 0) || (y < 0) || (w <= 0) || (h <= 0) || (w > 2048) || (h > 2048) || (buffer32 == NULL) || (sdl_render == NULL) || (sdl_tex == NULL)) {
        r_src.x = x;
        r_src.y = y;
        r_src.w = w;
        r_src.h = h;        
        sdl_real_blit(&r_src);
        blitreq = 0;
        return;
    }
   
    SDL_LockMutex(sdl_mutex);
    
    //psakhis 
    if (!vid_mister && video_fullscreen)
    {
    	if (sw_req_w != video_sw_req_w || sw_req_h != video_sw_req_h || fabs(sw_req_vfreq - video_sw_req_vfreq) > 0.1)    
    	{
    		if (!sw_switch)
    		   sw_timer = SDL_AddTimer(50, sw_timer_set, NULL);
    		else
    		{	    	
    			sw_switch = 0;		    					  		    			
	    		pclog("Apply from %dx%d@%f to %dx%d@%f\n", sw_req_w, sw_req_h, sw_req_vfreq, video_sw_req_w, video_sw_req_h, video_sw_req_vfreq);
	    		sw_req_w      = video_sw_req_w;
			sw_req_h      = video_sw_req_h;
			sw_req_vfreq  = video_sw_req_vfreq;
	    		sr_mode swres_result;
	    		int sr_mode_flags = 0;     		
	    		struct timeval tval_before, tval_after, tval_result;
		        gettimeofday(&tval_before, NULL);	       	      
		    	if (video_sw_req_h > 288)     		
		           sr_mode_flags |= SR_MODE_INTERLACED;	    		    	
		    	retSR = sr_add_mode(video_sw_req_w, video_sw_req_h <= 240 ? 240 : video_sw_req_h >= 350 ? 480 : video_sw_req_h, video_sw_req_vfreq, sr_mode_flags ,&swres_result);  
		    	if (retSR && (swres_result.width != video_sw_w || swres_result.height != video_sw_h || fabs(swres_result.vfreq - video_sw_vfreq) > 0.1)) 
		    	{         	
		    		retSR = sr_set_mode(swres_result.id);              
	#ifdef _WIN32
				SDL_SetWindowSize(sdl_win, swres_result.width, swres_result.height);
	#endif		
			}
			sw_x_scale     = swres_result.x_scale;
			sw_y_scale     = swres_result.y_scale;			
			video_sw_w     = swres_result.width;
			video_sw_h     = swres_result.height;
			video_sw_vfreq = swres_result.vfreq;			      	        
		        gettimeofday(&tval_after, NULL);
		        timersub(&tval_after, &tval_before, &tval_result);
		        pclog("Mode applied, time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);	
		}
	}	    
    }        
     
    //not used on fullscreen       
    if (resize_pending) {
        if (!video_fullscreen)
            sdl_resize(resize_w, resize_h);
        resize_pending = 0;
    }    
            
    r_src.x = x;
    r_src.y = y;
    r_src.w = w;
    r_src.h = h;             
    SDL_UpdateTexture(sdl_tex, &r_src, interpixels, 2048 * 4);
    blitreq = 0;    
              
    sdl_real_blit(&r_src);
    SDL_UnlockMutex(sdl_mutex);
}

static void
sdl_destroy_window(void)
{
    if (sdl_win != NULL) {
        if (window_remember) {
            SDL_GetWindowSize(sdl_win, &window_w, &window_h);
            if (strncasecmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0) {
                SDL_GetWindowPosition(sdl_win, &window_x, &window_y);
            }
        }
        SDL_DestroyWindow(sdl_win);
        sdl_win = NULL;
    }
}

static void
sdl_destroy_texture(void)
{
    /* SDL_DestroyRenderer also automatically destroys all associated textures. */
    if (sdl_render != NULL) {
        SDL_DestroyRenderer(sdl_render);
        sdl_render = NULL;
    }
}

void
sdl_close(void)
{    
    
    if (sdl_mutex != NULL)
        SDL_LockMutex(sdl_mutex);   

    /* Unregister our renderer! */
    video_setblit(NULL);

    if (sdl_enabled)
        sdl_enabled = 0;

    if (sdl_mutex != NULL) {
        SDL_DestroyMutex(sdl_mutex);
        sdl_mutex = NULL;
    }   

    sdl_destroy_texture();             
    sdl_destroy_window();       
    
    /* Quit. */
    SDL_Quit();
    sdl_flags = -1;        
}

void
sdl_enable(int enable)
{
    if (sdl_flags == -1)
        return;

    SDL_LockMutex(sdl_mutex);
    sdl_enabled = !!enable;

    if (enable == 1) {
        SDL_SetWindowSize(sdl_win, cur_ww, cur_wh);
        sdl_reinit_texture();
    }

    SDL_UnlockMutex(sdl_mutex);
}

static void
sdl_select_best_hw_driver(void)
{
    int              i;
    SDL_RendererInfo renderInfo;

    for (i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
        SDL_GetRenderDriverInfo(i, &renderInfo);
        if (renderInfo.flags & SDL_RENDERER_ACCELERATED) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderInfo.name);
            return;
        }
    }
}

void
sdl_reinit_texture(void)
{
    sdl_destroy_texture();

    if (sdl_flags & RENDERER_HARDWARE) {    	
    	sdl_flags = SDL_RENDERER_ACCELERATED;
    	sdl_flags |= SDL_RENDERER_PRESENTVSYNC;        
        sdl_render = SDL_CreateRenderer(sdl_win, -1, sdl_flags);        
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, video_filter_method ? "1" : "0");
    } else
        sdl_render = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_SOFTWARE);

    sdl_tex = SDL_CreateTexture(sdl_render, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, 2048, 2048);
}

void
sdl_set_fs(int fs)
{	 
    SDL_LockMutex(sdl_mutex);     
    SDL_SetWindowFullscreen(sdl_win, fs ? SDL_WINDOW_FULLSCREEN : 0);       
    plat_mouse_capture(fs);
      
    sdl_fs = fs;

    if (fs)
        sdl_flags |= RENDERER_FULL_SCREEN;
    else
        sdl_flags &= ~RENDERER_FULL_SCREEN;

    sdl_reinit_texture();
    SDL_UnlockMutex(sdl_mutex);
}

void
sdl_resize(int x, int y)
{
    int ww = 0, wh = 0, wx = 0, wy = 0;       
    
    if (video_fullscreen & 2)
        return;

    if ((x == cur_w) && (y == cur_h))
        return;

    SDL_LockMutex(sdl_mutex);

    ww = x;
    wh = y;

    cur_w = x;
    cur_h = y;

    cur_wx = wx;
    cur_wy = wy;
    cur_ww = ww;
    cur_wh = wh;    

    SDL_SetWindowSize(sdl_win, cur_ww, cur_wh);
    SDL_GL_GetDrawableSize(sdl_win, &sdl_w, &sdl_h);

    sdl_reinit_texture();

    SDL_UnlockMutex(sdl_mutex);
}
void
sdl_reload(void)
{
    if (sdl_flags & RENDERER_HARDWARE) {
        SDL_LockMutex(sdl_mutex);

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, video_filter_method ? "1" : "0");
        sdl_reinit_texture();

        SDL_UnlockMutex(sdl_mutex);
    }
}

static int
sdl_init_common(int flags)
{   
    SDL_version ver;

    /* Get and log the version of the DLL we are using. */
    SDL_GetVersion(&ver);
    fprintf(stderr, "SDL: version %d.%d.%d\n", ver.major, ver.minor, ver.patch);

    /* Initialize the SDL system. */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL: initialization failed (%s)\n", SDL_GetError());
        return (0);
    }

    if (flags & RENDERER_HARDWARE) {
        if (flags & RENDERER_OPENGL) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "OpenGL");
        } else
            sdl_select_best_hw_driver();
    }

    sdl_mutex = SDL_CreateMutex();
    sdl_win   = SDL_CreateWindow("86Box", strncasecmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0 && window_remember ? window_x : SDL_WINDOWPOS_CENTERED, strncasecmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0 && window_remember ? window_y : SDL_WINDOWPOS_CENTERED, scrnsz_x, scrnsz_y, SDL_WINDOW_OPENGL | (vid_resize & 1 ? SDL_WINDOW_RESIZABLE : 0));    
    
    if (!sdl_display()) {
    	pclog("Failed to index display %d.\n", vid_display);
    	sdl_close();
    	return (0);
    }    
    printf("sdl_opengl started\n");
    sdl_set_fs(video_fullscreen);   

    //psakhis init switchres    
    if (!vid_mister && video_fullscreen)
    {
    	sr_init(); 
    	char sr_monitor[256];
    	sprintf(sr_monitor, "%d", vid_display);        
    	if (vid_display) {
    	   printf("monitor detected %d\n", vid_display);
     	   retSR=sr_init_disp(sr_monitor, sdl_win);
     	} 
    	else {
    	   printf("monitor auto \n");
           retSR=sr_init_disp("auto", sdl_win);            
        }   
        sw_req_w = 640;
    	sw_req_h = 480; 
    	sw_req_vfreq = 59.701000; 
    	SDL_GL_GetDrawableSize(sdl_win, &video_sw_w, &video_sw_h);      
    }	       
    //end psakhis

    if (!(video_fullscreen & 1)) {
        if (vid_resize & 2)
            plat_resize(fixed_size_x, fixed_size_y, 0);
        else
            plat_resize(scrnsz_x, scrnsz_y, 0);
    }
    if ((vid_resize < 2) && window_remember) {
        SDL_SetWindowSize(sdl_win, window_w, window_h);
    } 
    
    /* Make sure we get a clean exit. */
    atexit(sdl_close);
    atexit(sr_deinit);
    
    /* Register our renderer! */
    video_setblit(sdl_blit_shim);

    sdl_enabled = 1;    

    return (1);
}

int
sdl_inits(void)
{
    return sdl_init_common(0);
}

int
sdl_inith(void)
{
    return sdl_init_common(RENDERER_HARDWARE);
}

int
sdl_initho(void)
{
    return sdl_init_common(RENDERER_HARDWARE | RENDERER_OPENGL);
}

int
sdl_pause(void)
{
    return (0);
}

void
plat_resize(int w, int h, UNUSED(int monitor_index)) //not used on fullscreen
{  
    SDL_LockMutex(sdl_mutex);
    resize_w       = w;
    resize_h       = h;
    resize_pending = 1;    
    SDL_UnlockMutex(sdl_mutex);
}

wchar_t    sdl_win_title[512] = { L'8', L'6', L'B', L'o', L'x', 0 };
SDL_mutex *titlemtx           = NULL;

void
ui_window_title_real(void)
{
    char *res;
    if (sizeof(wchar_t) == 1) {
        SDL_SetWindowTitle(sdl_win, (char *) sdl_win_title);
        return;
    }
    res = SDL_iconv_string("UTF-8", sizeof(wchar_t) == 2 ? "UTF-16LE" : "UTF-32LE", (char *) sdl_win_title, wcslen(sdl_win_title) * sizeof(wchar_t) + sizeof(wchar_t));
    if (res) {
        SDL_SetWindowTitle(sdl_win, res);
        SDL_free((void *) res);
    }
    title_set = 0;
}
extern SDL_threadID eventthread;

/* Only activate threading path on macOS, otherwise it will softlock Xorg.
   Wayland doesn't seem to have this issue. */
wchar_t *
ui_window_title(wchar_t *str)
{
    if (!str)
        return sdl_win_title;
#ifdef __APPLE__
    if (eventthread == SDL_ThreadID())
#endif
    {
        memset(sdl_win_title, 0, sizeof(sdl_win_title));
        wcsncpy(sdl_win_title, str, 512);
        ui_window_title_real();
        return str;
    }
#ifdef __APPLE__
    memset(sdl_win_title, 0, sizeof(sdl_win_title));
    wcsncpy(sdl_win_title, str, 512);
    title_set = 1;
#endif
    return str;
}

void
ui_init_monitor(int monitor_index)
{
}
void
ui_deinit_monitor(int monitor_index)
{
}

void
plat_resize_request(int w, int h, int monitor_index)
{
    atomic_store((&doresize_monitors[monitor_index]), 1);
}
