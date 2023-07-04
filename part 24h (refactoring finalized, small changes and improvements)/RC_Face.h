#ifndef RC_FACE_H
#define RC_FACE_H

#include "olcPixelGameEngine.h"

//////////////////////////////////  FACE BLUEPRINTS  //////////////////////////////////////

/* For faces and map cells you can define blueprints, that are used to build up the map. So the face blueprints are the
 * components for dressing the map cell blueprints, which in turn are used to define the map.
 *
 * This way you can define a character based map and have all kinds of behaviour in it: textured, animated
 *
 * See also the description in RC_Map.h
 */

// ==============================/  FaceBluePrint stuff   /==============================

// constants for identifying the face type
#define TYPE_FACE_WALL 0
#define TYPE_FACE_CEIL 1
#define TYPE_FACE_ROOF 2

typedef struct sFaceBluePrintStruct {
    int nID;           // id of this blue print
    int nFaceType;     // to determine if a wall, ceiling or roof sprite must be used
    int nFaceIndex;    // index into vWallSprites, vRoofSprites or vCeilSprite, depending on nFaceType
    bool bTransparent = false;     // "see-through" face - implemented with delayed rendering
    bool bAnimated    = false;
} FaceBluePrint;

// This container holds the data to initialize the face blueprint library
extern std::vector<FaceBluePrint> vInitFaceBluePrints;
// The library of faces is modeled as a std::vector, and can be indexed directly
extern std::vector<FaceBluePrint> vFaceBluePrintLib;
// Convenience function to add one face configuration - enables checking on input data
void AddFaceBluePrint( FaceBluePrint &rFBP );
// Uses the data from vInitFaceBluePrint to populate the library of faces (vFaceBluePrintLib)
// This construction decouples the blue print definition from its use, and enables error checking
// on the blue print data
void InitFaceBluePrints();

//////////////////////////////////  RC_Face   //////////////////////////////////////////

/* In its most basic form an RC_Face is just a texture. More advanced faces are animated (RC_FaceAnimated object)
 * and can have some kind of behaviour.
 */

 // constants for identifying the faces of a block
#define FACE_UNKNOWN  -1
#define FACE_EAST      0
#define FACE_NORTH     1
#define FACE_WEST      2
#define FACE_SOUTH     3
#define FACE_TOP       4
#define FACE_BOTTOM    5
#define FACE_NR_OF     6

// ==============================/  class RC_Face  /==============================

class RC_Face {

protected:
    int nFaceIndex;                   // one of FACE_EAST ... FACE_BOTTOM

    olc::Sprite *pSprite = nullptr;   // sprite or spritesheet for this face

    bool bTransparent = false;

public:
    RC_Face();
    ~RC_Face();

    virtual void Init( int nFaceIx, olc::Sprite *sprPtr, bool bTrnsp = false );

    int  GetIndex();
    void SetIndex( int nIndex );

    olc::Sprite *GetTexture();
    void         SetTexture( olc::Sprite *sprPtr );

    // per default a face is "just" textured and not animated
    virtual bool IsTextured();
    virtual bool IsAnimated();

    bool IsTransparent();
    void SetTransparent( bool bParam = true );

    // if not overriden, a face has no update behaviour
    virtual void Update( float fElapsedTime, bool &bPermFlag );

    virtual olc::Pixel Sample( float sX, float sY );
};

// ==============================/  class RC_FaceAnimated  /==============================

// constants for animation states
#define ANIM_STATE_CLOSED   0
#define ANIM_STATE_OPENED   1
#define ANIM_STATE_CLOSING  2
#define ANIM_STATE_OPENING  3

class RC_FaceAnimated : public RC_Face {

protected:

    int state;          // one of above constants

    int tileWidth, tileHight;     // the sprite pointer is assumed to point to an animated sprite sheet
    int tileX, tileY;             // these values are needed for animation of that sprite sheet

    float fTimer, fTickTime;      // these values control the speed and nr of steps of the animation
    int nCounter, nNrFrames;

public:
    RC_FaceAnimated();

    void Init( int nFaceIx, olc::Sprite *sprPtr, bool bTrnsp, int s, int tw, int th );

    // per default a face is "just" textured and not animated
    bool IsTextured() override;
    bool IsAnimated() override;

    int  GetState();
    void SetState( int newState );

    void Update( float fElapsedTime, bool &bPermeable ) override;

    // convert normalized sampling coordinates (sx, sy) into the subsprite that is currently active as (tileX, tileY)
    // and returns the sampled pixel
    olc::Pixel Sample( float sX, float sY ) override;
};

#endif // RC_FACE_H
