/* Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Common.h"

//--------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------

static bool CompareVector3WithEpsilon(const XMFLOAT3& lhs, const XMFLOAT3& rhs) 
{
	const XMFLOAT3 vector3Epsilon = XMFLOAT3(0.00001f, 0.00001f, 0.00001f);
	return XMVector3NearEqual(XMLoadFloat3(&lhs), XMLoadFloat3(&rhs), XMLoadFloat3(&vector3Epsilon)) == TRUE;
}

static bool CompareVector2WithEpsilon(const XMFLOAT2& lhs, const XMFLOAT2& rhs)
{
	const XMFLOAT2 vector2Epsilon = XMFLOAT2(0.00001f, 0.00001f);
	return XMVector3NearEqual(XMLoadFloat2(&lhs), XMLoadFloat2(&rhs), XMLoadFloat2(&vector2Epsilon)) == TRUE;
}

//--------------------------------------------------------------------------------------
// Global Structures
//--------------------------------------------------------------------------------------

struct ConfigInfo 
{
	int			width;
	int			height;
	string		model;
	HINSTANCE	instance;

	ConfigInfo() 
	{
		width = 640;
		height = 360;
		model = "";
		instance = NULL;
	}
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;

	bool operator==(const Vertex &v) const 
	{
		if (CompareVector3WithEpsilon(position, v.position)) 
		{
			if (CompareVector2WithEpsilon(uv, v.uv)) return true;
			return true;
		}
		return false;
	}

	Vertex& operator=(const Vertex& v) 
	{
		position = v.position;
		uv = v.uv;
		return *this;
	}
};

struct Material 
{
	string name;
	string texturePath;
	float  textureResolution;

	Material() 
	{
		name = "defaultMaterial";
		texturePath = "";
		textureResolution = 512;
	}
};

struct Model
{
	vector<Vertex>									vertices;
	vector<uint32_t>								indices;
};

struct TextureInfo
{
	vector<UINT8> pixels;
	int width;
	int height;
	int stride;
};

struct MaterialCB 
{
	XMFLOAT4 resolution;
};

struct ViewCB
{
	XMMATRIX view;
	XMFLOAT4 viewOriginAndTanHalfFovY;
	XMFLOAT2 resolution;

	ViewCB() 
	{
		view = XMMatrixIdentity();
		viewOriginAndTanHalfFovY = XMFLOAT4(0, 0.f, 0.f, 0.f);
		resolution = XMFLOAT2(1280, 720);
	}
};

//--------------------------------------------------------------------------------------
// Standard D3D12
//--------------------------------------------------------------------------------------

struct D3D12BufferCreateInfo
{
	UINT64 size;
	UINT64 alignment;
	D3D12_HEAP_TYPE heapType;
	D3D12_RESOURCE_FLAGS flags;
	D3D12_RESOURCE_STATES state;

	D3D12BufferCreateInfo() :
		size(0), alignment(0),
		heapType(D3D12_HEAP_TYPE_DEFAULT),
		flags(D3D12_RESOURCE_FLAG_NONE),
		state(D3D12_RESOURCE_STATE_COMMON) {}

	D3D12BufferCreateInfo(UINT64 InSize, UINT64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize), alignment(InAlignment),
		heapType(InHeapType),
		flags(InFlags),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize), alignment(0),
		heapType(D3D12_HEAP_TYPE_DEFAULT),
		flags(InFlags),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InState) :
		size(InSize), alignment(0),
		heapType(InHeapType),
		flags(D3D12_RESOURCE_FLAG_NONE),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags) :
		size(InSize), alignment(0),
		heapType(D3D12_HEAP_TYPE_DEFAULT),
		flags(InFlags),
		state(D3D12_RESOURCE_STATE_COMMON) {}
};

struct D3D12ShaderCompilerInfo 
{
	dxc::DxcDllSupport		DxcDllHelper;
	IDxcCompiler*			compiler;
	IDxcLibrary*			library;

	D3D12ShaderCompilerInfo() 
	{
		compiler = nullptr;
		library = nullptr;
	}
};

struct D3D12ShaderInfo 
{
	LPCWSTR		filename;
	LPCWSTR		entryPoint;
	LPCWSTR		targetProfile;

	D3D12ShaderInfo(LPCWSTR inFilename, LPCWSTR inEntryPoint, LPCWSTR inProfile) 
	{
		filename = inFilename;
		entryPoint = inEntryPoint;
		targetProfile = inProfile;
	}

	D3D12ShaderInfo() 
	{
		filename = NULL;
		entryPoint = NULL;
		targetProfile = NULL;
	}
};

struct D3D12Resources 
{
	ID3D12Resource*									DXROutput;

	ID3D12Resource*									vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW						vertexBufferView;
	ID3D12Resource*									indexBuffer;
	D3D12_INDEX_BUFFER_VIEW							indexBufferView;

	ID3D12Resource*									viewCB;
	ViewCB											viewCBData;
	UINT8*											viewCBStart;

	ID3D12Resource*									materialCB;	
	MaterialCB										materialCBData;	
	UINT8*											materialCBStart;

	ID3D12DescriptorHeap*							rtvHeap;
	ID3D12DescriptorHeap*							cbvSrvUavHeap;

	ID3D12Resource*									texture;
	ID3D12Resource*									textureUploadHeap;

	UINT											rtvDescSize;

	float											translationOffset;
	float											rotationOffset;
	XMFLOAT3										eyeAngle;
	XMFLOAT3										eyePosition;
};

struct D3D12Global
{
	IDXGIFactory4*									factory;
	IDXGIAdapter1*									adapter;
	ID3D12Device5*									device;
	ID3D12GraphicsCommandList4*						cmdList;

	ID3D12CommandQueue*								cmdQueue;
	ID3D12CommandAllocator*							cmdAlloc[2];

	IDXGISwapChain3*								swapChain;
	ID3D12Resource*									backBuffer[2];

	ID3D12Fence*									fence;
	UINT64											fenceValues[3];
	HANDLE											fenceEvent;
	UINT											frameIndex;

	int												width;
	int												height;
};

//--------------------------------------------------------------------------------------
//  DXR
//--------------------------------------------------------------------------------------

struct AccelerationStructureBuffer
{
	ID3D12Resource* pScratch;
	ID3D12Resource* pResult;
	ID3D12Resource* pInstanceDesc;			// only used in top-level AS

	AccelerationStructureBuffer()
	{
		pScratch = NULL;
		pResult = NULL;
		pInstanceDesc = NULL;
	}
};

struct RtProgram
{
	RtProgram(D3D12ShaderInfo shaderInfo)
	{
		info = shaderInfo;
		blob = nullptr;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		exportName = shaderInfo.entryPoint;
		exportDesc.ExportToRename = nullptr;
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		pRootSignature = nullptr;
	}

	void SetBytecode()
	{
		exportDesc.Name = exportName.c_str();

		dxilLibDesc.NumExports = 1;
		dxilLibDesc.pExports = &exportDesc;
		dxilLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
		dxilLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();

		subobject.pDesc = &dxilLibDesc;
	}

	RtProgram() 
	{
		blob = nullptr;
		exportDesc.ExportToRename = nullptr;
		pRootSignature = nullptr;
	}

	D3D12ShaderInfo			info = {};
	IDxcBlob*				blob;

	ID3D12RootSignature*	pRootSignature = nullptr;
	D3D12_DXIL_LIBRARY_DESC	dxilLibDesc;
	D3D12_EXPORT_DESC		exportDesc;
	D3D12_STATE_SUBOBJECT	subobject;
	std::wstring			exportName;
};

struct HitProgram
{
	HitProgram(LPCWSTR name) : exportName(name)
	{
		desc = {};
		desc.HitGroupExport = exportName.c_str();
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobject.pDesc = &desc;
	}

	void SetExports(bool anyHit)
	{
		desc.HitGroupExport = exportName.c_str();
		if (anyHit) desc.AnyHitShaderImport = ahs.exportDesc.Name;
		desc.ClosestHitShaderImport = chs.exportDesc.Name;
	}

	HitProgram() {}

	RtProgram ahs;
	RtProgram chs;

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct DXRGlobal
{
	AccelerationStructureBuffer						TLAS;
	AccelerationStructureBuffer						BLAS;
	uint64_t										tlasSize;

	ID3D12Resource*									shaderTable;
	uint32_t										shaderTableRecordSize;

	RtProgram										rgs;
	RtProgram										miss;
	HitProgram										hit;

	ID3D12StateObject*								rtpso;
	ID3D12StateObjectProperties*					rtpsoInfo;
};
