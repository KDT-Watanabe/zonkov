#pragma once

// バックパックデータ
struct BackpackData
{
	int itemId;
	int extraGridWidth;    // インベントリに追加される横幅
	int extraGridHeight;   // インベントリに追加される縦幅
	float speedPenalty;    // 移動速度へのペナルティ（0.0〜1.0）

	BackpackData()
		: itemId(0)
		, extraGridWidth(0)
		, extraGridHeight(0)
		, speedPenalty(0.0f)
	{}

	BackpackData(int id, int extraW, int extraH, float penalty = 0.0f)
		: itemId(id)
		, extraGridWidth(extraW)
		, extraGridHeight(extraH)
		, speedPenalty(penalty)
	{}

	int GetExtraCapacity() const { return extraGridWidth * extraGridHeight; }
};

// リグデータ
struct RigData
{
	int itemId;
	int quickSlots;        // クイックアクセススロット数
	int extraGridWidth;    // 少量の追加容量
	int extraGridHeight;

	RigData()
		: itemId(0)
		, quickSlots(0)
		, extraGridWidth(0)
		, extraGridHeight(0)
	{}

	RigData(int id, int slots, int extraW = 0, int extraH = 0)
		: itemId(id)
		, quickSlots(slots)
		, extraGridWidth(extraW)
		, extraGridHeight(extraH)
	{}
};
