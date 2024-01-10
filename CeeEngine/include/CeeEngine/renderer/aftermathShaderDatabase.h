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

#ifndef CEE_ENGINE_RENDERER_AFTERMATH_SHADER_DATABASE_H_
#define CEE_ENGINE_RENDERER_AFTERMATH_SHADER_DATABASE_H_

#include <CeeEngine/renderer/aftermathHelpers.h>

#include <vector>
#include <map>

namespace cee {
	class ShaderDatabase {
	public:
		ShaderDatabase();
		virtual ~ShaderDatabase();

		bool FindShaderBinary(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, std::vector<uint8_t>& shader) const;
		bool FindShaderBinaryWithDebugData(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName, std::vector<uint8_t>& shader) const;

	private:
		void AddShaderBinary(const char* shaderFilePath);
		void AddShaderBinaryWithDebugInfo(const char* strippedShaderFilePath, const char* shaderFilePath);

	private:
		std::map<GFSDK_Aftermath_ShaderBinaryHash, std::vector<uint8_t>> m_ShaderBinaries;
		std::map<GFSDK_Aftermath_ShaderDebugName, std::vector<uint8_t>> m_ShaderBinariesWithDebugInfo;
	};
}

#endif

