#ifndef RC_MISC_H
#define RC_MISC_H


#define PI 3.1415926535f

// this constant controls the significance of the trig lookup functions
#define SIG_POW10 100      // float angle is rounded at two decimal points (use 1000 for three, etc)

// this constant controls the significance of the float_rand_between() function
#define F_SIGNIF  1000.0f

// ==============================/  Prototypes for convenience and look up trig functions  /==============================

// convenience conversion functions
float deg2rad( float fAngleDeg );
float rad2deg( float fAngleRad );

// convenience modulo functions
// the offset parameter can be used to get a shifted modulo window, for instance
// to get an angle in [- PI, + PI)
float mod360( float fAngleDeg, float fOffsetDeg = 0.0f );
float mod2pi( float fAngleRad, float fOffsetRad = 0.0f );

// look up sine and cosine functions: init and call
void init_lu_sin_array();
void init_lu_cos_array();

float lu_sin( float fDegreeAngle );
float lu_cos( float fDegreeAngle );

// convenience functions for random range integers and floats
int     int_rand_between( int   nLow, int   nHgh );    // returns a random integer in the range [ nLow, nHgh ]
float float_rand_between( float fLow, float fHgh );    // returns a random float   in the range [ fLow, fHgh ]

// ==============================/  end of file   /==============================

#endif // RC_MISC_H
