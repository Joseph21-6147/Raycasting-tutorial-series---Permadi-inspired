// Ray casting tutorial by Permadi (see: https://permadi.com/1996/05/ray-casting-tutorial-4/)
//
// Implementation of part 16 - vertical motion: looking up & down
//
// Joseph21, april 5, 2022
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, floor and ceiling - use your own .png files and adapt in OnUserCreate()

/* Short description
   -----------------
   This implementation is a follow up of implementation part 14b. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

   This implementation is again a variable height wall version (see part 14a for the details of the variable height coding). So again,
   for this implementation ceiling texturing / rendering is disabled (outcommented), since it doesn't work well in combination with
   variable walls.

   For the look up / down effect, a new class variable fLookUp is defined. It's value represents an offset in pixel space, but for
   smooth movement (and modulation with fElapsedTime) its defined as a float. The value can be changed with the arrow up or down key.

   For the rendering two adaptations were done:
     1. offset the "horizon" (which was constant equal to half the screen height until now) with the fLookUp value.
     2. pass the "horizon" as a parameter to the function to calculate the projected wall bottom and ceiling heights, so that these
        projected heights are offset with fLookUp as well
   This way all rendering code is built upon this "horizon" (the variable is called nHalfScreenHeight in the code), which suffices
   for the looking up / down effect.

   One additional change I made was some error checking on the sprite file. If a sprite file can't be found, an error is reported in
   OnUserCreate(), and the program terminates. This prevents the situation of a running program with a completely black screen.


   Have fun!
 */


#include <cfloat>       // needed for constant FLT_MAX in the DDA function

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.1415926535f

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X    960
#define SCREEN_Y    600
#define PIXEL_X       1
#define PIXEL_Y       1

#define STRETCHED_TEXTURING  false  // if true, multiple levels are stretched textured. If false: per 1x1x1 block

// colour constants
#define COL_CEIL    olc::DARK_BLUE
#define COL_FLOOR   olc::DARK_YELLOW
#define COL_WALL    olc::GREY
#define COL_TEXT    olc::MAGENTA

// constants for speed movements - all movements are modulated with fElapsedTime
#define SPEED_ROTATE      60.0f   //                            60 degrees per second
#define SPEED_MOVE         5.0f   // forward and backward    -   5 units per second
#define SPEED_STRAFE       5.0f   // left and right strafing -   5 units per second
#define SPEED_LOOKUP     200.0f   // looking up or down      - 200 pixels per second

class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial - S:(" + std::to_string( SCREEN_X / PIXEL_X ) + ", " + std::to_string( SCREEN_Y / PIXEL_Y ) + ")" +
                                                  ", P:(" + std::to_string(            PIXEL_X ) + ", " + std::to_string(            PIXEL_Y ) + ")" ;
    }

private:
    // definition of the map
    std::string sMap;     // contains char's that define the type of block per map location
    int        *nMap;     // contains int's  that represent the height per block
    int nMapX = 32;
    int nMapY = 32;

    float fMaxDistance = 40.0f;     // take diagonal distance into account

    // player: position and looking angle
    float fPlayerX     = 2.0f;
    float fPlayerY     = 2.0f;
    float fPlayerA_deg = 0.0f;      // looking angle is in degrees

    // player: height of eye point and field of view
    float fPlayerH       =  0.5f;
    float fPlayerFoV_deg = 60.0f;   // in degrees !!

    // factor for looking up or down - initially 0.0f (in pixel space: float is for smooth movement)
    float fLookUp =  0.0f;
    float fDistToProjPlane;         // constant distance to projection plane - is calculated in OnUserCreate()

    olc::Sprite *pWallSprite  = nullptr;    // these pointers are populated in OnUserCreate()
    olc::Sprite *pFloorSprite = nullptr;
    olc::Sprite *pCeilSprite  = nullptr;

public:

