#pragma once

#include <RE/T/TESForm.h>

class Distributor
{
public:
	Distributor() { LoadIni(); }
	~Distributor()
	{
		for(char* c : settings.blacklist) delete[] c;
		for(char* c : settings.whitelist) delete[] c;
	};
	bool PopulateLists();
	bool Distribute();

private:
	enum category : size_t
	{
		clothing = 0,
		enchclothing,
		light,
		enchlight,
		heavy,
		enchheavy,
		_count,
		all = _count
	};
	struct ModEntry
	{
		std::vector<RE::TESObjectARMO*> armors[category::_count];
		RE::TESLevItem* lCategories[category::_count]{{nullptr}};
		RE::TESLevItem* lAll = nullptr;
	};

	// shit's recursive yo
	float ScanLvlList(const RE::TESLevItem* const levItem);
	//std::unordered_map<size_t, std::forward_list<const RE::TESObjectARMO*>[category::_count]>& List() { return _lists; }

	std::vector<RE::TESObjectARMO*>& ArmorList(uint16_t modid, category c) { return _entries[modid].armors[c]; }
	uint16_t CalculateLvl(const RE::TESObjectARMO* armor) const;
	//const RE::TESLevItem*& LvlList(uint16_t modid, category c) { return _entries[modid].lvllists[c]; }
	//const RE::TESLevItem*& AllLvlList(uint16_t modid) { return _entries[modid].lvlall; }

	std::unordered_map<uint16_t, ModEntry> _entries;
	enum chance : size_t
	{
		c10 = 0,
		c25,
		c50,
		c75,
		c100,
		_ccount
	};
	//enum level : size_t
	//{
	//	lvl1 = 0,
	//	lvl5,
	//	lvl10,
	//	lvl15,
	//	lvl20,
	//};
	// TODO: enchented items probs consider that huh
	//const RE::TESLevItem* _lAllCategory[chance::_count]{nullptr};
	RE::TESLevItem* _lAll[chance::_ccount]{nullptr};

	// we keep track of lists already scanned (and stop recursive loops in the process)
	std::unordered_map<const RE::TESLevItem*, float> _scannedLists;

	bool LoadIni();
	struct
	{
		std::vector<char*> blacklist;
		std::vector<char*> whitelist;
		int bottomlevel = 0; // higher=earlier armor
		int toplevel = 40; // higher=later armor
		uint16_t maxlevel = 50;
		uint16_t maxAdds = 5;
		bool usingwhitelist = false;
		bool verboselog = false;
		bool debuglog = false;
	} settings;
};