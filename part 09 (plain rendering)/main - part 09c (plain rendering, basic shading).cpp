// Ray casting tutorial by Permadi (see: // https://permadi.com/1996/05/ray-casting-tutorial-4/)
//
// Implementation of part 09 - c: basic rendering with basic lighting, [ part 15 ] horizontal motion, DDA implementation
//
// Joseph21, february 4, 2024
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//

/* Short description
   -----------------
   This implementation series starts with part 9. All previous parts of the Permadi tutorial are either theoretical or a build up to this
   first working implementation. So there are no implementations for previous parts.

   Deviations from the Permadi tutorial:
     * part 3 - I don't use cubes that are 64^3 units. Instead I use cubes that are unit size 1.0f^3
     * part 4 - this means also that the player's height isn't 32, but rather 0.5f.
     *        - the size of the projection plane is controlled by the constants SCREEN_X / _Y and PIXEL_X / _Y. The tutorial uses 320 x 200,
                but the implementation is flexible built around these constants.

   So part 3 upto and including part 8 of the tutorial don't lead to a working implementation, but most of the concepts are put into this code.
   The same holds for the horizontal movement (rotation, forward, backward moving and strafing), which appears in the tutorial as part 15.
   I implemented it in this version because without the ability to move around there's not much fun in testing and experimenting.

   For other raycasting introductions I'd suggest to check on the following video's by JavidX9 (for instance I don't find the explanation of
   the DDA by Permadi very comprehensive):

       FPS part 1 - https://youtu.be/xW8skO7MFYw
       FPS part 2 - https://youtu.be/HEb2akswCcw
       DDA video  - https://youtu.be/NbSee-XM7WA

    Have fun!
 */


#include <cfloat>     // I need FLT_MAX in the DDA function

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.1415926535f

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X   1400
#define SCREEN_Y    800
#define PIXEL_X       1
#define PIXEL_Y       1

