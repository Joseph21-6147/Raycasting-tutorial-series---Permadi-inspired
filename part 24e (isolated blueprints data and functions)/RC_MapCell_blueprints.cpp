#include "RC_MapCell.h"


//////////////////////////////////  MAP CELL BLUEPRINTS  //////////////////////////////////////

// this list contains the data to initialize the map cell blueprint library
std::vector<MapCellBluePrint> vInitMapCellBluePrints = {

    // +------------------------------------------------------- ID (char) of the map cell
    // |    +-------------------------------------------------- height of the map cell (should be in [0.0f, 1.0f])
    // |    |
    // |    |      +-------------------------------------------- index into face blue print lib for EAST
    // |    |      |   +----------------------------------------                                    NORTH
    // |    |      |   |   +------------------------------------                                    WEST
    // |    |      |   |   |   +--------------------------------                                    SOUTH
    // |    |      |   |   |   |   +----------------------------                                    TOP
    // |    |      |   |   |   |   |   +------------------------                                    BOTTOM
    // |    |      |   |   |   |   |   |
    // |    |      |   |   |   |   |   |    +------------------- flags whether map cell is permeable (user can pass it)
    // |    |      |   |   |   |   |   |    |
    // V    V      V   V   V   V   V   V    V

    { '.', 0.00f,  0,  0,  0,  0, 10, 20, false },   // char ID, height, indices into the lib of face blueprints
    { '#', 1.00f,  0,  0,  0,  0, 10, 20, false },
    { '%', 1.00f,  1,  1,  1,  1, 11, 21, false },
    { '!', 1.00f,  2,  2,  2,  2, 12, 22, false },
    { '@', 1.00f,  3,  3,  3,  3, 13, 23, false },
    { '$', 1.00f,  0,  4,  0,  4, 15, 25, false },   // door / gate (on North and South face)
    { '&', 1.00f,  5,  5,  5,  5, 15, 25, false },
    { '*', 1.00f,  6,  6,  6,  6, 10, 20, false },   // window
    { '+', 1.00f,  7,  7,  7,  7, 10, 20, false },   // barred window
    { 'Q', 0.25f,  0,  0,  0,  0, 10, 20, false },
    { 'H', 0.50f,  0,  0,  0,  0, 10, 20, false },
    { 'T', 0.75f,  0,  0,  0,  0, 10, 20, false },
    { '1', 0.10f,  0,  0,  0,  0, 10, 20, false },
    { '2', 0.20f,  0,  0,  0,  0, 10, 20, false },
    { '3', 0.30f,  0,  0,  0,  0, 10, 20, false },
    { '4', 0.40f,  0,  0,  0,  0, 10, 20, false },
    { '5', 0.50f,  0,  0,  0,  0, 10, 20, false },
    { '6', 0.60f,  0,  0,  0,  0, 10, 20, false },
    { '7', 0.70f,  0,  0,  0,  0, 10, 20, false },
    { '8', 0.80f,  0,  0,  0,  0, 10, 20, false },
    { '9', 0.90f,  0,  0,  0,  0, 10, 20, false },
};

// ==============================/  functions for MapCellBluePrint  /==============================

// The library of map cells is modeled as a std::map, for fast (O(n log n)) searching
std::map<char, MapCellBluePrint> mMapCellBluePrintLib;

// Convenience function to add one block configuration
void AddMapCellBluePrint( char cID, float fH, int nE, int nN, int nW, int nS, int nT, int nB, bool bP ) {
    MapCellBluePrint sTmpMapCellBP = { cID, fH, { nE, nN, nW, nS, nT, nB }, bP };
    mMapCellBluePrintLib.insert( std::make_pair( cID, sTmpMapCellBP ));
}

// Put all block configs you need into this function. Here the relation between the identifying character
// and the sprites per face are made, as well as the height of the block.
void InitMapCellBluePrints() {
    for (auto &elt : vInitMapCellBluePrints) {
        AddMapCellBluePrint(
            elt.cID,
            elt.fHeight,
            elt.nFaces[0],
            elt.nFaces[1],
            elt.nFaces[2],
            elt.nFaces[3],
            elt.nFaces[4],
            elt.nFaces[5],
            elt.bPermeable
        );
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

// ==============================/  end of file   /==============================
