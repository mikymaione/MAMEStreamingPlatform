// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

	mame.cpp

	Specific (per target) constants

****************************************************************************/

#include "emu.h"

#define APPNAME                 "MAME"
#define APPNAME_LOWER           "mame"
#define CONFIGNAME              "mame"
#define COPYRIGHT               "Copyright Nicola Salmoria\nand the MAME team\nhttp://mamedev.org"
#define COPYRIGHT_INFO          "Copyright Nicola Salmoria and the MAME team"

const char* emulator_info::get_appname() { return APPNAME; }
const char* emulator_info::get_appname_lower() { return APPNAME_LOWER; }
const char* emulator_info::get_configname() { return CONFIGNAME; }
const char* emulator_info::get_copyright() { return COPYRIGHT; }
const char* emulator_info::get_copyright_info() { return COPYRIGHT_INFO; }
