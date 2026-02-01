#include "DxLib.h"
#include "GameScene.h"
#include "BaseScene.h"
#include "System/Input/InputManager.h"
#include "System/GameData/GameData.h"
#include "System/Combat/CombatSystem.h"
#include "System/Resource/ResourceManager.h"
#include "System/UI/FeedbackSystem.h"
#include "System/Quest/QuestSystem.h"
#include "Data/ItemDatabase.h"
#include <algorithm>

GameScene::GameScene()
	: player_(nullptr)
	, maxWeight_(30.0f)
	, showInventory_(false)
	, inventoryOffsetX_(500)
	, inventoryOffsetY_(100)
	, isDragging_(false)
	, dragSlotIndex_(-1)
	, dragRotated_(false)
	, dragOffsetX_(0)
	, dragOffsetY_(0)
	, isCleared_(false)
	, isGameOver_(false)
	, nextScene_(nullptr)
	, cameraOffset_(0.0f, 0.0f)
	, rng_(std::random_device{}())
	, openContainer_(nullptr)
	, containerOffsetX_(CONTAINER_OFFSET_X)
	, containerOffsetY_(CONTAINER_OFFSET_Y)
	, isDraggingFromContainer_(false)
	, dragContainerSlotIndex_(-1)
{
}

void GameScene::Init()
{
	ItemDatabase::GetInstance().Initialize();
	CombatSystem::GetInstance().Initialize();
	FeedbackSystem::GetInstance().Clear();
	QuestSystem::GetInstance().Initialize();
	QuestSystem::GetInstance().GenerateRaidQuests();

	map_.LoadTestMap();

	// プレイヤー生成
	player_ = std::make_unique<Player>();
	player_->SetPosition(map_.GetPlayerStartPosition());

	isCleared_ = false;
	isGameOver_ = false;
	showInventory_ = false;
	isDragging_ = false;
	dragSlotIndex_ = -1;
	openContainer_ = nullptr;

	SpawnItems();
	SpawnEnemies();
	SpawnContainers();
}

void GameScene::Update()
{
	auto& input = InputManager::GetInstance();
	input.Update();

	// フィードバックシステム更新
	FeedbackSystem::GetInstance().Update(1.0f / 60.0f);

	// インベントリ開閉（TABキー）
	if (input.IsKeyTriggered(KEY_INPUT_TAB))
	{
		showInventory_ = !showInventory_;
		if (!showInventory_)
		{
			isDragging_ = false;
			dragSlotIndex_ = -1;
		}
	}

	// クリア時
	if (isCleared_)
	{
		if (input.IsKeyTriggered(KEY_INPUT_RETURN))
		{
			GameData::GetInstance().OnRaidSuccess();
			nextScene_ = std::make_unique<BaseScene>();
		}
		return;
	}

	// ゲームオーバー時
	if (isGameOver_)
	{
		if (input.IsKeyTriggered(KEY_INPUT_RETURN))
		{
			GameData::GetInstance().OnRaidFailed();
			nextScene_ = std::make_unique<BaseScene>();
		}
		return;
	}

	// 敵とプロジェクタイルは常に更新（インベントリ中も動く）
	UpdateEnemies();
	UpdateProjectiles();

	if (showInventory_)
	{
		UpdateInventoryUI();
		if (openContainer_)
		{
			UpdateContainerUI();
		}
	}
	else
	{
		UpdatePlayer();
		UpdateCombat();
		UpdateItemPickup();
		UpdateContainerInteraction();
		HandleQuickSlotInput();
	}

	// カメラ更新
	UpdateCamera();

	// プレイヤー死亡チェック
	if (player_ && !player_->IsAlive())
	{
		isGameOver_ = true;
	}

	// 脱出判定
	if (player_ && map_.IsInExitArea(player_->GetPosition()))
	{
		if (!isCleared_)
		{
			QuestSystem::GetInstance().OnEscaped();
		}
		isCleared_ = true;
	}
}

void GameScene::Draw()
{
	map_.Draw(cameraOffset_);
	DrawContainers();
	DrawItems();
	DrawEnemies();
	DrawProjectiles();
	DrawPlayer();

	// ダメージポップアップ描画
	FeedbackSystem::GetInstance().Draw(cameraOffset_);

	DrawUI();
	DrawCombatUI();
	DrawQuestUI();

	// 画面エフェクト描画
	FeedbackSystem::GetInstance().DrawScreenEffects();

	if (showInventory_)
	{
		DrawEquipmentUI();
		DrawInventory();
		if (openContainer_)
		{
			DrawContainerUI();
		}
		DrawDraggedItem();
		DrawItemTooltip();
	}

	// 通知描画
	FeedbackSystem::GetInstance().DrawNotifications();

	if (isCleared_)
	{
		DrawString(350, 280, "脱出成功!", GetColor(0, 255, 100));
		DrawString(320, 310, "ENTERでベースに戻る", GetColor(255, 255, 255));
	}

	if (isGameOver_)
	{
		DrawString(330, 280, "ゲームオーバー", GetColor(255, 50, 50));
		DrawString(320, 310, "ENTERでベースに戻る", GetColor(255, 255, 255));
	}
}

std::unique_ptr<IScene> GameScene::GetNextScene()
{
	return std::move(nextScene_);
}

Inventory& GameScene::GetInventory()
{
	return GameData::GetInstance().GetRaidInventory();
}

const Inventory& GameScene::GetInventory() const
{
	return GameData::GetInstance().GetRaidInventory();
}

void GameScene::UpdatePlayer()
{
	if (!player_) return;

	float deltaTime = 1.0f / 60.0f;
	float weightRatio = GetInventory().GetTotalWeight() / maxWeight_;

	player_->UpdateMovement(deltaTime, map_, weightRatio);
	player_->Update(deltaTime);
}

void GameScene::UpdateCombat()
{
	if (!player_) return;

	auto& input = InputManager::GetInstance();
	auto& combat = CombatSystem::GetInstance();

	// 攻撃（スペースキーまたは左クリック）
	if (input.IsKeyTriggered(KEY_INPUT_SPACE) || input.IsMouseTriggered(0))
	{
		if (player_->CanAttack() && !showInventory_)
		{
			// マウス方向を向く
			Vector2 mousePos = input.GetMousePosition();
			Vector2 mouseWorld = ScreenToWorld(mousePos);
			player_->FaceTowards(mouseWorld);

			// 遠距離武器チェック
			int weaponId = player_->GetEquippedItemId(EquipSlot::Weapon);
			const WeaponData* weapon = combat.GetWeaponData(weaponId);

			if (weapon && weapon->IsRanged())
			{
				// 遠距離攻撃
				if (TryRangedAttack())
				{
					player_->Attack();
				}
			}
			else
			{
				// 近接攻撃
				player_->Attack();
			}
		}
	}

	// 近接攻撃判定処理
	if (player_->IsAttacking())
	{
		int weaponId = player_->GetEquippedItemId(EquipSlot::Weapon);
		const WeaponData* weapon = combat.GetWeaponData(weaponId);

		// 近接武器のみ直接ヒット判定
		if (!weapon || !weapon->IsRanged())
		{
			combat.ProcessPlayerAttack(*player_, enemies_);
		}
	}

	// 敵の攻撃処理
	for (auto& enemy : enemies_)
	{
		if (enemy->IsAlive() && enemy->IsAttacking())
		{
			combat.ProcessEnemyAttack(*enemy, *player_);
		}
	}
}

