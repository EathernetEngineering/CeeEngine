#include <CeeEngine/util.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/assetManager.h>
#include <filesystem>
#include <fstream>
#include <memory>

#include "stb/stb_image.h"

#define ASSET_PATH_ENV_VAR "CEE_ASSET_PATH"
#define DEFAULT_ASSET_PATH "/usr/share/CeeEngine/Assets"

namespace cee {
AssetManager::AssetManager(std::filesystem::path basePath) {
	if (basePath.empty()) {
		if (!HasEnvironmentVariable(ASSET_PATH_ENV_VAR)) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "No asset filepath selected, using default: " DEFAULT_ASSET_PATH ".");
			SetEnvironmentVariable(ASSET_PATH_ENV_VAR, DEFAULT_ASSET_PATH);
		}
		basePath = GetEnvironmentVariable(ASSET_PATH_ENV_VAR);
	}
	if (!std::filesystem::exists(basePath)) {
		char str[FILENAME_MAX];
		sprintf(str, "Failed to set asset root path. File \"%s\" does not exist.", basePath.c_str());
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, str);
		return;
	}
	m_Path = basePath;
}

bool AssetManager::Exists(std::filesystem::path filePath) {
	return std::filesystem::exists(filePath);
}

void AssetManager::SetAssetRoot(std::filesystem::path rootPath) {
	if (!std::filesystem::exists(rootPath)) {
		char str[FILENAME_MAX];
		sprintf(str, "Failed to set asset root path. File \"%s\" does not exist.", rootPath.c_str());
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, str);
		return;
	}
	m_Path = rootPath;
}

template<typename T>
std::shared_ptr<T> AssetManager::LoadAsset(std::filesystem::path filePath) {
	(void)filePath; // Ignore unused variable warning. 
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Attempting to load unsupported asset type.");
	return std::make_shared<T>(nullptr);
}

template<typename T>
void AssetManager::SaveAsset(std::filesystem::path filePath, std::shared_ptr<T> asset) {
	(void)filePath; // Ignore unused variable warning. 
	(void)asset; // Ignore unused variable warning. 
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Attempting to save unsupported asset type.");
}

template<>
std::shared_ptr<ShaderBinary> AssetManager::LoadAsset<ShaderBinary>(std::filesystem::path filePath) {
	auto shaderFile = OpenFileR(filePath);

	size_t fileSize = shaderFile->tellg();
	shaderFile->seekg(0);
	
	std::shared_ptr<ShaderBinary> shaderBinary = std::make_shared<ShaderBinary>();
	shaderBinary->spvCode.resize(fileSize);

	shaderFile->read(reinterpret_cast<char*>(shaderBinary->spvCode.data()), fileSize);
	if (!shaderFile->good()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to read file \"%s\".");
		return std::shared_ptr<ShaderBinary>(nullptr);
	}

	shaderFile->close();

	return shaderBinary;
}

template<>
std::shared_ptr<ShaderCode> AssetManager::LoadAsset<ShaderCode>(std::filesystem::path filePath) {
	auto shaderFile = OpenFileR(filePath);

	size_t fileSize = shaderFile->tellg();
	shaderFile->seekg(0);
	
	std::shared_ptr<ShaderCode> shaderCode = std::make_shared<ShaderCode>();
	shaderCode->glslCode.resize(fileSize);

	shaderFile->read(reinterpret_cast<char*>(shaderCode->glslCode.data()), fileSize);
	if (!shaderFile->good()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to read file \"%s\".");
		return std::shared_ptr<ShaderCode>(nullptr);
	}

	shaderFile->close();

	return shaderCode;
}

template<>
std::shared_ptr<PipelineCache> AssetManager::LoadAsset<PipelineCache>(std::filesystem::path filePath) {
	auto file = OpenFileR(filePath);
	if (!file)
		return std::shared_ptr<PipelineCache>(nullptr);
	size_t fileSize = file->tellg();
	file->seekg(0);
	
	auto pipelineCache = std::make_shared<PipelineCache>();
	pipelineCache->data.resize(fileSize);

	file->read(reinterpret_cast<char*>(pipelineCache->data.data()), fileSize);
	if (!file->good()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to read file \"%s\".");
		return std::shared_ptr<PipelineCache>(nullptr);
	}

	file->close();

	return pipelineCache;
}

// TODO: Allow loading formats other than 4 channels.
template<>
std::shared_ptr<Image> AssetManager::LoadAsset<Image>(std::filesystem::path filePath) {
	filePath = m_Path / filePath;
	if (!Exists(filePath))
		return std::shared_ptr<Image>(nullptr);

	auto image = std::make_shared<Image>();
	image->pixels = stbi_load(filePath.c_str(), &image->width, &image->height, nullptr, 4);
	if (image->pixels == nullptr) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to load image \"%s\".");
		return std::shared_ptr<Image>(nullptr);
	}
	image->channels = 4;
	return image;
}

template<>
void AssetManager::SaveAsset<PipelineCache>(const std::filesystem::path filePath, std::shared_ptr<PipelineCache> asset) {
	auto file = OpenFileW(filePath);
	if (!file)
		return;

	file->write(reinterpret_cast<char*>(asset->data.data()), asset->data.size());
	if (!file->good()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to write to file \"%s\".");
	}

	file->close();
}

std::optional<std::ifstream> AssetManager::OpenFileR(std::filesystem::path filePath) {
	filePath = m_Path / filePath;
	if (!Exists(filePath)) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "File \"%s\" does not exist.");
		return std::nullopt;
	}
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to open file \"%s\".");
		return std::nullopt;
	}
	return std::optional<std::ifstream>{std::move(file)};
}

std::optional<std::ofstream> AssetManager::OpenFileW(std::filesystem::path filePath) {
	filePath = m_Path / filePath;
	if (!Exists(filePath)) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "File \"%s\" does not exist.");
		return std::nullopt;
	}
	std::ofstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to open file \"%s\".");
		return std::nullopt;
	}
	return std::optional<std::ofstream>{std::move(file)};
}
}

