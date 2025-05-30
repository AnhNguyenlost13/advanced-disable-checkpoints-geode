#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/GameOptionsLayer.hpp>

int checkpointCount = 0;
int notificationCount = 0;

$execute {
	listenForSettingChanges("mode", [](const std::string& mode)
	{
		if (mode == "Disabled") Mod::get()->setSavedValue<bool>("meow", false);
		else Mod::get()->setSavedValue<bool>("meow", true);

		Mod::get()->setSavedValue<bool>("time", mode.contains("Time"));
		Mod::get()->setSavedValue<bool>("checkpoint", mode.contains("Checkpoint"));
	});
}

#include <Geode/modify/PlayLayer.hpp>
class $modify(PL, PlayLayer)
{
	// ReSharper disable once CppHidingFunction
	$override void onQuit()
	{
		checkpointCount = 0; notificationCount = 0;
		PlayLayer::onQuit();
	}

	$override void checkpointActivated(CheckpointGameObject* checkpoint) override
	{
		PlayLayer::checkpointActivated(checkpoint);
		checkpointCount++;
	};

	$override void destroyPlayer(PlayerObject* player, GameObject* obj) override
	{
		PlayLayer::destroyPlayer(player, obj); if (obj == m_anticheatSpike) return;

		bool doDisableCheckpoints = false;

		const bool timeConditionUnsatisfied = Mod::get()->getSettingValue<std::string>("mode").contains("Time") && Mod::get()->getSettingValue<float>("time") > m_gameState.m_levelTime;
		const bool checkpointConditionUnsatisfied = Mod::get()->getSettingValue<std::string>("mode").contains("Checkpoint") && checkpointCount < Mod::get()->getSettingValue<int>("checkpoint-count");

		if (Mod::get()->getSettingValue<std::string>("mode").contains("&")) doDisableCheckpoints = timeConditionUnsatisfied && checkpointConditionUnsatisfied;
		else if (Mod::get()->getSettingValue<std::string>("mode").contains("|")) doDisableCheckpoints = timeConditionUnsatisfied || checkpointConditionUnsatisfied;

		if (doDisableCheckpoints)
		{
			log::info("doDisableCheckpoints!");
			GameManager::get()->setGameVariable("0046", doDisableCheckpoints);
			removeAllCheckpoints(); // Purge all checkpoints if minimum checkpoint count is not reached
		}
		checkpointCount = 0;
	}

	// i'm honestly sick of living this life
	$override void updateTimeLabel(const int seconds, const int milliseconds, const bool useAlternateFormatting) override
	{
		PlayLayer::updateTimeLabel(seconds, milliseconds, useAlternateFormatting);

		// break out early if notification functionality is disabled/level is not platformer
		if (!m_level->isPlatformer() || !Mod::get()->getSettingValue<bool>("notification") || Mod::get()->getSettingValue<std::string>("mode") == "Disabled" || notificationCount > 0) return;
		notificationCount++;

		// build string
		std::vector<std::string> shards;
		if (Mod::get()->getSettingValue<std::string>("mode").contains("Time")) shards.push_back(fmt::format("{} seconds", Mod::get()->getSettingValue<float>("time")));
		std::string comparator;
		if (Mod::get()->getSettingValue<std::string>("mode").contains("&")) comparator = "and";
		else if (Mod::get()->getSettingValue<std::string>("mode").contains("/")) comparator = "or";

		int checkpointCount = Mod::get()->getSettingValue<int>("checkpoint-count");
		if (Mod::get()->getSettingValue<std::string>("mode").contains("Checkpoint")) {
			shards.push_back(fmt::format("{} checkpoints", // there has to be at least 2 checkpoints anyway
				checkpointCount
			));
		}

		if (!shards.empty())
		{
			std::string notification = "Checkpoints enabled after ";

			if (shards.size() == 1)
				notification += shards[0];
			else if (shards.size() == 2)
				notification += fmt::format("{} {} {}", shards[0], comparator, shards[1]);
			else
			{
				for (size_t i = 0; i < shards.size() - 1; i++) notification += fmt::format("{}, ", shards[i]);
				notification += fmt::format("{} {}", comparator, shards.back());
			}
			Notification* x = Notification::create(notification, NotificationIcon::Info, Mod::get()->getSettingValue<float>("notification-time"));
			x->show();
		}
	};
};
