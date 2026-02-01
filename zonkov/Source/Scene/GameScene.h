#pragma once
#include "IScene.h"
#include "System/Map/MapSystem.h"
#include "System/Inventory/Inventory.h"
#include "Entity/Item/ItemDrop.h"
#include "Entity/Player/Player.h"
#include "Entity/Enemy/Enemy.h"
#include "Entity/Item/LootContainer.h"
#include "Entity/Projectile/Projectile.h"
#include "Utility/Vector2.h"
#include <memory>
#include <vector>
#include <random>

class GameScene : public IScene
{
public:
	GameScene();
	~GameScene() override = default;

	void Init() override;
	void Update() override;
	void Draw() override;
	std::unique_ptr<IScene> GetNextScene() override;

private:
	// 更新処理
	void UpdatePlayer();
	void UpdateCombat();
	void UpdateEnemies();
	void UpdateProjectiles();
	void UpdateItemPickup();
	void UpdateInventoryUI();
	void UpdateEquipmentUI();
	void UpdateCamera();

	// 描画処理
	void DrawPlayer() const;
	void DrawEnemies() const;
	void DrawProjectiles() const;
	void DrawItems() const;
	void DrawContainers() const;
	void DrawInventory() const;
	void DrawEquipmentUI() const;
	void DrawQuickSlotsUI() const;
	void DrawDraggedItem() const;
	void DrawItemTooltip() const;
	void DrawUI() const;
	void DrawCombatUI() const;
	void DrawQuestUI() const;
	void DrawContainerUI() const;

	// アイテム・敵・コンテナ配置
	void SpawnItems();
	void SpawnEnemies();
	void SpawnContainers();

	// コンテナ操作
	void UpdateContainerInteraction();
	void UpdateContainerUI();
	LootContainer* GetNearestContainer();
	bool ScreenToContainerGrid(int screenX, int screenY, int& gridX, int& gridY) const;
	void CreateCorpseAt(const Vector2& pos);

	// インベントリ座標変換
	bool ScreenToInventoryGrid(int screenX, int screenY, int& gridX, int& gridY) const;
	bool IsInsideInventory(int screenX, int screenY) const;

	// 装備スロット座標変換
	int GetEquipSlotAtPosition(int screenX, int screenY) const;
	bool TryEquipItem(int slotIndex);
	bool TryUnequipItem(EquipSlot slot);

	// アイテム使用
	bool TryUseItem(int slotIndex);
	bool TryAssignQuickSlot(int slotIndex);
	bool TryAssignQuickSlotTo(int slotIndex, int quickSlotIndex);

	// 装備変更時の処理
	void OnEquipmentChanged(EquipSlot slot);
	void UpdateInventorySize();
	void UpdateQuickSlots();
	void HandleQuickSlotInput();

	// 遠距離攻撃
	bool TryRangedAttack();
	void SpawnProjectile(ProjectileType type, const Vector2& pos, const Vector2& dir, int damage, float speed, float range, bool isPlayerOwned);

	// カメラ座標変換
	Vector2 WorldToScreen(const Vector2& worldPos) const;
	Vector2 ScreenToWorld(const Vector2& screenPos) const;

private:
	MapSystem map_;
	std::vector<ItemDrop> itemDrops_;

	// インベントリはGameDataのraidInventoryを使用する
	Inventory& GetInventory();
	const Inventory& GetInventory() const;

	// プレイヤー
	std::unique_ptr<Player> player_;
	float maxWeight_;

	// 敵
	std::vector<std::unique_ptr<Enemy>> enemies_;

	// 発射体
	std::vector<std::unique_ptr<Projectile>> projectiles_;

	// コンテナ
	std::vector<std::unique_ptr<LootContainer>> containers_;
	std::mt19937 rng_;

	// コンテナUI
	LootContainer* openContainer_;
	int containerOffsetX_;
	int containerOffsetY_;
	bool isDraggingFromContainer_;
	int dragContainerSlotIndex_;

	// インベントリ表示
	bool showInventory_;
	int inventoryOffsetX_;
	int inventoryOffsetY_;

	// ドラッグ状態
	bool isDragging_;
	int dragSlotIndex_;
	bool dragRotated_;
	int dragOffsetX_;
	int dragOffsetY_;

	// ゲーム状態
	bool isCleared_;
	bool isGameOver_;
	std::unique_ptr<IScene> nextScene_;

	// カメラ
	Vector2 cameraOffset_;

	// 画面サイズ定数
	static constexpr float SCREEN_WIDTH = 1280.0f;
	static constexpr float SCREEN_HEIGHT = 720.0f;

	// 装備UI定数
	static constexpr int EQUIP_OFFSET_X = 30;
	static constexpr int EQUIP_OFFSET_Y = 100;
	static constexpr int EQUIP_SLOT_SIZE = 60;
	static constexpr int EQUIP_SLOT_GAP = 10;

	// コンテナUI定数（インベントリの左側に配置）
	static constexpr int CONTAINER_OFFSET_X = 150;
	static constexpr int CONTAINER_OFFSET_Y = 100;
};
