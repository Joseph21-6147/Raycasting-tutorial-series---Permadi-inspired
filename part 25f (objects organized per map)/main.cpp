// ELABORATING ON - Ray casting tutorial by Permadi
// (starting with part 20 all code files are my own elaboration on the Permadi basis)
//
// Implementation of part 25 f - objects organized per map
//
// Joseph21, july 5, 2023
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, roofs, floor and ceiling, as well as objects in the scene: use your own .png files and
//      adapt the file names and paths in "map_16x16.h"
//   *  include file "map_16x16.h" - contains the map definitions and the sprite file names

/* Short description
   -----------------
   This implementation is a follow up of implementation part 25 e. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

   Summary of the changes I made:
     + main.cpp
         * Removed vListObjects as a class variable, since it is now put into the RC_Map object
         * The organizing of objects per map was a nice opportunity to refactor the object initializing code into separate functions:
           InitMaps() to initialize all maps depending on the user defined map file data, and InitObjectsPerMap(), which takes
           percentages of dynamic, static, bush and tree type objects to initialze in the designated map. This helped making
           OnUserCreate() a bit simpler
         * Renamings were needed relative to the active map everywhere the vObjectList was addressed in the code
     + RC_Map
         * the vObjectList is added to the RC_Map object
     + RC_Misc
         * Added convenience functions for is_in_range() checking
     + RC_Object
         * needed RC_Map, to be able to call Collide() on it. This caused a circular reference which I had to break. This necessitated
           the reference parameter of Update() to become a pointer.
         * differentiated percentages for different kinds of objects

   Have fun!
 */

#include <cfloat>       // needed for constant FLT_MAX in the DDA function

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

// ==============================/  specific include files   /==============================

#include "RC_Misc.h"
#include "RC_Face.h"
#include "RC_MapCell.h"
#include "RC_Map.h"
#include "RC_DepthDrawer.h"
#include "RC_Object.h"

// ==============================/  constants   /==============================

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X      1000
#define SCREEN_Y       600
#define PIXEL_SIZE       1

#define MULTI_LAYERS      true
#define RENDER_CEILING       !MULTI_LAYERS    // render ceilings only for single layer world

// shading constants
#define RENDER_SHADED        true
#define OBJECT_INTENSITY       5.0f   // for testing, reset to 1.5f afterwards!
#define MULTIPLIER_INTENSITY   5.0f
#define INTENSITY_SPEED        1.0f

#define SHADE_FACTOR_MIN       0.1f   // the shade factor is clamped between these two values
#define SHADE_FACTOR_MAX       1.0f

// constants for speed movements - all movements are modulated with fElapsedTime
#define SPEED_ROTATE          60.0f   //                            60 degrees per second
#define SPEED_MOVE             5.0f   // forward and backward    -   5 units per second
#define SPEED_STRAFE           5.0f   // left and right strafing -   5 units per second
#define SPEED_LOOKUP         200.0f   // looking up or down      - 200 pixels per second
#define SPEED_STRAFE_UP        1.0f   // flying or chrouching    -   1.0 unit per second

// mini map constants
#define MINIMAP_TILE_SIZE     (32 / PIXEL_SIZE)   // each minimap tile is ... pixels
#define MINIMAP_SCALE_FACTOR   0.4    // should be 0.2

#define SENSE_RADIUS    2.0f    // player must be this close to map cell center to be able to open doors etc
#define SENSE_BLENDF    0.4f    // bland factor to render sense circle around player

// colour constants
#define COL_HUD_TXT     olc::YELLOW
#define COL_HUD_BG      olc::VERY_DARK_GREEN

// ==============================/  map definition here   /==============================

#include "map_16x16.h"

// ==============================/  PGE derived ray caster engine   /==============================


class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial elaborations - S:(" + std::to_string( SCREEN_X / PIXEL_SIZE ) + ", " + std::to_string( SCREEN_Y / PIXEL_SIZE ) + ")" +
                                                               ", P:(" + std::to_string(            PIXEL_SIZE ) + ", " + std::to_string(            PIXEL_SIZE ) + ")" ;
    }

private:
    std::vector<RC_Map> vMaps;    // the list of all game map objects
    int nActiveMap = 0;           // keeps track of which of the maps is currently active
    float fMaxDistance;           // max visible distance - use length of map diagonal to overlook whole map

    float fPlayerX     =  4.5f;   // player: position
    float fPlayerY     =  4.5f;
    float fPlayerH     =  0.5f;   // player: height of eye point and field of view
    float fPlayerA_deg = 90.0f;   // player: looking angle is in degrees - NOTE: 0.0f is EAST
    float fPlayerLU    =  0.0f;   // factor for looking up or down (in pixel space: float is for smooth movement)
    // rationale: when fPlayerH == 0.5f, then lookup factor == 0. When fPlayerH == 1.5f, the lookup factor equals one screen height

    float fPlayerFoV_deg = 60.0f;   // in degrees !!
    float fPlayerFoV_rad;

    float fAnglePerPixel_deg;      // how many degrees are needed to rotate to next screen slice? - is calculated in OnUserCreate()
    float fDistToProjPlane;         // constant distance to projection plane - is calculated in OnUserCreate()

    // all sprites for texturing the scene and the objects, grouped in categories
    std::vector<olc::Sprite *> vWallSprites;
    std::vector<olc::Sprite *> vCeilSprites;
    std::vector<olc::Sprite *> vRoofSprites;
    std::vector<olc::Sprite *> vFlorSprites;
    std::vector<olc::Sprite *> vObjtSprites;

    // var's and initial values for shading - trigger keys INS and DEL
    float fObjectIntensity     = MULTI_LAYERS ? OBJECT_INTENSITY     :  0.2f;
    float fIntensityMultiplier = MULTI_LAYERS ? MULTIPLIER_INTENSITY : 10.0f;

    // toggles for rendering
    bool bMinimap     = false;    // toggle on mini map rendering (trigger key P)
    bool bMapRays     = false;    //           rays in mini map   (trigger key O)
    bool bPlayerInfo  = false;    //           player info hud    (trigger key I)
    bool bProcessInfo = false;    //           process info hud   (trigger key U)
    bool bTestSlice   = false;    //           visible test slice (trigger key G)
    bool bTestGrid    = false;    //           visible test grid  (trigger key H)

    typedef struct sRayStruct {
        olc::vf2d pointA, pointB;
        int layer;
    } RayType;
    std::vector<RayType> vRayList;    // needed for ray rendering in minimap

    // which of the slices to output in test mode
    float fTestSlice;
    bool bTestMode = false;           // to trigger test output

    // to keep statistics on recursion depth of slice rendering algorithm
    int nMaxRecursions = 0;
    int nMaxRecSlice = -1;

    RC_DepthDrawer cDDrawer;              // depth drawing object

