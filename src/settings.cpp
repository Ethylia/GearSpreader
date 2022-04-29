#include "settings.h"

static const char* const IGNORED_FILES[] = {"Skyrim.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Dragonborn.esm"};

static const char* const DEFAULTINI_PATH = "Data/SKSE/Plugins/GearSpreader.ini";
static const char* const CUSTOMINI_PATH = "Data/SKSE/Plugins/GearSpreaderCustom.ini";

bool SettingsLoader::LoadIni()
{
	ini.SetUnicode();
	ini.SetMultiKey();

	SI_Error err = ini.LoadFile(CUSTOMINI_PATH);
	if(err < 0)
	{
		err = ini.LoadFile(DEFAULTINI_PATH);
		if(err < 0)
		{
			SKSE::log::warn("Could not open either GearSpreaderCustom.ini or GearSpreader.ini, using default settings");
			return false;
		} else
			SKSE::log::info("Loaded GearSpreader.ini, you can create GearSpreaderCustom.ini to be loaded instead");
	} else
		SKSE::log::info("Loaded GearSpreaderCustom.ini");

	return true;
}

bool SettingsLoader::LoadSection(Settings& settings, const char* const section)
{
	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	{
		CSimpleIniA::TNamesDepend wl;
		ini.GetAllValues(section, "whitelist", wl);
		if(!wl.empty() && wl.front().pItem[0] != '\0')
		{
			wl.sort(CSimpleIniA::Entry::LoadOrder());
			for(CSimpleIniA::TNamesDepend::const_iterator entry = wl.begin(); entry != wl.end(); ++entry)
			{
				const char* last = entry->pItem;
				for(size_t i = 0;; ++i)
				{
					if(entry->pItem[i] == ',')
					{
						const std::string_view str(last, &entry->pItem[i] - last);
						const RE::TESFile* f = dh->LookupModByName(str);
						if(f && f->GetCompileIndex() != 255)
							settings.whitelist.push_back(f);
						else
							SKSE::log::warn("Mod {} whitelisted but not loaded in game", str);
						last = &entry->pItem[i + 1];
					} else if(entry->pItem[i] == '\0')
					{
						const std::string_view str(last, &entry->pItem[i] - last);
						const RE::TESFile* f = dh->LookupModByName(str);
						if(f && f->GetCompileIndex() != 255)
							settings.whitelist.push_back(f);
						else
							SKSE::log::warn("Mod {} whitelisted but not loaded in game", str);
						break;
					}
				}
			}
		}
	}
	if(settings.whitelist.empty())
	{
		// add the default ones
		for(const char* i : IGNORED_FILES)
		{
			const RE::TESFile* f = dh->LookupModByName(i);
			if(f)
				settings.blacklist.push_back(f);
		}

		CSimpleIniA::TNamesDepend bl;
		ini.GetAllValues(section, "ignorefiles", bl);
		bl.sort(CSimpleIniA::Entry::LoadOrder());
		if(!bl.empty() && bl.front().pItem[0] != '\0')
		{
			for(CSimpleIniA::TNamesDepend::const_iterator entry = bl.begin(); entry != bl.end(); ++entry)
			{
				const char* last = entry->pItem;
				for(size_t i = 0;; ++i)
				{
					if(entry->pItem[i] == ',')
					{
						const std::string_view str(last, &entry->pItem[i] - last);
						const RE::TESFile* f = dh->LookupModByName(str);
						if(f && f->GetCompileIndex() != 255)
							settings.blacklist.push_back(f);
						else
							SKSE::log::warn("Mod {} blacklisted but not loaded in game", str);
						last = &entry->pItem[i + 1];
					} else if(entry->pItem[i] == '\0')
					{
						const std::string_view str(last, &entry->pItem[i] - last);
						const RE::TESFile* f = dh->LookupModByName(str);
						if(f && f->GetCompileIndex() != 255)
							settings.blacklist.push_back(f);
						else
							SKSE::log::warn("Mod {} blacklisted but not loaded in game", str);
						break;
					}
				}
			}
		}
	}

	std::string_view method = ini.GetValue(section, "method", "rating");
	if(method == "rating")
		settings.method = Settings::Rating;
	else
		settings.method = Settings::Gold;

	// these default to their definition values in the header file if not set in the ini
	settings.botlevel = (int)ini.GetLongValue(section, "bottomlevel", settings.botlevel);
	settings.toplevel = (int)ini.GetLongValue(section, "toplevel", settings.toplevel);
	settings.maxadds = (uint16_t)ini.GetLongValue(section, "maxadds", settings.maxadds);
	settings.maxlevel = (uint16_t)ini.GetLongValue(section, "maxlevel", settings.maxlevel);
	settings.botgold = (int32_t)ini.GetLongValue(section, "bottomgold", settings.botgold);
	settings.topgold = (int32_t)ini.GetLongValue(section, "topgold", settings.topgold);
	settings.verboselog = ini.GetBoolValue(section, "verboselog", settings.verboselog);
	settings.debuglog = ini.GetBoolValue(section, "debuglog", settings.debuglog);
	settings.enable = ini.GetBoolValue(section, "enable", settings.enable);

	return true;
}