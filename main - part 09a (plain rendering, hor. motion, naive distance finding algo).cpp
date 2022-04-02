// Ray casting tutorial by Permadi (see: // https://permadi.com/1996/05/ray-casting-tutorial-4/)
//
// Implementation of part 09 - a: basic rendering, [ part 15 ] horizontal motion, "naive" distance finding
//
// Joseph21, march 31, 2022
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//

/* Short description
   -----------------
   This implementation series starts with part 9. All previous parts of the Permadi tutorial are either theoretical or a build up to this
   first working implementation. So there are no implementations for previous parts.

   Deviations from the tutorial:
     * part 3 - I don't use cubes that are 64^3 units. Instead I use cubes that are unit size 1.0f^3
     * part 4 - this means also that the player's height isn't 32, but rather 0.5f.
     *        - the size of the projection plane is controlled by the constants SCREEN_X / _Y and PIXEL_X / _Y. The tutorial uses 320 x 200,
                but the implementation is flexible built around these constants.
     * part 6 - the tutorial describes a DDA type algorithm, however for this implementation a more intuitive (but also naive) approach is used
                for finding the distance to the walls.

   So part 3 upto and including part 8 of the tutorial don't lead to a working implementation, but most of the concepts are put into this code.
   The same holds for the horizontal movement (rotation, forward, backward moving and strafing), which appears in the tutorial as part 15.
   I implemented it in this version because without the ability to move around the map there's not much fun in testing and experimenting.

   For other raycasting introductions I'd suggest to check on the following video's by JavidX9 (for instance I don't find the explanation of
   the DDA by Permadi very comprehensive):

       FPS part 1 - https://youtu.be/xW8skO7MFYw
       FPS part 2 - https://youtu.be/HEb2akswCcw
       DDA video  - https://youtu.be/NbSee-XM7WA

    Have fun!
 */

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.1415926535f

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X    960
#define SCREEN_Y    600
#define PIXEL_X       1
#define PIXEL_Y       1

// colour constants
#define COL_CEIL    olc::DARK_BLUE
#define COL_FLOOR   olc::DARK_YELLOW
#define COL_WALL    olc::GREY
#define COL_TEXT    olc::MAGENTA

// increment value for distance finding function
#define RAY_INCREMENT 0.001f    // larger value = faster, smaller value = more accurate

// constants for speed movements - all movements are modulated with fElapsedTime
#define SPEED_ROTATE      60.0f   //                          60 degrees per second
#define SPEED_MOVE         5.0f   // forward and backward -    5 units per second
#define SPEED_STRAFE       5.0f   // left and right strafing - 5 units per second

class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial - S:(" + std::to_string( SCREEN_X / PIXEL_X ) + ", " + std::to_string( SCREEN_Y / PIXEL_Y ) + ")" +
                                                  ", P:(" + std::to_string(            PIXEL_X ) + ", " + std::to_string(            PIXEL_Y ) + ")" ;
    }

private:
    // definition of the map
    std::string sMap;     // contains char's that define the type of block per map location
    int nMapX = 16;
    int nMapY = 16;

    float fMaxDistance = 25.0f;

    // player: position and looking angle
    float fPlayerX     = 2.0f;
    float fPlayerY     = 2.0f;
    float fPlayerA_deg = 0.0f;    // looking angle is in degrees

    // player: height of eye point and field of view
    float fPlayerH       = 0.5f;
    float fPlayerFoV_deg = 60.0f;   // in degrees !!
    float fDistToProjPlane;

