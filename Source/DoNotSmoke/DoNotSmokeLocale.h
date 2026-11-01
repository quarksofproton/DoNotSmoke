// Copyright Do Not Smoke Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EDoNotSmokeLanguage : uint8
{
	English,
	Turkish,
	Dutch,
	Count
};

/**
 * Small in-code string table for every on-screen text (English / Turkish / Dutch).
 * The chosen language lives in a process-wide static, so it survives level reloads,
 * and is saved to GameUserSettings.ini so it survives restarts too.
 */
namespace DoNotSmokeLocale
{
	/** Number of satirical jet banner slogans (keys JetSlogan0..JetSloganCount-1). */
	constexpr int32 JetSloganCount = 4;

	EDoNotSmokeLanguage GetLanguage();
	void SetLanguage(EDoNotSmokeLanguage Language);

	/** Advances to the next language, saves it and returns it. */
	EDoNotSmokeLanguage CycleLanguage();

	/** Localized string for Key in the current language; falls back to English, then to the key itself. */
	FString Get(const TCHAR* Key);

	FText Text(const TCHAR* Key);

	/** Fills {0}, {1}, ... placeholders of the localized string. */
	FText Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args);
}
