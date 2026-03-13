/**
 * Wrapper sam.h for TinyUSB DFP v3 compatibility.
 *
 * When TinyUSB source files include "sam.h", this wrapper is found first
 * (via tinyusb INTERFACE include path). It includes the real DFP sam.h
 * via #include_next, then layers on v2-style type and macro definitions.
 */
#include_next "sam.h"

#include "tusb_dfp_v3_compat.h"
