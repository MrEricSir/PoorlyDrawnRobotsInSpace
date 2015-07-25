// The Project

// Includes

#define WIN32_LEAN_AND_MEAN  
#define INITGUID

// Windows
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

// C and C++
#include <objbase.h>
#include <iostream.h> 
#include <fstream.h>
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

// DirectX
#include <ddraw.h>  
#include <dinput.h> 

// The core of the engine
#include "game.h"

// Defines

// defines for windows 
#define WINDOW_CLASS_NAME "WINXCLASS"  // class name

#define WINDOW_WIDTH    64   // size of window
#define WINDOW_HEIGHT   48

// size of level
#define LEVEL_HEIGHT 16
#define LEVEL_WIDTH 21

// dude directions
#define DUDE_RIGHT        0
#define DUDE_LEFT         1
#define DUDE_RIGHT_AIR    2  
#define DUDE_LEFT_AIR     3

// enemies
#define ENEMY_LEFT 0
#define ENEMY_RIGHT 1
#define NUM_ENEMIES 5

// points
#define BONUS_POINTS  100

// last level in the game
#define LAST_LEVEL 4


// Global vars

HWND main_window_handle = NULL; // save the window handle
HINSTANCE main_instance = NULL; // save the instance
char buffer[80];                // used to print text

// directinput globals
LPDIRECTINPUT        lpdi      = NULL;    // dinput object
LPDIRECTINPUTDEVICE  lpdikey   = NULL;    // dinput keyboard
LPDIRECTINPUTDEVICE  lpdimouse = NULL;    // dinput mouse
LPDIRECTINPUTDEVICE  lpdijoy   = NULL;    // dinput joystick 
LPDIRECTINPUTDEVICE2 lpdijoy2  = NULL;    // dinput joystick
GUID                 joystickGUID;        // guid for main joystick
char                 joyname[80];         // name of joystick

// these contain the target records for all di input packets
UCHAR keyboard_state[256]; // contains keyboard state table
DIMOUSESTATE mouse_state;  // contains state of mouse
DIJOYSTATE joy_state;      // contains state of joystick

// demo globals
PLAYER          dude;     // the player dude
MOVE enemy[NUM_ENEMIES];  // up to five bad guys per level
STAT back;  // background
STAT bonusblue, bonusred; // bonus
STAT key;
STAT lock;
STAT ladder;
STAT spikes;
STAT exito;

// true if it's time to advance to the next level
bool advance;

// point floater
FLOATER pointfloat;

int level[LEVEL_WIDTH][LEVEL_HEIGHT];  // level is (x, y)
	




BITMAP_IMAGE background;      // the background   

// Prototypes

// Game functions
int GameInit(void *parms=NULL);
int LevelInit(int levelnum);
int GameShutdown(void *parms=NULL);
int GameMain(void *parms=NULL);

// Other important functions
void ArtificialStupidity();
bool CheckY(int x, int y, int item);
bool CheckX(int x, int y, int item);
bool Touching(int x, int y, int item, int& xloc, int& yloc);
bool LoadLevel();


// ----- Functions to deal with Bill Gates' gang -----

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// this is the main message handler of the system
	PAINTSTRUCT	ps; // used in WM_PAINT
	HDC	hdc; // handle to a device context

	// what is the message 
	switch(msg)
		{	
		case WM_CREATE: 
			{
				// init
				return(0);
			} break;
	
		case WM_PAINT:
	         {
				// start painting
				hdc = BeginPaint(hwnd,&ps);
	
				// end painting
				EndPaint(hwnd,&ps);
				return(0);
			} break;
	
		case WM_DESTROY: 
			{
				// kill the application			
				PostQuitMessage(0);
				return(0);
			} break;
	
		default:break;
	
	    } 
	
	// return other messages for processing
	return (DefWindowProc(hwnd, msg, wparam, lparam));
}


