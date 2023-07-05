#ifndef RC_SCREEN_H_INCLUDED
#define RC_SCREEN_H_INCLUDED

// ==============================/  screen constants   /==============================

// These are placed in their own header file to enable the user defined map files to use these constants. They could
// be needed in there, for instance in setting the player initial location and orientation (lookup factor).

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X            1000
#define SCREEN_Y             600
#define PIXEL_SIZE             1


#endif // RC_SCREEN_H_INCLUDED
