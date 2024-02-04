// ELABORATING ON - Ray casting tutorial by Permadi
// (starting with part 20 all code files are my own elaboration on the Permadi basis)
//
// Implementation of part 22 b - holes, overhangs and floating blocks (adapted representation of ray cast maps)
//
// Joseph21, april 22, 2023
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, roofs, floor and ceiling - use your own .png files and adapt in OnUserCreate()

/* Short description
   -----------------
   This implementation is a follow up of implementation part 22 a. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

   Having added a new class RC_Map this version of the program is the first rebuild of the map representation in layers. The map is predefined 
   as a number of 2d string variables, and added as layers one by one. 
   
   Note: there are no holes or overhangs or floating blocks yet, just rendering the same map using a different internal representation.


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
#define OBJECT_INTENSITY       5.0f   // for testing, reset to 1.5f afterwards!
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
#define MINIMAP_TILE_SIZE     32     // each minimap tile is ... pixels
#define MINIMAP_SCALE_FACTOR   0.2   // should be 0.2

// test objects
#define NR_TEST_OBJECTS   0

// ==============================/  Prototypes look up trig functions  /==============================

float deg2rad( float fAngleDeg );
float rad2deg( float fAngleRad );

float deg_mod2pi( float fAngleDeg );
float rad_mod2pi( float fAngleRad );

void init_lu_sin_array();
void init_lu_cos_array();

float lu_sin( float fDegreeAngle );
float lu_cos( float fDegreeAngle );

// ==============================/  map definitions here   /==============================

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
// you can also set blocks of height 0.1f, 0.2f, etc by specifying '1', '2', etc resp.

// ==============================/  predefined maps /==============================

// level 0 - the blocks that are standing on ground level
std::string sMap_level0 =   // keep all the 1-9 and # cells unchanged. Replace all other non empty ones with #
//   0         1         2         3         4         5
//   0123456789012345678901234567890123456789012345678901
    "##############.."
    "#..............#"
    "#..............#"
    "#..............."
    "#..............."
    "#..............."
    "#..............#"
    "#..............#"
    "#..............#"
    "#.#............#"
    "#..............#"
    "#..............#"
    "#..............#"
    "#..............#"
    "...............#"
    ".###.#####.####.";

std::string sMap_level1 =
//   0         1         2         3         4         5
//   0123456789012345678901234567890123456789012345678901
    "#..............."
    "................"
    ".......#.......Q"
    "...............H"
    "...............T"
    "..........#....#"
    "...............#"
    "...............#"
    "...............#"
    "..##............"
    "...............#"
    "................"
    "...............#"
    "................"
    "...............#"
    "..##..##.######.";

std::string sMap_level2 =
//   0         1         2         3         4         5
//   0123456789012345678901234567890123456789012345678901
    "#..............."
    "................"
    "................"
    "................"
    "..........#....."
    "...............H"
    "...............#"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "...####......#..";

std::string sMap_level3 =
//   0         1         2         3         4         5
//   0123456789012345678901234567890123456789012345678901
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "....##..........";

// ==============================/   Class RC_Map   /==============================

class RC_Map {

public:
    // you can either initialize the map in one go, calling the next function...
    void InitMap( int nSizeX, int nSizeY, const std::string &sUserMap ) {

        // NOTE - this function in its current implementation has no layers - instead
        // the height value of the fMap can be more than 1.0f

        if (nSizeX * nSizeY != (int)sUserMap.length()) {
            std::cout << "ERROR: InitMap() -->  mismatch between map dimensions and length of map string" << std::endl;
        }

        std::string sMap = sUserMap;
        nMapX = nSizeX;
        nMapY = nSizeY;
        // Initialise fMap as a 2d array of floats, having the same size as sMap, and containing the height per cell.
        // NOTE - if MULTIPLE_LEVELS is false, the fMap will contain no values > 1
        float *fMap = new float[nMapX * nMapY];
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
        sMaps.push_back( sMap );
        fMaps.push_back( fMap );
    }

    // ... or you can initialize the map in steps using the following two functions
    void InitMap( int nSizeX, int nSizeY ) {
        nMapX = nSizeX;
        nMapY = nSizeY;
    }

    void AddLayer( const std::string &sUserMap ) {
        if (nMapX * nMapY != (int)sUserMap.length()) {
            std::cout << "ERROR: InitMap() -->  mismatch between map dimensions and length of map string" << std::endl;
        }

        std::string sMap = sUserMap;

        // Initialise fMap as a 2d array of floats, having the same size as sMap, and containing the height per cell.
        // NOTE - if MULTIPLE_LEVELS is false, the fMap will contain no values > 1
        float *fMap = new float[nMapX * nMapY];
        for (int y = 0; y < nMapY; y++) {
            for (int x = 0; x < nMapX; x++) {
                switch (sMap[ y * nMapX + x ]) {
                    case GRND_FLOOR : fMap[ y * nMapX + x ] = 0.0f; break;
                    case FRST_FLOOR : fMap[ y * nMapX + x ] = 1.0f; break;

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

                    default         : std::cout << "ERROR: AddLayer() --> unknown sMap value: " << sMap[ y * nMapX + x ] << std::endl;
                }
            }
        }
        sMaps.push_back( sMap );
        fMaps.push_back( fMap );
    }

    // method to clean up the object before it gets out of scope
    void FinalizeMap() {
        for (auto &elt : fMaps ) {
            delete elt;
        }
        sMaps.clear();
        fMaps.clear();
    }

    // getters for map width and height
    int Width() { return nMapX; }
    int Hight() { return nMapY; }

    // getter for (cumulated) cell height at coordinates (x, y)
    // Note - there's no intuitive meaning for this method in maps with holes
    float CellHeight( int x, int y ) {
        float result = -1.0f;
        if (x < 0 || x >= nMapX || y < 0 || y >= nMapY) {
            std::cout << "ERROR: CellHeight() --> error in mapindices" << std::endl;
        } else {
            result = 0.0f;
            for (int i = 0; i < (int)fMaps.size(); i++) {
                result += fMaps[i][y * nMapX + x];
            }
        }
        return result;
    }

    // getter for obtaining height value of the cell at level, coordinates (x, y)
    float CellHeightAt( int x, int y, int level ) {
        float result = -1.0f;
        if (x < 0 || x >= nMapX || y < 0 || y >= nMapY) {
            std::cout << "ERROR: CellHeightAt() --> error in mapindices" << std::endl;
        } else if (level >= (int)fMaps.size()) {
            std::cout << "ERROR: CellHeightAt() --> level argument out of range" << std::endl;
        } else {
            result = fMaps[ level ][y * nMapX + x];
        }
        return result;
    }

    // getter for obtaining the character value of the cell at level, coordinates (x, y)
    char CellValueAt( int x, int y, int level ) {
        char result = ' ';
        if (x < 0 || x >= nMapX || y < 0 || y >= nMapY) {
            std::cout << "ERROR: CellValueAt() --> error in mapindices" << std::endl;
        } else if (level >= (int)fMaps.size()) {
            std::cout << "ERROR: CellValueAt() --> level argument out of range" << std::endl;
        } else {
            result = sMaps[ level ][y * nMapX + x];
        }
        return result;
    }

    // returns the diagonal length of the map - useful for setting max distance value
    float DiagonalLength() {
        return sqrt( nMapX * nMapX + nMapY * nMapY );
    }

    // returns current number of layers in this map object
    int NrOfLayers() {
        return (int)fMaps.size();
    }

    // returns whether (x, y) is within map boundaries
    bool IsInBounds( int x, int y ) {
        return (x >= 0 && x < nMapX && 
                y >= 0 && y < nMapY);
    }

private:
    std::vector<std::string> sMaps;         // contains chars that define the type of block per map location
    std::vector<float *>     fMaps;    // contains floats that represent the height per block
    int nMapX;                // dimensions for the map
    int nMapY;
};

// ==============================/  PGE derived ray caster engine   /==============================

class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial - S:(" + std::to_string( SCREEN_X / PIXEL_X ) + ", " + std::to_string( SCREEN_Y / PIXEL_Y ) + ")" +
                                                  ", P:(" + std::to_string(            PIXEL_X ) + ", " + std::to_string(            PIXEL_Y ) + ")" ;
    }

private:
    // definition of the map object
    RC_Map cMap;

    // max visible distance - use length of map diagonal to overlook whole map
    float fMaxDistance;

    // player: position and looking angle
    float fPlayerX     = 2.5f;
    float fPlayerY     = 2.5f;
    float fPlayerA_deg = 0.0f;      // looking angle is in degrees - NOTE: 0.0f is EAST

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

#define MAX_OBJ_SPRITES 14

    olc::Sprite *pObjectSprite[MAX_OBJ_SPRITES] = { nullptr };

    bool bMouseControl = MOUSE_CONTROL;     // toggle on mouse control (trigger key M)

    // var's and initial values for shading
    float fObjectIntensity     = MULTIPLE_LEVELS ? OBJECT_INTENSITY     :  0.2f;
    float fIntensityMultiplier = MULTIPLE_LEVELS ? MULTIPLIER_INTENSITY : 10.0f;

    // toggles for rendering
    bool bMinimap   = false;    // toggle on mini map rendering (trigger key P)
    bool bMapRays   = false;    //                              (trigger key O)
    bool bDebugInfo = false;    //                              (trigger key I)

    std::vector<olc::vf2d> vRayList;    // needed for ray rendering in minimap

    // definition of object record
    typedef struct sObject {
        float x, y;             // position in the map
        float scale;            // 1.0f is 100%
        olc::Sprite *sprite;
        float distance, angle;  // w.r.t. player
    } Object;
    std::list<Object> vListObjects;

    // the 2D depth buffer
    float *fDepthBuffer = nullptr;

public:


    bool OnUserCreate() override {

        bool bSuccess = true;

        // create and fill the map - NOTE: string arguments in AddLayer() must match x and y dimensions in InitMap()!
        // the map itself is defined in a separate file
        cMap.InitMap( 16, 16 );
        cMap.AddLayer( sMap_level0 );
        cMap.AddLayer( sMap_level1 );
        cMap.AddLayer( sMap_level2 );
        cMap.AddLayer( sMap_level3 );
        // max ray length for DDA is diagonal length of the map
        fMaxDistance = cMap.DiagonalLength();

        // initialize sine and cosine lookup arrays - these are meant for performance improvement
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
        // load sprites for texturing walls, floor, ceilings and roofs
        std::string sSpritePath1 = "../sprites/";
        pWallSprite   = load_sprite_file( sSpritePath1 +    "new wall_brd.png" ); bSuccess &= (pWallSprite   != nullptr);
        pFloorSprite  = load_sprite_file( sSpritePath1 +   "grass_texture.png" ); bSuccess &= (pFloorSprite  != nullptr);
        pCeilSprite   = load_sprite_file( sSpritePath1 + "ceiling_texture.png" ); bSuccess &= (pCeilSprite   != nullptr);
        pRoofSprite   = load_sprite_file( sSpritePath1 +    "roof texture.png" ); bSuccess &= (pRoofSprite   != nullptr);

        // load sprites for rendering objects
        std::string sSpritePath2 = "sprites/";
        pObjectSprite[ 0] = load_sprite_file( sSpritePath2 + "elf-girl_stationary-front.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);   // elf girl

        pObjectSprite[ 1] = load_sprite_file( sSpritePath2 +            "bush_object_01.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);   // bushes
        pObjectSprite[ 2] = load_sprite_file( sSpritePath2 +            "bush_object_02.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[ 3] = load_sprite_file( sSpritePath2 +            "bush_object_03.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[ 4] = load_sprite_file( sSpritePath2 +            "bush_object_04.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);

        pObjectSprite[ 5] = load_sprite_file( sSpritePath2 +            "tree_object_01.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);   // trees
        pObjectSprite[ 6] = load_sprite_file( sSpritePath2 +            "tree_object_02.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[ 7] = load_sprite_file( sSpritePath2 +            "tree_object_03.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[ 8] = load_sprite_file( sSpritePath2 +            "tree_object_04.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[ 9] = load_sprite_file( sSpritePath2 +            "tree_object_05.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[10] = load_sprite_file( sSpritePath2 +            "tree_object_06.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[11] = load_sprite_file( sSpritePath2 +            "tree_object_07.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);
        pObjectSprite[12] = load_sprite_file( sSpritePath2 +            "tree_object_08.rbg.png" ); bSuccess &= (pObjectSprite != nullptr);

        // Initialize depth buffer
        fDepthBuffer = new float[ ScreenWidth() * ScreenHeight() ];

        // populate object list with randomly chosen, scaled and placed objects (switched off for current development phase)
        for (int i = 0; i < NR_TEST_OBJECTS; i++) {
            int nRandX, nRandY;
            bool bFoundEmpty = false;
            do {
                nRandX = rand() % cMap.Width();
                nRandY = rand() % cMap.Hight();

                bFoundEmpty = cMap.CellHeight( nRandX, nRandY ) == 0.0f;
            } while (!bFoundEmpty);
            int nRandObj = rand() % MAX_OBJ_SPRITES;
            int nRandSize;
            if (nRandObj == 0) {
                nRandSize = rand() %  5 + 5;
            } else if (nRandObj < 5) {
                nRandSize = rand() % 10 + 5;
            } else {
                nRandSize = rand() % 40 + 10;
            }

            Object tmpObj = { float( nRandX ) + 0.5f, float( nRandY ) + 0.5f, float( nRandSize / 10.0f ), pObjectSprite[ nRandObj ], -1.0f, 0.0f };
            vListObjects.push_back( tmpObj );
        }

        return bSuccess;
    }

    // Holds intersection point in float (world) coordinates and in int (tile) coordinates,
    // the distance to the intersection point and the height of the map at these tile coordinates
    typedef struct sIntersectInfo {
        float fHitX,         // world space
              fHitY;
        int   nMapCoordX,    // tile space
              nMapCoordY;
        float fDstFrnt;
        float fHeight;

        // these are on screen projected values (y coordinate in pixel space)
        int bottom_front;   // on screen projected bottom  of wall slice
        int   ceil_front;   //                     ceiling
        int   ceil_back;    //                     ceiling of wall at back
    } IntersectInfo;

    // Implementation of the DDA algorithm. This function uses class variables for the description of the map.
    // The players position is the "from point", a "to point" is determined using fRayAngle and fMaxDistance.
    // A ray is cast from the "from point" to the "to point". If there is a collision (intersection with a
    // change in height in the map) then the point of intersection, the distance and the map tile of the
    // wall cell is added to the hit list.
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
        bool bOutOfBounds = !cMap.IsInBounds( nCurX, nCurY );
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

            bOutOfBounds = !cMap.IsInBounds( nCurX, nCurY );
            if (bOutOfBounds) {
                bDestCellFound = false;

                // If out of bounds, finalize the list with one additional intersection with the map boundary and height 0.
                // (only if the list is not empty!) This additional intersection record is necessary for proper rendering at map boundaries.
                if (fCurHeight != 0.0f && !vHitList.empty()) {

                    fCurHeight = 0.0f;  // since we're out of bounds

                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    IntersectInfo sInfo;
                    sInfo.fDstFrnt  = fDistIfFound;
                    sInfo.fHitX      = fFromX + fDistIfFound * fDX;
                    sInfo.fHitY      = fFromY + fDistIfFound * fDY;
                    sInfo.nMapCoordX = nCurX;
                    sInfo.nMapCoordY = nCurY;
                    sInfo.fHeight    = fCurHeight;

                    vHitList.push_back( sInfo );
                }
            } else {
                // check if there's a difference in height found
                bool bHitFound = (cMap.CellHeight( nCurX, nCurY ) != fCurHeight);
                bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

                if (bHitFound) {

                    // reset current height to new value
                    fCurHeight = cMap.CellHeight( nCurX, nCurY );

                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    IntersectInfo sInfo;
                    sInfo.fDstFrnt  = fDistIfFound;
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

    // Returns the projected bottom and top of a wall slice as y screen coordinates.
    // The wall is at fCorrectedDistToWall from eye point, nHorHight is the height of the horizon
    // and fWallHeight as the height of the wall (in blocks) according to the map
    void CalculateWallBottomAndTop( float fCorrectedDistToWall, int nHorHeight, float fWallHeight, int &nWallTop, int &nWallBottom ) {
        // calculate slice height for a *unit height* wall
        int nSliceHeight = int((1.0f / fCorrectedDistToWall) * fDistToProjPlane);
        nWallTop    = nHorHeight - (nSliceHeight * (1.0f - fPlayerH)) - (fWallHeight - 1.0f) * nSliceHeight;
        nWallBottom = nHorHeight + (nSliceHeight *         fPlayerH );
    }

// ==============================/   Mini map rendering prototypes   /==============================

    void RenderMapGrid();        // function to render the mini map on the screen
    void RenderMapPlayer();      // function to render the player in the mini map on the screen
    void RenderMapRays();        // function to render the rays in the mini map on the screen
    void RenderMapObjects();     // function to render all the objects in the mini map on the screen
    void RenderDebugInfo();      // function to render debug info in a separate hud on the screen

    bool GetMouseSteering( float &fHorPerc, float &fVerPerc );	    // experimental function for mouse control
    olc::Pixel ShadePixel( const olc::Pixel &p, float fDistance );	// Shade the pixel p using fDistance as a factor in the shade formula

    // Variant on Draw() that takes fDepth and the depth buffer into account.
    // Pixel col is only drawn if fDepth is less than the depth buffer at that screen location (in which case the depth buffer is updated)
    void DrawDepth( float fDepth, int x, int y, olc::Pixel col ) {
        // prevent out of bounds drawing
        if (x >= 0 && x < ScreenWidth() &&
            y >= 0 && y < ScreenHeight()) {

            if (fDepth <= fDepthBuffer[ y * ScreenWidth() + x ]) {
                fDepthBuffer[ y * ScreenWidth() + x ] = fDepth;
                Draw( x, y, col );
            }
        }
    }

    bool OnUserUpdate( float fElapsedTime ) override {

        // step 1 - user input
        // ===================

        // reset look up value and player height on pressing 'R'
        if (GetKey( olc::R ).bReleased) { fPlayerH = 0.5f; fLookUp = 0.0f; }

        // toggles for HUDs
        if (GetKey( olc::I ).bPressed) bDebugInfo = !bDebugInfo;
        if (GetKey( olc::P ).bPressed) bMinimap   = !bMinimap;
        if (GetKey( olc::O ).bPressed) bMapRays   = !bMapRays;

        // For all movements and rotation you can speed up by keeping SHIFT pressed
        // or speed down by keeping CTRL pressed. This also affects shading/lighting
        float fSpeedUp = 1.0f;
        if (GetKey( olc::SHIFT ).bHeld) fSpeedUp = 3.0f;
        if (GetKey( olc::CTRL  ).bHeld) fSpeedUp = 0.2f;

        // Rotate - collision detection not necessary. Keep fPlayerA_deg between 0 and 360 degrees
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }

        // variables used for collision detection - work out the new location in a seperate coordinate pair, and only alter
        // the players coordinate if there's no collision
        float fNewX = fPlayerX;
        float fNewY = fPlayerY;

        // walking forward, backward and strafing left, right
        if (GetKey( olc::W ).bHeld) { fNewX += lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY += lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk forward
        if (GetKey( olc::S ).bHeld) { fNewX -= lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY -= lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk backwards
        if (GetKey( olc::Q ).bHeld) { fNewX += lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY -= lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe left
        if (GetKey( olc::E ).bHeld) { fNewX -= lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY += lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe right
        // collision detection - check if out of bounds or inside non-empty tile
        // only update position if no collision
        if (cMap.IsInBounds( fNewX, fNewY ) &&
            // collision detection criterion - is players height > height of map?
            cMap.CellHeight( int( fNewX ), int( fNewY )) < fPlayerH ) {

            fPlayerX = fNewX;
            fPlayerY = fNewY;
        }

        // looking up or down - collision detection not necessary
        // NOTE - there's no clamping to extreme values (yet)
        if (GetKey( olc::UP   ).bHeld) { fLookUp += SPEED_LOOKUP * fSpeedUp * fElapsedTime; }
        if (GetKey( olc::DOWN ).bHeld) { fLookUp -= SPEED_LOOKUP * fSpeedUp * fElapsedTime; }

        // Mouse control
        if (GetKey( olc::M ).bReleased) { bMouseControl = !bMouseControl; }  // toggle on or off
        float fRotFactor, fTiltFactor;
        if (bMouseControl && GetMouseSteering( fRotFactor, fTiltFactor )) {
            fPlayerA_deg += SPEED_ROTATE * fRotFactor  * fSpeedUp * fElapsedTime;
            fLookUp      -= SPEED_LOOKUP * fTiltFactor * fSpeedUp * fElapsedTime;
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
                if (fNewHeight > 0.0f && cMap.CellHeight( int( fPlayerX ), int( fPlayerY )) < fNewHeight ) {
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

        // alter object intensity and multiplier
        if (GetKey( olc::INS  ).bHeld) fObjectIntensity     += INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::DEL  ).bHeld) fObjectIntensity     -= INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::HOME ).bHeld) fIntensityMultiplier += INTENSITY_SPEED * fSpeedUp * fElapsedTime;
        if (GetKey( olc::END  ).bHeld) fIntensityMultiplier -= INTENSITY_SPEED * fSpeedUp * fElapsedTime;

        // step 2 - game logic
        // ===================

        // step 3 - render
        // ===============

        Clear( RENDER_CEILING ? olc::BLACK : olc::CYAN );

        int nHorizonHeight   = ScreenHeight() * fPlayerH + (int)fLookUp;
        float fAngleStep = fPlayerFoV_deg / float( ScreenWidth() );

        // render background scene:
        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle = float( x - (ScreenWidth() / 2)) * fAngleStep;
            float fCurAngle = fPlayerA_deg + fViewAngle;

            float fX_hit, fY_hit;        // to hold exact (float) hit location (world space)
            int   nX_hit, nY_hit;        // to hold coords of tile that was hit (tile space)

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
            std::vector<IntersectInfo> vHitPointList;
            float fColHeight = 1.0f;
            float fFrntDistance = 0.0f;     // distance var needed for wall shading
            if (GetDistancesToWalls( fCurAngle, vHitPointList )) {

                // at least one wall / block was hit. Extend the hit list with projected bottom / ceiling info
                for (int i = 0; i < (int)vHitPointList.size(); i++) {
                    // make correction for the fish eye effect
                    vHitPointList[i].fDstFrnt *= lu_cos( fViewAngle );
                    // calculate values for the on screen projections top_front and top_bottom
                    CalculateWallBottomAndTop( vHitPointList[i].fDstFrnt, nHorizonHeight, vHitPointList[i].fHeight, vHitPointList[i].ceil_front, vHitPointList[i].bottom_front );
                }
                // Extend the hit list with projected ceiling info for the back of the wall
                for (int i = 0; i < (int)vHitPointList.size(); i++) {
                    if (i == (int)vHitPointList.size() - 1) {
                        // last element, has no successor
                        vHitPointList[i].ceil_back = vHitPointList[i].ceil_front;
                    } else {
                        int nDummy;
                        CalculateWallBottomAndTop( vHitPointList[i + 1].fDstFrnt, nHorizonHeight, vHitPointList[i].fHeight, vHitPointList[i].ceil_back, nDummy );
                    }
                }

                // get the info from first hit point
                fX_hit       = vHitPointList[0].fHitX;
                fY_hit       = vHitPointList[0].fHitY;
                nX_hit       = vHitPointList[0].nMapCoordX;
                nY_hit       = vHitPointList[0].nMapCoordY;
                fColHeight   = vHitPointList[0].fHeight;
                fFrntDistance = vHitPointList[0].fDstFrnt;
                nWallTop     = vHitPointList[0].ceil_front;
                nWallTop2    = vHitPointList[0].ceil_back;
                nWallBottom  = vHitPointList[0].bottom_front;

            } else {
                // no wall was hit - set bottom and top value for wall at the horizon
                nWallTop     = nHorizonHeight;
                nWallTop2    = nWallTop;
                nWallBottom  = nHorizonHeight;
                fFrntDistance = fMaxDistance;
            }

            // populate ray list for rendering mini map
            if (!vHitPointList.empty()) {
                olc::vf2d curHitPoint = { vHitPointList[0].fHitX, vHitPointList[0].fHitY };
                vRayList.push_back( curHitPoint );
            }

            // Update depth buffer - note that this is the distance to the *first* hitpoint
            // if there are no hitpoints, it will be set to fMaxDistance
            for (int y = 0; y < ScreenHeight(); y++) {
                fDepthBuffer[ y * ScreenWidth() + x ] = fMaxDistance;
            }

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
                        if (nHitListIndex < (int)vHitPointList.size() - 1) {
                            // the y coord is above the current wall and roof slide, but there are still hit points to process
                            // so there could be other walls behind current wall sticking out above it
                            nHitListIndex += 1;

                            // get the info from next hit point
                            fX_hit       = vHitPointList[ nHitListIndex ].fHitX;
                            fY_hit       = vHitPointList[ nHitListIndex ].fHitY;
                            nX_hit       = vHitPointList[ nHitListIndex ].nMapCoordX;
                            nY_hit       = vHitPointList[ nHitListIndex ].nMapCoordY;
                            fColHeight   = vHitPointList[ nHitListIndex ].fHeight;
                            fFrntDistance = vHitPointList[ nHitListIndex ].fDstFrnt;
                            nWallTop     = vHitPointList[ nHitListIndex ].ceil_front;
                            nWallTop2    = vHitPointList[ nHitListIndex ].ceil_back;
                            nWallBottom  = vHitPointList[ nHitListIndex ].bottom_front;

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
                                olc::Pixel ceilSample = get_ceil_sample( x, y );   // shading is done in get_ceil_sample()
                                DrawDepth( fFrntDistance, x, y, ceilSample );
                            }
                        }
                        break;
                    case FLOOR_DRAWING: {                        // ========== render floor   ====================
                            olc::Pixel floorSample = get_floor_sample( x, y );   // shading is done in get_floor_sample()
                            DrawDepth( fMaxDistance, x, y, floorSample );
                        }
                        break;
                    case ROOF_DRAWING: {                        // ========== render roof   ====================
                            olc::Pixel roofSample = get_roof_sample( x, y, fColHeight );   // shading is done in get_roof_sample()
                            DrawDepth( fFrntDistance, x, y, roofSample );
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
                                // The major bottleneck here is that this analysis is done for each separate pixel in the slice:
                                // for now I solved this by caching the drawmode and checking if was a wall previous
                                // (note that I have to temper with the cached drawmode when multiple wall segments are behind each other)

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
                            olc::Pixel wallSample = ShadePixel( pWallSprite->Sample( fSampleX, fSampleY ), fFrntDistance );
                            DrawDepth( fFrntDistance, x, y, wallSample );
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
            [=]( const Object &a, const Object &b ) {
                return a.distance > b.distance;
            }
        );

        // phase 2: render it from large to smaller distances
        for (auto &object : vListObjects) {
            // can object be seen?
            float fObjDist = object.distance;
            float fObjA = object.angle;
            // determine whether object is in field of view (a bit larger to prevent objects being not rendered at
            // screen boundaries)
            bool bInFOV = fabs( fObjA ) < fPlayerFoV_rad / 1.2f;

            // render object only when within Field of View, and within visible distance.
            // the check on proximity is to prevent asymptotic errors when this distance becomes very small
            if (bInFOV && fObjDist >= 0.3f && fObjDist < fMaxDistance) {

                // determine the difference between standard player height (i.e. 0.5f = standing on the floor)
                // and current player height
                float fCompensatePlayerHeight = fPlayerH - 0.5f;
                // get the projected (halve) slice height of this object
                float fObjHlveSliceHeight     = float( ScreenHeight()                  / fObjDist);
                float fObjHlveSliceHeightScld = float((ScreenHeight() * object.scale ) / fObjDist);

                // work out where objects floor and ceiling are (in screen space)
                // due to scaling factor, differentiated a normalized (scale = 1.0f) ceiling and a scaled variant
                float fObjCeilingNormalized = float(nHorizonHeight) - fObjHlveSliceHeight;
                float fObjCeilingScaled     = float(nHorizonHeight) - fObjHlveSliceHeightScld;
                // and adapt all the scaling into the ceiling value
                float fScalingDifference = fObjCeilingNormalized - fObjCeilingScaled;
                float fObjCeiling = fObjCeilingNormalized - 2 * fScalingDifference;
                float fObjFloor   = float(nHorizonHeight) + fObjHlveSliceHeight;

                // compensate object projection heights for elevation of the player
                fObjCeiling += fCompensatePlayerHeight * fObjHlveSliceHeight * 2.0f;
                fObjFloor   += fCompensatePlayerHeight * fObjHlveSliceHeight * 2.0f;

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
                            olc::Pixel objSample = ShadePixel( object.sprite->Sample( fSampleX, fSampleY ), fObjDist );
                            if (objSample != olc::BLANK) {
                                DrawDepth( fObjDist, nObjColumn, fObjCeiling + fy, objSample );
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

        cMap.FinalizeMap();

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


// ==============================/   put bloat behind main()  /==============================

// ==============================/   Mini map rendering stuff   /==============================

// function to render the mini map on the screen
void MyRayCaster::RenderMapGrid() {
    // fill background for minimap
    float fMMFactor = MINIMAP_SCALE_FACTOR * MINIMAP_TILE_SIZE;
    FillRect( 0, 0, cMap.Width() * fMMFactor, cMap.Hight() * fMMFactor, olc::VERY_DARK_GREEN );
    // draw each tile
    for (int y = 0; y < cMap.Hight(); y++) {
        for (int x = 0; x < cMap.Width(); x++) {
            // colour different for different heights
            olc::Pixel p;
            bool bBorderFlag = true;
            if (cMap.CellHeight( x, y ) == 0.0f) {
                p = olc::VERY_DARK_GREEN;   // don't visibly render
                bBorderFlag = false;
            } else if (cMap.CellHeight( x, y )  <  1.0f) {
                p = olc::PixelF( cMap.CellHeight( x, y ), 0.0f, 0.0f );    // height < 1.0f = shades of red
            } else {
                float fColFactor = std::min( cMap.CellHeight( x, y ) / 4.0f + 0.5f, 1.0f );    // heights > 1.0f = shades of blue
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
void MyRayCaster::RenderMapPlayer() {
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
void MyRayCaster::RenderMapRays() {
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
void MyRayCaster::RenderMapObjects() {
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
void MyRayCaster::RenderDebugInfo() {
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

// experimental function for mouse control
bool MyRayCaster::GetMouseSteering( float &fHorPerc, float &fVerPerc ) {
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
olc::Pixel MyRayCaster::ShadePixel( const olc::Pixel &p, float fDistance ) {
    if (RENDER_SHADED) {
        float fShadeFactor = std::max( SHADE_FACTOR_MIN, std::min( SHADE_FACTOR_MAX, fObjectIntensity * ( fIntensityMultiplier /  fDistance )));
        return p * fShadeFactor;
    } else
        return p;
}

// ==============================/  convenience functions for angles  /==============================

#define SIGNIFICANCE 3      // float angle is rounded at three decimal points
#define SIG_POW10    1000

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

