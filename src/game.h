// Game Engine 1.0
//
// (C) 1999 Watermelon Productions
// Portions (C) 1998 Andre LaMothe

#ifndef GAME
#define GAME

// DEFINES ////////////////////////////////////////////////

// default screen size
#define SCREEN_WIDTH    640  // size of screen
#define SCREEN_HEIGHT   480
#define SCREEN_BPP      8    // bits per pixel

// bitmap defines
#define BITMAP_ID            0x4D42 // universal id for a bitmap
#define BITMAP_STATE_DEAD    0
#define BITMAP_STATE_ALIVE   1
#define BITMAP_STATE_DYING   2 
#define BITMAP_ATTR_LOADED   128

#define BITMAP_EXTRACT_MODE_CELL  0
#define BITMAP_EXTRACT_MODE_ABS   1

// defines for STAT
#define STATE_ANIM_DONE    1    // done animation state
#define MAX_FRAMES         64   // maximum number of frames
#define MAX_ANIMATIONS     16   // maximum number of animation sequeces
#define ATTR_VISIBLE        1   // visible bit
#define ATTR_LOADED         2   // loaded bit

// defines for FLOATER
#define NUMFLOATS      10       // max number of floaters

// MACROS /////////////////////////////////////////////////

// these read the keyboard asynchronously
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code)   ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// this builds a 16 bit color value
#define _RGB16BIT(r,g,b) ((b%32) + ((g%32) << 5) + ((r%32) << 10))

// bit manipulation macros
#define SET_BIT(word,bit_flag)   ((word)=((word) | (bit_flag)))
#define RESET_BIT(word,bit_flag) ((word)=((word) & (~bit_flag)))

// initializes a direct draw struct
#define DD_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

// TYPES //////////////////////////////////////////////////

// basic unsigned types
typedef unsigned short USHORT;
typedef unsigned short WORD;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;

// container structure for bitmaps .BMP file
typedef struct BITMAP_FILE_TAG
        {
        BITMAPFILEHEADER bitmapfileheader;  // this contains the bitmapfile header
        BITMAPINFOHEADER bitmapinfoheader;  // this is all the info including the palette
        PALETTEENTRY     palette[256];      // we will store the palette here
        UCHAR            *buffer;           // this is a pointer to the data

        } BITMAP_FILE, *BITMAP_FILE_PTR;


/*
 Stationary class //////////////////////////////
 Used for sprites that sit still, but can be animated
*/

class STAT  // base class
{
public:
	int attr;           // object attributes 
	//int varsI[16];      // stack of 16 integers
	//float varsF[16];    // stack of 16 floats
	int curr_frame;     // current animation frame
	int num_frames;     // total number of animation frames
	int curr_animation; // index of current animation
	int anim_counter;   // used to time animation transitions
	int anim_index;     // animation element index
	int anim_count_max; // number of cycles before animation
	int *animations[MAX_ANIMATIONS]; // animation sequences
	int width, height;  // width and height of sprite
    int width_fill;     // used for graphics functions
	LPDIRECTDRAWSURFACE images[MAX_FRAMES]; // the bitmap images DD surfaces

	// A sort of constructor 
	bool CreateSTAT(int w, int h, int frames, int a, int mem_flags);

	// Draws STAT to the screen
	bool Draw(int x, int y, LPDIRECTDRAWSURFACE dest);

	// Loads bitmaps to the STAT's surfaces
	bool LoadFrame(BITMAP_FILE_PTR bitmap, int frame, int cx, int cy, int mode);

	// Advances animation
	bool Animate();

	// Loads an animation
	bool LoadAnimation(int anim, int num, int *sequence);

	// Sets the speed of the animation
	bool SetAnimSpeed(int speed);

	// Changes the current animation
	bool SetAnimation(int anim_index);

	// Determine if STAT is visible
	bool Visible();

	// Make STAT appear or disapear on the screen
	bool Hide();
	bool Show();
	
	// A sort of deconstructor
	bool Destroy();

};




/*
 Moving class //////////////////////////////
 Used for sprites that move
*/

class MOVE : public STAT   // inherits the STAT class
{
	public:
        int state;          // the state of the object (general)        
        int x,y;            // position bitmap will be displayed at        


		// A sort of constructor 
		bool CreateMOVE(int xx, int yy, int width, int h ,int frames, int a,int mem_flags);              

		// Drawing function
		bool Draw(LPDIRECTDRAWSURFACE dest);

		// Test if two MOVES overlap
		friend bool TestCollide(MOVE & m1, MOVE & m2);
 
};



/*
 Player class //////////////////////////////
 Used for the main player
*/

class PLAYER : public MOVE  // inherits the MOVE class
{
	public:
	int points;              // keep track of score
	int jumpforce, jumpmax;  // for jumping
	int keys;				 // number of keys
	int lives;	     		 // keeps track of lives
	int startx, starty;      // starting point

	PLAYER() // inline constructor
	{ 
		startx=starty=points=jumpforce=jumpmax=keys=0;  // set everything to 0		
		lives=3;	// a default of 3 lives
	};
	
};


// a struct for the point floater
struct FLOATTYPE
{
	bool active;  // true if it's shown on the screen
	int x, y;     // coordinates
	int min;      // minimum y cordinate
	char *text;   // text it contains
};

/*
  Floater class //////////////////////////////
  Handles point floaters
*/