// constants for the different block types
#define GRND_FLOOR '.'     // no block
#define FRST_FLOOR '#'     // block of height 1
#define SCND_FLOOR '@'     //                 2
#define THRD_FLOOR '*'     //                 3
#define FRTH_FLOOR '-'     //                 4
#define FFTH_FLOOR '+'     //                 5
#define SXTH_FLOOR '='     //                 6

    bool OnUserCreate() override {

        bool bSuccess = true;

        // tile layout of the map - must be of size nMapX x nMapY

        //            0         1         2         3
        //            01234567890123456789012345678901
        sMap.append( "*##############################*" );
        sMap.append( "#..............................#" );
        sMap.append( "#........#@*#..................@" );
        sMap.append( "#..................##########..#" );
        sMap.append( "#...#.....#........#....#......@" );
        sMap.append( "#...@..............#.##.##..#..#" );
        sMap.append( "#...*@##..............#...#.#..@" );
        sMap.append( "#..................#..#.....#..#" );
        sMap.append( "#..................##########..@" );
        sMap.append( "#...#..........................#" );
        sMap.append( "#.......*#.#*..................@" );
        sMap.append( "#...@...#...#..................#" );
        sMap.append( "#.......#...#..................@" );
        sMap.append( "#...*....@@@...................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#...-..........................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#...+..........................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#...=..........................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#..............................#" );
        sMap.append( "#..............................@" );
        sMap.append( "***---+++===###..###===+++---***" );
        sMap.append( "#..............................@" );
        sMap.append( "#..............................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#..............................#" );
        sMap.append( "#..............................@" );
        sMap.append( "#..............................#" );
        sMap.append( "#..............................@" );
        sMap.append( "***---+++===###..###===+++---***" );

        // Initialise nMap as a 2d array of ints, having the same size as sMap, and containing the height per cell.
        nMap = new int[nMapX * nMapY];
        for (int y = 0; y < nMapY; y++) {
            for (int x = 0; x < nMapX; x++) {
                switch (sMap[ y * nMapX + x ]) {
                    case GRND_FLOOR: nMap[ y * nMapX + x ] = 0; break;
                    case FRST_FLOOR: nMap[ y * nMapX + x ] = 1; break;
                    case SCND_FLOOR: nMap[ y * nMapX + x ] = 2; break;
                    case THRD_FLOOR: nMap[ y * nMapX + x ] = 3; break;
                    case FRTH_FLOOR: nMap[ y * nMapX + x ] = 4; break;
                    case FFTH_FLOOR: nMap[ y * nMapX + x ] = 5; break;
                    case SXTH_FLOOR: nMap[ y * nMapX + x ] = 6; break;
                }
            }
        }
        // Work out distance to projection plane. This is a constant float value, depending on the width of the projection plane and the field of view.
        fDistToProjPlane = ((ScreenWidth() / 2.0f) / sin( (fPlayerFoV_deg / 2.0f) * PI / 180.0f)) * cos( (fPlayerFoV_deg / 2.0f) * PI / 180.0f);

        // lambda expression for loading sprite files with error checking on the existence of sFileName
        auto load_sprite_file = [=]( const std::string &sFileName ) {
            olc::Sprite *tmp = new olc::Sprite( sFileName );
            if (tmp->width == 0 || tmp->height == 0) {
                std::cout << "ERROR: OnUserCreate() --> can't load file: " << sFileName << std::endl;
                delete tmp;
                tmp = nullptr;
            }
            return tmp;
        };
        // load sprites for texturing walls, floor and ceiling
        std::string sSpritePath = "sprites/";
        pWallSprite  = load_sprite_file( sSpritePath + "wall01 - explicit.png" ); bSuccess &= (pWallSprite  != nullptr);
        pFloorSprite = load_sprite_file( sSpritePath +            "floor2.png" ); bSuccess &= (pFloorSprite != nullptr);
        pCeilSprite  = load_sprite_file( sSpritePath +              "wood.png" ); bSuccess &= (pCeilSprite  != nullptr);

        return bSuccess;
    }

