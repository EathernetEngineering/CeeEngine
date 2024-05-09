#ifndef CEE_ENGINE_ASSET_MANAGER_H_
#define CEE_ENGINE_ASSET_MANAGER_H_

#include <filesystem>
#include <optional>
#include <vector>

namespace cee {

struct ShaderBinary {
	std::vector<uint8_t> spvCode;
};

struct ShaderCode {
	std::string glslCode;
};

struct PipelineCache {
	std::vector<uint8_t> data;
};

struct Image {
	uint8_t* pixels;
	int32_t width, height;
	uint8_t channels;
};

class AssetManager {
public:
	AssetManager(std::filesystem::path basePath = {});
	~AssetManager() = default;

	bool Exists(std::filesystem::path filePath);
	void SetAssetRoot(std::filesystem::path rootPath);
	std::filesystem::path GetAssetRoot() const { return m_Path; }

	template<typename T>
	std::shared_ptr<T> LoadAsset(std::filesystem::path filePath);

	template<typename T>
	void SaveAsset(std::filesystem::path filePath, std::shared_ptr<T> asset);

private:
	std::optional<std::ifstream> OpenFileR(std::filesystem::path filePath);
	std::optional<std::ofstream> OpenFileW(std::filesystem::path filePath);

private:
	std::filesystem::path m_Path;
};
}

#endif
