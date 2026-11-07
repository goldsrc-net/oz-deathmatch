//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "pm_shared.h"
#include "UserMessages.h"

// Oz: OZ 3.02 has no observer.cpp. Its observer (SDK 2.2's Observer_FindNextPlayer,
// Observer_HandleButtons and Observer_SetMode, and its own StopObserver) is in player.cpp, as in
// the original; halflife-updated's Observer_CheckTarget and Observer_CheckProperties are not in
// it, so this file is left empty (it stays in dlls/CMakeLists.txt).
