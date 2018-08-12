//-----------------------------------------------------------------------------
// Project     : FossileAudio Delay VST
// Version     : 1.0
//
// Category    : Effect plugin
// Created by  : Aomets, 02/2017
// Description : Non disruptive echo / doppler shift / dynamic echo processor header
//
//-----------------------------------------------------------------------------
// LICENSE
// GNU GPLv3

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
class ADelayProcessor : public AudioEffect
{
public:
	ADelayProcessor ();

	float recurs(float v1, float rise1, float v2, float rise2, int pos, int i);
	float highPass(float in, float cut, float res, float &lp, float &bp);
	tresult PLUGIN_API initialize (FUnknown* context);
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);

	tresult PLUGIN_API setActive (TBool state);
	tresult PLUGIN_API process (ProcessData& data);
	
	//------------------------------------------------------------------------
	tresult PLUGIN_API setState (IBStream* state);
	tresult PLUGIN_API getState (IBStream* state);

	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new ADelayProcessor (); }

protected:
	ParamValue mWet;
	ParamValue mCut;
	ParamValue mDelay;
	float** mBuffer;
	float** iBuffer;
	int32 mBufferPos;
	ParamValue mFeedback;
	ParamValue mLFOSpeed;
	ParamValue mLFODepth;
	ParamValue mPing;
	float LR; // values [0-1]
    float* LFO;
	double pointLFO;
	int depthOfCursor;
	float* crs; //cursor sample values
	float* crsB; //cursor bias values
	float oldFeedback;
	float oldPCD;
	float** tParams;
	float oldWet;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
