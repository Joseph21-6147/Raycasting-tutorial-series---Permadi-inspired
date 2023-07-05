#include "RC_Face.h"

// ==============================/  face blue print stuff   /==============================

// The library of faces is modeled as a std::vector, and can be indexed directly
std::vector<FaceBluePrint> vFaceBluePrintLib;

// Convenience function to add one face configuration - enables error checking on input data
void AddFaceBluePrint(
    FaceBluePrint &rFBP,
    std::vector<olc::Sprite *> wallSprites,
    std::vector<olc::Sprite *> ceilSprites,
    std::vector<olc::Sprite *> roofSprites
) {
    // check on insertion order
    if (rFBP.nID != (int)vFaceBluePrintLib.size()) {
        std::cout << "ERROR - AddFaceBluePrint() --> add order violated, id passed = " << rFBP.nID << " and should have been " << (int)vFaceBluePrintLib.size() << std::endl;
    }
    // combined check on face type and on index in range of associated sprite pointer container
    auto check_index_with_file_list = [=]( int nIndex, int nSize, std::string sTypeString ) {
        if (nIndex < 0 || nIndex >= nSize) {
            std::cout << "ERROR - AddFaceBluePrint() --> " << sTypeString << " face index out of range: " << nIndex << " (should be < " << nSize << ")" << std::endl;
        }
    };
    switch( rFBP.nFaceType ) {
        case TYPE_FACE_WALL: check_index_with_file_list( rFBP.nFaceIndex, (int)wallSprites.size(), "Wall"    ); break;
        case TYPE_FACE_CEIL: check_index_with_file_list( rFBP.nFaceIndex, (int)ceilSprites.size(), "Ceiling" ); break;
        case TYPE_FACE_ROOF: check_index_with_file_list( rFBP.nFaceIndex, (int)roofSprites.size(), "Roof"    ); break;
        default: std::cout << "ERROR - AddFaceBluePrint() --> unknown face type: " << rFBP.nFaceType << std::endl;
    }
    vFaceBluePrintLib.push_back( rFBP );
}

// Uses the data from vInitFaceBluePrint to populate the library of faces (vFaceBluePrintLib)
// This construction decouples the blue print definition from its use, and enables error checking
// on the blue print data
void InitFaceBluePrints(
    std::vector<olc::Sprite *> wallSprites,
    std::vector<olc::Sprite *> ceilSprites,
    std::vector<olc::Sprite *> roofSprites
) {
    for (auto elt : vInitFaceBluePrints) {
        AddFaceBluePrint( elt, wallSprites, ceilSprites, roofSprites );
    }
}

// ==============================/  class RC_Face  /==============================

RC_Face::RC_Face() {}
RC_Face::~RC_Face() {}

void RC_Face::Init( int nFaceIx, olc::Sprite *sprPtr, bool bTrnsp ) {
    nFaceIndex   = nFaceIx;
    pSprite      = sprPtr;
    bTransparent = bTrnsp;
}

int  RC_Face::GetIndex() { return nFaceIndex; }
void RC_Face::SetIndex( int nIndex ) { nFaceIndex = nIndex; }

olc::Sprite *RC_Face::GetTexture() { return pSprite; }
void         RC_Face::SetTexture( olc::Sprite *sprPtr ) { pSprite = sprPtr; }

// per default a face is "just" textured and not animated
bool RC_Face::IsTextured() { return true; }
bool RC_Face::IsAnimated() { return false; }
bool RC_Face::IsPortal()   { return false; }

bool RC_Face::IsTransparent() { return bTransparent; }
void RC_Face::SetTransparent( bool bParam ) { bTransparent = bParam; }

// if not overriden, a face has no update behaviour
void RC_Face::Update( float fElapsedTime, bool &bPermFlag ) {}

// if not overriden, this is a regular (textured) face, and sampling is done on its sprite
olc::Pixel RC_Face::Sample( float sX, float sY ) {
    if (pSprite == nullptr) {
        std::cout << "ERROR: Sample() --> nullptr sprite ptr encountered" << std::endl;
        return olc::MAGENTA;
    }
    return pSprite->Sample( sX, sY );
}

// ==============================/  class RC_FaceAnimated  /==============================

RC_FaceAnimated::RC_FaceAnimated() {}

