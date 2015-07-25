// Game Engine 1.0
//
// (C) 1999 Watermelon Productions
// Portions (C) 1998 Andre LaMothe
 
// Includes

#define WIN32_LEAN_AND_MEAN  // as opposed to WIN32_FAT_AND_KIND

#include <windows.h>  
#include <windowsx.h> 
#include <mmsystem.h>
#include <iostream.h> 
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>

#include <ddraw.h>    // for directx
#include "game.h"  // the game lib

// Externals

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance

// Global vars/consts

FILE *fp_error                    = NULL; // general error file
LPDIRECTDRAW         lpdd         = NULL;  // dd object
LPDIRECTDRAWSURFACE  lpddsprimary = NULL;  // dd primary surface
LPDIRECTDRAWSURFACE  lpddsback    = NULL;  // dd back surface
LPDIRECTDRAWPALETTE  lpddpal      = NULL;  // a pointer to the created dd palette
LPDIRECTDRAWCLIPPER  lpddclipper  = NULL;  // dd clipper
PALETTEENTRY         palette[256];         // color palette
PALETTEENTRY         save_palette[256];    // used to save palettes
DDSURFACEDESC        ddsd;                 // a direct draw surface description struct
DDBLTFX              ddbltfx;              // used to fill
DDSCAPS              ddscaps;              // a direct draw surface capabilities struct
HRESULT              ddrval;               // result back from dd calls
UCHAR                *primary_buffer = NULL; // primary video buffer
UCHAR                *back_buffer    = NULL; // secondary back buffer
int                  primary_lpitch  = 0;    // memory line pitch
int                  back_lpitch     = 0;    // memory line pitch
BITMAP_FILE          bitmap16bit;            // a 16 bit bitmap file
BITMAP_FILE          bitmap8bit;             // a 8 bit bitmap file

DWORD                start_clock_count = 0; // used for timing

// these defined the general clipping rectangle
int min_clip_x = 0,                          // clipping rectangle 
    max_clip_x = SCREEN_WIDTH-1,
    min_clip_y = 0,
    max_clip_y = SCREEN_HEIGHT-1;

// these are overwritten globally by DD_Init()
int screen_width  = SCREEN_WIDTH,            // width of screen
    screen_height = SCREEN_HEIGHT,           // height of screen
    screen_bpp    = SCREEN_BPP;              // bits per pixel




// Class Functions


// STAT class

bool STAT::CreateSTAT(              
               int w, int h, // size of stat
               int frames,        // number of frames
               int a,              // attrs
               int mem_flags)         // memory flag
{

	DDSURFACEDESC ddsd; // used to create surface
	int index;          // looping var

	// set up animation vars
	curr_frame     = 0;
	num_frames     = frames;
	curr_animation = 0;
	anim_counter   = 0;
	anim_index     = 0;
	anim_count_max = 0; 
	attr = a;

	// set height and width
	width  = w;
	height = h;

	// set all images to null
	for (index=0; index<MAX_FRAMES; index++)
	    images[index] = NULL;

	// set all animations to null
	for (index=0; index<MAX_ANIMATIONS; index++)
	    animations[index] = NULL;

	// make sure width is a multiple of 8
	width_fill = ((w%8!=0) ? (8-w%8) : 0);

	// now create each surface
	for (index=0; index<num_frames; index++)
    {
	    // set to access caps, width, and height
	    memset(&ddsd,0,sizeof(ddsd));
	    ddsd.dwSize  = sizeof(ddsd);
	    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	
	    ddsd.dwWidth  = width + width_fill;
	    ddsd.dwHeight = height;

	    // set surface to offscreen plain
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

		// create the surfaces
		if (lpdd->CreateSurface(&ddsd,&images[index],NULL)!=DD_OK)
	        return(false);

	    // set color key to color 0
	    DDCOLORKEY color_key; // used to set color key
	    color_key.dwColorSpaceLowValue  = 0;
	    color_key.dwColorSpaceHighValue = 0;

	    // now set the color key for source blitting
	    images[index]->SetColorKey(DDCKEY_SRCBLT, &color_key);
    } 

	// return success
	return(true);
}



bool STAT::Draw(int x, int y, LPDIRECTDRAWSURFACE dest) 
{
	RECT dest_rect,   // the destination rectangle
	source_rect; // the source rectangle                             

	if (!(attr & ATTR_VISIBLE)) return(true);

	// fill in the destination rect
	dest_rect.left   = x;
	dest_rect.top    = y;
	dest_rect.right  = x+width-1;
	dest_rect.bottom = y+height-1;

	// fill in the source rect
	source_rect.left    = 0;
	source_rect.top     = 0;
	source_rect.right   = width-1;
	source_rect.bottom  = height-1;

	// blt to destination surface
	if (dest->Blt(&dest_rect, images[curr_frame],
		&source_rect,(DDBLT_WAIT | DDBLT_KEYSRC),
        NULL)!=DD_OK) return(false);

	// return success
	return(true);
} 