void GameScene::UpdateEnemies()
{
	if (!player_) return;

	float deltaTime = 1.0f / 60.0f;

	for (auto& enemy : enemies_)
	{
		enemy->Update(deltaTime);
	}

	// 死亡した敵を削除し、死体コンテナを生成
	for (auto it = enemies_.begin(); it != enemies_.end(); )
	{
		if (!(*it)->IsAlive())
		{
			CreateCorpseAt((*it)->GetPosition());
			QuestSystem::GetInstance().OnEnemyKilled();  // クエスト進捗
			it = enemies_.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void GameScene::UpdateItemPickup()
{
	if (!player_) return;

	auto& input = InputManager::GetInstance();

	if (input.IsKeyTriggered(KEY_INPUT_E))
	{
		for (auto& drop : itemDrops_)
		{
			if (!drop.IsActive()) continue;

			float dist = player_->GetPosition().Distance(drop.position);
			if (dist <= drop.pickupRadius + player_->GetRadius())
			{
				if (GetInventory().AddItem(drop.itemId, drop.stackCount))
				{
					drop.isPickedUp = true;

					// アイテム取得通知
					const ItemData* data = ItemDatabase::GetInstance().GetItemData(drop.itemId);
					if (data)
					{
						FeedbackSystem::GetInstance().ShowItemPickup(data->name, drop.stackCount);
					}

					// クエスト進捗
					QuestSystem::GetInstance().OnItemCollected(drop.itemId, drop.stackCount);
				}
				else
				{
					// インベントリがいっぱい
					FeedbackSystem::GetInstance().ShowWarning("インベントリがいっぱい!");
				}
				break;
			}
		}
	}
}

void GameScene::UpdateInventoryUI()
{
	auto& input = InputManager::GetInstance();
	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	// Ctrl+数字キーでクイックスロット登録
	bool ctrlPressed = input.IsKeyPressed(KEY_INPUT_LCONTROL) || input.IsKeyPressed(KEY_INPUT_RCONTROL);
	if (ctrlPressed)
	{
		int gridX, gridY;
		if (ScreenToInventoryGrid(mx, my, gridX, gridY))
		{
			int slotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				for (int i = 0; i < 6; ++i)
				{
					if (input.IsKeyTriggered(KEY_INPUT_1 + i))
					{
						TryAssignQuickSlotTo(slotIndex, i);
						break;
					}
				}
			}
		}
	}

	// Rキーで回転（ドラッグ中）
	if (isDragging_ && input.IsKeyTriggered(KEY_INPUT_R))
	{
		const InventorySlot* slot = GetInventory().GetSlot(dragSlotIndex_);
		if (slot)
		{
			const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
			if (data && data->gridWidth != data->gridHeight)
			{
				dragRotated_ = !dragRotated_;
			}
		}
	}

	// 左クリックでドラッグ開始/終了
	if (input.IsMouseTriggered(0))
	{
		if (!isDragging_)
		{
			// インベントリからドラッグ開始
			int gridX, gridY;
			if (ScreenToInventoryGrid(mx, my, gridX, gridY))
			{
				int slotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
				if (slotIndex >= 0)
				{
					const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
					if (slot && !slot->IsEmpty())
					{
						isDragging_ = true;
						dragSlotIndex_ = slotIndex;
						dragRotated_ = slot->isRotated;
						dragOffsetX_ = gridX - slot->gridX;
						dragOffsetY_ = gridY - slot->gridY;
					}
				}
			}
			// 装備スロットクリックで解除
			else
			{
				int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
				if (equipSlotIndex >= 0)
				{
					TryUnequipItem(static_cast<EquipSlot>(equipSlotIndex));
				}
			}
		}
		else
		{
			// ドラッグ終了
			bool placed = false;
			const InventorySlot* draggedSlot = GetInventory().GetSlot(dragSlotIndex_);

			// 装備スロットにドロップ
			int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
			if (equipSlotIndex >= 0)
			{
				placed = TryEquipItem(dragSlotIndex_);
			}

			// コンテナにドロップ（コンテナが開いている場合）
			if (!placed && openContainer_ && draggedSlot && !draggedSlot->IsEmpty())
			{
				int gridX, gridY;
				if (ScreenToContainerGrid(mx, my, gridX, gridY))
				{
					// 同じアイテムへのスタックを試みる
					int targetSlotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
					if (targetSlotIndex >= 0)
					{
						const InventorySlot* targetSlot = openContainer_->GetInventory().GetSlot(targetSlotIndex);
						if (targetSlot && targetSlot->itemId == draggedSlot->itemId)
						{
							// スタックを試みる
							if (openContainer_->GetInventory().TryMergeSlots(-1, targetSlotIndex))
							{
								// 外部からなのでマージではなく直接追加
								int added = openContainer_->GetInventory().AddToStack(targetSlotIndex, draggedSlot->stackCount);
								if (added > 0)
								{
									GetInventory().RemoveItem(dragSlotIndex_, added);
									placed = true;
								}
							}
							else
							{
								// AddToStackで追加
								int added = openContainer_->GetInventory().AddToStack(targetSlotIndex, draggedSlot->stackCount);
								if (added > 0)
								{
									GetInventory().RemoveItem(dragSlotIndex_, added);
									placed = true;
								}
							}
						}
					}

					// スタックできなかった場合、空きスペースに配置
					if (!placed)
					{
						int placeX = gridX - dragOffsetX_;
						int placeY = gridY - dragOffsetY_;
						if (openContainer_->GetInventory().CanFitItemAt(draggedSlot->itemId, placeX, placeY, dragRotated_))
						{
							openContainer_->GetInventory().AddItemAt(draggedSlot->itemId, draggedSlot->stackCount, placeX, placeY, dragRotated_);
							GetInventory().RemoveItem(dragSlotIndex_, draggedSlot->stackCount);
							placed = true;
						}
					}
				}
			}

			// インベントリにドロップ
			if (!placed && draggedSlot && !draggedSlot->IsEmpty())
			{
				int gridX, gridY;
				if (ScreenToInventoryGrid(mx, my, gridX, gridY))
				{
					// 同じアイテムへのスタックを試みる
					int targetSlotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
					if (targetSlotIndex >= 0 && targetSlotIndex != dragSlotIndex_)
					{
						const InventorySlot* targetSlot = GetInventory().GetSlot(targetSlotIndex);
						if (targetSlot && targetSlot->itemId == draggedSlot->itemId)
						{
							// スタックを試みる
							if (GetInventory().TryMergeSlots(dragSlotIndex_, targetSlotIndex))
							{
								placed = true;
							}
						}
					}

					// スタックできなかった場合、通常の移動
					if (!placed)
					{
						int placeX = gridX - dragOffsetX_;
						int placeY = gridY - dragOffsetY_;

						if (draggedSlot->isRotated != dragRotated_)
						{
							placeX = gridX;
							placeY = gridY;
						}

						if (GetInventory().CanPlaceItemAt(dragSlotIndex_, placeX, placeY, dragRotated_))
						{
							GetInventory().MoveItem(dragSlotIndex_, placeX, placeY, dragRotated_);
							placed = true;
						}
					}
				}
			}

			isDragging_ = false;
			dragSlotIndex_ = -1;
		}
	}

	// 右クリック処理
	if (input.IsMouseTriggered(1))
	{
		if (isDragging_)
		{
			// ドラッグ中なら右クリックでキャンセル
			isDragging_ = false;
			dragSlotIndex_ = -1;
		}
		else
		{
			// ドラッグ中でなければアイテム使用
			int gridX, gridY;
			if (ScreenToInventoryGrid(mx, my, gridX, gridY))
			{
				int slotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
				if (slotIndex >= 0)
				{
					// 右クリックでアイテム使用
					TryUseItem(slotIndex);
				}
			}
		}
	}
}

void GameScene::DrawPlayer() const
{
	if (player_)
	{
		player_->Draw(cameraOffset_);
	}
}

void GameScene::DrawEnemies() const
{
	for (const auto& enemy : enemies_)
	{
		enemy->Draw(cameraOffset_);
	}
}

void GameScene::DrawItems() const
{
	if (!player_) return;

	int ox = static_cast<int>(cameraOffset_.x);
	int oy = static_cast<int>(cameraOffset_.y);
	auto& res = ResourceManager::GetInstance();

	for (const auto& drop : itemDrops_)
	{
		if (!drop.IsActive()) continue;

		int x = static_cast<int>(drop.position.x) - ox;
		int y = static_cast<int>(drop.position.y) - oy;

		const ItemData* data = ItemDatabase::GetInstance().GetItemData(drop.itemId);

		// アイテム画像を試す
		std::string imgKey = "item_" + std::to_string(drop.itemId);
		int handle = res.GetGraph(imgKey);

		if (handle != -1)
		{
			// 画像描画（縮小して表示）
			int imgW, imgH;
			GetGraphSize(handle, &imgW, &imgH);
			int drawSize = 24;
			DrawExtendGraph(x - drawSize / 2, y - drawSize / 2,
				x + drawSize / 2, y + drawSize / 2, handle, TRUE);
		}
		else
		{
			// フォールバック: 色付き円
			unsigned int color = GetColor(200, 200, 100);
			if (data)
			{
				switch (data->type)
				{
				case ItemType::Consumable:
					color = GetColor(100, 200, 100);
					break;
				case ItemType::Weapon:
					color = GetColor(200, 100, 100);
					break;
				case ItemType::Armor:
					color = GetColor(100, 100, 200);
					break;
				case ItemType::Valuable:
					color = GetColor(255, 215, 0);
					break;
				case ItemType::Backpack:
					color = GetColor(180, 140, 100);
					break;
				case ItemType::Rig:
					color = GetColor(100, 180, 100);
					break;
				default:
					break;
				}
			}
			DrawCircle(x, y, 8, color, TRUE);
			DrawCircle(x, y, 8, GetColor(255, 255, 255), FALSE);
		}

		// 拾える範囲内なら強調表示
		float dist = player_->GetPosition().Distance(drop.position);
		if (dist <= drop.pickupRadius + player_->GetRadius())
		{
			DrawCircle(x, y, 14, GetColor(255, 255, 100), FALSE);
			if (data)
			{
				DrawString(x - 30, y - 25, data->name.c_str(), GetColor(255, 255, 255));
			}
		}
	}
}

void GameScene::DrawInventory() const
{
	const int cellSize = Inventory::CELL_SIZE;
	const int gridW = GetInventory().GetGridWidth();
	const int gridH = GetInventory().GetGridHeight();
	const int ox = inventoryOffsetX_;
	const int oy = inventoryOffsetY_;

	DrawBox(ox - 10, oy - 30, ox + gridW * cellSize + 10, oy + gridH * cellSize + 10,
		GetColor(30, 30, 40), TRUE);
	DrawBox(ox - 10, oy - 30, ox + gridW * cellSize + 10, oy + gridH * cellSize + 10,
		GetColor(100, 100, 120), FALSE);

	DrawString(ox, oy - 25, "インベントリ  R:回転  Ctrl+1-6:登録", GetColor(255, 255, 255));

	for (int y = 0; y < gridH; ++y)
	{
		for (int x = 0; x < gridW; ++x)
		{
			int px = ox + x * cellSize;
			int py = oy + y * cellSize;

			DrawBox(px, py, px + cellSize, py + cellSize,
				GetColor(50, 50, 60), TRUE);
			DrawBox(px, py, px + cellSize, py + cellSize,
				GetColor(80, 80, 90), FALSE);
		}
	}

	for (int i = 0; i < static_cast<int>(GetInventory().GetSlots().size()); ++i)
	{
		const InventorySlot& slot = GetInventory().GetSlots()[i];
		if (slot.IsEmpty()) continue;
		if (isDragging_ && i == dragSlotIndex_) continue;

		const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
		if (!data) continue;

		int px = ox + slot.gridX * cellSize;
		int py = oy + slot.gridY * cellSize;
		int w = slot.GetWidth(data) * cellSize;
		int h = slot.GetHeight(data) * cellSize;

		unsigned int bgColor = GetColor(80, 80, 60);
		switch (data->type)
		{
		case ItemType::Consumable:
			bgColor = GetColor(60, 100, 60);
			break;
		case ItemType::Weapon:
			bgColor = GetColor(100, 60, 60);
			break;
		case ItemType::Armor:
			bgColor = GetColor(60, 60, 100);
			break;
		case ItemType::Valuable:
			bgColor = GetColor(100, 90, 40);
			break;
		case ItemType::Backpack:
			bgColor = GetColor(90, 70, 50);
			break;
		case ItemType::Rig:
			bgColor = GetColor(50, 90, 50);
			break;
		default:
			break;
		}

		DrawBox(px + 2, py + 2, px + w - 2, py + h - 2, bgColor, TRUE);
		DrawBox(px + 2, py + 2, px + w - 2, py + h - 2, GetColor(150, 150, 150), FALSE);

		std::string shortName = data->name.substr(0, 6);
		DrawString(px + 4, py + 4, shortName.c_str(), GetColor(255, 255, 255));

		if (slot.stackCount > 1)
		{
			DrawFormatString(px + w - 20, py + h - 18, GetColor(255, 255, 200),
				"x%d", slot.stackCount);
		}
	}
}

void GameScene::DrawDraggedItem() const
{
	const InventorySlot* slot = nullptr;
	int slotIndex = -1;
	bool isFromContainer = false;

	if (isDragging_ && dragSlotIndex_ >= 0)
	{
		slot = GetInventory().GetSlot(dragSlotIndex_);
		slotIndex = dragSlotIndex_;
	}
	else if (isDraggingFromContainer_ && dragContainerSlotIndex_ >= 0 && openContainer_)
	{
		slot = openContainer_->GetInventory().GetSlot(dragContainerSlotIndex_);
		slotIndex = dragContainerSlotIndex_;
		isFromContainer = true;
	}

	if (!slot || slot->IsEmpty()) return;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
	if (!data) return;

	const int cellSize = Inventory::CELL_SIZE;
	auto& input = InputManager::GetInstance();
	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	int w = (dragRotated_ ? data->gridHeight : data->gridWidth) * cellSize;
	int h = (dragRotated_ ? data->gridWidth : data->gridHeight) * cellSize;

	int drawX = mx - dragOffsetX_ * cellSize;
	int drawY = my - dragOffsetY_ * cellSize;

	int gridX, gridY;
	bool canPlace = false;

	// インベントリ上でのドロップチェック
	if (ScreenToInventoryGrid(mx, my, gridX, gridY))
	{
		int placeX = gridX - dragOffsetX_;
		int placeY = gridY - dragOffsetY_;
		if (slot->isRotated != dragRotated_)
		{
			placeX = gridX;
			placeY = gridY;
			drawX = inventoryOffsetX_ + gridX * cellSize;
			drawY = inventoryOffsetY_ + gridY * cellSize;
		}

		if (isFromContainer)
		{
			canPlace = GetInventory().CanFitItemAt(slot->itemId, placeX, placeY, dragRotated_);
		}
		else
		{
			canPlace = GetInventory().CanPlaceItemAt(slotIndex, placeX, placeY, dragRotated_);
		}
	}
	// コンテナ上でのドロップチェック
	else if (openContainer_ && ScreenToContainerGrid(mx, my, gridX, gridY))
	{
		int placeX = gridX - dragOffsetX_;
		int placeY = gridY - dragOffsetY_;

		if (isFromContainer)
		{
			canPlace = openContainer_->GetInventory().CanPlaceItemAt(slotIndex, placeX, placeY, dragRotated_);
		}
		else
		{
			canPlace = openContainer_->GetInventory().CanFitItemAt(slot->itemId, placeX, placeY, dragRotated_);
		}
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);

	unsigned int bgColor = canPlace ? GetColor(80, 150, 80) : GetColor(150, 80, 80);
	DrawBox(drawX, drawY, drawX + w, drawY + h, bgColor, TRUE);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawBox(drawX, drawY, drawX + w, drawY + h,
		canPlace ? GetColor(100, 255, 100) : GetColor(255, 100, 100), FALSE);

	std::string shortName = data->name.substr(0, 6);
	DrawString(drawX + 4, drawY + 4, shortName.c_str(), GetColor(255, 255, 255));

	if (slot->stackCount > 1)
	{
		DrawFormatString(drawX + w - 20, drawY + h - 18, GetColor(255, 255, 200),
			"x%d", slot->stackCount);
	}
}

void GameScene::DrawUI() const
{
	int white = GetColor(255, 255, 255);
	int gray = GetColor(180, 180, 180);

	DrawString(10, 10, "WASD:移動 | SPACE:攻撃 | E:拾う | TAB:インベントリ", gray);

	float weight = GetInventory().GetTotalWeight();
	float ratio = weight / maxWeight_;
	unsigned int weightColor = white;
	if (ratio >= 1.0f) weightColor = GetColor(255, 50, 50);
	else if (ratio >= 0.8f) weightColor = GetColor(255, 150, 50);
	else if (ratio >= 0.6f) weightColor = GetColor(255, 255, 50);

	DrawFormatString(10, 30, weightColor, "Weight: %.1f / %.1f kg", weight, maxWeight_);

	int barX = 10;
	int barY = 50;
	int barW = 150;
	int barH = 10;
	DrawBox(barX, barY, barX + barW, barY + barH, GetColor(50, 50, 50), TRUE);
	int fillW = static_cast<int>(barW * (ratio < 1.0f ? ratio : 1.0f));
	DrawBox(barX, barY, barX + fillW, barY + barH, weightColor, TRUE);
	DrawBox(barX, barY, barX + barW, barY + barH, gray, FALSE);
}

void GameScene::DrawCombatUI() const
{
	if (!player_) return;

	// プレイヤーHPバー
	int barX = 10;
	int barY = 70;
	int barW = 150;
	int barH = 15;

	float hpRatio = static_cast<float>(player_->GetHP()) / static_cast<float>(player_->GetMaxHP());

	DrawBox(barX, barY, barX + barW, barY + barH, GetColor(50, 50, 50), TRUE);
	int fillW = static_cast<int>(barW * hpRatio);
	DrawBox(barX, barY, barX + fillW, barY + barH, GetColor(50, 200, 50), TRUE);
	DrawBox(barX, barY, barX + barW, barY + barH, GetColor(100, 100, 100), FALSE);

	DrawFormatString(barX + 5, barY + 1, GetColor(255, 255, 255), "HP: %d/%d",
		player_->GetHP(), player_->GetMaxHP());

	// 敵の数
	int aliveEnemies = 0;
	for (const auto& e : enemies_)
	{
		if (e->IsAlive()) aliveEnemies++;
	}
	DrawFormatString(10, 90, GetColor(255, 200, 200), "Enemies: %d", aliveEnemies);
}

void GameScene::SpawnItems()
{
	// フィールド上の直接アイテムドロップは削除
	// すべてのアイテムはコンテナから取得する
	itemDrops_.clear();
}

void GameScene::SpawnEnemies()
{
	enemies_.clear();

	// 敵1: スタート付近（左上）
	auto enemy1 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(400.0f, 400.0f));
	enemy1->SetTarget(player_.get());
	enemy1->SetMapSystem(&map_);
	std::vector<Vector2> patrol1 = { Vector2(400.0f, 400.0f), Vector2(500.0f, 400.0f), Vector2(500.0f, 500.0f), Vector2(400.0f, 500.0f) };
	enemy1->SetPatrolPoints(patrol1);
	enemies_.push_back(std::move(enemy1));

	// 敵2: 中央左エリア
	auto enemy2 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(550.0f, 750.0f));
	enemy2->SetTarget(player_.get());
	enemy2->SetMapSystem(&map_);
	std::vector<Vector2> patrol2 = { Vector2(550.0f, 750.0f), Vector2(700.0f, 750.0f), Vector2(700.0f, 850.0f) };
	enemy2->SetPatrolPoints(patrol2);
	enemies_.push_back(std::move(enemy2));

	// 敵3: 上部中央
	auto enemy3 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(1200.0f, 350.0f));
	enemy3->SetTarget(player_.get());
	enemy3->SetMapSystem(&map_);
	std::vector<Vector2> patrol3 = { Vector2(1200.0f, 350.0f), Vector2(1350.0f, 350.0f) };
	enemy3->SetPatrolPoints(patrol3);
	enemies_.push_back(std::move(enemy3));

	// 敵4: 中央エリア
	auto enemy4 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(1500.0f, 1000.0f));
	enemy4->SetTarget(player_.get());
	enemy4->SetMapSystem(&map_);
	std::vector<Vector2> patrol4 = { Vector2(1500.0f, 1000.0f), Vector2(1650.0f, 1000.0f), Vector2(1650.0f, 1100.0f), Vector2(1500.0f, 1100.0f) };
	enemy4->SetPatrolPoints(patrol4);
	enemies_.push_back(std::move(enemy4));

	// 敵5: 右上
	auto enemy5 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(2350.0f, 450.0f));
	enemy5->SetTarget(player_.get());
	enemy5->SetMapSystem(&map_);
	enemies_.push_back(std::move(enemy5));

	// 敵6: 下部中央
	auto enemy6 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(1000.0f, 1800.0f));
	enemy6->SetTarget(player_.get());
	enemy6->SetMapSystem(&map_);
	std::vector<Vector2> patrol6 = { Vector2(1000.0f, 1800.0f), Vector2(1100.0f, 1800.0f), Vector2(1100.0f, 1900.0f) };
	enemy6->SetPatrolPoints(patrol6);
	enemies_.push_back(std::move(enemy6));

	// 敵7: 中央右
	auto enemy7 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(2200.0f, 1350.0f));
	enemy7->SetTarget(player_.get());
	enemy7->SetMapSystem(&map_);
	std::vector<Vector2> patrol7 = { Vector2(2200.0f, 1350.0f), Vector2(2300.0f, 1350.0f), Vector2(2300.0f, 1450.0f), Vector2(2200.0f, 1450.0f) };
	enemy7->SetPatrolPoints(patrol7);
	enemies_.push_back(std::move(enemy7));

	// 敵8: 右下（出口付近の守衛）
	auto enemy8 = std::make_unique<Enemy>(EnemyType::Melee, Vector2(2800.0f, 2050.0f));
	enemy8->SetTarget(player_.get());
	enemy8->SetMapSystem(&map_);
	std::vector<Vector2> patrol8 = { Vector2(2800.0f, 2050.0f), Vector2(2900.0f, 2050.0f), Vector2(2900.0f, 2150.0f), Vector2(2800.0f, 2150.0f) };
	enemy8->SetPatrolPoints(patrol8);
	enemies_.push_back(std::move(enemy8));
}