void RC_FaceAnimated::Init( int nFaceIx, olc::Sprite *sprPtr, bool bTrnsp, int st, int tw, int th ) {
    nFaceIndex   = nFaceIx;
    pSprite      = sprPtr;
    bTransparent = bTrnsp;
    state        = st;
    tileWidth    = tw;
    tileHight    = th;

    SetState( state );

    fTimer   = fTickTime = 0.0f;
    nCounter = nNrFrames = 0;
}

// a face is either called animated or called textured
bool RC_FaceAnimated::IsTextured() { return false; }
bool RC_FaceAnimated::IsAnimated() { return true; }
bool RC_FaceAnimated::IsPortal()   { return false; }

int RC_FaceAnimated::GetState() { return state; }
// NOTE - contains hardcoded values currently!
void RC_FaceAnimated::SetState( int newState ) {
    state = newState;
    switch ( state ) {
        case ANIM_STATE_CLOSED : tileX = 0; tileY = 0; fTimer = 0.0f; fTickTime = 0.00f; nCounter = 0; nNrFrames = 1; break;
        case ANIM_STATE_OPENED : tileX = 7; tileY = 0; fTimer = 0.0f; fTickTime = 0.00f; nCounter = 0; nNrFrames = 1; break;

        case ANIM_STATE_CLOSING: tileX = 7; tileY = 0; fTimer = 0.0f; fTickTime = 0.10f; nCounter = 0; nNrFrames = 8; break;
        case ANIM_STATE_OPENING: tileX = 0; tileY = 0; fTimer = 0.0f; fTickTime = 0.10f; nCounter = 0; nNrFrames = 8; break;
    }
}

// NOTE - contains hardcoded values currently!
void RC_FaceAnimated::Update( float fElapsedTime, bool &bPermeable ) {
    fTimer += fElapsedTime;
    if (fTimer >= fTickTime) {
        fTimer -= fTickTime;

        // a tick has passed, advance frame counter
        nCounter += 1;
        if (nCounter == nNrFrames) {
            // animation sequence has finished
            nCounter = 0;
            switch (state) {
                case ANIM_STATE_CLOSED: /* no action needed */ break;
                case ANIM_STATE_OPENED: /* no action needed */ break;
                case ANIM_STATE_CLOSING:
                    // was closing and animation sequence terminated - set to closed
                    SetState( ANIM_STATE_CLOSED );
                    break;
                case ANIM_STATE_OPENING:
                    // was opening and animation sequence terminated - set to opened ...
                    SetState( ANIM_STATE_OPENED );
                    // ... and make permeable
                    bPermeable = true;
                    break;
            }
        } else {
            switch (state) {
                case ANIM_STATE_CLOSED: /* no action needed */ break;
                case ANIM_STATE_OPENED: /* no action needed */ break;

                // NOTE - sprite sheet specifics here!!
                case ANIM_STATE_CLOSING: tileX -= 1; bPermeable = false; break;
                case ANIM_STATE_OPENING: tileX += 1;                     break;
            }
        }
    }
}

// convert normalized sampling coordinates (sx, sy) into the subsprite that is currently active as (tileX, tileY)
// and returns the sampled pixel
olc::Pixel RC_FaceAnimated::Sample( float sX, float sY ) {
    if (pSprite == nullptr) {
        std::cout << "WARNING: Sample() --> nullptr sprite ptr encountered" << std::endl;
        return olc::MAGENTA;
    } else {
        float fx0 = float( ( tileX + sX ) * tileWidth ) / pSprite->width;
        float fy0 = float( ( tileY + sY ) * tileHight ) / pSprite->height;

        return pSprite->Sample( fx0, fy0 );
    }
}

// ==============================/  class RC_FacePortal  /==============================

RC_FacePortal::RC_FacePortal() {}

void RC_FacePortal::Init( int nFaceIx, olc::Sprite *sprPtr, int _nFromMap, int _nFromLevel, int _nFromX, int _nFromY, int _nToMap, int _nToLevel, int _nToX, int _nToY, float _fToA ) {
    nFaceIndex   = nFaceIx;
    pSprite      = sprPtr;
    bTransparent = true;
    nFmMap       = _nFromMap;
    nFmLevel     = _nFromLevel;
    nFmX         = _nFromX;
    nFmY         = _nFromY;
    nToMap       = _nToMap;
    nToLevel     = _nToLevel;
    nToX         = _nToX;
    nToY         = _nToY;

    fToAngle     = _fToA;
}

