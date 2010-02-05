/*
  glcd.cpp - Arduino library support for graphic LCDs 
  Copyright (c)2008, 2009, 2010 Michael Margolis & Bill Perry All right reserved

  vi:ts=4
    
  This file contains high level functions based on the previous ks0108 library
  The routines were dervived from code written and copyright by Fabian Maximilian Thiele.
  you can obtain a copy of his original work here:
  http://www.scienceprog.com/wp-content/uploads/2007/07/glcd_ks0108.zip
 
  The glcd class impliments high level graphics routines. 
  It is derived from the glcd_Device class that impliments the protocol for sending and
  receiving data and commands to a GLCD device
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

*/

#include <avr/pgmspace.h>
#include "glcd.h"
#include "glcd_Config.h" 


#define BITMAP_FIX // enables a bitmap rendering fix/patch


/**
 * Initilize the GLCD library and hardware
 *
 * @param invert specifices whether display is in normal mode or inverted mode.
 *
 * This should be called prior to any other graphic library function.
 * It does all the needed initializations including taking care of the
 * low level hardware initalization of the display device.
 *
 * The optional @em invert parameter specifies if the display should be run in a normal
 * mode, dark pixels on light background or inverted, light pixels on a dark background.
 *
 * To specify dark pixels use the define @b NON-INVERTED and to use light pixels use
 * the define @b INVERTED
 *
 * Upon completion of the initialization, then entire display will be cleared.
 */

 
glcd::glcd(){
   this->Inverted=0; 
}

void glcd::Init(uint8_t invert){
    this->Inverted=invert;
	glcd_Device::Init(invert);  
	//this->Text.Init((glcd_Device *)this); // new TA - init of gText no longer needed
	//this->Text = gText();
}		
	
// Note that the ClearPage functions are now private and probably no longer be necessary - mem 21 Jan
// if ClearScreen coding is modifed to do the clear page	
/**
 * Clear a full row of LCD memory pages
 *
 * @param page the row of pages to clear
 * @param color the color to
 *
 * An LCD memory page is 1 byte of memory that consists of 8 vertical pixels 
 * This function will fill every page from x address 0 through
 * GLCD.Width-1 with pixels of the specified color.
 *
 * row is a value from 0 to GLCD.Height/8
 *
 * Color is optional and defaults to WHITE.
 *
 */

void glcd::ClearPage(uint8_t page, uint8_t color){
	this->ClearPage(page, 0, DISPLAY_WIDTH, color );	 
}

/**
 * Clear a specified LCD memory pages
 *
 * @param page the row of pages to clear
 * @startX starting x addres within row
 * @length amound of pages to clear
 * @param color the color to
 *
 * An LCD memory page is 1 byte of memory that consists of 8 vertical pixels 
 * This function will fill @em length pages from x address @em x through
 * GLCD.Width-1 with pixels of the specified color.
 *
 * row is a value from 0 to GLCD.Height/8
 * 
 * Color is optional and defaults to WHITE.
 */

void glcd::ClearPage(uint8_t page, uint8_t startX, uint8_t length, uint8_t color){
	if(startX + length > DISPLAY_WIDTH)
		length = DISPLAY_WIDTH - startX;

	this->GotoXY(startX, page * 8); 
	while(length--){
		this->WriteData(color);
	}
}

/**
 * Clear the lcd display
 *
 * @param color BLACK or WHITE
 *
 * Sets all the pixels on the display from 0,0 to GLCD.Width-1,GLCD.Height-1
 * to the specified color.
 *
 * Color is optional and defaults to WHITE.
 *
 * @note
 * If the display is in INVERTED mode, then the color WHITE will paint the screen
 * BLACK and the color BLACK will paint the screen WHITE.
 *
 *
 */

void glcd::ClearScreen(uint8_t color){
 uint8_t page;
   for( page = 0; page < 8; page++){
	  ClearPage(page, color);
 }
}

/*
 * Drawline code is based on Bresenham's line algorithm
 * http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 *
 * This routine replaces the Teiele's DrawLine() routine which was
 * much larger and much slower.
 *	-- bperrybap
 */
 
/*
 * First, define a few macros to make the DrawLine code below read more like
 * the wikipedia example code.
 */

#define _GLCD_absDiff(x,y) ((x>y) ?  (x-y) : (y-x))
#define _GLCD_swap(a,b) \
do\
{\
uint8_t t;\
	t=a;\
	a=b;\
	b=t;\
} while(0)

