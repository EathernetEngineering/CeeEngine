//*********************************************************
//
// Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
//
//  Adapted by Chloe Eather.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#include <CeeEngine/renderer/aftermathShaderDatabase.h>

#include <CeeEngine/util.h>
#include <CeeEngine/debugMessenger.h>

namespace cee {
	ShaderDatabase::ShaderDatabase() {
	}

	ShaderDatabase::~ShaderDatabase() {
	}

	bool ShaderDatabase::FindShaderBinary(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, std::vector<uint8_t>& shader) const {
		auto it = m_ShaderBinaries.find(shaderHash);

		if (it == m_ShaderBinaries.end())
			return 0;

		shader = it->second;
		return true;
	}

	bool ShaderDatabase::FindShaderBinaryWithDebugData(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName, std::vector<uint8_t>& shader) const {
		auto it = m_ShaderBinariesWithDebugInfo.find(shaderDebugName);

		if (it == m_ShaderBinariesWithDebugInfo.end())
			return 0;

		shader = it->second;
		return true;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"

	void ShaderDatabase::AddShaderBinary(const char* shaderFilePath) {
		std::vector<uint8_t> data;
		int err = ::cee::util::ReadFile(shaderFilePath, data);
		if (err < 0) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to open file: \"%s\"", shaderFilePath);
			return;
		}

		const GFSDK_Aftermath_SpirvCode shader{ data.data(), static_cast<uint32_t>(data.size()) };
		GFSDK_Aftermath_ShaderBinaryHash shaderHash;
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderHashSpirv(GFSDK_Aftermath_Version_API,
																 &shader,
																 &shaderHash));
		m_ShaderBinaries[shaderHash].swap(data);
	}

	void ShaderDatabase::AddShaderBinaryWithDebugInfo(const char* strippedShaderFilePath, const char* shaderFilePath) {
		std::vector<uint8_t> strippedData;
		std::vector<uint8_t> data;
		int err = ::cee::util::ReadFile(strippedShaderFilePath, strippedData);
		if (err < 0) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to open file: \"%s\"", strippedShaderFilePath);
			return;
		}
		err = ::cee::util::ReadFile(shaderFilePath, data);
		if (err < 0) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to open file: \"%s\"", shaderFilePath);
			return;
		}

		const GFSDK_Aftermath_SpirvCode strippedShader{ strippedData.data(), static_cast<uint32_t>(strippedData.size()) };
		const GFSDK_Aftermath_SpirvCode shader{ data.data(), static_cast<uint32_t>(data.size()) };
		GFSDK_Aftermath_ShaderDebugName shaderDebugName;
		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugNameSpirv(GFSDK_Aftermath_Version_API,
																	  &shader,
																	  &strippedShader,
																	  &shaderDebugName));
		m_ShaderBinariesWithDebugInfo[shaderDebugName].swap(data);
	}
#pragma GCC diagnostic pop
}
