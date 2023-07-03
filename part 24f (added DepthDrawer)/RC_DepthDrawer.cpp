#include "RC_DepthDrawer.h"

#include <cfloat>

// ==============================/  class RC_DepthDrawer   /==============================

RC_DepthDrawer::RC_DepthDrawer() {}
RC_DepthDrawer::~RC_DepthDrawer() {
    delete fDepthBuffer;
}

void RC_DepthDrawer::Init( olc::PixelGameEngine *gfx ) {
    // store pointer to pge to enable calling it's (render) functions
    pgePtr = gfx;
    // Initialize depth buffer
    fDepthBuffer = new float[ gfx->ScreenWidth() * gfx->ScreenHeight() ];
}

int RC_DepthDrawer::ScreenWidth() {  return pgePtr->ScreenWidth();  }
int RC_DepthDrawer::ScreenHeight() { return pgePtr->ScreenHeight(); }

// Variant on Draw() that takes fDepth and the depth buffer into account.
// Pixel col is only drawn if fDepth is less than the depth buffer at that screen location (in which case the depth buffer is updated)
void RC_DepthDrawer::Draw( float fDepth, int x, int y, olc::Pixel col ) {
    // prevent out of bounds drawing
    if (x >= 0 && x < pgePtr->ScreenWidth() &&
        y >= 0 && y < pgePtr->ScreenHeight()) {

        if (fDepth <= fDepthBuffer[ y * pgePtr->ScreenWidth() + x ]) {
            fDepthBuffer[ y * pgePtr->ScreenWidth() + x ] = fDepth;
            pgePtr->Draw( x, y, col );
        }
    }
}

// sets all pixels of the depth buffer to absolute max depth value
void RC_DepthDrawer::Reset() {
    for (int i = 0; i < pgePtr->ScreenHeight() * pgePtr->ScreenWidth(); i++) {
        fDepthBuffer[ i ] = FLT_MAX;
    }
}
