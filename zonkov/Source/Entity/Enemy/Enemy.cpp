#include "Enemy.h"
#include "Entity/Player/Player.h"
#include "System/Map/MapSystem.h"
#include "System/Resource/ResourceManager.h"
#include <DxLib.h>
#include <cmath>

Enemy::Enemy(EnemyType type, const Vector2& startPos)
	: type_(type)
	, state_(EnemyState::Idle)
	, target_(nullptr)
	, map_(nullptr)
	, currentPatrolIndex_(0)
	, patrolWaitTimer_(0.0f)
	, patrolWaitDuration_(2.0f)
	, detectionRange_(150.0f)
	, loseAggroRange_(250.0f)
	, attackRange_(35.0f)
	, attackType_(AttackType::Blunt)
	, attackDamage_(10)
	, attackCooldown_(1.0f)
	, attackTimer_(0.0f)
	, isAttacking_(false)
	, attackDuration_(0.3f)
	, hasHitThisAttack_(false)
	, moveSpeed_(80.0f)
	, chaseSpeedMult_(1.5f)
	, facingDirection_(1.0f, 0.0f)
	, invincibilityTimer_(0.0f)
{
	position_ = startPos;
	radius_ = 14.0f;
	maxHP_ = 50;
	hp_ = maxHP_;
}

void Enemy::Update(float deltaTime)
{
	if (!IsAlive()) return;

	// 攻撃クールダウン
	if (attackTimer_ > 0.0f)
	{
		attackTimer_ -= deltaTime;
	}

	// 無敵時間の更新
	if (invincibilityTimer_ > 0.0f)
	{
		invincibilityTimer_ -= deltaTime;
		if (invincibilityTimer_ < 0.0f)
		{
			invincibilityTimer_ = 0.0f;
		}
	}

	// 攻撃アニメーション終了
	if (isAttacking_)
	{
		if (attackTimer_ <= attackCooldown_ - attackDuration_)
		{
			isAttacking_ = false;
			hasHitThisAttack_ = false;
		}
	}

	// 状態別更新
	switch (state_)
	{
	case EnemyState::Idle:
		UpdateIdle(deltaTime);
		break;
	case EnemyState::Patrol:
		UpdatePatrol(deltaTime);
		break;
	case EnemyState::Chase:
		UpdateChase(deltaTime);
		break;
	case EnemyState::Attack:
		UpdateAttack(deltaTime);
		break;
	default:
		break;
	}
}