bool GameScene::ScreenToInventoryGrid(int screenX, int screenY, int& gridX, int& gridY) const
{
	const int cellSize = Inventory::CELL_SIZE;
	int relX = screenX - inventoryOffsetX_;
	int relY = screenY - inventoryOffsetY_;

	if (relX < 0 || relY < 0) return false;

	gridX = relX / cellSize;
	gridY = relY / cellSize;

	if (gridX >= GetInventory().GetGridWidth() || gridY >= GetInventory().GetGridHeight())
		return false;

	return true;
}

bool GameScene::IsInsideInventory(int screenX, int screenY) const
{
	int gridX, gridY;
	return ScreenToInventoryGrid(screenX, screenY, gridX, gridY);
}

void GameScene::UpdateEquipmentUI()
{
	// 装備UIの更新処理（必要に応じて追加）
}

void GameScene::DrawEquipmentUI() const
{
	const char* slotNames[] = { "Weapon", "Shield", "Armor", "Backpck", "Rig" };
	auto& gameData = GameData::GetInstance();
	const ItemDatabase& itemDb = ItemDatabase::GetInstance();

	// 背景
	int bgX = EQUIP_OFFSET_X - 5;
	int bgY = EQUIP_OFFSET_Y - 25;
	int bgW = EQUIP_SLOT_SIZE + 10;
	int bgH = (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP) * static_cast<int>(EquipSlot::Count) + 20;
	DrawBox(bgX, bgY, bgX + bgW, bgY + bgH, GetColor(30, 30, 40), TRUE);
	DrawBox(bgX, bgY, bgX + bgW, bgY + bgH, GetColor(100, 100, 120), FALSE);
	DrawString(bgX + 5, bgY + 5, "装備", GetColor(255, 255, 255));

	for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i)
	{
		int x = EQUIP_OFFSET_X;
		int y = EQUIP_OFFSET_Y + i * (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP);

		// スロット背景
		unsigned int slotColor = GetColor(50, 50, 60);
		int itemId = gameData.GetEquippedItem(static_cast<EquipSlot>(i));
		if (itemId != 0)
		{
			// アイテムタイプに応じた色
			const ItemData* data = itemDb.GetItem(itemId);
			if (data)
			{
				if (data->type == ItemType::Weapon)
					slotColor = GetColor(100, 60, 60);
				else if (data->type == ItemType::Armor)
					slotColor = GetColor(60, 60, 100);
				else if (data->type == ItemType::Backpack)
					slotColor = GetColor(80, 60, 40);
				else if (data->type == ItemType::Rig)
					slotColor = GetColor(60, 80, 60);
			}
		}
		DrawBox(x, y, x + EQUIP_SLOT_SIZE, y + EQUIP_SLOT_SIZE, slotColor, TRUE);
		DrawBox(x, y, x + EQUIP_SLOT_SIZE, y + EQUIP_SLOT_SIZE, GetColor(80, 80, 90), FALSE);

		// スロット名
		DrawString(x + 2, y + EQUIP_SLOT_SIZE + 2, slotNames[i], GetColor(150, 150, 150));

		// 装備中アイテム
		if (itemId != 0)
		{
			const ItemData* data = itemDb.GetItem(itemId);
			if (data)
			{
				std::string shortName = data->name.substr(0, 7);
				DrawString(x + 3, y + 3, shortName.c_str(), GetColor(255, 255, 255));
			}
		}
	}

	// クイックスロット表示
	DrawQuickSlotsUI();
}