bool STAT::LoadFrame(BITMAP_FILE_PTR bitmap, int frame, int cx, int cy, int mode)
// Loads a bitmap into a STAT surface
{
	UCHAR *source_ptr,   // working pointers
		*dest_ptr;

	DDSURFACEDESC ddsd;  //  direct draw surface description 

	// test the mode of extraction, cell based or absolute
	if (mode==BITMAP_EXTRACT_MODE_CELL)
	{
		// re-compute x,y
		cx = cx*(width+1);
		cy = cy*(height+1);
	} 

	// extract bitmap data
	source_ptr = bitmap->buffer + cy*bitmap->bitmapinfoheader.biWidth+cx;

	// get the addr to destination surface memory

	// set size of the structure
	ddsd.dwSize = sizeof(ddsd);

	// lock the display surface
	images[frame]->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);

	// assign a pointer to the memory surface for manipulation
	dest_ptr = (UCHAR *)ddsd.lpSurface;

	// iterate thru each scanline and copy bitmap
	for (int index_y=0; index_y < height; index_y++)
    {
		// copy next line of data to destination
		memcpy(dest_ptr, source_ptr,width);

		// advance pointers
		dest_ptr   += (width+width_fill);
		source_ptr += bitmap->bitmapinfoheader.biWidth;
    }

	// unlock the surface 
	images[frame]->Unlock(ddsd.lpSurface);

	// set state to loaded
	attr |= ATTR_LOADED;

	// return success
	return(true);

} 



bool STAT::Animate()
// animates a STAT - does nothing for single frame STATs
{
	// first test if its time to animate
	if (++anim_counter >= anim_count_max)
	{
		// reset counter
		anim_counter = 0;
			// increment the animation frame index
		anim_index++;
     
		// extract the next frame from animation list 
		curr_frame = animations[curr_animation][anim_index];
    
		// is this and end sequence flag -1
		if (curr_frame == -1)
		{				
			// reset animation index
			anim_index = 0;
			// extract first animation frame
			curr_frame = animations[curr_animation][anim_index];	
		} 
	} 
	return(true);
} 



bool STAT::LoadAnimation(int anim, int frames, int *sequence)
// Loads an animation.  False if cannot allocate memory.
{
	// allocate memory for animation
	if (!(animations[anim] = (int *)malloc((frames+1)*sizeof(int))))
		return(false);

	// loads data
	for (int index=0; index<frames; index++)
	    animations[anim][index] = sequence[index];

	// set the end of the list to a -1 for later use
	animations[anim][index] = -1;

	// return success
	return(true);
}



bool STAT::SetAnimSpeed(int speed)
// simply changes the speed of the animation
{    
	// set speed
	anim_count_max = speed;
	
	return(true);
}



bool STAT::SetAnimation(int anim)
// Sets the current animation
{
	// set the animation index
	curr_animation = anim;

	// reset animation 
	anim_index = 0;
	
	return(true);
}



bool STAT::Hide()
{
	// reset the visibility bit
	RESET_BIT(attr, ATTR_VISIBLE);

	return(true);
}



bool STAT::Show()
// Make STAT visible
{
	// set the visibility bit
	SET_BIT(attr, ATTR_VISIBLE);
	
	return(true);
}


bool STAT::Visible()
// Is STAT visible?
{
	// it's visible
	if (attr & ATTR_VISIBLE) return(true);	

	// it's not
	return(false);
}




bool STAT::Destroy()
// destroys a STAT
{
	// destroy each bitmap surface
	for (int index=0; index < MAX_FRAMES; index++)
	    if (images[index]) images[index]->Release();

	// release memory for animation sequences 
	for (index=0; index<MAX_ANIMATIONS; index++)
	    if (animations[index]) free(animations[index]);

	return(true);
} 





//  MOVE Class

bool MOVE::CreateMOVE(int xx, int yy,int w, int h,int frames,
			int a,int mem_flags)
{	
	x = xx;
	y = yy;
	return CreateSTAT(w, h, frames, a, mem_flags);
}


bool MOVE::Draw(LPDIRECTDRAWSURFACE dest) 
{
	return STAT::Draw(x, y, dest);
}



// Friends of MOVE

bool TestCollide(MOVE& m1, MOVE& m2)
// True if the two MOVES have collided
{
	// they've collided
	if (m1.x+1 < m2.x + m2.width-1  &&  m1.x + m1.width-1 > m2.x+1 &&
		m1.y+1 < m2.y + m2.height-1 && m1.y + m1.height-1 > m2.y+1) return true;

	// they haven't
	return false;
}



//   FLOATER Class

FLOATER::FLOATER()
// constructor
{
	for (int i=0; i < NUMFLOATS; i++)
	{
		floats[i].active = false;
		floats[i].x = floats[i].y = floats[i].min = 0;
		floats[i].text = "";
	}
}


void FLOATER::Create(int x, int y, char *text)
{
	int curr=0; // pointer to float being used

	// find first empty floater to use
	while (curr < NUMFLOATS)
	{
		if (floats[curr].active==false) break;
		curr++;
	}

	// all the floats are in use, go back to first one!
	if (curr == NUMFLOATS-1 && floats[curr].active==true)
		curr = 0;

	// set up the floater
	floats[curr].x = x;
	floats[curr].y = y;
	floats[curr].min = y-25;
	floats[curr].text = text;
	floats[curr].active = true;
}


