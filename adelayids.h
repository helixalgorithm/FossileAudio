//-----------------------------------------------------------------------------
// Project     : FossileAudio Delay VST
// Version     : 1.0
//
// Category    : Effect plugin
// Created by  : Aomets, 02/2017
// Description : Non disruptive echo / doppler shift / dynamic echo id header
//
//-----------------------------------------------------------------------------
// LICENSE
// GNU GPLv3

#pragma once

namespace Steinberg {
namespace Vst {

// parameter tags
enum {
	kDelayId = 100,
	kWetId = 101,
	kFeedbackId = 102,
	kLFOSpeedId = 103,
	kLFODepthId = 104,
	kCutId = 105,
	kPingId = 106
};

// unique class ids
static const FUID ADelayProcessorUID (0x0CDBB669, 0x85D548A9, 0xBFD83719, 0x09D24BB3);
static const FUID ADelayControllerUID (0x038E7FA9, 0x629A4EAA, 0x8541B889, 0x18E8952C);

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
