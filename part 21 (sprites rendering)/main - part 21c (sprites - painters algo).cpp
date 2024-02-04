// ELABORATING ON - Ray casting tutorial by Permadi
// (starting with part 20 all code files are my own elaboration on the Permadi basis)
//
// Implementation of part 21 c - introducing sprites (billboards): added painters algorithm for object rendering
//
// Joseph21, april 18, 2023
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, roofs, floor and ceiling - use your own .png files and adapt in OnUserCreate()

/* Short description
   -----------------
   This implementation is a follow up of implementation part 21 b. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

     * Added a scale to objects, and adapted the rendering code to take the scale into account
     * Added distance and angle field to object struct to support painters algorithm on objects

   Have fun!
 */


#include <cfloat>       // needed for constant FLT_MAX in the DDA function

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.1415926535f

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X   1200
#define SCREEN_Y    720
#define PIXEL_X       1
#define PIXEL_Y       1


#define STRETCHED_TEXTURING  false  // if true, multiple levels are stretched textured. If false: per 1x1x1 block
#define MULTIPLE_LEVELS      true
#define RENDER_CEILING       !MULTIPLE_LEVELS    // render ceilings only for single level world

#define MOUSE_CONTROL        false

// shading constants
#define RENDER_SHADED        true
#define OBJECT_INTENSITY       1.5f
#define MULTIPLIER_INTENSITY   5.0f
#define INTENSITY_SPEED        1.0f

#define SHADE_FACTOR_MIN  0.1f      // the shade factor is clamped between these two values
#define SHADE_FACTOR_MAX  1.0f

// colour constants
#define ROOF_COLOUR  olc::RED
#define TEXT_COLOUR  olc::YELLOW
#define SHADE_COLOUR olc::BLACK    // white for fog effect, black for night effect

// constants for speed movements - all movements are modulated with fElapsedTime
#define SPEED_ROTATE      60.0f   //                            60 degrees per second
#define SPEED_MOVE         5.0f   // forward and backward    -   5 units per second
#define SPEED_STRAFE       5.0f   // left and right strafing -   5 units per second
#define SPEED_LOOKUP     200.0f   // looking up or down      - 200 pixels per second
#define SPEED_STRAFE_UP    1.0f   // flying or chroucing     -   1.0 block per second

// mini map constants
#define MINIMAP_TILE_SIZE     32
#define MINIMAP_SCALE_FACTOR   0.2   // should be 0.2

#define SIGNIFICANCE 3
#define SIG_POW10    1000

// ==============================/  convenience functions for angles  /==============================

float deg2rad( float fAngleDeg ) { return fAngleDeg * PI / 180.0f; }
float rad2deg( float fAngleRad ) { return fAngleRad / PI * 180.0f; }
float deg_mod2pi( float fAngleDeg ) {
    while (fAngleDeg <    0.0f) fAngleDeg += 360.0f;
    while (fAngleDeg >= 360.0f) fAngleDeg -= 360.0f;
    return fAngleDeg;
}
float rad_mod2pi( float fAngleRad ) {
    while (fAngleRad <  0.0f     ) fAngleRad += 2.0f * PI;
    while (fAngleRad >= 2.0f * PI) fAngleRad -= 2.0f * PI;
    return fAngleRad;
}

// ==============================/  lookup sine and cosine functions  /==============================

float lu_sin_array[360 * SIG_POW10];
float lu_cos_array[360 * SIG_POW10];

void init_lu_sin_array() {
    for (int i = 0; i < 360; i++) {
        for (int j = 0; j < SIG_POW10; j++) {
            int nIndex = i * SIG_POW10 + j;
            float fArg_deg = float( nIndex ) / float( SIG_POW10 );
            lu_sin_array[ nIndex ] = sinf( deg2rad( fArg_deg ));
        }
    }
}

void init_lu_cos_array() {
    for (int i = 0; i < 360; i++) {
        for (int j = 0; j < SIG_POW10; j++) {
            int nIndex = i * SIG_POW10 + j;
            float fArg_deg = float( nIndex ) / float( SIG_POW10 );
            lu_cos_array[ nIndex ] = cosf( deg2rad( fArg_deg ));
        }
    }
}

float lu_sin( float fDegreeAngle ) {
    fDegreeAngle = deg_mod2pi( fDegreeAngle );
    int nWholeNr = int( fDegreeAngle );
    int nRemainder = int( (fDegreeAngle - nWholeNr) * float( SIG_POW10 ));
    int nIndex = nWholeNr * SIG_POW10 + nRemainder;
    return lu_sin_array[ nIndex ];
}

float lu_cos( float fDegreeAngle ) {
    fDegreeAngle = deg_mod2pi( fDegreeAngle );
    int nWholeNr = int( fDegreeAngle );
    int nRemainder = int( (fDegreeAngle - nWholeNr) * float( SIG_POW10 ));
    int nIndex = nWholeNr * SIG_POW10 + nRemainder;
    return lu_cos_array[ nIndex ];
}


// ==============================/  Start of PGE derived class   /==============================


class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial - S:(" + std::to_string( SCREEN_X / PIXEL_X ) + ", " + std::to_string( SCREEN_Y / PIXEL_Y ) + ")" +
                                                  ", P:(" + std::to_string(            PIXEL_X ) + ", " + std::to_string(            PIXEL_Y ) + ")" ;
    }