void FLOATER::Process(LPDIRECTDRAWSURFACE lpdds)
{
	for (int i=0; i < NUMFLOATS; i++)
		// if the current float is active
		if (floats[i].active)
		{
			// draw it
			Draw_Text_GDI(floats[i].text,floats[i].x,floats[i].y,RGB(255,255,0),lpdds);
			// move it up
			floats[i].y--;
			// check to see if it's still alive
			if (floats[i].y == floats[i].min) floats[i].active = false;
		}
	
}





// Graphics routines


int DD_Init(int width, int height, int bpp)
{
// this function initializes directdraw
int index; // looping variable
// create object and test for error
if (DirectDrawCreate(NULL,&lpdd,NULL)!=DD_OK)
   return(0);

// set cooperation level to windowed mode normal
if (lpdd->SetCooperativeLevel(main_window_handle,
           DDSCL_ALLOWMODEX | DDSCL_FULLSCREEN | 
           DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT)!=DD_OK)
    return(0);

// set the display mode
if (lpdd->SetDisplayMode(width,height,bpp)!=DD_OK)
   return(0);

// set globals
screen_height = height;
screen_width = width;
screen_bpp = bpp;

// Create the primary surface
memset(&ddsd,0,sizeof(ddsd));
ddsd.dwSize = sizeof(ddsd);
ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;

// we need to let dd know that we want a complex 
// flippable surface structure, set flags for that
ddsd.ddsCaps.dwCaps = 
  DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

// set the backbuffer count to 1
ddsd.dwBackBufferCount = 1;

// create the primary surface
lpdd->CreateSurface(&ddsd,&lpddsprimary,NULL);

// query for the backbuffer i.e the secondary surface
ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
lpddsprimary->GetAttachedSurface(&ddscaps,&lpddsback);

// create and attach palette

// create palette data
// clear all entries defensive programming
memset(palette,0,256*sizeof(PALETTEENTRY));

// create a R,G,B,GR gradient palette
for (index=0; index < 256; index++)
    {
    // set each entry
    if (index < 64) 
        palette[index].peRed = index*4; 
    else           // shades of green
    if (index >= 64 && index < 128) 
        palette[index].peGreen = (index-64)*4;
    else           // shades of blue
    if (index >= 128 && index < 192) 
       palette[index].peBlue = (index-128)*4;
    else           // shades of grey
    if (index >= 192 && index < 256) 
        palette[index].peRed = palette[index].peGreen = 
        palette[index].peBlue = (index-192)*4;
    
    // set flags
    palette[index].peFlags = PC_NOCOLLAPSE;
    } // end for index

// now create the palette object
if (lpdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,
                         palette,&lpddpal,NULL)!=DD_OK)
   return(0);

// attach the palette to the primary
if (lpddsprimary->SetPalette(lpddpal)!=DD_OK)
   return(0);

// clear out both primary and secondary surfaces
DD_Fill_Surface(lpddsprimary,0);
DD_Fill_Surface(lpddsback,0);

// return success
return(1);
} // end DD_Init

///////////////////////////////////////////////////////////

int DD_Shutdown(void)
{
// this function release all the resources directdraw
// allocated, mainly to com objects

// release the clipper first
if (lpddclipper)
    lpddclipper->Release();

// release the palette
if (lpddpal)
   lpddpal->Release();

// release the secondary surface
if (lpddsback)
    lpddsback->Release();

// release the primary surface
if (lpddsprimary)
   lpddsprimary->Release();

// finally, the main dd object
if (lpdd)
    lpdd->Release();

// return success
return(1);
} // end DD_Shutdown

///////////////////////////////////////////////////////////   

LPDIRECTDRAWCLIPPER DD_Attach_Clipper(LPDIRECTDRAWSURFACE lpdds,
                                      int num_rects,
                                      LPRECT clip_list)

