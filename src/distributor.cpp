#include "distributor.h"

#include <RE/I/IFormFactory.h>
#include <SKSE/SKSE.h>

static const char* const ACATEGORY_STRINGS[] = {"Clothing", "EnchClothing", "Light", "EnchLight", "Heavy", "EnchHeavy", "All"};
static const char* const WCATEGORY_STRINGS[] = {"OneHand", "EnchOneHand", "TwoHand", "EnchTwoHand", "Range", "EnchRange", "Other", "EnchOther", "All"};

static const int8_t CHANCE_INTS[] = {10, 25, 50, 75, 100, 100};
static const size_t CATEGORY_MAX = 255ull * 255ull;

static bool SortByLvl(const RE::LEVELED_OBJECT& lhs, const RE::LEVELED_OBJECT& rhs)
{
	return lhs.level < rhs.level;
}

Distributor::Distributor()
{
	SettingsLoader settings{};
	settings.LoadIni();
	settings.LoadSection(asettings, "Armor");
	settings.LoadSection(wsettings, "Weapon");
}

bool Distributor::isValid(const RE::TESObjectARMO* const armor) const
{
	const RE::TESFile* file = armor->GetFile(0);
	if(!armor->GetPlayable() || !(file->compileIndex < 0xFF) || !armor->GetName()[0])
		return false;
	if(asettings.whitelist.empty())
	{
		for(auto f : asettings.blacklist)
		{ // check if origin file is an ignored file
			if(file == f)
			{
				return false;
				break;
			}
		}
		return true;
	} else
	{
		for(auto f : asettings.whitelist)
		{ // check if origin file is whitelisted
			if(file == f)
			{
				return true;
				break;
			}
		}
		return false;
	}
}
bool Distributor::isValid(const RE::TESObjectWEAP* const weapon) const
{
	const RE::TESFile* const file = weapon->GetFile(0);
	if(!weapon->GetPlayable() || !(file->compileIndex < 0xFF) || !weapon->GetName()[0] || weapon->weaponData.flags.all(RE::TESObjectWEAP::Data::Flag::kCantDrop) || weapon->GetSpeed() <= 0.0f)
		return false;
	if(wsettings.whitelist.empty())
	{
		for(auto f : wsettings.blacklist)
		{ // check if origin file is an ignored file
			if(file == f)
				return false;
		}
		return true;
	} else
	{
		for(auto f : wsettings.whitelist)
		{ // check if origin file is whitelisted
			if(file == f)
				return true;
		}
		return false;
	}
}