private:
    // definition of the map
    std::string sMap;     // contains chars that define the type of block per map location
    float      *fMap;     // contains floats that represent the height per block
    int nMapX = 32;
    int nMapY = 32;

    // max visible distance - use length of map diagonal to overlook whole map
    float fMaxDistance = sqrt( nMapX * nMapX + nMapY * nMapY );

    // player: position and looking angle
    float fPlayerX     = 2.5f;
    float fPlayerY     = 2.5f;
    float fPlayerA_deg = 0.0f;      // looking angle is in degrees, 0.0f is EAST

    // player: height of eye point and field of view
    float fPlayerH       =  0.5f;
    float fPlayerFoV_deg = 60.0f;   // in degrees !!

    // factor for looking up or down - initially 0.0f (in pixel space: float is for smooth movement)
    float fLookUp        =  0.0f;
    float fDistToProjPlane;         // constant distance to projection plane - is calculated in OnUserCreate()

    olc::Sprite *pWallSprite  = nullptr;    // these pointers are populated in OnUserCreate()
    olc::Sprite *pFloorSprite = nullptr;
    olc::Sprite *pCeilSprite  = nullptr;
    olc::Sprite *pRoofSprite  = nullptr;

    olc::Sprite *pObjectSprite = nullptr;

    bool bMouseControl = MOUSE_CONTROL;     // toggle on mouse control (trigger key M)

    // var's and initial values for shading
    float fObjectIntensity     = MULTIPLE_LEVELS ? OBJECT_INTENSITY     :  0.2f;
    float fIntensityMultiplier = MULTIPLE_LEVELS ? MULTIPLIER_INTENSITY : 10.0f;

    // toggles for rendering
    bool bMinimap   = false;    // toggle on mini map rendering (trigger key P)
    bool bMapRays   = false;    //                              (trigger key O)
    bool bDebugInfo = false;    //                              (trigger key I)

    std::vector<olc::vf2d> vRayList;    // needed for ray rendering in minimap

    struct sObject {
        float x, y;
        float scale;
        olc::Sprite *sprite;
        float distance, angle;
    };
    std::list<sObject> vListObjects;

    float *fDepthBuffer = nullptr;

public:

// constants for the different block types
#define GRND_FLOOR '.'     // no block
#define FRST_FLOOR '#'     // block of height 1
#define SCND_FLOOR '@'     //                 2
#define THRD_FLOOR '*'     //                 3
#define FRTH_FLOOR '-'     //                 4
#define FFTH_FLOOR '+'     //                 5
#define SXTH_FLOOR '='     //                 6

