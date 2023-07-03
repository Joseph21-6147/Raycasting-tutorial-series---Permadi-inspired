#include "RC_Face.h"

// ==============================/  face blue print stuff   /==============================

// The library of faces is modeled as a vector, and can be indexed directly
std::vector<FaceBluePrint> vFaceBluePrintLib;

// Convenience function to add one face configuration
void AddFaceBluePrint( int nID, int nType, int nIndex, bool bT, bool bA ) {
    if (nID != (int)vFaceBluePrintLib.size()) {
        std::cout << "ERROR - AddFaceBluePrint() --> add order violated, id passed = " << nID << " and should have been " << (int)vFaceBluePrintLib.size() << std::endl;
    }
    FaceBluePrint sTmpFaceBP = { nID, nType, nIndex, bT, bA };
    vFaceBluePrintLib.push_back(sTmpFaceBP );
}

// Put all face configs you need into this function. Here the relation between the face index in the face blueprint lib
// and the sprite for that face are made, as well as the animation abilities of this face.
void InitFaceBluePrints() {
    AddFaceBluePrint(  0, TYPE_FACE_WALL,  0 );
    AddFaceBluePrint(  1, TYPE_FACE_WALL,  1 );
    AddFaceBluePrint(  2, TYPE_FACE_WALL,  2 );
    AddFaceBluePrint(  3, TYPE_FACE_WALL,  3 );
    AddFaceBluePrint(  4, TYPE_FACE_WALL,  4, true, true );    // animated gate blueprint
    AddFaceBluePrint(  5, TYPE_FACE_WALL,  5 );
    AddFaceBluePrint(  6, TYPE_FACE_WALL,  6, true );  // transparent, but not animated
    AddFaceBluePrint(  7, TYPE_FACE_WALL,  7, true );
    AddFaceBluePrint(  8, TYPE_FACE_WALL,  0 );    // fill out so that roof textures start at index 10
    AddFaceBluePrint(  9, TYPE_FACE_WALL,  0 );

    AddFaceBluePrint( 10, TYPE_FACE_ROOF,  0 );
    AddFaceBluePrint( 11, TYPE_FACE_ROOF,  1 );
    AddFaceBluePrint( 12, TYPE_FACE_ROOF,  2 );
    AddFaceBluePrint( 13, TYPE_FACE_ROOF,  3 );
    AddFaceBluePrint( 14, TYPE_FACE_ROOF,  4 );
    AddFaceBluePrint( 15, TYPE_FACE_ROOF,  5 );
    AddFaceBluePrint( 16, TYPE_FACE_ROOF,  6 );
    AddFaceBluePrint( 17, TYPE_FACE_ROOF,  7 );
    AddFaceBluePrint( 18, TYPE_FACE_ROOF,  0 );    // fill out so that ceiling textures start at index 20
    AddFaceBluePrint( 19, TYPE_FACE_ROOF,  0 );

    AddFaceBluePrint( 20, TYPE_FACE_CEIL,  0 );
    AddFaceBluePrint( 21, TYPE_FACE_CEIL,  1 );
    AddFaceBluePrint( 22, TYPE_FACE_CEIL,  2 );
    AddFaceBluePrint( 23, TYPE_FACE_CEIL,  3 );
    AddFaceBluePrint( 24, TYPE_FACE_CEIL,  4 );
    AddFaceBluePrint( 25, TYPE_FACE_CEIL,  5 );
    AddFaceBluePrint( 26, TYPE_FACE_CEIL,  6 );
    AddFaceBluePrint( 27, TYPE_FACE_CEIL,  7 );
    AddFaceBluePrint( 28, TYPE_FACE_CEIL,  0 );
    AddFaceBluePrint( 29, TYPE_FACE_CEIL,  0 );
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

void RC_FaceAnimated::Init( int nFaceIx, olc::Sprite *sprPtr, bool bTrnsp, int s, int tw, int th ) {
    nFaceIndex   = nFaceIx;
    pSprite      = sprPtr;
    bTransparent = bTrnsp;
    state        = s;
    tileWidth    = tw;
    tileHight    = th;

    SetState( state );

    fTimer   = fThreshold = 0.0f;
    nCounter = nThreshold = 0;
}

// a face is either called animated or called textured
bool RC_FaceAnimated::IsTextured() { return false; }
bool RC_FaceAnimated::IsAnimated() { return true; }

int RC_FaceAnimated::GetState() { return state; }
// NOTE - contains hardcoded values currently!
void RC_FaceAnimated::SetState( int newState ) {
    state = newState;
    switch ( state ) {
        case ANIM_STATE_CLOSED : tileX = 0; tileY = 0; fTimer = 0.0f; fThreshold = 0.0f; nCounter = 0; nThreshold = 1; break;
        case ANIM_STATE_OPENED : tileX = 7; tileY = 0; fTimer = 0.0f; fThreshold = 0.0f; nCounter = 0; nThreshold = 1; break;

        case ANIM_STATE_CLOSING: tileX = 7; tileY = 0; fTimer = 0.0f; fThreshold = 0.10f; nCounter = 0; nThreshold = 8; break;
        case ANIM_STATE_OPENING: tileX = 0; tileY = 0; fTimer = 0.0f; fThreshold = 0.10f; nCounter = 0; nThreshold = 8; break;
    }
}

void RC_FaceAnimated::Update( float fElapsedTime, bool &bPermeable ) {
    fTimer += fElapsedTime;
    if (fTimer >= fThreshold) {
        fTimer -= fThreshold;

        // one tick gone by, advance counter
        nCounter += 1;
        if (nCounter == nThreshold) {
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

// ==============================/  end of file   /==============================