void Enemy::Draw(const Vector2& cameraOffset) const
{
	if (!IsAlive()) return;

	// 無敵中は点滅（0.1秒ごとに表示/非表示を切り替え）
	if (IsInvincible())
	{
		int blinkPhase = static_cast<int>(invincibilityTimer_ * 10) % 2;
		if (blinkPhase == 0)
		{
			// 非表示フレーム - 半透明で描画
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
		}
	}

	int ox = static_cast<int>(cameraOffset.x);
	int oy = static_cast<int>(cameraOffset.y);
	int px = static_cast<int>(position_.x) - ox;
	int py = static_cast<int>(position_.y) - oy;
	int r = static_cast<int>(radius_);

	auto& res = ResourceManager::GetInstance();
	bool isAggressive = (state_ == EnemyState::Chase || state_ == EnemyState::Attack);
	const char* imgKey = isAggressive ? "enemy_melee_aggro" : "enemy_melee";
	int handle = res.GetGraph(imgKey);

	// フォールバック
	if (handle == -1 && isAggressive)
	{
		handle = res.GetGraph("enemy_melee");
	}

	if (handle != -1)
	{
		// 画像描画
		int imgW, imgH;
		GetGraphSize(handle, &imgW, &imgH);

		if (facingDirection_.x < 0)
		{
			DrawTurnGraph(px - imgW / 2, py - imgH / 2, handle, TRUE);
		}
		else
		{
			DrawGraph(px - imgW / 2, py - imgH / 2, handle, TRUE);
		}
	}
	else
	{
		// フォールバック: デバッグ描画
		unsigned int color = GetColor(200, 50, 50);
		if (state_ == EnemyState::Chase)
		{
			color = GetColor(255, 100, 50);
		}
		else if (isAttacking_)
		{
			color = GetColor(255, 50, 255);
		}

		DrawCircle(px, py, r, color, TRUE);
		DrawCircle(px, py, r, GetColor(150, 30, 30), FALSE);

		// 向き表示
		int dirX = px + static_cast<int>(facingDirection_.x * radius_);
		int dirY = py + static_cast<int>(facingDirection_.y * radius_);
		DrawLine(px, py, dirX, dirY, GetColor(255, 255, 255));
	}

	// 無敵の点滅終了時にブレンドモードをリセット
	if (IsInvincible())
	{
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// HPバー（常に表示）
	int barWidth = 30;
	int barHeight = 4;
	int barX = px - barWidth / 2;
	int barY = py - r - 10;

	float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHP_);
	int fillWidth = static_cast<int>(barWidth * hpRatio);

	DrawBox(barX, barY, barX + barWidth, barY + barHeight, GetColor(50, 50, 50), TRUE);
	DrawBox(barX, barY, barX + fillWidth, barY + barHeight, GetColor(200, 50, 50), TRUE);
	DrawBox(barX, barY, barX + barWidth, barY + barHeight, GetColor(100, 100, 100), FALSE);

	// 攻撃範囲（攻撃中）
	if (isAttacking_)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
		DrawCircle(px, py, static_cast<int>(attackRange_), GetColor(255, 100, 100), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void Enemy::SetPatrolPoints(const std::vector<Vector2>& points)
{
	patrolPoints_ = points;
	if (!patrolPoints_.empty())
	{
		TransitionTo(EnemyState::Patrol);
	}
}

bool Enemy::CanAttack() const
{
	return attackTimer_ <= 0.0f && !isAttacking_;
}

void Enemy::OnAttackHit()
{
	hasHitThisAttack_ = true;
}

void Enemy::TakeDamage(int amount)
{
	// 無敵中はダメージを受けない
	if (IsInvincible())
	{
		return;
	}

	// ダメージ適用
	hp_ -= amount;
	if (hp_ < 0)
	{
		hp_ = 0;
	}

	// 無敵時間を開始（生きている場合のみ）
	if (hp_ > 0)
	{
		invincibilityTimer_ = INVINCIBILITY_DURATION;
	}
}

void Enemy::UpdateIdle(float deltaTime)
{
	// プレイヤー発見チェック
	if (CanSeeTarget())
	{
		TransitionTo(EnemyState::Chase);
		return;
	}

	// 巡回ポイントがあれば巡回開始
	if (!patrolPoints_.empty())
	{
		TransitionTo(EnemyState::Patrol);
	}
}

void Enemy::UpdatePatrol(float deltaTime)
{
	// プレイヤー発見チェック
	if (CanSeeTarget())
	{
		TransitionTo(EnemyState::Chase);
		return;
	}

	if (patrolPoints_.empty()) return;

	// 現在の巡回ポイントへ移動
	Vector2 targetPoint = patrolPoints_[currentPatrolIndex_];
	float distance = position_.Distance(targetPoint);

	if (distance < 10.0f)
	{
		// ポイント到達、待機
		patrolWaitTimer_ += deltaTime;
		if (patrolWaitTimer_ >= patrolWaitDuration_)
		{
			patrolWaitTimer_ = 0.0f;
			currentPatrolIndex_ = (currentPatrolIndex_ + 1) % static_cast<int>(patrolPoints_.size());
		}
	}
	else
	{
		MoveToward(targetPoint, deltaTime);
	}
}

void Enemy::UpdateChase(float deltaTime)
{
	if (!target_ || !target_->IsAlive())
	{
		TransitionTo(EnemyState::Patrol);
		return;
	}

	float distance = GetDistanceToTarget();

	// 見失った
	if (distance > loseAggroRange_)
	{
		TransitionTo(EnemyState::Patrol);
		return;
	}

	// 攻撃範囲内
	if (distance <= attackRange_)
	{
		TransitionTo(EnemyState::Attack);
		return;
	}

	// 追跡
	FaceTarget();
	MoveToward(target_->GetPosition(), deltaTime * chaseSpeedMult_);
}

void Enemy::UpdateAttack(float deltaTime)
{
	if (!target_ || !target_->IsAlive())
	{
		TransitionTo(EnemyState::Patrol);
		return;
	}

	FaceTarget();

	float distance = GetDistanceToTarget();

	// 攻撃範囲外に出た
	if (distance > attackRange_ * 1.5f)
	{
		TransitionTo(EnemyState::Chase);
		return;
	}

	// 攻撃実行
	if (CanAttack() && IsTargetInRange())
	{
		isAttacking_ = true;
		attackTimer_ = attackCooldown_;
		hasHitThisAttack_ = false;
	}
}

void Enemy::TransitionTo(EnemyState newState)
{
	state_ = newState;
	patrolWaitTimer_ = 0.0f;
}

bool Enemy::CanSeeTarget() const
{
	if (!target_ || !target_->IsAlive()) return false;
	return GetDistanceToTarget() <= detectionRange_;
}

bool Enemy::IsTargetInRange() const
{
	if (!target_) return false;
	return GetDistanceToTarget() <= attackRange_;
}

float Enemy::GetDistanceToTarget() const
{
	if (!target_) return 999999.0f;
	return position_.Distance(target_->GetPosition());
}

Vector2 Enemy::GetDirectionToTarget() const
{
	if (!target_) return Vector2(1.0f, 0.0f);
	Vector2 dir = target_->GetPosition() - position_;
	if (dir.LengthSquared() > 0.0f)
	{
		return dir.Normalized();
	}
	return Vector2(1.0f, 0.0f);
}

void Enemy::MoveToward(const Vector2& target, float deltaTime)
{
	Vector2 dir = target - position_;
	float distance = dir.GetLength();

	if (distance > 0.0f)
	{
		dir = dir.Normalized();
		facingDirection_ = dir;

		Vector2 delta = dir * moveSpeed_ * deltaTime;

		// 簡易的な壁判定（MapSystemがあれば使用）
		if (map_)
		{
			Vector2 newPos = position_ + delta;
			if (!map_->IsCollidingWithWall(newPos, radius_))
			{
				position_ = newPos;
			}
			else
			{
				// X軸のみ試行
				Vector2 testX(position_.x + delta.x, position_.y);
				if (!map_->IsCollidingWithWall(testX, radius_))
				{
					position_.x = testX.x;
				}

				// Y軸のみ試行
				Vector2 testY(position_.x, position_.y + delta.y);
				if (!map_->IsCollidingWithWall(testY, radius_))
				{
					position_.y = testY.y;
				}
			}
		}
		else
		{
			position_ = position_ + delta;
		}
	}
}

void Enemy::FaceTarget()
{
	if (!target_) return;
	facingDirection_ = GetDirectionToTarget();
}