int GameScene::GetEquipSlotAtPosition(int screenX, int screenY) const
{
	for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i)
	{
		int x = EQUIP_OFFSET_X;
		int y = EQUIP_OFFSET_Y + i * (EQUIP_SLOT_SIZE + EQUIP_SLOT_GAP);

		if (screenX >= x && screenX < x + EQUIP_SLOT_SIZE &&
			screenY >= y && screenY < y + EQUIP_SLOT_SIZE)
		{
			return i;
		}
	}
	return -1;
}

bool GameScene::TryEquipItem(int slotIndex)
{
	const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	// アイテムが装備可能なスロットを取得
	EquipSlot equipSlot = CombatSystem::GetInstance().GetEquipSlotForItem(slot->itemId);
	if (equipSlot == EquipSlot::Count) return false;  // 装備不可

	auto& gameData = GameData::GetInstance();
	int newItemId = slot->itemId;

	// 既に装備がある場合は入れ替え
	int currentEquipped = gameData.GetEquippedItem(equipSlot);
	if (currentEquipped != 0)
	{
		// 旧装備をインベントリに戻せるか確認
		if (!GetInventory().AddItem(currentEquipped, 1))
		{
			FeedbackSystem::GetInstance().ShowWarning("インベントリがいっぱい!");
			return false;  // 空きがない
		}
	}

	// 新しい装備をセット
	gameData.SetEquippedItem(equipSlot, newItemId);

	// インベントリから削除
	GetInventory().RemoveItem(slotIndex, 1);

	// 装備通知
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(newItemId);
	if (data)
	{
		FeedbackSystem::GetInstance().ShowEquipChange(data->name, true);
	}

	// 装備変更処理
	OnEquipmentChanged(equipSlot);

	return true;
}

bool GameScene::TryUnequipItem(EquipSlot slot)
{
	auto& gameData = GameData::GetInstance();
	int itemId = gameData.GetEquippedItem(slot);
	if (itemId == 0) return false;  // 装備なし

	// インベントリに戻せるか確認
	if (!GetInventory().AddItem(itemId, 1))
	{
		FeedbackSystem::GetInstance().ShowWarning("インベントリがいっぱい!");
		return false;  // 空きがない
	}

	// 装備解除
	gameData.UnequipItem(slot);

	// 装備解除通知
	const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
	if (data)
	{
		FeedbackSystem::GetInstance().ShowEquipChange(data->name, false);
	}

	// 装備変更処理
	OnEquipmentChanged(slot);

	return true;
}

bool GameScene::TryUseItem(int slotIndex)
{
	if (!player_) return false;

	const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
	if (!data) return false;

	// 消費アイテムのみ使用可能
	if (data->type != ItemType::Consumable) return false;

	// 回復効果
	if (data->healAmount > 0)
	{
		// HPが満タンなら使用不可
		if (player_->GetHP() >= player_->GetMaxHP())
		{
			FeedbackSystem::GetInstance().ShowWarning("HPは既に満タン!");
			return false;
		}

		int healedAmount = player_->Heal(data->healAmount);

		// 回復エフェクト
		FeedbackSystem::GetInstance().ShowHeal(player_->GetPosition(), healedAmount);
		FeedbackSystem::GetInstance().TriggerHealEffect(0.4f);
		FeedbackSystem::GetInstance().ShowItemUse(data->name);
	}

	// スタック減少
	GetInventory().RemoveItem(slotIndex, 1);
	return true;
}

void GameScene::DrawItemTooltip() const
{
	if (isDragging_) return;  // ドラッグ中は表示しない

	auto& input = InputManager::GetInstance();
	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	int itemId = 0;
	const char* slotName = nullptr;

	// インベントリ上のアイテムをチェック
	int gridX, gridY;
	if (ScreenToInventoryGrid(mx, my, gridX, gridY))
	{
		int slotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
		if (slotIndex >= 0)
		{
			const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
			if (slot && !slot->IsEmpty())
			{
				itemId = slot->itemId;
			}
		}
	}

	// 装備スロット上のアイテムをチェック
	if (itemId == 0)
	{
		int equipSlotIndex = GetEquipSlotAtPosition(mx, my);
		if (equipSlotIndex >= 0)
		{
			itemId = GameData::GetInstance().GetEquippedItem(static_cast<EquipSlot>(equipSlotIndex));
			if (equipSlotIndex == 0) slotName = "Weapon";
			else if (equipSlotIndex == 1) slotName = "Shield";
			else if (equipSlotIndex == 2) slotName = "Armor";
		}
	}

	// コンテナ内のアイテムをチェック
	if (itemId == 0 && openContainer_)
	{
		int gridX, gridY;
		if (ScreenToContainerGrid(mx, my, gridX, gridY))
		{
			int slotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = openContainer_->GetInventory().GetSlot(slotIndex);
				if (slot && !slot->IsEmpty())
				{
					itemId = slot->itemId;
				}
			}
		}
	}

	if (itemId == 0) return;

	// アイテム情報取得
	const ItemData* item = ItemDatabase::GetInstance().GetItem(itemId);
	if (!item) return;

	// ツールチップ描画位置
	int tooltipX = mx + 15;
	int tooltipY = my + 15;
	int tooltipW = 200;
	int tooltipH = 80;

	// 武器・防具・消費アイテム・ギアは追加情報があるのでツールチップを大きく
	const WeaponData* weapon = CombatSystem::GetInstance().GetWeaponData(itemId);
	const ArmorData* armor = CombatSystem::GetInstance().GetArmorData(itemId);
	const BackpackData* backpack = CombatSystem::GetInstance().GetBackpackData(itemId);
	const RigData* rig = CombatSystem::GetInstance().GetRigData(itemId);
	bool isConsumable = (item->type == ItemType::Consumable);
	if (weapon || armor) tooltipH = 120;
	else if (backpack || rig) tooltipH = 100;
	else if (isConsumable && item->healAmount > 0) tooltipH = 100;

	// 画面外にはみ出る場合は調整
	if (tooltipX + tooltipW > static_cast<int>(SCREEN_WIDTH)) tooltipX = mx - tooltipW - 5;
	if (tooltipY + tooltipH > static_cast<int>(SCREEN_HEIGHT)) tooltipY = my - tooltipH - 5;

	// 背景
	DrawBox(tooltipX, tooltipY, tooltipX + tooltipW, tooltipY + tooltipH, GetColor(20, 20, 30), TRUE);
	DrawBox(tooltipX, tooltipY, tooltipX + tooltipW, tooltipY + tooltipH, GetColor(100, 100, 120), FALSE);

	int textY = tooltipY + 5;

	// アイテム名
	unsigned int nameColor = GetColor(255, 255, 255);
	if (item->type == ItemType::Weapon) nameColor = GetColor(255, 150, 150);
	else if (item->type == ItemType::Armor) nameColor = GetColor(150, 150, 255);
	else if (item->type == ItemType::Valuable) nameColor = GetColor(255, 215, 0);
	DrawString(tooltipX + 5, textY, item->name.c_str(), nameColor);
	textY += 18;

	// 説明
	if (!item->description.empty())
	{
		DrawString(tooltipX + 5, textY, item->description.c_str(), GetColor(180, 180, 180));
		textY += 16;
	}

	// 重量
	DrawFormatString(tooltipX + 5, textY, GetColor(150, 150, 150), "Weight: %.2f kg", item->weight);
	textY += 16;

	// 武器ステータス
	if (weapon)
	{
		const char* atkTypeName = "Unknown";
		switch (weapon->attackType)
		{
		case AttackType::Slash: atkTypeName = "Slash"; break;
		case AttackType::Pierce: atkTypeName = "Pierce"; break;
		case AttackType::Blunt: atkTypeName = "Blunt"; break;
		default: break;
		}
		DrawFormatString(tooltipX + 5, textY, GetColor(255, 200, 100), "DMG: %d  Type: %s", weapon->baseDamage, atkTypeName);
		textY += 16;
		DrawFormatString(tooltipX + 5, textY, GetColor(200, 200, 150), "Range: %.0f  Speed: %.1f", weapon->range, weapon->attackSpeed);

		// 遠距離武器情報
		if (weapon->IsRanged())
		{
			textY += 16;
			const char* categoryName = weapon->category == WeaponCategory::Bow ? "Bow" : "Magic";
			DrawFormatString(tooltipX + 5, textY, GetColor(150, 200, 255), "Type: %s", categoryName);

			if (weapon->RequiresAmmo())
			{
				const ItemData* ammoData = ItemDatabase::GetInstance().GetItemData(weapon->ammoItemId);
				if (ammoData)
				{
					textY += 16;
					DrawFormatString(tooltipX + 5, textY, GetColor(200, 180, 150),
						"Ammo: %s x%d", ammoData->name.c_str(), weapon->ammoPerShot);
				}
			}
		}
	}

	// 防具ステータス
	if (armor)
	{
		DrawFormatString(tooltipX + 5, textY, GetColor(100, 200, 255), "DEF: %d", armor->baseDefense);
		textY += 16;
		DrawFormatString(tooltipX + 5, textY, GetColor(150, 200, 150), "Slash:%.0f%% Pierce:%.0f%% Blunt:%.0f%%",
			armor->resistances.slash * 100, armor->resistances.pierce * 100, armor->resistances.blunt * 100);
		if (armor->isShield && armor->blockChance > 0)
		{
			textY += 16;
			DrawFormatString(tooltipX + 5, textY, GetColor(200, 200, 100), "Block: %.0f%%", armor->blockChance * 100);
		}
	}

	// 消費アイテムステータス
	if (isConsumable)
	{
		if (item->healAmount > 0)
		{
			DrawFormatString(tooltipX + 5, textY, GetColor(100, 255, 100), "Heal: +%d HP", item->healAmount);
			textY += 16;
		}
		DrawString(tooltipX + 5, textY, "[Right-click to use]", GetColor(255, 255, 100));
	}

	// バックパックステータス
	if (backpack)
	{
		DrawFormatString(tooltipX + 5, textY, GetColor(180, 140, 100),
			"Storage: +%dx%d", backpack->extraGridWidth, backpack->extraGridHeight);
		textY += 16;
		if (backpack->speedPenalty > 0)
		{
			DrawFormatString(tooltipX + 5, textY, GetColor(255, 150, 100),
				"Speed: -%.0f%%", backpack->speedPenalty * 100);
		}
	}

	// リグステータス
	if (rig)
	{
		DrawFormatString(tooltipX + 5, textY, GetColor(140, 180, 140),
			"Quick Slots: %d", rig->quickSlots);
		textY += 16;
		if (rig->extraGridWidth > 0 || rig->extraGridHeight > 0)
		{
			DrawFormatString(tooltipX + 5, textY, GetColor(180, 180, 140),
				"Storage: +%dx%d", rig->extraGridWidth, rig->extraGridHeight);
		}
	}
}

