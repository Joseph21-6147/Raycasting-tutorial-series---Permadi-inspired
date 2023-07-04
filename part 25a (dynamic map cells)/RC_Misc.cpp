#include <cmath>

#include "RC_Misc.h"

// =========/  convenience functions for angle conversions   /==============================

float deg2rad( float fAngleDeg ) { return fAngleDeg * PI / 180.0f; }
float rad2deg( float fAngleRad ) { return fAngleRad / PI * 180.0f; }

// generic float modulus function. The value of fValToMod will be brought with the range [ fOffset, fOffset + fDivisior )
float mod( float fValToMod, float fDivisor, float fOffset ) {
    while (fValToMod <  ( 0.0f     + fOffset)) fValToMod += fDivisor;
    while (fValToMod >= ( fDivisor + fOffset)) fValToMod -= fDivisor;
    return fValToMod;
}
float mod360( float fAngleDeg, float fOffsetDeg ) { return mod( fAngleDeg, 360.0f     , fOffsetDeg ); }
float mod2pi( float fAngleRad, float fOffsetRad ) { return mod( fAngleRad,   2.0f * PI, fOffsetRad ); }

// =========/  lookup sine and cosine functions  /==============================

// the look up tables
float lu_sin_array[360 * SIG_POW10];
float lu_cos_array[360 * SIG_POW10];

// call these to initialise the look up tables

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

// call these to index into the lookup tables

float lu_sin( float fDegreeAngle ) {
    fDegreeAngle = mod360( fDegreeAngle );
    int nWholeNr = int( fDegreeAngle );
    int nRemainder = int( (fDegreeAngle - nWholeNr) * float( SIG_POW10 ));
    int nIndex = nWholeNr * SIG_POW10 + nRemainder;
    return lu_sin_array[ nIndex ];
}

float lu_cos( float fDegreeAngle ) {
    fDegreeAngle = mod360( fDegreeAngle );
    int nWholeNr = int( fDegreeAngle );
    int nRemainder = int( (fDegreeAngle - nWholeNr) * float( SIG_POW10 ));
    int nIndex = nWholeNr * SIG_POW10 + nRemainder;
    return lu_cos_array[ nIndex ];
}

// ==========/  convenience functions for random range integers and floats  /==============================

// returns a random integer in the range [nLow, nHgh]
int int_rand_between( int nLow, int nHgh ) {
    return (rand() % (nHgh - nLow + 1)) + nLow;
}

// returns a random float in the range [ fLow, fHgh ]
float float_rand_between( float fLow, float fHgh ) {
    int nLow = int( F_SIGNIF * fLow );
    int nHgh = int( F_SIGNIF * fHgh );
    return float( int_rand_between( nLow, nHgh )) / F_SIGNIF;
}

// ==============================/  end of file   /==============================

