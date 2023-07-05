#ifndef RC_DEPTHDRAWER_H
#define RC_DEPTHDRAWER_H

#include "olcPixelGameEngine.h"

//////////////////////////////////  RC_DepthDrawer   //////////////////////////////////////////

/* I need a uniform way to draw to screen using the PGE, and incorporating a shared depth buffer (2D).
 * This class implements that functionality.
 */

// ==============================/  class RC_DepthDrawer   /==============================

class RC_DepthDrawer {
private:
    // the 2D depth buffer
    float *fDepthBuffer = nullptr;
    olc::PixelGameEngine *pgePtr = nullptr;

public:
    RC_DepthDrawer();

    ~RC_DepthDrawer();

    void Init( olc::PixelGameEngine *gfx );

    int ScreenWidth();
    int ScreenHeight();

    // Variant on Draw() that takes fDepth and the depth buffer into account.
    // Pixel col is only drawn if fDepth is less than the depth buffer at that screen location (in which case the depth buffer is updated)
    void Draw( float fDepth, int x, int y, olc::Pixel col );

    // sets all pixels of the depth buffer to absolute max depth value
    void Reset();
    void Reset( int nSlice, int nLowY, int nHghY );

    bool IsMasked( int x, int y, float fDepth );
};


#endif // RC_DEPTHDRAWER_H