inline uint16_t interpolate(int v, int X, int Y, int bot, int top, int max)
{
	const float Z = (float)Y - (float)X;
	return (uint16_t)std::clamp<int>((int)((v - X) * ((top - bot) / Z) + bot), 1, max);
	// this math is top notch if you look at it from really far away
}
inline uint16_t interpolate(float v, int X, int Y, int bot, int top, int max)
{ // float overload (: (mostly because I hate warnings and converting all the time)
	const float Z = (float)Y - (float)X;
	return (uint16_t)std::clamp<int>((int)((v - (float)X) * ((top - bot) / Z) + bot), 1, max);
}
uint16_t Distributor::CalculateLvl(const RE::TESObjectARMO* armor) const
{
	RE::BGSBipedObjectForm::ArmorType at = armor->GetArmorType();
	int ar = (int)(armor->armorRating / 100); // rating is 100x for some reason
	if(at == RE::BGSBipedObjectForm::ArmorType::kClothing || ar < 1 || asettings.method == Settings::Gold)
	{
		return interpolate(armor->GetGoldValue(), asettings.botgold, asettings.topgold, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
	} else if(at == RE::BGSBipedObjectForm::ArmorType::kLightArmor)
	{
		if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kBody))
			return interpolate(ar, 20, 41, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHair))
			return interpolate(ar, 10, 17, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kShield))
			return interpolate(ar, 15, 29, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else
			return interpolate(ar, 5, 12, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
	} else if(at == RE::BGSBipedObjectForm::ArmorType::kHeavyArmor)
	{
		if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kBody))
			return interpolate(ar, 25, 49, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHair))
			return interpolate(ar, 15, 23, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kShield))
			return interpolate(ar, 20, 46, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else if(std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(armor->GetSlotMask()) & std::to_underlying<RE::BIPED_MODEL::BipedObjectSlot>(RE::BIPED_MODEL::BipedObjectSlot::kHands))
			return interpolate(ar, 11, 18, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
		else
			return interpolate(ar, 10, 18, asettings.botlevel, asettings.toplevel, asettings.maxlevel);
	}
	return 1;
}
uint16_t Distributor::CalculateLvl(const RE::TESObjectWEAP* weapon) const
{
	if(wsettings.method == Settings::Gold)
		return interpolate(weapon->GetGoldValue(), wsettings.botgold, wsettings.topgold, wsettings.botlevel, wsettings.toplevel, wsettings.maxlevel);

	float dps = ((float)weapon->GetAttackDamage() + ((float)weapon->GetCritDamage() * std::max(weapon->criticalData.prcntMult - 1.0f, 0.0f))) * weapon->GetSpeed();
	switch(weapon->GetWeaponType())
	{
	case RE::WEAPON_TYPE::kHandToHandMelee:
	case RE::WEAPON_TYPE::kOneHandSword:
	case RE::WEAPON_TYPE::kOneHandAxe:
	case RE::WEAPON_TYPE::kOneHandDagger:
	case RE::WEAPON_TYPE::kOneHandMace:
		// one handed dps
		return interpolate(dps, 7, 14, wsettings.botlevel, wsettings.toplevel, wsettings.maxlevel);
	case RE::WEAPON_TYPE::kTwoHandSword:
	case RE::WEAPON_TYPE::kTwoHandAxe:
		// two handed dps
		return interpolate(dps, 10, 16, wsettings.botlevel, wsettings.toplevel, wsettings.maxlevel);
	case RE::WEAPON_TYPE::kCrossbow:
		dps *= 0.7f; // crossbows are wack
	case RE::WEAPON_TYPE::kBow:
		// ranged dps
		return interpolate(dps, 6, 18, wsettings.botlevel, wsettings.toplevel, wsettings.maxlevel);
	case RE::WEAPON_TYPE::kStaff:
	default:
		// use gold
		return interpolate(weapon->GetGoldValue(), wsettings.botgold, wsettings.topgold, wsettings.botlevel, wsettings.toplevel, wsettings.maxlevel);
	}
}

