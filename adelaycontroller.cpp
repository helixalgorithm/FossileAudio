//-----------------------------------------------------------------------------
// Project     : Fossile Audio Delay VST
// Version     : 1.0
//
// Category    : Effect plugin
// Created by  : Aomets, 02/2017
// Description : Non disruptive echo / doppler shift / dynamic echo controller script
//
//-----------------------------------------------------------------------------
// LICENSE
// GNU GPLv3

#include "adelaycontroller.h"
#include "adelayids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"

#if TARGET_OS_IPHONE
#include "interappaudio/iosEditor.h"
#endif

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayController::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		parameters.addParameter(STR16("Wet"), STR16("Mix"), 0, 1, ParameterInfo::kCanAutomate, kWetId);
		parameters.addParameter(STR16("Cutoff"), STR16("Highpass"), 0, 1, ParameterInfo::kCanAutomate, kCutId);
		parameters.addParameter (STR16("Delay"), STR16("sec"), 0, 1, ParameterInfo::kCanAutomate, kDelayId);
		parameters.addParameter(STR16("Feedback"), STR16("fb"), 0, 1, ParameterInfo::kCanAutomate, kFeedbackId);
		parameters.addParameter(STR16("LFO speed"), STR16("speed"), 0, 1, ParameterInfo::kCanAutomate, kLFOSpeedId);
		parameters.addParameter(STR16("LFO depth"), STR16("depth"), 0, 1, ParameterInfo::kCanAutomate, kLFODepthId);
		parameters.addParameter(STR16("PingPong depth"), STR16("PingPong"), 0, 1, ParameterInfo::kCanAutomate, kPingId);
	}
	return kResultTrue;
}

#if TARGET_OS_IPHONE
//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API ADelayController::createView (FIDString name)
{
	if (strcmp (name, ViewType::kEditor) == 0)
	{
		return new ADelayEditorForIOS (this);
	}
	return 0;
}
#endif

//------------------------------------------------------------------------
tresult PLUGIN_API ADelayController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (state)
	{
		float savedWet = 0.f;
		if (state->read(&savedWet, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}
#if BYTEORDER == kBigEndian
		SWAP_32(savedWet)
#endif
		setParamNormalized(kWetId, savedWet);
		float savedCut = 0.f;
		if (state->read(&savedCut, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedCut)
#endif
		setParamNormalized(kCutId, savedCut);

		float savedDelay = 0.f;
		if (state->read (&savedDelay, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32 (savedDelay)
#endif
		setParamNormalized (kDelayId, savedDelay);
			
		float savedFeedback = 0.f;
		if (state->read(&savedFeedback, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedFeedback)
#endif
		setParamNormalized(kFeedbackId, savedFeedback);

		float savedLFOSpeed = 0.f;
		if (state->read(&savedLFOSpeed, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedLFOSpeed)
#endif
		setParamNormalized(kLFOSpeedId, savedLFOSpeed);
	
		float savedLFODepth = 0.f;
		if (state->read(&savedLFODepth, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedLFODepth)
#endif
		setParamNormalized(kLFODepthId, savedLFODepth);

		float savedPing = 0.f;
		if (state->read(&savedPing, sizeof(float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32(savedPing)
#endif
			setParamNormalized(kPingId, savedPing);
	}
	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
