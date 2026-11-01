// Copyright Do Not Smoke Team. All Rights Reserved.

#include "DoNotSmokeLocale.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	struct FLocEntry
	{
		const TCHAR* Key;
		const TCHAR* Text[static_cast<int32>(EDoNotSmokeLanguage::Count)];
	};

	// English / Turkish / Dutch
	// Jet banners are drawn by a TextRenderComponent whose default font has no Turkish glyphs, so their Turkish text stays ASCII
	const FLocEntry Entries[] =
	{
		// Main menu
		{ TEXT("MenuTagline"), { TEXT("Renew the sky, one chimney at a time."), TEXT("Dumanı kes, güneşi geri getir."), TEXT("Vernieuw de lucht, schoorsteen voor schoorsteen.") } },
		{ TEXT("MenuPlay"), { TEXT("PLAY"), TEXT("OYNA"), TEXT("SPELEN") } },
		{ TEXT("MenuQuit"), { TEXT("QUIT"), TEXT("ÇIKIŞ"), TEXT("AFSLUITEN") } },
		{ TEXT("MenuLanguage"), { TEXT("EN"), TEXT("TR"), TEXT("NL") } },

		// Pause menu
		{ TEXT("PauseTitle"), { TEXT("PAUSED"), TEXT("DURAKLATILDI"), TEXT("GEPAUZEERD") } },
		{ TEXT("PauseResume"), { TEXT("RESUME"), TEXT("DEVAM ET"), TEXT("HERVATTEN") } },
		{ TEXT("PauseExit"), { TEXT("EXIT"), TEXT("ÇIKIŞ"), TEXT("AFSLUITEN") } },

		// Countdown
		{ TEXT("CountdownGo"), { TEXT("RENEW IT!"), TEXT("YENİLE!"), TEXT("VERNIEUW HET!") } },

		// Controls panel
		{ TEXT("CtrlTitle"), { TEXT("CONTROLS"), TEXT("KONTROLLER"), TEXT("BESTURING") } },
		{ TEXT("CtrlKeyPitch"), { TEXT("W / S"), TEXT("W / S"), TEXT("W / S") } },
		{ TEXT("CtrlPitch"), { TEXT("Nose down / up"), TEXT("Burun aşağı / yukarı"), TEXT("Neus omlaag / omhoog") } },
		{ TEXT("CtrlKeyRoll"), { TEXT("A / D"), TEXT("A / D"), TEXT("A / D") } },
		{ TEXT("CtrlRoll"), { TEXT("Bank left / right"), TEXT("Sola / sağa yatış"), TEXT("Naar links / rechts hellen") } },
		{ TEXT("CtrlKeyAim"), { TEXT("Mouse"), TEXT("Fare"), TEXT("Muis") } },
		{ TEXT("CtrlAim"), { TEXT("Look and aim"), TEXT("Bak ve nişan al"), TEXT("Kijken en richten") } },
		{ TEXT("CtrlKeyFire"), { TEXT("Left Click"), TEXT("Sol Tık"), TEXT("Linkermuisknop") } },
		{ TEXT("CtrlFire"), { TEXT("Fire at a chimney rim"), TEXT("Baca ağzına atış yap"), TEXT("Op een schoorsteenrand schieten") } },
		{ TEXT("CtrlKeyBoost"), { TEXT("Space / Shift"), TEXT("Space / Shift"), TEXT("Spatie / Shift") } },
		{ TEXT("CtrlBoost"), { TEXT("Solar boost (uses battery)"), TEXT("Güneş boost (batarya harcar)"), TEXT("Zonneboost (kost batterij)") } },
		{ TEXT("CtrlHint"), { TEXT("Fly in sunlight to recharge"), TEXT("Şarj olmak için güneşte uç"), TEXT("Vlieg in de zon om op te laden") } },

		// Flight HUD
		{ TEXT("HudStatus"), { TEXT("STACKS {0}/{1}  SUN {2}%  {3}"), TEXT("BACA {0}/{1}  GÜNEŞ {2}%  {3}"), TEXT("PIJPEN {0}/{1}  ZON {2}%  {3}") } },
		{ TEXT("HudCarbon"), { TEXT("CO2 {0} / {1} kg"), TEXT("CO2 {0} / {1} kg"), TEXT("CO2 {0} / {1} kg") } },
		{ TEXT("HudJetIncoming"), { TEXT("!  VIP JET INCOMING  !"), TEXT("!  VIP JET GELİYOR  !"), TEXT("!  VIP-JET NADERT  !") } },
		{ TEXT("HudPullUp"), { TEXT("PULL UP!"), TEXT("YÜKSEL!"), TEXT("OPTREKKEN!") } },

		// Crash screen
		{ TEXT("CrashGround"), { TEXT("GROUND IMPACT"), TEXT("YERE ÇAKILDIN"), TEXT("NEERGESTORT") } },
		{ TEXT("CrashSmokestack"), { TEXT("SMOKESTACK COLLISION"), TEXT("BACAYA ÇARPTIN"), TEXT("TEGEN EEN SCHOORSTEEN") } },
		{ TEXT("CrashToxicPlume"), { TEXT("CHOKED IN TOXIC PLUME"), TEXT("ZEHİRLİ DUMANDA BOĞULDUN"), TEXT("GESTIKT IN GIFTIGE ROOK") } },
		{ TEXT("CrashRestart"), { TEXT("RESTART  [R]"), TEXT("TEKRAR  [R]"), TEXT("OPNIEUW  [R]") } },

		// COP31 newspaper
		{ TEXT("PaperMasthead"), { TEXT("THE DAILY SMOG"), TEXT("GÜNLÜK DUMAN"), TEXT("DE DAGELIJKSE SMOG") } },
		{ TEXT("PaperEdition"), { TEXT("COP31 CLIMATE SUMMIT  ·  SPECIAL EDITION"), TEXT("COP31 İKLİM ZİRVESİ  ·  ÖZEL BASKI"), TEXT("COP31 KLIMAATTOP  ·  SPECIALE EDITIE") } },
		{ TEXT("PaperAudit"), { TEXT("AUDIT"), TEXT("DENETİM"), TEXT("AUDIT") } },
		{ TEXT("NextLevel"), { TEXT("NEXT LEVEL"), TEXT("SONRAKİ BÖLÜM"), TEXT("VOLGEND LEVEL") } },
		{ TEXT("PaperFooter"), { TEXT("[R]  RESTART SECTOR"), TEXT("[R]  BÖLGEYİ BAŞTAN OYNA"), TEXT("[R]  SECTOR OPNIEUW") } },
		{ TEXT("PaperCarbon"), { TEXT("CO2  {0} kg  /  target {1} kg"), TEXT("CO2  {0} kg  /  hedef {1} kg"), TEXT("CO2  {0} kg  /  doel {1} kg") } },
		{ TEXT("PaperTime"), { TEXT("TIME  {0} s"), TEXT("SÜRE  {0} sn"), TEXT("TIJD  {0} s") } },
		{ TEXT("PaperAccuracy"), { TEXT("ACCURACY  {0}%"), TEXT("İSABET  {0}%"), TEXT("TREFZEKERHEID  {0}%") } },
		{ TEXT("Headline3"), {
			TEXT("THE SKY BREATHES AGAIN: MYSTERY GLIDER SHUTS DOWN INDUSTRIAL STACKS AT THE SOURCE!"),
			TEXT("GÖKYÜZÜ NEFES ALDI: GİZEMLİ PLANÖR SANAYİ BACALARINI KAYNAĞINDA MÜHÜRLEDİ!"),
			TEXT("DE LUCHT ADEMT WEER: MYSTERIEUZE ZWEEFVLIEGER LEGT FABRIEKSSCHOORSTENEN STIL!") } },
		{ TEXT("Headline2"), {
			TEXT("EMISSIONS CURBED, BUT THE CARBON CLOUD HAS NOT FULLY CLEARED"),
			TEXT("EMİSYONLAR FRENLENDİ AMA KARBON BULUTU TAM DAĞILMADI"),
			TEXT("UITSTOOT AFGEREMD, MAAR DE KOOLSTOFWOLK HANGT ER NOG") } },
		{ TEXT("Headline1"), {
			TEXT("NARROW ESCAPE: STACKS SHUT, BUT THE CITY SWALLOWED THE SOOT"),
			TEXT("KIL PAYI KURTULUŞ: BACALAR KAPANDI AMA ŞEHİR İS YUTTU"),
			TEXT("OP HET NIPPERTJE: SCHOORSTENEN DICHT, MAAR DE STAD SLIKTE HET ROET") } },

		// VIP jet banners (see note above: Turkish kept ASCII)
		{ TEXT("JetSlogan0"), { TEXT("ON MY WAY TO CLIMATE SUMMIT!"), TEXT("IKLIM ZIRVESINE GIDIYORUM!"), TEXT("OP WEG NAAR DE KLIMAATTOP!") } },
		{ TEXT("JetSlogan1"), { TEXT("DON'T FORGET YOUR PAPER STRAW!"), TEXT("KAGIT PIPETINI UNUTMA!"), TEXT("VERGEET JE PAPIEREN RIETJE NIET!") } },
		{ TEXT("JetSlogan2"), { TEXT("THOUGHTS & PRAYERS FOR THE PLANET"), TEXT("GEZEGEN ICIN DUALARIMIZLA"), TEXT("WE DENKEN AAN DE PLANEET") } },
		{ TEXT("JetSlogan3"), { TEXT("NET-ZERO BY 2099!"), TEXT("2099'DA NET SIFIR!"), TEXT("NETTO NUL IN 2099!") } },
	};

	const TCHAR* const ConfigSection = TEXT("/Script/DoNotSmoke.Language");
	const TCHAR* const ConfigKey = TEXT("Language");

	EDoNotSmokeLanguage CurrentLanguage = EDoNotSmokeLanguage::English;
	bool bLanguageLoaded = false;

	void EnsureLanguageLoaded()
	{
		if (bLanguageLoaded)
		{
			return;
		}
		bLanguageLoaded = true;

		int32 Saved = 0;
		if (GConfig && GConfig->GetInt(ConfigSection, ConfigKey, Saved, GGameUserSettingsIni))
		{
			if (Saved >= 0 && Saved < static_cast<int32>(EDoNotSmokeLanguage::Count))
			{
				CurrentLanguage = static_cast<EDoNotSmokeLanguage>(Saved);
			}
		}
	}

	const FLocEntry* FindEntry(const TCHAR* Key)
	{
		for (const FLocEntry& Entry : Entries)
		{
			if (FCString::Strcmp(Entry.Key, Key) == 0)
			{
				return &Entry;
			}
		}
		return nullptr;
	}
}