public:
    bool OnUserCreate() override {

        // tile layout of the map - must be of size nMapX x nMapY

        //            0         1
        //            0123456789012345
        sMap.append( "################" );
        sMap.append( "#..............#" );
        sMap.append( "#........####..#" );
        sMap.append( "#..............#" );
        sMap.append( "#...#.....#....#" );
        sMap.append( "#...#..........#" );
        sMap.append( "#...####.......#" );
        sMap.append( "#..............#" );
        sMap.append( "#..............#" );
        sMap.append( "#..............#" );
        sMap.append( "#......##.##...#" );
        sMap.append( "#......#...#...#" );
        sMap.append( "#......#...#...#" );
        sMap.append( "#.......###....#" );
        sMap.append( "#..............#" );
        sMap.append( "################" );

        // work out distance to projection plane. This is a constant depending on the width of the projection plane and the field of view.
        fDistToProjPlane = ((ScreenWidth() / 2.0f) / sin( (fPlayerFoV_deg / 2.0f) * PI / 180.0f)) * cos( (fPlayerFoV_deg / 2.0f) * PI / 180.0f);

        return true;
    }

    // Uses class variables (fPlayerX, fPlayerY) to determine if a wall was hit, and what is the distance in that case.
    // A naive approach is implemented, which increments the ray with tiny increment at a time
    // If there is a collision (intersection with wall cell in the map) then the point of intersection, the distance and
    // the map tile of the wall cell is returned in the references.
    bool GetDistanceToWall( float fRayAngle, float &fHitX, float &fHitY, float &fDistIfFound, int &nMapCoordX, int &nMapCoordY ) {
        // The player's position is the from point.
        float fFromX = fPlayerX;
        float fFromY = fPlayerY;
        // calculate trig values once (since fRayAngle is constant)
        float fCurCos = cos( fRayAngle * PI / 180.0f );
        float fCurSin = sin( fRayAngle * PI / 180.0f );
        // set running variables to initial values
        float fCurX = fFromX;
        float fCurY = fFromY;
        int nCurX = int( fCurX );
        int nCurY = int( fCurY );

        // did analysis get out of map boundaries?
        bool bOutOfBounds = (nCurX < 0 || nCurX >= nMapX ||
                             nCurY < 0 || nCurY >= nMapY );
        // was a hit with a wall cell found?
        bool bHitFound = bOutOfBounds ? false : sMap[ nCurY * nMapX + nCurX ] != '.';

        fDistIfFound = 0.0f;
        while (!bOutOfBounds && !bHitFound && fDistIfFound < fMaxDistance) {

            fDistIfFound += RAY_INCREMENT;
            fCurX = fFromX + fDistIfFound * fCurCos;
            fCurY = fFromY + fDistIfFound * fCurSin;
            nCurX = int( fCurX );
            nCurY = int( fCurY );

            bOutOfBounds   = (nCurX < 0 || nCurX >= nMapX ||
                              nCurY < 0 || nCurY >= nMapY );
            bHitFound      = bOutOfBounds ? false : sMap[ nCurY * nMapX + nCurX ] != '.';
        }

        if (bHitFound) {
            // return the correct values - note that fDistIfFound already has the correct value
            fHitX      = fCurX;
            fHitY      = fCurY;
            nMapCoordX = nCurX;
            nMapCoordY = nCurY;
        } else {
            // make sure ref. variables don't suggest meaningful values
            fHitX        = -1.0f;
            fHitY        = -1.0f;
            nMapCoordX   = -1;
            nMapCoordY   = -1;
            fDistIfFound =  0.0f;
        }

        return bHitFound;
    }

    // ====================/   End of naive implementation   /=========================

    bool OnUserUpdate( float fElapsedTime ) override {

        // step 1 - user input
        // ===================

        // rotate - collision detection not needed
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }

        float fNewX = fPlayerX;
        float fNewY = fPlayerY;

        // walk forward - collision detection checked
        if (GetKey( olc::W ).bHeld) {
            fNewX += cos( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
            fNewY += sin( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
        }
        // walk backwards - collision detection checked
        if (GetKey( olc::S ).bHeld) {
            fNewX -= cos( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
            fNewY -= sin( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
        }
        // strafe left - collision detection checked
        if (GetKey( olc::Q ).bHeld) {
            fNewX += sin( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
            fNewY -= cos( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
        }
        // strafe right - collision detection checked
        if (GetKey( olc::E ).bHeld) {
            fNewX -= sin( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
            fNewY += cos( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
        }
        // collision detection - check if out of bounds or inside occupied tile
        if (fNewX >= 0 && fNewX < nMapX &&
            fNewY >= 0 && fNewY < nMapY &&
            sMap[ int( fNewY ) * nMapX + int( fNewX ) ] != '#') {
            fPlayerX = fNewX;
            fPlayerY = fNewY;
        }

        // step 2 - game logic
        // ===================

        // step 3 - render
        // ===============

        Clear( olc::BLACK );

        int nHalfScreenWidth  = ScreenWidth()  / 2;
        float fAngleStep = fPlayerFoV_deg / float( ScreenWidth() );

        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle = float( x - nHalfScreenWidth ) * fAngleStep;
            float fCurAngle = fPlayerA_deg + fViewAngle;

            float fRawDistToWall, fCorrectDistToWall;
            float fX_hit, fY_hit;   // to hold exact (float) hit location
            int   nX_hit, nY_hit;   // to hold coords of tile that was hit

            int nWallCeil, nWallFloor;   // to store the top and bottom y coord of the wall per column

            if (GetDistanceToWall( fCurAngle, fX_hit, fY_hit, fRawDistToWall, nX_hit, nY_hit )) {
                // a wall was hit - set bottom and top value depending on the distance found

                // make correction for the fish eye effect
                fCorrectDistToWall = fRawDistToWall * cos( fViewAngle * PI / 180.0f );
                int nSliceHeight = int((1.0f / fCorrectDistToWall) * fDistToProjPlane);
                nWallCeil  = (ScreenHeight() / 2.0f) - (nSliceHeight / 2.0f);
                nWallFloor = (ScreenHeight() / 2.0f) + (nSliceHeight / 2.0f);
            } else {
                // no wall was hit - set bottom and top value for wall at the horizon
                nWallCeil  = ScreenHeight() / 2;
                nWallFloor = ScreenHeight() / 2;
            }

            // fill column with pixels
            for (int y = 0; y < ScreenHeight(); y++) {
                if (y < nWallCeil) {
                    Draw( x, y, COL_CEIL );                    // render ceiling
                } else if (y > nWallFloor) {
                    Draw( x, y, COL_FLOOR );                   // render floor
                } else {
                    Draw( x, y, COL_WALL );                    // render wall
                }
            }
        }

        // output player values for debugging
        DrawString( 10, 10, "fPlayerX = " + std::to_string( fPlayerX     ), COL_TEXT );
        DrawString( 10, 20, "fPlayerY = " + std::to_string( fPlayerY     ), COL_TEXT );
        DrawString( 10, 30, "fPlayerA = " + std::to_string( fPlayerA_deg ), COL_TEXT );

        return true;
    }
};

int main()
{
	MyRayCaster demo;
	if (demo.Construct( SCREEN_X / PIXEL_X, SCREEN_Y / PIXEL_Y, PIXEL_X, PIXEL_Y ))
		demo.Start();

	return 0;
}
