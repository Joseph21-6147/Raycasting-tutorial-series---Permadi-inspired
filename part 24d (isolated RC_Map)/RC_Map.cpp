#include "RC_Map.h"

// =========/  functions/methods for class RC_Map  /==============================

RC_Map::RC_Map() {}

RC_Map::~RC_Map() {}

// First initialize the map calling this method ...
void RC_Map::InitMap( int nSizeX, int nSizeY ) {
    nMapX = nSizeX;
    nMapY = nSizeY;
}

// ... then add at least 1 layer to it using this method
void RC_Map::AddLayer( const std::string &sUserMap, std::vector<olc::Sprite *> vWallTextures,
                                            std::vector<olc::Sprite *> vCeilTextures,
                                            std::vector<olc::Sprite *> vRoofTextures ) {

    if (nMapX * nMapY != (int)sUserMap.length()) {
        std::cout << "ERROR: InitMap() -->  mismatch between map dimensions and length of map string - map width = "
                  << nMapX << ", map height = " << nMapY << ", string length = " << (int)sUserMap.length() << std::endl;
    }

    // prepare a container of blocks for this additional level, as well as a single block pointer
    std::vector<RC_MapCell *> vLevelMapCells;
    RC_MapCell *pMapCellPtr = nullptr;

    int nCntRefTransp = 0;
    int nCntFceTransp = 0;

    for (int y = 0; y < nMapY; y++) {
        for (int x = 0; x < nMapX; x++) {

            // grab the character from the input map, and use it to obtain the block info from the blueprint library
            char cTileID = sUserMap[ y * nMapX + x ];
            MapCellBluePrint &refMapCell = GetMapCellBluePrint( cTileID );

            // create a new block for this map cell...
            pMapCellPtr = new RC_MapCell;
            pMapCellPtr->Init( x, y, (int)bMaps.size());
            // ... and put the basic info into the new block
            pMapCellPtr->SetID(          refMapCell.cID          );
            pMapCellPtr->SetHeight(      refMapCell.fHeight      );
            pMapCellPtr->SetPermeable(   refMapCell.bPermeable   );

            if (cTileID == '.') {
                // if this is an empty block, we set it to empty and are finished
                pMapCellPtr->SetEmpty( true );
            } else {
                // if this block is not empty, we need to fill all the faces for it
                pMapCellPtr->SetEmpty( false );

                // having the block blueprint, create the faces for this (not empty) block
                for (int i = 0; i < FACE_NR_OF; i++) {
                    // use the index from the refMapCell to grab a reference to the face blueprint for this face index (i)
                    int nFaceBPIx = refMapCell.nFaces[i];
                    FaceBluePrint &refFace = vFaceBluePrintLib[ nFaceBPIx ];

                    nCntRefTransp += (refFace.bTransparent ? 1 : 0);

                    // prepare a new sprite pointer value
                    olc::Sprite *auxSpritePtr = nullptr;
                    switch ( refFace.nFaceType ) {
                        case TYPE_FACE_WALL: auxSpritePtr = vWallTextures[ refFace.nFaceIndex ]; break;
                        case TYPE_FACE_CEIL: auxSpritePtr = vCeilTextures[ refFace.nFaceIndex ]; break;
                        case TYPE_FACE_ROOF: auxSpritePtr = vRoofTextures[ refFace.nFaceIndex ]; break;
                        default: std::cout << "ERROR: AddLayer() --> face type unknown: " << refFace.nFaceType << std::endl;
                    }
                    if (refFace.bAnimated) {
                        RC_FaceAnimated *auxPtr = new RC_FaceAnimated;
                        auxPtr->Init( i, auxSpritePtr, refFace.bTransparent, ANIM_STATE_CLOSED, 32, 32 );
                        pMapCellPtr->SetFacePtr( i, auxPtr );
                    } else {
                        RC_Face *pFacePtr = new RC_Face;
                        pFacePtr->Init( i, auxSpritePtr, refFace.bTransparent );
                        pMapCellPtr->SetFacePtr( i, pFacePtr );
                    }

                    nCntFceTransp += (pMapCellPtr->GetFacePtr( i )->IsTransparent() ? 1 : 0);
                }
            }
            // having set up the block node, add it to this level
            vLevelMapCells.push_back( pMapCellPtr );
        }
    }

    std::cout << "MAP CREATION - ";
    std::cout <<  "# transparent refs: " << nCntRefTransp;
    std::cout << " #transparent faces: " << nCntFceTransp << std::endl;

    // add the level to the map
    bMaps.push_back( vLevelMapCells );
}