int WINAPI WinMain(	HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline,
					int ncmdshow)
{
	WNDCLASS winclass;	// class holder
	HWND	 hwnd;		// window handle
	MSG		 msg;		// message

	// first fill in the window class stucture
	winclass.style			= CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc	= WindowProc;
	winclass.cbClsExtra		= 0;
	winclass.cbWndExtra		= 0;
	winclass.hInstance		= hinstance;
	winclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground	= GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName	= NULL; 
	winclass.lpszClassName	= WINDOW_CLASS_NAME;

	// register the window class
	if (!RegisterClass(&winclass)) return(0);

	// create the window
	if (!(hwnd = CreateWindow(WINDOW_CLASS_NAME, "Watermelon Productions", 
		WS_POPUP | WS_VISIBLE, 0,0,	WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL,
		hinstance, NULL))) return(0);

	// save the window handle and instance in a global
	main_window_handle = hwnd;
	main_instance      = hinstance;

	// load graphics, etc
	GameInit();

	// not time to go to the next level
	advance = false;

	// first level
	int currlevel = 1;

	// load the first level
	LevelInit(currlevel);  

	// enter main event loop
	while(true)
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{ 
			// if a quit message is posted, bust out of loop
			if (msg.message == WM_QUIT) break;
	
			// translate any accelerator keys
			TranslateMessage(&msg);

			// send the message to the window proc
			DispatchMessage(&msg);
		}
    
		// the main part of the game
		if (currlevel != LAST_LEVEL) GameMain();

		// load the next level if it's time
		if (advance) 
		{
			currlevel++;			
			LevelInit(currlevel);
			advance = false;
		}
	}

	// stop game, release memory, release DirectX
	GameShutdown();

	// return to Windows
	return(msg.wParam);
} 


// ----- Functions for use within the game -----


void ArtificialStupidity()
// Controls the enemies
// Like artificial inteligence, only stupider
{
	for (int i = 0; i < NUM_ENEMIES; i++)
		if (enemy[i].Visible())
		{		
			
			// if enemy is going left
			if (enemy[i].curr_animation == ENEMY_LEFT)
			{
				// make sure he's not about to run into something
				if (!CheckX(enemy[i].x-1, enemy[i].y, 1) &&
				// or off a ledge!
					!CheckY(enemy[i].x-1, enemy[i].y+30, 0))
					enemy[i].x-=2;
				else
				{
					// if he is, turn 'em around!
					enemy[i].curr_animation = ENEMY_RIGHT;
					enemy[i].x+=2;
				}
			}
			// if enemy is going right
			else if (enemy[i].curr_animation == ENEMY_RIGHT)
			{
				// make sure he's not about to run into something
				if (!CheckX(enemy[i].x+31, enemy[i].y, 1) &&
				// or off a ledge!
					!CheckY(enemy[i].x+1, enemy[i].y+30, 0))
					enemy[i].x+=2;
				else 
				{
					// if he is, turn 'em around!
					enemy[i].curr_animation = ENEMY_LEFT;
					enemy[i].x-=2;
				}
			}		
		}
	return;
}



bool CheckY(int x, int y, int item)
// returns true if item blocks the player's path
{
	// off screen, only if wall being detected
	if ((item == 1) && (y < 0 || y > 479)) return true;

	if (x%30 != 0)
	{
		if ((level[x/30][y/30] == item) ||
		(level[(x/30)+1][y/30] == item)) return true;
	} 
	else 
		if (level[x/30][y/30] == item) return true;
	return false;
}



bool CheckX(int x, int y, int item)
// returns true if item blocks the player's path
{
	// off screen
	if ((item == 1) && (x < 0 || x > 629)) return true;

	if (y%30 != 0)
	{
		if ((level[x/30][y/30] == item) ||
		(level[x/30][(y/30)+1] == item)) return true;
	} 
	else  
		if (level[x/30][y/30] == item) return true;
	return false;
}


bool Touching(int x, int y, int item, int& xloc, int& yloc)
// Figures out if player at x, y is touching a bonus
{

	// compute location of player on level map
	int minx = int(x/30),
		miny = int(y/30),
		maxx = minx+1,
		maxy = miny+1;
	

	// if player is aligned with the level	
	if (x%30 == 0) 
	{
		//minx++;
		maxx--;
	}
	if (y%30 == 0) 
	{
		//miny++;
		maxy--;
	} 

	// checks to see if we've run into the bonus
	if (level[minx][miny] == item) 
	{
		xloc = minx;
		yloc = miny;
		return true;
	}
	if (level[maxx][miny] == item) 
	{
		xloc = maxx;
		yloc = miny;
		return true;
	}
	if (level[minx][maxy] == item) 
	{
		xloc = minx;
		yloc = maxy;
		return true;
	}
	if (level[maxx][maxy] == item) 
	{
		xloc = maxx;
		yloc = maxy;
		return true;
	}

	return false;
}