/**
 * Draw a line
 *
 * @param x1 a value from 0 to GLCD.Width-1  indicating start x coordinate 
 * @param y1 a value fron 0 to GLCD.Height-1 indicating start y coordinate
 * @param x2 a value from 0 to GLCD.Width-1  indicating end x coordinate 
 * @param y2 a value fron 0 to GLCD.Height-1 indicating end y coordinate
 * @param color BLACK or WHITE 
 *
 * Draws a line starting at x1,y2 and ending at x2,y2.
 *
 * Color is optional and defaults to BLACK.
 *
 * @see DrawHLine()
 * @see DrawVLine()
 *
 */

void glcd::DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
uint8_t deltax, deltay, x,y, steep;
int8_t error, ystep;

  /*
   * Fudge up values of coordinate if out of range.
   */
	if(x1>=DISPLAY_WIDTH) x1=0;
	if(x2>=DISPLAY_WIDTH) x2=0;
	if(y1>=DISPLAY_HEIGHT) y1=0;
	if(y2>=DISPLAY_HEIGHT) y2=0;

	steep = _GLCD_absDiff(y1,y2) > _GLCD_absDiff(x1,x2);  

	if ( steep )
	{
		_GLCD_swap(x1, y1);
		_GLCD_swap(x2, y2);
	}

	if (x1 > x2)
	{
		_GLCD_swap(x1, x2);
		_GLCD_swap(y1, y2);
	}

	deltax = x2 - x1;
	deltay =_GLCD_absDiff(y2,y1);  
	error = deltax / 2;
	y = y1;
	if(y1 < y2) ystep = 1;  else ystep = -1;

	for(x = x1; x <= x2; x++)
	{
		if (steep) this->SetDot(y,x, color); else this->SetDot(x,y, color);
   		error = error - deltay;
		if (error < 0)
		{
			y = y + ystep;
			error = error + deltax;
    	}
	}
}

/**
 * Draw a rectangle of given width and height
 *
 * @param x the x coordinate of the upper left corner of the rectangle
 * @param y the y coordinate of the upper left corner of the rectangle
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param color BLACK or WHITE
 *
 * Draws a rectangle with the specified width and height.
 * The upper left corner at x,y and the lower right
 * corner at x+width,y+width.
 *
 * The length of the horizontal sides will be width+1 pixels
 * The length of the vertical sides will be height+1 pixels
 *
 * Color is optional and defaults to BLACK.
 *
 * @note The width and height parameters work differently than FillRect()
 *
 * @see FillRect()
 * @see InvertRect()
 */

void glcd::DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
	DrawHLine(x, y, width, color);				// top
	DrawHLine(x, y+height, width, color);		// bottom
	DrawVLine(x, y, height, color);			    // left
	DrawVLine(x+width, y, height, color);		// right
}

/**
 * Draw a rectangle with rounder corners
 *
 * @param x the x coordinate of the upper left corner of the rectangle
 * @param y the y coordinate of the upper left corner of the rectangle
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param radius ????
 * @param color BLACK or WHITE
 *
 * Draws a rectangle the same as DrawRect() but with rounded corners.
 * Radius is a value from 1 to half the height or width of the rectangle.
 * (what does that really mean?????)
 * Which is it, or is it a the smaller of the two?
 * FIXME FIXME need more description here.
 *
 */

void glcd::DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color) {
  	int16_t tSwitch; 
	uint8_t x1 = 0, y1 = radius; //BAPTEST
  	tSwitch = 3 - 2 * radius;
	
	while (x1 <= y1) {
	    this->SetDot(x+radius - x1, y+radius - y1, color);
	    this->SetDot(x+radius - y1, y+radius - x1, color);

	    this->SetDot(x+width-radius + x1, y+radius - y1, color);
	    this->SetDot(x+width-radius + y1, y+radius - x1, color);
	    
	    this->SetDot(x+width-radius + x1, y+height-radius + y1, color);
	    this->SetDot(x+width-radius + y1, y+height-radius + x1, color);

	    this->SetDot(x+radius - x1, y+height-radius + y1, color);
	    this->SetDot(x+radius - y1, y+height-radius + x1, color);

	    if (tSwitch < 0) {
	    	tSwitch += (4 * x1 + 6);
	    } else {
	    	tSwitch += (4 * (x1 - y1) + 10);
	    	y1--;
	    }
	    x1++;
	}
	  	
	this->DrawHLine(x+radius, y, width-(2*radius), color);			// top
	this->DrawHLine(x+radius, y+height, width-(2*radius), color);	// bottom
	this->DrawVLine(x, y+radius, height-(2*radius), color);			// left
	this->DrawVLine(x+width, y+radius, height-(2*radius), color);	// right
}

