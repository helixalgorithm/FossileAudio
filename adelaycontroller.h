//-----------------------------------------------------------------------------
// Project     : Fossile Audio Delay VST
// Version     : 1.0
//
// Category    : Effect plugin
// Created by  : Aomets, 02/2017
// Description : Non disruptive echo / doppler shift / dynamic echo controller header
//
//-----------------------------------------------------------------------------
// LICENSE
// GNU GPLv3

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

#if MAC
#include <TargetConditionals.h>
#endif

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
class ADelayController : public EditController
{
public:
	//------------------------------------------------------------------------
	// create function required for Plug-in factory,
	// it will be called to create new instances of this controller
	//------------------------------------------------------------------------
	static FUnknown* createInstance (void*) { return (IEditController*)new ADelayController (); }

	//---from IPluginBase--------
	tresult PLUGIN_API initialize (FUnknown* context);
	
	//---from EditController-----
#if TARGET_OS_IPHONE
	IPlugView* PLUGIN_API createView (FIDString name) override;
#endif
	tresult PLUGIN_API setComponentState (IBStream* state);
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