#define FLOOR_1QRTR 'Q'    // block of height 1/4
#define FLOOR_HALVE 'H'    //                 2/4
#define FLOOR_3QRTR 'T'    //                 3/4


    bool OnUserCreate() override {

        bool bSuccess = true;

        // tile layout of the map - must be of size nMapX x nMapY

        //            0         1         2         3
        //            01234567890123456789012345678901
        sMap.append( "............###................." );
        sMap.append( ".*#########################....#" );
        sMap.append( ".#............................##" );
        sMap.append( ".#..............Q.H.T.#.......@." );
        sMap.append( ".#............................@." );
        sMap.append( ".#............................@." );
        sMap.append( ".#...................Q........@." );
        sMap.append( ".#...................H........@." );
        sMap.append( ".#...................T........@." );
        sMap.append( ".#...................#........@." );
        sMap.append( ".#...................T........@." );
        sMap.append( ".#.......*#.#*.......H........@." );
        sMap.append( ".#...@...#...#.......Q........#." );
        sMap.append( ".#.......#...#................@." );
        sMap.append( ".#...*....@@@.................#." );
        sMap.append( ".#............................@." );
        sMap.append( ".#...-..........1.............#." );
        sMap.append( ".#...............2............@." );
        sMap.append( ".#...+............3...........#." );
        sMap.append( ".#.................4..........@." );
        sMap.append( ".#...=..............5.........#." );
        sMap.append( ".#.........1234......6........@." );
        sMap.append( ".#............5.......7.......#." );
        sMap.append( ".#.........9876........8......@." );
        sMap.append( ".#......................9.....@." );
        sMap.append( ".#.......................#....@." );
        sMap.append( "..............................@." );
        sMap.append( "..#@*-+++===#@*.*@#===+++---***." );
        sMap.append( "..............#.#..............." );
        sMap.append( "................................" );
        sMap.append( "................................" );
        sMap.append( "................................" );

        // Initialise fMap as a 2d array of floats, having the same size as sMap, and containing the height per cell.
        // NOTE - if MULTIPLE_LEVELS is false, the fMap will contain no values > 1
        fMap = new float[nMapX * nMapY];
        for (int y = 0; y < nMapY; y++) {
            for (int x = 0; x < nMapX; x++) {
                switch (sMap[ y * nMapX + x ]) {
                    case GRND_FLOOR: fMap[ y * nMapX + x ] =                          0.0f; break;
                    case FRST_FLOOR: fMap[ y * nMapX + x ] =                          1.0f; break;
                    case SCND_FLOOR: fMap[ y * nMapX + x ] = MULTIPLE_LEVELS ? 2.0f : 1.0f; break;
                    case THRD_FLOOR: fMap[ y * nMapX + x ] = MULTIPLE_LEVELS ? 3.0f : 1.0f; break;
                    case FRTH_FLOOR: fMap[ y * nMapX + x ] = MULTIPLE_LEVELS ? 4.0f : 1.0f; break;
                    case FFTH_FLOOR: fMap[ y * nMapX + x ] = MULTIPLE_LEVELS ? 5.0f : 1.0f; break;
                    case SXTH_FLOOR: fMap[ y * nMapX + x ] = MULTIPLE_LEVELS ? 6.0f : 1.0f; break;

                    case FLOOR_1QRTR: fMap[ y * nMapX + x ] = 0.25f; break;
                    case FLOOR_HALVE: fMap[ y * nMapX + x ] = 0.50f; break;
                    case FLOOR_3QRTR: fMap[ y * nMapX + x ] = 0.75f; break;

                    case         '1': fMap[ y * nMapX + x ] = 0.10f; break;
                    case         '2': fMap[ y * nMapX + x ] = 0.20f; break;
                    case         '3': fMap[ y * nMapX + x ] = 0.30f; break;
                    case         '4': fMap[ y * nMapX + x ] = 0.40f; break;
                    case         '5': fMap[ y * nMapX + x ] = 0.50f; break;
                    case         '6': fMap[ y * nMapX + x ] = 0.60f; break;
                    case         '7': fMap[ y * nMapX + x ] = 0.70f; break;
                    case         '8': fMap[ y * nMapX + x ] = 0.80f; break;
                    case         '9': fMap[ y * nMapX + x ] = 0.90f; break;
                }
            }
        }

        // initialize sine and cosine lookup arrays
        init_lu_sin_array();
        init_lu_cos_array();

        // Work out distance to projection plane. This is a constant float value, depending on the width of the projection plane and the field of view.
        fDistToProjPlane = ((ScreenWidth() / 2.0f) / lu_sin( fPlayerFoV_deg / 2.0f )) * lu_cos( fPlayerFoV_deg / 2.0f );

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
        std::string sSpritePath = "../sprites/";
        pWallSprite   = load_sprite_file( sSpritePath +    "new wall_brd.png" ); bSuccess &= (pWallSprite   != nullptr);
        pFloorSprite  = load_sprite_file( sSpritePath +   "grass_texture.png" ); bSuccess &= (pFloorSprite  != nullptr);
        pCeilSprite   = load_sprite_file( sSpritePath + "ceiling_texture.png" ); bSuccess &= (pCeilSprite   != nullptr);
        pRoofSprite   = load_sprite_file( sSpritePath +    "roof texture.png" ); bSuccess &= (pRoofSprite   != nullptr);

        // load sprites for rendering objects
        pObjectSprite = load_sprite_file(              "tree 100x100.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);

        // Initialize depth buffer
        fDepthBuffer = new float[ ScreenWidth() ];

        // populate object list with static initializer list
        vListObjects = {
            { 10.5f,  4.5f, 1.2f, pObjectSprite, -1.0f, 0.0f },
            { 14.5f,  5.5f, 1.0f, pObjectSprite, -1.0f, 0.0f },
            { 14.5f,  6.5f, 1.5f, pObjectSprite, -1.0f, 0.0f },
            { 14.5f,  7.5f, 2.0f, pObjectSprite, -1.0f, 0.0f },
            {  6.5f,  9.5f, 3.0f, pObjectSprite, -1.0f, 0.0f },
        };

        return bSuccess;
    }

// adaptation of the DDA function to support a list of intersections (instead of just the first one)
// as well as the info needed to render roofs (top faces of walls)

    // Holds intersection point in float (world) coordinates and in int (tile) coordinates,
    // the distance to the intersection point and the height of the map at these tile coordinates
    struct IntersectInfo {
        float fHitX,
              fHitY;
        float fDistance;
        int   nMapCoordX,
              nMapCoordY;
        float fHeight;

        // Adaptation to support multilevel flying and crouching
        // these are y coordinate (projected) values
        int bottom_front;   // on screen projected bottom  of wall slice
        int   ceil_front;   //                     ceiling
        int   ceil_back;    //                     ceiling of wall at back
    };

    // Implementation of the DDA algorithm. This function uses class variables for the description of the map.
    // The players position is the "from point", a "to point" is determined using fRayAngle and fMaxDistance.
    // A ray is cast from the "from point" to the "to point". If there is a collision (intersection with a
    // change in height in the map) then the point of intersection, the distance and the map tile of the
    // wall cell is put into the hit list.
    bool GetDistancesToWalls( float fRayAngle, std::vector<IntersectInfo> &vHitList ) {

        // The player's position is the "from point"
        float fFromX = fPlayerX;
        float fFromY = fPlayerY;
        // Calculate the "to point" using the player's angle and fMaxDistance
        float fToX = fPlayerX + fMaxDistance * lu_cos( fRayAngle );
        float fToY = fPlayerY + fMaxDistance * lu_sin( fRayAngle );
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

        // work out if line is going right or left resp. down or up
        int nGridStepX = (fDX > 0.0f) ? +1 : -1;
        int nGridStepY = (fDY > 0.0f) ? +1 : -1;

        float fLengthPartialRayX = 0.0f;
        float fLengthPartialRayY = 0.0f;

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
        // did analysis reach the destination cell?
        bool bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

        float fDistIfFound = 0.0f;  // accumulates distance of analysed piece of ray
        float fCurHeight   = 0.0f;  // to check on differences in height

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

            bOutOfBounds = (nCurX < 0 || nCurX >= nMapX ||
                            nCurY < 0 || nCurY >= nMapY );
            if (bOutOfBounds) {
                bDestCellFound = false;

                // If out of bounds, finalize the list with one additional intersection with the map boundary and height 0.
                // (only if the list is not empty!) This additional intersection record is necessary for proper rendering at map boundaries.
                if (fCurHeight != 0.0f && !vHitList.empty()) {

                    fCurHeight = 0.0f;  // since we're out of bounds

                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    IntersectInfo sInfo;
                    sInfo.fDistance  = fDistIfFound;
                    sInfo.fHitX      = fFromX + fDistIfFound * fDX;
                    sInfo.fHitY      = fFromY + fDistIfFound * fDY;
                    sInfo.nMapCoordX = nCurX;
                    sInfo.nMapCoordY = nCurY;
                    sInfo.fHeight    = fCurHeight;
                    vHitList.push_back( sInfo );
                }
            } else {
                // check if there's a difference in height found
                bool bHitFound = (fMap[ nCurY * nMapX + nCurX ] != fCurHeight);
                bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

                if (bHitFound) {

                    // reset current height to new value
                    fCurHeight = fMap[ nCurY * nMapX + nCurX ];

                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    IntersectInfo sInfo;
                    sInfo.fDistance  = fDistIfFound;
                    sInfo.fHitX      = fFromX + fDistIfFound * fDX;
                    sInfo.fHitY      = fFromY + fDistIfFound * fDY;
                    sInfo.nMapCoordX = nCurX;
                    sInfo.nMapCoordY = nCurY;
                    sInfo.fHeight    = fCurHeight;
                    vHitList.push_back( sInfo );
                }
            }
        }

        return (vHitList.size() > 0);
    }

