#pragma once
#include <string>
#include <map>
#include <vector>

class ResourceManager
{
public:
	static ResourceManager& GetInstance()
	{
		static ResourceManager instance;
		return instance;
	}

	// 全ゲームアセットを読み込む
	void LoadAllAssets();
	bool IsLoaded() const { return isLoaded_; }

	int LoadGraph(const std::string& key, const std::string& filePath);
	int GetGraph(const std::string& key) const;
	bool HasGraph(const std::string& key) const;

	int LoadDivGraph(const std::string& key, const std::string& filePath,
		int divX, int divY, int divWidth, int divHeight);
	const std::vector<int>& GetDivGraph(const std::string& key) const;

	int LoadSound(const std::string& key, const std::string& filePath);
	int GetSound(const std::string& key) const;

	void DeleteGraph(const std::string& key);
	void DeleteSound(const std::string& key);
	void Clear();

private:
	ResourceManager() : isLoaded_(false) {}
	~ResourceManager() = default;
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	bool isLoaded_;
	std::map<std::string, int> graphHandles_;
	std::map<std::string, std::vector<int>> divGraphHandles_;
	std::map<std::string, int> soundHandles_;
};
