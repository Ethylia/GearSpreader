#include "distributor.h"

#include <RE/I/IFormFactory.h>
#include <SKSE/SKSE.h>

#include <array>

static const char* IGNORED_FILES[] = {"Skyrim.esm", "Update.esm", "Dawnguard.esm", "HearthFires.esm", "Dragonborn.esm"};

static const char* CATEGORY_STRINGS[] = {"Clothing", "EnchClothing", "Light", "EnchLight", "Heavy", "EnchHeavy", "All"};
static const int8_t CHANCE_INTS[] = {10, 25, 50, 75, 100, 100};

template<class T>
static size_t FListSize(std::forward_list<T>& list)
{
	size_t s = 0;
	for([[maybe_unused]] auto i : list) ++s;
	return s;
}
static bool strequal(const char* str, const char* cmpstr)
{ // boomer 'performant' equality check, probably implemented somewhere in the std but screw it
	size_t i = 0;
	for(; str[i] != '\0' && cmpstr[i] != '\0'; ++i)
		if(str[i] != cmpstr[i])
			return false;
	if(str[i] == cmpstr[i])
		return true;
	return false;
}

static uint16_t GetMinLvl(const RE::TESLevItem* list)
{
	uint16_t lvl = 1000;
	for(size_t i = 0; i < (size_t)list->numEntries; ++i)
		lvl = std::min<uint16_t>(list->entries[i].level, lvl);
	return lvl;
}

static bool SortByLvl(const RE::LEVELED_OBJECT& lhs, const RE::LEVELED_OBJECT& rhs)
{
	return lhs.level < rhs.level;
}

uint16_t Distributor::CalculateLvl(const RE::TESObjectARMO* armor) const
{
	RE::BGSBipedObjectForm::ArmorType at = armor->GetArmorType();
	int ar = (int)(armor->armorRating / 100); // rating is 100x for some reason
	if(at == RE::BGSBipedObjectForm::ArmorType::kClothing)
		return 1;
	else if(at == RE::BGSBipedObjectForm::ArmorType::kLightArmor)
	{
		if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kBody))
		{ // look I dunno how to shorten that if statement ok
			// linear interpolation between lowest vanilla level armor and highest then grouped into 5 level slices
			return std::clamp<uint16_t>((uint16_t)(((ar - 20) * ((settings.toplevel - settings.bottomlevel) / 21) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHair))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 10) * ((settings.toplevel - settings.bottomlevel) / 7) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kShield))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 15) * ((settings.toplevel - settings.bottomlevel) / 14) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 5) * ((settings.toplevel - settings.bottomlevel) / 7) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		}
	} else if(at == RE::BGSBipedObjectForm::ArmorType::kHeavyArmor)
	{
		if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kBody))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 25) * ((settings.toplevel - settings.bottomlevel) / 24) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHair))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 15) * ((settings.toplevel - settings.bottomlevel) / 8) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kShield))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 20) * ((settings.toplevel - settings.bottomlevel) / 26) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHands))
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 11) * ((settings.toplevel - settings.bottomlevel) / 7) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		} else
		{
			return std::clamp<uint16_t>((uint16_t)(((ar - 10) * ((settings.toplevel - settings.bottomlevel) / 8) + settings.bottomlevel) / 5 * 5), 1, settings.maxlevel);
		}
	}
	return 1;
}

