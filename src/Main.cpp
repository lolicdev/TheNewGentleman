#include "Core.h"
#include "Hooks.h"
#include "Papyrus.h"
#include "SEEvents.h"
#include "Util.h"

bool CheckRequirements()
{
	if (!ut->SEDH()->LookupModByName(Common::mainFile)) {
		const char* err = fmt::format("Mod [{}] was not found! Make sure that the mod is active in your plugin load order!", Common::mainFile).c_str();
		ut->ShowSkyrimMessage(err);
		return false;
	}
	if (GetModuleHandleW(L"Data\\SKSE\\Plugins\\acon.dll")) {
		ut->ShowSkyrimMessage("Warning: TNG is not compatible with acon.dll. Please don't use TNG with mods from that website!");
		return false;
	}
	return true;
}

void InitializeLogging()
{
	const auto plugin = SKSE::PluginDeclaration::GetSingleton();
	auto path{ SKSE::log::log_directory() };
	if (!path) {
		SKSE::stl::report_and_fail("Unable to lookup SKSE logs directory.");
	}
	*path /= std::format("{}.log", plugin->GetName());

	std::shared_ptr<spdlog::logger> log;
	log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
	log->set_level(core->GetLogLvl());
	log->flush_on(spdlog::level::trace);
	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
}

void EventListener(SKSE::MessagingInterface::Message* message)
{
	static bool isOk = true;
	if (!isOk)
		return;
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			isOk = CheckRequirements();
			if (!isOk)
				return;
			core->Process();
			events->RegisterEvents();
			Hooks::Install();
			SKSE::log::info("TheNewGentleman finished initialization.");
		}
		break;

	case SKSE::MessagingInterface::kNewGame:
		{
			core->UpdatePlayerAfterLoad();
		}
		break;

	case SKSE::MessagingInterface::kPreLoadGame:
		{
			const std::string savePath{ static_cast<char*>(message->data), message->dataLen };
			core->LoadPlayerInfos(savePath);
		}
		break;

	case SKSE::MessagingInterface::kPostLoadGame:
		{
			core->UpdatePlayerAfterLoad();
		}
		break;

	case SKSE::MessagingInterface::kSaveGame:
		{
			core->SaveMainIni();
		}
		break;

	default:
		break;
	}
}


extern "C" [[maybe_unused]] __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* skse)
{
	const auto plugin = SKSE::PluginDeclaration::GetSingleton();
	InitializeLogging();
	SKSE::Init(skse, false);
	SKSE::log::info("Initializing TheNewGentleman {}!", plugin->GetName());
	SKSE::log::info("Game version : {}", skse->RuntimeVersion().string());
	SKSE::GetMessagingInterface()->RegisterListener(EventListener);
	SKSE::GetPapyrusInterface()->Register(Papyrus::BindPapyrus);
	return true;
}
