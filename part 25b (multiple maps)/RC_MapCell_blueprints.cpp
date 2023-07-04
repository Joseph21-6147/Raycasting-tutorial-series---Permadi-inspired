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
    // |    |      |   |   |   |   |   |    |      +------------                           dynamic
    // |    |      |   |   |   |   |   |    |      |      +-----                           empty
    // |    |      |   |   |   |   |   |    |      |      |
    // V    V      V   V   V   V   V   V    V      V      V

    { '.', 0.00f,  0,  0,  0,  0, 10, 20, false, false, true  },   // char ID, height, indices into the lib of face blueprints
    { '#', 1.00f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '%', 1.00f,  1,  1,  1,  1, 11, 21, false, false, false },
    { '!', 1.00f,  2,  2,  2,  2, 12, 22, false, false, false },
    { '@', 1.00f,  3,  3,  3,  3, 13, 23, false, false, false },
    { '$', 1.00f,  0,  4,  0,  4, 15, 25, false, false, false },   // door / gate (on North and South face)
    { '&', 1.00f,  5,  5,  5,  5, 15, 25, false, false, false },
    { '*', 1.00f,  6,  6,  6,  6, 10, 20, false, false, false },   // window
    { '+', 1.00f,  7,  7,  7,  7, 10, 20, false, false, false },   // barred window
    { 'x', 1.00f,  0,  0,  0,  0, 10, 20, false, true , false },   // dynamic map cell example
    { 'Q', 0.25f,  0,  0,  0,  0, 10, 20, false, false, false },
    { 'H', 0.50f,  0,  0,  0,  0, 10, 20, false, false, false },
    { 'T', 0.75f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '1', 0.10f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '2', 0.20f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '3', 0.30f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '4', 0.40f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '5', 0.50f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '6', 0.60f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '7', 0.70f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '8', 0.80f,  0,  0,  0,  0, 10, 20, false, false, false },
    { '9', 0.90f,  0,  0,  0,  0, 10, 20, false, false, false },
};

// ==============================/  end of file   /==============================
