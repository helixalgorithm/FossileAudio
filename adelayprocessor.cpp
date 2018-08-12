//-----------------------------------------------------------------------------
// Project     : Fossile Audio Delay VST
// Version     : 1.0
//
// Category    : Effect plugin
// Created by  : Aomets, 02/2017
// Description : Non disruptive echo / doppler shift / dynamic echo processor script
//
//-----------------------------------------------------------------------------
// LICENSE
// GNU GPLv3

#include "adelayprocessor.h"
#include "adelayids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <algorithm>
#include <math.h>

#define PI 3.14159265

namespace Steinberg {
namespace Vst {
//-----------------------------------------------------------------------------
	ADelayProcessor::ADelayProcessor()
		: mWet(0.5f)
		, mCut(0)
		, mDelay(0.5f)
		, mBuffer(0)
		, iBuffer(0)
		, mBufferPos(0)
		, mFeedback(0.5f)
		, mLFOSpeed(0)
		, mLFODepth(0)
		, mPing(0)
		, LR(0.0) 
		, LFO(0)
		, pointLFO(0)
		, depthOfCursor(0)
		, crs(0)
		, crsB(0)
		, oldFeedback(0) // for de-zipper
		, oldPCD(0) // for de-zipper
		, tParams(0)
		, oldWet(0)
{
	setControllerClass (ADelayControllerUID);
}

