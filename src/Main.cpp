#include "Logging.h"

#include "distributor.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();
    const auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);
    Init(skse);

	 if(!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message)
		 {
			 if(message->type == MessagingInterface::kDataLoaded)
			 {
				 // All ESM/ESL/ESP plugins have loaded, main menu is now active.
				 // It is now safe to access form data.
				 Distributor distributor;
				 if(distributor.PopulateArmorLists())
					 distributor.DistributeArmors();
				 if(distributor.PopulateWeaponLists())
					 distributor.DistributeWeapons();
			 }
		 }))
	 {
		 stl::report_and_fail("Unable to register message listener.");
	 }

    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
