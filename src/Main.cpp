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
			logger::info("TheNewGentleman finished initialization.");
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

  const auto InitLogger = [&plugin]() -> bool {
    auto path = logger::log_directory();
    if (!path)
      return false;
    *path /= std::format("{}.log", plugin->GetName());
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

    log->set_level(core->GetLogLvl());
    log->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");

    logger::info("Initializing {} v{}", plugin->GetName(), plugin->GetVersion());
    return true;
  };

	SKSE::Init(skse, false);
	logger::info("Game version : {}", skse->RuntimeVersion().string());
	SKSE::GetMessagingInterface()->RegisterListener(EventListener);
	SKSE::GetPapyrusInterface()->Register(Papyrus::BindPapyrus);
  
	return true;
}