void GameScene::UpdateCamera()
{
	if (!player_) return;

	// プレイヤーを画面中央に配置
	cameraOffset_.x = player_->GetPosition().x - SCREEN_WIDTH / 2.0f;
	cameraOffset_.y = player_->GetPosition().y - SCREEN_HEIGHT / 2.0f;

	// マップ境界でクランプ
	if (cameraOffset_.x < 0.0f)
	{
		cameraOffset_.x = 0.0f;
	}
	if (cameraOffset_.y < 0.0f)
	{
		cameraOffset_.y = 0.0f;
	}
	if (cameraOffset_.x > map_.GetWidth() - SCREEN_WIDTH)
	{
		cameraOffset_.x = map_.GetWidth() - SCREEN_WIDTH;
	}
	if (cameraOffset_.y > map_.GetHeight() - SCREEN_HEIGHT)
	{
		cameraOffset_.y = map_.GetHeight() - SCREEN_HEIGHT;
	}
}

Vector2 GameScene::WorldToScreen(const Vector2& worldPos) const
{
	return Vector2(worldPos.x - cameraOffset_.x, worldPos.y - cameraOffset_.y);
}

Vector2 GameScene::ScreenToWorld(const Vector2& screenPos) const
{
	return Vector2(screenPos.x + cameraOffset_.x, screenPos.y + cameraOffset_.y);
}

void GameScene::SpawnContainers()
{
	containers_.clear();

	// エリア1: スタート付近（左上）
	auto crate1 = std::make_unique<LootContainer>(ContainerType::Crate, Vector2(300.0f, 200.0f));
	crate1->GenerateLoot(rng_);
	containers_.push_back(std::move(crate1));

	auto medical1 = std::make_unique<LootContainer>(ContainerType::MedicalCrate, Vector2(250.0f, 350.0f));
	medical1->GenerateLoot(rng_);
	containers_.push_back(std::move(medical1));

	// エリア2: 中央左
	auto barrel1 = std::make_unique<LootContainer>(ContainerType::Barrel, Vector2(450.0f, 650.0f));
	barrel1->GenerateLoot(rng_);
	containers_.push_back(std::move(barrel1));

	auto crate2 = std::make_unique<LootContainer>(ContainerType::Crate, Vector2(650.0f, 800.0f));
	crate2->GenerateLoot(rng_);
	containers_.push_back(std::move(crate2));

	auto weaponCrate0 = std::make_unique<LootContainer>(ContainerType::WeaponCrate, Vector2(500.0f, 700.0f));
	weaponCrate0->GenerateLoot(rng_);
	containers_.push_back(std::move(weaponCrate0));

	// ギア箱（スタート近く）
	auto gear1 = std::make_unique<LootContainer>(ContainerType::GearCrate, Vector2(600.0f, 500.0f));
	gear1->GenerateLoot(rng_);
	containers_.push_back(std::move(gear1));

	// 遠距離武器箱
	auto ranged1 = std::make_unique<LootContainer>(ContainerType::RangedCrate, Vector2(800.0f, 300.0f));
	ranged1->GenerateLoot(rng_);
	containers_.push_back(std::move(ranged1));

	// エリア3: 上部中央
	auto weaponCrate1 = std::make_unique<LootContainer>(ContainerType::WeaponCrate, Vector2(1150.0f, 350.0f));
	weaponCrate1->GenerateLoot(rng_);
	containers_.push_back(std::move(weaponCrate1));

	auto treasure0 = std::make_unique<LootContainer>(ContainerType::Treasure, Vector2(900.0f, 350.0f));
	treasure0->GenerateLoot(rng_);
	containers_.push_back(std::move(treasure0));

	// エリア4: 中央
	auto crate3 = std::make_unique<LootContainer>(ContainerType::Crate, Vector2(1450.0f, 900.0f));
	crate3->GenerateLoot(rng_);
	containers_.push_back(std::move(crate3));

	auto barrel2 = std::make_unique<LootContainer>(ContainerType::Barrel, Vector2(1650.0f, 1050.0f));
	barrel2->GenerateLoot(rng_);
	containers_.push_back(std::move(barrel2));

	auto medical2 = std::make_unique<LootContainer>(ContainerType::MedicalCrate, Vector2(1500.0f, 950.0f));
	medical2->GenerateLoot(rng_);
	containers_.push_back(std::move(medical2));

	auto armor1 = std::make_unique<LootContainer>(ContainerType::ArmorCrate, Vector2(1550.0f, 1100.0f));
	armor1->GenerateLoot(rng_);
	containers_.push_back(std::move(armor1));

	// ギア箱（中央エリア）
	auto gear2 = std::make_unique<LootContainer>(ContainerType::GearCrate, Vector2(1300.0f, 600.0f));
	gear2->GenerateLoot(rng_);
	containers_.push_back(std::move(gear2));

	// 遠距離武器箱（中央）
	auto ranged2 = std::make_unique<LootContainer>(ContainerType::RangedCrate, Vector2(1800.0f, 500.0f));
	ranged2->GenerateLoot(rng_);
	containers_.push_back(std::move(ranged2));

	// エリア5: 右上
	auto treasure1 = std::make_unique<LootContainer>(ContainerType::Treasure, Vector2(2450.0f, 400.0f));
	treasure1->GenerateLoot(rng_);
	containers_.push_back(std::move(treasure1));

	auto medical3 = std::make_unique<LootContainer>(ContainerType::MedicalCrate, Vector2(2300.0f, 500.0f));
	medical3->GenerateLoot(rng_);
	containers_.push_back(std::move(medical3));

	// エリア6: 下部中央
	auto weaponCrate2 = std::make_unique<LootContainer>(ContainerType::WeaponCrate, Vector2(900.0f, 1700.0f));
	weaponCrate2->GenerateLoot(rng_);
	containers_.push_back(std::move(weaponCrate2));

	auto barrel3 = std::make_unique<LootContainer>(ContainerType::Barrel, Vector2(1050.0f, 1900.0f));
	barrel3->GenerateLoot(rng_);
	containers_.push_back(std::move(barrel3));

	auto armor2 = std::make_unique<LootContainer>(ContainerType::ArmorCrate, Vector2(1000.0f, 1850.0f));
	armor2->GenerateLoot(rng_);
	containers_.push_back(std::move(armor2));

	// エリア7: 中央右
	auto crate4 = std::make_unique<LootContainer>(ContainerType::Crate, Vector2(2100.0f, 1300.0f));
	crate4->GenerateLoot(rng_);
	containers_.push_back(std::move(crate4));

	auto armor3 = std::make_unique<LootContainer>(ContainerType::ArmorCrate, Vector2(2150.0f, 1350.0f));
	armor3->GenerateLoot(rng_);
	containers_.push_back(std::move(armor3));

	auto medical4 = std::make_unique<LootContainer>(ContainerType::MedicalCrate, Vector2(2250.0f, 1400.0f));
	medical4->GenerateLoot(rng_);
	containers_.push_back(std::move(medical4));

	// ギア箱（後半エリア）
	auto gear3 = std::make_unique<LootContainer>(ContainerType::GearCrate, Vector2(2000.0f, 1500.0f));
	gear3->GenerateLoot(rng_);
	containers_.push_back(std::move(gear3));

	auto gear4 = std::make_unique<LootContainer>(ContainerType::GearCrate, Vector2(2500.0f, 1800.0f));
	gear4->GenerateLoot(rng_);
	containers_.push_back(std::move(gear4));

	// 弾薬補給
	auto ranged3 = std::make_unique<LootContainer>(ContainerType::RangedCrate, Vector2(2200.0f, 1000.0f));
	ranged3->GenerateLoot(rng_);
	containers_.push_back(std::move(ranged3));

	// エリア8: 右下（出口付近）
	auto treasure2 = std::make_unique<LootContainer>(ContainerType::Treasure, Vector2(2950.0f, 2150.0f));
	treasure2->GenerateLoot(rng_);
	containers_.push_back(std::move(treasure2));

	auto treasure3 = std::make_unique<LootContainer>(ContainerType::Treasure, Vector2(2850.0f, 2000.0f));
	treasure3->GenerateLoot(rng_);
	containers_.push_back(std::move(treasure3));
}

