// Ray casting tutorial by Permadi (see: // https://permadi.com/1996/05/ray-casting-tutorial-4/)
//
// Implementation of part 13 (textured ceiling)
//
// Joseph21, march 31, 2022
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, floor and ceiling - use your own .png files and adapt in OnUserCreate()

/* Short description
   -----------------
   This implementation is the follow up of implementation part 12. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

   The ceiling texturing is a pretty straightforward variation on the floor texturing.

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
#define SCREEN_X    960
#define SCREEN_Y    600
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

    olc::Sprite *pWallSprite  = nullptr;    // these pointers are populated in OnUserCreate()
    olc::Sprite *pFloorSprite = nullptr;
    olc::Sprite *pCeilSprite  = nullptr;

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

        // load sprites for texturing walls, floor and ceiling
        std::string sSpritePath = "sprites/";
        pWallSprite  = new olc::Sprite( sSpritePath + "wall01.png" );
        pFloorSprite = new olc::Sprite( sSpritePath + "floor3.png" );
        pCeilSprite  = new olc::Sprite( sSpritePath + "wood.png" );

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
            nMapCoordX = nCurX;
            nMapCoordY = nCurY;
        } else {
            // make sure ref. variables don't suggest meaningful values
            fHitX = -1.0f;
            fHitY = -1.0f;
            nMapCoordX = -1;
            nMapCoordY = -1;
            fDistIfFound = 0.0f;
        }

        return bHitFound;
    }

    bool OnUserUpdate( float fElapsedTime ) override {

        // step 1 - user input
        // ===================

        // Rotate - collision detection not necessary. Keep fPlayerA_deg between 0 and 360 degrees
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }

        // variables used for collision detection - work out the new location in a seperate coordinate pair, and only alter
        // the players coordinate if there's no collision
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
        int nHalfScreenHeight = ScreenHeight() / 2;
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
                if (y < nWallCeil) {                           // ========== render ceiling ====================

                    // work out the distance to the location on the ceiling you are looking at through this pixel
                    // (the pixel is given since you know the x and y screen coordinate to draw to)
                float fCeilProjDistance = ((fPlayerH / float( nHalfScreenHeight - y )) * fDistToProjPlane) / cos( fViewAngle * PI / 180.0f );
                // calculate the world ceiling coordinate from the player's position, the distance and the view angle + player angle
                    float fCeilProjX = fPlayerX + fCeilProjDistance * cos( fCurAngle * PI / 180.0f );
                    float fCeilProjY = fPlayerY + fCeilProjDistance * sin( fCurAngle * PI / 180.0f );
                    // calculate the sample coordinates for that world ceiling coordinate, by subtracting the
                    // integer part and only keeping the fractional part
                    float fSampleX = fCeilProjX - int(fCeilProjX);
                    float fSampleY = fCeilProjY - int(fCeilProjY);
                    // having both sample coordinates, get the sample and draw the pixel
                    olc::Pixel auxSample = pCeilSprite->Sample( fSampleX, fSampleY );
                    Draw( x, y, auxSample );

                } else if (y > nWallFloor) {                   // ========== render floor   ====================

                    // work out the distance to the location on the floor you are looking at through this pixel
                    // (the pixel is given since you know the x and y to draw to)
                    float fFloorProjDistance = ((fPlayerH / float( y - nHalfScreenHeight )) * fDistToProjPlane) / cos( fViewAngle * PI / 180.0f );
                // calculate the world floor coordinate from the distance and the view angle + player angle
                    float fFloorProjX = fPlayerX + fFloorProjDistance * cos( fCurAngle * PI / 180.0f );
                    float fFloorProjY = fPlayerY + fFloorProjDistance * sin( fCurAngle * PI / 180.0f );
                    // calculate the sample coordinates for that world floor coordinate, by subtracting the
                    // integer part and only keeping the fractional part
                    float fSampleX = fFloorProjX - int(fFloorProjX);
                    float fSampleY = fFloorProjY - int(fFloorProjY);
                    // having both sample coordinates, get the sample and draw the pixel
                    olc::Pixel auxSample = pFloorSprite->Sample( fSampleX, fSampleY );
                    Draw( x, y, auxSample );

                } else {                                       // ========== render wall    ====================

                    // the y sample coordinate depends only on the pixel y coord on the screen
                    // in relation to the vertical space the wall is taking up
                    float fSampleY = float(y - nWallCeil) / float(nWallFloor - nWallCeil);

                    // the x sample coordinate takes more work to figure out. You need to check what side of the
                    // block was hit
                    float fSampleX;
                    float fBlockMidX = (float)nX_hit + 0.5f;   // location of middle of the cell that was hit
                    float fBlockMidY = (float)nY_hit + 0.5f;
                    // determine in what quadrant the hit location is wrt the block mid point
                    float fTestAngle = atan2f((fY_hit - fBlockMidY), (fX_hit - fBlockMidX));
                    if (-0.75f * PI <= fTestAngle && fTestAngle <  -0.25f * PI) fSampleX = fX_hit - (float)nX_hit;  // south side
                    if (-0.25f * PI <= fTestAngle && fTestAngle <   0.25f * PI) fSampleX = fY_hit - (float)nY_hit;  // east  side
                    if ( 0.25f * PI <= fTestAngle && fTestAngle <   0.75f * PI) fSampleX = fX_hit - (float)nX_hit;  // north side
                    if (-0.75f * PI >  fTestAngle || fTestAngle >=  0.75f * PI) fSampleX = fY_hit - (float)nY_hit;  // west  side

                    // having both sample coordinates, get the sample and draw the pixel
                    olc::Pixel auxSample = pWallSprite->Sample( fSampleX, fSampleY );
                    Draw( x, y, auxSample );
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