/**
 * Fill a Rectangle
 * 
 * @param x the x coordinate of the upper left corner of the rectangle
 * @param y the y coordinate of the upper left corner of the rectangle
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param color BLACK or WHITE
 *
 * Fills a rectanglular area of the specified width and height.
 *
 * The resulting rectangle covers an area @em width pixels wide by @em height pixels 
 * tall starting from the pixel at x,y. 
 *
 * The upper left corner at x,y and the lower right corner at x+width-1,y+width-1.
 *
 *
 * The length of the horizontal sides will be width pixels
 * The length of the vertical sides will be height pixels
 *
 * Color is optional and defaults to BLACK.
 *
 * @note The width and height parameters work differently than DrawRect()
 *
 *
 * @warning FillRect() behavior has changed from the previous versions of the ks0108 library.  
 *	The filled rectangle will be one pixel smaller in width and height than the old version. 
 *	This change was to make the functionality consistent with the way 
 *	Java and C# create filled rectangles
 *
 * @see DrawRect()
 * @see InvertRect()
 */

void glcd::FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    this->SetPixels(x,y,x+width,y+height,color);
}

/**
 * Invert a Rectangular area
 *
 * @param x the x coordinate of the upper left corner of the rectangle
 * @param y the y coordinate of the upper left corner of the rectangle
 * @param width width of the rectangle
 * @param height height of the rectangle
 * @param color BLACK or WHITE
 *
 * Inverts the pixels in the rectanglular area of the specified width and height.
 * BLACK pixels becom WHITE and WHITE pixels become BLACK.
 *
 * See FillRect() for full the full details of the rectangular area.
 *
 * @note The width and height parameters work differently than DrawRect()
 *
 * @warning InvertRect() behavior has changed from the previous versions of the ks0108 library.  
 *	The inverted rectangle will be one pixel smaller in width and height than the old version. 
 *
 * @see DrawRect()
 * @see FillRect()
 */


void glcd::InvertRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
	uint8_t mask, pageOffset, h, i, data, tmpData;
	height++;
	
	pageOffset = y%8;
	y -= pageOffset;
	mask = 0xFF;
	if(height < 8-pageOffset) {
		mask >>= (8-height);
		h = height;
	} else {
		h = 8-pageOffset;
	}
	mask <<= pageOffset;
	
	/*
	 * First do the fractional pages at the top of the region
	 */
	this->GotoXY(x, y);
	for(i=0; i<=width; i++) {
		data = this->ReadData();
		tmpData = ~data;
		data = (tmpData & mask) | (data & ~mask);
		this->WriteData(data);
	}
	
	/*
	 * Now do the full pages
	 */
	while(h+8 <= height) {
		h += 8;
		y += 8;
		this->GotoXY(x, y);
		
		for(i=0; i<=width; i++) {
			data = this->ReadData();
			this->WriteData(~data);
		}
	}
	
	/*
	 * Now do the fractional pages aat the bottom of the region
	 */
	if(h < height) {
		mask = ~(0xFF << (height-h));
		this->GotoXY(x, y+8);
		
		for(i=0; i<=width; i++) {
			data = this->ReadData();
			tmpData = ~data;
			data = (tmpData & mask) | (data & ~mask);
			this->WriteData(data);
		}
	}
}
/**
 * Set LCD inverted mode
 *
 * @param invert Inverted mode
 *
 * Sets the graphical state mode for the entire LCD display 
 * to @b NON_INVERTED (BLACK colorerd pixeld are dark)
 * or @b INVERTED (WHITE colored pixels are dark)
 *
 */

void glcd::SetDisplayMode(uint8_t invert) {  // was named SetInverted
	if(this->Inverted != invert) {
		this->InvertRect(0,0,DISPLAY_WIDTH-1,DISPLAY_HEIGHT-1);
		this->Inverted = invert;
	}
}