class FLOATER 
{
private:
	FLOATTYPE floats[NUMFLOATS]; // up to 10 at once

public:
	// constructor
	FLOATER(); 

	// creates a new float
	void Create(int x, int y, char *text);

	// processes the floats
	void Process(LPDIRECTDRAWSURFACE lpdds);
};





// the simple bitmap image
typedef struct BITMAP_IMAGE_TYP
        {
        int state;          // state of bitmap
        int attr;           // attributes of bitmap
        int x,y;            // position of bitmap
        int width, height;  // size of bitmap
        int num_bytes;      // total bytes of bitmap
        UCHAR *buffer;      // pixels of bitmap

        } BITMAP_IMAGE, *BITMAP_IMAGE_PTR;

// PROTOTYPES /////////////////////////////////////////////

// DirectDraw functions
int DD_Init(int width, int height, int bpp);
int DD_Shutdown(void);
LPDIRECTDRAWCLIPPER DD_Attach_Clipper(LPDIRECTDRAWSURFACE lpdds, int num_rects, LPRECT clip_list);
LPDIRECTDRAWSURFACE DD_Create_Surface(int width, int height, int mem_flags);
int DD_Flip(void);
int DD_Wait_For_Vsync(void);
int DD_Fill_Surface(LPDIRECTDRAWSURFACE lpdds,int color);
UCHAR *DD_Lock_Surface(LPDIRECTDRAWSURFACE lpdds,int *lpitch);
int DD_Unlock_Surface(LPDIRECTDRAWSURFACE lpdds, UCHAR *surface_buffer);
UCHAR *DD_Lock_Primary_Surface(void);
int DD_Unlock_Primary_Surface(void);
UCHAR *DD_Lock_Back_Surface(void);
int DD_Unlock_Back_Surface(void);


// general utility functions
DWORD Get_Clock(void);
DWORD Start_Clock(void);
DWORD Wait_Clock(DWORD count);


// palette functions
int Set_Palette_Entry(int color_index, LPPALETTEENTRY color);
int Get_Palette_Entry(int color_index, LPPALETTEENTRY color);
int Load_Palette_From_File(char *filename, LPPALETTEENTRY palette);
int Save_Palette_To_File(char *filename, LPPALETTEENTRY palette);
int Save_Palette(LPPALETTEENTRY sav_palette);
int Set_Palette(LPPALETTEENTRY set_palette);
int Rotate_Colors(int start_index, int colors);


// simple bitmap image functions
int Create_Bitmap(BITMAP_IMAGE_PTR image, int x, int y, int width, int height);
int Destroy_Bitmap(BITMAP_IMAGE_PTR image);
int Draw_Bitmap(BITMAP_IMAGE_PTR source_bitmap,UCHAR *dest_buffer, int lpitch, int transparent);
int Load_Image_Bitmap(BITMAP_IMAGE_PTR image,BITMAP_FILE_PTR bitmap,int cx,int cy,int mode);               
int Scroll_Bitmap(void); // ni
int Copy_Bitmap(void); // ni
int Flip_Bitmap(UCHAR *image, int bytes_per_line, int height);

// bitmap file functions
int Load_Bitmap_File(BITMAP_FILE_PTR bitmap, char *filename);
int Unload_Bitmap_File(BITMAP_FILE_PTR bitmap);

// gdi functions
int Draw_Text_GDI(char *text, int x,int y,COLORREF color, LPDIRECTDRAWSURFACE lpdds);
int Draw_Text_GDI(char *text, int x,int y,int color, LPDIRECTDRAWSURFACE lpdds);

// error functions
int Open_Error_File(char *filename);
int Close_Error_File(void);
int Write_Error(char *string, ...);

// GLOBALS ////////////////////////////////////////////////

extern FILE *fp_error;                           // general error file
extern LPDIRECTDRAW         lpdd;                 // dd object
extern LPDIRECTDRAWSURFACE  lpddsprimary;         // dd primary surface
extern LPDIRECTDRAWSURFACE  lpddsback;            // dd back surface
extern LPDIRECTDRAWPALETTE  lpddpal;              // a pointer to the created dd palette
extern LPDIRECTDRAWCLIPPER  lpddclipper;          // dd clipper
extern PALETTEENTRY         palette[256];         // color palette
extern PALETTEENTRY         save_palette[256];    // used to save palettes
extern DDSURFACEDESC        ddsd;                 // a direct draw surface description struct
extern DDBLTFX              ddbltfx;              // used to fill
extern DDSCAPS              ddscaps;              // a direct draw surface capabilities struct
extern HRESULT              ddrval;               // result back from dd calls
extern UCHAR                *primary_buffer;      // primary video buffer
extern UCHAR                *back_buffer;         // secondary back buffer
extern int                  primary_lpitch;       // memory line pitch
extern int                  back_lpitch;          // memory line pitch
extern BITMAP_FILE          bitmap16bit;          // a 16 bit bitmap file
extern BITMAP_FILE          bitmap8bit;           // a 8 bit bitmap file

extern DWORD                start_clock_count;    // used for timing

// these defined the general clipping rectangle
extern int min_clip_x,                             // clipping rectangle 
           max_clip_x,                  
           min_clip_y,     
           max_clip_y;                  

// these are overwritten globally by DD_Init()
extern int screen_width,                            // width of screen
           screen_height,                           // height of screen
           screen_bpp;                              // bits per pixel 

#endif