// no dynamic behaviour implemented (yet)
void RC_FacePortal::Update( float fElapsedTime, bool &bPermFlag ) {}

bool RC_FacePortal::IsTextured() { return false; }
bool RC_FacePortal::IsAnimated() { return false; }
bool RC_FacePortal::IsPortal()   { return true;  }

int RC_FacePortal::GetFromMap()   { return nFmMap;   }
int RC_FacePortal::GetFromLevel() { return nFmLevel; }
int RC_FacePortal::GetFromX()     { return nFmX;     }
int RC_FacePortal::GetFromY()     { return nFmY;     }

int RC_FacePortal::GetToMap()   { return nToMap;   }
int RC_FacePortal::GetToLevel() { return nToLevel; }
int RC_FacePortal::GetToX()     { return nToX;     }
int RC_FacePortal::GetToY()     { return nToY;     }

float RC_FacePortal::GetToAngle() { return fToAngle; }


//int RC_FacePortal::GetExitDir() { return nFaceIndex; }
// the exit direction of the portal is the opposite of the face position
int RC_FacePortal::GetExitDir() {
    int nResult = FACE_UNKNOWN;
    switch (nFaceIndex) {
        case FACE_EAST : nResult = FACE_WEST;  break;
        case FACE_NORTH: nResult = FACE_SOUTH; break;
        case FACE_WEST : nResult = FACE_EAST;  break;
        case FACE_SOUTH: nResult = FACE_NORTH; break;
        default: std::cout << "ERROR: RC_FacePortal::GetExitDir() --> invalid exit direction: " << nFaceIndex << std::endl;
    }
    return nResult;
}

// returns the angle associated with the exit direction: EAST = 0.0f, SOUTH = 90.0f, etc
float RC_FacePortal::GetExitAngleDeg() {
    float fResult_deg = -1.0f;
    switch (GetExitDir()) {
        case FACE_EAST : fResult_deg =   0.0f; break;
        case FACE_SOUTH: fResult_deg =  90.0f; break;
        case FACE_WEST : fResult_deg = 180.0f; break;
        case FACE_NORTH: fResult_deg = 270.0f; break;
        default: std::cout << "ERROR: RC_FacePortal::GetExitAngleDeg() --> invalid exit direction: " << GetExitDir() << std::endl;
    }
    return fResult_deg;
}

// this function returns true if the previous and current locations are on different sides of this face.
bool RC_FacePortal::HasCrossedPortal( float fPh_prev, float fPx_prev, float fPy_prev, float fPh, float fPx, float fPy, int nPh, int nPx, int nPy ) {

    bool bResult = false;

    auto is_in_map_cell = [=]( float fx, float fy, float fz, int nx, int ny, int nz ) {
        return (
            int( fx ) == nx &&
            int( fy ) == ny &&
            int( fz ) == nz
        );
    };

    // check that current position is inside map cell containing this portal, and
    // previous position was not inside this map cell
    if ( is_in_map_cell( fPx, fPy, fPh, nPx, nPy, nPh ) &&
        !is_in_map_cell( fPx_prev, fPy_prev, fPh_prev, nPx, nPy, nPh )) {
            // also check if the two coordinate tupels differ in the correct way
            switch (nFaceIndex) {
                case FACE_EAST : bResult = int( fPx_prev ) >  int( fPx ) && int( fPy_prev ) == int( fPy ) && int( fPh_prev ) == int( fPh ); break;
                case FACE_WEST : bResult = int( fPx_prev ) <  int( fPx ) && int( fPy_prev ) == int( fPy ) && int( fPh_prev ) == int( fPh ); break;
                case FACE_NORTH: bResult = int( fPx_prev ) == int( fPx ) && int( fPy_prev ) <  int( fPy ) && int( fPh_prev ) == int( fPh ); break;
                case FACE_SOUTH: bResult = int( fPx_prev ) == int( fPx ) && int( fPy_prev ) >  int( fPy ) && int( fPh_prev ) == int( fPh ); break;
            }
        }

    return bResult;
}

// ==============================/  end of file   /==============================