/**
 * Draw a bitmap image
 *
 * @param bitmap a ponter to the bitmap data
 * @param x the x coordinate of the upper left corner of the bitmap
 * @param y the y coordinate of the upper left corner of the bitmap
 * @param color BLACK or WHITE
 *
 * Draws a bitmap image with the upper left corner at location x,y
 * The bitmap data is assumed to be in program memory.
 *
 * Color is optional and defaults to BLACK.
 *
 */

void glcd::DrawBitmap(const uint8_t * bitmap, uint8_t x, uint8_t y, uint8_t color){
uint8_t width, height;
uint8_t i, j;

  width = ReadPgmData(bitmap++); 
  height = ReadPgmData(bitmap++);

#ifdef BITMAP_FIX // temporary ifdef just to show what changes if a new 
				// bit rendering routine is written.
							
  /*
   * In the absence of a new/better/proper bitmap rendering routine,
   * this routine will provide a temporary fix for drawing bitmaps that are
   * are non multiples of 8 pixels in height and start on non LCD page Y addresses.
   *
   * Note: nomally a routine like this should not have knowledge of how
   *	   how the lower level write routine works. But in this case it does.
   *
   *	Currently, the low level WriteData() routine ORs in the pixels when data spans
   *	a LCD memory page. So.....
   *
   * In order to ensure that the bitmap data is written to the pixels *exactly* as it
   * it defined in the bitmap data, i.e every black pixels is black and every white
   * pixel is white,...
   *
   * If height or y coordinate is not on a page boundary, clear the background first
   *	Techincally, this could be done all the time and it wouldn't hurt, it
   *	would just suck up a few more cycles.
   */
  if( (y & 7) || (height & 7))
  {
  	this->FillRect(x, y, width, height, WHITE);
  }
#endif

  for(j = 0; j < height / 8; j++) {
     this->GotoXY(x, y + (j*8) );
	 for(i = 0; i < width; i++) {
		 uint8_t displayData = ReadPgmData(bitmap++);
	   	 if(color == BLACK)
			this->WriteData(displayData);
		 else
		    this->WriteData(~displayData);
	 }
  }
}
// the following inline functions were added 2 Dec 2009 to replace macros

/**
 * Draw a Vertical Line
 *
 * @param x a value from 0 to GLCD.Width-1
 * @param y a value from 0 to GLCD.Height-1
 * @param height a value from 1 to GLCD.Height-y-1
 * @param color color of line
 *
 * color of BLACK or WHITE is an optional parameter indicating pixel color, default is BLACK
 *
 * The line drawn will be height+1 pixels.
 *
 * @note This function was previously named DrawVertLine() in the ks0108 library
 *
 * @see DrawLine()
 * @see DrawHLine()
 */
 

void glcd::DrawVLine(uint8_t x, uint8_t y, uint8_t height, uint8_t color){
  // this->FillRect(x, y, 0, length, color);
   this->SetPixels(x,y,x,y+height,color);
}
	
/**
 * Draw a Horizontal Line
 *
 * @param x a value from 0 to GLCD.Width-1
 * @param y a value from 0 to GLCD.Height-1
 * @param width a value from 1 to GLCD.Width-x-1
 * @param color BLACK or WHITE 
 *
 * The line drawn will be width+1 pixels.
 *
 * color is an optional parameter indicating pixel color and defaults to BLACK
 *
 * @note This function was previously named DrawHoriLine() in the ks0108 library
 *
 * @see DrawLine()
 * @see DrawVLine()
 */

void glcd::DrawHLine(uint8_t x, uint8_t y, uint8_t width, uint8_t color){
   // this->FillRect(x, y, length, 0, color);
    this->SetPixels(x,y, x+width, y, color);
}

/**
 * Draw a Circle
 *
 * @param xCenter X coordinate of the center of the circle
 * @param yCenter Y coordinate of the center of the circle
 * @param radius radius of circle
 * @param color BLACK or WHITE
 *
 * Draws a circle of the given radius extending out from
 * the center pixel.
 * The circle will fit inside a rectanglular area bounded by
 * x-radius,y-radius and x+radius,y+radius
 *
 * Because the circle is drawn from the center pixel out,
 * the diameter will be 2 * radius +1 pixels.
 *
 * Color is optional and defaults to BLACK.
 *
 * @see FillCircle()
 */
void glcd::DrawCircle(uint8_t xCenter, uint8_t yCenter, uint8_t radius, uint8_t color){
   this->DrawRoundRect(xCenter-radius, yCenter-radius, 2*radius, 2*radius, radius, color);
}