bool Distributor::LoadIni()
{
	const char* path = "Data/SKSE/Plugins/GearSpreader.ini";
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	SI_Error err = ini.LoadFile(path);
	if(err < 0) return false;

	{
		CSimpleIniA::TNamesDepend whitelist;
		ini.GetAllValues("GearSpreader", "whitelist", whitelist);
		if(!whitelist.empty() && whitelist.front().pItem[0] != '\0')
		{
			settings.usingwhitelist = true;
			whitelist.sort(CSimpleIniA::Entry::LoadOrder());
			for(CSimpleIniA::TNamesDepend::const_iterator entry = whitelist.begin(); entry != whitelist.end(); ++entry)
			{
				const char* last = entry->pItem;
				for(size_t i = 0;; ++i)
				{
					if(entry->pItem[i] == ',')
					{
						//ignores[i] = '\0';
						char* newstr = new char[&entry->pItem[i] - last + 1];
						settings.whitelist.push_back(newstr);
						memcpy(newstr, last, &entry->pItem[i] - last);
						newstr[&entry->pItem[i] - last] = 0;
						last = &entry->pItem[i + 1];
					} else if(entry->pItem[i] == '\0')
					{
						char* newstr = new char[&entry->pItem[i] - last + 1];
						settings.whitelist.push_back(newstr);
						memcpy(newstr, last, &entry->pItem[i] - last + 1);
						break;
					}
				}
			}
		}
	}
	if(!settings.usingwhitelist)
	{
		// add the default ones
		for(const char* i : IGNORED_FILES)
		{
			size_t len = strlen(i) + 1;
			char* newstr = new char[len];
			memcpy(newstr, i, len);
			settings.blacklist.push_back(newstr);
		}

		CSimpleIniA::TNamesDepend blacklist;
		ini.GetAllValues("GearSpreader", "ignorefiles", blacklist);
		blacklist.sort(CSimpleIniA::Entry::LoadOrder());
		if(!blacklist.empty() && blacklist.front().pItem[0] != '\0')
		{
			for(CSimpleIniA::TNamesDepend::const_iterator entry = blacklist.begin(); entry != blacklist.end(); ++entry)
			{
				const char* last = entry->pItem;
				for(size_t i = 0;; ++i)
				{
					if(entry->pItem[i] == ',')
					{
						char* newstr = new char[&entry->pItem[i] - last + 1];
						settings.blacklist.push_back(newstr);
						memcpy(newstr, last, &entry->pItem[i] - last);
						newstr[&entry->pItem[i] - last] = 0;
						last = &entry->pItem[i + 1];
					} else if(entry->pItem[i] == '\0')
					{
						char* newstr = new char[&entry->pItem[i] - last + 1];
						memcpy(newstr, last, &entry->pItem[i] - last + 1);
						settings.blacklist.push_back(newstr);
						break;
					}
				}
			}
		}
	}

	// these default to their definition values in the header file if not set in the ini
	settings.bottomlevel = (int)ini.GetLongValue("GearSpreader", "bottomlevel", settings.bottomlevel);
	settings.toplevel = (int)ini.GetLongValue("GearSpreader", "toplevel", settings.toplevel);
	settings.maxAdds = (uint16_t)ini.GetLongValue("GearSpreader", "maxadds", settings.maxAdds);
	settings.maxlevel = (uint16_t)ini.GetLongValue("GearSpreader", "maxlevel", settings.maxlevel);
	settings.verboselog = ini.GetBoolValue("GearSpreader", "verboselog", settings.verboselog);

	return true;
}