void GameScene::DrawContainers() const
{
	for (const auto& container : containers_)
	{
		container->Draw(cameraOffset_);
	}
}

void GameScene::UpdateContainerInteraction()
{
	if (!player_) return;

	auto& input = InputManager::GetInstance();

	// Eキーでコンテナを開く
	if (input.IsKeyTriggered(KEY_INPUT_E))
	{
		LootContainer* nearest = GetNearestContainer();
		if (nearest && !nearest->IsEmpty())
		{
			openContainer_ = nearest;
			showInventory_ = true;
		}
	}
}

void GameScene::UpdateContainerUI()
{
	if (!openContainer_) return;

	auto& input = InputManager::GetInstance();
	Vector2 mousePos = input.GetMousePosition();
	int mx = static_cast<int>(mousePos.x);
	int my = static_cast<int>(mousePos.y);

	// コンテナを閉じる（範囲外に出た or ESC）
	if (!openContainer_->IsInRange(player_->GetPosition(), player_->GetRadius()) ||
		input.IsKeyTriggered(KEY_INPUT_ESCAPE))
	{
		if (openContainer_->IsEmpty())
		{
			openContainer_->MarkAsLooted();
		}
		openContainer_ = nullptr;
		isDraggingFromContainer_ = false;
		dragContainerSlotIndex_ = -1;
		return;
	}

	// ドラッグ中の回転
	if ((isDragging_ || isDraggingFromContainer_) && input.IsKeyTriggered(KEY_INPUT_R))
	{
		dragRotated_ = !dragRotated_;
	}

	// 左クリックでドラッグ開始
	if (input.IsMouseTriggered(0))
	{
		if (!isDragging_ && !isDraggingFromContainer_)
		{
			// コンテナからドラッグ開始
			int gridX, gridY;
			if (ScreenToContainerGrid(mx, my, gridX, gridY))
			{
				int slotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
				if (slotIndex >= 0)
				{
					const InventorySlot* slot = openContainer_->GetInventory().GetSlot(slotIndex);
					if (slot && !slot->IsEmpty())
					{
						isDraggingFromContainer_ = true;
						dragContainerSlotIndex_ = slotIndex;
						dragRotated_ = slot->isRotated;
						dragOffsetX_ = gridX - slot->gridX;
						dragOffsetY_ = gridY - slot->gridY;
					}
				}
			}
		}
	}

	// 左クリック離してドロップ
	if (input.IsMouseReleased(0))
	{
		// コンテナからドラッグ中の場合
		if (isDraggingFromContainer_ && dragContainerSlotIndex_ >= 0)
		{
			const InventorySlot* slot = openContainer_->GetInventory().GetSlot(dragContainerSlotIndex_);
			if (slot && !slot->IsEmpty())
			{
				bool placed = false;
				int itemId = slot->itemId;
				int stackCount = slot->stackCount;

				// インベントリにドロップ
				int gridX, gridY;
				if (ScreenToInventoryGrid(mx, my, gridX, gridY))
				{
					// 同じアイテムへのスタックを試みる
					int targetSlotIndex = GetInventory().GetSlotIndexAt(gridX, gridY);
					if (targetSlotIndex >= 0)
					{
						const InventorySlot* targetSlot = GetInventory().GetSlot(targetSlotIndex);
						if (targetSlot && targetSlot->itemId == itemId)
						{
							// スタックを試みる
							int added = GetInventory().AddToStack(targetSlotIndex, stackCount);
							if (added > 0)
							{
								openContainer_->GetInventory().RemoveItem(dragContainerSlotIndex_, added);

								// アイテム取得通知
								const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
								if (data)
								{
									FeedbackSystem::GetInstance().ShowItemPickup(data->name, added);
								}
								QuestSystem::GetInstance().OnItemCollected(itemId, added);
								placed = (added == stackCount);
							}
						}
					}

					// スタックできなかった場合、空きスペースに配置
					if (!placed)
					{
						// スロット情報を再取得（スタックで変わっている可能性がある）
						slot = openContainer_->GetInventory().GetSlot(dragContainerSlotIndex_);
						if (slot && !slot->IsEmpty())
						{
							int placeX = gridX - dragOffsetX_;
							int placeY = gridY - dragOffsetY_;
							if (GetInventory().CanFitItemAt(slot->itemId, placeX, placeY, dragRotated_))
							{
								GetInventory().AddItemAt(slot->itemId, slot->stackCount, placeX, placeY, dragRotated_);
								openContainer_->GetInventory().RemoveItem(dragContainerSlotIndex_, slot->stackCount);

								const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
								if (data)
								{
									FeedbackSystem::GetInstance().ShowItemPickup(data->name, slot->stackCount);
								}
								QuestSystem::GetInstance().OnItemCollected(slot->itemId, slot->stackCount);
								placed = true;
							}
						}
					}
				}

				// コンテナ内で移動/スタック
				if (!placed)
				{
					slot = openContainer_->GetInventory().GetSlot(dragContainerSlotIndex_);
					if (slot && !slot->IsEmpty() && ScreenToContainerGrid(mx, my, gridX, gridY))
					{
						// 同じアイテムへのスタックを試みる
						int targetSlotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
						if (targetSlotIndex >= 0 && targetSlotIndex != dragContainerSlotIndex_)
						{
							const InventorySlot* targetSlot = openContainer_->GetInventory().GetSlot(targetSlotIndex);
							if (targetSlot && targetSlot->itemId == slot->itemId)
							{
								openContainer_->GetInventory().TryMergeSlots(dragContainerSlotIndex_, targetSlotIndex);
								placed = true;
							}
						}

						// 通常の移動
						if (!placed)
						{
							int placeX = gridX - dragOffsetX_;
							int placeY = gridY - dragOffsetY_;
							openContainer_->GetInventory().MoveItem(dragContainerSlotIndex_, placeX, placeY, dragRotated_);
						}
					}
				}
			}

			isDraggingFromContainer_ = false;
			dragContainerSlotIndex_ = -1;
		}

		// インベントリからドラッグ中にコンテナへドロップ
		if (isDragging_ && dragSlotIndex_ >= 0)
		{
			const InventorySlot* slot = GetInventory().GetSlot(dragSlotIndex_);
			if (slot && !slot->IsEmpty())
			{
				int gridX, gridY;
				if (ScreenToContainerGrid(mx, my, gridX, gridY))
				{
					bool placed = false;
					int itemId = slot->itemId;
					int stackCount = slot->stackCount;

					// 同じアイテムへのスタックを試みる
					int targetSlotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
					if (targetSlotIndex >= 0)
					{
						const InventorySlot* targetSlot = openContainer_->GetInventory().GetSlot(targetSlotIndex);
						if (targetSlot && targetSlot->itemId == itemId)
						{
							int added = openContainer_->GetInventory().AddToStack(targetSlotIndex, stackCount);
							if (added > 0)
							{
								GetInventory().RemoveItem(dragSlotIndex_, added);
								placed = (added == stackCount);
							}
						}
					}

					// スタックできなかった場合、空きスペースに配置
					if (!placed)
					{
						slot = GetInventory().GetSlot(dragSlotIndex_);
						if (slot && !slot->IsEmpty())
						{
							int placeX = gridX - dragOffsetX_;
							int placeY = gridY - dragOffsetY_;
							if (openContainer_->GetInventory().CanFitItemAt(slot->itemId, placeX, placeY, dragRotated_))
							{
								openContainer_->GetInventory().AddItemAt(slot->itemId, slot->stackCount, placeX, placeY, dragRotated_);
								GetInventory().RemoveItem(dragSlotIndex_, slot->stackCount);
							}
						}
					}
				}
			}
			// Note: isDragging_とdragSlotIndex_のリセットはUpdateInventoryUIで行われる
		}
	}

	// 右クリック処理：コンテナ内アイテムの直接装備/使用
	if (input.IsMouseTriggered(1) && !isDragging_ && !isDraggingFromContainer_)
	{
		int gridX, gridY;
		if (ScreenToContainerGrid(mx, my, gridX, gridY))
		{
			int slotIndex = openContainer_->GetInventory().GetSlotIndexAt(gridX, gridY);
			if (slotIndex >= 0)
			{
				const InventorySlot* slot = openContainer_->GetInventory().GetSlot(slotIndex);
				if (slot && !slot->IsEmpty())
				{
					const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
					if (data)
					{
						bool handled = false;
						auto& combat = CombatSystem::GetInstance();
						auto& gameData = GameData::GetInstance();

						// 装備可能アイテムの場合、直接装備を試みる
						EquipSlot targetSlot = combat.GetEquipSlotForItem(slot->itemId);
						if (targetSlot != EquipSlot::Count)
						{
							// 現在装備を外す
							int currentEquipped = gameData.GetEquippedItem(targetSlot);
							if (currentEquipped != 0)
							{
								// 装備中のアイテムをインベントリに戻す
								if (GetInventory().AddItem(currentEquipped, 1))
								{
									gameData.SetEquippedItem(targetSlot, 0);
								}
								else
								{
									FeedbackSystem::GetInstance().ShowWarning("インベントリがいっぱい!");
									handled = true;
								}
							}

							if (!handled)
							{
								// コンテナから装備
								gameData.SetEquippedItem(targetSlot, slot->itemId);
								openContainer_->GetInventory().RemoveItem(slotIndex, 1);
								FeedbackSystem::GetInstance().ShowItemPickup(data->name + " equipped", 1);
								OnEquipmentChanged(targetSlot);
								handled = true;
							}
						}
						// 消費アイテムの場合、使用を試みる
						else if (data->type == ItemType::Consumable)
						{
							if (data->healAmount > 0 && player_)
							{
								player_->Heal(data->healAmount);
								FeedbackSystem::GetInstance().ShowHeal(
									player_->GetPosition(), data->healAmount);
								FeedbackSystem::GetInstance().ShowItemUse(data->name);
								openContainer_->GetInventory().RemoveItem(slotIndex, 1);
								handled = true;
							}
						}
					}
				}
			}
		}
	}
}

