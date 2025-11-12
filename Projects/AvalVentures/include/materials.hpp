#pragma once
#include "../../Engine/ext/json/json.hpp"
using json = nlohmann::ordered_json;
// Units:
// yield: MPa (megapascals) - when material permanently deforms
// break: MPa - when material fractures/tears
// hardness: Mohs scale (1-10) for minerals, or Vickers (HV) for metals
// density: g/cm³
// thick: mm (for sheet materials)
// elastic: dimensionless ratio (0-1)
const std::string IROOT = "../Projects/AvalVentures/assets/images/";

// BIOLOGICAL - HARD
json mat_tooth_enamel = {
    {"name", "enamel"},
    {"yield", 10},        // MPa - actually yields ~10 MPa
    {"break", 15},        // MPa - fractures ~10-30 MPa range
    {"hardness", 5},      // Mohs scale
    {"density", 2.95},    // g/cm³ - real value ~2.95
    {"vickers", 343}      // HV - optional, real hardness
};

json mat_bone = {
    {"name", "bone"},
    {"yield", 104},       // MPa - cortical bone yields ~104-121 MPa
    {"break", 130},       // MPa - fractures ~130-150 MPa
    {"hardness", 3.5},    // Mohs
    {"density", 1.85},    // g/cm³
    {"vickers", 40}       // HV
};

json mat_rodent_tooth = {
    {"name", "rodent_tooth"},
    {"yield", 12},        // MPa - slightly harder than human
    {"break", 18},        // MPa
    {"hardness", 5.5},    // Mohs
    {"density", 2.95}     // g/cm³
};

// BIOLOGICAL - SOFT
json mat_skin_human = {
    {"name", "skin"},
    {"yield", 1.0},       // MPa - skin yields ~0.5-2 MPa
    {"break", 20},        // MPa - tensile strength ~15-30 MPa
    {"thick", 1.5},       // mm - average ~1-2mm
    {"density", 1.09},    // g/cm³
    {"elastic", 0.6}      // ~60% elastic recovery
};

json mat_muscle = {
    {"name", "muscle"},
    {"yield", 0.3},       // MPa - muscle tissue yields ~0.1-0.5 MPa
    {"break", 1.0},       // MPa - tears ~0.5-2 MPa
    {"density", 1.06},    // g/cm³
    {"elastic", 0.3}
};

json mat_fat = {
    {"name", "fat"},
    {"yield", 0.01},      // MPa - very soft
    {"break", 0.05},      // MPa
    {"density", 0.92},    // g/cm³
    {"elastic", 0.5}
};

json mat_fur = {
    {"name", "fur"},
    {"yield", 50},        // MPa - individual hair is strong
    {"break", 200},       // MPa - hair tensile ~150-250 MPa
    {"thick", 0.5},       // mm
    {"density", 1.3}      // g/cm³
};

// PLANT MATERIALS
json mat_apple_skin = {
    {"name", "apple_skin"},
    {"yield", 1.5},       // MPa - estimated for thin peel
    {"break", 4},         // MPa - tears fairly easily
    {"density", 0.76},    // g/cm³
    {"hardness", 2}       // Mohs estimate
};

json mat_apple_flesh = {
    {"name", "apple_flesh"},
    {"yield", 0.5},       // MPa - very soft
    {"break", 0.8},       // MPa
    {"density", 0.83},    // g/cm³ - real apple density
    {"brittle", 0.3}
};

json mat_apple_core = {
    {"name", "apple_core"},
    {"yield", 2},         // MPa - firmer than flesh
    {"break", 5},         // MPa
    {"density", 0.9},     // g/cm³
    {"hardness", 2.5},    // Mohs
    {"brittle", 0.5}
};

json mat_wood_soft = {
    {"name", "wood_soft"},  // Pine, balsa
    {"yield", 30},        // MPa - compressive yield ~25-35 MPa
    {"break", 40},        // MPa - bending strength ~40-70 MPa
    {"density", 0.45},    // g/cm³ - pine ~0.35-0.6
    {"hardness", 2}       // Mohs
};

json mat_wood_hard = {
    {"name", "wood_hard"},  // Oak, maple
    {"yield", 50},        // MPa - compressive ~40-60 MPa
    {"break", 100},       // MPa - bending ~90-120 MPa
    {"density", 0.75},    // g/cm³ - oak ~0.6-0.9
    {"hardness", 3.5}     // Mohs
};

// INORGANIC
json mat_stone = {
    {"name", "stone"},      // Limestone/sandstone
    {"yield", 100},       // MPa - varies widely
    {"break", 150},       // MPa
    {"density", 2.3},     // g/cm³
    {"hardness", 6},      // Mohs - varies by stone type
    {"brittle", 0.8}
};

json mat_granite = {
    {"name", "granite"},
    {"yield", 200},       // MPa
    {"break", 250},       // MPa
    {"density", 2.7},     // g/cm³
    {"hardness", 7},      // Mohs
    {"brittle", 0.9}
};

json mat_steel = {
    {"name", "steel"},      // Mild steel
    {"yield", 250},       // MPa - real yield strength
    {"break", 400},       // MPa - tensile strength
    {"density", 7.85},    // g/cm³
    {"hardness", 4.5},    // Mohs
    {"vickers", 200}      // HV - mild steel
};

json mat_iron = {
    {"name", "iron"},       // Wrought iron
    {"yield", 180},       // MPa
    {"break", 340},       // MPa
    {"density", 7.87},    // g/cm³
    {"vickers", 150}      // HV
};

json mat_leather = {
    {"name", "leather"},
    {"yield", 10},        // MPa
    {"break", 25},        // MPa - vegetable-tanned ~20-30 MPa
    {"thick", 2.5},       // mm
    {"density", 0.86}     // g/cm³
};

json mat_cloth = {
    {"name", "cloth"},      // Cotton fabric
    {"yield", 20},        // MPa - woven fabric
    {"break", 40},        // MPa
    {"thick", 0.3},       // mm
    {"density", 0.5}      // g/cm³ - effective with air gaps
};

// FLUIDS/SEMI-SOLIDS
json mat_water = {
    {"name", "water"},
    {"yield", 0},         // MPa - fluids don't yield
    {"density", 1.0},     // g/cm³
    {"viscosity", 0.001}  // Pa·s at 20°C - CORRECT unit!
};

json mat_blood = {
    {"name", "blood"},
    {"yield", 0},
    {"density", 1.06},    // g/cm³
    {"viscosity", 0.004}  // Pa·s - ~3-4x water
};

json mat_mud = {
    {"name", "mud"},
    {"yield", 0.001},     // MPa - minimal structure
    {"density", 1.6},     // g/cm³
    {"viscosity", 0.1},   // Pa·s - varies wildly
    {"sticky", 0.8}
};