	float ADelayProcessor::recurs(float v1, float rise1, float v2, float rise2, int pos, int i)
	{
		float AX = (float)(i / 2);
		float AY = v1 + rise1 * AX;
		float BY = v2 - rise2 * AX;
		float UY = (AY + BY) / 2; // UY = AY + Uk * i/2
		float Uk = (BY - AY) / i / 4;
		if (i > 1)
		{
			i = i / 2;
			crs[pos - i] = recurs(v1, rise1, UY, Uk, pos - i, i);
			crs[pos + i] = recurs(UY, Uk, v2, rise2, pos + i, i);
		}
		crsB[pos] = Uk;
		return UY;
	}
	float ADelayProcessor::highPass(float in, float cut, float res, float &lp1, float &bp1)
	{
		float multi = 0.3333333333333333f;
		float rez = 1 - res;
		//cutoff clamp at 1.0 and 20/44100
		float x = 3.141592 * (cut * 0.026315789473684210526315789473684) * 2 * 3.141592;
		float x2 = x * x;
		float x3 = x2 * x;
		float x5 = x3 * x2;
		float x7 = x5 * x2;
		float f = 2.0 * (x - (x3 * 0.16666666666666666666666666666667) + (x5 * 0.0083333333333333333333333333333333) - (x7 * 0.0001984126984126984126984126984127));
		in += 0.000000001f;
		lp1 = lp1 + f * bp1;
		float hp = in - lp1 - rez * bp1;
		bp1 = f * hp + bp1;
		float highPass = hp;
		lp1 = lp1 + f * bp1;
		hp = in - lp1 - rez * bp1;
		bp1 = f * hp + bp1;
		highPass += hp;
		in -= 0.000000001f;
		lp1 = lp1 + f * bp1;
		hp = in - lp1 - rez * bp1;
		return (highPass + hp) * multi;
	}
/*
float limit(float value) 
{
	if (value * value > 1) {
		return (float)(value - (int32)value);
	}
	else return value;
}*/
//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result == kResultTrue)
	{
		addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
		addAudioOutput (STR16 ("AudioOutput"), SpeakerArr::kStereo);
	}
	return result;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	// we only support one in and output bus and these buses must have the same number of channels
	if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::setActive (TBool state)
{
	SpeakerArrangement arr;
	if (getBusArrangement (kOutput, 0, arr) != kResultTrue)
		return kResultFalse;
	int32 numChannels = SpeakerArr::getChannelCount (arr);
	if (numChannels == 0)
		return kResultFalse;

	if (state)
	{
		mBuffer = (float**)malloc (numChannels * sizeof (float*)); //delay buffer
		iBuffer = (float**)malloc (numChannels * sizeof (float*)); //interpolation buffer
		mBufferPos = processSetup.sampleRate * 2;
		LR = 1.0;
		size_t size = (size_t)(processSetup.sampleRate * 2 * sizeof (float) + 0.5);
		size_t small = (size_t)(4 * sizeof(float) + 0.5);
		size_t tiny = (size_t)(2 * sizeof(float) + 0.5);
		depthOfCursor = 32;
		size_t cursorSize = (size_t)(depthOfCursor * sizeof(float) + 0.5);
		size_t lfoSize = (size_t)(512 * sizeof(float));
		crs = (float*)malloc(cursorSize);// new float[depthOfCursor + 1];
		memset(crs, 0, cursorSize);
		crsB = (float*)malloc(cursorSize);//new float[depthOfCursor + 1];
		memset(crsB, 0, cursorSize);
		tParams = (float**)malloc(numChannels * sizeof(float*));
		LFO = (float*)malloc(lfoSize);//new float[512]; //Wavetable sine
		double param = 2 * PI / 512;
		for (int i = 0; i < 512; i++) 
		{
			LFO[i] = sin((float)(i * param));
		}
		for (int32 channel = 0; channel < numChannels; channel++)
		{
			iBuffer[channel] = (float*)malloc(small);	// 4 samples
			memset(iBuffer[channel], 0, small);
			tParams[channel] = (float*)malloc(tiny);
			memset(tParams[channel], 0, tiny);
		}
		for (int32 channel = 0; channel < numChannels + 1; channel++)
		{
			mBuffer[channel] = (float*)malloc(size);	// 2 second delay max
			memset(mBuffer[channel], 0, size);
		}
		pointLFO = 0;
		mBufferPos = 0;
		LR = 0;
		oldFeedback = mFeedback;
		oldPCD = mDelay * mDelay * (depthOfCursor * 8 - 1) + 1;
		oldWet = mWet;
	}
	else
	{
		if (mBuffer)
		{
			for (int32 channel = 0; channel < numChannels; channel++)
			{
				free (mBuffer[channel]);
			}
			free (mBuffer);
			mBuffer = 0;
		}
		if (iBuffer) 
		{
			for (int32 channel = 0; channel < numChannels; channel++)
			{
				free(iBuffer[channel]);
			}
			free(iBuffer);
			iBuffer = 0;
		}
		if (tParams) 
		{
			for (int32 channel = 0; channel < numChannels; channel++)
			{
				free(tParams[channel]);
			}
			free(tParams);
			tParams = 0;
		}
		if (LFO) 
		{
			free(LFO);
		}
		if (crs) {
			free(crs);
		}
		if (crsB) {
			free(crsB);
		}
	}
	return AudioEffect::setActive (state);
}
//-----------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::process (ProcessData& data)
{
	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData (index);
			if (paramQueue)
			{
				ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				switch (paramQueue->getParameterId ())
				{
					case kWetId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mWet = value; //gives read value
						}
						break;
					case kCutId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mCut = value; //gives read value
						}
						break;
					case kDelayId:
						if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
							mDelay = value;
						break;
					case kFeedbackId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mFeedback = value; //gives read value
						}
						break;
					case kLFOSpeedId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mLFOSpeed = value; //gives read value
						}
						break;
					case kLFODepthId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mLFODepth = value; //gives read value
						}
						break;
					case kPingId:
						if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
						{
							mPing = value; //gives read value
						}
						break;
				}
			}
		}
	}

	if (data.numSamples > 0)
	{
		SpeakerArrangement arr;
		getBusArrangement (kOutput, 0, arr);
		int32 numChannels = SpeakerArr::getChannelCount (arr);
		// apply delay
		int cursor128 = depthOfCursor * 8;
		float pcD = mDelay * mDelay * (cursor128 - 1) + 1; // 1 = 128 samples 0 is 1 sample
		float LFORatio = mLFOSpeed * mLFOSpeed / 3;
		float LFOMagnitude = mLFODepth; //Problem: the larger pcD, the greater audible LFO
		float k1;
		float k2;
		float readPile; //Sum of samples in frame
		float LRcrs; //Offset of reading wavetable
		float dry; //input signal
		float tempLR; //temporary left reminder
		int32 tempBufferPos = mBufferPos;
		int32 curPos; //buffer position in a buffer loop
		int32 expect;
		float theRest;
		double tempLFOPos;
		int ct;
		float applyTransition = (mFeedback - oldFeedback) / data.numSamples;
		float applyDelayTransition = (pcD - oldPCD) / data.numSamples;
		float applyWetTransition = (mWet - oldWet) / data.numSamples;
		float filterFactor = 20 / processSetup.sampleRate;
		float cutoff = filterFactor + mCut * mCut * (1 - filterFactor);
		float temporary;
		for (int32 channel = 0; channel < numChannels; channel++)
		{
			float* inputChannel = data.inputs[0].channelBuffers32[channel];
			float* outputChannel = data.outputs[0].channelBuffers32[channel];
			curPos = mBufferPos;
			tempLR = LR;
			tempLFOPos = pointLFO;
			float curFeedback = oldFeedback; //smooth feedback knob tweak de-zipper
			float curPCD = oldPCD; //smooth delay time knob tweak de-zipper
			float curWet = oldWet;
			for (int32 sample = 0; sample < data.numSamples; sample++)
			{ // Sample buffer
				// LFO
				curPCD += applyDelayTransition; // de-zipper
				curWet += applyWetTransition; // de-zipper
				float sineSMP = LFOMagnitude * LFO[(int)(tempLFOPos)]; // sample from a wavetable
				float tempPcD = curPCD + sineSMP; //depthOfCursor / pace = samples in one frame
				if (tempPcD < 1) tempPcD = 1;
				if (tempPcD > cursor128) tempPcD = cursor128;
				float Pace = cursor128 / tempPcD; // 128 / 1 gives the longest cursor; 128 / 128 is the shortest (longest delay)
				//LFO
				// First: interpolation samples
				iBuffer[channel][0] = iBuffer[channel][1];
				iBuffer[channel][1] = iBuffer[channel][2];
				iBuffer[channel][2] = iBuffer[channel][3];
				iBuffer[channel][3] = inputChannel[sample];
				// First. four pillars of interpolation are now created
				// Second: interpolation algorithm
				k1 = (iBuffer[channel][3] - iBuffer[channel][1]) / depthOfCursor;
				k2 = (iBuffer[channel][2] - iBuffer[channel][0]) / depthOfCursor;
				crs[0] = iBuffer[channel][2]; //wavetable is to be read downto (from c to b )
				crsB[0] = k1;
				crs[depthOfCursor] = iBuffer[channel][1];
				crsB[depthOfCursor] = k2;
				crs[depthOfCursor / 2] = recurs(iBuffer[channel][2], k1, iBuffer[channel][1], k2, depthOfCursor/2, depthOfCursor/2);
				// Second. crs it now a wavetable of depthOfCursor float samples
				// Third
				dry = (1.0f - curWet) * inputChannel[sample];
				readPile = 0;
				expect = processSetup.sampleRate * 2 - 1 - curPos;
				LRcrs = depthOfCursor - tempLR * tempPcD / 8 - 0.000001;
				curFeedback += applyTransition;
				ct = 0;
				if (expect < Pace) //Cursor frame
				{//cursor in half; first determine the number of samples before 0
					theRest = Pace - tempLR + 0.999999 - (float)expect;
					for (int i = 0; i < expect; i++)
					{
						readPile += mBuffer[channel][curPos];
						int smp = (int)(LRcrs); //position of a sample and its inclination
						float trans = LRcrs + 0.5 - smp; //linear interpolation factor
						temporary = mBuffer[channel][curPos] * curFeedback + crs[smp] + crsB[smp] * trans;
						mBuffer[channel][curPos] = temporary * (1 - mPing) + mBuffer[numChannels][curPos] * mPing; //Y = a + bX
						mBuffer[numChannels][curPos] = temporary;
						LRcrs -= tempPcD / 8;
						curPos += 1;
						ct++;
					}
					curPos = 0;
					for (int i = 0; i < (int)(theRest); i++)
					{
						readPile += mBuffer[channel][curPos];
						int smp = (int)(LRcrs); //position of a sample and its inclination
						float trans = LRcrs + 0.5 - smp; //linear interpolation factor
						temporary = mBuffer[channel][curPos] * curFeedback + crs[smp] + crsB[smp] * trans;
						mBuffer[channel][curPos] = temporary * (1 - mPing) + mBuffer[numChannels][curPos] * mPing;
						mBuffer[numChannels][curPos] = temporary;
						LRcrs -= tempPcD / 8;
						curPos += 1;
						ct++;
					}
					tempLR += (int)(Pace - tempLR + 0.999999) - Pace;
					curPos = (int)theRest;
				}
				else // no breaking point
				{	
					expect = (int)(Pace - tempLR + 0.999999);
					for (int i = 0; i < expect; i++)
					{
						readPile += mBuffer[channel][curPos];
						int smp = (int)(LRcrs); //position of a sample and its inclination
						float trans = LRcrs + 0.5 - smp; //linear interpolation factor
						temporary = mBuffer[channel][curPos] * curFeedback + crs[smp] + crsB[smp] * trans;
						mBuffer[channel][curPos] = temporary * (1 - mPing) + mBuffer[numChannels][curPos] * mPing;
						mBuffer[numChannels][curPos] = temporary;
						LRcrs -= tempPcD / 8;
						curPos += 1;
						ct++;
					}
					tempLR += (float)expect - Pace;
					if (tempLR >= 1)
					{
						tempLR = tempLR - 1;
					}
					if (processSetup.sampleRate * 2 == curPos)
					{
						curPos = 0;
					}
				} // else
				outputChannel[sample] = dry + highPass(curWet * (float)(readPile / ct), cutoff, 0.2f, tParams[channel][0], tParams[channel][1]);
				// third.
				tempLFOPos += LFORatio;
				if (tempLFOPos >= 512) 
				{
					tempLFOPos -= (int)tempLFOPos;
				}
			} // sample buffer loop
			tempBufferPos = curPos;
		} // channel loop
		mBufferPos = tempBufferPos;
		LR = tempLR;
		pointLFO = tempLFOPos;
		oldFeedback = mFeedback; //smooth feedback knob tweak
		oldPCD = pcD; //smooth delay time knob tweak
		oldWet = mWet;
	}	
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;
	// called when we load a preset, the model has to be reloaded
	float savedWet = 0.f;
	if (state->read(&savedWet, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedCut = 0.f;
	if (state->read(&savedCut, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedDelay = 0.f;
	if (state->read (&savedDelay, sizeof (float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedFeedback = 0.f;
	if (state->read(&savedFeedback, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedLFOSpeed = 0.f;
	if (state->read(&savedLFOSpeed, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedLFODepth = 0.f;
	if (state->read(&savedLFODepth, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}
	float savedPing = 0.f;
	if (state->read(&savedPing, sizeof(float)) != kResultOk)
	{
		return kResultFalse;
	}

#if BYTEORDER == kBigEndian
	SWAP_32 (savedWet)
	SWAP_32 (savedCut)
	SWAP_32 (savedDelay)
	SWAP_32 (savedFeedback)
	SWAP_32 (savedLFOSpeed)
	SWAP_32 (savedLFODepth)
	SWAP_32(savedPing)
#endif
	mWet = savedWet;
	mCut = savedCut;
	mDelay = savedDelay;
	mFeedback = savedFeedback;
	mLFOSpeed = savedLFOSpeed;
	mLFODepth = savedLFODepth;
	mPing = savedPing;
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ADelayProcessor::getState (IBStream* state)
{
	// here we need to save the model
	float toSaveWet = mWet;
	float toSaveCut = mCut;
	float toSaveDelay = mDelay;
	float toSaveFeedback = mFeedback;
	float toSaveLFOSpeed = mLFOSpeed;
	float toSaveLFODepth = mLFODepth;
	float toSavePing = mPing;
#if BYTEORDER == kBigEndian
	SWAP_32 (toSaveWet)
	SWAP_32 (toSaveCut)
	SWAP_32 (toSaveDelay)
	SWAP_32 (toSaveFeedback)
	SWAP_32 (toSaveLFOSpeed)
	SWAP_32 (toSaveLFODepth)
	SWAP_32(toSavePing)
#endif
	state->write(&toSaveWet, sizeof(float));
	state->write(&toSaveCut, sizeof(float));
	state->write(&toSaveDelay, sizeof (float));
	state->write(&toSaveFeedback, sizeof(float));
	state->write(&toSaveLFOSpeed, sizeof(float));
	state->write(&toSaveLFODepth, sizeof(float));
	state->write(&toSavePing, sizeof(float));
	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