{
// this function creates a clipper from the sent clip list and attaches
// it to the sent surface

int index;                         // looping var
LPDIRECTDRAWCLIPPER lpddclipper;   // pointer to the newly created dd clipper
LPRGNDATA region_data;             // pointer to the region data that contains
                                   // the header and clip list

// first create the direct draw clipper
if ((lpdd->CreateClipper(0,&lpddclipper,NULL))!=DD_OK)
   return(NULL);

// now create the clip list from the sent data

// first allocate memory for region data
region_data = (LPRGNDATA)malloc(sizeof(RGNDATAHEADER)+num_rects*sizeof(RECT));

// now copy the rects into region data
memcpy(region_data->Buffer, clip_list, sizeof(RECT)*num_rects);

// set up fields of header
region_data->rdh.dwSize          = sizeof(RGNDATAHEADER);
region_data->rdh.iType           = RDH_RECTANGLES;
region_data->rdh.nCount          = num_rects;
region_data->rdh.nRgnSize        = num_rects*sizeof(RECT);

region_data->rdh.rcBound.left    =  64000;
region_data->rdh.rcBound.top     =  64000;
region_data->rdh.rcBound.right   = -64000;
region_data->rdh.rcBound.bottom  = -64000;

// find bounds of all clipping regions
for (index=0; index<num_rects; index++)
    {
    // test if the next rectangle unioned with the current bound is larger
    if (clip_list[index].left < region_data->rdh.rcBound.left)
       region_data->rdh.rcBound.left = clip_list[index].left;

    if (clip_list[index].right > region_data->rdh.rcBound.right)
       region_data->rdh.rcBound.right = clip_list[index].right;

    if (clip_list[index].top < region_data->rdh.rcBound.top)
       region_data->rdh.rcBound.top = clip_list[index].top;

    if (clip_list[index].bottom > region_data->rdh.rcBound.bottom)
       region_data->rdh.rcBound.bottom = clip_list[index].bottom;

    } // end for index

// now we have computed the bounding rectangle region and set up the data
// now let's set the clipping list

if ((lpddclipper->SetClipList(region_data, 0))!=DD_OK)
   {
   // release memory and return error
   free(region_data);
   return(NULL);
   } // end if

// now attach the clipper to the surface
if ((lpdds->SetClipper(lpddclipper))!=DD_OK)
   {
   // release memory and return error
   free(region_data);
   return(NULL);
   } // end if

// all is well, so release memory and send back the pointer to the new clipper
free(region_data);
return(lpddclipper);

} // end DD_Attach_Clipper

///////////////////////////////////////////////////////////   
   
LPDIRECTDRAWSURFACE DD_Create_Surface(int width, int height, int mem_flags)
{
// this function creates an offscreen plain surface

DDSURFACEDESC ddsd;         // working description
LPDIRECTDRAWSURFACE lpdds;  // temporary surface
    
// set to access caps, width, and height
memset(&ddsd,0,sizeof(ddsd));
ddsd.dwSize  = sizeof(ddsd);
ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

// set dimensions of the new bitmap surface
ddsd.dwWidth  =  width;
ddsd.dwHeight =  height;

// set surface to offscreen plain
ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | mem_flags;

// create the surface
if (lpdd->CreateSurface(&ddsd,&lpdds,NULL)!=DD_OK)
   return(NULL);

// set color key to color 0
DDCOLORKEY color_key; // used to set color key
color_key.dwColorSpaceLowValue  = 0;
color_key.dwColorSpaceHighValue = 0;

// now set the color key for source blitting
lpdds->SetColorKey(DDCKEY_SRCBLT, &color_key);

// return surface
return(lpdds);
} // end DD_Create_Surface

///////////////////////////////////////////////////////////   
   
int DD_Flip(void)
{
// this function flip the primary surface with the secondary surface

// test if either of the buffers are locked
if (primary_buffer || back_buffer)
   return(0);

// flip pages
while(lpddsprimary->Flip(NULL, DDFLIP_WAIT)!=DD_OK);

// flip the surface

// return success
return(1);

} // end DD_Flip

///////////////////////////////////////////////////////////   
   
int DD_Wait_For_Vsync(void)
{
// this function waits for a vertical blank to begin
    
lpdd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN,0);

// return success
return(1);
} // end DD_Wait_For_Vsync

///////////////////////////////////////////////////////////   
   
int DD_Fill_Surface(LPDIRECTDRAWSURFACE lpdds,int color)
{
DDBLTFX ddbltfx; // this contains the DDBLTFX structure

// clear out the structure and set the size field 
DD_INIT_STRUCT(ddbltfx);

// set the dwfillcolor field to the desired color
ddbltfx.dwFillColor = color; 

// ready to blt to surface
lpdds->Blt(NULL,       // ptr to dest rectangle
           NULL,       // ptr to source surface, NA            
           NULL,       // ptr to source rectangle, NA
           DDBLT_COLORFILL | DDBLT_WAIT,   // fill and wait                   
           &ddbltfx);  // ptr to DDBLTFX structure

// return success
return(1);
} // end DD_Fill_Surface

///////////////////////////////////////////////////////////   
   
UCHAR *DD_Lock_Surface(LPDIRECTDRAWSURFACE lpdds,int *lpitch)
{
// this function locks the sent surface and returns a pointer to it

// is this surface valid
if (!lpdds)
   return(NULL);

// lock the surface
DD_INIT_STRUCT(ddsd);
lpdds->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

// set the memory pitch
if (lpitch)
   *lpitch = ddsd.lPitch;

// return pointer to surface
return((UCHAR *)ddsd.lpSurface);

} // end DD_Lock_Surface

///////////////////////////////////////////////////////////   
   
int DD_Unlock_Surface(LPDIRECTDRAWSURFACE lpdds, UCHAR *surface_buffer)
{
// this unlocks a general surface

// is this surface valid
if (!lpdds)
   return(0);

// unlock the surface memory
lpdds->Unlock(surface_buffer);

// return success
return(1);
} // end DD_Unlock_Surface

