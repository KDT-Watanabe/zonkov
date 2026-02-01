#pragma once
#include <string>
#include <vector>
#include <functional>

// クエスト目標タイプ
enum class QuestObjectiveType
{
	KillEnemies,    // 敵を倒す
	CollectItem,    // アイテムを集める
	Escape,         // 脱出する
	ExploreArea,    // エリアを探索する
	SurviveTime,    // 一定時間生き残る
	Count
};

// クエスト目標
struct QuestObjective
{
	QuestObjectiveType type;
	std::string description;
	int targetId;       // アイテムIDまたはエリアID
	int requiredCount;  // 必要数
	int currentCount;   // 現在の進捗
	bool isCompleted;

	QuestObjective()
		: type(QuestObjectiveType::KillEnemies)
		, description("")
		, targetId(0)
		, requiredCount(1)
		, currentCount(0)
		, isCompleted(false)
	{}

	QuestObjective(QuestObjectiveType t, const std::string& desc, int count, int target = 0)
		: type(t)
		, description(desc)
		, targetId(target)
		, requiredCount(count)
		, currentCount(0)
		, isCompleted(false)
	{}

	void AddProgress(int amount = 1)
	{
		if (isCompleted) return;
		currentCount += amount;
		if (currentCount >= requiredCount)
		{
			currentCount = requiredCount;
			isCompleted = true;
		}
	}

	float GetProgressRatio() const
	{
		if (requiredCount <= 0) return 1.0f;
		return static_cast<float>(currentCount) / static_cast<float>(requiredCount);
	}
};

// クエスト報酬
struct QuestReward
{
	int money;
	std::vector<std::pair<int, int>> items;  // アイテムID, 数量

	QuestReward() : money(0) {}
};

// クエストデータ
struct Quest
{
	int id;
	std::string name;
	std::string description;
	std::vector<QuestObjective> objectives;
	QuestReward reward;
	bool isActive;
	bool isCompleted;
	bool isFailed;

	Quest()
		: id(0)
		, name("")
		, description("")
		, isActive(false)
		, isCompleted(false)
		, isFailed(false)
	{}

	bool CheckAllObjectivesCompleted() const
	{
		for (const auto& obj : objectives)
		{
			if (!obj.isCompleted) return false;
		}
		return true;
	}
};

// クエストシステム（シングルトン）
class QuestSystem
{
public:
	static QuestSystem& GetInstance()
	{
		static QuestSystem instance;
		return instance;
	}

	void Initialize();
	void Clear();

	// レイド開始時にクエストを生成
	void GenerateRaidQuests();

	// クエスト進捗通知
	void OnEnemyKilled();
	void OnItemCollected(int itemId, int count);
	void OnAreaReached(int areaId);
	void OnEscaped();

	// クエスト取得
	const std::vector<Quest>& GetActiveQuests() const { return activeQuests_; }
	bool HasActiveQuests() const { return !activeQuests_.empty(); }

	// クエスト完了チェック
	void CheckQuestCompletion();

	// 報酬計算
	int CalculateTotalMoneyReward() const;
	std::vector<std::pair<int, int>> CalculateTotalItemRewards() const;

private:
	QuestSystem() = default;
	QuestSystem(const QuestSystem&) = delete;
	QuestSystem& operator=(const QuestSystem&) = delete;

	std::vector<Quest> activeQuests_;
	bool isInitialized_ = false;
};