bool LoadLevel(int levelnum, char *filename)
// Loads a level from a file
{
	// open the file
	fstream data("level.txt", ios::in);
	int i;
	char c;

	for (int y=0; y < LEVEL_HEIGHT; y++)
	{
		for (int x=0; x < LEVEL_WIDTH; x++)
		{
			data >> c;
			if ( c == '*') level[x][y] = 10;
			else
			{
				i = c - '0';
				level[x][y] = i;
			}
		}
	}

	// close the file and exit
	data.close();
	return true;
}



void DrawScore()
// draws some important stats at the bottom of the screen
{
	char text[15];  // a max of 15 digits

	// lives
	sprintf(text, "%d", dude.lives);  // turn lives into text
	Draw_Text_GDI("Lives:",5,455,RGB(255,0,0),lpddsback);
	Draw_Text_GDI(text,55,455,RGB(0,255,255),lpddsback);

	// keys
	sprintf(text, "%d", dude.keys);  // turn keys into text
	Draw_Text_GDI("Keys:",80,455,RGB(255,0,0),lpddsback);
	Draw_Text_GDI(text,127,455,RGB(0,255,255),lpddsback);

	// score
	sprintf(text, "%d", dude.points);  // turn score into text
	Draw_Text_GDI("Score:",150,455,RGB(255,0,0),lpddsback);
	Draw_Text_GDI(text,205,455,RGB(0,255,255),lpddsback);
	
	return;
}



bool LoseLife()
// true if life can be lost, false if player is dead
{
	// check to see if he's dead :-(
	if (dude.lives == 0) return false;

	// ouch - there goes a life!
	dude.lives--;

	// move him back to the starting position
	dude.x = dude.startx;
	dude.y = dude.starty;		

	return true;
}