///////////////////////////////////////////////////////////

UCHAR *DD_Lock_Primary_Surface(void)
{
// this function locks the priamary surface and returns a pointer to it
// and updates the global variables primary_buffer, and primary_lpitch

// is this surface already locked
if (primary_buffer)
   {
   // return to current lock
   return(primary_buffer);
   } // end if

// lock the primary surface
DD_INIT_STRUCT(ddsd);
lpddsprimary->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

// set globals
primary_buffer = (UCHAR *)ddsd.lpSurface;
primary_lpitch = ddsd.lPitch;

// return pointer to surface
return(primary_buffer);

} // end DD_Lock_Primary_Surface

///////////////////////////////////////////////////////////   
   
int DD_Unlock_Primary_Surface(void)
{
// this unlocks the primary

// is this surface valid
if (!primary_buffer)
   return(0);

// unlock the primary surface
lpddsprimary->Unlock(primary_buffer);

// reset the primary surface
primary_buffer = NULL;
primary_lpitch = 0;

// return success
return(1);
} // end DD_Unlock_Primary_Surface

//////////////////////////////////////////////////////////

UCHAR *DD_Lock_Back_Surface(void)
{
// this function locks the secondary back surface and returns a pointer to it
// and updates the global variables secondary buffer, and back_lpitch

// is this surface already locked
if (back_buffer)
   {
   // return to current lock
   return(back_buffer);
   } // end if

// lock the primary surface
DD_INIT_STRUCT(ddsd);
lpddsback->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL); 

// set globals
back_buffer = (UCHAR *)ddsd.lpSurface;
back_lpitch = ddsd.lPitch;

// return pointer to surface
return(back_buffer);

} // end DD_Lock_Back_Surface

///////////////////////////////////////////////////////////   
   
int DD_Unlock_Back_Surface(void)
{
// this unlocks the secondary

// is this surface valid
if (!back_buffer)
   return(0);

// unlock the secondary surface
lpddsback->Unlock(back_buffer);

// reset the secondary surface
back_buffer = NULL;
back_lpitch = 0;

// return success
return(1);
} // end DD_Unlock_Back_Surface

///////////////////////////////////////////////////////////

DWORD Get_Clock(void)
{
// this function returns the current tick count

// return time
return(GetTickCount());

} // end Get_Clock

///////////////////////////////////////////////////////////

DWORD Start_Clock(void)
{
// this function starts the clock, that is, saves the current
// count, use in conjunction with Wait_Clock()

return(start_clock_count = Get_Clock());

} // end Start_Clock

////////////////////////////////////////////////////////////

DWORD Wait_Clock(DWORD count)
{
// this function is used to wait for a specific number of clicks
// since the call to Start_Clock

while((Get_Clock() - start_clock_count) < count);
return(Get_Clock());

} // end Wait_Clock

///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
   
int Set_Palette_Entry(int color_index, LPPALETTEENTRY color)
{
// this function sets a palette color in the palette
lpddpal->SetEntries(0,color_index,1,color);

// set data in shadow palette
memcpy(&palette[color_index],color,sizeof(PALETTEENTRY));

// return success
return(1);
} // end Set_Palette_Entry

///////////////////////////////////////////////////////////   
   
int Get_Palette_Entry(int color_index,LPPALETTEENTRY color)
{
// this function retrieves a palette entry from the color
// palette

// copy data out from shadow palette
memcpy(color, &palette[color_index],sizeof(PALETTEENTRY));

// return success
return(1);
} // end Get_Palette_Entry

///////////////////////////////////////////////////////////
   
int Load_Palette_From_File(char *filename, LPPALETTEENTRY palette)
{
// this function loads a palette from disk into a palette
// structure, but does not set the pallette

FILE *fp_file; // working file

// try and open file
if ((fp_file = fopen(filename,"r"))==NULL)
   return(0);

// read in all 256 colors RGBF
for (int index=0; index<256; index++)
    {
    // read the next entry in
    fscanf(fp_file,"%d %d %d %d",&palette[index].peRed,
                                 &palette[index].peGreen,
                                 &palette[index].peBlue,                                
                                 &palette[index].peFlags);
    } // end for index

// close the file
fclose(fp_file);

// return success
return(1);
} // end Load_Palette_From_Disk

///////////////////////////////////////////////////////////   
   
int Save_Palette_To_File(char *filename, LPPALETTEENTRY palette)
{
// this function saves a palette to disk

FILE *fp_file; // working file

// try and open file
if ((fp_file = fopen(filename,"w"))==NULL)
   return(0);

// write in all 256 colors RGBF
for (int index=0; index<256; index++)
    {
    // read the next entry in
    fprintf(fp_file,"\n%d %d %d %d",palette[index].peRed,
                                    palette[index].peGreen,
                                    palette[index].peBlue,                                
                                    palette[index].peFlags);
    } // end for index

// close the file
fclose(fp_file);

// return success
return(1);

} // end Save_Palette_To_Disk

///////////////////////////////////////////////////////////

