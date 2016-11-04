#include <string.h>
#include <string>

#include "libretro.h"

//must be included first or other headers will throw errors
#include "sysdeps.h"

#include "sys.h"
#include "prefs.h"
#include "vm_alloc.h"
#include "cpu_emulation.h"
#include "main.h"

retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;

static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// CPU and FPU type, addressing mode
int CPUType;
bool CPUIs68060;
int FPUType;
bool TwentyFourBitAddressing;

void QuitEmulator(){
   // Exit 680x0 emulation
   Exit680x0();
   // Deinitialize everything
   ExitAll();
   // Exit VM wrappers
   vm_exit();
   // Exit system routines
   SysExit();
   // Exit preferences
   PrefsExit();
   exit(0);
}

void retro_init(void)
{
   unsigned colorMode = RETRO_PIXEL_FORMAT_XRGB8888;
   const char* vmdir;
   
   char* syspath;
   std::string path;
   bool worked = environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,(void *)&syspath);
   if(!worked)QuitEmulator();
   
   path = syspath;
   if(path[path.length() - 1] != '/')path += '/';
   path += "Basilisk2/";
   
   vmdir = path.c_str();
   
   int uselessargc = 0;
   char** uselessargv = NULL;
   
   // Read preferences
   PrefsInit(vmdir, uselessargc, uselessargv);
   // Init system routines
   SysInit();
   // Initialize VM system
   vm_init();
   // Initialize everything
   if (!InitAll(vmdir))QuitEmulator();
   
   
   environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &colorMode);
   
   // Start 68k and jump to ROM boot routine
   //Start680x0();//cant be used,runs forever (or must run on seprate thread)
}

void retro_deinit(void)
{
   // Exit 680x0 emulation
   Exit680x0();
   // Deinitialize everything
   ExitAll();
   // Exit VM wrappers
   vm_exit();
   // Exit system routines
   SysExit();
   // Exit preferences
   PrefsExit();
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Basilisk2";
   info->library_version  = "v1.0";
   info->need_fullpath    = false;
   info->valid_extensions = NULL; /* Anything is fine, we don't care. */
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps = 60.0;
   info->timing.sample_rate = 48000.0;

   info->geometry.base_width   = 512;
   info->geometry.base_height  = 384;
   info->geometry.max_width    = 1600;
   info->geometry.max_height   = 1200;
   info->geometry.aspect_ratio = 1;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   
}

void retro_run(void)
{
   //return newest frame
   //return generated audio samples
   //set pressed buttons
}

bool retro_load_game(const struct retro_game_info *info)
{
   return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   
}

bool retro_serialize(void *data_, size_t size)
{
   return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   
}

size_t retro_get_memory_size(unsigned id)
{
   
}

void retro_cheat_reset(void){}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

//callbacks from basilisk
bool initlibretrovideo(){
   
}

void closelibretrovideo(){
   
}

void libretrovideointerrupt(){
   
}

void refreshlibretrovideo(){
   
}

