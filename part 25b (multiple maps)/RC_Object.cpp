#include "RC_Object.h"

//////////////////////////////////  RC_Object   //////////////////////////////////////////

/* Besides background scene, consisting of walls, floor, roof and ceilings, the game is built up using objects.
 * They can be stationary or moving around. These objects are modeled by the RC_Object class.
 */

// ==============================/  class RC_Object   /==============================

RC_Object::RC_Object() {}

RC_Object::RC_Object( float fX, float fY, float fS, float fD, float fA, olc::Sprite *pS ) {
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

void RC_Object::SetX( float fX ) { x = fX; }
void RC_Object::SetY( float fY ) { y = fY; }

float RC_Object::GetX() { return x; }
float RC_Object::GetY() { return y; }

void RC_Object::SetPos( float fX, float fY ) {
    x = fX;
    y = fY;
}

void RC_Object::SetScale(         float fS ) { scale          = fS; }
void RC_Object::SetDistToPlayer(  float fD ) { fDistToPlayer  = fD; }
void RC_Object::SetAngleToPlayer( float fA ) { fAngleToPlayer = fA; }

float RC_Object::GetScale() {         return scale         ; }
float RC_Object::GetDistToPlayer() {  return fDistToPlayer ; }
float RC_Object::GetAngleToPlayer() { return fAngleToPlayer; }

void RC_Object::SetSprite( olc::Sprite *pS ) { sprite = pS; }
olc::Sprite *RC_Object::GetSprite() { return sprite; }

void RC_Object::SetVX( float fVX ) { vx = fVX; UpdateObjAngle(); UpdateObjSpeed(); }
void RC_Object::SetVY( float fVY ) { vy = fVY; UpdateObjAngle(); UpdateObjSpeed(); }

float RC_Object::GetVX() { return vx; }
float RC_Object::GetVY() { return vy; }
float RC_Object::GetAngle() { return fObjAngle_rad; }
float RC_Object::GetSpeed() { return fObjSpeed; }

void RC_Object::Update( RC_Map &cMap, float fElapsedTime ) {
    if (!bStationary) {
        float newX = x + vx * fElapsedTime;
        float newY = y + vy * fElapsedTime;
        if (!cMap.Collides( newX, y, RADIUS_ELF, RADIUS_ELF, vx, vy )) {
            x = newX;
        } else {
            vx = -vx;
            UpdateObjAngle();
            UpdateObjSpeed();
        }
        if (!cMap.Collides( x, newY, RADIUS_ELF, RADIUS_ELF, vx, vy )) {
            y = newY;
        } else {
            vy = -vy;
            UpdateObjAngle();
            UpdateObjSpeed();
        }
    }
}

void RC_Object::Print() {
    std::cout << "object @ pos: (" <<  x << ", " <<  y << "), ";
    std::cout << "vel: (" << vx << ", " << vy << "), ";
    std::cout << (bStationary ? "STATIONARY " : "DYNAMIC ");
    std::cout << std::endl;
}

// work out distance and angle between object and player, and
// store it in the object itself
void RC_Object::PrepareRender( float fPx, float fPy, float fPa_deg ) {
    // can object be seen?
    float fVecX = GetX() - fPx;
    float fVecY = GetY() - fPy;
    SetDistToPlayer( sqrtf( fVecX * fVecX + fVecY * fVecY ));
    // calculate angle between vector from player to object, and players looking direction
    // to determine if object is in players field of view
    float fEyeX = lu_cos( fPa_deg );
    float fEyeY = lu_sin( fPa_deg );
    float fObjA_rad = atan2f( fVecY, fVecX ) - atan2f( fEyeY, fEyeX );
    // "bodge" angle into range [ -PI, PI ]
    fObjA_rad = mod2pi( fObjA_rad, - PI );
    SetAngleToPlayer( fObjA_rad );
}

void RC_Object::Render( RC_DepthDrawer &ddrwr, float fPh, float fFOV_rad, float fMaxDist, int nHorHeight ) {
    // determine whether object is in field of view (a bit larger to prevent objects being not rendered at
    // screen boundaries)
    float fObjDist = GetDistToPlayer();
    float fObjA_rad = GetAngleToPlayer();
    bool bInFOV = fabs( fObjA_rad ) < fFOV_rad / 1.2f;

    // render object only when within Field of View, and within visible distance.
    // the check on proximity is to prevent asymptotic errors when distance to player becomes very small
    if (bInFOV && fObjDist >= 0.3f && fObjDist < fMaxDist) {

        // determine the difference between standard player height (i.e. 0.5f = standing on the floor)
        // and current player height
        float fCompensatePlayerHeight = fPh - 0.5f;
        // get the projected (halve) slice height of this object
        float fObjHalveSliceHeight     = float( ddrwr.ScreenHeight()  / fObjDist);
        float fObjHalveSliceHeightScld = float((ddrwr.ScreenHeight()) / fObjDist) * GetScale();

        // work out where objects floor and ceiling are (in screen space)
        // due to scaling factor, differentiated a normalized (scale = 1.0f) ceiling and a scaled variant
        float fObjCeilingNormalized = float(nHorHeight) - fObjHalveSliceHeight;
        float fObjCeilingScaled     = float(nHorHeight) - fObjHalveSliceHeightScld;
        // and adapt all the scaling into the ceiling value
        float fScalingDifference = fObjCeilingNormalized - fObjCeilingScaled;
        float fObjCeiling = fObjCeilingNormalized - 2 * fScalingDifference;
        float fObjFloor   = float(nHorHeight) + fObjHalveSliceHeight;

        // compensate object projection heights for elevation of the player
        fObjCeiling += fCompensatePlayerHeight * fObjHalveSliceHeight * 2.0f;
        fObjFloor   += fCompensatePlayerHeight * fObjHalveSliceHeight * 2.0f;

        // get height, aspect ratio and width
        float fObjHeight = fObjFloor - fObjCeiling;
        float fObjAR = float( GetSprite()->height ) / float( GetSprite()->width );
        float fObjWidth  = fObjHeight / fObjAR;
        // work out where the object is across the screen width
        float fMidOfObj = (0.5f * (fObjA_rad / (fFOV_rad / 2.0f)) + 0.5f) * float( ddrwr.ScreenWidth());

        // render the sprite
        for (float fx = 0.0f; fx < fObjWidth; fx++) {
            // get distance across the screen to render
            int nObjColumn = int( fMidOfObj + fx - (fObjWidth / 2.0f));
            // only render this column if it's on the screen
            if (nObjColumn >= 0 && nObjColumn < ddrwr.ScreenWidth()) {
                for (float fy = 0.0f; fy < fObjHeight; fy++) {
                    // calculate sample coordinates as a percentage of object width and height
                    float fSampleX = fx / fObjWidth;
                    float fSampleY = fy / fObjHeight;
                    // sample the pixel and draw it
//                        olc::Pixel objSample = ShadePixel( GetSprite()->Sample( fSampleX, fSampleY ), fObjDist );
                    olc::Pixel objSample = GetSprite()->Sample( fSampleX, fSampleY );
                    if (objSample != olc::BLANK) {
                        ddrwr.Draw( fObjDist, nObjColumn, fObjCeiling + fy, objSample );
                    }
                }
            }
        }
    }
}

void RC_Object::UpdateObjAngle() { fObjAngle_rad = mod2pi( atan2f( vy, vx )); }
void RC_Object::UpdateObjSpeed() { fObjSpeed = sqrt( vx * vx + vy * vy ); }

// ==============================/  end of file   /==============================