int Save_Palette(LPPALETTEENTRY sav_palette)
{
// this function saves the current palette 

memcpy(sav_palette, palette,256*sizeof(PALETTEENTRY));

// return success
return(1);
} // end Save_Palette

///////////////////////////////////////////////////////////   
   
int Set_Palette(LPPALETTEENTRY set_palette)
{
// this function writes the sent palette

// first save the new palette in shadow
memcpy(palette, set_palette,256*sizeof(PALETTEENTRY));

// now set the new palette
lpddpal->SetEntries(0,0,256,palette);

// return success
return(1);
} // end Set_Palette

///////////////////////////////////////////////////////////

int Rotate_Colors(int start_index, int colors)
{
// this function rotates the color between start and end

PALETTEENTRY work_pal[256]; // working palette

// get the color palette
lpddpal->GetEntries(0,start_index,colors,work_pal);

// shift the colors
lpddpal->SetEntries(0,start_index+1,colors-1,work_pal);

// fix up the last color
lpddpal->SetEntries(0,start_index,1,&work_pal[colors - 1]);

// update shadow palette
lpddpal->GetEntries(0,0,256,palette);

// return sucess
return(1);

} // end Rotate_Colors

///////////////////////////////////////////////////////////   
   
int Draw_Text_GDI(char *text, int x,int y,COLORREF color, LPDIRECTDRAWSURFACE lpdds)
{
// this function draws the sent text on the sent surface 
// using color index as the color in the palette

HDC xdc; // the working dc

// get the dc from surface
if (lpdds->GetDC(&xdc)!=DD_OK)
   return(0);

// set the colors for the text up
SetTextColor(xdc,color);

// set background mode to transparent so black isn't copied
SetBkMode(xdc, TRANSPARENT);

// draw the text a
TextOut(xdc,x,y,text,strlen(text));

// release the dc
lpdds->ReleaseDC(xdc);

// return success
return(1);
} // end Draw_Text_GDI

///////////////////////////////////////////////////////////

int Draw_Text_GDI(char *text, int x,int y,int color, LPDIRECTDRAWSURFACE lpdds)
{
// this function draws the sent text on the sent surface 
// using color index as the color in the palette

HDC xdc; // the working dc

// get the dc from surface
if (lpdds->GetDC(&xdc)!=DD_OK)
   return(0);

// set the colors for the text up
SetTextColor(xdc,RGB(palette[color].peRed,palette[color].peGreen,palette[color].peBlue) );

// set background mode to transparent so black isn't copied
SetBkMode(xdc, TRANSPARENT);

// draw the text a
TextOut(xdc,x,y,text,strlen(text));

// release the dc
lpdds->ReleaseDC(xdc);

// return success
return(1);
} // end Draw_Text_GDI

///////////////////////////////////////////////////////////

int Open_Error_File(char *filename)
{
// this function opens the output error file

FILE *fp_temp; // temporary file

// test if this file is valid
if ((fp_temp = fopen(filename,"w"))==NULL)
   return(0);

// close old file if there was one
if (fp_error)
   Close_Error_File();

// assign new file

fp_error = fp_temp;

// write out header
Write_Error("\nOpening Error Output File (%s):\n",filename);

// return success
return(1);

} // end Open_Error_File

///////////////////////////////////////////////////////////

int Close_Error_File(void)
{
// this function closes the error file

if (fp_error)
    {
    // write close file string
    Write_Error("\nClosing Error Output File.");

    // close the file handle
    fclose(fp_error);
    fp_error = NULL;

    // return success
    return(1);
    } // end if
else
   return(0);

} // end Close_Error_File

///////////////////////////////////////////////////////////

int Write_Error(char *string, ...)
{
// this function prints out the error string to the error file

char buffer[80]; // working buffer

va_list arglist; // variable argument list

// make sure both the error file and string are valid
if (!string || !fp_error)
   return(0);

// print out the string using the variable number of arguments on stack
va_start(arglist,string);
vsprintf(buffer,string,arglist);
va_end(arglist);

// write string to file
fprintf(fp_error,buffer);

// return success
return(1);
} // end Write_Error

///////////////////////////////////////////////////////////////////////////////

int Create_Bitmap(BITMAP_IMAGE_PTR image, int x, int y, int width, int height)
{
// this function is used to intialize a bitmap

// allocate the memory
if (!(image->buffer = (UCHAR *)malloc(width*height)))
   return(0);

// initialize variables
image->state     = BITMAP_STATE_ALIVE;
image->attr      = 0;
image->width     = width;
image->height    = height;
image->x         = x;
image->y         = y;
image->num_bytes = width*height;

// clear memory out
memset(image->buffer,0,width*height);

// return success
return(1);

} // end Create_Bitmap

///////////////////////////////////////////////////////////////////////////////

int Destroy_Bitmap(BITMAP_IMAGE_PTR image)
{
// this function releases the memory associated with a bitmap

if (image && image->buffer)
   {
   // free memory and reset vars
   free(image->buffer);

   // set all vars in structure to 0
   memset(image,0,sizeof(BITMAP_IMAGE));

   // return success
   return(1);

   } // end if
else  // invalid entry pointer of the object wasn't initialized
   return(0);

} // end Destroy_Bitmap

