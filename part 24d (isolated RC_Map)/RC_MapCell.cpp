#include "RC_MapCell.h"

std::map<char, MapCellBluePrint> mMapCellBluePrintLib;

// Convenience function to add one block configuration
void AddMapCellBluePrint( char cID, float fH, int nE, int nN, int nW, int nS, int nT, int nB, bool bP ) {
    MapCellBluePrint sTmpMapCellBP = { cID, fH, { nE, nN, nW, nS, nT, nB }, bP };
    mMapCellBluePrintLib.insert( std::make_pair( cID, sTmpMapCellBP ));
}

// Put all block configs you need into this function. Here the relation between the identifying character
// and the sprites per face are made, as well as the height of the block.
void InitMapCellBluePrints() {
  AddMapCellBluePrint( '.', 0.00f, 0, 0, 0, 0, 10, 20 );   // char ID, height, indices into the lib of face blueprints
  AddMapCellBluePrint( '#', 1.00f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '%', 1.00f, 1, 1, 1, 1, 11, 21 );
  AddMapCellBluePrint( '!', 1.00f, 2, 2, 2, 2, 12, 22 );
  AddMapCellBluePrint( '@', 1.00f, 3, 3, 3, 3, 13, 23 );
  AddMapCellBluePrint( '$', 1.00f, 0, 4, 0, 4, 15, 25 );   // door / gate (on North and South face)
  AddMapCellBluePrint( '&', 1.00f, 5, 5, 5, 5, 15, 25 );
  AddMapCellBluePrint( '*', 1.00f, 6, 6, 6, 6, 10, 20 );   // window
  AddMapCellBluePrint( '+', 1.00f, 7, 7, 7, 7, 10, 20 );   // barred window
  AddMapCellBluePrint( 'Q', 0.25f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( 'H', 0.50f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( 'T', 0.75f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '1', 0.10f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '2', 0.20f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '3', 0.30f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '4', 0.40f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '5', 0.50f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '6', 0.60f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '7', 0.70f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '8', 0.80f, 0, 0, 0, 0, 10, 20 );
  AddMapCellBluePrint( '9', 0.90f, 0, 0, 0, 0, 10, 20 );
}

// return a reference to the block in the library having id cID
MapCellBluePrint &GetMapCellBluePrint( char cID ) {
    std::map<char, MapCellBluePrint>::iterator itMapCellBP = mMapCellBluePrintLib.find( cID );
    if (itMapCellBP == mMapCellBluePrintLib.end()) {
        std::cout << "ERROR: GetMapCellBluePrint() --> can't find element with ID: " << cID << std::endl;
    }
    return (*itMapCellBP).second;
}

// getters for the members of the MapCellBluePrint
char  GetMapCellBPID(     MapCellBluePrint &b ) { return b.cID; }
float GetMapCellBPHeight( MapCellBluePrint &b ) { return b.fHeight; }
int   GetMapCellBPFaceIx( MapCellBluePrint &b, int nFace ) {
    int nIndex = FACE_UNKNOWN;
    if (nFace < 0 || nFace >= (int)vFaceBluePrintLib.size()) {
        std::cout << "GetMapCellBPFaceIx() --> index out of range: " << nFace << " (should be between 0 and " << (int)vFaceBluePrintLib.size() << std::endl;
    } else {
        nIndex = b.nFaces[ nFace ];
    }
    return nIndex;
}


// ==============================/  class RC_MapCell  /==============================

RC_MapCell::RC_MapCell() {}
RC_MapCell::~RC_MapCell() {}

void RC_MapCell::Init( int px, int py, int l ) {
    x = px;
    y = py;
    level = l;
}

void RC_MapCell::SetX( int px ) { x = px; }
void RC_MapCell::SetY( int py ) { y = py; }
void RC_MapCell::SetLevel( int nLevel ) { level = nLevel; }

int RC_MapCell::GetX() { return x; }
int RC_MapCell::GetY() { return y; }
int RC_MapCell::GetLevel() { return level; }

void RC_MapCell::Update( float fElapsedTime, bool &bPermFlag ) {
    if (!bEmpty) {
        for (int i = 0; i < FACE_NR_OF; i++) {
            pFaces[i]->Update( fElapsedTime, bPermFlag );
        }
    }
}

// if not overriden, this is an empty block and sampling always returns olc::BLANK
olc::Pixel RC_MapCell::Sample( int nFaceIx, float sX, float sY ) {
    if (bEmpty) {
        return olc::BLANK;
    } else if (nFaceIx < 0 || nFaceIx >= FACE_NR_OF) {
        std::cout << "WARNING: Sample() --> face index out of range: " << nFaceIx << std::endl;
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