public:

    // create and fill the maps - these are defined in a separate file
    // NOTES: 1) string arguments in AddLayer() must all have the same x and y dimensions
    //        2) the z-dimension of the map is determined by the nr of layers added to it
    //        3) the parameters vWallSprites, vCeilSprites and vRoofSprites must be initialised upfront
    void InitMaps() {

        // lambda expression to determine the sky clour per map
        auto get_sky_colour = [=]( int mapID ) -> olc::Pixel {
            if (mapID < (int)vSkyColours.size()) {
                return vSkyColours[ mapID ];
            } else {
                return olc::CYAN;
            }
        };
        // initialize all layers (counter n) for all maps (counter m) in the input data
        for (int m = 0; m < (int)vMapLayouts.size(); m++) {
            RC_Map tmp;
            tmp.InitMap( m, vMapPortals[m], vFlorSprites[m], get_sky_colour( m ));
            MapType &sMapLayout = vMapLayouts[m];
            for (int n = 0; n < (int)sMapLayout.size(); n++) {
                tmp.AddLayer( sMapLayout[n], vWallSprites, vCeilSprites, vRoofSprites );
            }
            vMaps.push_back( tmp );
        }
    }

    // Four percentages are passed, for dynamic objects, stationary objects, bushes and trees.
    void InitObjectsPerMap( RC_Map *pCurMapPtr, float fObjDynPerc, float fObjStatPerc, float fObjBushPerc, float fObjTreePerc ) {

        // aux map to keep track of placed objects
        // count nr of occupied cells at the same time
        std::string sObjMap;
        int nTilesOccupied = 0;
        for (int y = 0; y < pCurMapPtr->GetHeight(); y++) {
            for (int x = 0; x < pCurMapPtr->GetWidth(); x++) {
                sObjMap.append( " " );
                if (pCurMapPtr->CellHeight( x, y ) != 0.0f) {
                    nTilesOccupied += 1;
                }
            }
        }
        // only place objects where there's nothing in the immediate (8 connected) neighbourhood
        auto space_for_object = [=]( int x, int y ) -> bool {
            int xMin = std::max( 0, x - 1 );
            int yMin = std::max( 0, y - 1 );
            int xMax = std::min( pCurMapPtr->GetWidth()  - 1, x + 1 );
            int yMax = std::min( pCurMapPtr->GetHeight() - 1, y + 1 );
            bool bOccupied = false;
            for (int r = yMin; r <= yMax && !bOccupied; r++) {
                for (int c = xMin; c <= xMax && !bOccupied; c++) {
                    bOccupied = pCurMapPtr->CellHeight( c, r ) != 0.0f || sObjMap[ r * pCurMapPtr->GetWidth() + c ] != ' ';
                }
            }
            return !bOccupied;
        };

        int nNrDynObjects  = int((pCurMapPtr->GetWidth() * pCurMapPtr->GetHeight() - nTilesOccupied) * fObjDynPerc);
        int nNrStatObjects = int((pCurMapPtr->GetWidth() * pCurMapPtr->GetHeight() - nTilesOccupied) * fObjStatPerc);
        int nNrBushObjects = int((pCurMapPtr->GetWidth() * pCurMapPtr->GetHeight() - nTilesOccupied) * fObjBushPerc);
        int nNrTreeObjects = int((pCurMapPtr->GetWidth() * pCurMapPtr->GetHeight() - nTilesOccupied) * fObjTreePerc);
        int nTotalNrObjects = nNrDynObjects + nNrStatObjects + nNrBushObjects + nNrTreeObjects;

        int nDynChoices  =  1;    // these values are depending on the available object sprite files loaded via
        int nStatChoices =  2;    // the user defined map
        int nBushChoices = 10;
        int nTreeChoices = 18;

        // populate object list with randomly chosen, scaled and placed objects
        for (int i = 0; i < nTotalNrObjects; i++) {
            int nRandX, nRandY;
            bool bFoundEmpty = false;
            bool bMakeDynamic = false;
            // find an empty spot in the map
            do {
                nRandX = rand() % pCurMapPtr->GetWidth();
                nRandY = rand() % pCurMapPtr->GetHeight();

                bFoundEmpty = space_for_object( nRandX, nRandY );
            } while (!bFoundEmpty);

            // determine object type - make sure that at least MIN_DYNAMIC_OBJS objects are dynamic
            int nRandObj;
            if (is_in_range( i,              0, nNrDynObjects  )) { nRandObj = rand() % nDynChoices;                                              } else
            if (is_in_range( i,  nNrDynObjects, nNrStatObjects )) { nRandObj = rand() % nStatChoices + nDynChoices;                               } else
            if (is_in_range( i, nNrStatObjects, nNrBushObjects )) { nRandObj = rand() % nBushChoices + nDynChoices + nStatChoices;                } else
                                                                  { nRandObj = rand() % nTreeChoices + nDynChoices + nStatChoices + nBushChoices; }

            // depending on object type, make dynamic or static and determine size
            int nRandSize;
            if (is_in_range( nRandObj, 0,  1 )) { bMakeDynamic = true ; nRandSize = rand() %  3 +  3; } else   // this is an elf girl, make dynamic
            if (is_in_range( nRandObj, 1,  3 )) { bMakeDynamic = false; nRandSize =                6; } else   // these are stationary objects of fixed size
            if (is_in_range( nRandObj, 3, 13 )) { bMakeDynamic = false; nRandSize = rand() %  8 +  2; } else   // these are bushes
                                                { bMakeDynamic = false; nRandSize = rand() % 15 + 10; }        // trees

            // create the object and put it in the list of objects
            RC_Object tmpObj( float( nRandX ) + 0.5f, float( nRandY ) + 0.5f, float( nRandSize / 10.0f ), -1.0f, 0.0f, vObjtSprites[ nRandObj ] );
            if (bMakeDynamic) {
                tmpObj.bStationary = false;
                tmpObj.SetVX( float_rand_between( -5.0f, 5.0f ));
                tmpObj.SetVY( float_rand_between( -5.0f, 5.0f ));
            } else {
                tmpObj.bStationary = true;
                tmpObj.SetVX( 0.0f );
                tmpObj.SetVY( 0.0f );
            }

            pCurMapPtr->vListObjects.push_back( tmpObj );

            // mark that an object is placed at this tile
            sObjMap[ nRandY * pCurMapPtr->GetWidth() + nRandX ] = 'X';
        }
    }

    bool OnUserCreate() override {

        bool bSuccess = true;

        // seed randomizer
        srand( time( 0 ));
        // initialize sine and cosine lookup arrays - these are meant for performance improvement
        init_lu_sin_array();
        init_lu_cos_array();
        // Work out distance to projection plane. This is a constant float value, depending on the width of the projection plane and the field of view.
        fDistToProjPlane = ((ScreenWidth() / 2.0f) / lu_sin( fPlayerFoV_deg / 2.0f )) * lu_cos( fPlayerFoV_deg / 2.0f );

        // lambda expression for loading sprite files with error checking
        auto load_sprite_file = [=]( const std::string &sFileName ) {
            olc::Sprite *tmp = new olc::Sprite( sFileName );
            if (tmp->width == 0 || tmp->height == 0) {
                std::cout << "ERROR: OnUserCreate() --> can't load file: " << sFileName << std::endl;
                delete tmp;
                tmp = nullptr;
            }
            return tmp;
        };
        // lambda expression for loading all sprites for one category (walls, ceilings, roofs, floors or objects) into the
        // associated container
        auto load_sprites_from_files = [=]( std::vector<std::string> &vFileNames, std::vector<olc::Sprite *> &vSpritePtrs, const std::string &sType ) {
            bool bNoErrors = true;
            for (auto &sf : vFileNames) {
                olc::Sprite *tmpPtr = load_sprite_file( sf );
                bNoErrors &= (tmpPtr != nullptr);
                vSpritePtrs.push_back( tmpPtr );
            }
            std::cout << "Loaded: " << (int)vFileNames.size() << " files into " << (int)vSpritePtrs.size() << " " << sType << " sprites."    << std::endl;
            return bNoErrors;
        };
        // load all sprites into their associated container
        bSuccess &= load_sprites_from_files( vWallSpriteFiles, vWallSprites, "wall"    );
        bSuccess &= load_sprites_from_files( vCeilSpriteFiles, vCeilSprites, "ceiling" );
        bSuccess &= load_sprites_from_files( vRoofSpriteFiles, vRoofSprites, "roof"    );
        bSuccess &= load_sprites_from_files( vFlorSpriteFiles, vFlorSprites, "floor"   );
        bSuccess &= load_sprites_from_files( vObjtSpriteFiles, vObjtSprites, "object"  );

        // fill the library of face blueprints
        InitFaceBluePrints( vWallSprites, vCeilSprites, vRoofSprites );
        // fill the library of map cell blueprints
        InitMapCellBluePrints();
        // initialise all layers of all maps using the vMapLayouts vector
        // as a results the vMaps vector is populated
        InitMaps();
        // initialise objects per map
        float fObjPercentage = 0.0f;
        for (int i = 0; i < (int)vMaps.size(); i++) {
            switch( i ) {
                case 0: fObjPercentage = 2.0f; break;
                case 1: fObjPercentage = 0.0f; break;
                case 2: fObjPercentage = 1.0f; break;
            }
            InitObjectsPerMap(
                &(vMaps[i]),
                fObjPercentage * OBJ_PERC_DYN,
                fObjPercentage * OBJ_PERC_STAT,
                fObjPercentage * OBJ_PERC_BUSH,
                fObjPercentage * OBJ_PERC_TREE
            );
        }
        // set the active map according to map def. data file
        nActiveMap = 0;
        // max ray length for DDA is diagonal length of the map
        fMaxDistance = vMaps[ nActiveMap ].DiagonalLength();

        // set initial test slice value at middle of screen
        fTestSlice = ScreenWidth() / 2.0f;
        // determine how much degrees one pixel shift represents.
        fAnglePerPixel_deg = fPlayerFoV_deg / float( ScreenWidth());
        // determine the player field of view in radians as well
        fPlayerFoV_rad = deg2rad( fPlayerFoV_deg );
        // initialise the depth drawer object
        cDDrawer.Init( this );

        return bSuccess;
    }

    // Holds intersection point in float (world) coordinates and in int (tile) coordinates,
    // the distance to the intersection point and the height of the map at these tile coordinates
    typedef struct sIntersectInfo {
        float fHitX,             // world space hit point
              fHitY;
        int   nHitX,             // tile space hit point
              nHitY;
        float fDistFrnt_raw,     // raw distances to front and back faces of hit block
              fDistBack_raw;
        float fDistFrnt_corr,    // corrected distances to front and back faces of hit block
              fDistBack_corr;
        float fHeight;           // height within the layer
        int   nLayer = -1;       // nLayer == 0 --> ground layer

        // these are on screen projected (O.S.P.) values (y coordinate in pixel space)
        int osp_bot_frnt = -1;    // on screen projected bottom  of wall slice
        int osp_bot_back = -1;    //                     bottom  of wall at back
        int osp_top_frnt = -1;    //                     ceiling of wall slice
        int osp_top_back = -1;    //                     ceiling of wall at back

        int nFaceHit = FACE_UNKNOWN;     // which face was hit?
        bool bHorizHit;                  // was the hit on a horizontal grid line?
    } IntersectInfo;

    // put info from hit point p to screen
    void PrintHitPoint( IntersectInfo &p, bool bVerbose ) {
        std::cout << "hit (world): ( " << p.fHitX << ", " << p.fHitY << " ) ";
        std::cout << "hit (tile): ( " << p.nHitX << ", " << p.nHitY << " ) ";
        std::cout << "raw dist.: "     << p.fDistFrnt_raw  << " ";
        std::cout << "corr. dist.: "   << p.fDistFrnt_corr << " ";
        std::cout << "lvl: " << p.nLayer << " hght: " << p.fHeight << " ";
        if (bVerbose) {
            std::cout << "bot frnt: " << p.osp_bot_frnt << " bot back: " << p.osp_bot_back << " ";
            std::cout << "top frnt: " << p.osp_top_frnt << " top back: " << p.osp_top_back << " ";
            switch (p.nFaceHit) {
                case FACE_EAST   : std::cout << "EAST";    break;
                case FACE_NORTH  : std::cout << "NORTH";   break;
                case FACE_WEST   : std::cout << "WEST";    break;
                case FACE_SOUTH  : std::cout << "SOUTH";   break;
                case FACE_TOP    : std::cout << "TOP";     break;
                case FACE_BOTTOM : std::cout << "BOTTOM";  break;
                case FACE_UNKNOWN: std::cout << "UNKNOWN"; break;
                default          : std::cout << "ERROR: "   << p.nFaceHit;
            }
        }
        std::cout << std::endl;
    }

    // put info from hit list vHitList to screen
    void PrintHitList( std::vector<IntersectInfo> &vHitList, bool bVerbose = false ) {
        for (int i = 0; i < (int)vHitList.size(); i++) {
            std::cout << "Elt: " << i << " = ";
            PrintHitPoint( vHitList[i], bVerbose );
        }
        std::cout << std::endl;
    }

    // Implementation of the DDA algorithm.
    // This function uses nCurMap as the index into the vMaps array to obtain the correct map.
    // It then casts the ray from (fPx, fPy, nPz) in the direction of fRayAngle_deg:
    // A "to point" is determined using + fRayAngle_deg and fMaxDistance. A ray is cast from the "from point" to the "to point".
    bool CastRayPerLevelAndAngle( int nCurMap, float fPx, float fPy, int layer, float fRayAngle_deg, std::vector<IntersectInfo> &vHitList ) {

        // get a reference to the map
        RC_Map &pCurMap = vMaps[ nCurMap ];
        // counter for nr of hit points found
        int nHitPointsFound = 0;

        // The player's position is the "from point"
        float fFromX = fPx;
        float fFromY = fPy;
        // Calculate the "to point" using the player's angle and fMaxDistance
        float fToX = fPx + fMaxDistance * lu_cos( fRayAngle_deg );
        float fToY = fPy + fMaxDistance * lu_sin( fRayAngle_deg );
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

        // init loop variables
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
        bool bOutOfBounds = !pCurMap.IsInBounds( nCurX, nCurY );
        // did analysis reach the destination cell?
        bool bDestCellReached = (nCurX == int( fToX ) && nCurY == int( fToY ));
        // to keep track of what direction you are searching
        bool bCheckHor;

        // lambda to return index value of face that was hit
        auto get_face_hit = [=]( bool bHorGridLine ) {
            int nFaceValue = FACE_UNKNOWN;
            if (bHorGridLine) {
                nFaceValue = (nGridStepY < 0 ? FACE_SOUTH : FACE_NORTH);
            } else {
                nFaceValue = (nGridStepX < 0 ? FACE_EAST  : FACE_WEST );
            }
            return nFaceValue;
        };

        // convenience lambda to add hit point with one call
        auto add_hit_point = [&]( std::vector<IntersectInfo> &vHList, float fDst, float fStrtX, float fStrtY, float fDeltaX, float fDeltaY, int nTileX, int nTileY, float fHght, int nLayer, bool bHorGrid ) {
            IntersectInfo sInfo;

            sInfo.fDistFrnt_raw = fDst;
            sInfo.fHitX      = fStrtX + fDst * fDeltaX;
            sInfo.fHitY      = fStrtY + fDst * fDeltaY;
            sInfo.nHitX      = nTileX;
            sInfo.nHitY      = nTileY;
            sInfo.fHeight    = fHght;
            sInfo.nLayer     = nLayer;
            sInfo.nFaceHit   = get_face_hit( bHorGrid );
            sInfo.bHorizHit  = bHorGrid;

            vHList.push_back( sInfo );
        };

        float fDistIfFound = 0.0f;  // accumulates distance of analysed piece of ray
        float fCurHeight   = 0.0f;  // to check on differences in height

        bool bPrevWasTransparent = false;
        // terminate the loop / algorithm if out of bounds or destination found or maxdistance exceeded
        while (!bOutOfBounds && !bDestCellReached && fDistIfFound < fMaxDistance) {

            // advance to next map cell, depending on length of partial ray's
            if (fLengthPartialRayX < fLengthPartialRayY) {
                // continue analysis in x direction
                nCurX += nGridStepX;
                fDistIfFound = fLengthPartialRayX;
                fLengthPartialRayX += fSX;
                bCheckHor = false;
            } else {
                // continue analysis in y direction
                nCurY += nGridStepY;
                fDistIfFound = fLengthPartialRayY;
                fLengthPartialRayY += fSY;
                bCheckHor = true;
            }

            bOutOfBounds = !pCurMap.IsInBounds( nCurX, nCurY );
            if (bOutOfBounds) {
                bDestCellReached = false;

                // If out of bounds, finalize the list with one additional intersection with the map boundary and height 0.
                // (only if the list is not empty!) This additional intersection record is necessary for proper rendering at map boundaries.
                if (fCurHeight != 0.0f && nHitPointsFound > 0) {

                    fCurHeight = 0.0f;  // since we're out of bounds
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, layer, bCheckHor );
                }
            } else {
                // check if there's a difference in height found
                bool bHitFound = (pCurMap.CellHeightAt( nCurX, nCurY, layer ) != fCurHeight);
                // NEW CODE HERE:
                // check if this is a transparent map cell
                RC_MapCell *auxMapCellPtr = pCurMap.MapCellPtrAt( nCurX, nCurY, layer );
                bool bTrnspMapCell;
                // have a portal cell count as not being transparent, otherwise the rendering goes wrong
                if (auxMapCellPtr->IsEmpty() || auxMapCellPtr->IsPortal()) {
                    bTrnspMapCell = false;
                } else {
                    RC_Face *auxFacePtr = auxMapCellPtr->GetFacePtr( get_face_hit( bCheckHor ));
                    bTrnspMapCell = auxFacePtr->IsTransparent();
                }

                // check if destination cell is found already (for loop control)
                bDestCellReached = (nCurX == int( fToX ) && nCurY == int( fToY ));

                if (bHitFound || bPrevWasTransparent) {
                    bPrevWasTransparent = bTrnspMapCell;
                    nHitPointsFound += 1;
                    // set current height to new value
                    fCurHeight = pCurMap.CellHeightAt( nCurX, nCurY, layer );
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, layer, bCheckHor );
                } else if (bTrnspMapCell) {
                    bPrevWasTransparent = true;
                    nHitPointsFound += 1;
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, layer, bCheckHor );
                }
            }
        }
        // return whether any hitpoints were found on this layer
        return (nHitPointsFound > 0);
    }

    // Returns the on screen projected values (i.e. the y screen coordinates) for top and bottom of a map cell.
    // The map cell is at fCorrDistToWall from eye point, fViewPointHeight is the player's height, nHorHight is the
    // height of the horizon, nLayerHeight is the layer for this block and fWallHeight is the height of the map cell (in (0.0f, 1.0f] )
    // according to the map
    // The calculated results (on screen projections = OSP's) are passed in the two reference parameters.
    void CalculateBlockProjections( float fCorrDistToWall, float fViewPointHeight, int nHorHeight, int nLayerHeight, float fWallHeight, int &nOspTop, int &nOspBottom ) {
        // calculate projected slice height for a *unit height* wall (in screen space)
        int nSliceHeight = int((1.0f / fCorrDistToWall) * fDistToProjPlane);
        nOspTop    = nHorHeight - (nSliceHeight * (1.0f - fViewPointHeight)) - (nLayerHeight + fWallHeight - 1.0f) * nSliceHeight;
        nOspBottom = nOspTop + nSliceHeight * fWallHeight;
    }