///////////////////////////////////////////////////////////

int Draw_Bitmap(BITMAP_IMAGE_PTR source_bitmap,UCHAR *dest_buffer, int lpitch, int transparent)
{
// this function draws the bitmap onto the destination memory surface
// if transparent is 1 then color 0 will be transparent
// note this function does NOT clip, so be carefull!!!

UCHAR *dest_addr,   // starting address of bitmap in destination
      *source_addr; // starting adddress of bitmap data in source

UCHAR pixel;        // used to hold pixel value

int index,          // looping vars
    pixel_x;

// test if this bitmap is loaded

if (source_bitmap->attr & BITMAP_ATTR_LOADED)
   {
   // compute starting destination address
   dest_addr = dest_buffer + source_bitmap->y*lpitch + source_bitmap->x;

   // compute the starting source address
   source_addr = source_bitmap->buffer;

   // is this bitmap transparent
   if (transparent)
   {
   // copy each line of bitmap into destination with transparency
   for (index=0; index<source_bitmap->height; index++)
       {
       // copy the memory
       for (pixel_x=0; pixel_x<source_bitmap->width; pixel_x++)
           {
           if ((pixel = source_addr[pixel_x])!=0)
               dest_addr[pixel_x] = pixel;

           } // end if

       // advance all the pointers
       dest_addr   += lpitch;
       source_addr += source_bitmap->width;

       } // end for index
   } // end if
   else
   {
   // non-transparent version
   // copy each line of bitmap into destination

   for (index=0; index<source_bitmap->height; index++)
       {
       // copy the memory
       memcpy(dest_addr, source_addr, source_bitmap->width);

       // advance all the pointers
       dest_addr   += lpitch;
       source_addr += source_bitmap->width;

       } // end for index

   } // end else

   // return success
   return(1);

   } // end if
else
   return(0);

} // end Draw_Bitmap

///////////////////////////////////////////////////////////////////////////////

int Load_Image_Bitmap(BITMAP_IMAGE_PTR image, // bitmap image to load with data
                      BITMAP_FILE_PTR bitmap,    // bitmap to scan image data from
                      int cx,int cy,   // cell or absolute pos. to scan image from
                      int mode)        // if 0 then cx,cy is cell position, else 
                                    // cx,cy are absolute coords
{
// this function extracts a bitmap out of a bitmap file

UCHAR *source_ptr,   // working pointers
      *dest_ptr;

// is this a valid bitmap
if (!image)
   return(0);

// test the mode of extraction, cell based or absolute
if (mode==BITMAP_EXTRACT_MODE_CELL)
   {
   // re-compute x,y
   cx = cx*(image->width+1) + 1;
   cy = cy*(image->height+1) + 1;
   } // end if

// extract bitmap data
source_ptr = bitmap->buffer +
      cy*bitmap->bitmapinfoheader.biWidth+cx;

// assign a pointer to the bimap image
dest_ptr = (UCHAR *)image->buffer;

// iterate thru each scanline and copy bitmap
for (int index_y=0; index_y<image->height; index_y++)
    {
    // copy next line of data to destination
    memcpy(dest_ptr, source_ptr,image->width);

    // advance pointers
    dest_ptr   += image->width;
    source_ptr += bitmap->bitmapinfoheader.biWidth;
    } // end for index_y

// set state to loaded
image->attr |= BITMAP_ATTR_LOADED;

// return success
return(1);

} // end Load_Image_Bitmap

///////////////////////////////////////////////////////////

int Scroll_Bitmap(void)
{
// this function scrolls a bitmap
// not implemented

// return success
return(1);
} // end Scroll_Bitmap
///////////////////////////////////////////////////////////

int Copy_Bitmap(void)
{
// this function copies a bitmap from one source to another
// not implemented


// return success
return(1);
} // end Copy_Bitmap

///////////////////////////////////////////////////////////

