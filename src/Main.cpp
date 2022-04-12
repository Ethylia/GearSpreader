#include "Config.h"

#include "distributor.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

/**
 * Declaration of the plugin metadata.
 *
 * <p>
 * Modern versions of SKSE look for this static data in the DLL. It is required to have a plugin version and runtime
 * compatibility information. You should usually specify your runtime compatibility with <code>UsesAddressLibrary</code>
 * to be version independent. If you don't use Address Library you can specify specific Skyrim versions that are
 * supported.
 * </p>
 */
EXTERN_C [[maybe_unused]] SAMPLE_EXPORT constinit auto SKSEPlugin_Version = []() noexcept
{
	SKSE::PluginVersionData v;
	v.PluginName("GearSpreader");
	v.PluginVersion({0, 2, 0, 0});
	v.UsesAddressLibrary(true);
	return v;
}();

/**
 * Callback used by SKSE for Skyrim runtime versions 1.5.x to detect if a DLL is an SKSE plugin.
 *
 * <p>
 * This function should set the plugin information in the plugin info and return true. For post-AE executables it is
 * never called. This implementation sets up the same information as is defined in <code>SKSEPlugin_Version</code>,
 * allowing you to still control all settings via the newer AE-compatible method while remaining backwards-compatible
 * with pre-AE SSE.
 * </p>
 *
 * <p>
 * For modern SKSE development I encourage leaving this function implemented exactly as is; in the past it was not
 * uncommon to implement functionality in this function such as initializing logging, but such logic is not compatible
 * with the post-AE initialization system. To be cross-compatible, all such logic should be in
 * <code>SKSEPlugin_Load</code>.
 * </p>
 */
EXTERN_C [[maybe_unused]] SAMPLE_EXPORT bool SKSEAPI SKSEPlugin_Query(const QueryInterface*, PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

namespace
{
	/**
	 * Setup logging.
	 *
	 * <p>
	 * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
	 * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
	 * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
	 * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
	 * (or <code>Skyrim VR</code> if you are using VR).
	 * </p>
	 */
	void InitializeLogging()
	{
		auto path = log_directory();
		if(!path)
		{
			report_and_fail("Unable to lookup SKSE logs directory.");
		}
		*path /= SKSEPlugin_Version.pluginName;
		*path += L".log";

		std::shared_ptr<spdlog::logger> log;
		if(IsDebuggerPresent())
		{
			log = std::make_shared<spdlog::logger>(
				"Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
		} else
		{
			log = std::make_shared<spdlog::logger>(
				"Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		}
		const auto& debugConfig = GearSpreader::Config::GetSingleton().GetDebug();
		log->set_level(debugConfig.GetLogLevel());
		log->flush_on(debugConfig.GetFlushLevel());

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
	}
	void InitializeMessaging()
	{
		if(!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message)
			{
				if(message->type == MessagingInterface::kDataLoaded)
				{
					// All ESM/ESL/ESP plugins have loaded, main menu is now active.
					// It is now safe to access form data.
					Distributor distributor;
					if(distributor.PopulateLists())
						distributor.Distribute();
				}
			}))
		{
			stl::report_and_fail("Unable to register message listener.");
		}
	}
}

/**
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
EXTERN_C [[maybe_unused]] SAMPLE_EXPORT bool SKSEAPI SKSEPlugin_Load(const LoadInterface* skse)
{
	InitializeLogging();
	log::info("{} is loading...", SKSEPlugin_Version.pluginName);
	Init(skse);
	InitializeMessaging();

	log::info("{} has finished loading.", SKSEPlugin_Version.pluginName);
	return true;
}
