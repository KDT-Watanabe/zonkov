#include "QuestSystem.h"
#include "System/UI/FeedbackSystem.h"
#include <random>

void QuestSystem::Initialize()
{
	if (isInitialized_) return;
	isInitialized_ = true;
	Clear();
}

void QuestSystem::Clear()
{
	activeQuests_.clear();
}

void QuestSystem::GenerateRaidQuests()
{
	Clear();

	// 毎回固定のクエストセットを生成（後でランダム化可能）

	// メインクエスト: 脱出
	Quest escapeQuest;
	escapeQuest.id = 1;
	escapeQuest.name = "ゾーンからの脱出";
	escapeQuest.description = "出口を見つけて生還せよ";
	escapeQuest.objectives.emplace_back(QuestObjectiveType::Escape, "出口に到達", 1);
	escapeQuest.reward.money = 500;
	escapeQuest.isActive = true;
	activeQuests_.push_back(escapeQuest);

	// サブクエスト1: 敵を倒す
	Quest killQuest;
	killQuest.id = 2;
	killQuest.name = "エリア掃討";
	killQuest.description = "敵の脅威を排除せよ";
	killQuest.objectives.emplace_back(QuestObjectiveType::KillEnemies, "敵を5体倒す", 5);
	killQuest.reward.money = 200;
	killQuest.reward.items.emplace_back(1, 3);  // 小回復薬 x3
	killQuest.isActive = true;
	activeQuests_.push_back(killQuest);

	// サブクエスト2: 貴重品を集める
	Quest collectQuest;
	collectQuest.id = 3;
	collectQuest.name = "トレジャーハンター";
	collectQuest.description = "貴重品を収集せよ";
	collectQuest.objectives.emplace_back(QuestObjectiveType::CollectItem, "金の指輪を3個集める", 3, 40);
	collectQuest.reward.money = 300;
	collectQuest.isActive = true;
	activeQuests_.push_back(collectQuest);

	// サブクエスト3: 武器を入手
	Quest weaponQuest;
	weaponQuest.id = 4;
	weaponQuest.name = "武装準備";
	weaponQuest.description = "武器を入手せよ";
	weaponQuest.objectives.emplace_back(QuestObjectiveType::CollectItem, "剣を入手", 1, 10);  // ショートソード
	QuestObjective altWeapon(QuestObjectiveType::CollectItem, "または剣を入手", 1, 11);  // ロングソード
	weaponQuest.objectives.push_back(altWeapon);
	weaponQuest.reward.money = 100;
	weaponQuest.isActive = true;
	activeQuests_.push_back(weaponQuest);

	FeedbackSystem::GetInstance().ShowNotification("新しいクエストが発生!", NotificationType::QuestUpdate);
}

void QuestSystem::OnEnemyKilled()
{
	for (auto& quest : activeQuests_)
	{
		if (!quest.isActive || quest.isCompleted) continue;

		for (auto& obj : quest.objectives)
		{
			if (obj.type == QuestObjectiveType::KillEnemies && !obj.isCompleted)
			{
				obj.AddProgress(1);

				if (obj.isCompleted)
				{
					FeedbackSystem::GetInstance().ShowNotification(
						"目標達成: " + obj.description,
						NotificationType::QuestUpdate
					);
				}
			}
		}
	}

	CheckQuestCompletion();
}

void QuestSystem::OnItemCollected(int itemId, int count)
{
	for (auto& quest : activeQuests_)
	{
		if (!quest.isActive || quest.isCompleted) continue;

		for (auto& obj : quest.objectives)
		{
			if (obj.type == QuestObjectiveType::CollectItem && !obj.isCompleted)
			{
				if (obj.targetId == itemId || obj.targetId == 0)
				{
					obj.AddProgress(count);

					if (obj.isCompleted)
					{
						FeedbackSystem::GetInstance().ShowNotification(
							"目標達成: " + obj.description,
							NotificationType::QuestUpdate
						);
					}
				}
			}
		}
	}

	CheckQuestCompletion();
}

void QuestSystem::OnAreaReached(int areaId)
{
	for (auto& quest : activeQuests_)
	{
		if (!quest.isActive || quest.isCompleted) continue;

		for (auto& obj : quest.objectives)
		{
			if (obj.type == QuestObjectiveType::ExploreArea && !obj.isCompleted)
			{
				if (obj.targetId == areaId || obj.targetId == 0)
				{
					obj.AddProgress(1);

					if (obj.isCompleted)
					{
						FeedbackSystem::GetInstance().ShowNotification(
							"目標達成: " + obj.description,
							NotificationType::QuestUpdate
						);
					}
				}
			}
		}
	}

	CheckQuestCompletion();
}

void QuestSystem::OnEscaped()
{
	for (auto& quest : activeQuests_)
	{
		if (!quest.isActive || quest.isCompleted) continue;

		for (auto& obj : quest.objectives)
		{
			if (obj.type == QuestObjectiveType::Escape && !obj.isCompleted)
			{
				obj.AddProgress(1);
			}
		}
	}

	CheckQuestCompletion();
}

void QuestSystem::CheckQuestCompletion()
{
	for (auto& quest : activeQuests_)
	{
		if (!quest.isActive || quest.isCompleted) continue;

		if (quest.CheckAllObjectivesCompleted())
		{
			quest.isCompleted = true;
			FeedbackSystem::GetInstance().ShowNotification(
				"クエスト完了: " + quest.name,
				NotificationType::QuestUpdate
			);
		}
	}
}

int QuestSystem::CalculateTotalMoneyReward() const
{
	int total = 0;
	for (const auto& quest : activeQuests_)
	{
		if (quest.isCompleted)
		{
			total += quest.reward.money;
		}
	}
	return total;
}

std::vector<std::pair<int, int>> QuestSystem::CalculateTotalItemRewards() const
{
	std::vector<std::pair<int, int>> rewards;

	for (const auto& quest : activeQuests_)
	{
		if (quest.isCompleted)
		{
			for (const auto& item : quest.reward.items)
			{
				// 既存のアイテムに追加するか新規追加
				bool found = false;
				for (auto& r : rewards)
				{
					if (r.first == item.first)
					{
						r.second += item.second;
						found = true;
						break;
					}
				}
				if (!found)
				{
					rewards.push_back(item);
				}
			}
		}
	}

	return rewards;
}
