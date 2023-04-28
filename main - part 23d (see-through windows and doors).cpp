// ELABORATING ON - Ray casting tutorial by Permadi
// (starting with part 20 all code files are my own elaboration on the Permadi basis)
//
// Implementation of part 23 d - dynamic, animated and wall objects (see-through windows and doors)
//
// Joseph21, april 28, 2023
//
// Dependencies:
//   *  olcPixelGameEngine.h - (olc::PixelGameEngine header file) by JavidX9 (see: https://github.com/OneLoneCoder/olcPixelGameEngine)
//   *  sprite files for texturing walls, roofs, floor and ceiling, as well as objects in the scene: use your own .png files and
//      adapt the file names and paths in OnUserCreate()
//   *  include file "map_16x16 - part 23d.h" - contains the map definitions and the sprite file names

/* Short description
   -----------------
   This implementation is a follow up of implementation part 23 c. See the description of that implementation as well (and check on
   the differences between that cpp file and this one).

     * Added idea for distance correction depending on height and pixel you are looking through: the y coordinate of the screen pixel
       together with the horizon height represent an angle of looking up or down. Find this angle from the field of view and screen
       width, and apply the cos of it as a correction factor to get the slanted distance.
     * Optimized the DDA code for face detection (you could also say - fixed some bugs from previous version :)
     * Created a new type DelayedPixel. Added some code in wall rendering to implement wall objects, that are detected during wall
       rendering, but only rendered after wall rendering (such as see-through doors and windows). This way you can render them *on top
       of* the walls and background scene.

   Have fun!
 */

#include <cfloat>       // needed for constant FLT_MAX in the DDA function

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define PI 3.1415926535f

// Screen and pixel constants - keep the screen sizes constant and vary the resolution by adapting the pixel size
// to prevent accidentally defining too large a window
#define SCREEN_X      1000
#define SCREEN_Y       600
#define PIXEL_SIZE       1

#define MULTIPLE_LEVELS      true
#define RENDER_CEILING       !MULTIPLE_LEVELS    // render ceilings only for single level world

#define MOUSE_CONTROL        false

// shading constants
#define RENDER_SHADED        true
#define OBJECT_INTENSITY       5.0f   // for testing, reset to 1.5f afterwards!
#define MULTIPLIER_INTENSITY   5.0f
#define INTENSITY_SPEED        1.0f

#define SHADE_FACTOR_MIN       0.1f   // the shade factor is clamped between these two values
#define SHADE_FACTOR_MAX       1.0f

// colour constants
#define TEXT_COLOUR     olc::YELLOW
#define HUD_BG_COLOUR   olc::VERY_DARK_GREEN

// constants for speed movements - all movements are modulated with fElapsedTime
#define SPEED_ROTATE          60.0f   //                            60 degrees per second
#define SPEED_MOVE             5.0f   // forward and backward    -   5 units per second
#define SPEED_STRAFE           5.0f   // left and right strafing -   5 units per second
#define SPEED_LOOKUP         200.0f   // looking up or down      - 200 pixels per second
#define SPEED_STRAFE_UP        1.0f   // flying or chrouching     -   1.0 block per second

// mini map constants
#define MINIMAP_TILE_SIZE     (32 / PIXEL_SIZE)   // each minimap tile is ... pixels
#define MINIMAP_SCALE_FACTOR   0.2    // should be 0.2

// constants for collision detection with walls
#define RADIUS_PLAYER   0.1f
#define RADIUS_ELF      0.2f

// ==============================/  Prototypes look up trig functions  /==============================

// convenience conversion functions
float deg2rad( float fAngleDeg );
float rad2deg( float fAngleRad );
// convenience modulo functions
float deg_mod2pi( float fAngleDeg );
float rad_mod2pi( float fAngleRad );
// look up sine and cosine functions: init and call
void init_lu_sin_array();
void init_lu_cos_array();
float lu_sin( float fDegreeAngle );
float lu_cos( float fDegreeAngle );

// convenience functions for random range integers and floats
int     int_rand_between( int   nLow, int   nHgh );
float float_rand_between( float fLow, float fHgh );

// ==============================/  map definitions here   /==============================

#include "map_16x16 - part 23d.h"

// test objects
#define NR_TEST_OBJECTS    20


// ==============================/  definition of BlockType  /==============================

// constants for identifying the faces of a block
#define FACE_UNKNOWN  -1
#define FACE_EAST      0
#define FACE_NORTH     1
#define FACE_WEST      2
#define FACE_SOUTH     3
#define FACE_TOP       4
#define FACE_BOTTOM    5
#define FACE_NR_OF     6

// A "BlockType" object is a combination of
//   + a character identifying that block in the map definition
//   + a specific height
//   + 6 faces (EAST through BOTTOM, see constants above) containing indexes in the sprites vectors:
//       The indexes contains in EAST, NORTH, WEST and SOUTH index into the vWallSprites vector,
//       whereas TOP indexes into the vRoofSprites vector and BOTTOM into the vCeilSprites vector.
typedef struct sBlockStruct {
    char cID;
    float fHeight;
    int nFaces[ FACE_NR_OF ];
    bool bTransparent = false;  // "see-through" block - implemented with delayed rendering
} BlockType;
// The library of blocks is modeled as a map, for fast (O(n log n)) searching
std::map<char, BlockType> mBlockLook;

// Convenience function to add one block configuration
void AddBlock( char cID, float fH, int nE, int nN, int nW, int nS, int nT, int nB, bool bT = false ) {
    BlockType sTmpBlock = { cID, fH, nE, nN, nW, nS, nT, nB, bT };
    mBlockLook.insert( std::make_pair( cID, sTmpBlock ));
}

// Put all block configs you need into this function. Here the relation between the identifying character
// and the sprites per face are made, as well as the height of the block.
void InitBlockTypes() {
  AddBlock( '.', 0.00f, 0, 0, 0, 0, 0, 0 );

  AddBlock( '#', 1.00f, 0, 0, 0, 0, 0, 0 );   // these are indices into the vectors of wall, roof and ceiling sprite pointers
  AddBlock( '%', 1.00f, 1, 1, 1, 1, 1, 1 );
  AddBlock( '!', 1.00f, 2, 2, 2, 2, 2, 2 );
  AddBlock( '@', 1.00f, 3, 3, 3, 3, 3, 3 );
  AddBlock( '$', 1.00f, 4, 4, 4, 4, 0, 0, true );   // door / gate
  AddBlock( '&', 1.00f, 5, 5, 5, 5, 5, 5 );
  AddBlock( '*', 1.00f, 6, 6, 6, 6, 0, 0, true );   // window
  AddBlock( '+', 1.00f, 7, 7, 7, 7, 0, 0, true );   // barred window

  AddBlock( 'Q', 0.25f, 0, 0, 0, 0, 0, 0 );
  AddBlock( 'H', 0.50f, 0, 0, 0, 0, 0, 0 );
  AddBlock( 'T', 0.75f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '1', 0.10f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '2', 0.20f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '3', 0.30f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '4', 0.40f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '5', 0.50f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '6', 0.60f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '7', 0.70f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '8', 0.80f, 0, 0, 0, 0, 0, 0 );
  AddBlock( '9', 0.90f, 0, 0, 0, 0, 0, 0 );
}