namespace DoNotSmokeLocale
{
	EDoNotSmokeLanguage GetLanguage()
	{
		EnsureLanguageLoaded();
		return CurrentLanguage;
	}

	void SetLanguage(EDoNotSmokeLanguage Language)
	{
		bLanguageLoaded = true;
		CurrentLanguage = Language;

		if (GConfig)
		{
			GConfig->SetInt(ConfigSection, ConfigKey, static_cast<int32>(Language), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
	}

	EDoNotSmokeLanguage CycleLanguage()
	{
		const int32 Next = (static_cast<int32>(GetLanguage()) + 1) % static_cast<int32>(EDoNotSmokeLanguage::Count);
		SetLanguage(static_cast<EDoNotSmokeLanguage>(Next));
		return CurrentLanguage;
	}

	FString Get(const TCHAR* Key)
	{
		const FLocEntry* Entry = FindEntry(Key);
		if (!Entry)
		{
			return FString(Key);
		}

		const TCHAR* Localized = Entry->Text[static_cast<int32>(GetLanguage())];
		return FString(Localized ? Localized : Entry->Text[0]);
	}

	FText Text(const TCHAR* Key)
	{
		return FText::FromString(Get(Key));
	}

	FText Format(const TCHAR* Key, const FStringFormatOrderedArguments& Args)
	{
		return FText::FromString(FString::Format(*Get(Key), Args));
	}
}
