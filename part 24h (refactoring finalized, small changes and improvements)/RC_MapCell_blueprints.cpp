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

    { '.', 0.00f,  0,  0,  0,  0, 10, 20, false, false },   // char ID, height, indices into the lib of face blueprints
    { '#', 1.00f,  0,  0,  0,  0, 10, 20, false, false },
    { '%', 1.00f,  1,  1,  1,  1, 11, 21, false, false },
    { '!', 1.00f,  2,  2,  2,  2, 12, 22, false, false },
    { '@', 1.00f,  3,  3,  3,  3, 13, 23, false, false },
    { '$', 1.00f,  0,  4,  0,  4, 15, 25, false, false },   // door / gate (on North and South face)
    { '&', 1.00f,  5,  5,  5,  5, 15, 25, false, false },
    { '*', 1.00f,  6,  6,  6,  6, 10, 20, false, false },   // window
    { '+', 1.00f,  7,  7,  7,  7, 10, 20, false, false },   // barred window
    { 'Q', 0.25f,  0,  0,  0,  0, 10, 20, false, false },
    { 'H', 0.50f,  0,  0,  0,  0, 10, 20, false, false },
    { 'T', 0.75f,  0,  0,  0,  0, 10, 20, false, false },
    { '1', 0.10f,  0,  0,  0,  0, 10, 20, false, false },
    { '2', 0.20f,  0,  0,  0,  0, 10, 20, false, false },
    { '3', 0.30f,  0,  0,  0,  0, 10, 20, false, false },
    { '4', 0.40f,  0,  0,  0,  0, 10, 20, false, false },
    { '5', 0.50f,  0,  0,  0,  0, 10, 20, false, false },
    { '6', 0.60f,  0,  0,  0,  0, 10, 20, false, false },
    { '7', 0.70f,  0,  0,  0,  0, 10, 20, false, false },
    { '8', 0.80f,  0,  0,  0,  0, 10, 20, false, false },
    { '9', 0.90f,  0,  0,  0,  0, 10, 20, false, false },
};

// ==============================/  end of file   /==============================
