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

// ==============================/  end of file   /==============================
