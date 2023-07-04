#include "RC_MapCell.h"


//////////////////////////////////  MAP CELL BLUEPRINTS  //////////////////////////////////////

// ==============================/  functions for MapCellBluePrint  /==============================

// The library of map cells is modeled as a std::map, for fast (O(n log n)) searching
std::map<char, MapCellBluePrint> mMapCellBluePrintLib;

// Convenience function to add one map cell configuration
void AddMapCellBluePrint( MapCellBluePrint &rMBP ) {
    mMapCellBluePrintLib.insert( std::make_pair( rMBP.cID, rMBP ));
}

// Uses the data from vInitMapCellBluePrints to populate the library of map cells (mMapCellBluePrintLib)
// This construction decouples the blue print definition from its use, and enables error checking
// on the blue print data
void InitMapCellBluePrints() {
    for (auto elt : vInitMapCellBluePrints) {
        AddMapCellBluePrint( elt );
    }
}

// return a reference to the block in the library having id cID
MapCellBluePrint &GetMapCellBluePrint( char cID ) {
    std::map<char, MapCellBluePrint>::iterator itMapCellBP = mMapCellBluePrintLib.find( cID );
    if (itMapCellBP == mMapCellBluePrintLib.end()) {
        std::cout << "ERROR: GetMapCellBluePrint() --> can't find element with ID: " << cID << std::endl;
    }
    return (*itMapCellBP).second;
}

// ==============================/  class RC_MapCell  /==============================

RC_MapCell::RC_MapCell() {}
RC_MapCell::~RC_MapCell() {}

void RC_MapCell::Init( int px, int py, int l ) {
    x     = px;
    y     = py;
    layer = l;
}

int RC_MapCell::GetX() { return x; }
int RC_MapCell::GetY() { return y; }
int RC_MapCell::GetLayer() { return layer; }

void RC_MapCell::SetX( int px ) { x = px; }
void RC_MapCell::SetY( int py ) { y = py; }
void RC_MapCell::SetLayer( int nLayer ) { layer = nLayer; }

void RC_MapCell::Update( float fElapsedTime, bool &bPermFlag ) {
    if (!bEmpty) {
        for (int i = 0; i < FACE_NR_OF; i++) {
            pFaces[i]->Update( fElapsedTime, bPermFlag );
        }
    }
}

// if this is an empty map cell sampling will return olc::BLANK
olc::Pixel RC_MapCell::Sample( int nFaceIx, float sX, float sY ) {
    if (bEmpty) {
        return olc::BLANK;
    } else if (nFaceIx < 0 || nFaceIx >= FACE_NR_OF) {
        std::cout << "WARNING: RC_MapCell::Sample() --> face index out of range: " << nFaceIx << std::endl;
        return olc::MAGENTA;
    }

    return pFaces[nFaceIx]->Sample( sX, sY );
}

char RC_MapCell::GetID() { return id; }
void RC_MapCell::SetID( char cID ) { id = cID; }

float RC_MapCell::GetHeight() { return height; }
void  RC_MapCell::SetHeight( float fH ) { height = fH; }

bool RC_MapCell::IsEmpty() {     return bEmpty;       }
bool RC_MapCell::IsPermeable() { return bPermeable;   }

void RC_MapCell::SetEmpty( bool bParam ) { bEmpty       = bParam; }
void RC_MapCell::SetPermeable( bool bParam ) { bPermeable   = bParam; }

void RC_MapCell::SetFacePtr( int nFaceIx, RC_Face *pFace ) {
    if (nFaceIx < 0 || nFaceIx >= FACE_NR_OF) {
        std::cout << "WARNING: SetFacePtr() --> face index out of range: " << nFaceIx << std::endl;
    } else {
        pFaces[ nFaceIx ] = pFace;
    }
}

RC_Face *RC_MapCell::GetFacePtr( int nFaceIx ) {
    RC_Face *result = nullptr;
    if (nFaceIx < 0 || nFaceIx >= FACE_NR_OF) {
        std::cout << "WARNING: GetFacePtr() --> face index out of range: " << nFaceIx << std::endl;
    } else {
        result = pFaces[ nFaceIx ];
        if (result == nullptr) {
            std::cout << "FATAL: GetFacePtr() --> nullptr result for face index: " << nFaceIx << std::endl;
        }
    }
    return result;
}

