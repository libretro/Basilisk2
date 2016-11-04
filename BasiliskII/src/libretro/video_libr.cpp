#include "libretroexports.h"
#include "memalign.h"

//must be included first or other headers will throw errors
#include "sysdeps.h"

#include "video.h"
#include "video_blit.h"

// Supported video modes
static vector<video_mode> VideoModes;

static bool redraw_thread_active = false;			// Flag: Redraw thread installed
static uint8 *the_buffer = NULL;                // Mac frame buffer (where MacOS draws into)
static uint8 *the_buffer_copy = NULL;				// Copy of Mac frame buffer (for refreshed modes)
static uint32 the_buffer_size;						// Size of allocated the_buffer

// Add mode to list of supported modes
static void add_mode(uint32 width, uint32 height, uint32 resolution_id, uint32 bytes_per_row, video_depth depth)
{
   video_mode mode;
   mode.x = width;
   mode.y = height;
   mode.resolution_id = resolution_id;
   mode.bytes_per_row = bytes_per_row;
   mode.depth = depth;
   mode.user_data = 0;
   VideoModes.push_back(mode);
}

// Add standard list of windowed modes for given color depth
static void add_window_modes(video_depth depth)
{
   add_mode(512, 384, 0x80, TrivialBytesPerRow(512, depth), depth);
   add_mode(640, 480, 0x81, TrivialBytesPerRow(640, depth), depth);
   add_mode(800, 600, 0x82, TrivialBytesPerRow(800, depth), depth);
   add_mode(1024, 768, 0x83, TrivialBytesPerRow(1024, depth), depth);
   add_mode(1152, 870, 0x84, TrivialBytesPerRow(1152, depth), depth);
   add_mode(1280, 1024, 0x85, TrivialBytesPerRow(1280, depth), depth);
   add_mode(1600, 1200, 0x86, TrivialBytesPerRow(1600, depth), depth);
}

bool VideoInit(bool classic)
{
   the_buffer = (uint8 *)memalign_alloc_aligned(1600*1200*4);//1600 by 1200 32bit //7MB framebuffer
   the_buffer_copy = (uint8 *)memalign_alloc_aligned(1600*1200*4);//1600 by 1200 32bit //7MB framebuffer
   the_buffer_size = 1600*1200*4;
   
   if(classic)add_mode(512, 342, 0x80, 64, VDEPTH_1BIT);
   else {
      for (unsigned d=VDEPTH_1BIT; d<=VDEPTH_32BIT; d++) {
         add_window_modes(video_depth(d));
      }
   }
   return initlibretrovideo();
}


/*
 *  Deinitialization
 */

void VideoExit(void)
{
   if (the_buffer) {
      memalign_free(the_buffer);
      the_buffer = NULL;
   }
   if (the_buffer_copy) {
      memalign_free(the_buffer_copy);
      the_buffer_copy = NULL;
   }
   closelibretrovideo();
}


//unused,retroarch will handle fullscreen on its own
void VideoQuitFullScreen(void){}


/*
 *  Mac VBL interrupt
 */

void VideoInterrupt(void)
{
   libretrovideointerrupt();
}


// This function is called on non-threaded platforms from a timer interrupt
void VideoRefresh(void)
{
	// We need to check redraw_thread_active to inhibit refreshed during
	// mode changes on non-threaded platforms
	if (redraw_thread_active)refreshlibretrovideo();
}
