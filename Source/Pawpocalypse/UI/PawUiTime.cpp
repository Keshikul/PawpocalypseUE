#include "UI/PawUiTime.h"

#include "Misc/App.h"

namespace PawUi
{
	float StepDelta(const UObject* Widget, float SlateDelta)
	{
		if (!FApp::UseFixedTimeStep())
		{
			return SlateDelta;
		}
		// The engine frame each widget last stepped in (only ever filled under a fixed step: a dev recording).
		static TMap<const UObject*, uint64> LastFrame;
		uint64& Last = LastFrame.FindOrAdd(Widget, MAX_uint64);
		if (Last == GFrameCounter)
		{
			return 0.0f;
		}
		Last = GFrameCounter;
		return static_cast<float>(FApp::GetFixedDeltaTime());
	}
}
