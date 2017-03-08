#include "sysdeps.h"

#include "user_strings.h"

struct B2_mutex {
   int wastespace;
};

B2_mutex* B2_create_mutex(void) {return NULL;}
void B2_lock_mutex(B2_mutex *mutex) {}
void B2_unlock_mutex(B2_mutex *mutex) {}
void B2_delete_mutex(B2_mutex *mutex) {}


/*
 *  Interrupt flags (must be handled atomically!)
 */

uint32 InterruptFlags = 0;

void SetInterruptFlag(uint32 flag)
{
   InterruptFlags |= flag;
}

void ClearInterruptFlag(uint32 flag)
{
   InterruptFlags &= ~flag;
}


/*
 *  Code was patched, flush caches if neccessary (i.e. when using a real 680x0
 *  or a dynamically recompiling emulator)
 */

void FlushCodeCache(void *start, uint32 size)
{

}


/*
 *  Display error alert
 */

void ErrorAlert(const char *text)
{
   printf(GetString(STR_SHELL_ERROR_PREFIX), text);
}


/*
 *  Display warning alert
 */

void WarningAlert(const char *text)
{
   printf(GetString(STR_SHELL_WARNING_PREFIX), text);
}


/*
 *  Display choice alert
 */

bool ChoiceAlert(const char *text, const char *pos, const char *neg)
{
   printf(GetString(STR_SHELL_WARNING_PREFIX), text);
   return false;	//!!
}