void GameScene::DrawContainerUI() const
{
	if (!openContainer_) return;

	const int cellSize = Inventory::CELL_SIZE;
	const Inventory& inv = openContainer_->GetInventory();
	const int gridW = inv.GetGridWidth();
	const int gridH = inv.GetGridHeight();
	const int ox = containerOffsetX_;
	const int oy = containerOffsetY_;

	// 背景
	DrawBox(ox - 10, oy - 30, ox + gridW * cellSize + 10, oy + gridH * cellSize + 10,
		GetColor(40, 30, 20), TRUE);
	DrawBox(ox - 10, oy - 30, ox + gridW * cellSize + 10, oy + gridH * cellSize + 10,
		GetColor(120, 100, 80), FALSE);

	// タイトル
	const char* title = "CONTAINER";
	switch (openContainer_->GetType())
	{
	case ContainerType::Crate: title = "CRATE"; break;
	case ContainerType::Barrel: title = "BARREL"; break;
	case ContainerType::WeaponCrate: title = "WEAPON CRATE"; break;
	case ContainerType::Treasure: title = "TREASURE"; break;
	case ContainerType::Corpse: title = "CORPSE"; break;
	case ContainerType::GearCrate: title = "GEAR CRATE"; break;
	case ContainerType::RangedCrate: title = "RANGED CRATE"; break;
	case ContainerType::ArmorCrate: title = "ARMOR CRATE"; break;
	case ContainerType::MedicalCrate: title = "MEDICAL CRATE"; break;
	default: break;
	}
	DrawString(ox, oy - 25, title, GetColor(255, 220, 150));
	// ヒント：右クリックで装備/使用
	DrawString(ox + gridW * cellSize - 100, oy - 25, "[RClick:Equip]", GetColor(180, 180, 150));

	// グリッド描画
	for (int y = 0; y < gridH; ++y)
	{
		for (int x = 0; x < gridW; ++x)
		{
			int px = ox + x * cellSize;
			int py = oy + y * cellSize;

			DrawBox(px, py, px + cellSize, py + cellSize,
				GetColor(60, 50, 40), TRUE);
			DrawBox(px, py, px + cellSize, py + cellSize,
				GetColor(100, 80, 60), FALSE);
		}
	}

	// アイテム描画
	for (int i = 0; i < static_cast<int>(inv.GetSlots().size()); ++i)
	{
		const InventorySlot& slot = inv.GetSlots()[i];
		if (slot.IsEmpty()) continue;

		// ドラッグ中のアイテムはスキップ
		if (isDraggingFromContainer_ && i == dragContainerSlotIndex_) continue;

		const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot.itemId);
		if (!data) continue;

		int px = ox + slot.gridX * cellSize;
		int py = oy + slot.gridY * cellSize;
		int w = slot.GetWidth(data) * cellSize;
		int h = slot.GetHeight(data) * cellSize;

		unsigned int bgColor = GetColor(100, 80, 60);
		switch (data->type)
		{
		case ItemType::Consumable:
			bgColor = GetColor(60, 100, 60);
			break;
		case ItemType::Weapon:
			bgColor = GetColor(100, 60, 60);
			break;
		case ItemType::Armor:
			bgColor = GetColor(60, 60, 100);
			break;
		case ItemType::Valuable:
			bgColor = GetColor(100, 90, 40);
			break;
		default:
			break;
		}

		DrawBox(px + 2, py + 2, px + w - 2, py + h - 2, bgColor, TRUE);
		DrawBox(px + 2, py + 2, px + w - 2, py + h - 2, GetColor(150, 150, 150), FALSE);

		std::string shortName = data->name.substr(0, 6);
		DrawString(px + 4, py + 4, shortName.c_str(), GetColor(255, 255, 255));

		if (slot.stackCount > 1)
		{
			DrawFormatString(px + w - 20, py + h - 18, GetColor(255, 255, 200),
				"x%d", slot.stackCount);
		}
	}

	// ヒント
	DrawString(ox, oy + gridH * cellSize + 5, "[Drag to take]", GetColor(200, 200, 150));
}

LootContainer* GameScene::GetNearestContainer()
{
	if (!player_) return nullptr;

	LootContainer* nearest = nullptr;
	float nearestDist = 999999.0f;

	for (auto& container : containers_)
	{
		if (container->IsInRange(player_->GetPosition(), player_->GetRadius()))
		{
			float dist = player_->GetPosition().Distance(container->GetPosition());
			if (dist < nearestDist)
			{
				nearestDist = dist;
				nearest = container.get();
			}
		}
	}

	return nearest;
}

bool GameScene::ScreenToContainerGrid(int screenX, int screenY, int& gridX, int& gridY) const
{
	if (!openContainer_) return false;

	const int cellSize = Inventory::CELL_SIZE;
	int relX = screenX - containerOffsetX_;
	int relY = screenY - containerOffsetY_;

	if (relX < 0 || relY < 0) return false;

	gridX = relX / cellSize;
	gridY = relY / cellSize;

	const Inventory& inv = openContainer_->GetInventory();
	if (gridX >= inv.GetGridWidth() || gridY >= inv.GetGridHeight())
		return false;

	return true;
}

void GameScene::CreateCorpseAt(const Vector2& pos)
{
	auto corpse = std::make_unique<LootContainer>(ContainerType::Corpse, pos);
	corpse->GenerateLoot(rng_);
	containers_.push_back(std::move(corpse));
}

bool GameScene::TryAssignQuickSlot(int slotIndex)
{
	const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
	if (!data) return false;

	// 消費アイテムのみクイックスロットに登録可能
	if (data->type != ItemType::Consumable)
	{
		FeedbackSystem::GetInstance().ShowWarning("消費アイテムのみ!");
		return false;
	}

	int quickSlotCount = GetInventory().GetQuickSlotCount();
	if (quickSlotCount <= 0)
	{
		FeedbackSystem::GetInstance().ShowWarning("まずリグを装備!");
		return false;
	}

	// 空いているクイックスロットを探す、または同じアイテムがあるスロットを探す
	int targetSlot = -1;
	for (int i = 0; i < quickSlotCount; ++i)
	{
		int existingItem = GetInventory().GetQuickSlotItem(i);
		if (existingItem == slot->itemId)
		{
			// 既に登録済み
			FeedbackSystem::GetInstance().ShowNotification("登録済み", NotificationType::Info);
			return true;
		}
		if (existingItem == 0 && targetSlot < 0)
		{
			targetSlot = i;
		}
	}

	if (targetSlot < 0)
	{
		// 空きがない場合は最初のスロットに上書き
		targetSlot = 0;
	}

	GetInventory().SetQuickSlotItem(targetSlot, slot->itemId);
	FeedbackSystem::GetInstance().ShowNotification(
		data->name + " -> スロット" + std::to_string(targetSlot + 1),
		NotificationType::Info
	);
	return true;
}

bool GameScene::TryAssignQuickSlotTo(int slotIndex, int quickSlotIndex)
{
	const InventorySlot* slot = GetInventory().GetSlot(slotIndex);
	if (!slot || slot->IsEmpty()) return false;

	const ItemData* data = ItemDatabase::GetInstance().GetItemData(slot->itemId);
	if (!data) return false;

	// 消費アイテムのみクイックスロットに登録可能
	if (data->type != ItemType::Consumable)
	{
		FeedbackSystem::GetInstance().ShowWarning("消費アイテムのみ!");
		return false;
	}

	int quickSlotCount = GetInventory().GetQuickSlotCount();
	if (quickSlotCount <= 0)
	{
		FeedbackSystem::GetInstance().ShowWarning("まずリグを装備!");
		return false;
	}

	// 指定されたスロットが範囲外
	if (quickSlotIndex < 0 || quickSlotIndex >= quickSlotCount)
	{
		FeedbackSystem::GetInstance().ShowWarning("スロットがありません!");
		return false;
	}

	GetInventory().SetQuickSlotItem(quickSlotIndex, slot->itemId);
	FeedbackSystem::GetInstance().ShowNotification(
		data->name + " -> スロット" + std::to_string(quickSlotIndex + 1),
		NotificationType::Info
	);
	return true;
}

void GameScene::DrawQuickSlotsUI() const
{
	auto& gameData = GameData::GetInstance();
	const Inventory& inv = GetInventory();
	int quickSlotCount = inv.GetQuickSlotCount();

	if (quickSlotCount <= 0)
	{
		// リグ未装備時のヒント
		if (showInventory_)
		{
			int hintX = static_cast<int>(SCREEN_WIDTH) / 2 - 100;
			int hintY = static_cast<int>(SCREEN_HEIGHT) - 50;
			DrawString(hintX, hintY, "リグを装備するとクイックスロットが使えます", GetColor(100, 140, 100));
		}
		return;
	}

	// クイックスロットを画面下部に表示
	int slotSize = 40;
	int gap = 5;
	int totalWidth = quickSlotCount * slotSize + (quickSlotCount - 1) * gap;
	int startX = (static_cast<int>(SCREEN_WIDTH) - totalWidth) / 2;
	int startY = static_cast<int>(SCREEN_HEIGHT) - 70;

	// 背景
	DrawBox(startX - 5, startY - 25, startX + totalWidth + 5, startY + slotSize + 20,
		GetColor(30, 30, 40), TRUE);
	DrawBox(startX - 5, startY - 25, startX + totalWidth + 5, startY + slotSize + 20,
		GetColor(100, 100, 120), FALSE);

	// リグ説明テキスト
	DrawString(startX, startY - 20, "クイックスロット Ctrl+1-6", GetColor(140, 180, 140));

	for (int i = 0; i < quickSlotCount; ++i)
	{
		int x = startX + i * (slotSize + gap);
		int y = startY;

		// スロット背景
		DrawBox(x, y, x + slotSize, y + slotSize, GetColor(50, 60, 50), TRUE);
		DrawBox(x, y, x + slotSize, y + slotSize, GetColor(80, 100, 80), FALSE);

		// キー番号
		DrawFormatString(x + slotSize / 2 - 4, y + slotSize + 2, GetColor(150, 150, 150), "%d", i + 1);

		// アイテム表示
		int itemId = inv.GetQuickSlotItem(i);
		if (itemId != 0)
		{
			const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
			if (data)
			{
				std::string shortName = data->name.substr(0, 5);
				DrawString(x + 2, y + 2, shortName.c_str(), GetColor(255, 255, 255));

				// インベントリ内の残り数を表示
				int count = 0;
				for (const auto& slot : inv.GetSlots())
				{
					if (slot.itemId == itemId)
						count += slot.stackCount;
				}
				if (count > 0)
				{
					DrawFormatString(x + slotSize - 15, y + slotSize - 15,
						GetColor(255, 255, 200), "%d", count);
				}
			}
		}
	}
}