// adaptation of the DDA function to support a list of intersections (instead of just the first one)

    // Holds intersection point in float (world) coordinates and in int (tile) coordinates,
    // the distance to the intersection point and the height of the map at these tile coordinates
    struct IntersectInfo {
        float fHitX,
              fHitY;
        float fDistance;
        int   nMapCoordX,
              nMapCoordY;
        int nHeight;
    };

    // Implementation of the DDA algorithm. This function uses class variables for the description of the map.
    // The players position is the "from point", a "to point" is determined using fRayAngle and fMaxDistance.
    // A ray is cast from the "from point" to the "to point". If there is a collision (intersection with wall cell
    // in the map) then the point of intersection, the distance and the map tile of the wall cell is returned in the references.
    bool GetDistancesToWalls( float fRayAngle, std::vector<IntersectInfo> &vHitList ) {

        // The player's position is the "from point"
        float fFromX = fPlayerX;
        float fFromY = fPlayerY;
        // Calculate the "to point" using the player's angle and fMaxDistance
        float fToX = fPlayerX + fMaxDistance * cos( fRayAngle * PI / 180.0f );
        float fToY = fPlayerY + fMaxDistance * sin( fRayAngle * PI / 180.0f );
        // work out normalized direction vector (fDX, fDY)
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

        // check whether analysis got out of map boundaries
        bool bOutOfBounds = (nCurX < 0 || nCurX >= nMapX ||
                             nCurY < 0 || nCurY >= nMapY );
        // was a hit with a wall cell found?
        bool bHitFound = bOutOfBounds ? false : sMap[ nCurY * nMapX + nCurX ] != '.';
        // did analysis reach the destination cell?
        bool bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

        float fDistIfFound = 0.0f;  // accumulates distance of analysed piece of ray

        while (!bOutOfBounds && !bDestCellFound && fDistIfFound < fMaxDistance) {

            // advance to next map cell, depending on length of partial ray's
            if (fLengthPartialRayX < fLengthPartialRayY) {
                // continue analysis in x direction
                nCurX += nGridStepX;
                fDistIfFound = fLengthPartialRayX;
                fLengthPartialRayX += fSX;
            } else {
                // continue analysis in y direction
                nCurY += nGridStepY;
                fDistIfFound = fLengthPartialRayY;
                fLengthPartialRayY += fSY;
            }

            bOutOfBounds   = (nCurX < 0 || nCurX >= nMapX ||
                              nCurY < 0 || nCurY >= nMapY );
            if (bOutOfBounds) {
                bHitFound      = false;
                bDestCellFound = false;
            } else {
                // check if a collision is found
                bHitFound = sMap[ nCurY * nMapX + nCurX ] != '.';
                bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

                if (bHitFound) {
                    // put the collision values in a new IntersectInfo node and push it up the hit list
                    IntersectInfo sInfo;

                    sInfo.fDistance  = fDistIfFound;
                    sInfo.fHitX      = fFromX + fDistIfFound * fDX;
                    sInfo.fHitY      = fFromY + fDistIfFound * fDY;
                    sInfo.nMapCoordX = nCurX;
                    sInfo.nMapCoordY = nCurY;
                    sInfo.nHeight    = nMap[ nCurY * nMapX + nCurX ];

                    vHitList.push_back( sInfo );
                }
            }
        }

        return (vHitList.size() > 0);
    }

    // This function calculates the y screen coordinates of the bottom and ceiling of a wall slice that has
    // a certain height and is at a certain distance from the player / viewpoint
    void CalculateWallBottomAndTop( float fCorrectedDistToWall, int nHorHeight, int nWallHeight, int &nWallTop, int &nWallBottom ) {
        // calculate slice height for a *unit height* wall
        int nSliceHeight = int((1.0f / fCorrectedDistToWall) * fDistToProjPlane);
        // offset wall slice height from halfway screen height (horizon) - take wall height into account
        nWallTop    = nHorHeight - (nSliceHeight / 2.0f) - (nWallHeight - 1) * nSliceHeight;
        nWallBottom = nHorHeight + (nSliceHeight / 2.0f);
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

        // walking forward, backward and strafing left, right
        float fPlayerA_rad = fPlayerA_deg * PI / 180.0f;
        if (GetKey( olc::W ).bHeld) { fNewX += cos( fPlayerA_rad ) * SPEED_MOVE   * fElapsedTime; fNewY += sin( fPlayerA_rad ) * SPEED_MOVE   * fElapsedTime; }   // walk forward
        if (GetKey( olc::S ).bHeld) { fNewX -= cos( fPlayerA_rad ) * SPEED_MOVE   * fElapsedTime; fNewY -= sin( fPlayerA_rad ) * SPEED_MOVE   * fElapsedTime; }   // walk backwards
        if (GetKey( olc::Q ).bHeld) { fNewX += sin( fPlayerA_rad ) * SPEED_STRAFE * fElapsedTime; fNewY -= cos( fPlayerA_rad ) * SPEED_STRAFE * fElapsedTime; }   // strafe left
        if (GetKey( olc::E ).bHeld) { fNewX -= sin( fPlayerA_rad ) * SPEED_STRAFE * fElapsedTime; fNewY += cos( fPlayerA_rad ) * SPEED_STRAFE * fElapsedTime; }   // strafe right
        // collision detection - check if out of bounds or inside non-empty tile
        // only update position if no collision
        if (fNewX >= 0 && fNewX < nMapX &&
            fNewY >= 0 && fNewY < nMapY &&
                sMap[ int( fNewY ) * nMapX + int( fNewX ) ] == GRND_FLOOR) {
            fPlayerX = fNewX;
            fPlayerY = fNewY;
        }

        // looking up or down - code for part 16 of tutorial (together with introduction of variable fLookUp)
        // NOTE - there's no clamping to extreme values yet
        if (GetKey( olc::UP   ).bHeld) { fLookUp += SPEED_LOOKUP * fElapsedTime; }
        if (GetKey( olc::DOWN ).bHeld) { fLookUp -= SPEED_LOOKUP * fElapsedTime; }

        // step 2 - game logic
        // ===================

        // step 3 - render
        // ===============

        Clear( olc::BLACK );

        int nHalfScreenWidth  = ScreenWidth()  / 2;
        int nHalfScreenHeight = ScreenHeight() / 2 + (int)fLookUp;   // acts as "horizon" value
        float fAngleStep = fPlayerFoV_deg / float( ScreenWidth() );

        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle = float( x - nHalfScreenWidth ) * fAngleStep;
            float fCurAngle = fPlayerA_deg + fViewAngle;

            float fRawDistToWall, fCorrectDistToWall;
            float fX_hit, fY_hit;   // to hold exact (float) hit location
            int   nX_hit, nY_hit;   // to hold coords of tile that was hit

            int nWallCeil, nWallFloor;   // to store the top and bottom y coord of the wall per column

            // this lambda returns a sample of the ceiling through the pixel at screen coord (px, py)
            auto get_ceil_sample = [=]( int px, int py ) -> olc::Pixel {
                // work out the distance to the location on the ceiling you are looking at through this pixel
                // (the pixel is given since you know the x and y screen coordinate to draw to)
                float fCeilProjDistance = ((fPlayerH / float( nHalfScreenHeight - py )) * fDistToProjPlane) / cos( fViewAngle * PI / 180.0f );
                // calculate the world ceiling coordinate from the player's position, the distance and the view angle + player angle
                float fCeilProjX = fPlayerX + fCeilProjDistance * cos( fCurAngle * PI / 180.0f );
                float fCeilProjY = fPlayerY + fCeilProjDistance * sin( fCurAngle * PI / 180.0f );
                // calculate the sample coordinates for that world ceiling coordinate, by subtracting the
                // integer part and only keeping the fractional part
                float fSampleX = fCeilProjX - int(fCeilProjX);
                float fSampleY = fCeilProjY - int(fCeilProjY);
                // having both sample coordinates, get the sample and draw the pixel
                return pCeilSprite->Sample( fSampleX, fSampleY );
            };

            // this lambda returns a sample of the floor through the pixel at screen coord (px, py)
            auto get_floor_sample = [=]( int px, int py ) -> olc::Pixel {
                // work out the distance to the location on the floor you are looking at through this pixel
                // (the pixel is given since you know the x and y to draw to)
                float fFloorProjDistance = ((fPlayerH / float( py - nHalfScreenHeight )) * fDistToProjPlane) / cos( fViewAngle * PI / 180.0f );
                // calculate the world floor coordinate from the distance and the view angle + player angle
                float fFloorProjX = fPlayerX + fFloorProjDistance * cos( fCurAngle * PI / 180.0f );
                float fFloorProjY = fPlayerY + fFloorProjDistance * sin( fCurAngle * PI / 180.0f );
                // calculate the sample coordinates for that world floor coordinate, by subtracting the
                // integer part and only keeping the fractional part
                float fSampleX = fFloorProjX - int(fFloorProjX);
                float fSampleY = fFloorProjY - int(fFloorProjY);
                // having both sample coordinates, get the sample and draw the pixel
                return pFloorSprite->Sample( fSampleX, fSampleY );
            };

            // prepare the rendering for this screen slice by calculating the list of intersections in this direction
            std::vector<IntersectInfo> vColHitlist;
            int nColHeight = 1;
            if (GetDistancesToWalls( fCurAngle, vColHitlist )) {
                // a wall was hit - set bottom and top value depending on the distance found

                // get the info from first hit point
                fX_hit         = vColHitlist[0].fHitX;
                fY_hit         = vColHitlist[0].fHitY;
                nX_hit         = vColHitlist[0].nMapCoordX;
                nY_hit         = vColHitlist[0].nMapCoordY;
                fRawDistToWall = vColHitlist[0].fDistance;
                nColHeight     = vColHitlist[0].nHeight;

                // make correction for the fish eye effect
                fCorrectDistToWall = fRawDistToWall * cos( fViewAngle * PI / 180.0f );
                CalculateWallBottomAndTop( fCorrectDistToWall, nHalfScreenHeight, nColHeight, nWallCeil, nWallFloor );
            } else {
                // no wall was hit - set bottom and top value for wall at the horizon
                nWallCeil  = nHalfScreenHeight;
                nWallFloor = nHalfScreenHeight;
            }

            // now render this slice using the info of the hit list
            int nHitListIndex = 0;
            // note that we are working upwards
            for (int y = ScreenHeight() - 1; y >= 0; y--) {

// constants for different types of rendering
#define UNKNOWN_DRAWING 0
#define   FLOOR_DRAWING 1
#define    WALL_DRAWING 2
#define    CEIL_DRAWING 3

                // determine what type of segment is rendered: floor, wall or ceiling
                int nDrawMode = UNKNOWN_DRAWING;
                if (y >= nWallFloor) {
                    nDrawMode = FLOOR_DRAWING;
                } else if (nWallFloor > y && y > nWallCeil) {
                    nDrawMode = WALL_DRAWING;
                } else if (y <= nWallCeil) {
                    while (nDrawMode == UNKNOWN_DRAWING) {
                        if (nHitListIndex < (int)vColHitlist.size() - 1) {
                            // the y coord is above the current wall and roof slide, but there are still hit points to process
                            // so there could be other walls behind current wall sticking out above it
                            nHitListIndex += 1;

                            // get the info from next hit point
                            fX_hit         = vColHitlist[ nHitListIndex ].fHitX;
                            fY_hit         = vColHitlist[ nHitListIndex ].fHitY;
                            nX_hit         = vColHitlist[ nHitListIndex ].nMapCoordX;
                            nY_hit         = vColHitlist[ nHitListIndex ].nMapCoordY;
                            fRawDistToWall = vColHitlist[ nHitListIndex ].fDistance;
                            nColHeight     = vColHitlist[ nHitListIndex ].nHeight;

                            // make correction for the fish eye effect
                            fCorrectDistToWall = fRawDistToWall * cos( fViewAngle * PI / 180.0f );
                            // get new values for nWallFloor and nWallCeil
                            int nCacheWallCeil = nWallCeil;
                            CalculateWallBottomAndTop( fCorrectDistToWall, nHalfScreenHeight, nColHeight, nWallCeil, nWallFloor );

                            // use this intersection point only if the ceiling of its wall is higher than the previous wall segment.
                            if (nWallCeil < nCacheWallCeil)
                                nDrawMode = WALL_DRAWING;
                        } else {
                            nDrawMode = CEIL_DRAWING;
                        }
                    }
                } else {
                	// this code is probably obsolete by now
                    std::cout << "ERROR: OnUserUpdate() --> can't position y value as floor, wall or ceiling..." << std::endl;
                }

                // now we know what type of segment we're working on, render it
                switch (nDrawMode) {
                    case CEIL_DRAWING: {                         // ========== render ceiling ====================
                        // ceiling texturing doesn't work correctly with variable heights walls
//                            olc::Pixel auxSample = get_ceil_sample( x, y );
//                            Draw( x, y, auxSample );
                        }
                        break;
                    case FLOOR_DRAWING: {                        // ========== render floor   ====================
                            olc::Pixel auxSample = get_floor_sample( x, y );
                            Draw( x, y, auxSample );
                        }
                        break;
                    case WALL_DRAWING: {                         // ========== render wall    ====================

                            float fSampleY;
                            if (STRETCHED_TEXTURING) {
                                // original sampling = stretched over full height of wall
                                // the y sample coordinate depends only on the pixel y coord on the screen
                                // in relation to the vertical space the wall is taking up
                                fSampleY = float(y - nWallCeil) / float(nWallFloor - nWallCeil);
                            } else {
                                // sophisticated sampling = sampling per unit block size
                                float fBlockProjHeight = float( nWallFloor - nWallCeil ) / float( nColHeight );
                                float fRelativeY = float( y - nWallCeil );
                                while (fRelativeY > fBlockProjHeight)
                                    fRelativeY -= fBlockProjHeight;
                                fSampleY = fRelativeY / fBlockProjHeight;
                            }

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
                        break;
                    default:
                        // this code is probably obsolete by now
                        std::cout << "ERROR: OnUserUpdate() --> unknown draw mode encountered: " << nDrawMode << std::endl;
                        // draw a yellow pixel for debugging
                        olc::Pixel auxSample = olc::YELLOW;
                        Draw( x, y, auxSample );
                        break;
                }
            }
        }

        // output some player values for debugging
        DrawString( 10, 10, "fPlayerX = " + std::to_string( fPlayerX     ), COL_TEXT );
        DrawString( 10, 20, "fPlayerY = " + std::to_string( fPlayerY     ), COL_TEXT );
        DrawString( 10, 30, "fPlayerA = " + std::to_string( fPlayerA_deg ), COL_TEXT );
        DrawString( 10, 50, "fLookUp  = " + std::to_string( fLookUp      ), COL_TEXT );

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

