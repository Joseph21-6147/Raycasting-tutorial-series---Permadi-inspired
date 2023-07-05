#include "RC_Map.h"

// =========/  functions/methods for class RC_Map  /==============================

RC_Map::RC_Map() {}

RC_Map::~RC_Map() {}

// First initialize the map calling this method ...
void RC_Map::InitMap(
    int nID,
    std::vector<PortalDescriptor> &vPortDescs,
    olc::Sprite *floorTxtr,
    olc::Pixel skyCol
) {
/* Additional checks to build in:
 *   1. consistency of map ID with location in map vector
 */
    nMapID          = nID;
    vPDs            = vPortDescs;
    pFloorSpritePtr = floorTxtr;
    SkyColour       = skyCol;

    nMapX = nMapY = nMapZ = -1;
}

// ... then add at least 1 layer to it using this method
void RC_Map::AddLayer(
    std::vector<std::string> &sUserMap,
    std::vector<olc::Sprite *> &vWallTextures,
    std::vector<olc::Sprite *> &vCeilTextures,
    std::vector<olc::Sprite *> &vRoofTextures
) {
    // for the first layer (with index 0) these values will be set and can be used for error checking
    if (nMapX == -1) { nMapX = (sUserMap.empty() ? 0 : (int)sUserMap[0].length()); }
    if (nMapY == -1) { nMapY = (int)sUserMap.size(); }

    // grab layer nr to add
    int nCurLevel = (int)bMaps.size();
    // prepare a container of map cells for this additional layer, as well as an auxiliary map cell pointer
    std::vector<RC_MapCell *> vLevelMapCells;
    RC_MapCell *pMapCellPtr = nullptr;

    // check if map passed is empty
    if (sUserMap.empty()) {
        std::cout << "ERROR: AddLayer() --> user map is empty..." << std::endl;
    } else {
        // check if consistent on x dimension
        for (int i = 0; i < (int)sUserMap.size(); i++) {
            if ((int)sUserMap[i].length() != nMapX) {
                std::cout << "ERROR: AddLayer() --> mismatch in line " << i << " between nMapX = " << nMapX << " and dimensions of sUserMap = " << (int)sUserMap[i].length() << std::endl;
            }
        }
        // check if consistent on y dimension
        if ((int)sUserMap.size() != nMapY) {
            std::cout << "ERROR: AddLayer() --> mismatch between nMapY = " << nMapY << " and dimensions of sUserMap = " << (int)sUserMap.size() << std::endl;
        }
    }

    for (int y = 0; y < nMapY; y++) {
        for (int x = 0; x < nMapX; x++) {

            // get the character from the input map, and use it to obtain the map cell info from the blueprint library
            char cTileID = sUserMap[ y ][ x ];
            MapCellBluePrint &refMapCell = GetMapCellBluePrint( cTileID );

            // distinguish following cases: 1. empty map cell, 2. portal 3. dynamic 4. regular (textured)
            if (refMapCell.bEmpty) {
                // create a new map cell...
                pMapCellPtr = new RC_MapCell;
                pMapCellPtr->Init( x, y, nCurLevel );
                // ... and set it to empty
                pMapCellPtr->SetEmpty( true );
            } else {

                // map cell is not empty: do additional stuff for portal and dynamic map cells
                if (refMapCell.bPortal) {
                    // get a reference to the associated portal info
                    PortalDescriptor &PDref = GetPortalDescriptor( nCurLevel, x, y );
                    // create a portal map cell
                    RC_MapCellPortal *aux = new RC_MapCellPortal;
                    // initialize the portal map cell with the portal info from the reference
                    aux->Init( nCurLevel, x, y, PDref.nMapExit, PDref.nLevelExit, PDref.nTileExitX, PDref.nTileExitY, PDref.nExitFace );

                    pMapCellPtr = aux;
                } else if (refMapCell.bDynamic) {
                    // create a dynamic map cell
                    RC_MapCellDynamic *aux = new RC_MapCellDynamic;
                    // initialise the dynamic map cell
                    aux->Init( x, y, nCurLevel );

                    pMapCellPtr = aux;
                } else {
                    // if its not a portal cell, nor a dynamic cell, it's a regular (textured) map cell
                    pMapCellPtr = new RC_MapCell;
                    pMapCellPtr->Init( x, y, nCurLevel );
                }
                // either way it is not empty
                pMapCellPtr->SetEmpty( false );

                // since this block is not empty, we need to fill all the faces for it
                for (int i = 0; i < FACE_NR_OF; i++) {
                    // use the index from the refMapCell to grab a reference to the face blueprint for this face index (i)
                    int nFaceBPIx = refMapCell.nFaces[i];
                    FaceBluePrint &refFace = vFaceBluePrintLib[ nFaceBPIx ];

                    // prepare a new sprite pointer value
                    olc::Sprite *auxSpritePtr = nullptr;
                    switch ( refFace.nFaceType ) {
                        case TYPE_FACE_WALL: auxSpritePtr = vWallTextures[ refFace.nFaceIndex ]; break;
                        case TYPE_FACE_CEIL: auxSpritePtr = vCeilTextures[ refFace.nFaceIndex ]; break;
                        case TYPE_FACE_ROOF: auxSpritePtr = vRoofTextures[ refFace.nFaceIndex ]; break;
                        default: std::cout << "ERROR: AddLayer() --> face type unknown: " << refFace.nFaceType << std::endl;
                    }
                    // if this face is an animated type face, we need to create a different type RC_Face for it
                    if (refFace.bAnimated) {
                        RC_FaceAnimated *pFacePtr = new RC_FaceAnimated;
                        pFacePtr->Init( i, auxSpritePtr, refFace.bTransparent, ANIM_STATE_CLOSED, 32, 32 );
                        pMapCellPtr->SetFacePtr( i, pFacePtr );
                    } else {
                        RC_Face *pFacePtr = new RC_Face;
                        pFacePtr->Init( i, auxSpritePtr, refFace.bTransparent );
                        pMapCellPtr->SetFacePtr( i, pFacePtr );
                    }
                }
            }
            // Finally, put the basic info into the new map cell
            pMapCellPtr->SetID( refMapCell.cID );
            pMapCellPtr->SetHeight( refMapCell.fHeight );
            pMapCellPtr->SetPermeable( refMapCell.bPermeable );

            // Having set up the map cell, add it to this map layer
            vLevelMapCells.push_back( pMapCellPtr );
        }
    }

    // after assembly integrity check
    //   * check whether all map cell pointers have a value and a map cell object associated,
    //   * if the map cell object is not empty, check that there are 6 face objects associated with the map cell
    for (int y = 0; y < nMapY; y++) {
        for (int x = 0; x < nMapX; x++) {
            RC_MapCell *aux = vLevelMapCells[ y * nMapX + x ];
            if (aux == nullptr) {
                std::cout << "ERROR: AddLayer() --> nullptr map cell ptr encountered at ( " << x << ", " << y << ", " << nMapZ << ")" << std::endl;
            } else {
                if (aux->IsEmpty()) {
                    for (int i = 0; i < FACE_NR_OF; i++) {
                    	// map cell is empty: all face pointers should equal nullptr
                        if (aux->GetFacePtr_raw( i ) != nullptr) {
                            std::cout << "ERROR: AddLayer() --> empty map cell at ( " << x << ", " << y << ", " << nMapZ << ") contains non-nullptr face pointer for face " << i << std::endl;
                        }
                    }
            	} else {
                    for (int i = 0; i < FACE_NR_OF; i++) {
                    	// map cell is not empty: all face pointers should NOT equal nullptr
                        if (aux->GetFacePtr( i ) == nullptr) {
                            std::cout << "ERROR: AddLayer() --> nullptr face pointer encountered for face " << i << " at non-empty map cell ( " << x << ", " << y << ", " << nMapZ << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }

    // having set up the complete map layer, add the layer to the map
    bMaps.push_back( vLevelMapCells );
    nMapZ = (int)bMaps.size();
}

// method to clean up the object before it gets out of scope
void RC_Map::FinalizeMap() {

    for (auto &elt : bMaps ) {
        for (auto &elt2 : elt) {
            delete elt2;
        }
        elt.clear();
    }

    bMaps.clear();
}

// getters for map ID, width and height
int RC_Map::GetID() {     return nMapID; }
int RC_Map::GetWidth() {  return nMapX;  }
int RC_Map::GetHeight() { return nMapY;  }

// returns current number of layers in this map object - use fMaps as representative model
int RC_Map::NrOfLayers() {
    return (int)bMaps.size();
}

// returns the diagonal length of the map (2D) - useful for setting max distance value
float RC_Map::DiagonalLength() {
    return sqrt( nMapX * nMapX + nMapY * nMapY );
}

// returns the diagonal length of the map (3D)
float RC_Map::DiagonalLength3D() {
    return sqrt( nMapX * nMapX + nMapY * nMapY + nMapZ * nMapZ );
}

// returns whether (x, y) is within map boundaries
bool RC_Map::IsInBounds( float x, float y ) {
    return (x >= 0 && x < nMapX && y >= 0 && y < nMapY);
}

// returns whether (x, y, z) is within map boundaries
bool RC_Map::IsInBounds( float x, float y, float z ) {
    return IsInBounds( x, y ) && (z >= 0 && z < nMapZ);
}

// getter for (cumulated) cell height at coordinates (x, y)
// Note - there's no intuitive meaning for this method in maps with holes
float RC_Map::CellHeight( int x, int y ) {
    float result = 0.0f;
    if (!IsInBounds( x, y )) {
        std::cout << "ERROR: RC_Map::CellHeight() --> map indices out of bounds: (" << x << ", " << y << ")" << std::endl;
        result = -1.0f;
    } else {
        for (int i = 0; i < (int)bMaps.size(); i++) {
            result += bMaps[i][y * nMapX + x]->GetHeight();
        }
    }
    return result;
}

// getter for obtaining height value of the cell at layer, coordinates (x, y)
float RC_Map::CellHeightAt( int x, int y, int layer ) {
    float result = -1.0f;
    if (!IsInBounds( x, y, layer )) {
        std::cout << "ERROR: RC_Map::CellHeightAt() --> map indices out of bounds: (" << x << ", " << y << ", " << layer << ")" << std::endl;
    } else {
        result = bMaps[layer][y * nMapX + x]->GetHeight();
    }
    return result;
}

// getter for obtaining the character value of the cell at layer, coordinates (x, y)
char RC_Map::CellValueAt( int x, int y, int layer ) {
    char result = ' ';
    if (!IsInBounds( x, y, layer )) {
        std::cout << "ERROR: RC_Map::CellValueAt() --> map indices out of bounds: (" << x << ", " << y << ", " << layer << ")" << std::endl;
    } else {
        result = bMaps[layer][y * nMapX + x]->GetID();
    }
    return result;
}

// getter for obtaining the pointer to the associated block (can return nullptr) of the cell at layer, coordinates (x, y)
RC_MapCell *RC_Map::MapCellPtrAt( int x, int y, int layer ) {
    RC_MapCell *result = nullptr;
    if (!IsInBounds( x, y, layer )) {
        std::cout << "ERROR: RC_Map::MapCellPtrAt() --> map indices out of bounds: (" << x << ", " << y << ", " << layer << ")" << std::endl;
        std::cout << "Map ID = " << nMapID << ", size X = " << nMapX << ", size Y = " << nMapY << ", size Z = " << nMapZ << std::endl;
    } else {
        result = bMaps[ layer ][y * nMapX + x];
        // build in error check here, so you don't have to check afterwards
        if (result == nullptr) {
            std::cout << "FATAL: RC_Map::MapCellPtrAt() --> nullptr result at: ("  << x << ", " << y << "),  layer: " << layer << std::endl;
        }
    }
    return result;
}

// collision detection on the map:
// Note that int( fH ) denotes layer to check, and (fH - int( fH )) denotes height to check within that layer
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

void RC_Map::SetFloorSpritePtr( olc::Sprite *pSpritePtr ) {
    pFloorSpritePtr = pSpritePtr;
}

olc::Sprite *RC_Map::GetFloorSpritePtr() {
    return pFloorSpritePtr;
}

void RC_Map::SetSkyColour( olc::Pixel col ) {
    SkyColour = col;
}

olc::Pixel RC_Map::GetSkyColour() {
    return SkyColour;
}

// searches the portal descriptor for this RC_Map that is identified by the combination of (nL, nX, nY)
// returns a reference to it.
PortalDescriptor &RC_Map::GetPortalDescriptor( int nL, int nX, int nY ) {

    int nPDindex = -1;
    for (int i = 0; i < (int)vPDs.size() && (nPDindex == -1); i++) {
        PortalDescriptor &aux = vPDs[i];
        if (aux.nMapEntry != nMapID) {
            std::cout << "ERROR: RC_Map::GetPortalDescriptor() --> encountered mismatch between this map ID: " << nMapID << " and reference map id: " << aux.nMapEntry << std::endl;
        } else {
            if (aux.nLevelEntry == nL && aux.nTileEntryX == nX && aux.nTileEntryY == nY) {
                nPDindex = i;
            }
        }
    }

    // check on errors
    if (nPDindex == -1) {
        std::cout << "ERROR: RC_Map::GetPortalDescriptor() --> couldn't find descriptor for map ID: " << nMapID << ", layer: " << nL << ", tile: (" << nX << ", " << nY << "). ";
        std::cout << "using first available portal (index 0) " << std::endl;
        nPDindex = 0;
    }

    return vPDs[ nPDindex ];
}

// ==============================/  end of file   /==============================