bool RC_MapCell::IsDynamic() { return false; }
bool RC_MapCell::IsPortal() {  return false; }

// ==============================/  class RC_MapCellPortal  /==============================

RC_MapCellPortal::RC_MapCellPortal() {}

void RC_MapCellPortal::Init( int _nFromLevel, int _nFromX, int _nFromY, int _nToMap, int _nToLevel, int _nToX, int _nToY, int _nExitFace ) {
    x         = _nFromX;
    y         = _nFromY;
    layer     = _nFromLevel;
    nToMap    = _nToMap;
    nToLevel  = _nToLevel;
    nToX      = _nToX;
    nToY      = _nToY;
    nExitFace = _nExitFace;

    bEmpty = false;
    bPermeable = true;
}

void RC_MapCellPortal::Update( float fElapsedTime, bool &bPermFlag ) {
    // no behaviour yet
}

bool RC_MapCellPortal::IsDynamic() { return false; }
bool RC_MapCellPortal::IsPortal()  { return true;  }

int RC_MapCellPortal::GetToMap()   { return nToMap;    }
int RC_MapCellPortal::GetToLevel() { return nToLevel;  }
int RC_MapCellPortal::GetToX()     { return nToX;      }
int RC_MapCellPortal::GetToY()     { return nToY;      }
int RC_MapCellPortal::GetExitDir() { return nExitFace; }

#define SWAP_HYSTERESIS 0.01f

// the position denoted by fPh, fPx, fPy is in the portal cell if these values
// truncated are equal to the portal cell coordinates
bool RC_MapCellPortal::IsInPortal( float fPh, float fPx, float fPy ) {
    return int( fPh ) == layer &&
           int( fPx ) == x &&
           int( fPy ) == y;
}

bool RC_MapCellPortal::SwapToOtherSide( float fPh, float fPx, float fPy ) {
    bool result = false;
    if (IsInPortal( fPh, fPx, fPy )) {
        switch (nExitFace) {
            case FACE_EAST : result = (fPx >= (x +         SWAP_HYSTERESIS )); break;
            case FACE_SOUTH: result = (fPy >= (y +         SWAP_HYSTERESIS )); break;
            case FACE_WEST : result = (fPx <= (x + (1.0f - SWAP_HYSTERESIS))); break;
            case FACE_NORTH: result = (fPy <= (y + (1.0f - SWAP_HYSTERESIS))); break;
            default: std::cout << "ERROR: SwapToOtherSide() --> invalid portal direction: " << nExitFace << std::endl;
        }
    }
    return result;
}

// ==============================/  class RC_MapCellDynamic  /==============================


RC_MapCellDynamic::RC_MapCellDynamic() {}

// NOTE: contains hardcoded stuff currently!
void RC_MapCellDynamic::Init( int px, int py, int l ) {
    x     = px;
    y     = py;
    layer = l;

    fTimer    =   0.0f;   // local timer for this dynamic map cell
    fTickTime =   0.05f;  // tick every ... seconds
    nCounter  =   0;      // keep track of nr of ticks
    nNrSteps  = 101;      // cycle every ... ticks
}

// NOTE: contains hardcoded stuff currently!
void RC_MapCellDynamic::Update( float fElapsedTime, bool &bPermFlag ) {
    // first update all the faces of this block
    if (!bEmpty) {
        for (int i = 0; i < FACE_NR_OF; i++) {
            pFaces[i]->Update( fElapsedTime, bPermFlag );
        }
    }
    // then update the block itself
    fTimer += fElapsedTime;
    if (fTimer >= fTickTime) {
        // if the threshold is small, 1 frame could exceed the threshold multiple times
        while (fTimer >= fTickTime) {
            fTimer -= fTickTime;
            // one tick gone by, advance counter
            nCounter += 1;
        }
        if (nCounter >= nNrSteps) {
            // animation sequence reverses
            nCounter -= nNrSteps;
            bUp = !bUp;
        } else {
            height = (bUp ? float( nCounter ) / 100.0f : 1.0f - float( nCounter ) / 100.0f);
        }
    }
}

bool RC_MapCellDynamic::IsEmpty() { return bEmpty; }

bool RC_MapCellDynamic::IsDynamic() { return true; }
bool RC_MapCellDynamic::IsPortal() { return false; }

// ==============================/  end of file   /==============================
