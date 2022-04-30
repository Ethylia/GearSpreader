#pragma once

#include "SimpleIni.h"

#include <RE/T/TESForm.h>
#include <vector>

struct Settings
{
	enum levelmethod : uint8_t
	{
		Rating = 0,
		Gold
	};

	std::vector<const RE::TESFile*> blacklist;
	std::vector<const RE::TESFile*> whitelist;
	int botlevel = 0; // lower=earlier armor
	int toplevel = 40; // higher=later armor
	uint16_t maxlevel = 50;
	uint16_t maxadds = 5;
	uint8_t method = 0;
	int32_t botgold = Gold;
	int32_t topgold = 2500;
	int32_t maxcloset = 0;
	uint16_t maxclosetadds = 1;
	bool verboselog = false;
	bool debuglog = false;
	bool enable = true;
};

class SettingsLoader
{
public:
	bool LoadIni();
	bool LoadSection(Settings& settings, const char* const section);
private:
	CSimpleIniA ini;
};