void GameScene::OnEquipmentChanged(EquipSlot slot)
{
	if (slot == EquipSlot::Backpack)
	{
		UpdateInventorySize();
	}
	else if (slot == EquipSlot::Rig)
	{
		UpdateQuickSlots();
	}
}

void GameScene::UpdateInventorySize()
{
	auto& gameData = GameData::GetInstance();
	auto& combat = CombatSystem::GetInstance();

	int baseWidth = Inventory::BASE_GRID_WIDTH;
	int baseHeight = Inventory::BASE_GRID_HEIGHT;
	int extraWidth = 0;
	int extraHeight = 0;

	// バックパックによる追加容量
	int backpackId = gameData.GetEquippedItem(EquipSlot::Backpack);
	if (backpackId != 0)
	{
		const BackpackData* bp = combat.GetBackpackData(backpackId);
		if (bp)
		{
			extraWidth = bp->extraGridWidth;
			extraHeight = bp->extraGridHeight;
		}
	}

	// リグによる追加容量
	int rigId = gameData.GetEquippedItem(EquipSlot::Rig);
	if (rigId != 0)
	{
		const RigData* rig = combat.GetRigData(rigId);
		if (rig)
		{
			extraWidth += rig->extraGridWidth;
			extraHeight += rig->extraGridHeight;
		}
	}

	// インベントリサイズを更新
	int newWidth = baseWidth + extraWidth;
	int newHeight = baseHeight + extraHeight;
	GetInventory().Resize(newWidth, newHeight);
}

void GameScene::UpdateQuickSlots()
{
	auto& gameData = GameData::GetInstance();
	auto& combat = CombatSystem::GetInstance();

	int quickSlots = 0;

	int rigId = gameData.GetEquippedItem(EquipSlot::Rig);
	if (rigId != 0)
	{
		const RigData* rig = combat.GetRigData(rigId);
		if (rig)
		{
			quickSlots = rig->quickSlots;
		}
	}

	GetInventory().SetQuickSlotCount(quickSlots);
}

void GameScene::HandleQuickSlotInput()
{
	if (!player_) return;
	if (showInventory_) return;  // インベントリ開いてる時は無効

	auto& input = InputManager::GetInstance();
	auto& inv = GetInventory();
	int quickSlotCount = inv.GetQuickSlotCount();

	// Ctrlキーが押されていなければ無効
	bool ctrlPressed = input.IsKeyPressed(KEY_INPUT_LCONTROL) || input.IsKeyPressed(KEY_INPUT_RCONTROL);
	if (!ctrlPressed) return;

	// Ctrl+1-6キーでクイックスロット使用
	for (int i = 0; i < quickSlotCount && i < 6; ++i)
	{
		if (input.IsKeyTriggered(KEY_INPUT_1 + i))
		{
			int itemId = inv.GetQuickSlotItem(i);
			if (itemId != 0)
			{
				const ItemData* data = ItemDatabase::GetInstance().GetItemData(itemId);
				if (data && data->type == ItemType::Consumable)
				{
					// 消費アイテムを使用
					for (int j = 0; j < static_cast<int>(inv.GetSlots().size()); ++j)
					{
						const InventorySlot* slot = inv.GetSlot(j);
						if (slot && slot->itemId == itemId)
						{
							if (TryUseItem(j))
							{
								// 使用成功
								break;
							}
						}
					}
				}
			}
		}
	}
}

bool GameScene::TryRangedAttack()
{
	if (!player_) return false;

	auto& combat = CombatSystem::GetInstance();
	int weaponId = player_->GetEquippedItemId(EquipSlot::Weapon);
	const WeaponData* weapon = combat.GetWeaponData(weaponId);

	if (!weapon || !weapon->IsRanged()) return false;

	// 弾薬チェック
	if (weapon->RequiresAmmo())
	{
		bool hasAmmo = false;
		for (const auto& slot : GetInventory().GetSlots())
		{
			if (slot.itemId == weapon->ammoItemId && slot.stackCount >= weapon->ammoPerShot)
			{
				hasAmmo = true;
				break;
			}
		}

		if (!hasAmmo)
		{
			FeedbackSystem::GetInstance().ShowWarning("弾切れ!");
			return false;
		}

		// 弾薬消費
		for (int i = 0; i < static_cast<int>(GetInventory().GetSlots().size()); ++i)
		{
			const InventorySlot* slot = GetInventory().GetSlot(i);
			if (slot && slot->itemId == weapon->ammoItemId && slot->stackCount >= weapon->ammoPerShot)
			{
				GetInventory().RemoveItem(i, weapon->ammoPerShot);
				break;
			}
		}
	}

	// 発射体タイプを決定
	ProjectileType projType = ProjectileType::Arrow;
	if (weapon->category == WeaponCategory::Magic)
	{
		// Fire WandならFireball、それ以外はMagicBolt
		if (weaponId == 16)
		{
			projType = ProjectileType::Fireball;
		}
		else
		{
			projType = ProjectileType::MagicBolt;
		}
	}

	// 発射体生成
	SpawnProjectile(
		projType,
		player_->GetPosition(),
		player_->GetFacingDirection(),
		weapon->baseDamage,
		weapon->projectileSpeed,
		weapon->range,
		true
	);

	return true;
}

void GameScene::SpawnProjectile(ProjectileType type, const Vector2& pos, const Vector2& dir,
	int damage, float speed, float range, bool isPlayerOwned)
{
	auto proj = std::make_unique<Projectile>(type, pos, dir, damage, speed, range, isPlayerOwned);
	projectiles_.push_back(std::move(proj));
}

void GameScene::UpdateProjectiles()
{
	float deltaTime = 1.0f / 60.0f;

	for (auto& proj : projectiles_)
	{
		if (!proj->IsActive()) continue;

		proj->Update(deltaTime);

		// 壁との衝突チェック
		if (map_.IsCollidingWithWall(proj->GetPosition(), proj->GetRadius()))
		{
			proj->Deactivate();
			continue;
		}

		// プレイヤー発射体の敵へのヒット判定
		if (proj->IsPlayerOwned())
		{
			for (auto& enemy : enemies_)
			{
				if (!enemy->IsAlive()) continue;
				if (enemy->IsInvincible()) continue;  // 無敵中はスキップ
				if (proj->IsPiercing() && proj->HasHitTarget(enemy.get())) continue;

				float dist = proj->GetPosition().Distance(enemy->GetPosition());
				if (dist <= proj->GetRadius() + enemy->GetRadius())
				{
					enemy->TakeDamage(proj->GetDamage());
					FeedbackSystem::GetInstance().ShowDamage(enemy->GetPosition(), proj->GetDamage(), false);

					if (proj->IsPiercing())
					{
						proj->AddHitTarget(enemy.get());
					}
					else
					{
						proj->Deactivate();
						break;
					}
				}
			}
		}
		else
		{
			// 敵発射体のプレイヤーへのヒット判定
			if (player_ && player_->IsAlive() && !player_->IsInvincible())
			{
				float dist = proj->GetPosition().Distance(player_->GetPosition());
				if (dist <= proj->GetRadius() + player_->GetRadius())
				{
					player_->TakeDamage(proj->GetDamage());
					FeedbackSystem::GetInstance().ShowDamage(player_->GetPosition(), proj->GetDamage(), false);
					FeedbackSystem::GetInstance().TriggerDamageEffect(0.3f);
					proj->Deactivate();
				}
			}
		}
	}

	// 非アクティブな発射体を削除
	projectiles_.erase(
		std::remove_if(projectiles_.begin(), projectiles_.end(),
			[](const std::unique_ptr<Projectile>& p) { return !p->IsActive(); }),
		projectiles_.end()
	);
}

void GameScene::DrawProjectiles() const
{
	for (const auto& proj : projectiles_)
	{
		proj->Draw(cameraOffset_);
	}
}

void GameScene::DrawQuestUI() const
{
	if (showInventory_) return;  // インベントリ表示中は非表示

	const auto& quests = QuestSystem::GetInstance().GetActiveQuests();
	if (quests.empty()) return;

	// 右上にクエストUI表示（1280px画面用）
	int startX = static_cast<int>(SCREEN_WIDTH) - 220;
	int startY = 10;
	int width = 210;

	// 背景
	int height = 20;
	for (const auto& quest : quests)
	{
		if (!quest.isActive) continue;
		height += 18;
		for (const auto& obj : quest.objectives)
		{
			height += 14;
		}
		height += 5;
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
	DrawBox(startX - 5, startY - 5, startX + width + 5, startY + height,
		GetColor(20, 20, 30), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawBox(startX - 5, startY - 5, startX + width + 5, startY + height,
		GetColor(80, 80, 100), FALSE);

	DrawString(startX, startY, "クエスト", GetColor(255, 220, 100));

	int y = startY + 18;

	for (const auto& quest : quests)
	{
		if (!quest.isActive) continue;

		// クエスト名
		unsigned int nameColor = quest.isCompleted ? GetColor(100, 255, 100) : GetColor(255, 255, 255);
		std::string questName = quest.name;
		if (quest.isCompleted)
		{
			questName += " 完了";
		}
		if (questName.length() > 25)
		{
			questName = questName.substr(0, 22) + "...";
		}
		DrawString(startX, y, questName.c_str(), nameColor);
		y += 16;

		// 目標
		for (const auto& obj : quest.objectives)
		{
			unsigned int objColor = obj.isCompleted ? GetColor(80, 200, 80) : GetColor(180, 180, 180);

			std::string progress;
			if (obj.requiredCount > 1)
			{
				progress = " " + std::to_string(obj.currentCount) + "/" + std::to_string(obj.requiredCount);
			}

			std::string objText = "  - " + obj.description + progress;
			if (objText.length() > 28)
			{
				objText = objText.substr(0, 25) + "...";
			}

			// チェックマーク
			if (obj.isCompleted)
			{
				DrawString(startX, y, "  *", objColor);
				DrawString(startX + 20, y, (obj.description + progress).c_str(), objColor);
			}
			else
			{
				DrawString(startX, y, "  -", objColor);
				DrawString(startX + 20, y, (obj.description + progress).c_str(), objColor);
			}
			y += 14;
		}

		y += 5;
	}
}