bool Distributor::PopulateArmorLists()
{ // we make all the lvllists here
	if(!asettings.enable)
		return false;

	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	auto& frms = dh->GetFormArray(RE::FormType::Armor);
	if(!frms.size())
		return false;

	for(auto& form : frms)
	{
		if(!isValid(form->As<RE::TESObjectARMO>()))
			continue;

		const RE::TESFile* const file = form->GetFile(0);

		// Categorize all armors and put em in a vector for later
		acategory c = _acount;
		switch(form->As<RE::TESObjectARMO>()->GetArmorType())
		{
		case RE::BGSBipedObjectForm::ArmorType::kClothing:
			c = acategory::clothing;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kLightArmor:
			c = acategory::light;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kHeavyArmor:
			c = acategory::heavy;
			break;
		default:
			SKSE::log::warn("Unknown armor type for: {}", form->GetName());
			break;
		}
		if(form->As<RE::TESObjectARMO>()->formEnchanting && c != _acount) c = (acategory)(c + 1); // idk how to do it better ugh enums
		if(c != _acount) // accessing the unordered map creates an entry if it doesn't exist
			ArmorList(file, c).push_back(form->As<RE::TESObjectARMO>());
	}

	if(_aentries.empty())
		return false;

	auto fac = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESLevItem>();
	if(!fac) return false;

	uint8_t mCount = (uint8_t)std::min<size_t>(_aentries.size(), 255);

	auto* lAllMods = fac->Create();
	lAllMods->chanceNone = 0;
	lAllMods->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
	lAllMods->entries.resize(mCount);
	lAllMods->numEntries = mCount;
	RE::LEVELED_OBJECT lAllModsObj = {0};
	lAllModsObj.count = 1;
	size_t itrAllMods = 0; // for adding mods to full lvllist
	for(auto& [file, entry] : _aentries)
	{
		for(size_t i = 0; i < acategory::_acount; ++i)
		{
			if(entry.armors[i].empty())
				continue; // Skip empty categories
			const size_t size = entry.armors[i].size();

			if(size > CATEGORY_MAX)
				SKSE::log::warn("Too many armors for category {} in mod {} (max {}, total {})", ACATEGORY_STRINGS[i], file->GetFilename(), CATEGORY_MAX, size);

			// total armor in category
			size_t count = std::min<size_t>(size, CATEGORY_MAX);
			// number of required sublists to fit them
			uint8_t sublistcount = (uint8_t)std::min<size_t>(((count + (count / 255) - 1) / 256) + 1, 255); // wowie I suck at math don't look
			// number that is split between each
			auto subcount = [&count, &sublistcount]() { return count / sublistcount + ((count % sublistcount) ? 1 : 0); };

			if(asettings.debuglog)
				SKSE::log::info("PLUGIN {} CALCULATED {} SUBLISTS FOR {} ARMORS SPLIT IN {}", file->GetFilename(), sublistcount, count, subcount());

			auto* lCategory = fac->Create();
			lCategory->chanceNone = 0;
			lCategory->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
			lCategory->entries.resize(sublistcount);
			lCategory->numEntries = sublistcount;

			RE::LEVELED_OBJECT lObj{0};
			lObj.count = 1;

			for(; sublistcount > 0; --sublistcount)
			{
				size_t sc = (uint8_t)subcount();
				auto* lSub = fac->Create();
				lSub->chanceNone = 0;
				lSub->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
				lSub->entries.resize(sc);
				lSub->numEntries = (uint8_t)sc;

				RE::LEVELED_OBJECT subObj{0};
				subObj.count = 1;
				for(size_t itr = 0; itr < sc; ++itr)
				{
					subObj.form = entry.armors[i][count - 1];
					subObj.level = CalculateLvl(entry.armors[i][count - 1]);
					lSub->entries[itr] = subObj;
					--count;
				}
				std::sort(lSub->entries.begin(), lSub->entries.end(), SortByLvl);
				lObj.form = lSub;
				lObj.level = lSub->entries.front().level; // first entry is lowest level since its sorted now

				lCategory->entries[(size_t)sublistcount - 1] = lObj;
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
			lAllObj.level = lvlList->entries.front().level;
			lAll->entries[i++] = lAllObj;
		}
		std::sort(lAll->entries.begin(), lAll->entries.end(), SortByLvl);
		entry.lAll = lAll;

		// add mod to lvllist with all mods
		if(itrAllMods < 255)
		{
			lAllModsObj.form = lAll;
			lAllModsObj.level = lAll->entries.front().level;
			lAllMods->entries[itrAllMods++] = lAllModsObj;
		} else
			SKSE::log::warn("Armor Mod limit reached, ignoring mod {}", file->GetFilename());
	}

	std::sort(lAllMods->entries.begin(), lAllMods->entries.end(), SortByLvl);

	//lets create all the full lvllists for all chances
	size_t itr = 0;
	for(auto& fullList : _alAll)
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

		_alistcache[lChance] = 1.0f * (1.0f - ((float)(lChance->chanceNone) * 0.01f));
		_wlistcache[lChance] = 0.0f;
		_clistcache[lChance] = 0.0f;

		fullList = lChance;
		++itr;
	}

	return true;
}
bool Distributor::PopulateWeaponLists()
{
	if(!wsettings.enable)
		return false;

	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	auto& frms = dh->GetFormArray(RE::FormType::Weapon);
	if(!frms.size())
		return false;

	for(auto& form : frms)
	{
		if(!isValid(form->As<RE::TESObjectWEAP>()))
			continue;

		const RE::TESFile* f = form->GetFile(0);

		wcategory c = _wcount;
		switch(form->As<RE::TESObjectWEAP>()->GetWeaponType())
		{
		case RE::WEAPON_TYPE::kHandToHandMelee:
		case RE::WEAPON_TYPE::kOneHandSword:
		case RE::WEAPON_TYPE::kOneHandAxe:
		case RE::WEAPON_TYPE::kOneHandDagger:
		case RE::WEAPON_TYPE::kOneHandMace:
			c = onehand;
			break;
		case RE::WEAPON_TYPE::kTwoHandSword:
		case RE::WEAPON_TYPE::kTwoHandAxe:
			c = twohand;
			break;
		case RE::WEAPON_TYPE::kBow:
		case RE::WEAPON_TYPE::kCrossbow:
		case RE::WEAPON_TYPE::kStaff:
			c = range;
			break;
		default:
			c = other;
			break;
		}
		if(form->As<RE::TESObjectWEAP>()->formEnchanting) c = (wcategory)(c + 1); // idk how to do it better ugh enums
		if(c < _wcount)
			WeaponList(f, c).push_back(form->As<RE::TESObjectWEAP>());
	}

	if(_wentries.empty())
		return false;

	auto fac = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESLevItem>();
	if(!fac) return false;

	uint8_t mCount = (uint8_t)std::min<size_t>(_wentries.size(), 255);

	auto* lAllMods = fac->Create();
	lAllMods->chanceNone = 0;
	lAllMods->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
	lAllMods->entries.resize(mCount);
	lAllMods->numEntries = mCount;
	RE::LEVELED_OBJECT lAllModsObj = {0};
	lAllModsObj.count = 1;
	size_t itrAllMods = 0; // for adding mods to full lvllist
	for(auto& [file, entry] : _wentries)
	{
		for(size_t i = 0; i < _wcount; ++i)
		{
			if(entry.weapons[i].empty())
				continue; // Skip empty categories
			const size_t size = entry.weapons[i].size();

			if(size > CATEGORY_MAX)
				SKSE::log::warn("Too many armors for category {} in mod {} (max {}, total {})", WCATEGORY_STRINGS[i], file->GetFilename(), CATEGORY_MAX, size);

			// total weapons in category
			size_t count = std::min<size_t>(size, CATEGORY_MAX);
			// number of required sublists to fit them
			uint8_t sublistcount = (uint8_t)std::min<size_t>(((count + (count / 255) - 1) / 256) + 1, 255); // wowie I suck at math don't look
			// number that is split between each
			auto subcount = [&count, &sublistcount]() { return count / sublistcount + ((count % sublistcount) ? 1 : 0); }; // why is this a lambda who knows

			if(wsettings.debuglog)
				SKSE::log::info("PLUGIN {} CALCULATED {} SUBLISTS FOR {} WEAPONS SPLIT IN {}", file->GetFilename(), sublistcount, count, subcount());

			auto* lCategory = fac->Create();
			lCategory->chanceNone = 0;
			lCategory->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
			lCategory->entries.resize(sublistcount);
			lCategory->numEntries = sublistcount;

			RE::LEVELED_OBJECT lObj{0};
			lObj.count = 1;

			for(; sublistcount > 0; --sublistcount)
			{
				size_t sc = (uint8_t)subcount();
				auto* lSub = fac->Create();
				lSub->chanceNone = 0;
				lSub->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
				lSub->entries.resize(sc);
				lSub->numEntries = (uint8_t)sc;

				RE::LEVELED_OBJECT subObj{0};
				subObj.count = 1;
				for(size_t itr = 0; itr < sc; ++itr)
				{
					subObj.form = entry.weapons[i][count - 1];
					subObj.level = CalculateLvl(entry.weapons[i][count - 1]);
					lSub->entries[itr] = subObj;
					--count;
				}
				std::sort(lSub->entries.begin(), lSub->entries.end(), SortByLvl);
				lObj.form = lSub;
				lObj.level = lSub->entries.front().level; // first entry is lowest level since its sorted now

				lCategory->entries[(size_t)sublistcount - 1] = lObj;
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
			lAllObj.level = lvlList->entries.front().level;
			lAll->entries[i++] = lAllObj;
		}
		std::sort(lAll->entries.begin(), lAll->entries.end(), SortByLvl);
		entry.lAll = lAll;

		// add mod to lvllist with all mods
		if(itrAllMods < 255)
		{
			lAllModsObj.form = lAll;
			lAllModsObj.level = lAll->entries.front().level;
			lAllMods->entries[itrAllMods++] = lAllModsObj;
		} else
			SKSE::log::warn("Weapon Mod limit reached, ignoring mod {}", file->GetFilename());
	}

	std::sort(lAllMods->entries.begin(), lAllMods->entries.end(), SortByLvl);

	//lets create all the full lvllists for all chances
	size_t itr = 0;
	for(auto& fullList : _wlAll)
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

		_alistcache[lChance] = 0.0f;
		_wlistcache[lChance] = 1.0f * (1.0f - ((float)(lChance->chanceNone) * 0.01f));
		_clistcache[lChance] = 0.0f;

		fullList = lChance;
		++itr;
	}
	return true;
}

