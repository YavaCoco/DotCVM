// TODO: separate window creation from this file
// TODO: make window creation for other plateforms

#include <log.h>
#include <app_main.h>
#include <devices.h>
#include <stdio.h>
#include <debug.h>
#include <window.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include <stdarg.h>

WindowData mainWin;

static bool s_CloseRequested = false;
static int s_ExitCode = 0;
static std::string* s_CloseMessage = new std::string("");

static uint* s_CurrentScreenBuffer = new uint[720 * 480];

void requestClose(int exitCode, const char* msg)
{
   s_CloseRequested = true;
   s_ExitCode       = exitCode;
   delete s_CloseMessage;
   s_CloseMessage   = new std::string(msg);
}

#ifdef DEBUG
static const char* s_WinTitle = "DotCVM_DEBUG";
#else
static const char* s_WinTitle = "DotCVM";
#endif

void init()
{
   for(uint x = 0; x < SCR_WIDTH; x++)
      for(uint y = 0; y < SCR_HEIGHT; y++)
         s_CurrentScreenBuffer[x + y * SCR_WIDTH] = 0;
}

void update()
{
   devices::update();
}

static bool hasChanged(uint* scrBuffer)
{
   for(uint x = 0; x < SCR_WIDTH; x++)
      for(uint y = 0; y < SCR_HEIGHT; y++)
            if(scrBuffer[x + y * SCR_WIDTH] != s_CurrentScreenBuffer[x + y * SCR_WIDTH])
            {
               debug("Pixel at: " << x << " " << y << " changed from " << s_CurrentScreenBuffer[x + y * SCR_WIDTH] << " to " << scrBuffer[x + y * SCR_WIDTH]);
               return true;
            }
   return false;
}

void refreshScr(WindowData window, uint* screenBuffer, bool force)
{
   if(hasChanged(screenBuffer) || force)
   {
      debug("Refreshing screen");
      for(uint x = 0; x < SCR_WIDTH; x++)
         for(uint y = 0; y < SCR_HEIGHT; y++)
         {
            XSetForeground(window.display, DefaultGC(window.display, window.screen), screenBuffer[x + y * SCR_WIDTH]);
            XDrawPoint(window.display, window.window, DefaultGC(window.display, window.screen), x, y);
            s_CurrentScreenBuffer[x + y * SCR_WIDTH] = screenBuffer[x + y * SCR_WIDTH];
         }
   }
}

void exit()
{
   log("Closing DotC Virtual Machine");

   // TODO: do this only if asked to
   log("Saving machine state");
   FILE* statFile = fopen("dotcvm_stat.dat", "w+");
   fwrite(&(devices::cpu->mem()[0]), 1, devices::cpu->mem().size(), statFile);
   fclose(statFile);
   log("Saved machine stat");
   devices::exit();
   log(*s_CloseMessage);
   delete s_CloseMessage;
   delete s_CurrentScreenBuffer;
   checkMemoryLeaks();
   log("Bye");
}

int main() {
   log("Starting DotC Virtual Machine");
   debug("Setting up fault handler");
   setupFaultHandler();
   debug("Creating window");
   mainWin = createWindow(s_WinTitle, 720, 480);
   Atom wmDeleteWindowAtom = XInternAtom(mainWin.display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(mainWin.display, mainWin.window, &wmDeleteWindowAtom, 1);
   init();
   devices::init(MEM_SIZE, DISK_SIZE);
   refreshScr(mainWin, devices::cpu->screen().buffer(), true);
   XEvent event;
   while (!s_CloseRequested) 
   {
      if(XEventWaiting(mainWin.display, KeyPress, event))
         debug("KeyPress: " << event.xkey.keycode);
      if(XEventWaiting(mainWin.display, KeyRelease, event))
         debug("KeyRelease: " << event.xkey.keycode);
      if(XEventWaiting(mainWin.display, ButtonPress, event))
         debug("ButtonPress: " << event.xbutton.button);
      if(XEventWaiting(mainWin.display, ButtonRelease, event))
         debug("ButtonRelease: " << event.xbutton.button);
      if(XEventWaiting(mainWin.display, MotionNotify, event))
         debug("Motion: " << event.xmotion.x << " " << event.xmotion.y);
      if(XEventWaiting(mainWin.display, Expose, event));
      if(XEventWaiting(mainWin.display, ClientMessage, event) && event.xclient.data.l[0] == wmDeleteWindowAtom)
         requestClose(0, "");
      update();
   }
   exit();
   XCloseDisplay(mainWin.display);
   return s_ExitCode;
}