/**
 * Draw a Filled in a Circle
 *
 * @param xCenter X coordinate of the center of the circle
 * @param yCenter Y coordinate of the center of the circle
 * @param radius radius of circle
 * @param color
 *
 * Draws a filled in circle of the given radius extending out from
 * the center pixel.
 *
 * See DrawCircle() for the full details on sizing.
 *
 * Color is optional and defaults to BLACK.
 *
 * @see DrawCircle()
 *
 */

void glcd::FillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color)
{
/*
 * Circle fill Code is merely a modification of the midpoint
 * circle algorithem which is an adaption of Bresenham's line algorithm
 * http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 * http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 *
 * While it looks different it is the same calculation that is in
 * DrawRoundRect()
 *
 * Note: FillCircle()
 *		could be modified to work like DrawCircle() by using
 *		a DrawRoundRect() function.
 * 		DrawRoundRect() first plots the cicrular quadrants and then draws
 *		the horizontal and verticl lines between them to create
 *		a rounded rectangle.
 *		
 *		For fills of rounded rectangles, this type of circle fill algorithm
 *		would fill the upper and lower quadrants on each side of the rounded
 *		rectangle by connecting them vertically.
 *		When that was done, a filled rectangle would be neded need to fill the 
 *		space between them.
 *		There really isn't an easy way to fill a rounded rectangle.
 *
 *		For now, it is limited to circles.
 *
 * 			--- bperrybap
 */

int f = 1 - radius;
int ddF_x = 1;
int ddF_y = -2 * radius;
uint8_t x = 0;
uint8_t y = radius;
 
	/*
	 * Fill in the center between the two halves
	 */
	DrawLine(x0, y0-radius, x0, y0+radius, color);
 
	while(x < y)
	{
    // ddF_x == 2 * x + 1;
    // ddF_y == -2 * y;
    // f == x*x + y*y - radius*radius + 2*x - y + 1;
		if(f >= 0) 
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;    

		/*
		 * Now draw vertical lines between the points on the circle rather than
		 * draw the points of the circle. This draws lines between the 
		 * perimeter points on the upper and lower quadrants of the 2 halves of the circle.
		 */

		DrawLine(x0+x, y0+y, x0+x, y0-y, color);
		DrawLine(x0-x, y0+y, x0-x, y0-y, color);
		DrawLine(x0+y, y0+x, y+x0, y0-x, color);
		DrawLine(x0-y, y0+x, x0-y, y0-x, color);
  	}
}

	

//
// Font Functions
//

uint8_t ReadPgmData(const uint8_t* ptr) {  // note this is a static function
	return pgm_read_byte(ptr);
}


void glcd::write(uint8_t c)  // method needed for Print base class
{
  Text.PutChar(c);
} 


// Font and text methods now implemented in the gText class
void glcd::SelectFont(const uint8_t* font, uint8_t color)
{
   Text.SelectFont(font, color);
}

void glcd::SetFontColor(uint8_t color)
{
   Text.SetFontColor(color);
}

void glcd::SetTextMode(textMode mode)
{
  Text.SetTextMode(mode);
}

void glcd::Puts_P(PGM_P str) {
	char c;
	while((c = pgm_read_byte(str)) != 0) {
		write(c);//   was Text.PutChar(c);
		str++;
	}
}

void glcd::EraseTextLine( eraseLine_t type) 
{
   Text.EraseTextLine(type);
} 

void glcd::EraseTextLine( uint8_t row)  
{
     Text.EraseTextLine(row);
}
	
void glcd::CursorTo( uint8_t column, uint8_t row)    // 0 based coordinates for fixed width fonts (i.e. systemFont5x7)
{

	/*
	 * Text position is relative to default text window which is the entire display
	 */
	Text.CursorTo(column, row); 
}

void glcd::CursorToXY( uint8_t x, uint8_t y)
{
	/*
	 * Pixel coordinataes relative to the default text window which is the entire display
	 */
   	Text.CursorToXY(x,y); 
}

uint8_t glcd::CharWidth(char c)
{
   return Text.CharWidth(c);
}

uint16_t glcd::StringWidth(const char* str)
{
  return Text.StringWidth(str);
}

uint16_t glcd::StringWidth_P(PGM_P str)
{
  return Text.StringWidth_P(str);
}
 

// Make one instance for the user
glcd GLCD = glcd();
 