// ==============================/   Mini map rendering stuff   /==============================

    // function to render the mini map on the screen
    void RenderMapGrid() {
        // fill background for minimap
        float fMMFactor = MINIMAP_SCALE_FACTOR * MINIMAP_TILE_SIZE;
        FillRect( 0, 0, nMapX * fMMFactor, nMapY * fMMFactor, olc::VERY_DARK_GREEN );
        // draw each tile
        for (int y = 0; y < nMapY; y++) {
            for (int x = 0; x < nMapX; x++) {
                // colour different for different heights
                olc::Pixel p;
                bool bBorderFlag = true;
                if (fMap[ y * nMapX + x ] == 0.0f) {
                    p = olc::VERY_DARK_GREEN;   // don't visibly render
                    bBorderFlag = false;
                } else if (fMap[ y * nMapX + x ] <  1.0f) {
                    p = olc::PixelF( fMap[ y * nMapX + x], 0.0f, 0.0f );    // height < 1.0f = shades of red
                } else {
                    float fColFactor = std::min( fMap[ y * nMapX + x] / 4.0f + 0.5f, 1.0f );    // heights > 1.0f = shades of blue
                    p = olc::PixelF( 0.0f, 0.0f, fColFactor );
                }
                // render this tile
                FillRect( x * fMMFactor + 1, y * fMMFactor + 1, fMMFactor - 1, fMMFactor - 1, p );
                if (bBorderFlag) {
                    p = olc::WHITE;
                    DrawRect( x * fMMFactor, y * fMMFactor, fMMFactor, fMMFactor, p);
                }
            }
        }
    }

    // function to render the player in the mini map on the screen
    void RenderMapPlayer() {
        float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
        olc::Pixel p = olc::YELLOW;
        float px = fPlayerX * fMMFactor;
        float py = fPlayerY * fMMFactor;
        float pr = 0.6f     * fMMFactor;
        FillCircle( px, py, pr, p );
        float dx = lu_cos( fPlayerA_deg );
        float dy = lu_sin( fPlayerA_deg );
        float pdx = dx * 2.0f * fMMFactor;
        float pdy = dy * 2.0f * fMMFactor;
        DrawLine( px, py, px + pdx, py + pdy, p );
    }

    // function to render the rays in the mini map on the screen
    void RenderMapRays() {
        float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
        for (auto &elt : vRayList) {
            DrawLine(
                fPlayerX * fMMFactor,
                fPlayerY * fMMFactor,
                elt.x * fMMFactor,
                elt.y * fMMFactor,
                olc::GREEN
            );
        }
    }

    // function to render all the objects in the mini map on the screen
    void RenderMapObjects() {
        float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
        olc::Pixel p = olc::RED;
        for (auto &elt : vListObjects) {
            float px = elt.x * fMMFactor;
            float py = elt.y * fMMFactor;
            float pr = 0.4f  * fMMFactor;
            FillCircle( px, py, pr, p );
        }
    }

    // function to render debug info in a separate hud on the screen
    void RenderDebugInfo() {
        int nStartX = ScreenWidth() - 200;
        int nStartY =  10;
        // render background pane for debug info
        FillRect( nStartX, nStartY, 195, 85, olc::VERY_DARK_GREEN );
        // output player and rendering values for debugging
        DrawString( nStartX + 5, nStartY +  5, "fPlayerX = "   + std::to_string( fPlayerX             ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 15, "fPlayerY = "   + std::to_string( fPlayerY             ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 25, "fPlayerA = "   + std::to_string( fPlayerA_deg         ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 35, "fPlayerH = "   + std::to_string( fPlayerH             ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 45, "fLookUp  = "   + std::to_string( fLookUp              ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 65, "Intensity  = " + std::to_string( fObjectIntensity     ), TEXT_COLOUR );
        DrawString( nStartX + 5, nStartY + 75, "Multiplier = " + std::to_string( fIntensityMultiplier ), TEXT_COLOUR );
    }

    // Returns the projected bottom and top of a wall slice as y screen coordinates.
    // The wall is at fCorrectedDistToWall from eye point, nHorHight is the height of the horizon
    // and fWallHeight as the height of the wall (in blocks) according to the map
    void CalculateWallBottomAndTop( float fCorrectedDistToWall, int nHorHeight, float fWallHeight, int &nWallTop, int &nWallBottom ) {
        // calculate slice height for a *unit height* wall
        int nSliceHeight = int((1.0f / fCorrectedDistToWall) * fDistToProjPlane);
        nWallTop    = nHorHeight - (nSliceHeight * (1.0f - fPlayerH)) - (fWallHeight - 1) * nSliceHeight;
        nWallBottom = nHorHeight + (nSliceHeight *         fPlayerH );
    }

	// experimental function for mouse control
    bool GetMouseSteering( float &fHorPerc, float &fVerPerc ) {
        // grab mouse coordinates
        int nMouseX = GetMouseX();
        int nMouseY = GetMouseY();

        // express mouse coords in -1.0f, 1.0f range
        float fRangeX = (nMouseX - (ScreenWidth()  / 2)) / float(ScreenWidth()  / 2);
        float fRangeY = (nMouseY - (ScreenHeight() / 2)) / float(ScreenHeight() / 2);

        // the screen width / height is mapped onto [ -1.0, +1.0 ] range
        // the range [ -0.2f, +0.2f ] is the stable (inactive) zone
        fHorPerc = 0.0f;
        fVerPerc = 0.0f;
        // if outside the stable zone, map to [ -1.0f, +1.0f ] again
        if (fRangeX < -0.2f) fHorPerc = (fRangeX + 0.2f) * ( 1.0f / 0.8f );
        if (fRangeX >  0.2f) fHorPerc = (fRangeX - 0.2f) * ( 1.0f / 0.8f );
        if (fRangeY < -0.2f) fVerPerc = (fRangeY + 0.2f) * ( 1.0f / 0.8f );
        if (fRangeY >  0.2f) fVerPerc = (fRangeY - 0.2f) * ( 1.0f / 0.8f );

        return (fHorPerc != 0.0f || fVerPerc != 0.0f);
    }

	// Shade the pixel p using fDistance as a factor in the shade formula
    olc::Pixel ShadePixel( const olc::Pixel &p, float fDistance ) {
        if (RENDER_SHADED) {
            float fShadeFactor = std::max( SHADE_FACTOR_MIN, std::min( SHADE_FACTOR_MAX, fObjectIntensity * ( fIntensityMultiplier /  fDistance )));
            return p * fShadeFactor;
        } else
            return p;
    }

    olc::Pixel ShadePixel_new( const olc::Pixel &org_pix, float fDistance, const olc::Pixel &shade_pix = SHADE_COLOUR, float fIntensity = 1.5f ) {
        return PixelLerp( org_pix, shade_pix, std::min( 1.0f, fIntensity * fDistance / fMaxDistance ));
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
        if (GetKey( olc::W ).bHeld) { fNewX += lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fElapsedTime; fNewY += lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fElapsedTime; }   // walk forward
        if (GetKey( olc::S ).bHeld) { fNewX -= lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fElapsedTime; fNewY -= lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fElapsedTime; }   // walk backwards
        if (GetKey( olc::Q ).bHeld) { fNewX += lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fElapsedTime; fNewY -= lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fElapsedTime; }   // strafe left
        if (GetKey( olc::E ).bHeld) { fNewX -= lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fElapsedTime; fNewY += lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fElapsedTime; }   // strafe right
        // collision detection - check if out of bounds or inside non-empty tile
        // only update position if no collision
        if (fNewX >= 0 && fNewX < nMapX &&
            fNewY >= 0 && fNewY < nMapY &&
            // collision detection criterion - is players height > height of map?
            float( fMap[ int( fNewY ) * nMapX + int( fNewX ) ] ) < fPlayerH ) {
            fPlayerX = fNewX;
            fPlayerY = fNewY;
        }

        // for looking up/down or crouching/flying you can speed up by keeping SHIFT pressed
        float fSpeedUp = GetKey( olc::SHIFT ).bHeld ? 4.0f : 1.0f;
        // looking up or down - collision detection not necessary
        // NOTE - there's no clamping to extreme values (yet)
        if (GetKey( olc::UP   ).bHeld) { fLookUp += SPEED_LOOKUP * fSpeedUp * fElapsedTime; }
        if (GetKey( olc::DOWN ).bHeld) { fLookUp -= SPEED_LOOKUP * fSpeedUp * fElapsedTime; }

        // Mouse control
        if (GetKey( olc::M ).bReleased) { bMouseControl = !bMouseControl; }  // toggle on or off
        float fRotFactor, fTiltFactor;
        if (bMouseControl && GetMouseSteering( fRotFactor, fTiltFactor )) {
            fPlayerA_deg += SPEED_ROTATE * fRotFactor  * fElapsedTime;
            fLookUp      -= SPEED_LOOKUP * fTiltFactor * fElapsedTime;
        }

        // flying or crouching
        // NOTE - for multi level rendering there's only clamping to keep fPlayerH > 0.0f, there's no upper limit.

        // cache current height of horizon, so that you can compensate for changes in it via the look up value
        float fCacheHorHeight = float( ScreenHeight() * fPlayerH ) + fLookUp;
        if (MULTIPLE_LEVELS) {
            // if the player height is adapted, keep horizon height stable by compensating with look up value
            if (GetKey( olc::PGUP ).bHeld) {
                fPlayerH += SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                fLookUp = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
            }
            if (GetKey( olc::PGDN ).bHeld) {
                float fNewHeight = fPlayerH - SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                // prevent negative height, and do CD on the height map
                if (fNewHeight > 0.0f && float( fMap[ int( fPlayerY ) * nMapX + int( fPlayerX ) ] ) < fNewHeight ) {
                    fPlayerH = fNewHeight;
                    fLookUp = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
        } else {
            if (GetKey( olc::PGUP ).bHeld) {
                float fNewHeight = fPlayerH + SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                if (fNewHeight < 1.0f) {
                    fPlayerH = fNewHeight;
                    // compensate look up value so that horizon remains stable
                    fLookUp = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
            if (GetKey( olc::PGDN ).bHeld) {
                float fNewHeight = fPlayerH - SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                if (fNewHeight > 0.0f) {
                    fPlayerH = fNewHeight;
                    // compensate look up value so that horizon remains stable
                    fLookUp = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
        }
        // reset look up value and player height on pressing 'R'
        if (GetKey( olc::R ).bReleased) { fPlayerH = 0.5f; fLookUp = 0.0f; }

        // alter object intensity and multiplier
        if (GetKey( olc::INS  ).bHeld) fObjectIntensity     += INTENSITY_SPEED * fElapsedTime;
        if (GetKey( olc::DEL  ).bHeld) fObjectIntensity     -= INTENSITY_SPEED * fElapsedTime;
        if (GetKey( olc::HOME ).bHeld) fIntensityMultiplier += INTENSITY_SPEED * fElapsedTime;
        if (GetKey( olc::END  ).bHeld) fIntensityMultiplier -= INTENSITY_SPEED * fElapsedTime;

        // toggles for HUDs
        if (GetKey( olc::I ).bPressed) bDebugInfo = !bDebugInfo;
        if (GetKey( olc::P ).bPressed) bMinimap   = !bMinimap;
        if (GetKey( olc::O ).bPressed) bMapRays   = !bMapRays;

        // step 2 - game logic
        // ===================

        // step 3 - render
        // ===============

        Clear( RENDER_CEILING ? olc::BLACK : olc::CYAN );

        int nHalfScreenWidth = ScreenWidth()  / 2;
        int nHorizonHeight   = ScreenHeight() * fPlayerH + (int)fLookUp;
        float fAngleStep = fPlayerFoV_deg / float( ScreenWidth() );

        // render background scene:
        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle = float( x - nHalfScreenWidth ) * fAngleStep;
            float fCurAngle = fPlayerA_deg + fViewAngle;

            float fX_hit, fY_hit;   // to hold exact (float) hit location
            int   nX_hit, nY_hit;   // to hold coords of tile that was hit

            int nWallTop, nWallTop2, nWallBottom;   // to store the top and bottom y coord of the wall per column

            // this lambda returns a sample of the ceiling through the pixel at screen coord (px, py)
            auto get_ceil_sample = [=]( int px, int py ) -> olc::Pixel {
                // work out the distance to the location on the ceiling you are looking at through this pixel
                // (the pixel is given since you know the x and y screen coordinate to draw to)
                float fCeilProjDistance = (( (1.0f - fPlayerH) / float( nHorizonHeight - py )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // calculate the world ceiling coordinate from the player's position, the distance and the view angle + player angle
                float fCeilProjX = fPlayerX + fCeilProjDistance * lu_cos( fCurAngle );
                float fCeilProjY = fPlayerY + fCeilProjDistance * lu_sin( fCurAngle );

                // calculate the sample coordinates for that world ceiling coordinate, by subtracting the
                // integer part and only keeping the fractional part. Wrap around if the result < 0
                float fSampleX = fCeilProjX - int(fCeilProjX); if (fSampleX < 0.0f) fSampleX += 1.0f;
                float fSampleY = fCeilProjY - int(fCeilProjY); if (fSampleY < 0.0f) fSampleY += 1.0f;
                // having both sample coordinates, get the sample, shade and return it
                return ShadePixel( pCeilSprite->Sample( fSampleX, fSampleY), fCeilProjDistance );
            };

            // this lambda returns a sample of the floor through the pixel at screen coord (px, py)
            auto get_floor_sample = [=]( int px, int py ) -> olc::Pixel {
                // work out the distance to the location on the floor you are looking at through this pixel
                // (the pixel is given since you know the x and y to draw to)
                float fFloorProjDistance = ((fPlayerH / float( py - nHorizonHeight )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // calculate the world floor coordinate from the distance and the view angle + player angle
                float fFloorProjX = fPlayerX + fFloorProjDistance * lu_cos( fCurAngle );
                float fFloorProjY = fPlayerY + fFloorProjDistance * lu_sin( fCurAngle );

                // calculate the sample coordinates for that world floor coordinate, by subtracting the
                // integer part and only keeping the fractional part. Wrap around if the result < 0
                float fSampleX = fFloorProjX - int(fFloorProjX); if (fSampleX < 0.0f) fSampleX += 1.0f;
                float fSampleY = fFloorProjY - int(fFloorProjY); if (fSampleY < 0.0f) fSampleY += 1.0f;
                // having both sample coordinates, get the sample, shade and return it
                return ShadePixel( pFloorSprite->Sample( fSampleX, fSampleY ), fFloorProjDistance );
            };

            // this lambda returns a sample of the roof through the pixel at screen coord (px, py)
            auto get_roof_sample = [=]( int px, int py, float fHeight ) -> olc::Pixel {
                // work out the distance to the location on the roof you are looking at through this pixel
                // (the pixel is given since you know the x and y to draw to)
                float fRoofProjDistance = (( (fPlayerH - fHeight ) / float( py - nHorizonHeight )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // calculate the world floor coordinate from the distance and the view angle + player angle
                float fRoofProjX = fPlayerX + fRoofProjDistance * lu_cos( fCurAngle );
                float fRoofProjY = fPlayerY + fRoofProjDistance * lu_sin( fCurAngle );

                // calculate the sample coordinates for that world floor coordinate, by subtracting the
                // integer part and only keeping the fractional part. Wrap around if the result < 0
                float fSampleX = fRoofProjX - int(fRoofProjX); if (fSampleX < 0.0f) fSampleX += 1.0f;
                float fSampleY = fRoofProjY - int(fRoofProjY); if (fSampleY < 0.0f) fSampleY += 1.0f;
                // having both sample coordinates, get the sample, shade and return it
                return ShadePixel( pRoofSprite->Sample( fSampleX, fSampleY ), fRoofProjDistance );
            };

            // prepare the rendering for this slice by calculating the list of intersections in this ray's direction
            std::vector<IntersectInfo> vColHitlist;
            float fColHeight = 1.0f;
            float fCurDistance = 0.0f;     // distance var needed for wall shading
            if (GetDistancesToWalls( fCurAngle, vColHitlist )) {

                // at least one wall / block was hit. Extend the hit list with projected bottom / ceiling info
                for (int i = 0; i < (int)vColHitlist.size(); i++) {
                    // make correction for the fish eye effect
                    vColHitlist[i].fDistance *= lu_cos( fViewAngle );
                    CalculateWallBottomAndTop( vColHitlist[i].fDistance, nHorizonHeight, vColHitlist[i].fHeight, vColHitlist[i].ceil_front, vColHitlist[i].bottom_front );
                }
                // Extend the hit list with projected ceiling info for the back of the wall
                for (int i = 0; i < (int)vColHitlist.size(); i++) {
                    if (i == (int)vColHitlist.size() - 1) {
                        vColHitlist[i].ceil_back = vColHitlist[i].ceil_front;
                    } else {
                        int nDummy;
                        CalculateWallBottomAndTop( vColHitlist[i + 1].fDistance, nHorizonHeight, vColHitlist[i].fHeight, vColHitlist[i].ceil_back, nDummy );
                    }
                }

                // get the info from first hit point
                fX_hit     = vColHitlist[0].fHitX;
                fY_hit     = vColHitlist[0].fHitY;
                nX_hit     = vColHitlist[0].nMapCoordX;
                nY_hit     = vColHitlist[0].nMapCoordY;
                fColHeight = vColHitlist[0].fHeight;
                fCurDistance = vColHitlist[0].fDistance;

                nWallTop     = vColHitlist[0].ceil_front;
                nWallTop2    = vColHitlist[0].ceil_back;
                nWallBottom  = vColHitlist[0].bottom_front;

            } else {
                // no wall was hit - set bottom and top value for wall at the horizon
                nWallTop     = nHorizonHeight;
                nWallTop2    = nWallTop;
                nWallBottom  = nHorizonHeight;
                fCurDistance = fMaxDistance;
            }

            // populate ray list for rendering mini map
            if (!vColHitlist.empty()) {
                olc::vf2d curHitPoint = { vColHitlist[0].fHitX, vColHitlist[0].fHitY };
                vRayList.push_back( curHitPoint );
            }

            // Update depth buffer - note that this is the distance to the *first* hitpoint
            // if there are no hitpoints, it will be set to fMaxDistance
            fDepthBuffer[x] = fCurDistance;

// constants for different types of rendering
#define UNKNOWN_DRAWING 0
#define   FLOOR_DRAWING 1
#define    WALL_DRAWING 2
#define     SKY_DRAWING 3
#define    ROOF_DRAWING 4

            // now render this slice using the info of the hit list
            int nHitListIndex = 0;
            int nCacheMode = UNKNOWN_DRAWING;
            float fSampleX;

            // note that we are working upwards
            for (int y = ScreenHeight() - 1; y >= 0; y--) {

                // determine what type of segment is rendered: floor, wall, roof or ceiling
                int nDrawMode = UNKNOWN_DRAWING;

                if (y >= nWallBottom) {
                    nDrawMode = (y <= nHorizonHeight) ? SKY_DRAWING : FLOOR_DRAWING;
                } else if (nWallBottom > y && y > nWallTop) {
                    nDrawMode = WALL_DRAWING;
                } else if (nWallTop >= y && y > nWallTop2) {
                    nDrawMode = (fColHeight == 0.0f) ? FLOOR_DRAWING : ROOF_DRAWING;
                } else {
                    while (nDrawMode == UNKNOWN_DRAWING) {
                        if (nHitListIndex < (int)vColHitlist.size() - 1) {
                            // the y coord is above the current wall and roof slide, but there are still hit points to process
                            // so there could be other walls behind current wall sticking out above it
                            nHitListIndex += 1;

                            // get the info from next hit point
                            fX_hit     = vColHitlist[ nHitListIndex ].fHitX;
                            fY_hit     = vColHitlist[ nHitListIndex ].fHitY;
                            nX_hit     = vColHitlist[ nHitListIndex ].nMapCoordX;
                            nY_hit     = vColHitlist[ nHitListIndex ].nMapCoordY;
                            fColHeight = vColHitlist[ nHitListIndex ].fHeight;
                            fCurDistance = vColHitlist[ nHitListIndex ].fDistance;
                            nWallTop     = vColHitlist[ nHitListIndex ].ceil_front;
                            nWallTop2    = vColHitlist[ nHitListIndex ].ceil_back;
                            nWallBottom  = vColHitlist[ nHitListIndex ].bottom_front;

                            if (y >= nWallBottom) {
                                nDrawMode = (y <= nHorizonHeight) ? SKY_DRAWING : FLOOR_DRAWING;
                            } else if (nWallBottom > y && y > nWallTop) {
                                nDrawMode = WALL_DRAWING;
                                nCacheMode = UNKNOWN_DRAWING;
                            } else if (nWallTop >= y && y > nWallTop2) {
                                nDrawMode = ROOF_DRAWING;
                            }
                        } else {
                            nDrawMode = (y <= nHorizonHeight) ? SKY_DRAWING : FLOOR_DRAWING;
                        }
                    }
                }

                // now we know what type of segment we're working on, render it
                switch (nDrawMode) {
                    case SKY_DRAWING: {                         // ========== render ceiling ====================
                            if (RENDER_CEILING) {
                                olc::Pixel ceilSample = get_ceil_sample( x, y );
                                Draw( x, y, ceilSample );
                            }
                        }
                        break;
                    case FLOOR_DRAWING: {                        // ========== render floor   ====================
                            olc::Pixel floorSample = get_floor_sample( x, y );
                            Draw( x, y, floorSample );
                        }
                        break;
                    case ROOF_DRAWING: {                        // ========== render roof   ====================
                            olc::Pixel roofSample = get_roof_sample( x, y, fColHeight );
                            Draw( x, y, roofSample );
                        }
                        break;
                    case WALL_DRAWING: {                         // ========== render wall    ====================
                            if (nCacheMode != WALL_DRAWING) {
                                // this is the first pixel of this wall sub slice, so get the fSampleX value

                                // the x sample coordinate takes more work to figure out. You need to check what side of the
                                // block was hit
                                float fBlockMidX = (float)nX_hit + 0.5f;   // location of middle of the cell that was hit
                                float fBlockMidY = (float)nY_hit + 0.5f;
                                // determine in what quadrant the hit location is wrt the block mid point
                                float fTestAngle = atan2f((fY_hit - fBlockMidY), (fX_hit - fBlockMidX));

                                // I tested several supposedly faster approximations for atan2f(), but the results are really not significant

                                // The major bottleneck is that this analysis is done for each separate pixel in the slice:
                                //   * possible improvement 1: determine the ranges within a slice so that you don't have to repeat the atan2f() call for each pixel
                                //   * possible improvement 2: (after 1) render these slice parts as scaled decals

                                if (-0.75f * PI <= fTestAngle && fTestAngle <  -0.25f * PI) fSampleX = fX_hit - (float)nX_hit;  // south side
                                if (-0.25f * PI <= fTestAngle && fTestAngle <   0.25f * PI) fSampleX = fY_hit - (float)nY_hit;  // east  side
                                if ( 0.25f * PI <= fTestAngle && fTestAngle <   0.75f * PI) fSampleX = fX_hit - (float)nX_hit;  // north side
                                if (-0.75f * PI >  fTestAngle || fTestAngle >=  0.75f * PI) fSampleX = fY_hit - (float)nY_hit;  // west  side
                            }

                            float fSampleY;
                            if (STRETCHED_TEXTURING) {
                                // original sampling = stretched over full height of wall
                                // the y sample coordinate depends only on the pixel y coord on the screen
                                // in relation to the vertical space the wall is taking up
                                fSampleY = float(y - nWallTop) / float(nWallBottom - nWallTop);
                            } else {
                                // sophisticated sampling = sampling per unit block size
                                float fBlockProjHeight = float( nWallBottom - nWallTop ) / fColHeight;
                                float fRelativeY = float( y - nWallTop );
                                while (fRelativeY > fBlockProjHeight)
                                    fRelativeY -= fBlockProjHeight;
                                fSampleY = fRelativeY / fBlockProjHeight;
                            }

                            // having both sample coordinates, get the sample, shade it and draw the pixel
                            olc::Pixel wallSample = pWallSprite->Sample( fSampleX, fSampleY );
                            Draw( x, y, ShadePixel( wallSample, fCurDistance ));
                        }
                        break;
                }
                nCacheMode = nDrawMode;
            }
        }

        // display all objects after the background rendering and before displaying the minimap or debugging output
        float fPlayerFoV_rad = deg2rad( fPlayerFoV_deg );
        // split the rendering into two phase so that it can be sorted on distance (painters algo) before rendering

        // phase 1 - just determine distance (and angle cause of convenience)
        for (auto &object : vListObjects) {
            // can object be seen?
            float fVecX = object.x - fPlayerX;
            float fVecY = object.y - fPlayerY;
            object.distance = sqrtf( fVecX * fVecX + fVecY * fVecY );
            // calculate angle between vector from player to object, and players looking direction
            // to determine if object is in players field of view
            float fEyeX = lu_cos( fPlayerA_deg );
            float fEyeY = lu_sin( fPlayerA_deg );
            float fObjA = atan2f( fVecY, fVecX ) - atan2f( fEyeY, fEyeX );
            // "bodge" angle into range [ -PI, PI ]
            if (fObjA < - PI) fObjA += 2.0f * PI;
            if (fObjA >   PI) fObjA -= 2.0f * PI;
            object.angle = fObjA;
        }

        vListObjects.sort(
            [=]( const sObject &a, const sObject &b ) {
                return a.distance > b.distance;
            }
        );

        // phase 2: render it from large to smaller distances
        for (auto &object : vListObjects) {
            // can object be seen?
            float fObjDist = object.distance;
            float fObjA = object.angle;
            // determine whether object is in field of view (slightly larger to prevent objects being not rendered at
            // screen boundaries)
            bool bInFOV = fabs( fObjA ) < fPlayerFoV_rad / 1.6f;

            // render object only when within Field of View, and within visible distance.
            // the check on proximity is to prevent asymptotic errors when this distance becomes very small
            if (bInFOV && fObjDist >= 0.3f && fObjDist < fMaxDistance) {
                // get the projected (halve) slice height of this object
                float fObjHlveSliceHeight     = float( ScreenHeight()                  / fObjDist);
                float fObjHlveSliceHeightScld = float((ScreenHeight() * object.scale ) / fObjDist);

                // work out where objects floor and ceiling are (in screen space)
                // due to scaling factor, differentiated a normalized (scale = 1.0f) ceiling and a scaled variant
                float fObjCeilingNormalized = float(ScreenHeight() / 2.0f) - fObjHlveSliceHeight;
                float fObjCeilingScaled     = float(ScreenHeight() / 2.0f) - fObjHlveSliceHeightScld;
                // and adapt all the scaling into the ceiling value
                float fScalingDifference = fObjCeilingNormalized - fObjCeilingScaled;
                float fObjCeiling = fObjCeilingNormalized - 2 * fScalingDifference;
                float fObjFloor = float(ScreenHeight() / 2.0f) + fObjHlveSliceHeight;

                // get height, aspect ratio and width
                float fObjHeight = fObjFloor - fObjCeiling;
                float fObjAR = float( object.sprite->height ) / float( object.sprite->width );
                float fObjWidth  = fObjHeight / fObjAR;
                // work out where the object is across the screen width
                float fMidOfObj = (0.5f * (fObjA / (fPlayerFoV_rad / 2.0f)) + 0.5f) * float( ScreenWidth());

                // render the sprite
                for (float fx = 0.0f; fx < fObjWidth; fx++) {
                    // get distance across the screen to render
                    int nObjColumn = int( fMidOfObj + fx - (fObjWidth / 2.0f));
                    // only render this column if it's on the screen
                    if (nObjColumn >= 0 && nObjColumn < ScreenWidth()) {
                        for (float fy = 0.0f; fy < fObjHeight; fy++) {
                            // calculate sample coordinates as a percentage of object width and height
                            float fSampleX = fx / fObjWidth;
                            float fSampleY = fy / fObjHeight;
                            // sample the pixel and draw it
                            olc::Pixel pSample = object.sprite->Sample( fSampleX, fSampleY );
                            if (pSample != olc::BLANK && fDepthBuffer[nObjColumn] >= fObjDist) {
                                Draw( nObjColumn, fObjCeiling + fy, pSample );
                                fDepthBuffer[nObjColumn] = fObjDist;
                            }
                        }
                    }
                }
            }
        }

        if (bMinimap) {
            RenderMapGrid();
            if (bMapRays) {
                RenderMapRays();
            }
            RenderMapPlayer();
            RenderMapObjects();
        }
        vRayList.clear();

        if (bDebugInfo) {
            RenderDebugInfo();
        }

        return true;
    }

    bool OnUserDestroy() {

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