// return a reference to the block in the library having id cID
BlockType &GetBlockType( char cID ) {
    std::map<char, BlockType>::iterator itBlock = mBlockLook.find( cID );
    if (itBlock == mBlockLook.end()) {
        std::cout << "ERROR: GetBlock() --> can't find element with ID: " << cID << std::endl;
    }
    return (*itBlock).second;
}

// getters for the members of the BlockType
char GetBlockID(     BlockType &b ) { return b.cID; }
char GetBlockHeight( BlockType &b ) { return b.fHeight; }
char GetBlockFaceIx( BlockType &b, int nFace ) {
    int nIndex = FACE_UNKNOWN;
    if (nFace < 0 || nFace >= FACE_NR_OF) {
        std::cout << "GetBlockFaceIx() --> index out of range: " << nFace << std::endl;
    } else {
        nIndex = b.nFaces[ nFace ];
    }
    return nIndex;
}

// ==============================/  class RC_Map  /==============================

class RC_Map {

public:
    RC_Map() {
        InitMap( 0, 0 );
    }

    // First initialize the map calling this method ...
    void InitMap( int nSizeX, int nSizeY ) {
        nMapX = nSizeX;
        nMapY = nSizeY;
    }

    // ... then add at least 1 layer to it using this method
    void AddLayer( const std::string &sUserMap ) {
        if (nMapX * nMapY != (int)sUserMap.length()) {
            std::cout << "ERROR: InitMap() -->  mismatch between map dimensions and length of map string" << std::endl;
        }
        // temporary character map to add current layer
        std::string sMap = sUserMap;

        // Initialise fMap as a 2d array of floats, having the same size as sMap, and containing the height per cell.
        float *fMap = new float[nMapX * nMapY];
        for (int y = 0; y < nMapY; y++) {
            for (int x = 0; x < nMapX; x++) {
                BlockType &refBlock = GetBlockType( sMap[ y * nMapX + x ] );
                fMap[ y * nMapX + x ] = refBlock.fHeight;
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

    // returns whether (x, y) is within map boundaries
    bool IsInBounds( float x, float y ) {
        return (x >= 0 && x < nMapX && y >= 0 && y < nMapY);
    }

    // getter for (cumulated) cell height at coordinates (x, y)
    // Note - there's no intuitive meaning for this method in maps with holes
    float CellHeight( int x, int y ) {
        float result = -1.0f;
        if (!IsInBounds( x, y )) {
            std::cout << "ERROR: CellHeight() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
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
        if (!IsInBounds( x, y )) {
            std::cout << "ERROR: CellHeightAt() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
        } else if (level >= (int)fMaps.size()) {
            std::cout << "ERROR: CellHeightAt() --> level argument out of range: " << level << std::endl;
        } else {
            result = fMaps[ level ][y * nMapX + x];
        }
        return result;
    }

    // getter for obtaining the character value of the cell at level, coordinates (x, y)
    char CellValueAt( int x, int y, int level ) {
        char result = ' ';
        if (!IsInBounds( x, y )) {
            std::cout << "ERROR: CellValueAt() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
        } else if (level >= (int)fMaps.size()) {
            std::cout << "ERROR: CellValueAt() --> level argument out of range: " << level << std::endl;
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

    // collision detection on the map:
    // Note that int( fH ) denotes level to check, and (fH - int( fH )) denotes height to check within that level
    // and fR is the radius of the object (considered as a pillar shape)
    bool Collides( float fX, float fY, float fH, float fR, float fVX, float fVY ) {

        float fOffsetX;
        float fOffsetY;
        if (fVX == 0.0f) { fOffsetX = 0.0f; } else { fOffsetX = (fVX < 0.0f ? -fR : fR); }
        if (fVY == 0.0f) { fOffsetY = 0.0f; } else { fOffsetY = (fVY < 0.0f ? -fR : fR); }
        return (
            !IsInBounds( fX + fOffsetX, fY + fOffsetY ) ||
            (fH - fR) < 0.0f || (
                fH <= NrOfLayers() &&
                CellHeightAt( int( fX + fOffsetX ), int( fY + fOffsetY ), int( fH )) >= (fH - int( fH ))
            )
        );
    }

private:
    std::vector<std::string> sMaps;    // contains chars that define the type of block per map location
    std::vector<float *>     fMaps;    // contains floats that represent the height per block

    int nMapX;                // dimensions for the map
    int nMapY;
};


// ==============================/  class RC_Object   /==============================

// used to be definition of object record
class RC_Object {

public:
    RC_Object() {}
    RC_Object( float fX, float fY, float fS, float fD, float fA, olc::Sprite *pS ) {
        x              = fX;
        y              = fY;
        scale          = fS;
        fDistToPlayer  = fD;
        fAngleToPlayer = fA;
        sprite         = pS;
        vx             = 0.0f;
        vy             = 0.0f;
        UpdateObjAngle();
        UpdateObjSpeed();
    }

    void SetX( float fX ) { x = fX; }
    void SetY( float fY ) { y = fY; }

    float GetX() { return x; }
    float GetY() { return y; }

    void SetPos( float fX, float fY ) {
        x = fX;
        y = fY;
    }

    void SetScale(    float fS ) { scale          = fS; }
    void SetDistToPlayer( float fD ) { fDistToPlayer  = fD; }
    void SetAngleToPlayer(    float fA ) { fAngleToPlayer = fA; }

    float GetScale(   ) { return scale         ; }
    float GetDistToPlayer() { return fDistToPlayer ; }
    float GetAngleToPlayer(   ) { return fAngleToPlayer; }

    void SetSprite( olc::Sprite *pS ) { sprite = pS; }
    olc::Sprite *GetSprite() { return sprite; }

    void SetVX( float fVX ) { vx = fVX; UpdateObjAngle(); UpdateObjSpeed(); }
    void SetVY( float fVY ) { vy = fVY; UpdateObjAngle(); UpdateObjSpeed(); }

    float GetVX() { return vx; }
    float GetVY() { return vy; }
    float GetAngle() { return fObjAngle; }   // in radians!!
    float GetSpeed() { return fObjSpeed; }

    void Update( RC_Map &cMap, float fElapsedTime ) {
        if (!bStatic) {
            float newX = x + vx * fElapsedTime;
            float newY = y + vy * fElapsedTime;
            if (!cMap.Collides( newX, y, scale, RADIUS_ELF, vx, vy )) {
                x = newX;
            } else {
                vx = -vx;
                UpdateObjAngle();
                UpdateObjSpeed();
            }
            if (!cMap.Collides( x, newY, scale, RADIUS_ELF, vx, vy )) {
                y = newY;
            } else {
                vy = -vy;
                UpdateObjAngle();
                UpdateObjSpeed();
            }
        }
    }

    void Print() {
        std::cout << "object @ pos: (" <<  x << ", " <<  y << "), ";
        std::cout << "vel: (" << vx << ", " << vy << "), ";
        std::cout << (bStatic ? "STATIONARY " : "DYNAMIC ");
        std::cout << std::endl;
    }

public:
    bool bStatic = true;

private:
    void UpdateObjAngle() { fObjAngle = rad_mod2pi( atan2f( vy, vx )); }
    void UpdateObjSpeed() { fObjSpeed = sqrt( vx * vx + vy * vy );    }

private:
    float x, y;             // position in the map
    float scale;            // 1.0f is 100%

    float vx, vy;           // velocity
    float fObjAngle;
    float fObjSpeed;

    float fDistToPlayer,
          fAngleToPlayer;  // w.r.t. player

    olc::Sprite *sprite;
};

// ==============================/  PGE derived ray caster engine   /==============================


class MyRayCaster : public olc::PixelGameEngine {

public:
    MyRayCaster() {    // display the screen and pixel dimensions in the window caption
        sAppName = "MyRayCaster - Permadi tutorial - S:(" + std::to_string( SCREEN_X / PIXEL_SIZE ) + ", " + std::to_string( SCREEN_Y / PIXEL_SIZE ) + ")" +
                                                  ", P:(" + std::to_string(            PIXEL_SIZE ) + ", " + std::to_string(            PIXEL_SIZE ) + ")" ;
    }

private:
    // definition of the map object
    RC_Map cMap;

    // max visible distance - use length of map diagonal to overlook whole map
    float fMaxDistance;

    // player: position and looking angle
    float fPlayerX      = 2.5f;
    float fPlayerY      = 2.5f;
    float fPlayerA_deg  = 0.0f;      // looking angle is in degrees - NOTE: 0.0f is EAST

    // player: height of eye point and field of view
    float fPlayerH       =  0.5f;
    float fPlayerFoV_deg = 60.0f;   // in degrees !!
    float fAnglePerPixel_deg;

    // factor for looking up or down - initially 0.0f (in pixel space: float is for smooth movement)
    float fLookUp        =  0.0f;
    float fDistToProjPlane;         // constant distance to projection plane - is calculated in OnUserCreate()

    // all sprites for texturing the scene and the objects, grouped in categories
    std::vector<olc::Sprite *> vWallSprites;
    std::vector<olc::Sprite *> vCeilSprites;
    std::vector<olc::Sprite *> vRoofSprites;
    std::vector<olc::Sprite *> vFloorSprites;
    std::vector<olc::Sprite *> vObjectSprites;

    bool bMouseControl = MOUSE_CONTROL;     // toggle on mouse control (trigger key M)

    // var's and initial values for shading - trigger keys INS and DEL
    float fObjectIntensity     = MULTIPLE_LEVELS ? OBJECT_INTENSITY     :  0.2f;
    float fIntensityMultiplier = MULTIPLE_LEVELS ? MULTIPLIER_INTENSITY : 10.0f;

    // toggles for rendering
    bool bMinimap   = false;    // toggle on mini map rendering (trigger key P)
    bool bMapRays   = false;    //                              (trigger key O)
    bool bDebugInfo = false;    //                              (trigger key I)
    bool bTestSlice = false;    //                              (trigger key G)
    bool bTestGrid  = false;    //                              (trigger key H)

    std::vector<olc::vf2d> vRayList;    // needed for ray rendering in minimap

    std::list<RC_Object> vListObjects;     // list of all objects in the game

    // the 2D depth buffer
    float *fDepthBuffer = nullptr;

    // which of the slices to output in test mode
    float fTestSlice;

public:

    bool OnUserCreate() override {

        bool bSuccess = true;

        srand( time( 0 ));

        InitBlockTypes();

        // create and fill the map - NOTE: string arguments in AddLayer() must match x and y dimensions in InitMap()!
        // the map itself is defined in a separate file
        cMap.InitMap( glbMapX, glbMapY );
        for (int i = 0; i < (int)vMap_level.size(); i++) {
            cMap.AddLayer( vMap_level[i] );
        }

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
        // lambda expression for loading all sprites for one category (walls, ceilings, roofs, floors or objects) into the
        // associated container
        auto load_sprites_from_files = [=]( std::vector<std::string> &vFileNames, std::vector<olc::Sprite *> &vSpritePtrs ) {
            bool bNoErrors = true;
            for (auto &s : vFileNames) {
                olc::Sprite *tmpPtr = load_sprite_file( s );
                bNoErrors &= (tmpPtr != nullptr);
                vSpritePtrs.push_back( tmpPtr );
            }
            return bNoErrors;
        };
        // load all sprites into their associated container
        bSuccess &= load_sprites_from_files(   vWallSpriteFiles, vWallSprites );
        bSuccess &= load_sprites_from_files(   vCeilSpriteFiles, vCeilSprites );
        bSuccess &= load_sprites_from_files(   vRoofSpriteFiles, vRoofSprites );
        bSuccess &= load_sprites_from_files(  vFloorSpriteFiles, vFloorSprites );
        bSuccess &= load_sprites_from_files( vObjectSpriteFiles, vObjectSprites );

        // Initialize depth buffer
        fDepthBuffer = new float[ ScreenWidth() * ScreenHeight() ];

        // aux map to keep track of placed objects
        std::string sObjMap;
        for (int y = 0; y < cMap.Hight(); y++) {
            for (int x = 0; x < cMap.Width(); x++) {
                sObjMap.append( " " );
            }
        }
        // only place objects where there's nothing in the immediate (8 connected) neighbourhood
        auto space_for_object = [=]( int x, int y ) -> bool {
            int xMin = std::max( 0, x - 1 );
            int yMin = std::max( 0, y - 1 );
            int xMax = std::min( cMap.Width() - 1, x + 1 );
            int yMax = std::min( cMap.Hight() - 1, y + 1 );
            bool bOccupied = false;
            for (int r = yMin; r <= yMax && !bOccupied; r++) {
                for (int c = xMin; c <= xMax && !bOccupied; c++) {
                    bOccupied = cMap.CellHeight( c, r ) != 0.0f || sObjMap[ r * cMap.Width() + c ] != ' ';
                }
            }
            return !bOccupied;
        };

        // populate object list with randomly chosen, scaled and placed objects
        for (int i = 0; i < NR_TEST_OBJECTS; i++) {
            int nRandX, nRandY;
            bool bFoundEmpty = false;
            bool bMakeDynamic = false;
            // find an empty spot in the map
            do {
                nRandX = rand() % cMap.Width();
                nRandY = rand() % cMap.Hight();

                bFoundEmpty = space_for_object( nRandX, nRandY );
            } while (!bFoundEmpty);

            // determine object type - make sure that at least ... objects are dynamic
            int nRandObj = (i < 5) ? 0 : rand() % vObjectSprites.size();
            // depending on object type, make dynamic or static and determine size
            int nRandSize;
            switch (nRandObj) {
                case   0: bMakeDynamic = true;  nRandSize = rand() %  5 + 5; break;   // this is an elf girl, make dynamic
                case   1:
                case   2: bMakeDynamic = false; nRandSize = 10;              break;   // these are stationary objects of fixed size
                case   3:
                case   4:
                case   5:
                case   6: bMakeDynamic = false; nRandSize = rand() % 10 + 5; break;   // these are bushes
                default : bMakeDynamic = false; nRandSize = rand() % 20 + 10;         // trees
            }
            // create the object and put it in the list of objects
            RC_Object tmpObj( float( nRandX ) + 0.5f, float( nRandY ) + 0.5f, float( nRandSize / 10.0f ), -1.0f, 0.0f, vObjectSprites[ nRandObj ] );
            if (bMakeDynamic) {
                tmpObj.bStatic = false;
                tmpObj.SetVX( float_rand_between( -5.0f, 5.0f ));
                tmpObj.SetVY( float_rand_between( -5.0f, 5.0f ));
            } else {
                tmpObj.bStatic = true;
                tmpObj.SetVX( 0.0f );
                tmpObj.SetVY( 0.0f );
            }
            vListObjects.push_back( tmpObj );
            // make a mark that an object is placed at this tile
            sObjMap[ nRandY * cMap.Width() + nRandX ] = 'X';
        }

        // set initial test slice value at middle of screen
        fTestSlice = ScreenWidth() / 2;
        // determine how much degrees one pixel shift represents.
        fAnglePerPixel_deg = fPlayerFoV_deg / ScreenWidth();

        return bSuccess;
    }

    // Holds intersection point in float (world) coordinates and in int (tile) coordinates,
    // the distance to the intersection point and the height of the map at these tile coordinates
    typedef struct sIntersectInfo {
        float fHitX,         // world space
              fHitY;
        int   nMapCoordX,    // tile space
              nMapCoordY;
        float fDistFrnt,     // distances to front and back faces of hit block
              fDistBack;
        float fHeight;       // height within the level
        int   nLevel = -1;   // nLevel == 0 --> ground level

        // these are on screen projected values (y coordinate in pixel space)
        int bot_front = -1;    // on screen projected bottom  of wall slice
        int bot_back  = -1;    //                     bottom  of wall at back
        int top_front = -1;    //                     ceiling
        int top_back  = -1;    //                     ceiling of wall at back

        int nFaceHit = FACE_UNKNOWN;     // what face was hit?
    } IntersectInfo;

    void PrintHitPoint( IntersectInfo &p, bool bVerbose ) {
        std::cout << "hit (world): ( " << p.fHitX << ", " << p.fHitY << " ) ";
        std::cout << "hit (tile): ( " << p.nMapCoordX << ", " << p.nMapCoordY << " ) ";
        std::cout << "dist.: " << p.fDistFrnt << " ";
        std::cout << "lvl: " << p.nLevel << " hght: " << p.fHeight << " ";
        if (bVerbose) {
            std::cout << "bot frnt: " << p.bot_front << " bot back: " << p.bot_back << " ";
            std::cout << "top frnt: " << p.top_front << " top back: " << p.top_back << " ";

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

    void PrintHitList( std::vector<IntersectInfo> &vHitList, bool bVerbose = false ) {
        for (int i = 0; i < (int)vHitList.size(); i++) {
            std::cout << "Elt: " << i << " = ";
            PrintHitPoint( vHitList[i], bVerbose );
        }
        std::cout << std::endl;
    }

    // Implementation of the DDA algorithm. This function uses class variables for the description of the map
    // at "level" (where level 0 is ground level)
    // The players position is the "from point", a "to point" is determined using fRayAngle and fMaxDistance.
    // A ray is cast from the "from point" to the "to point". If there is a collision (intersection with a
    // change in height in the map) then the point of intersection, the distance and the map tile of the
    // wall cell is added to the hit list.
    bool GetDistancesToWallsPerLevel( int level, float fRayAngle, std::vector<IntersectInfo> &vHitList ) {

        // counter for nr of hit points found
        int nHitPointsFound = 0;

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

        bool bCheckHor;             // to keep track of what direction you are searching

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
        auto add_hit_point = [&]( std::vector<IntersectInfo> &vHList, float fDst, float fStrtX, float fStrtY, float fDeltaX, float fDeltaY, int nTileX, int nTileY, float fHght, int nLevel, bool bHorGrid ) {
            IntersectInfo sInfo;
            sInfo.fDistFrnt  = fDst;
            sInfo.fHitX      = fStrtX + fDst * fDeltaX;
            sInfo.fHitY      = fStrtY + fDst * fDeltaY;
            sInfo.nMapCoordX = nTileX;
            sInfo.nMapCoordY = nTileY;
            sInfo.fHeight    = fHght;
            sInfo.nLevel     = nLevel;
            sInfo.nFaceHit   = get_face_hit( bHorGrid );
            vHList.push_back( sInfo );
        };

        float fDistIfFound = 0.0f;  // accumulates distance of analysed piece of ray
        float fCurHeight   = 0.0f;  // to check on differences in height

        bool bPrevWasTransparent = false;
        while (!bOutOfBounds && !bDestCellFound && fDistIfFound < fMaxDistance) {

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

            bOutOfBounds = !cMap.IsInBounds( nCurX, nCurY );
            if (bOutOfBounds) {
                bDestCellFound = false;

                // If out of bounds, finalize the list with one additional intersection with the map boundary and height 0.
                // (only if the list is not empty!) This additional intersection record is necessary for proper rendering at map boundaries.
                if (fCurHeight != 0.0f && nHitPointsFound > 0) {

                    fCurHeight = 0.0f;  // since we're out of bounds
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, level, bCheckHor );
                }
            } else {
                // check if there's a difference in height found
                bool bHitFound = (cMap.CellHeightAt( nCurX, nCurY, level ) != fCurHeight);
                // check if this is a transparent block
                char cBlockID = cMap.CellValueAt( nCurX, nCurY, level );
                BlockType &bt = GetBlockType( cBlockID );
                bool bTrnspBlock = bt.bTransparent;
                // check for loop control if destination cell is found already
                bDestCellFound = (nCurX == int( fToX ) && nCurY == int( fToY ));

                if (bHitFound || bPrevWasTransparent) {
                    bPrevWasTransparent = bTrnspBlock;
                    nHitPointsFound += 1;
                    // set current height to new value
                    fCurHeight = cMap.CellHeightAt( nCurX, nCurY, level );
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, level, bCheckHor );
                } else if (bTrnspBlock) {
                    bPrevWasTransparent = true;
                    nHitPointsFound += 1;
                    // put the collision info in a new IntersectInfo node and push it up the hit list
                    add_hit_point( vHitList, fDistIfFound, fFromX, fFromY, fDX, fDY, nCurX, nCurY, fCurHeight, level, bCheckHor );
                }
            }
        }
        // return whether any hitpoints were found on this level
        return (nHitPointsFound > 0);
    }

    // Returns the projected bottom and top (i.e. the y screen coordinates for them) of a wall block.
    // The wall is at fCorrectedDistToWall from eye point, nHorHight is the height of the horizon, nLevelHeight
    // is the level for this block and fWallHeight is the height of the wall (in blocks) according to the map
    void CalculateWallBottomAndTop2( float fCorrectedDistToWall, int nHorHeight, int nLevelHeight, float fWallHeight, int &nWallTop, int &nWallBottom ) {
        // calculate slice height for a *unit height* wall
        int nSliceHeight = int((1.0f / fCorrectedDistToWall) * fDistToProjPlane);
        nWallTop    = nHorHeight - (nSliceHeight * (1.0f - fPlayerH)) - (nLevelHeight + fWallHeight - 1.0f) * nSliceHeight;
        nWallBottom = nWallTop + nSliceHeight * fWallHeight;
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

        // For all movements and rotation you can speed up by keeping SHIFT pressed
        // or speed down by keeping CTRL pressed. This also affects shading/lighting
        float fSpeedUp = 1.0f;
        if (GetKey( olc::SHIFT ).bHeld) fSpeedUp = 3.0f;
        if (GetKey( olc::CTRL  ).bHeld) fSpeedUp = 0.2f;

        // set test mode and test slice values
        bool bTestMode = GetKey( olc::Key::T ).bPressed;
        if (GetKey( olc::Key::F1 ).bHeld) fTestSlice = std::max( fTestSlice - 40.0f * fElapsedTime * fSpeedUp,                 0.0f );
        if (GetKey( olc::Key::F2 ).bHeld) fTestSlice = std::min( fTestSlice + 40.0f * fElapsedTime * fSpeedUp, ScreenWidth() - 1.0f );

        // reset look up value and player height on pressing 'R'
        if (GetKey( olc::R ).bReleased) { fPlayerH = 0.5f; fLookUp = 0.0f; }

        // toggles for HUDs
        if (GetKey( olc::I ).bPressed) bDebugInfo = !bDebugInfo;
        if (GetKey( olc::P ).bPressed) bMinimap   = !bMinimap;
        if (GetKey( olc::O ).bPressed) bMapRays   = !bMapRays;
        if (GetKey( olc::G ).bPressed) bTestSlice = !bTestSlice;
        if (GetKey( olc::H ).bPressed) bTestGrid  = !bTestGrid;

        // Rotate - collision detection not necessary. Keep fPlayerA_deg between 0 and 360 degrees
        if (GetKey( olc::D ).bHeld) { fPlayerA_deg += SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg >= 360.0f) fPlayerA_deg -= 360.0f; }
        if (GetKey( olc::A ).bHeld) { fPlayerA_deg -= SPEED_ROTATE * fSpeedUp * fElapsedTime; if (fPlayerA_deg <    0.0f) fPlayerA_deg += 360.0f; }
        // Rotate to discrete angles
        if (GetKey( olc::NP6 ).bPressed) { fPlayerA_deg =   0.0f; }
        if (GetKey( olc::NP3 ).bPressed) { fPlayerA_deg =  45.0f; }
        if (GetKey( olc::NP2 ).bPressed) { fPlayerA_deg =  90.0f; }
        if (GetKey( olc::NP1 ).bPressed) { fPlayerA_deg = 135.0f; }
        if (GetKey( olc::NP4 ).bPressed) { fPlayerA_deg = 180.0f; }
        if (GetKey( olc::NP7 ).bPressed) { fPlayerA_deg = 225.0f; }
        if (GetKey( olc::NP8 ).bPressed) { fPlayerA_deg = 270.0f; }
        if (GetKey( olc::NP9 ).bPressed) { fPlayerA_deg = 315.0f; }

        // variables used for collision detection - work out the new location in a seperate coordinate pair, and only alter
        // the players coordinate if there's no collision
        float fNewX = fPlayerX;
        float fNewY = fPlayerY;

        // walking forward, backward and strafing left, right
        if (GetKey( olc::W ).bHeld) { fNewX += lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY += lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk forward
        if (GetKey( olc::S ).bHeld) { fNewX -= lu_cos( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; fNewY -= lu_sin( fPlayerA_deg ) * SPEED_MOVE   * fSpeedUp * fElapsedTime; }   // walk backwards

        if (GetKey( olc::Q ).bHeld) { fNewX += lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY -= lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe left
        if (GetKey( olc::E ).bHeld) { fNewX -= lu_sin( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; fNewY += lu_cos( fPlayerA_deg ) * SPEED_STRAFE * fSpeedUp * fElapsedTime; }   // strafe right
        // collision detection - only update position if no collision
        if (!cMap.Collides( fNewX, fNewY, fPlayerH, RADIUS_PLAYER, 0.0f, 0.0f )) {
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
                float fNewHeight = fPlayerH + SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                // do CD on the height map - player velocity is not relevant since movement is up/down
                if (!cMap.Collides( fPlayerX, fPlayerY, fNewHeight, 0.1f, 0.0f, 0.0f )) {
                    fPlayerH = fNewHeight;
                    fLookUp = fCacheHorHeight - float( ScreenHeight() * fPlayerH );
                }
            }
            if (GetKey( olc::PGDN ).bHeld) {
                float fNewHeight = fPlayerH - SPEED_STRAFE_UP * fSpeedUp * fElapsedTime;
                // prevent negative height, and do CD on the height map - player velocity is not relevant since movement is up/down
                if (!cMap.Collides( fPlayerX, fPlayerY, fNewHeight, 0.1f, 0.0f, 0.0f )) {
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

        // update all objects
        for (auto &elt : vListObjects) {
            elt.Update( cMap, fElapsedTime );
        }

        // step 3 - render
        // ===============

        // BACK GROUND SCENE RENDERING
        // ===========================

        // typically, the horizon height is halfway the screen height. However, you have to offset with look up value,
        // and the viewpoint of the player is variable too (since flying and crouching)
        int nHorizonHeight = ScreenHeight() * fPlayerH + (int)fLookUp;
        float fAngleStep = fPlayerFoV_deg / float( ScreenWidth() );

        // having set the horizon height, determine the cos of all the angles through each of the pixels in this slice
        std::vector<float> fHeightAngleCos( ScreenHeight() );
        for (int y = 0; y < ScreenHeight(); y++) {
            fHeightAngleCos[y] = std::abs( lu_cos( (y - nHorizonHeight) * fAnglePerPixel_deg ));
        }

        typedef struct sDelayedPixel {
            int x, y;       // screen coordinates
            float depth;    // for depth drawing
            olc::Pixel p;
        } DelayedPixel;
        std::vector<DelayedPixel> vRenderLater;

        // iterate over all screen slices, processing the screen in columns
        for (int x = 0; x < ScreenWidth(); x++) {
            float fViewAngle = float( x - (ScreenWidth() / 2)) * fAngleStep;
            float fCurAngle = fPlayerA_deg + fViewAngle;

            float fX_hit, fY_hit;        // to hold exact (float) hit location (world space)
            int   nX_hit, nY_hit;        // to hold coords of tile that was hit (tile space)

            int   nWallTop, nWallTop2;   // to store the top and bottom y coord of the wall per column (screen space)
            int   nWallBot, nWallBot2;   // the ...2 variant represents the back of the current block

            // This lambda performs much of the sampling proces of horizontal surfaces. It can be used for floors, roofs and ceilings etc.
            // fProjDistance is the distance from the player to the hit point on the surface.
            auto generic_sampling = [=]( float fProjDistance, olc::Sprite *cTexturePtr ) -> olc::Pixel {
                // calculate the world coordinates from the distance and the view angle + player angle
                float fProjX = fPlayerX + fProjDistance * lu_cos( fCurAngle );
                float fProjY = fPlayerY + fProjDistance * lu_sin( fCurAngle );
                // calculate the sample coordinates for that world coordinate, by subtracting the
                // integer part and only keeping the fractional part. Wrap around if the result < 0 or > 1
                float fSampleX = fProjX - int(fProjX); if (fSampleX < 0.0f) fSampleX += 1.0f; if (fSampleX >= 1.0f) fSampleX -= 1.0f;
                float fSampleY = fProjY - int(fProjY); if (fSampleY < 0.0f) fSampleY += 1.0f; if (fSampleY >= 1.0f) fSampleY -= 1.0f;
                // having both sample coordinates, use the texture pointer to get the sample, and shade and return it
                return ShadePixel( cTexturePtr->Sample( fSampleX, fSampleY ), fProjDistance );
            };

            // This lambda performs much of the sampling proces of horizontal surfaces. It can be used for floors, roofs and ceilings etc.
            // fProjDistance is the distance from the player to the hit point on the surface.
            auto generic_sampling_new = [=]( float fProjDistance, int nLevel, int nFaceID ) -> olc::Pixel {
                // calculate the world coordinates from the distance and the view angle + player angle
                float fProjX = fPlayerX + fProjDistance * lu_cos( fCurAngle );
                float fProjY = fPlayerY + fProjDistance * lu_sin( fCurAngle );
                // calculate the sample coordinates for that world coordinate, by subtracting the
                // integer part and only keeping the fractional part. Wrap around if the result < 0 or > 1
                float fSampleX = fProjX - int(fProjX); if (fSampleX < 0.0f) fSampleX += 1.0f; if (fSampleX >= 1.0f) fSampleX -= 1.0f;
                float fSampleY = fProjY - int(fProjY); if (fSampleY < 0.0f) fSampleY += 1.0f; if (fSampleY >= 1.0f) fSampleY -= 1.0f;

                // select the sprite to render the ceiling depending on the block that was hit
                int nTileX = std::clamp( int( fProjX ), 0, cMap.Width() - 1);
                int nTileY = std::clamp( int( fProjY ), 0, cMap.Hight() - 1);
                char cBlockType = cMap.CellValueAt( nTileX, nTileY, nLevel );
                BlockType &sBlockHit = GetBlockType( cBlockType );
                int nSpriteIx = GetBlockFaceIx( sBlockHit, nFaceID );
                olc::Sprite *cTexturePtr = nullptr;
                switch (nFaceID) {
                    case FACE_TOP   : cTexturePtr = vRoofSprites[ nSpriteIx ]; break;
                    case FACE_BOTTOM: cTexturePtr = vCeilSprites[ nSpriteIx ]; break;
                    default: std::cout << "ERROR: generic_sampling_new() --> invalid face id: " << nFaceID << std::endl;
                }

                // having the sample coordinates and the texture pointer, get the sample, and shade and return it
                return ShadePixel( cTexturePtr->Sample( fSampleX, fSampleY ), fProjDistance );
            };

            // this lambda returns a sample of the floor through the pixel at screen coord (px, py)
            auto get_floor_sample = [=]( int px, int py ) -> olc::Pixel {
                // work out the distance to the location on the floor you are looking at through this pixel
                float fFloorProjDistance = ((fPlayerH / float( py - nHorizonHeight )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // call the generic sampler to work out the rest
                return generic_sampling( fFloorProjDistance, vFloorSprites[0] );
            };

            // this lambda returns a sample of the roof through the pixel at screen coord (px, py)
            // NOTE: fHeightWithinLevel denotes the height of the hit point on the roof. This is typically the height of the block + its level
            auto get_roof_sample = [=]( int px, int py, int nLevel, float fHeightWithinLevel, float &fRoofProjDistance ) -> olc::Pixel {
                // work out the distance to the location on the roof you are looking at through this pixel
                fRoofProjDistance = (( (fPlayerH - (float( nLevel ) + fHeightWithinLevel)) / float( py - nHorizonHeight )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // call the generic sampler to work out the rest
                return generic_sampling_new( fRoofProjDistance, nLevel, FACE_TOP );
            };

            // this lambda returns a sample of the ceiling through the pixel at screen coord (px, py)
            // NOTE: fHeightWithinLevel denotes the height of the hit point on the ceiling. This is typically the level of the block, WITHOUT its height!
            auto get_ceil_sample = [=]( int px, int py, int nLevel, float fHeightWithinLevel, float &fCeilProjDistance ) -> olc::Pixel {
                // work out the distance to the location on the ceiling you are looking at through this pixel
                fCeilProjDistance = (( (float( nLevel ) - fPlayerH) / float( nHorizonHeight - py )) * fDistToProjPlane) / lu_cos( fViewAngle );
                // call the generic sampler to work out the rest
                return generic_sampling_new( fCeilProjDistance, nLevel, FACE_BOTTOM );
            };

            // prepare the rendering for this slice by calculating the list of intersections in this ray's direction
            // for each level, get the list of hitpoints in that level, work out front and back distances and projections
            // on screen, and add to the global vHitPointList
            std::vector<IntersectInfo> vHitPointList;
            for (int k = 0; k < cMap.NrOfLayers(); k++) {

                std::vector<IntersectInfo> vCurLevelList;
                GetDistancesToWallsPerLevel( k, fCurAngle, vCurLevelList );

                for (int i = 0; i < (int)vCurLevelList.size(); i++) {
                    // make correction for the fish eye effect
                    vCurLevelList[i].fDistFrnt *= lu_cos( fViewAngle );
                    // calculate values for the on screen projections top_front and top_bottom
                    CalculateWallBottomAndTop2(
                        vCurLevelList[i].fDistFrnt,
                        nHorizonHeight,
                        vCurLevelList[i].nLevel,
                        vCurLevelList[i].fHeight,
                        vCurLevelList[i].top_front,
                        vCurLevelList[i].bot_front
                    );
                }
                // Extend the hit list with projected ceiling info for the back of the wall
                for (int i = 0; i < (int)vCurLevelList.size(); i++) {
                    if (i == (int)vCurLevelList.size() - 1) {
                        // last element, has no successor
                        vCurLevelList[i].fDistBack = vCurLevelList[i].fDistFrnt;
                        vCurLevelList[i].top_back  = vCurLevelList[i].top_front;
                        vCurLevelList[i].bot_back  = vCurLevelList[i].bot_front;
                    } else {
                        // calculate values for the on screen projections top_front and top_bottom
                        vCurLevelList[i].fDistBack = vCurLevelList[i + 1].fDistFrnt;
                        CalculateWallBottomAndTop2(
                            vCurLevelList[i].fDistBack,
                            nHorizonHeight,
                            vCurLevelList[i].nLevel,
                            vCurLevelList[i].fHeight,
                            vCurLevelList[i].top_back,
                            vCurLevelList[i].bot_back
                        );
                    }
                }
                // add the hit points for this level list to the combined hit point list
                vHitPointList.insert( vHitPointList.end(), vCurLevelList.begin(), vCurLevelList.end());
            }

            // populate ray list for rendering mini map
            if (bMinimap && !vHitPointList.empty()) {
                olc::vf2d curHitPoint = { vHitPointList[0].fHitX, vHitPointList[0].fHitY };
                vRayList.push_back( curHitPoint );
            }

            // remove all hit points with height 0.0f - they were necessary for calculating the back face projection
            // of blocks, but that part is done now
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

            // sort hit points from far away to close by (painters algorithm)
            std::sort(
                vHitPointList.begin(),
                vHitPointList.end(),
                []( IntersectInfo &a, IntersectInfo &b ) {
                    return (a.fDistFrnt > b.fDistFrnt) ||
                           (a.fDistFrnt == b.fDistFrnt && a.nLevel < b.nLevel);
                }
            );

            // start rendering this slice by putting sky and floor in it
            float fWellAway = fMaxDistance + 100.0f;
            for (int y = ScreenHeight() - 1; y >= 0; y--) {
                // reset depth buffer for this slice
                fDepthBuffer[ y * ScreenWidth() + x ] = fWellAway;
                // draw floor and horizon
                if (y < nHorizonHeight) {
                    olc::Pixel skySample = olc::CYAN;
                    DrawDepth( fWellAway, x, y, skySample );
                } else {
                    olc::Pixel floorSample = get_floor_sample( x, y );   // shading is done in get_floor_sample()
                    DrawDepth( fWellAway, x, y, floorSample );
                }
            }

            // now render all hit points back to front
            for (auto &hitRec : vHitPointList) {

                float fBlockElevation = 1.0f;
                int   nBlockLevel     = 0;
                float fFrntDistance   = 0.0f;     // distance var also used for wall shading
                int   nFaceHit        = FACE_UNKNOWN;

                // For the distance calculations we needed also points where the height returns to 0.0f (the
                // back faces of the block). For the rendering we must skip these "hit points"
                if (hitRec.fHeight > 0.0f) {
                    // load the info from next hit point
                    fX_hit          = hitRec.fHitX;
                    fY_hit          = hitRec.fHitY;
                    nX_hit          = hitRec.nMapCoordX;
                    nY_hit          = hitRec.nMapCoordY;
                    fBlockElevation = hitRec.fHeight;
                    nBlockLevel     = hitRec.nLevel;
                    fFrntDistance   = hitRec.fDistFrnt;
                    nFaceHit        = hitRec.nFaceHit;
                    // make sure the screen y coordinate is within screen boundaries
                    nWallTop        = std::clamp( hitRec.top_front, 0, ScreenHeight() - 1 );
                    nWallTop2       = std::clamp( hitRec.top_back , 0, ScreenHeight() - 1 );
                    nWallBot        = std::clamp( hitRec.bot_front, 0, ScreenHeight() - 1 );
                    nWallBot2       = std::clamp( hitRec.bot_back , 0, ScreenHeight() - 1 );

                    // render roof segment if it's visible
                    for (int y = nWallTop2; y < nWallTop; y++) {
                        // the distance to this point is calculated and passed from get_roof_sample
                        float fRenderDistance;
                        olc::Pixel roofSample = get_roof_sample( x, y, nBlockLevel, fBlockElevation, fRenderDistance );   // shading is done in get_roof_sample()
                        DrawDepth( fRenderDistance / fHeightAngleCos[y], x, y, roofSample );
                    }

                    // render wall segment
                    float fSampleX = -1.0f;
                    for (int y = nWallTop; y <= nWallBot; y++) {

                        // first get x sample coordinate from face hit info
                        if (fSampleX == -1.0f) {
                            switch (nFaceHit) {
                                case FACE_SOUTH:
                                case FACE_NORTH: fSampleX = fX_hit - (float)nX_hit; break;
                                case FACE_EAST :
                                case FACE_WEST : fSampleX = fY_hit - (float)nY_hit; break;
                                default        : std::cout << "ERROR: OnUserUpdate() --> invalid face value: " << nFaceHit << std::endl;
                            }
                        }

                        float fSampleY;
                        // the y sample coordinate depends only on the pixel y coord on the screen
                        // in relation to the vertical space the wall is taking up
                        fSampleY = fBlockElevation * float(y - hitRec.top_front) / float(hitRec.bot_front - hitRec.top_front);
                        // select the sprite to render the wall depending on the block that was hit
                        char cBlockType = cMap.CellValueAt( nX_hit, nY_hit, nBlockLevel );
                        BlockType &sBlockHit = GetBlockType( cBlockType );
                        int nWallSpriteIx = GetBlockFaceIx( sBlockHit, nFaceHit );
                        olc::Sprite *pSpritePtr = vWallSprites[ nWallSpriteIx ];
                        // having the sample coordinates and the sprite pointer, get the sample, shade it and draw the pixel
                        olc::Pixel wallSample = ShadePixel( pSpritePtr->Sample( fSampleX, fSampleY ), fFrntDistance );

                        // render or store for later rendering, depending on the block type
                        if (sBlockHit.bTransparent) {
                            DelayedPixel aux = { x, y, fFrntDistance / fHeightAngleCos[y], wallSample };
                            vRenderLater.push_back( aux );
                        } else {
                            DrawDepth( fFrntDistance / fHeightAngleCos[y], x, y, wallSample );
                        }
                    }

                    // render ceiling segment if it's visible
                    for (int y = nWallBot + 1; y <= nWallBot2; y++) {
                        float fRenderDistance;
                        olc::Pixel ceilSample = get_ceil_sample( x, y, nBlockLevel, fBlockElevation, fRenderDistance );   // shading is done in get_ceil_sample()
                        DrawDepth( fRenderDistance / fHeightAngleCos[y], x, y, ceilSample );
                    }
                }
            }
            // if test mode triggerd, print the hit list along the test slice
            if (bTestMode && x == int( fTestSlice )) {
                PrintHitList( vHitPointList, true );
            }
        }

        // DELAYED WALL RENDERING (with masking of blank pixels)
        for (auto &elt : vRenderLater) {
            if (elt.p != olc::BLANK) {
                DrawDepth( elt.depth, elt.x, elt.y, elt.p );
            }
        }

        // OBJECT RENDERING
        // ================

        // display all objects after the background rendering and before displaying the minimap or debugging output
        float fPlayerFoV_rad = deg2rad( fPlayerFoV_deg );
        // split the rendering into two phase so that it can be sorted on distance (painters algo) before rendering

        // phase 1 - just determine distance (and angle cause of convenience)
        for (auto &object : vListObjects) {
            // can object be seen?
            float fVecX = object.GetX() - fPlayerX;
            float fVecY = object.GetY() - fPlayerY;
            object.SetDistToPlayer( sqrtf( fVecX * fVecX + fVecY * fVecY ));
            // calculate angle between vector from player to object, and players looking direction
            // to determine if object is in players field of view
            float fEyeX = lu_cos( fPlayerA_deg );
            float fEyeY = lu_sin( fPlayerA_deg );
            float fObjA = atan2f( fVecY, fVecX ) - atan2f( fEyeY, fEyeX );
            // "bodge" angle into range [ -PI, PI ]
            if (fObjA < - PI) fObjA += 2.0f * PI;
            if (fObjA >   PI) fObjA -= 2.0f * PI;
            object.SetAngleToPlayer( fObjA );

            // experiment
            object.SetDistToPlayer( object.GetDistToPlayer() * lu_cos( fObjA ));
        }

        // sort for painters algo
        vListObjects.sort(
            [=]( RC_Object &a, RC_Object &b ) {
                return a.GetDistToPlayer() > b.GetDistToPlayer();
            }
        );

        // phase 2: render it from large to smaller distances
        for (auto &object : vListObjects) {
            // can object be seen?
            float fObjDist = object.GetDistToPlayer();
            float fObjA = object.GetAngleToPlayer();
            // determine whether object is in field of view (a bit larger to prevent objects being not rendered at
            // screen boundaries)
            bool bInFOV = fabs( fObjA ) < fPlayerFoV_rad / 1.2f;

            // render object only when within Field of View, and within visible distance.
            // the check on proximity is to prevent asymptotic errors when distance to player becomes very small
            if (bInFOV && fObjDist >= 0.3f && fObjDist < fMaxDistance) {

                // determine the difference between standard player height (i.e. 0.5f = standing on the floor)
                // and current player height
                float fCompensatePlayerHeight = fPlayerH - 0.5f;
                // get the projected (halve) slice height of this object
                float fObjHalveSliceHeight     = float( ScreenHeight()                      / fObjDist);
                float fObjHalveSliceHeightScld = float((ScreenHeight() * object.GetScale()) / fObjDist);

                // work out where objects floor and ceiling are (in screen space)
                // due to scaling factor, differentiated a normalized (scale = 1.0f) ceiling and a scaled variant
                float fObjCeilingNormalized = float(nHorizonHeight) - fObjHalveSliceHeight;
                float fObjCeilingScaled     = float(nHorizonHeight) - fObjHalveSliceHeightScld;
                // and adapt all the scaling into the ceiling value
                float fScalingDifference = fObjCeilingNormalized - fObjCeilingScaled;
                float fObjCeiling = fObjCeilingNormalized - 2 * fScalingDifference;
                float fObjFloor   = float(nHorizonHeight) + fObjHalveSliceHeight;

                // compensate object projection heights for elevation of the player
                fObjCeiling += fCompensatePlayerHeight * fObjHalveSliceHeight * 2.0f;
                fObjFloor   += fCompensatePlayerHeight * fObjHalveSliceHeight * 2.0f;

                // get height, aspect ratio and width
                float fObjHeight = fObjFloor - fObjCeiling;
                float fObjAR = float( object.GetSprite()->height ) / float( object.GetSprite()->width );
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
                            olc::Pixel objSample = ShadePixel( object.GetSprite()->Sample( fSampleX, fSampleY ), fObjDist );
                            if (objSample != olc::BLANK) {
                                DrawDepth( fObjDist, nObjColumn, fObjCeiling + fy, objSample );
                            }
                        }
                    }
                }
            }
        }

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

        if (bMinimap) {
            RenderMapGrid();
            if (bMapRays) {
                RenderMapRays();
            }
            RenderMapPlayer();
            RenderMapObjects();

            vRayList.clear();
        }

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
	if (demo.Construct( SCREEN_X / PIXEL_SIZE, SCREEN_Y / PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE ))
		demo.Start();

	return 0;
}


// ==============================/   put bloat behind main()  /==============================

// ==============================/   Mini map rendering stuff   /==============================

// function to render the mini map on the screen
void MyRayCaster::RenderMapGrid() {
    // fill background for minimap
    float fMMFactor = MINIMAP_SCALE_FACTOR * MINIMAP_TILE_SIZE;
    FillRect( 0, 0, cMap.Width() * fMMFactor, cMap.Hight() * fMMFactor, HUD_BG_COLOUR );
    // draw each tile
    for (int y = 0; y < cMap.Hight(); y++) {
        for (int x = 0; x < cMap.Width(); x++) {
            // colour different for different heights
            olc::Pixel p;
            bool bBorderFlag = true;
            if (cMap.CellHeight( x, y ) == 0.0f) {
                p = HUD_BG_COLOUR;   // don't visibly render
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
    for (auto &elt : vListObjects) {

        olc::Pixel p = (elt.bStatic ? olc::RED : olc::MAGENTA);

        float px = elt.GetX() * fMMFactor;
        float py = elt.GetY() * fMMFactor;
        float pr = 0.4f  * fMMFactor;
        FillCircle( px, py, pr, p );

        if (!elt.bStatic) {
            float dx = lu_cos( rad2deg( elt.GetAngle()));
            float dy = lu_sin( rad2deg( elt.GetAngle()));
            float pdx = dx * 0.3f * elt.GetSpeed() * fMMFactor;
            float pdy = dy * 0.3f * elt.GetSpeed() * fMMFactor;
            DrawLine( px, py, px + pdx, py + pdy, p );
        }
    }
}

// function to render debug info in a separate hud on the screen
void MyRayCaster::RenderDebugInfo() {
    int nStartX = ScreenWidth() - 200;
    int nStartY =  10;
    // render background pane for debug info
    FillRect( nStartX, nStartY, 195, 85, HUD_BG_COLOUR );
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

#define SIGNIFICANCE 3      // float angle is rounded at three decimal points
#define SIG_POW10    1000

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


// convenience functions for random range integers and floats
int int_rand_between( int nLow, int nHgh ) {
    return (rand() % (nHgh - nLow + 1)) + nLow;
}

float float_rand_between( float fLow, float fHgh ) {
    int nLow = int( 1000.0f * fLow );
    int nHgh = int( 1000.0f * fHgh );
    return float( int_rand_between( nLow, nHgh )) / 1000.0f;
}

// ==============================/  end of file   /==============================