bool Distributor::PopulateLists()
{ // we make all the lvllists here
	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	auto& frms = dh->GetFormArray(RE::FormType::Armor);
	if(!frms.size())
		return false;

	for(auto& form : frms)
	{
		auto* file = form->GetFile(0);
		bool invalid = false;
		if(!settings.usingwhitelist)
		{
			for(auto str : settings.blacklist)
			{ // check if origin file is an ignored file
				if(strequal(str, file->GetFilename().data()) || !(file->compileIndex < 0xFF))
				{
					invalid = true;
					break;
				}
			}
		} else
		{
			invalid = true;
			for(auto str : settings.whitelist)
			{ // check if origin file is whitelisted
				if(strequal(str, file->GetFilename().data()) && (file->compileIndex < 0xFF))
				{
					invalid = false;
					break;
				}
			}
		}

		// let's invalidate unnamed armors for safety
		if(invalid || !form->GetPlayable() || !form->GetName()[0])
			continue;

		// Categorize all armors and put em in a vector for later
		category c = category::_count;
		switch(form->As<RE::TESObjectARMO>()->GetArmorType())
		{
		case RE::BGSBipedObjectForm::ArmorType::kClothing:
			if(form->As<RE::TESObjectARMO>()->formEnchanting)
				c = category::enchclothing;
			else
				c = category::clothing;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kLightArmor:
			if(form->As<RE::TESObjectARMO>()->formEnchanting)
				c = category::enchlight;
			else
				c = category::light;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kHeavyArmor:
			if(form->As<RE::TESObjectARMO>()->formEnchanting)
				c = category::enchheavy;
			else
				c = category::heavy;
			break;
		default:
			SKSE::log::warn("Unknown armor type for: {}", form->GetName());
			break;
		}
		if(c != category::_count) // accessing the unordered map creates an entry if it doesn't exist
			ArmorList((uint16_t)file->GetCompileIndex() + file->GetSmallFileCompileIndex(), c).push_front(form->As<RE::TESObjectARMO>());
	}

	if(_entries.empty())
		return false;

	auto fac = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESLevItem>();
	if(!fac) return false;

	uint8_t mCount = (uint8_t)std::min<size_t>(_entries.size(), 255);

	auto* lAllMods = fac->Create();
	lAllMods->chanceNone = 0;
	lAllMods->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
	lAllMods->entries.resize(mCount);
	lAllMods->numEntries = mCount;
	RE::LEVELED_OBJECT lAllModsObj = {0};
	lAllModsObj.count = 1;

	size_t itrAllMods = 0; // for adding mods to full lvllist
	for(auto& [modid, entry] : _entries)
	{
		for(size_t i = 0; i < category::_count; ++i)
		{
			size_t size = FListSize(entry.armors[i]);
			if(entry.armors[i].empty())
				continue; // Skip empty categories
			if(size > 255)
			{
				const char* modname = 0;
				if(modid > 253) // 254 and higher is a light mod
					modname = dh->LookupLoadedLightModByIndex((uint16_t)modid - 254)->GetFilename().data();
				else
					modname = dh->LookupLoadedModByIndex((uint8_t)modid)->GetFilename().data();
				SKSE::log::warn("Too many armors for category {} in mod {} (max 255, total {})", CATEGORY_STRINGS[i], modname, size);
			}

			uint8_t count = (uint8_t)std::min<size_t>(size, 255);

			auto* lCategory = fac->Create();
			lCategory->chanceNone = 0;
			lCategory->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
			lCategory->entries.resize(count);
			lCategory->numEntries = count;

			uint8_t j = 0;
			RE::LEVELED_OBJECT lObj = {0};
			lObj.count = 1;
			for(auto armor : entry.armors[i])
			{
				lObj.level = CalculateLvl(armor);
				lObj.form = armor;
				lCategory->entries[j++] = lObj;
				if(j > 254) break;
			}
			std::sort(lCategory->entries.begin(), lCategory->entries.end(), SortByLvl);
			entry.lCategories[i] = lCategory;
		}
		// lvllist with all categories
		auto* lAll = fac->Create();

		// get ammount of categories with a lvllist
		uint8_t count = 0;
		for(auto& counter : entry.lCategories) if(counter) ++count;

		lAll->chanceNone = 0;
		lAll->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
		lAll->entries.resize(count);
		lAll->numEntries = count;
		RE::LEVELED_OBJECT lAllObj = {0};
		lAllObj.count = 1;
		uint8_t i = 0;
		for(auto& lvlList : entry.lCategories)
		{
			if(!lvlList)
				continue;

			lAllObj.form = lvlList;
			lAllObj.level = GetMinLvl(lvlList);
			lAll->entries[i++] = lAllObj;
		}
		std::sort(lAll->entries.begin(), lAll->entries.end(), SortByLvl);
		entry.lAll = lAll;

		// add mod to lvllist with all mods
		if(itrAllMods < 255)
		{
			lAllModsObj.form = lAll;
			lAllModsObj.level = GetMinLvl(lAll);
			lAllMods->entries[itrAllMods++] = lAllModsObj;
		} else
		{
			const char* modname = 0;
			if(modid > 253) // 254 and higher is a light mod
				modname = dh->LookupLoadedLightModByIndex((uint16_t)modid - 254)->GetFilename().data();
			else
				modname = dh->LookupLoadedModByIndex((uint8_t)modid)->GetFilename().data();
			SKSE::log::warn("Armor Mod limit reached, ignoring mod {}", modname);
		}
	}

	std::sort(lAllMods->entries.begin(), lAllMods->entries.end(), SortByLvl);

	//lets create all the full lvllists for all chances
	size_t itr = 0;
	for(auto& fullList : _lAll)
	{
		auto* lChance = fac->Create();

		lChance->chanceNone = (int8_t)100 - CHANCE_INTS[itr];
		lChance->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
		lChance->entries.resize(1);
		lChance->numEntries = 1;
		RE::LEVELED_OBJECT lChanceObj = {0};
		lChanceObj.count = 1;
		lChanceObj.level = 1;

		lChanceObj.form = lAllMods;
		lChance->entries[0] = lChanceObj;

		fullList = lChance;
		++itr;
	}

	return true;
}