float Distributor::ScanArmorChance(const RE::TESLevItem* const levItem)
{ // My first art project (:
	auto s = _alistcache.find(levItem);
	if(s != _alistcache.end())
		return s->second; // we avoid infinite loops now who could have thought it could cause problems
	else
	{
		if(levItem->numEntries == 0)
			return 0.0f; // just in case the list is empty for some reason, we don't want to divide by 0
		float weight = 0.0f;
		auto& list = _alistcache[levItem];
		list = 0.0f;
		for(size_t i = 0; i < levItem->numEntries; ++i)
		{
			if(levItem->entries[i].form->Is(RE::FormType::LeveledItem))
				weight += ScanArmorChance(levItem->entries[i].form->As<RE::TESLevItem>()) * (float)levItem->entries[i].count;
			else if(levItem->entries[i].form->Is(RE::FormType::Armor) && levItem->entries[i].form->As<RE::TESObjectARMO>()->GetArmorType() != RE::BGSBipedObjectForm::ArmorType::kClothing)
				weight += 1.0f * (float)levItem->entries[i].count; // we ignore clothing for now TODO: don't
		}
		// it gets the average amount of armors that can spawn in a lvllist
		list = (weight / (float)levItem->numEntries) * ((100.0f - (float)levItem->chanceNone) * 0.01f);
		return list;
	}
}
float Distributor::ScanWeaponChance(const RE::TESLevItem* const levItem)
{ // My second art project (:
	auto s = _wlistcache.find(levItem);
	if(s != _wlistcache.end())
		return s->second;
	else
	{
		if(levItem->numEntries == 0)
			return 0.0f; // just in case the list is empty for some reason, we don't want to divide by 0
		float weight = 0.0f;
		auto& list = _wlistcache[levItem];
		list = 0.0f;
		for(size_t i = 0; i < levItem->numEntries; ++i)
		{
			if(levItem->entries[i].form->Is(RE::FormType::LeveledItem))
				weight += ScanWeaponChance(levItem->entries[i].form->As<RE::TESLevItem>()) * (float)levItem->entries[i].count;
			else if(levItem->entries[i].form->Is(RE::FormType::Weapon))
				weight += 1.0f * (float)levItem->entries[i].count;
		}
		// it gets the average amount of armors that can spawn in a lvllist
		list = (weight / (float)levItem->numEntries) * ((100.0f - (float)levItem->chanceNone) * 0.01f);
		return list;
	}
}
float Distributor::ScanClothesChance(const RE::TESLevItem* const levItem)
{ // My third art project o:
	auto s = _clistcache.find(levItem);
	if(s != _clistcache.end())
		return s->second;
	else
	{
		if(levItem->numEntries == 0)
			return 0.0f;
		float weight = 0.0f;
		auto& list = _clistcache[levItem];
		list = 0.0f;
		for(size_t i = 0; i < levItem->numEntries; ++i)
		{
			if(levItem->entries[i].form->Is(RE::FormType::LeveledItem))
				weight += ScanClothesChance(levItem->entries[i].form->As<RE::TESLevItem>()) * (float)levItem->entries[i].count;
			else if(levItem->entries[i].form->Is(RE::FormType::Armor)
				&& levItem->entries[i].form->As<RE::TESObjectARMO>()->GetArmorType() == RE::BGSBipedObjectForm::ArmorType::kClothing
				&& levItem->entries[i].form->GetGoldValue() <= asettings.maxcloset)
				weight += 1.0f * (float)levItem->entries[i].count;
		}
		list = (weight / (float)levItem->numEntries) * ((100.0f - (float)levItem->chanceNone) * 0.01f);
		return list;
	}
}