int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename)
{
// this function opens a bitmap file and loads the data into bitmap

int file_handle,  // the file handle
    index;        // looping index

UCHAR *temp_buffer = NULL; // used to convert 24 bit images to 16 bit
OFSTRUCT file_data;        // the file data information

// open the file if it exists
if ((file_handle = OpenFile(filename,&file_data,OF_READ))==-1)
   return(0);

// now load the bitmap file header
_lread(file_handle, &bitmap->bitmapfileheader,sizeof(BITMAPFILEHEADER));

// test if this is a bitmap file
if (bitmap->bitmapfileheader.bfType!=BITMAP_ID)
   {
   // close the file
   _lclose(file_handle);

   // return error
   return(0);
   } // end if

// now we know this is a bitmap, so read in all the sections

// first the bitmap infoheader

// now load the bitmap file header
_lread(file_handle, &bitmap->bitmapinfoheader,sizeof(BITMAPINFOHEADER));

// now load the color palette if there is one
if (bitmap->bitmapinfoheader.biBitCount == 8)
   {
   _lread(file_handle, &bitmap->palette,256*sizeof(PALETTEENTRY));

   // now set all the flags in the palette correctly and fix the reversed 
   // BGR RGBQUAD data format
   for (index=0; index < 256; index++)
       {
       // reverse the red and green fields
       int temp_color = bitmap->palette[index].peRed;
       bitmap->palette[index].peRed  = bitmap->palette[index].peBlue;
       bitmap->palette[index].peBlue = temp_color;
       
       // always set the flags word to this
       bitmap->palette[index].peFlags = PC_NOCOLLAPSE;
       } // end for index

    } // end if

// finally the image data itself
_lseek(file_handle,-(int)(bitmap->bitmapinfoheader.biSizeImage),SEEK_END);

// now read in the image, if the image is 8 or 16 bit then simply read it
// but if its 24 bit then read it into a temporary area and then convert
// it to a 16 bit image

if (bitmap->bitmapinfoheader.biBitCount==8 || bitmap->bitmapinfoheader.biBitCount==16)
   {
   // delete the last image if there was one
   if (bitmap->buffer)
       free(bitmap->buffer);

   // allocate the memory for the image
   if (!(bitmap->buffer = (UCHAR *)malloc(bitmap->bitmapinfoheader.biSizeImage)))
      {
      // close the file
      _lclose(file_handle);

      // return error
      return(0);
      } // end if

   // now read it in
   _lread(file_handle,bitmap->buffer,bitmap->bitmapinfoheader.biSizeImage);

   } // end if
else
   {
   // this must be a 24 bit image, load it in and convert it to 16 bit
//   printf("\nconverting 24 bit image...");

   // allocate temporary buffer
   if (!(temp_buffer = (UCHAR *)malloc(bitmap->bitmapinfoheader.biSizeImage)))
      {
      // close the file
      _lclose(file_handle);

      // return error
      return(0);
      } // end if
   
   // allocate final 16 bit storage buffer
   if (!(bitmap->buffer=(UCHAR *)malloc(2*bitmap->bitmapinfoheader.biWidth*bitmap->bitmapinfoheader.biHeight)))
      {
      // close the file
      _lclose(file_handle);

      // release working buffer
      free(temp_buffer);

      // return error
      return(0);
      } // end if

   // now read it in
   _lread(file_handle,temp_buffer,bitmap->bitmapinfoheader.biSizeImage);

   // now convert each 24 bit RGB value into a 16 bit value
   for (index=0; index<bitmap->bitmapinfoheader.biWidth*bitmap->bitmapinfoheader.biHeight; index++)
       {
       // extract RGB components (in BGR order), note the scaling
       UCHAR blue  = (temp_buffer[index*3 + 0] >> 3),
             green = (temp_buffer[index*3 + 1] >> 3),
             red   = (temp_buffer[index*3 + 2] >> 3);

       // build up 16 bit color word
       USHORT color = _RGB16BIT(red,green,blue);

       // write color to buffer
       ((USHORT *)bitmap->buffer)[index] = color;

       } // end for index

   // finally write out the correct number of bits
   bitmap->bitmapinfoheader.biBitCount=16;

   } // end if

#if 0
// write the file info out 
printf("\nfilename:%s \nsize=%d \nwidth=%d \nheight=%d \nbitsperpixel=%d \ncolors=%d \nimpcolors=%d",
        filename,
        bitmap->bitmapinfoheader.biSizeImage,
        bitmap->bitmapinfoheader.biWidth,
        bitmap->bitmapinfoheader.biHeight,
		bitmap->bitmapinfoheader.biBitCount,
        bitmap->bitmapinfoheader.biClrUsed,
        bitmap->bitmapinfoheader.biClrImportant);
#endif

// close the file
_lclose(file_handle);

// flip the bitmap
Flip_Bitmap(bitmap->buffer, 
            bitmap->bitmapinfoheader.biWidth*(bitmap->bitmapinfoheader.biBitCount/8), 
            bitmap->bitmapinfoheader.biHeight);

// return success
return(1);

} // end Load_Bitmap_File

///////////////////////////////////////////////////////////

int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap)
{
// this function releases all memory associated with "bitmap"
if (bitmap->buffer)
   {
   // release memory
   free(bitmap->buffer);

   // reset pointer
   bitmap->buffer = NULL;

   } // end if

// return success
return(1);

} // end Unload_Bitmap_File

///////////////////////////////////////////////////////////

int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height)
{
// this function is used to flip upside down .BMP images

UCHAR *buffer; // used to perform the image processing
int index;     // looping index

// allocate the temporary buffer
if (!(buffer = (UCHAR *)malloc(bytes_per_line*height)))
   return(0);

// copy image to work area
memcpy(buffer,image,bytes_per_line*height);

// flip vertically
for (index=0; index < height; index++)
    memcpy(&image[((height-1) - index)*bytes_per_line],
           &buffer[index*bytes_per_line], bytes_per_line);

// release the memory
free(buffer);

// return success
return(1);

} // end Flip_Bitmap

///////////////////////////////////////////////////////////