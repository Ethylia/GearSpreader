#pragma once

#include <RE/T/TESForm.h>

#include "settings.h"

class Distributor
{
public:
	Distributor();
	bool PopulateArmorLists();
	bool DistributeArmors();
	bool PopulateWeaponLists();
	bool DistributeWeapons();

private:
	enum acategory : size_t
	{
		clothing = 0,
		enchclothing,
		light,
		enchlight,
		heavy,
		enchheavy,
		_acount,
		aall = _acount
	};
	enum wcategory : size_t
	{
		onehand = 0,
		enchonehand,
		twohand,
		enchtwohand,
		range,
		enchrange,
		other,
		enchother,
		_wcount,
		wall = _wcount
	};
	struct AModEntry
	{
		std::vector<RE::TESObjectARMO*> armors[_acount];
		RE::TESLevItem* lCategories[acategory::_acount]{{nullptr}};
		RE::TESLevItem* lAll = nullptr;
	};
	struct WModEntry
	{
		std::vector<RE::TESObjectWEAP*> weapons[_wcount];
		RE::TESLevItem* lCategories[_wcount]{{nullptr}};
		RE::TESLevItem* lAll = nullptr;
	};

	// shit's recursive yo
	float ScanArmorChance(const RE::TESLevItem* const levItem);
	float ScanWeaponChance(const RE::TESLevItem* const levItem);
	float ScanClothesChance(const RE::TESLevItem* const levItem);
	//std::unordered_map<size_t, std::forward_list<const RE::TESObjectARMO*>[category::_count]>& List() { return _lists; }

	std::vector<RE::TESObjectARMO*>& ArmorList(const RE::TESFile* f, acategory c) { return _aentries[f].armors[c]; }
	std::vector<RE::TESObjectWEAP*>& WeaponList(const RE::TESFile* f, wcategory c) { return _wentries[f].weapons[c]; }
	uint16_t CalculateLvl(const RE::TESObjectARMO* const armor) const;
	uint16_t CalculateLvl(const RE::TESObjectWEAP* const weapon) const;
	bool isValid(const RE::TESObjectARMO* const armor) const;
	bool isValid(const RE::TESObjectWEAP* const weapon) const;
	//const RE::TESLevItem*& LvlList(uint16_t modid, category c) { return _entries[modid].lvllists[c]; }
	//const RE::TESLevItem*& AllLvlList(uint16_t modid) { return _entries[modid].lvlall; }

	std::unordered_map<const RE::TESFile*, AModEntry> _aentries;
	std::unordered_map<const RE::TESFile*, WModEntry> _wentries;
	enum chance : size_t
	{
		c10 = 0,
		c25,
		c50,
		c75,
		c100,
		_ccount
	};
	RE::TESLevItem* _alAll[chance::_ccount]{nullptr};
	RE::TESLevItem* _wlAll[chance::_ccount]{nullptr};

	// we keep a cache of lists already scanned (and stop recursive loops in the process)
	std::unordered_map<const RE::TESLevItem*, float> _alistcache;
	std::unordered_map<const RE::TESLevItem*, float> _wlistcache;
	std::unordered_map<const RE::TESLevItem*, float> _clistcache;

	Settings asettings;
	Settings wsettings;
};