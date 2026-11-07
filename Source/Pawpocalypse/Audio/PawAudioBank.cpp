#include "Audio/PawAudioBank.h"

#include "Sound/SoundWave.h"

const FPawAudioEvent* UPawAudioBank::FindEvent(const FString& Id) const
{
	return Events.FindByPredicate([&Id](const FPawAudioEvent& E) { return E.Id == Id; });
}

USoundWave* UPawAudioBank::FindTrack(const FString& Id) const
{
	const FPawAudioTrack* T = Music.FindByPredicate([&Id](const FPawAudioTrack& M) { return M.Id == Id; });
	return T ? T->Sound.Get() : nullptr;
}