float Distributor::ScanLvlList(const RE::TESLevItem* const levItem)
{ // My first art project (:
	auto s = _scannedLists.find(levItem);
	if(s != _scannedLists.end())
		return s->second; // we avoid infinite loops now who could have thought it could cause problems
	else
	{
		float weight = 0.0f;
		auto& list = _scannedLists[levItem];
		list = 0.0f;
		for(size_t i = 0; i < levItem->numEntries; ++i)
		{
			if(levItem->entries[i].form->Is(RE::FormType::LeveledItem))
				weight += ScanLvlList(levItem->entries[i].form->As<RE::TESLevItem>()) * (float)levItem->entries[i].count;
			else if(levItem->entries[i].form->Is(RE::FormType::Armor) && levItem->entries[i].form->As<RE::TESObjectARMO>()->GetArmorType() != RE::BGSBipedObjectForm::ArmorType::kClothing)
				weight += 1.0f * (float)levItem->entries[i].count; // we ignore clothing for now TODO: don't
		}
		// it gets the average amount of armors that can spawn in a lvllist
		list = (weight / (float)levItem->numEntries) * ((float)(100 - levItem->chanceNone) * 0.01f);
		return list;
	}
}

bool Distributor::Distribute()
{
	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	if(settings.verboselog)
	{
		SKSE::log::info("It's chatty time");
		for(size_t moditr = 0; moditr < _lAll[0]->entries[0].form->As<RE::TESLevItem>()->numEntries; ++moditr)
		{
			SKSE::log::info("------ PLUGIN {}/{} ------", moditr + 1, _entries.size());
			const RE::TESLevItem* modlist = _lAll[0]->entries[0].form->As<RE::TESLevItem>();
			size_t catitr = 0;
			for(auto cat : modlist->entries[moditr].form->As<RE::TESLevItem>()->entries)
			{
				auto* catlist = cat.form->As<RE::TESLevItem>();
				SKSE::log::info("\t|\tCATEGORY {} COUNT: {} CHANCENONE: {} LEVEL: {}", catitr, catlist->numEntries, catlist->chanceNone, cat.level);
				for(auto armor : catlist->entries)
				{
					auto armorform = armor.form->As<RE::TESObjectARMO>();
					SKSE::log::info("\t|\t|\tARMOR: {} ID: {:x} LEVEL: {}", armorform->GetName(), armorform->GetFormID(), armor.level);
				}
				++catitr;
			}
		}
	} else
		SKSE::log::info("Set verboselog to true in the ini and check back or throw me a copy if you're having problems");

	for(auto containers = dh->GetFormArray(RE::FormType::Container).begin(); containers != dh->GetFormArray(RE::FormType::Container).end(); ++containers)
	{
		// TODO: we gotta separate the whites and the col- the armor categories
		float weight = 0.0f;
		(*containers)->As<RE::TESContainer>()->ForEachContainerObject([&weight,this](RE::ContainerObject& c)
			{
				if(c.obj->Is(RE::FormType::LeveledItem))
					weight += ScanLvlList(c.obj->As<RE::TESLevItem>()) * (float)c.count;
				else if(c.obj->Is(RE::FormType::Armor))
					weight += 1.0f * (float)c.count;
				return true;
			});
		uint16_t c = std::min<uint16_t>((uint16_t)(weight + 0.99f), settings.maxAdds); // truncates too
		if(c)
		{
			chance ch;
			if(weight < 0.1875f)
				ch = chance::c10;
			else if(weight < 0.3875f)
				ch = chance::c25;
			else if(weight < 0.6325f)
				ch = chance::c50;
			else if(weight < 0.8875f)
				ch = chance::c75;
			else
				ch = chance::c100;
			(*containers)->As<RE::TESContainer>()->AddObjectToContainer(_lAll[ch], c, nullptr);
			if(settings.verboselog)
				SKSE::log::info("CONTAINER: {:x} CHANCE: {}", (*containers)->formID, weight);
		}
	}
	
	return true;
}