// ==============================/   Mini map rendering prototypes   /==============================

    // function to render the mini map on the screen. If nRenderLevel == -1, all layers are taken into account.
    // Otherwise only the specified level is renderd in the minimap
    void RenderMap( int nRenderLevel = -1 );
    void RenderMapPlayer();      // function to render the player in the mini map on the screen
    void RenderMapRays( int nPlayerLevel );        // function to render the rays in the mini map on the screen
    void RenderMapObjects();     // function to render all the objects in the mini map on the screen
    void RenderPlayerInfo();      // function to render player info in a separate hud on the screen
    void RenderProcessInfo();     // function to render process info in a separate hud on the screen

    olc::Pixel ShadePixel( const olc::Pixel &p, float fDistance );	// Shade the pixel p using fDistance as a factor in the shade formula

    /* Recursion based slice renderer
     *   (fPx, fPy, fPh) = world space coordinate of player / viewpoint
     *   nSlice          = slice index (aka screen x coord)
     *   nStrtY, nStopY  = upper and lower y coordinate of subslice (screen space), use 0 and ScreenHeight() - 1 for full slice
     *   nHorHght        = height of horizon for this (sub)slice
     *   fAStep_deg      = the angle step in degrees per screen column
     * the vector of floats is the precalced cos value for each screen pixel y coordinate
     */
    void RenderSubSlice(
        int nRecursionDepth,
        int nExitDir,
        float fViewAngle_deg, float fCurAngle_deg,
        int nCurMap, float fPx, float fPy, float fPh,
        float fStrtDist,
        int nSlice, int nStrtY, int nStopY,
        int nHorHght,
        std::vector<float> &vDownAngleCos
    ) {

        // get a reference to the current map
        RC_Map *pCurMap = &vMaps[ nCurMap ];

        if (nRecursionDepth > nMaxRecursions) {
            nMaxRecursions = nRecursionDepth;
            nMaxRecSlice = nSlice;
        }

        int   nOspTopFrnt, nOspTopBack;   // to store the top and bottom y coord of the cell projection per column (screen space)
        int   nOspBotFrnt, nOspBotBack;

        typedef struct sDelayedPixel {
            float depth;    // for depth drawing
            int x, y;       // screen coordinates
            olc::Pixel p;
        } DelayedPixel;
        std::vector<DelayedPixel> vRenderLater;

        /////////////////////   SAMPLE LAMBDA's    /////////////////////////////

        // These lambdas calculate the sample coordinates for horizontal surfaces. They can be used for floors, roofs and ceilings.
        // fProjDistance is the distance from the player to the hit point on the surface.
        auto get_texel_u = [=]( float fProjDistance ) {
            // calculate the world coordinates from the distance and the view angle + player angle
            float fProjX = fPx + fProjDistance * lu_cos( fCurAngle_deg );
            // calculate the sample coordinates for that world coordinate. Wrap around if the result < 0 or >= 1
            float fSampleX = fProjX - int(fProjX);
            if (fSampleX <  0.0f) fSampleX += 1.0f;
            if (fSampleX >= 1.0f) fSampleX -= 1.0f;
            return fSampleX;
        };

        auto get_texel_v = [=]( float fProjDistance ) {
            // calculate the world coordinates from the distance and the view angle + player angle
            float fProjY = fPy + fProjDistance * lu_sin( fCurAngle_deg );
            // calculate the sample coordinates for that world coordinate. Wrap around if the result < 0 or >= 1
            float fSampleY = fProjY - int(fProjY);
            if (fSampleY <  0.0f) fSampleY += 1.0f;
            if (fSampleY >= 1.0f) fSampleY -= 1.0f;
            return fSampleY;
        };

        // this lambda returns a sample of the floor through the pixel at screen coord (px, py)
        auto get_floor_sample = [=]( int px, int py, float fDistOffset ) -> olc::Pixel {
            // work out the distance to the location on the floor you are looking at through this pixel
            float fFloorProjDistance;
            fFloorProjDistance = ((fPh / float( py - nHorHght )) * fDistToProjPlane );
            // it turns out that for ray casting into another level, the distance must be corrected so that it
            // reflects the distance from the portal into the other world
            fFloorProjDistance -= fDistOffset;
            fFloorProjDistance /= lu_cos( fViewAngle_deg );

            // calculate the texels from this distance
            float fSampleX = get_texel_u( fFloorProjDistance );
            float fSampleY = get_texel_v( fFloorProjDistance );
            // sample the pixel, shade it with the distance and return it
            // NOTE: for the depth drawing the uncorrected distance is needed
            return ShadePixel( pCurMap->GetFloorSpritePtr()->Sample( fSampleX, fSampleY ), fFloorProjDistance );
        };

        // This lambda performs much of the sampling proces of horizontal surfaces. It can be used for floors, roofs and ceilings etc.
        // fProjDistance is the distance from the player to the hit point on the surface.
        auto generic_sampling_cell = [=]( float fProjDistance, int nLevel, int nFaceID ) -> olc::Pixel {
            // calculate the world coordinates from the distance and the view angle + player angle
            float fProjX = fPx + fProjDistance * lu_cos( fCurAngle_deg );
            float fProjY = fPy + fProjDistance * lu_sin( fCurAngle_deg );
            // calculate the sample coordinates for that world coordinate, by subtracting the
            // integer part and only keeping the fractional part. Wrap around if the result < 0 or > 1
            float fSampleX = fProjX - int(fProjX); if (fSampleX < 0.0f) fSampleX += 1.0f; if (fSampleX >= 1.0f) fSampleX -= 1.0f;
            float fSampleY = fProjY - int(fProjY); if (fSampleY < 0.0f) fSampleY += 1.0f; if (fSampleY >= 1.0f) fSampleY -= 1.0f;

            // select the sprite to render the ceiling depending on the block that was hit
            int nTileX = std::clamp( int( fProjX ), 0, pCurMap->GetWidth()  - 1);
            int nTileY = std::clamp( int( fProjY ), 0, pCurMap->GetHeight() - 1);
            // obtain a pointer to the block that was hit
            RC_MapCell *auxMapCellPtr = pCurMap->MapCellPtrAt( nTileX, nTileY, nLevel );
            // sample that block passing the face that was hit and the sample coordinates
            olc::Pixel sampledPixel = (auxMapCellPtr == nullptr) ? olc::MAGENTA : auxMapCellPtr->Sample( nFaceID, fSampleX, fSampleY );
            // shade and return the pixel
            return ShadePixel( sampledPixel, fProjDistance );
        };

        // this lambda returns a sample of the roof through the pixel at screen coord (px, py)
        // NOTE: fHeightWithinLevel denotes the height of the hit point on the roof. This is typically the height of the block + its layer
        auto get_roof_sample = [=]( int px, int py, int nLevel, float fHeightWithinLevel, float &fRoofProjDistance ) -> olc::Pixel {
            // work out the distance to the location on the roof you are looking at through this pixel
            fRoofProjDistance = (( (fPh - (float( nLevel ) + fHeightWithinLevel)) / float( py - nHorHght )) * fDistToProjPlane) / lu_cos( fViewAngle_deg );
            // call the generic sampler to work out the rest
            return generic_sampling_cell( fRoofProjDistance, nLevel, FACE_TOP );
        };

        // this lambda returns a sample of the ceiling through the pixel at screen coord (px, py)
        // NOTE: fHeightWithinLevel denotes the height of the hit point on the ceiling. This is typically the layer of the block, WITHOUT its height!
        auto get_ceil_sample = [=]( int px, int py, int nLevel, float fDistOffset, float fHeightWithinLevel, float &fCeilProjDistance ) -> olc::Pixel {
            // work out the distance to the location on the ceiling you are looking at through this pixel
            float fCeilProjDistance_raw;
            fCeilProjDistance_raw = (( (float( nLevel ) - fPh) / float( nHorHght - py )) * fDistToProjPlane);
            // correct for the fish eye effect
            fCeilProjDistance     =  fCeilProjDistance_raw                / lu_cos( fViewAngle_deg );
            // for sampling into another map, we need to correct the distance with the distance to the portal face
            fCeilProjDistance_raw = (fCeilProjDistance_raw - fDistOffset) / lu_cos( fViewAngle_deg );

            // call the generic sampler to work out the rest
            return generic_sampling_cell( fCeilProjDistance_raw, nLevel, FACE_BOTTOM );
        };

        /////////////////////   OBTAIN HITPOINT INFO    /////////////////////////////

        // prepare the rendering for this slice by calculating the list of intersections along this ray
        // for each layer, get the list of hit points in that layer, work out front and back distances and
        // on screen projections, and add to the global vHitPointList
        std::vector<IntersectInfo> vHitPointList;
        for (int k = 0; k < pCurMap->NrOfLayers(); k++) {

            std::vector<IntersectInfo> vCurLevelList;
            CastRayPerLevelAndAngle( nCurMap, fPx, fPy, k, fCurAngle_deg, vCurLevelList );

            for (int i = 0; i < (int)vCurLevelList.size(); i++) {
                // make correction for the fish eye effect
                vCurLevelList[i].fDistFrnt_corr = vCurLevelList[i].fDistFrnt_raw * lu_cos( fViewAngle_deg );
                // add the start distance - this is needed since we're working with staged rendering through portals
                vCurLevelList[i].fDistFrnt_corr += fStrtDist;

                // calculate values for the on screen projections osp_top_frnt and top_bottom
                CalculateBlockProjections(
                    vCurLevelList[i].fDistFrnt_corr,
                    fPh,
                    nHorHght,
                    vCurLevelList[i].nLayer,
                    vCurLevelList[i].fHeight,
                    vCurLevelList[i].osp_top_frnt,
                    vCurLevelList[i].osp_bot_frnt
                );
            }
            // Extend the hit list with projected ceiling info for the back of the block
            for (int i = 0; i < (int)vCurLevelList.size(); i++) {
                if (i == (int)vCurLevelList.size() - 1) {
                    // last element, has no successor
                    vCurLevelList[i].fDistBack_raw  = vCurLevelList[i].fDistFrnt_raw;
                    vCurLevelList[i].fDistBack_corr = vCurLevelList[i].fDistFrnt_corr;
                    vCurLevelList[i].osp_top_back   = vCurLevelList[i].osp_top_frnt;
                    vCurLevelList[i].osp_bot_back   = vCurLevelList[i].osp_bot_frnt;
                } else {
                    // calculate values for the on screen projections top_front and top_bottom
                    vCurLevelList[i].fDistBack_raw  = vCurLevelList[i + 1].fDistFrnt_raw;
                    vCurLevelList[i].fDistBack_corr = vCurLevelList[i + 1].fDistFrnt_corr;
                    CalculateBlockProjections(
                        vCurLevelList[i].fDistBack_corr,
                        fPh,
                        nHorHght,
                        vCurLevelList[i].nLayer,
                        vCurLevelList[i].fHeight,
                        vCurLevelList[i].osp_top_back,
                        vCurLevelList[i].osp_bot_back
                    );
                }
            }

            // NOTE - shouldn't vRayList be a reference parameter?

            // populate ray list for rendering mini map - note, only the current map, not recursively
            if (bMinimap && nRecursionDepth == 0 && !vCurLevelList.empty()) {
                RayType curHitPoint = { { fPx, fPy }, { vCurLevelList[0].fHitX, vCurLevelList[0].fHitY }, vCurLevelList[0].nLayer };
                vRayList.push_back( curHitPoint );
            }
            // add the hit points for this layer list to the combined hit point list
            vHitPointList.insert( vHitPointList.end(), vCurLevelList.begin(), vCurLevelList.end());
        }

        // remove all hit points with height 0.0f - they were necessary for calculating the back face projection
        // of blocks, but we don't need them anymore
        vHitPointList.erase(
            std::remove_if(
                vHitPointList.begin(),
                vHitPointList.end(),
                []( IntersectInfo &a ) {
                    return a.fHeight == 0.0f;
                }
            ),
           vHitPointList.end()
        );

        // if test mode is triggered, print the hit list along the test slice
        if (bTestMode && nSlice == int( fTestSlice )) {
            int nMap = -1;
            for (int i = 0; i < (int)vMaps.size() && nMap == -1; i++) {
                if (pCurMap == &vMaps[i]) {
                    nMap = i;
                }
            }
            std::cout << "Map: " << nMap << std::endl;
            PrintHitList( vHitPointList, true );
        }

            /////////////////////   RENDER BACKGROUND    /////////////////////////////

            // start rendering this sub slice by putting sky and floor in it
        float fWellAway = fMaxDistance + 1000.0f;
        olc::Pixel skySample = pCurMap->GetSkyColour();
            for (int y = nStrtY; y <= nStopY; y++) {
            // draw floor and horizon
            if (y < nHorHght) {
                cDDrawer.Draw( fWellAway, nSlice, y, skySample );
            } else {
                olc::Pixel floorSample = get_floor_sample( nSlice, y, fStrtDist );   // distance needs to be corrected
                cDDrawer.Draw( fWellAway, nSlice, y, floorSample );
            }
        }

        // now render all hit points (i.e. wall sub slices) back to front
        for (auto &hitRec : vHitPointList) {
            // For the distance calculations we needed also points where the height returns to 0.0f (the
            // back faces of the block). For the rendering we must skip these "hit points"
            if (hitRec.fHeight > 0.0f) {

                // make sure the screen y coordinate is within sub slice boundaries
                nOspTopFrnt = std::clamp( hitRec.osp_top_frnt, nStrtY, nStopY );
                nOspTopBack = std::clamp( hitRec.osp_top_back, nStrtY, nStopY );
                nOspBotFrnt = std::clamp( hitRec.osp_bot_frnt, nStrtY, nStopY );
                nOspBotBack = std::clamp( hitRec.osp_bot_back, nStrtY, nStopY );

                // get a pointer to the map cell that was hit
                RC_MapCell *auxMapCellPtr = pCurMap->MapCellPtrAt( hitRec.nHitX, hitRec.nHitY, hitRec.nLayer );
                // get a pointer to the top face for roof rendering
                RC_Face *auxFacePtr = auxMapCellPtr->GetFacePtr( FACE_TOP );
                // render roof segment if it's visible (if top back >= top front, roof is not visible and nothing will be rendered)
                for (int y = nOspTopBack; y < nOspTopFrnt; y++) {
                    // the distance to this point is calculated and passed from get_roof_sample
                    float fRenderDistance;
                    olc::Pixel roofSample = get_roof_sample( nSlice, y, hitRec.nLayer, hitRec.fHeight, fRenderDistance );   // shading is done in get_roof_sample()

                    // either render or store for later rendering, depending on face transparency
                    if (auxFacePtr->IsTransparent()) {
                        DelayedPixel aux = { fRenderDistance / vDownAngleCos[y], nSlice, y, roofSample };
                        vRenderLater.push_back( aux );
                    } else {
                        cDDrawer.Draw( fRenderDistance / vDownAngleCos[y], nSlice, y, roofSample );
                    }
                }

                // render wall segment - first check if other map must be rendered via portal, then render this wall face on top of it

                if (auxMapCellPtr->IsPortal()) {

                    // prevent infinite recursion - only call recursively if the sub slice to work out is significant
                    if (nStopY > nStrtY) {

                        // a recursive call must be made here to insert the sub slice from the other map
                        RC_MapCellPortal *auxMCPtr = (RC_MapCellPortal *)auxMapCellPtr;

                        float fOtherX, fOtherY;
                        int nDeltaX = auxMCPtr->GetToX() - int( hitRec.fHitX );
                        int nDeltaY = auxMCPtr->GetToY() - int( hitRec.fHitY );

                        switch ( auxMCPtr->GetExitDir() ) {
                            // in cases where you need to add 1.0f, add slightly less, to prevent out of bounds conditions
                            case FACE_EAST : fOtherX = auxMCPtr->GetToX()           ; fOtherY = hitRec.fHitY + nDeltaY;        break;
                            case FACE_WEST : fOtherX = auxMCPtr->GetToX() + 0.99999f; fOtherY = hitRec.fHitY + nDeltaY;        break;
                            case FACE_SOUTH: fOtherX = hitRec.fHitX + nDeltaX;        fOtherY = auxMCPtr->GetToY()           ; break;
                            case FACE_NORTH: fOtherX = hitRec.fHitX + nDeltaX;        fOtherY = auxMCPtr->GetToY() + 0.99999f; break;
                            default: std::cout << "ERROR: RenderSubSlice() --> this exit direction doesn't implement" << std::endl;
                        }

                        RenderSubSlice(
                            nRecursionDepth + 1,
                            auxMCPtr->GetExitDir(),
                            fViewAngle_deg,
                            fCurAngle_deg,

                            auxMCPtr->GetToMap(),
                            fOtherX, fOtherY, fPh,

                            hitRec.fDistFrnt_corr,
                            nSlice, nOspTopFrnt, nOspBotFrnt,
                            nHorHght,
                            vDownAngleCos
                        );
                    }
                }

                // now also render the wall part, this enables for transparent portals
                float fSampleX = -1.0f;
                for (int y = nOspTopFrnt; y <= nOspBotFrnt; y++) {

                    // first get x sample coordinate from face hit info
                    if (fSampleX == -1.0f) {
                        switch (hitRec.nFaceHit) {
                            case FACE_SOUTH:
                            case FACE_NORTH: fSampleX = hitRec.fHitX - (float)hitRec.nHitX; break;
                            case FACE_EAST :
                            case FACE_WEST : fSampleX = hitRec.fHitY - (float)hitRec.nHitY; break;
                            default        : std::cout << "ERROR: RenderSubSlice() --> invalid face value: " << hitRec.nFaceHit << std::endl;
                        }
                    }

                    // the y sample coordinate depends only on the pixel y coord on the screen in relation to the vertical space the wall is taking up
                    float fSampleY = hitRec.fHeight * float(y - hitRec.osp_top_frnt) / float(hitRec.osp_bot_frnt - hitRec.osp_top_frnt);
                    // sample that block passing the face that was hit and the sample coordinates
                    olc::Pixel sampledPixel = (auxMapCellPtr == nullptr) ? olc::MAGENTA : auxMapCellPtr->Sample( hitRec.nFaceHit, fSampleX, fSampleY );
                    // shade the pixel
                    olc::Pixel wallSample =  ShadePixel( sampledPixel, hitRec.fDistFrnt_corr );

                    // either render or store for later rendering, depending on face transparency
                    RC_Face *auxFacePtr = auxMapCellPtr->GetFacePtr( hitRec.nFaceHit );
                    if (auxFacePtr->IsTransparent()) {
                        DelayedPixel aux = { hitRec.fDistFrnt_corr / vDownAngleCos[y], nSlice, y, wallSample };
                        vRenderLater.push_back( aux );
                    } else {
                        cDDrawer.Draw( hitRec.fDistFrnt_corr / vDownAngleCos[y], nSlice, y, wallSample );
                    }
                }

                // get a pointer to the bottom face for ceiling rendering
                auxFacePtr = auxMapCellPtr->GetFacePtr( FACE_BOTTOM );
                // render ceiling segment if it's visible
                for (int y = nOspBotFrnt + 1; y <= nOspBotBack; y++) {
                    float fRenderDistance;
                    olc::Pixel ceilSample = get_ceil_sample( nSlice, y, hitRec.nLayer, fStrtDist, hitRec.fHeight, fRenderDistance );   // shading is done in get_ceil_sample()

                    // either render or store for later rendering, depending on face transparency
                    if (auxFacePtr->IsTransparent()) {
                        DelayedPixel aux = { fRenderDistance / vDownAngleCos[y], nSlice, y, ceilSample };
                        vRenderLater.push_back( aux );
                    } else {
                        cDDrawer.Draw( fRenderDistance / vDownAngleCos[y], nSlice, y, ceilSample );
                    }
                }
            }

            // DELAYED WALL RENDERING for this slice (with masking of blank pixels)
            // ======================
            for (auto &elt : vRenderLater) {
                if (elt.p != olc::BLANK) {
                    cDDrawer.Draw( elt.depth, elt.x, elt.y, elt.p );
                }
            }
        }
    }

    // this var is used to keep track of door opening or closing
    int nTestAnimState = ANIM_STATE_CLOSED;

    bool OnUserUpdate( float fElapsedTime ) override {

        // step 1 - user input
        // ===================

        // For all movements and rotation you can speed up by keeping SHIFT pressed
        // or speed down by keeping CTRL pressed. This also affects shading/lighting
        float fSpeedUp = 1.0f;
        if (GetKey( olc::SHIFT ).bHeld) fSpeedUp = 3.0f;
        if (GetKey( olc::CTRL  ).bHeld) fSpeedUp = 0.2f;

        // set test mode and test slice values
        bTestMode = GetKey( olc::Key::T ).bPressed;
        if (GetKey( olc::Key::F1 ).bHeld) fTestSlice = std::max( fTestSlice - 40.0f * fElapsedTime * fSpeedUp,                 0.0f );
        if (GetKey( olc::Key::F2 ).bHeld) fTestSlice = std::min( fTestSlice + 40.0f * fElapsedTime * fSpeedUp, ScreenWidth() - 1.0f );

        // reset look up value and player height on pressing 'R'
        if (GetKey( olc::R ).bReleased) { fPlayerH = 0.5f; fPlayerLU = 0.0f; }

        // toggles for HUDs
        if (GetKey( olc::U ).bPressed) bProcessInfo = !bProcessInfo;
        if (GetKey( olc::I ).bPressed) bPlayerInfo  = !bPlayerInfo;
        if (GetKey( olc::P ).bPressed) bMinimap     = !bMinimap;
        if (GetKey( olc::O ).bPressed) bMapRays     = !bMapRays;
        // toggles for on screen orientation lines
        if (GetKey( olc::G ).bPressed) bTestSlice   = !bTestSlice;
        if (GetKey( olc::H ).bPressed) bTestGrid    = !bTestGrid;

        // Rotate - collision detection not necessary. Keep fPlayerA_deg between 0 and 360 degrees
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }
        // Rotate to discrete angle
        if (GetKey( olc::NP6 ).bPressed) { fPlayerA_deg =   0.0f; }
        if (GetKey( olc::NP3 ).bPressed) { fPlayerA_deg =  45.0f; }
        if (GetKey( olc::NP2 ).bPressed) { fPlayerA_deg =  90.0f; }
        if (GetKey( olc::NP1 ).bPressed) { fPlayerA_deg = 135.0f; }
        if (GetKey( olc::NP4 ).bPressed) { fPlayerA_deg = 180.0f; }
        if (GetKey( olc::NP7 ).bPressed) { fPlayerA_deg = 225.0f; }
        if (GetKey( olc::NP8 ).bPressed) { fPlayerA_deg = 270.0f; }
        if (GetKey( olc::NP9 ).bPressed) { fPlayerA_deg = 315.0f; }

        // variables used for collision detection - work out the new location in a separate coordinate pair, and only alter
        // the players coordinate if there's no collision
        float fNewX = fPlayerX;
        float fNewY = fPlayerY;

        // walking forward, backward and strafing left, right
        if (GetKey( olc::W ).bHeld) { fNewX += lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY += lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk forward
        if (GetKey( olc::S ).bHeld) { fNewX -= lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY -= lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk backwards

        if (GetKey( olc::Q ).bHeld) { fNewX += lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY -= lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe left
        if (GetKey( olc::E ).bHeld) { fNewX -= lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY += lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe right
        // collision detection - only update position if no collision
        if (!vMaps[ nActiveMap ].Collides( fNewX, fNewY, fPlayerH, RADIUS_PLAYER, 0.0f, 0.0f )) {
            fPlayerX = fNewX;
            fPlayerY = fNewY;
        }

        // looking up or down - collision detection not necessary
        // NOTE - there's no clamping to extreme values (yet)
        if (GetKey( olc::UP   ).bHeld) { fPlayerLU += SPEED_LOOKUP * fSpeedUp * fElapsedTime; }
        if (GetKey( olc::DOWN ).bHeld) { fPlayerLU -= SPEED_LOOKUP * fSpeedUp * fElapsedTime; }

        // flying or crouching
        // NOTE - for multi layer rendering there's only clamping to keep fPlayerH > 0.0f, there's no upper limit.

        // cache current height of horizon, so that you can compensate for changes in it via the look up value
        float fCacheHorHeight = float( ScreenHeight() * fPlayerH ) + fPlayerLU;
        if (MULTI_LAYERS) {
            // if the player height is adapted, keep horizon height stable by compensating with look up value
            if (GetKey( olc::PGUP ).bHeld) {
                float fNewHeight = fPlayerH + SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                // do CD on the height map - player velocity is not relevant since movement is up/down
                if (!vMaps[ nActiveMap ].Collides( fPlayerX, fPlayerY, fNewHeight, 0.1f, 0.0f, 0.0f )) {
                    fPlayerH = fNewHeight;
                    fPlayerLU = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
            if (GetKey( olc::PGDN ).bHeld) {
                float fNewHeight = fPlayerH - SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                // prevent negative height, and do CD on the height map - player velocity is not relevant since movement is up/down
                if (!vMaps[ nActiveMap ].Collides( fPlayerX, fPlayerY, fNewHeight, 0.1f, 0.0f, 0.0f )) {
                    fPlayerH  = fNewHeight;
                    fPlayerLU = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
        } else {
            if (GetKey( olc::PGUP ).bHeld) {
                float fNewHeight = fPlayerH + SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                if (fNewHeight < 1.0f) {
                    fPlayerH = fNewHeight;
                    // compensate look up value so that horizon remains stable
                    fPlayerLU = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
            if (GetKey( olc::PGDN ).bHeld) {
                float fNewHeight = fPlayerH - SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                if (fNewHeight > 0.0f) {
                    fPlayerH = fNewHeight;
                    // compensate look up value so that horizon remains stable
                    fPlayerLU = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
        }

        // alter object intensity and multiplier - for shading
        if (GetKey( olc::INS  ).bHeld) fObjectIntensity     += INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::DEL  ).bHeld) fObjectIntensity     -= INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::HOME ).bHeld) fIntensityMultiplier += INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::END  ).bHeld) fIntensityMultiplier -= INTENSITY_SPEED * fSpeedUp * fElapsedTime;


        // step 2 - game logic
        // ===================

        bool bStateChanged = false;
        // directly setting to opened or closed is not useful. State can only become Opening if it was closed, and vice versa
        if (GetKey( olc::F6 ).bPressed) { bStateChanged = true; nTestAnimState = ANIM_STATE_CLOSING; }
        if (GetKey( olc::F5 ).bPressed) { bStateChanged = true; nTestAnimState = ANIM_STATE_OPENING; }

        // little lambda returns whether distance between b and c is <= a (note - sqrt not needed here)
        auto within_distance = [=]( int a, int b, int c ) {
            return (b * b + c * c) <= (a * a);
        };
        // iterate over all the map cells in the map
        // the break out bool is needed in case a portal transition occurs, which should
        // abruptly stop the updating since the player stepped into another map and the player variables
        // (and loop control variables) are not valid in the current map anymore
        bool bBreakOut = false;
        for (int h = 0; h < vMaps[ nActiveMap ].NrOfLayers() && !bBreakOut; h++) {
            for (int y = 0; y < vMaps[ nActiveMap ].GetHeight() && !bBreakOut; y++) {
                for (int x = 0; x < vMaps[ nActiveMap ].GetWidth() && !bBreakOut; x++) {

                    // grab a pointer to the current map cell
                    RC_MapCell *pMapCell = vMaps[ nActiveMap ].MapCellPtrAt( x, y, h );
                    if (!pMapCell->IsEmpty()) {
                        // update this map cell (this will update all it's faces)
                        bool bTmp = pMapCell->IsPermeable();
                        pMapCell->Update( fElapsedTime, bTmp );
                        pMapCell->SetPermeable( bTmp );

                        // test code for manually changing state of animated faces
                        for (int i = 0; i < FACE_NR_OF; i++) {
                            RC_Face *facePtr = pMapCell->GetFacePtr( i );
                            if (facePtr->IsAnimated()) {
                                // only trigger gate if close enough
                                if (bStateChanged &&
                                    within_distance( SENSE_RADIUS, x + 0.5f - fPlayerX, y + 0.5f - fPlayerY )) {
                                    // You must cast to RC_FaceAnimated * to get the function working properly...
                                    ((RC_FaceAnimated *)facePtr)->SetState( nTestAnimState );
                                }
                            }
                        }
                        if (pMapCell->IsPortal()) {
                            RC_MapCellPortal *auxPtr = (RC_MapCellPortal *)pMapCell;
                            if (auxPtr->SwapToOtherSide( fPlayerH, fPlayerX, fPlayerY )) {
                                int nOtherMap = auxPtr->GetToMap();
                                int nOtherL   = auxPtr->GetToLevel();
                                int nOtherX   = auxPtr->GetToX();
                                int nOtherY   = auxPtr->GetToY();
                                float fOtherL = fPlayerH - int(fPlayerH) + nOtherL;
                                float fOtherX = fPlayerX - int(fPlayerX) + nOtherX;
                                float fOtherY = fPlayerY - int(fPlayerY) + nOtherY;

                                std::cout << "Map transition from map: " << nActiveMap << " to: " << nOtherMap << std::endl;

                                nActiveMap = nOtherMap;
                                fPlayerH = fOtherL;
                                fPlayerX = fOtherX;
                                fPlayerY = fOtherY;

                                bBreakOut = true;
                            }
                        } // iterate faces
                    } // else - block is empty, skip it
                } // iterate x
            } // iterate y
        } // iterate layers

        // update all objects in active map
        for (auto &elt : vMaps[nActiveMap].vListObjects) {
            elt.Update( &vMaps[ nActiveMap ], fElapsedTime );
        }

        // step 3 - render
        // ===============


        // WALL RENDERING (aka BACK GROUND SCENE rendering)
        // ==============

        // typically, the horizon height is halfway the screen height. However, you have to offset with look up value,
        // and the viewpoint of the player is variable too (since flying and crouching)
        int nHorizonHeight = ScreenHeight() * fPlayerH + (int)fPlayerLU;
        float fAngleStep_deg = fPlayerFoV_deg / float( ScreenWidth());

        // having set the horizon height, determine the cos of all the angles through each of the pixels in this slice
        std::vector<float> fHeightAngleCos( ScreenHeight() );
        for (int y = 0; y < ScreenHeight(); y++) {
            fHeightAngleCos[y] = std::abs( lu_cos( (y - nHorizonHeight) * fAnglePerPixel_deg ));
        }

        // clear depth buffer
        cDDrawer.Reset();

        // reset recursion statistics
        nMaxRecursions =  0;
        nMaxRecSlice   = -1;
        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle_deg = float( x - (ScreenWidth() / 2)) * fAngleStep_deg;
            float fCurAngle_deg = fPlayerA_deg + fViewAngle_deg;
            // render this slice - possibly recursively rendering subslices with the main slice
            RenderSubSlice( 0, FACE_UNKNOWN, fViewAngle_deg, fCurAngle_deg, nActiveMap, fPlayerX, fPlayerY, fPlayerH, 0.0f, x, 0, ScreenHeight() - 1, nHorizonHeight, fHeightAngleCos );
        }

        // OBJECT RENDERING
        // ================

        // display all objects after the background rendering and before displaying the minimap or debugging output
        // split the rendering into two phase so that it can be sorted on distance (painters algo) before rendering

        // phase 1 - just determine distance (and angle cause of convenience)
        for (auto &object : vMaps[nActiveMap].vListObjects) {

            // work out distance and angle between object and player, and
            // store it in the object itself
            object.PrepareRender( fPlayerX, fPlayerY, fPlayerA_deg );
        }

        // sort farthest object first (for painters algo)
        vMaps[nActiveMap].vListObjects.sort(
            [=]( RC_Object &a, RC_Object &b ) {
                return a.GetDistToPlayer() > b.GetDistToPlayer();
            }
        );

        // phase 2: render object
        for (auto &object : vMaps[nActiveMap].vListObjects) {
            object.Render( cDDrawer, fPlayerH, fPlayerFoV_rad, fMaxDistance, nHorizonHeight );
        }

        // TEST STUFF RENDERING
        // ====================

        // to aim the slice that is output on testmode
        if (bTestSlice) {
            DrawLine( int( fTestSlice ), 0, int( fTestSlice ), ScreenHeight() - 1, olc::MAGENTA );
        }

        // horizontal grid lines for testing
        if (bTestGrid) {
            for (int i = 0; i < ScreenHeight(); i+= 100) {
                for (int j = 0; j < 100; j+= 10) {
                    DrawLine( 0, i + j, ScreenWidth() - 1, i + j, olc::BLACK );
                }
                DrawLine( 0, i, ScreenWidth() - 1, i, olc::DARK_GREY );
                DrawString( 0, i - 5, std::to_string( i ), olc::WHITE );
            }
        }

        // MINIMAP & HUD RENDERING
        // =======================

        if (bMinimap) {
            RenderMap( 0 );
            if (bMapRays) {
                RenderMapRays( int( fPlayerH ));
            }
            RenderMapPlayer();
            RenderMapObjects();

            vRayList.clear();
        }

        if (bPlayerInfo) {
            RenderPlayerInfo();
        }

        if (bProcessInfo) {
            RenderProcessInfo();
        }

        return true;
    }

    bool OnUserDestroy() {

		for (int i = 0; i < (int)vMaps.size(); i++) {
	        vMaps[i].FinalizeMap();
		}

        return true;
    }
};

int main()
{
	MyRayCaster demo;
	if (demo.Construct( SCREEN_X / PIXEL_SIZE, SCREEN_Y / PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE ))
		demo.Start();

	return 0;
}

//////////////////////////////////   put bloat behind main()  /////////////////////////////////

// ==============================/   Mini map rendering stuff   /==============================

// function to render the mini map on the screen
void MyRayCaster::RenderMap( int nRenderLevel ) {

    auto local_map_cell_height = [=]( int nLayer, int x, int y ) {
        if (nLayer < 0) return vMaps[ nActiveMap ].CellHeight( x, y );
        if (nLayer > vMaps[ nActiveMap ].NrOfLayers()) return 0.0f;
        return vMaps[ nActiveMap ].CellHeightAt( x, y, nRenderLevel );
    };
    // fill background for minimap
    float fMMFactor = MINIMAP_SCALE_FACTOR * MINIMAP_TILE_SIZE;
    FillRect( 0, 0, vMaps[ nActiveMap ].GetWidth() * fMMFactor, vMaps[ nActiveMap ].GetHeight() * fMMFactor, COL_HUD_BG );
    // draw each tile
    for (int y = 0; y < vMaps[ nActiveMap ].GetHeight(); y++) {
        for (int x = 0; x < vMaps[ nActiveMap ].GetWidth(); x++) {
            // colour different for different heights
            olc::Pixel p;
            bool bBorderFlag = true;
            if (local_map_cell_height( nRenderLevel, x, y ) == 0.0f) {
                p = COL_HUD_BG;   // don't visibly render
                bBorderFlag = false;
            } else if (local_map_cell_height( nRenderLevel, x, y )  <  1.0f) {
                p = olc::PixelF( vMaps[ nActiveMap ].CellHeight( x, y ), 0.0f, 0.0f );    // height < 1.0f = shades of red
            } else {
                float fColFactor = std::min( vMaps[ nActiveMap ].CellHeight( x, y ) / 4.0f + 0.5f, 1.0f );    // heights > 1.0f = shades of blue
                p = olc::PixelF( 0.0f, 0.0f, fColFactor );
            }
            // render this tile
            FillRect( x * fMMFactor + 1, y * fMMFactor + 1, fMMFactor - 1, fMMFactor - 1, p );
            if (bBorderFlag) {
                p = olc::WHITE;

                RC_MapCell *pMapCellPtr = vMaps[ nActiveMap ].MapCellPtrAt( x, y, 0 );

                if (pMapCellPtr != nullptr && pMapCellPtr->IsPortal()) {
                    p = olc::RED;
                }

                DrawRect( x * fMMFactor, y * fMMFactor, fMMFactor, fMMFactor, p);
            }
        }
    }
}

// function to render the player in the mini map on the screen
void MyRayCaster::RenderMapPlayer() {
    float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
    olc::Pixel p = olc::YELLOW;
    float px = fPlayerX * fMMFactor;
    float py = fPlayerY * fMMFactor;
    float pr = 0.6f     * fMMFactor;

    // draw sense radius around player - let it blend to get it semi-transparent
    SetPixelBlend( SENSE_BLENDF );
    SetPixelMode( olc::Pixel::ALPHA );
    FillCircle( px, py, SENSE_RADIUS * fMMFactor, olc::DARK_GREY );
    SetPixelMode( olc::Pixel::NORMAL );

    // Draw player object itself
    FillCircle( px, py, pr, p );
    // Draw player direction pointer
    float dx = lu_cos( fPlayerA_deg );
    float dy = lu_sin( fPlayerA_deg );
    float pdx = dx * 2.0f * fMMFactor;
    float pdy = dy * 2.0f * fMMFactor;
    DrawLine( px, py, px + pdx, py + pdy, p );
}

// function to render the rays in the mini map on the screen
void MyRayCaster::RenderMapRays( int nPlayerLevel ) {
    // choose different colour for each layer
    auto get_layer_col = [=]( int nLvl ) {
        olc::Pixel result = olc::WHITE;
        switch (nLvl) {
            case 0 : result = olc::GREEN;   break;
            case 1 : result = olc::RED;     break;
            case 2 : result = olc::BLUE;    break;
            case 3 : result = olc::GREY;    break;
            case 4 : result = olc::MAGENTA; break;
            default: result = olc::YELLOW;  break;
        }
        return result;
    };

    float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
    // draw an outline of the visible part of the world
    // use a different colour per layer
    olc::Pixel layerCol = get_layer_col( nPlayerLevel );
    for (auto &elt : vRayList) {
    if (elt.layer == nPlayerLevel) {
            DrawLine(
            elt.pointA.x * fMMFactor,
            elt.pointA.y * fMMFactor,
            elt.pointB.x * fMMFactor,
            elt.pointB.y * fMMFactor,
                layerCol
            );
        }
    }
}

// function to render all the objects in the mini map on the screen
void MyRayCaster::RenderMapObjects() {
    float fMMFactor = MINIMAP_TILE_SIZE * MINIMAP_SCALE_FACTOR;
    for (auto &elt : vMaps[nActiveMap].vListObjects) {

        olc::Pixel p = (elt.bStationary ? olc::RED : olc::MAGENTA);

        float px = elt.GetX() * fMMFactor;
        float py = elt.GetY() * fMMFactor;
        float pr = 0.4f  * fMMFactor;
        FillCircle( px, py, pr, p );

        if (!elt.bStationary) {
            float dx = lu_cos( rad2deg( elt.GetAngle()));
            float dy = lu_sin( rad2deg( elt.GetAngle()));
            float pdx = dx * 0.3f * elt.GetSpeed() * fMMFactor;
            float pdy = dy * 0.3f * elt.GetSpeed() * fMMFactor;
            DrawLine( px, py, px + pdx, py + pdy, p );
        }
    }
}

// function to render player info in a separate hud on the screen
void MyRayCaster::RenderPlayerInfo() {
    int nStartX = ScreenWidth() - 200;
    int nStartY =  10;
    // render background pane for debug info
    FillRect( nStartX, nStartY, 190, 65, COL_HUD_BG );
    // output player and rendering values for debugging
    DrawString( nStartX + 5, nStartY +   5, "X      = " + std::to_string( fPlayerX     ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  15, "Y      = " + std::to_string( fPlayerY     ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  25, "H      = " + std::to_string( fPlayerH     ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  35, "Angle  = " + std::to_string( fPlayerA_deg ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  55, "LookUp = " + std::to_string( fPlayerLU    ), COL_HUD_TXT );
}

// function to render performance info in a separate hud on the screen
void MyRayCaster::RenderProcessInfo() {
    int nStartX = ScreenWidth()  - 200;
    int nStartY = ScreenHeight() - 200;
    // render background pane for debug info
    FillRect( nStartX, nStartY, 195, 125, COL_HUD_BG );
    // output player and rendering values for debugging
    DrawString( nStartX + 5, nStartY +  5, "Intensity  = " + std::to_string( fObjectIntensity        ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY + 15, "Multiplier = " + std::to_string( fIntensityMultiplier    ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY + 25, "# Objects  = " + std::to_string( (int)vMaps[ nActiveMap ].vListObjects.size()), COL_HUD_TXT );

    DrawString( nStartX + 5, nStartY +  45, "Recurs. depth  = " + std::to_string( nMaxRecursions      ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  55, "@ slice        = " + std::to_string( nMaxRecSlice        ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  65, "Test slice     = " + std::to_string( int( fTestSlice )   ), COL_HUD_TXT );

    DrawString( nStartX + 5, nStartY +  85, "Acive map      = " + std::to_string( nActiveMap                      ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY +  95, "Map size - X   = " + std::to_string( vMaps[ nActiveMap ].GetWidth()  ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY + 105, "Map size - Y   = " + std::to_string( vMaps[ nActiveMap ].GetHeight() ), COL_HUD_TXT );
    DrawString( nStartX + 5, nStartY + 115, "Map size - Z   = " + std::to_string( vMaps[ nActiveMap ].NrOfLayers()), COL_HUD_TXT );

}

// Shade the pixel p using fDistance as a factor in the shade formula
olc::Pixel MyRayCaster::ShadePixel( const olc::Pixel &p, float fDistance ) {
    if (RENDER_SHADED) {
        float fShadeFactor = std::max( SHADE_FACTOR_MIN, std::min( SHADE_FACTOR_MAX, fObjectIntensity * ( fIntensityMultiplier /  fDistance )));
        return p * fShadeFactor;
    } else
        return p;
}

// ==============================/  end of file   /==============================

