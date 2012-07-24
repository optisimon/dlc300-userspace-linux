/**
 * Test talking to some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#ifndef GUIHELPERS_H_
#define GUIHELPERS_H_

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_gfxPrimitives_font.h>


class SDLWindow {
   void Slock(SDL_Surface *screen)
   {
     if ( SDL_MUSTLOCK(screen) )
     {
       if ( SDL_LockSurface(screen) < 0 )
       {
         return;
       }
     }
   }

   void Sulock(SDL_Surface *screen)
   {
     if ( SDL_MUSTLOCK(screen) )
     {
       SDL_UnlockSurface(screen);
     }
   }

   void clear(SDL_Surface *screen)
   {
      //Slock(screen);
      memset(screen->pixels, 0, screen->pitch*h_);
      //Sulock(screen);
   }

   void safeDrawPixel(SDL_Surface *screen, int x, int y,
                                         Uint8 R, Uint8 G, Uint8 B)
   {
	   if( x >= 0 && x < w_ && y >= 0 && y < h_)
	   {
		   DrawPixel(screen, x, y, R, G, B);
	   }
   }

   void DrawPixel(SDL_Surface *screen, int x, int y,
                                       Uint8 R, Uint8 G, Uint8 B)
   {
     Uint32 color = SDL_MapRGB(screen->format, R, G, B);
     switch (screen->format->BytesPerPixel)
     {
       case 1: // Assuming 8-bpp
         {
           Uint8 *bufp;
           bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
           *bufp = color;
         }
         break;
       case 2: // Probably 15-bpp or 16-bpp
         {
           Uint16 *bufp;
           bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
           *bufp = color;
         }
         break;
       case 3: // Slow 24-bpp mode, usually not used
         {
           Uint8 *bufp;
           bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
           if(SDL_BYTEORDER == SDL_LIL_ENDIAN)
           {
             bufp[0] = color;
             bufp[1] = color >> 8;
             bufp[2] = color >> 16;
           } else {
             bufp[2] = color;
             bufp[1] = color >> 8;
             bufp[0] = color >> 16;
           }
         }
         break;
       case 4: // Probably 32-bpp
         {
           Uint32 *bufp;
           bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
           *bufp = color;
         }
         break;
     }
   }

   bool should_call_sdl_quit_;
   const int w_;
   const int h_;
   SDL_Surface *screen_;
public:
   SDLWindow() : should_call_sdl_quit_(true), w_(1024), h_(768+10), screen_(0)
   {
      if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
      {
         printf("Unable to init SDL: %s\n", SDL_GetError());
         exit(1);
      }

      screen_=SDL_SetVideoMode(w_, h_, 32, SDL_HWSURFACE/*|SDL_DOUBLEBUF*/);
      if ( screen_ == NULL )
      {
         printf("Unable to set %dx%d video: %s\n", w_, h_, SDL_GetError());
         exit(1);
      }
      clear(screen_);
      SDL_Flip(screen_);
      clear(screen_);
   }

   ~SDLWindow()
   {
      if (should_call_sdl_quit_)
         SDL_Quit();
   }

   /**
    * This version does not do any kind of demosaiking, it just takes the 4x4 bayer patch,
    * and converts it into a single pixel without any interpolation
    * */
   void drawBayerAsRGB(unsigned char* img, int w, int h)
   {
	   Slock(screen_);
	   //clear();

	   assert(w/2 <= w_);
	   assert(h/2 <= h_);

	   for (int y = 0; y < h/2; y++)
	   {
		   for( int x = 0; x < w/2; x++)
		   {
			   uint8_t R  = img[(y*2  )*w + x*2 + 0];
			   uint8_t G1 = img[(y*2  )*w + x*2 + 1];
			   uint8_t G2 = img[(y*2+1)*w + x*2 + 0];
			   uint8_t B =  img[(y*2+1)*w + x*2 + 1];
			   safeDrawPixel(screen_, x, y+10, R, (G1 + G2)/2, B);
		   }
	   }

	   stringRGBA(screen_, 0, 0, "ESC = quit, F1 = take 1 snapshot, F2 = toggle taking snapshots continuously", 255, 255, 255, 255);

	   Sulock(screen_);
	   SDL_Flip(screen_);
   }
};

class SDLEventHandler {
	bool should_quit_;
	bool should_take_snapshot_;
	bool take_snapshots_continuous_;
public:
	SDLEventHandler() : should_quit_(false), should_take_snapshot_(false), take_snapshots_continuous_(false)
	{
		refresh();
	}

	void refresh()
	{
		SDL_Event event;

		while ( SDL_PollEvent(&event) )
		{
			if ( event.type == SDL_QUIT )
			{
				should_quit_ = true;
			}

			if ( event.type == SDL_KEYDOWN )
			{
				switch(event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					should_quit_ = true;
					break;
				case SDLK_F1:
					should_take_snapshot_ = true;
					break;
				case SDLK_F2:
					take_snapshots_continuous_ = ! take_snapshots_continuous_;
					break;
				case SDLK_PLUS:
					break;
				case SDLK_MINUS:
					break;
				case SDLK_SPACE:
					break;
				default:
					break;
				}
			}
		}
	}

	bool shouldQuit()
	{
		return should_quit_;
	}

	bool shouldTakeSnapshot()
	{
		bool tmp = should_take_snapshot_;
		should_take_snapshot_ = false;
		return tmp || take_snapshots_continuous_;
	}
};

#endif /* GUIHELPERS_H_ */