bool Distributor::DistributeArmors()
{
	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	if(asettings.debuglog)
	{
		for(size_t moditr = 0; moditr < _alAll[0]->entries[0].form->As<RE::TESLevItem>()->numEntries; ++moditr)
		{
			SKSE::log::info("--- PLUGIN {}/{} ---", moditr + 1, _aentries.size());
			const RE::TESLevItem* modlist = _alAll[0]->entries[0].form->As<RE::TESLevItem>();
			size_t catitr = 0;
			for(auto cat : modlist->entries[moditr].form->As<RE::TESLevItem>()->entries)
			{
				const auto* catlist = cat.form->As<RE::TESLevItem>();
				SKSE::log::info("\t|\tCATEGORY {} SUBLIST COUNT: {} LEVEL: {}", catitr, catlist->numEntries, cat.level);
				size_t subi = 0;
				for(auto& sublist : catlist->entries)
				{
					SKSE::log::info("\t|\t|\tSUBLIST {} COUNT: {} LEVEL: {}", subi, sublist.form->As<RE::TESLevItem>()->numEntries, sublist.level);
					for(auto& armor : sublist.form->As<RE::TESLevItem>()->entries)
					{
						const auto armorform = armor.form->As<RE::TESObjectARMO>();
						SKSE::log::info("\t|\t|\t|\tARMOR: {} ID: {:x} LEVEL: {}", armorform->GetName(), armorform->GetFormID(), armor.level);
					}
					++subi;
				}
				++catitr;
			}
		}
	} else if(asettings.verboselog)
	{
		size_t moditr = 1;
		for(auto& [file, entry] : _aentries)
		{
			SKSE::log::info("\tPLUGIN {}:{:x} {}/{} ------",
				file->GetFilename(), ((file->GetCompileIndex() != 254) ? file->GetCompileIndex() : 0) + file->GetSmallFileCompileIndex(), moditr, _aentries.size());
			size_t catitr = 0;
			for(auto& cat : entry.lCategories)
			{
				if(!cat)
				{
					SKSE::log::info("\t|\tUNUSED CATEGORY: {}", ACATEGORY_STRINGS[&cat - entry.lCategories]);
					continue;
				}
				size_t count = 0;
				for(size_t i = 0; i < cat->As<RE::TESLevItem>()->numEntries; ++i)
					count += cat->As<RE::TESLevItem>()->entries[i].form->As<RE::TESLevItem>()->numEntries;

				SKSE::log::info("\t|\tCATEGORY: {} COUNT: {} MINLVL: {}", ACATEGORY_STRINGS[&cat - entry.lCategories], count, cat->entries.front().level);
				for(auto& sublist : cat->As<RE::TESLevItem>()->entries)
				{
					for(auto& armor : sublist.form->As<RE::TESLevItem>()->entries)
					{
						const auto armorform = armor.form->As<RE::TESObjectARMO>();
						SKSE::log::info("\t|\t|\tARMOR: {} ID: {:x} LEVEL: {}", armorform->GetName(), armorform->GetFormID(), armor.level);
					}
				}
				++catitr;
			}
			++moditr;
		}
	}

	RE::TESLevItem* closetclotheslists[_ccount]{0};
	if(asettings.maxcloset > 0)
	{
		auto fac = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESLevItem>();
		if(!fac) return false;

		std::vector<const RE::LEVELED_OBJECT*> closetclothes;
		for(auto& [mod, entry] : _aentries)
		{
			if(entry.lCategories[clothing])
			{
				for(auto& sublist : entry.lCategories[clothing]->entries)
				{
					for(auto& obj : sublist.form->As<RE::TESLevItem>()->entries)
					{
						if(obj.form->GetGoldValue() <= asettings.maxcloset)
							closetclothes.push_back(&obj);
						else // the list is sorted by gold value so we can stop iterating after it evaluates false
							break;
					}
				}
			}
		}
		if(!closetclothes.empty())
		{
			const size_t size = closetclothes.size();

			// total armor in category
			size_t count = std::min<size_t>(size, CATEGORY_MAX);
			// number of required sublists to fit them
			uint8_t sublistcount = (uint8_t)std::min<size_t>(((count + (count / 255) - 1) / 256) + 1, 255); // wowie I suck at math don't look
			// number that is split between each
			auto subcount = [&count, &sublistcount]() { return count / sublistcount + ((count % sublistcount) ? 1 : 0); };

			if(asettings.debuglog)
				SKSE::log::info("CALCULATED {} SUBLISTS FOR {} ARMORS SPLIT IN {} FOR CLOSET CLOTHES", sublistcount, count, subcount());

			RE::TESLevItem* closetclotheslist = fac->Create();
			closetclotheslist->chanceNone = 0;
			closetclotheslist->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
			closetclotheslist->entries.resize(sublistcount);
			closetclotheslist->numEntries = sublistcount;

			RE::LEVELED_OBJECT lObj{0};
			lObj.count = 1;

			for(; sublistcount > 0; --sublistcount)
			{
				uint8_t sc = (uint8_t)subcount();
				auto* lSub = fac->Create();
				lSub->chanceNone = 0;
				lSub->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
				lSub->entries.resize(sc);
				lSub->numEntries = sc;
				for(size_t itr = 0; itr < sc; ++itr)
				{
					lSub->entries[itr] = *(closetclothes[count - 1]);
					--count;
				}
				std::sort(lSub->entries.begin(), lSub->entries.end(), SortByLvl);
				lObj.form = lSub;
				lObj.level = lSub->entries[0].level;
				closetclotheslist->entries[(size_t)sublistcount - 1] = lObj;
			}
			std::sort(closetclotheslist->entries.begin(), closetclotheslist->entries.end(), SortByLvl);
			size_t i = 0; 
			for(RE::TESLevItem*& l : closetclotheslists)
			{
				l = fac->Create();
				l->chanceNone = 100 - CHANCE_INTS[i];
				l->llFlags = (RE::TESLeveledList::Flag)(RE::TESLeveledList::Flag::kCalculateFromAllLevelsLTOrEqPCLevel | RE::TESLeveledList::Flag::kCalculateForEachItemInCount);
				l->entries.resize(1);
				l->numEntries = 1;
				l->entries[0] = RE::LEVELED_OBJECT{.form = closetclotheslist, .count = 1, .level = closetclotheslist->entries[0].level};
				_alistcache[l] = 0.0f;
				_wlistcache[l] = 0.0f;
				_clistcache[l] = 1.0f * (1.0f - ((float)(l->chanceNone) * 0.01f));
				++i;
			}
			if(asettings.debuglog)
			{
				SKSE::log::info("--- CLOSET LIST --- SUBLIST COUNT: {} LEVEL: {}", closetclotheslist->numEntries, closetclotheslists[0]->entries[0].level);
				size_t subi = 0;
				for(auto& sublist : closetclotheslist->entries)
				{
					SKSE::log::info("\t|\t|\tSUBLIST {} COUNT: {} LEVEL: {}", subi, sublist.form->As<RE::TESLevItem>()->numEntries, sublist.level);
					for(auto& armor : sublist.form->As<RE::TESLevItem>()->entries)
					{
						const auto armorform = armor.form->As<RE::TESObjectARMO>();
						SKSE::log::info("\t|\t|\t|\tCLOTHING: {} ID: {:x} LEVEL: {}", armorform->GetName(), armorform->GetFormID(), armor.level);
					}
					++subi;
				}
			}
		}
	}

	for(auto containers = dh->GetFormArray(RE::FormType::Container).begin(); containers != dh->GetFormArray(RE::FormType::Container).end(); ++containers)
	{
		// TODO: we gotta separate the whites and the col- the armor categories
		float weight = 0.0f;
		bool closet = false;
		(*containers)->As<RE::TESContainer>()->ForEachContainerObject([&weight, this](RE::ContainerObject& c)
			{
				if(c.obj->Is(RE::FormType::LeveledItem))
					weight += ScanArmorChance(c.obj->As<RE::TESLevItem>()) * (float)c.count;
				else if(c.obj->Is(RE::FormType::Armor))
					weight += 1.0f * (float)c.count;
				return true;
			});
		uint16_t c = std::min<uint16_t>((uint16_t)(weight + 0.98f), asettings.maxadds); // truncates too
		if(!c && closetclotheslists[0])
		{
			(*containers)->As<RE::TESContainer>()->ForEachContainerObject([&weight, this](RE::ContainerObject& c)
				{
					if(c.obj->Is(RE::FormType::LeveledItem))
						weight += ScanClothesChance(c.obj->As<RE::TESLevItem>()) * (float)c.count;
					else if(c.obj->Is(RE::FormType::Armor))
						weight += 1.0f * (float)c.count;
					return true;
				});
			c = std::min<uint16_t>((uint16_t)(weight + 0.98f), asettings.maxclosetadds);
			closet = true;
		}
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
			(*containers)->As<RE::TESContainer>()->AddObjectToContainer((closet) ? closetclotheslists[ch] : _alAll[ch], c, nullptr);
			if(asettings.debuglog)
				SKSE::log::info("({}) CONTAINER: {:x} CHANCE: {}", (closet) ? "CLOTHING" : "ARMOR", (*containers)->formID, weight);
		}
	}

	return true;
}
bool Distributor::DistributeWeapons()
{
	RE::TESDataHandler* const dh = RE::TESDataHandler::GetSingleton();

	if(wsettings.debuglog)
	{
		for(size_t moditr = 0; moditr < _wlAll[0]->entries[0].form->As<RE::TESLevItem>()->numEntries; ++moditr)
		{
			SKSE::log::info("--- PLUGIN {}/{} ---", moditr + 1, _wentries.size());
			const RE::TESLevItem* modlist = _wlAll[0]->entries[0].form->As<RE::TESLevItem>();
			size_t catitr = 0;
			for(auto cat : modlist->entries[moditr].form->As<RE::TESLevItem>()->entries)
			{
				const auto* catlist = cat.form->As<RE::TESLevItem>();
				SKSE::log::info("\t|\tCATEGORY {} SUBLIST COUNT: {} LEVEL: {}", catitr, catlist->numEntries, cat.level);
				size_t subi = 0;
				for(auto& sublist : catlist->entries)
				{
					SKSE::log::info("\t|\t|\tSUBLIST {} COUNT: {} LEVEL: {}", subi, sublist.form->As<RE::TESLevItem>()->numEntries, sublist.level);
					for(auto& weapon : sublist.form->As<RE::TESLevItem>()->entries)
					{
						const auto weaponform = weapon.form->As<RE::TESObjectWEAP>();
						SKSE::log::info("\t|\t|\t|\tWEAPON: {} ID: {:x} LEVEL: {}", weaponform->GetName(), weaponform->GetFormID(), weapon.level);
					}
					++subi;
				}
				++catitr;
			}
		}
	} else if(wsettings.verboselog)
	{
		size_t moditr = 1;
		for(auto& [file, entry] : _wentries)
		{
			SKSE::log::info("\tPLUGIN {}:{:x} {}/{} ------",
				file->GetFilename(), ((file->GetCompileIndex() != 254) ? file->GetCompileIndex() : 0) + file->GetSmallFileCompileIndex(), moditr, _wentries.size());
			size_t catitr = 0;
			for(auto& cat : entry.lCategories)
			{
				if(!cat)
				{
					SKSE::log::info("\t|\tUNUSED CATEGORY: {}", WCATEGORY_STRINGS[&cat - entry.lCategories]);
					continue;
				}
				size_t count = 0;
				for(size_t i = 0; i < cat->As<RE::TESLevItem>()->numEntries; ++i)
					count += cat->As<RE::TESLevItem>()->entries[i].form->As<RE::TESLevItem>()->numEntries;

				SKSE::log::info("\t|\tCATEGORY: {} COUNT: {} MINLVL: {}", WCATEGORY_STRINGS[&cat - entry.lCategories], count, cat->entries.front().level);
				for(auto& sublist : cat->As<RE::TESLevItem>()->entries)
				{
					for(auto& weapon : sublist.form->As<RE::TESLevItem>()->entries)
					{
						const auto weaponform = weapon.form->As<RE::TESObjectWEAP>();
						SKSE::log::info("\t|\t|\tWEAPON: {} ID: {:x} LEVEL: {}", weaponform->GetName(), weaponform->GetFormID(), weapon.level);
					}
				}
				++catitr;
			}
			++moditr;
		}
	}

	for(auto containers = dh->GetFormArray(RE::FormType::Container).begin(); containers != dh->GetFormArray(RE::FormType::Container).end(); ++containers)
	{
		float weight = 0.0f;
		(*containers)->As<RE::TESContainer>()->ForEachContainerObject([&weight, this](RE::ContainerObject& c)
			{
				if(c.obj->Is(RE::FormType::LeveledItem))
					weight += ScanWeaponChance(c.obj->As<RE::TESLevItem>()) * (float)c.count;
				else if(c.obj->Is(RE::FormType::Weapon))
					weight += 1.0f * (float)c.count;
				return true;
			});
		uint16_t c = std::min<uint16_t>((uint16_t)(weight + 0.99f), wsettings.maxadds); // truncates too
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
			(*containers)->As<RE::TESContainer>()->AddObjectToContainer(_wlAll[ch], c, nullptr);
			if(wsettings.debuglog)
				SKSE::log::info("(WEAPON) CONTAINER: {:x} CHANCE: {}", (*containers)->formID, weight);
		}
	}

	return true;
}

