// Covox.h: interface for the CCovox class.
//


#pragma once

#include "BKSoundDevice.h"

class CCovox : public CBKSoundDevice
{
	public:
		CCovox();
		virtual ~CCovox() override;
		virtual void        ReInit() override;

		virtual void        SetData(uint16_t inVal) override;
		virtual void        GetSample(sOneSample *pSm) override;
};