int GameInit(void *parms)
// game init
{
	// animation sequences
	int dude_walk_right[7] = {0, 1, 2, 3, 4, 5, 6 };
	int dude_walk_left[7] = {7, 8, 9, 10, 11, 12, 13 };
	int dude_jump_right[1] = {14};
	int dude_jump_left[1] = {15};
	int enemy_left_anim[2] = {0,1};
	int enemy_right_anim[2] = {2, 3};
	int spikes_anim[2] = {0, 1};

	char *filename; // used for file names

	// initialize directdraw
	DD_Init(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP);

	// create background image buffer
	Create_Bitmap(&background, 0,0, 640, 480);

	// show the title screen
	Load_Bitmap_File(&bitmap8bit, "title.bmp");
	Load_Image_Bitmap(&background,&bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
	Set_Palette(bitmap8bit.palette);
	Unload_Bitmap_File(&bitmap8bit);
	Start_Clock();
	DD_Fill_Surface(lpddsback, 0);
	DD_Lock_Back_Surface();
	Draw_Bitmap(&background, back_buffer, back_lpitch, 0);
	DD_Unlock_Back_Surface();
    DD_Flip();

	// keyboard creation section ////////////////////////////////

	// first create the direct input object
	if (DirectInputCreate(main_instance,DIRECTINPUT_VERSION,&lpdi,NULL)!=DI_OK)
		return(0);

	// create a keyboard device  //////////////////////////////////
	if (lpdi->CreateDevice(GUID_SysKeyboard, &lpdikey, NULL)!=DI_OK) return(0);
	
	// set cooperation level
	if (lpdikey->SetCooperativeLevel(main_window_handle, 
                 DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)!=DI_OK) return(0);

	// set data format
	if (lpdikey->SetDataFormat(&c_dfDIKeyboard)!=DI_OK) return(0);

	// acquire the keyboard
	if (lpdikey->Acquire()!=DI_OK) return(0);


	//---------------------------------------------------------\\
	

	// create and load frames

	// create the player
	if (!dude.CreateMOVE(0,0,31,31,16, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create the enemies	
	for (int i = 0; i < 5; i++)
		if (!enemy[i].CreateMOVE(0,0,31,31,4, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create walls (only used in debuging)
	if (!back.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create bonuses
	if (!bonusblue.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);
	if (!bonusred.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create key
	if (!key.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create lock
	if (!lock.CreateSTAT(31, 31, 1, 
		ATTR_VISIBLE /*| ATTR_SINGLE_FRAME*/,DDSCAPS_SYSTEMMEMORY)) return(0);

	// create ladder
	if (!ladder.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create spikes
	if (!spikes.CreateSTAT(31, 31, 2, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// create exit
	if (!exito.CreateSTAT(31, 31, 1, ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY)) return(0);

	// load bitmap
	filename = "items.bmp";
	Load_Bitmap_File(&bitmap8bit,filename);
		

	// get images for main player
	for (i = 0; i < 7; i++)  
	{
		// get images for main character image
		dude.LoadFrame(&bitmap8bit,   i, i, 0, BITMAP_EXTRACT_MODE_CELL);  
		dude.LoadFrame(&bitmap8bit, i+7, i, 1, BITMAP_EXTRACT_MODE_CELL);  		
	}	
	dude.LoadFrame(&bitmap8bit, 14, 0, 2, BITMAP_EXTRACT_MODE_CELL);  
	dude.LoadFrame(&bitmap8bit, 15, 1, 2, BITMAP_EXTRACT_MODE_CELL);  
	
	// turn individual frames into animations
	dude.LoadAnimation(DUDE_RIGHT, 7, dude_walk_right);
	dude.LoadAnimation(DUDE_LEFT, 7, dude_walk_left);
	dude.LoadAnimation(DUDE_RIGHT_AIR, 1, dude_jump_right);
	dude.LoadAnimation(DUDE_LEFT_AIR, 1, dude_jump_left);


	// get images for enemies
	for (int j = 0; j < NUM_ENEMIES; j++)
	{
		for (i = 0; i < 4; i++)  	
			enemy[j].LoadFrame(&bitmap8bit,   i, i+2, 2, BITMAP_EXTRACT_MODE_CELL);  
	
		enemy[j].LoadAnimation(ENEMY_LEFT, 2,enemy_left_anim);
		enemy[j].LoadAnimation(ENEMY_RIGHT,2,enemy_right_anim);
	}
		

	// ladder image
	ladder.LoadFrame(&bitmap8bit,0,2,3,BITMAP_EXTRACT_MODE_CELL);

	// lock and key images		
	key.LoadFrame(&bitmap8bit,0,0,3,BITMAP_EXTRACT_MODE_CELL);
	lock.LoadFrame(&bitmap8bit,0,1,3,BITMAP_EXTRACT_MODE_CELL);

	// load background image
	back.LoadFrame(&bitmap8bit,0,0,4,BITMAP_EXTRACT_MODE_CELL);
	
	// load bonuses
	bonusblue.LoadFrame(&bitmap8bit,0,3,3,BITMAP_EXTRACT_MODE_CELL);
	bonusred.LoadFrame(&bitmap8bit,0,4,3,BITMAP_EXTRACT_MODE_CELL);

	// load exit
	exito.LoadFrame(&bitmap8bit,0,5,3,BITMAP_EXTRACT_MODE_CELL);	

	// load spikes
	spikes.LoadFrame(&bitmap8bit,0,6,2,BITMAP_EXTRACT_MODE_CELL);
	spikes.LoadFrame(&bitmap8bit,1,7,2,BITMAP_EXTRACT_MODE_CELL);

	// create animation for spikes
	spikes.LoadAnimation(0, 2, spikes_anim);
	spikes.SetAnimSpeed(4);

	// remove bitmap from memory
	Unload_Bitmap_File(&bitmap8bit);
 
	// set up stating states of dude
	dude.SetAnimation(0);
	dude.SetAnimSpeed(4);
	
	// set clipping rectangle, turn off cursor
	RECT screen_rect = {0,0,screen_width,screen_height};
	lpddclipper = DD_Attach_Clipper(lpddsback,1,&screen_rect);
	ShowCursor(FALSE);

	// wait, clear screen
	Wait_Clock(6000);
	Start_Clock();
	DD_Fill_Surface(lpddsback, 0);
	DD_Flip();
	Wait_Clock(30);
	
	return true;
} 




int LevelInit(int levelnum)
{
	// if it's the last level, quit
	if (levelnum == LAST_LEVEL)
	{
		PostMessage(main_window_handle, WM_DESTROY,0,0);
		return true;
	}

	// load level
	fstream data("level.txt", ios::in);
	char c;
	char text[75];
	int i;

	// find correct location in file
	for (i = 0; i < (levelnum-1)* (LEVEL_HEIGHT+1); i++)
	{
		// get each line, throw it away		
		data.getline(text, 75);
	}

	// load level into array	
	for (int y=0; y < LEVEL_HEIGHT; y++)
	{
		for (int x=0; x < LEVEL_WIDTH; x++)
		{
			// take in data
			data >> c;

			// convert it to level format
			if ( c == '*') level[x][y] = 10;
			else level[x][y] = c - '0';
		}
	}

	// close the level file
	data.close();

	// load the background
	char filename[11];
	
	sprintf(filename, "level%d.bmp", levelnum);	
	Load_Bitmap_File(&bitmap8bit, filename);

	// set the palette
	Set_Palette(bitmap8bit.palette);

	// load the background bitmap image	
	Load_Image_Bitmap(&background,&bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
	Unload_Bitmap_File(&bitmap8bit);

	// set up starting states of enemy
	for (i=0; i < NUM_ENEMIES; i++)
	{
		enemy[i].Hide();
		enemy[i].curr_animation = ENEMY_LEFT;
		enemy[i].SetAnimation(0);
		enemy[i].SetAnimSpeed(4);
	}

	// set coordinates of starting locations
	i = 0;
	for (y=0; y < LEVEL_HEIGHT; y++)
		for (int x=0; x < LEVEL_WIDTH; x++)
		{
			// enemy
			if (level[x][y]==9)
			{
				level[x][y] = 0;
				enemy[i].x=x*30;
				enemy[i].y=y*30;
				enemy[i].Show();
				i++;
			}
			// main player
			if (level[x][y]==10)
			{
				level[x][y] = 0;
				dude.x=dude.startx=x*30;
				dude.y=dude.starty=y*30;
			}
		}

	return true;
}


int GameShutdown(void *parms)
// releases memory, etc
{	
	// show game over screen
	Load_Bitmap_File(&bitmap8bit, "over.bmp");
	Load_Image_Bitmap(&background,&bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
	Set_Palette(bitmap8bit.palette);
	Unload_Bitmap_File(&bitmap8bit);
	Start_Clock();
	DD_Fill_Surface(lpddsback, 0);
	DD_Lock_Back_Surface();
	Draw_Bitmap(&background, back_buffer, back_lpitch, 0);
	DD_Unlock_Back_Surface();
    DD_Flip();
	
	// kill dude
	dude.Destroy();

	// kill enemies
	for (int i = 0; i < NUM_ENEMIES; i++)
		enemy[i].Destroy();

	// kill back
	back.Destroy();

	// kill ladder
	ladder.Destroy();

	// kill lock and key
	key.Destroy();
	lock.Destroy();

	// kill bonuses
	bonusblue.Destroy();
	bonusred.Destroy();

	// kill spikes
	spikes.Destroy();

	// kill exit
	exito.Destroy();

	// release keyboard
	lpdikey->Unacquire();
	lpdikey->Release();
	lpdi->Release();

	// wait before blanking screen
	Wait_Clock(6000);

	// now blank the screen so this looks less janky
	Start_Clock();
	DD_Fill_Surface(lpddsback, 0);
	DD_Flip();
	Wait_Clock(60);	

	// kill the background
	Destroy_Bitmap(&background);

	// turn off DirectDraw
	DD_Shutdown();

	return true;
} 



// ------- The actual game.  This ain't no pong! --------

int GameMain(void *parms)
// where the good stuff happens
// this function is called over, and over, and over...
{	
	bool player_moving = false,  // is player moving?
		 player_air = false;     // is the player in the air?

	int lx=0, ly=0;
	
	// check of user is trying to exit
	if (KEY_DOWN(VK_ESCAPE)) PostMessage(main_window_handle, WM_DESTROY,0,0);

	// start the timing clock
	Start_Clock();

	// clear the drawing surface
	DD_Fill_Surface(lpddsback, 0);

	// lock the back buffer
	DD_Lock_Back_Surface();

	// draw the background background image
	Draw_Bitmap(&background, back_buffer, back_lpitch, 0);

	// unlock the back buffer
	DD_Unlock_Back_Surface();

	// get keyboard info
	lpdikey->GetDeviceState(256, (LPVOID)keyboard_state);

	
	// Climbing the stairway to heaven
	if (keyboard_state[DIK_UP] && Touching(dude.x, dude.y, 3, lx, ly)
		&& !CheckY(dude.x, dude.y-1, 1))
	{
		// if he's not right over the ladder
		if (dude.x != lx * 30)
		{
			if (dude.x > lx *30) dude.x -= (dude.x%30);
			else if (dude.x < lx *30) dude.x += (30 - (dude.x%30));
		}
		// move up!
		dude.y -= 5;

		// makes the animation look better
		player_air = true;
		player_moving = true;
	}

	// Going down
	if (keyboard_state[DIK_DOWN] && Touching(dude.x, dude.y+1, 3, lx, ly) &&
	   !CheckY(dude.x, dude.y+30, 1))
	{
		// if he's not right over the ladder
		if (dude.x != lx * 30)
		{
			if (dude.x > lx *30) dude.x -= (dude.x%30);
			else if (dude.x < lx *30) dude.x += (30 - (dude.x%30));
		}
		// move down!
		dude.y += 5;

		// makes the animation look better
		player_air = true;
		player_moving = true;
	}

	// Might as well jump
	if ((keyboard_state[DIK_LCONTROL] || keyboard_state[DIK_RCONTROL]) &&		
	((CheckY(dude.x, dude.y+30, 1) && !CheckY(dude.x, dude.y-30, 1)) || // wall
	 (CheckY(dude.x, dude.y+30, 2) && !CheckY(dude.x, dude.y-30, 2)) || // keyhole
	 (CheckY(dude.x, dude.y+30, 3) && !CheckY(dude.x, dude.y-30, 3))) && // ladder
	(dude.jumpmax == 0))
	{
		dude.jumpmax += 15;		
	}
	

	// Simulate gravity
	if (dude.jumpforce == 0 && !CheckY(dude.x, dude.y+30, 1)
		&& !CheckY(dude.x, dude.y+30, 2) && !CheckY(dude.x, dude.y+30, 3))
	{
		// fall down 5 pixels
		dude.y+=5;

		// set animation up correctly
		player_moving = true;
		player_air = true;	
	}

	
	// Jumping routine

	// top of jump
	if (dude.jumpforce > 0 && dude.jumpforce == dude.jumpmax)
	{
		dude.jumpmax = 0;
		dude.jumpforce = 0;		

	}
	// jumping
	if (dude.jumpmax > 0)
	{
		// set animation up correctly
		player_moving = true;
		player_air = true;
		
		// jump up
		if (!CheckY(dude.x, dude.y-dude.jumpmax+dude.jumpforce-1, 1)  // wall
			&& !CheckY(dude.x, dude.y-dude.jumpmax+dude.jumpforce-1, 2)) // keyhole			
		{
			dude.jumpforce +=1;
			dude.y-=(dude.jumpmax-dude.jumpforce);
		}
		// he hit his head!
		else
		{
			dude.y -= (dude.y%30);  // get it back on line
			dude.jumpforce = 0;
			dude.jumpmax = 0;
		}
	}



	// Moving the player (with the keyboard)
	if (keyboard_state[DIK_RIGHT] && keyboard_state[DIK_LEFT])
	{
		// don't move either way
	}
	else if (keyboard_state[DIK_RIGHT] && !CheckX(dude.x+30, dude.y, 1)
		&& !CheckX(dude.x+30, dude.y, 2))
	{
		// move right
		dude.x+=2;		

		// check animation needs to change
		if (dude.curr_animation != DUDE_RIGHT)
			dude.SetAnimation(DUDE_RIGHT);		

		// set motion flag
		player_moving = true;
	} 
	else if (keyboard_state[DIK_LEFT] && !CheckX(dude.x-1, dude.y, 1)
		&& !CheckX(dude.x-1, dude.y, 2))  
	{
		// move left
		dude.x-=2;			   
		
	    // check if animation needs to change
		if (dude.curr_animation != DUDE_LEFT)
			dude.SetAnimation(DUDE_LEFT);

		// set motion flag
		player_moving = true;
	} 		


	// if the player is in the air, show a different sprite
	if (player_air)
	{
		if (dude.curr_animation == DUDE_RIGHT) dude.SetAnimation(DUDE_RIGHT_AIR);
		else if (dude.curr_animation == DUDE_LEFT) dude.SetAnimation(DUDE_LEFT_AIR);
	}


	// open locks
	if (dude.keys > 0)
	{
		// lock to the left
		if(CheckX(dude.x-1, dude.y, 2))
		{		
			level[(dude.x/30)-1][dude.y/30] = 0;
			dude.keys--;
		}
		// lock to the right
		if(CheckX(dude.x+31, dude.y, 2))
		{		
			level[(dude.x/30)+1][dude.y/30] = 0;
			dude.keys--;
		}
		// lock on the floor 
		if(CheckY(dude.x, dude.y+30, 2))
		{		
			if (level[dude.x/30][(dude.y/30)+1] == 2) 
				level[dude.x/30][(dude.y/30)+1] = 0;
			else if (level[(dude.x/30)+1][(dude.y/30)+1] == 2) 
				level[(dude.x/30)+1][(dude.y/30)+1] = 0;
			dude.keys--;
		}
		// lock on the ceiling
		// (these are skany due to the restrictions of this engine)
		if(CheckY(dude.x, dude.y-1, 2))
		{		
			if (level[dude.x/30][(dude.y/30)-1] == 2) 
				level[dude.x/30][(dude.y/30)-1] = 0;
			else if (level[(dude.x/30)+1][(dude.y/30)-1] == 2) 
				level[(dude.x/30)+1][(dude.y/30)-1] = 0;
			dude.keys--;
		}
	}

	// detect collisions with non-solid objects (bonuses :-)

	// check for points bonuses
	if (Touching(dude.x, dude.y, 5, lx, ly) || Touching(dude.x, dude.y, 6, lx, ly))
	{			
		pointfloat.Create(lx*30, ly*30, "100"); 
		dude.points+=BONUS_POINTS;
		level[lx][ly] = 0;
	}

	// check for keys
	if (Touching(dude.x, dude.y, 4, lx, ly))
	{				
		// first destroy key
		level[lx][ly] = 0;

		// add a key
		dude.keys++;
	}

	// make spikes go up and down
	spikes.Animate();  

		
	// map sprites
	for (int gry = 0; gry < 16; gry++)
		for (int grx = 0; grx < 21; grx++)
		{			
			// item 1 - wall
			//if (level[grx][gry] == 1) back.Draw(grx * 30, gry * 30, lpddsback);
			// item 2 - lock
			if (level[grx][gry] == 2) lock.Draw(grx * 30, gry * 30, lpddsback);
			// item 3 - ladder
			if (level[grx][gry] == 3) ladder.Draw(grx * 30, gry * 30, lpddsback);
			// item 4 - key
			if (level[grx][gry] == 4) key.Draw(grx * 30, gry * 30, lpddsback);
			// item 5 - blue bonus
			if (level[grx][gry] == 5) bonusblue.Draw(grx * 30, gry * 30, lpddsback);
			// item 6 - red bonus
			if (level[grx][gry] == 6) bonusred.Draw(grx * 30, gry * 30, lpddsback);
			// item 7 - spikes
			if (level[grx][gry] == 7) spikes.Draw(grx * 30, gry * 30, lpddsback);
			// item 8 - exit
			if (level[grx][gry] == 8) exito.Draw(grx * 30, gry * 30, lpddsback);
		}

	// move the enemies
	ArtificialStupidity();
	
	// Check to see if player is touching an enemy (ouch!)
	for (int i = 0; i < NUM_ENEMIES; i++)
	{
		// if the player has touched an enemy
		if (enemy[i].Visible() && TestCollide(dude, enemy[i]))
		{
			// he's lost a life!
			if (!LoseLife()) 
			{
				// hide dude
				dude.Hide();

				// quit the game
				PostMessage(main_window_handle, WM_DESTROY,0,0);
			}
		}
	}

	// Check to see if player is touching spikes (doh!)
	if (Touching(dude.x, dude.y, 7, lx, ly))
	{
		// he's lost a life!		
		if (!LoseLife()) 
		{
			// hide dude
			dude.Hide();

			// quit the game
			PostMessage(main_window_handle, WM_DESTROY,0,0);
		}
	}

	// See if player can leave the level!!!
	if (Touching(dude.x, dude.y, 8, lx, ly))
	{
		// time to leave
		advance = true;
	}

	// draw the enemies
	for (i = 0; i < NUM_ENEMIES; i++)
	{
		enemy[i].Animate();
		enemy[i].Draw(lpddsback);
	}

	// only animate if player is moving
	if (player_moving) dude.Animate(); 	// animate dude

	
	// draw the dude
	dude.Draw(lpddsback);

	// draw point floaters
	pointfloat.Process(lpddsback);

	// score, lives, etc
	DrawScore();		

	// flip the surfaces
	DD_Flip();

	// sync to 30 mseconds
	Wait_Clock(30);

	return true;
} 