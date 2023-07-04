#ifndef RC_OBJECT_H
#define RC_OBJECT_H

#include "RC_DepthDrawer.h"
#include "RC_Map.h"
#include "RC_Misc.h"

// constants for collision detection with walls
#define RADIUS_PLAYER   0.1f
#define RADIUS_ELF      0.2f

// test objects
#define TEST_OBJ_PERCENTAGE   0.02f    // this percent of *empty* tiles will be used as the nr of test objects
#define MIN_DYNAMIC_OBJS      2        // the first x objects will be dynamic objects

//////////////////////////////////  RC_Object   //////////////////////////////////////////

/* Besides background scenery (walls, floor, roof and ceilings), the game experience is built up using objects.
 * They can be stationary or moving around. These objects are modeled by the RC_Object class.
 */

// ==============================/  class RC_Object   /==============================

// used to be definition of object record
class RC_Object {

private:
    float x, y;             // position in the map
    float scale;            // 1.0f is 100%

    float vx, vy;           // velocity
    float fObjAngle_rad;
    float fObjSpeed;

    float fDistToPlayer,
          fAngleToPlayer;  // w.r.t. player

    olc::Sprite *sprite = nullptr;

public:
    RC_Object();
    RC_Object( float fX, float fY, float fS, float fD, float fA, olc::Sprite *pS );

    void SetX( float fX );
    void SetY( float fY );

    float GetX();
    float GetY();

    void SetPos( float fX, float fY );

    void SetScale(            float fS );
    void SetDistToPlayer(     float fD );
    void SetAngleToPlayer(    float fA );

    float GetScale();
    float GetDistToPlayer();
    float GetAngleToPlayer();

    void SetSprite( olc::Sprite *pS );
    olc::Sprite *GetSprite();

    void SetVX( float fVX );
    void SetVY( float fVY );

    float GetVX();
    float GetVY();
    float GetAngle();   // in radians!!
    float GetSpeed();

    void Update( RC_Map &cMap, float fElapsedTime );

    void Print();

    // work out distance and angle between object and player, and
    // store it in the object itself
    void PrepareRender( float fPx, float fPy, float fPa_deg );
    void Render( RC_DepthDrawer &ddrwr, float fPh, float fFOV_rad, float fMaxDist, int nHorHeight );

public:
    bool bStationary = true;
    bool bAnimated   = false;   // future use

private:
    void UpdateObjAngle();
    void UpdateObjSpeed();
};

#endif // RC_OBJECT_H