// colour constants
#define COL_CEIL    olc::DARK_BLUE
#define COL_FLOOR   olc::DARK_YELLOW
#define COL_WALL    olc::GREY
#define COL_TEXT    olc::MAGENTA

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

    float fDistToProjPlane;         // constant distance to projection plane - is calculated in OnUserCreate()

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

    // Implementation of the DDA algorithm. This function uses class variables for the description of the map.
    // The players position is the "from point", a "to point" is determined using fRayAngle and fMaxDistance.
    // A ray is cast from the "from point" to the "to point". If there is a collision (intersection with wall cell
    // in the map) then the point of intersection, the distance and the map tile of the wall cell is returned in the references.
    bool GetDistanceToWall( float fRayAngle, float &fHitX, float &fHitY, float &fDistIfFound, int &nMapCoordX, int &nMapCoordY ) {

        // The player's position is the "from point"
        float fFromX = fPlayerX;
        float fFromY = fPlayerY;
        // Calculate the "to point" using it's angle and fMaxDistance
        float fToX = fPlayerX + fMaxDistance * cos( fRayAngle * PI / 180.0f );
        float fToY = fPlayerY + fMaxDistance * sin( fRayAngle * PI / 180.0f );
        // work out the direction vector (fDX, fDY) and normalize it
        float fDX = fToX - fFromX;
        float fDY = fToY - fFromY;
        float fRayLen = sqrt( fDX * fDX + fDY * fDY );
        fDX /= fRayLen;
        fDY /= fRayLen;

        // calculate the scaling factors for the ray increments per unit in x resp y direction
        // this calculation takes division by 0.0f into account
        float fSX = (fDX == 0.0f) ? FLT_MAX : sqrt( 1.0f + (fDY / fDX) * (fDY / fDX));
        float fSY = (fDY == 0.0f) ? FLT_MAX : sqrt( 1.0f + (fDX / fDY) * (fDX / fDY));

        float fLengthPartialRayX = 0.0f;
        float fLengthPartialRayY = 0.0f;
        // work out if line is going right or left resp. down or up
        int nGridStepX = (fDX > 0.0f) ? +1 : -1;
        int nGridStepY = (fDY > 0.0f) ? +1 : -1;
        int nCurX = int( fFromX );
        int nCurY = int( fFromY );

        // work out the first intersections with the grid
        if (nGridStepX < 0) { // ray is going left - get scaled difference between start point and left cell border
            fLengthPartialRayX = (fFromX - float( nCurX )) * fSX;
        } else {              // ray is going right - get scaled difference between right cell border and start point
            fLengthPartialRayX = (float( nCurX + 1.0f ) - fFromX) * fSX;
        }
        if (nGridStepY < 0) { // ray is going up - get scaled difference between start point and top cell border
            fLengthPartialRayY = (fFromY - float( nCurY )) * fSY;
        } else {              // ray is going down - get scaled difference between bottom cell border and start point
            fLengthPartialRayY = (float( nCurY + 1.0f ) - fFromY) * fSY;
        }

        // did analysis get out of map boundaries?
        bool bOutOfBounds = (nCurX < 0 || nCurX >= nMapX ||
                             nCurY < 0 || nCurY >= nMapY );
        // was a hit with a wall cell found?
        bool bHitFound = bOutOfBounds ? false : sMap[ nCurY * nMapX + nCurX ] != '.';
        // did analysis reach the destination cell?
        bool bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

        fDistIfFound = 0.0f;
        while (!bOutOfBounds && !bHitFound && !bDestCellFound && fDistIfFound < fMaxDistance) {

            // advance to next map cell, depending on length of partial ray's
            if (fLengthPartialRayX < fLengthPartialRayY) {
                // move in x direction
                nCurX += nGridStepX;
                fDistIfFound = fLengthPartialRayX;
                fLengthPartialRayX += fSX;
            } else {
                // move in y direction
                nCurY += nGridStepY;
                fDistIfFound = fLengthPartialRayY;
                fLengthPartialRayY += fSY;
            }

            bOutOfBounds   = (nCurX < 0 || nCurX >= nMapX ||
                              nCurY < 0 || nCurY >= nMapY );
            bHitFound      = bOutOfBounds ? false : sMap[ nCurY * nMapX + nCurX ] != '.';
            bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));
        }

        if (bHitFound) {
            // return the correct values - note that fDistIfFound already has the correct value
            fHitX = fFromX + fDistIfFound * fDX;
            fHitY = fFromY + fDistIfFound * fDY;
            nMapCoordX  = nCurX;
            nMapCoordY  = nCurY;
        } else {
            // make sure ref. variables don't suggest meaningful values
            fHitX = -1.0f;
            fHitY = -1.0f;
            nMapCoordX  = -1;
            nMapCoordY  = -1;
            fDistIfFound   =  0.0f;
        }

        return bHitFound;
    }

    bool OnUserUpdate( float fElapsedTime ) override {

        // step 1 - user input
        // ===================

        // rotate - collision detection not needed
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }

        float fNewX = fPlayerX;
        float fNewY = fPlayerY;

        // walk forward - CD checked
        if (GetKey( olc::W ).bHeld) {
            fNewX += cos( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
            fNewY += sin( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
        }
        // walk backwards - CD checked
        if (GetKey( olc::S ).bHeld) {
            fNewX -= cos( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
            fNewY -= sin( fPlayerA_deg * PI / 180.0f ) * SPEED_MOVE * fElapsedTime;
        }
        // strafe left - CD checked
        if (GetKey( olc::Q ).bHeld) {
            fNewX += sin( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
            fNewY -= cos( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
        }
        // strafe right - CD checked
        if (GetKey( olc::E ).bHeld) {
            fNewX -= sin( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
            fNewY += cos( fPlayerA_deg * PI / 180.0f ) * SPEED_STRAFE * fElapsedTime;
        }
        // collision detection - check if out of bounds or inside occupied tile
        // only update position if no collision
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

            float fLighting = 1.0f;
            if (GetDistanceToWall( fCurAngle, fX_hit, fY_hit, fRawDistToWall, nX_hit, nY_hit )) {
                // a wall was hit - set bottom and top value depending on the distance found

                // little lambda expression to compare float values
                auto my_float_equal = [=]( float a, float b ) -> bool {
                    float fEpsilon = 0.00001f;
                    return abs( a - b ) < fEpsilon;
                };

                // determine shading factor depending on which face was hit
                if (my_float_equal( fX_hit, float( nX_hit     ))) { fLighting = 1.0f; } else    // west  face was hit
                if (my_float_equal( fY_hit, float( nY_hit     ))) { fLighting = 0.8f; } else    // north
                if (my_float_equal( fX_hit, float( nX_hit + 1 ))) { fLighting = 0.6f; } else    // east
                if (my_float_equal( fY_hit, float( nY_hit + 1 ))) { fLighting = 0.4f; } else {  // south
                    std::cout << "ERROR: OnUserUpdate() --> this situation should not occur" << std::endl;
                    fLighting = 0.2f;
                }

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
                    Draw( x, y, COL_WALL * fLighting );        // render wall
                }
            }
        }

        // output some player values for debugging
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