// method to clean up the object before it gets out of scope
void RC_Map::FinalizeMap() {

    for (auto &elt : bMaps ) {
        elt.clear();
    }

    bMaps.clear();
}

// getters for map width and height
int RC_Map::Width() { return nMapX; }
int RC_Map::Hight() { return nMapY; }

// returns whether (x, y) is within map boundaries
bool RC_Map::IsInBounds( float x, float y ) {
    return (x >= 0 && x < nMapX && y >= 0 && y < nMapY);
}

// getter for (cumulated) cell height at coordinates (x, y)
// Note - there's no intuitive meaning for this method in maps with holes
float RC_Map::CellHeight( int x, int y ) {
    float result = -1.0f;
    if (!IsInBounds( x, y )) {
        std::cout << "ERROR: CellHeight() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
    } else {
        result = 0.0f;
        for (int i = 0; i < (int)bMaps.size(); i++) {
            result += bMaps[i][y * nMapX + x]->GetHeight();
        }
    }
    return result;
}

// getter for obtaining height value of the cell at level, coordinates (x, y)
float RC_Map::CellHeightAt( int x, int y, int level ) {
    float result = -1.0f;
    if (!IsInBounds( x, y )) {
        std::cout << "ERROR: CellHeightAt() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
    } else if (level >= (int)bMaps.size()) {
        std::cout << "ERROR: CellHeightAt() --> level argument out of range: " << level << std::endl;
    } else {
        result = bMaps[level][y * nMapX + x]->GetHeight();
    }
    return result;
}

// getter for obtaining the character value of the cell at level, coordinates (x, y)
char RC_Map::CellValueAt( int x, int y, int level ) {
    char result = ' ';
    if (!IsInBounds( x, y )) {
        std::cout << "ERROR: CellValueAt() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
    } else if (level >= (int)bMaps.size()) {
        std::cout << "ERROR: CellValueAt() --> level argument out of range: " << level << std::endl;
    } else {
        result = bMaps[level][y * nMapX + x]->GetID();
    }
    return result;
}

// getter for obtaining the pointer to the associated block (can return nullptr) of the cell at level, coordinates (x, y)
RC_MapCell *RC_Map::MapCellPtrAt( int x, int y, int level ) {
    RC_MapCell *result = nullptr;
    if (!IsInBounds( x, y )) {
        std::cout << "ERROR: MapCellPtrAt() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
    } else if (level >= (int)bMaps.size()) {
        std::cout << "ERROR: MapCellPtrAt() --> level argument out of range: " << level << std::endl;
    } else {
        result = bMaps[ level ][y * nMapX + x];
        // build in error check here, so you don't have to check afterwards
        if (result == nullptr) {
            std::cout << "FATAL: MapCellPtrAt() --> nullptr result at: ("  << x << ", " << y << "),  level: " << level << std::endl;
        }
    }
    return result;
}

// returns the diagonal length of the map - useful for setting max distance value
float RC_Map::DiagonalLength() {
    return sqrt( nMapX * nMapX + nMapY * nMapY );
}

// returns current number of layers in this map object - use fMaps as representative model
int RC_Map::NrOfLayers() {
    return (int)bMaps.size();
}

// collision detection on the map:
// Note that int( fH ) denotes level to check, and (fH - int( fH )) denotes height to check within that level
// and fR is the radius of the object (considered as a pillar shape)
bool RC_Map::Collides( float fX, float fY, float fH, float fR, float fVX, float fVY ) {

    bool bResult;

    float fOffsetX;
    float fOffsetY;
    if (fVX == 0.0f) { fOffsetX = 0.0f; } else { fOffsetX = (fVX < 0.0f ? -fR : fR); }
    if (fVY == 0.0f) { fOffsetY = 0.0f; } else { fOffsetY = (fVY < 0.0f ? -fR : fR); }

    if (!IsInBounds( fX + fOffsetX, fY + fOffsetY ) || (fH - fR) < 0.0f) {
        bResult = true;
    } else if (fH > NrOfLayers()) {
        bResult = false;
    } else  {
        bResult = (
            CellHeightAt( int( fX + fOffsetX ), int( fY + fOffsetY ), int( fH )) >= (fH - int( fH )) &&
            !MapCellPtrAt( int( fX + fOffsetX ), int( fY + fOffsetY ), int( fH ))->IsPermeable()
        );
    }
    return bResult;
}

// ==============================/  end of file   /==============================

