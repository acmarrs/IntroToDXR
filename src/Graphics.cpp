/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
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

#include "Graphics.h"

//--------------------------------------------------------------------------------------
// Resource Functions
//--------------------------------------------------------------------------------------

namespace D3DResources
{

/**
* Create a matrix transform buffer and copy a transform to the upload heap.
*/
void Create_Transform_Buffer(D3D12Global &d3d, D3D12Resources &resources) 
{
	HRESULT hr;

	D3D12_RESOURCE_DESC matrixDesc = {};
	matrixDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	matrixDesc.Alignment = 0;
	matrixDesc.Width = sizeof(XMMATRIX);
	matrixDesc.Height = 1;
	matrixDesc.DepthOrArraySize = 1;
	matrixDesc.MipLevels = 1;
	matrixDesc.Format = DXGI_FORMAT_UNKNOWN;
	matrixDesc.SampleDesc.Count = 1;
	matrixDesc.SampleDesc.Quality = 0;
	matrixDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	matrixDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// Create the matrix transform buffer
	hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &matrixDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.transform));
	Utils::Validate(hr, L"Error: failed to create matrix transform resource!");
	
	// Create the matrix transform upload heap
	hr = d3d.device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &matrixDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resources.transformUploadHeap));
	Utils::Validate(hr, L"Failed to create matrix transform upload heap!");

	// Copy the transform matrix to the upload heap
	XMFLOAT4X4* pMatrix(nullptr);
	resources.transformUploadHeap->Map(0, nullptr, (void**)&pMatrix);
	::XMStoreFloat4x4(pMatrix, XMMatrixIdentity());
	resources.transformUploadHeap->Unmap(0, nullptr);

	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = resources.transform;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	d3d.cmdList->CopyBufferRegion(resources.transform, 0, resources.transformUploadHeap, 0, sizeof(XMMATRIX));
	d3d.cmdList->ResourceBarrier(1, &barrierDesc);
}

/**
* Create a GPU buffer resource.
*/
void Create_Buffer(D3D12Global &d3d, D3D12BufferCreateInfo& info, ID3D12Resource** ppResource)
{
	HRESULT hr;

	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = info.heapType;
	heapDesc.CreationNodeMask = 1;
	heapDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = info.alignment;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Width = info.size;
	resourceDesc.Flags = info.flags;

	// Create the GPU resource
	hr = d3d.device->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(ppResource));
	Utils::Validate(hr, L"Error: failed to create buffer resource!");
}

/**
* Create a texture.
*/
void Create_Texture(D3D12Global &d3d, D3D12Resources &resources, Material &material) 
{
	HRESULT hr;
	TextureInfo texture;

	// Load the texture
	texture = Utils::LoadTexture(material.texturePath);
	material.textureResolution = static_cast<float>(texture.width);

	// Describe the texture
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = texture.width;
	textureDesc.Height = texture.height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	// Create the texture resource
	hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resources.texture));
	Utils::Validate(hr, L"Error: failed to create texture!");

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resources.texture, 0, 1);

	// Describe the resource
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = uploadBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// Create the upload heap
	hr = d3d.device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resources.textureUploadHeap));
	Utils::Validate(hr, L"Error: failed to create texture upload heap!");

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = texture.pixels.data();
	textureData.RowPitch = texture.width * texture.stride;
	textureData.SlicePitch = textureData.RowPitch * texture.height;

	// Schedule a copy from the upload heap to the Texture2D resource
	UpdateSubresources(d3d.cmdList, resources.texture, resources.textureUploadHeap, 0, 0, 1, &textureData);

	// Transition the texture to a shader resource
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resources.texture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	d3d.cmdList->ResourceBarrier(1, &barrier);
}

/*
* Create the vertex buffer.
*/
void Create_Vertex_Buffer(D3D12Global &d3d, D3D12Resources &resources, Model &model) 
{
	// Create the buffer resource from the model's vertices
	D3D12BufferCreateInfo info(((UINT)model.vertices.size() * sizeof(Vertex)), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	Create_Buffer(d3d, info, &resources.vertexBuffer);

#if defined(_DEBUG)
	resources.vertexBuffer->SetName(L"VertexBuffer");
#endif

	// Copy the vertex data to the vertex buffer
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange = {};
	HRESULT hr = resources.vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	Utils::Validate(hr, L"Error: failed to map vertex buffer!");

	memcpy(pVertexDataBegin, model.vertices.data(), info.size);
	resources.vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view
	resources.vertexBufferView.BufferLocation = resources.vertexBuffer->GetGPUVirtualAddress();
	resources.vertexBufferView.StrideInBytes = sizeof(Vertex);
	resources.vertexBufferView.SizeInBytes = static_cast<UINT>(info.size);
}

/**
* Create the index buffer.
*/
void Create_Index_Buffer(D3D12Global &d3d, D3D12Resources &resources, Model &model) 
{
	// Create the index buffer resource
	D3D12BufferCreateInfo info((UINT)model.indices.size() * sizeof(UINT), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	Create_Buffer(d3d, info, &resources.indexBuffer);

#if defined(_DEBUG)
	resources.indexBuffer->SetName(L"IndexBuffer");
#endif

	// Copy the index data to the index buffer
	UINT8* pIndexDataBegin;
	D3D12_RANGE readRange = {};
	HRESULT hr = resources.indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
	Utils::Validate(hr, L"Error: failed to map index buffer!");

	memcpy(pIndexDataBegin, model.indices.data(), info.size);
	resources.indexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view
	resources.indexBufferView.BufferLocation = resources.indexBuffer->GetGPUVirtualAddress();
	resources.indexBufferView.SizeInBytes = static_cast<UINT>(info.size);
	resources.indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

/*
* Create a constant buffer.
*/
void Create_Constant_Buffer(D3D12Global &d3d, ID3D12Resource** buffer, UINT64 size) 
{
	D3D12BufferCreateInfo bufferInfo((size + 255) & ~255, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	Create_Buffer(d3d, bufferInfo, buffer);
}

/**
* Create the back buffer's RTV view.
*/
void Create_BackBuffer_RTV(D3D12Global &d3d, D3D12Resources &resources)
{
	HRESULT hr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

	rtvHandle = resources.rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// Create a RTV for each back buffer
	for (UINT n = 0; n < 2; n++)
	{
		hr = d3d.swapChain->GetBuffer(n, IID_PPV_ARGS(&d3d.backBuffer[n]));
		if (FAILED(hr)) {
			throw runtime_error("Failed to get swap chain buffer!");
		}

		d3d.device->CreateRenderTargetView(d3d.backBuffer[n], nullptr, rtvHandle);
		rtvHandle.ptr += (1 * resources.rtvDescSize);
	}
}

/**
* Create the samplers.
*/
void Create_Samplers(D3D12Global &d3d, D3D12Resources &resources) 
{
	// Get the sampler descriptor heap handle
	D3D12_CPU_DESCRIPTOR_HANDLE handle = resources.samplerHeap->GetCPUDescriptorHandleForHeapStart();

	// Describe the sampler
	D3D12_SAMPLER_DESC desc = {};
	memset(&desc, 0, sizeof(desc));
	desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.MipLODBias = 0.f;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	desc.MinLOD = 0.f;
	desc.MaxLOD = D3D12_FLOAT32_MAX;
	desc.MaxAnisotropy = 1;

	// Create the sampler
	d3d.device->CreateSampler(&desc, handle);
}

/**
* Create and initialize the view constant buffer.
*/
void Create_View_CB(D3D12Global &d3d, D3D12Resources &resources) 
{
	Create_Constant_Buffer(d3d, &resources.viewCB, sizeof(ViewCB));

	HRESULT hr = resources.viewCB->Map(0, nullptr, reinterpret_cast<void**>(&resources.viewCBStart));
	Utils::Validate(hr, L"Error: failed to map View constant buffer!");

	memcpy(resources.viewCBStart, &resources.viewCBData, sizeof(resources.viewCBData));
}

/**
* Create and initialize the material constant buffer.
*/
void Create_Material_CB(D3D12Global &d3d, D3D12Resources &resources, const Material &material) 
{
	Create_Constant_Buffer(d3d, &resources.materialCB, sizeof(MaterialCB));

	resources.materialCBData.resolution = XMFLOAT4(material.textureResolution, 0.f, 0.f, 0.f);

	HRESULT hr = resources.materialCB->Map(0, nullptr, reinterpret_cast<void**>(&resources.materialCBStart));
	Utils::Validate(hr, L"Error: failed to map Material constant buffer!");

	memcpy(resources.materialCBStart, &resources.materialCBData, sizeof(resources.materialCBData));
}

/**
* Create the RTV and sampler descriptor heaps.
*/
void Create_Descriptor_Heaps(D3D12Global &d3d, D3D12Resources &resources)
{
	// Describe the RTV heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
	rtvDesc.NumDescriptors = 2;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the RTV heap
	HRESULT hr = d3d.device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&resources.rtvHeap));
	Utils::Validate(hr, L"Error: failed to create RTV descriptor heap!");

	resources.rtvDescSize = d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Describe the sampler heap
	D3D12_DESCRIPTOR_HEAP_DESC samplerDesc = {};
	samplerDesc.NumDescriptors = 1;
	samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// Create the sampler heap
	hr = d3d.device->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&resources.samplerHeap));
	Utils::Validate(hr, L"Error: failed to create sampler descriptor heap!");
}

/**
* Update the view constant buffer.
*/
void Update_View_CB(D3D12Global &d3d, D3D12Resources &resources) 
{
	const float rotationSpeed = 0.005f;
	XMMATRIX view, invView;
	XMFLOAT3 eye, focus, up;
	float aspect, fov;

	resources.eyeAngle.x += rotationSpeed;

#if _DEBUG
	float x = 2.f * cosf(resources.eyeAngle.x);
	float y = 0.f;
	float z = 2.25f + 2.f * sinf(resources.eyeAngle.x);
	focus = XMFLOAT3(0.f, 0.f, 0.f);
#else
	float x = 8.f * cosf(resources.eyeAngle.x);
	float y = 1.5f + 1.5f * cosf(resources.eyeAngle.x);
	float z = 8.f + 2.25f * sinf(resources.eyeAngle.x);
	focus = XMFLOAT3(0.f, 1.75f, 0.f);
#endif

	eye = XMFLOAT3(x, y, z);
	up = XMFLOAT3(0.f, 1.f, 0.f);

	aspect = (float)d3d.width / (float)d3d.height;
	fov = 65.f * (XM_PI / 180.f);							// convert to radians

	resources.rotationOffset += rotationSpeed;

	view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&focus), XMLoadFloat3(&up));
	invView = XMMatrixInverse(NULL, view);

	resources.viewCBData.view = XMMatrixTranspose(invView);
	resources.viewCBData.viewOriginAndTanHalfFovY = XMFLOAT4(eye.x, eye.y, eye.z, tanf(fov * 0.5f));
	resources.viewCBData.resolution = XMFLOAT2((float)d3d.width, (float)d3d.height);
	memcpy(resources.viewCBStart, &resources.viewCBData, sizeof(resources.viewCBData));
}

/**
 * Release the resources.
 */
void Destroy(D3D12Resources &resources)
{
	if (resources.viewCB) resources.viewCB->Unmap(0, nullptr);
	if (resources.viewCBStart) resources.viewCBStart = nullptr;
	if (resources.materialCB) resources.materialCB->Unmap(0, nullptr);
	if (resources.materialCBStart) resources.materialCBStart = nullptr;

	SAFE_RELEASE(resources.DXROutput);
	SAFE_RELEASE(resources.vertexBuffer);
	SAFE_RELEASE(resources.indexBuffer);
	SAFE_RELEASE(resources.rtvHeap);
	SAFE_RELEASE(resources.cbvSrvUavHeap);
	SAFE_RELEASE(resources.samplerHeap);
	SAFE_RELEASE(resources.transform);
	SAFE_RELEASE(resources.transformUploadHeap);
	SAFE_RELEASE(resources.texture);
	SAFE_RELEASE(resources.textureUploadHeap);
}

}

//--------------------------------------------------------------------------------------
// D3D12 Shader Functions
//--------------------------------------------------------------------------------------

namespace D3DShaders
{

/**
* Compile a D3D HLSL shader using dxcompiler.
*/
void Compile_Shader(D3D12ShaderCompilerInfo &compilerInfo, D3D12ShaderInfo &info, IDxcBlob** blob) 
{
	HRESULT hr;
	UINT32 codePage(0);
	IDxcBlobEncoding* pShaderText(nullptr);

	// Load and encode the shader file
	hr = compilerInfo.library->CreateBlobFromFile(info.filename, &codePage, &pShaderText);
	Utils::Validate(hr, L"Error: failed to create blob from shader file!");

	// Create the compiler include handler
	CComPtr<IDxcIncludeHandler> dxcIncludeHandler;
	hr = compilerInfo.library->CreateIncludeHandler(&dxcIncludeHandler);
	Utils::Validate(hr, L"Error: failed to create include handler");

	// Compile the shader
	IDxcOperationResult* result;
	hr = compilerInfo.compiler->Compile(pShaderText, info.filename, L"", info.targetProfile, nullptr, 0, nullptr, 0, dxcIncludeHandler, &result);
	Utils::Validate(hr, L"Error: failed to compile shader!");

	// Verify the result
	result->GetStatus(&hr);
	if (FAILED(hr)) 
	{
		IDxcBlobEncoding* error;
		hr = result->GetErrorBuffer(&error);
		Utils::Validate(hr, L"Error: failed to get shader compiler error buffer!");

		// Convert error blob to a string
		vector<char> infoLog(error->GetBufferSize() + 1);
		memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
		infoLog[error->GetBufferSize()] = 0;

		string errorMsg = "Shader Compiler Error:\n";
		errorMsg.append(infoLog.data());

		MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
		return;
	}

	hr = result->GetResult(blob);
	Utils::Validate(hr, L"Error: failed to get shader blob result!");
}

/**
* Compile a D3D DXRT HLSL shader using dxcompiler.
*/
void Compile_Shader(D3D12ShaderCompilerInfo &compilerInfo, RtProgram &program) 
{
	Compile_Shader(compilerInfo, program.info, &program.blob);	
	program.SetBytecode();
}

/**
* Initialize the shader compiler.
*/
void Init_Shader_Compiler(D3D12ShaderCompilerInfo &shaderCompiler) 
{
	HRESULT hr = shaderCompiler.DxcDllHelper.Initialize();
	Utils::Validate(hr, L"Failed to initialize DxCDllSupport!");

	hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &shaderCompiler.compiler);
	Utils::Validate(hr, L"Failed to create DxcCompiler!");

	hr = shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &shaderCompiler.library);
	Utils::Validate(hr, L"Failed to create DxcLibrary!");
}

/**
 * Release shader compiler resources.
 */
void Destroy(D3D12ShaderCompilerInfo &shaderCompiler)
{
	SAFE_RELEASE(shaderCompiler.compiler);
	SAFE_RELEASE(shaderCompiler.library);
	shaderCompiler.DxcDllHelper.Cleanup();
}

}

//--------------------------------------------------------------------------------------
// D3D12 Functions
//--------------------------------------------------------------------------------------

namespace D3D12
{

/**
* Create the device.
*/
void Create_Device(D3D12Global &d3d)
{
#if defined(_DEBUG)
	// Enable the D3D12 debug layer.
	{
		ID3D12Debug* debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	// Create a DXGI Factory
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&d3d.factory));
	Utils::Validate(hr, L"Error: failed to create DXGI factory!");

	// Create the device
	d3d.adapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != d3d.factory->EnumAdapters1(adapterIndex, &d3d.adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		d3d.adapter->GetDesc1(&adapterDesc);

		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{			
			continue;	// Don't select the Basic Render Driver adapter.
		}

		if (SUCCEEDED(D3D12CreateDevice(d3d.adapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device5), (void**)&d3d.device)))
		{
			// Check if the device supports ray tracing.
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features = {};
			HRESULT hr = d3d.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features));
			if (FAILED(hr) || features.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
			{
				SAFE_RELEASE(d3d.device);
				d3d.device = nullptr;
				continue;
			}

			printf("Running on DXGI Adapter %S\n", adapterDesc.Description);
			break;
		}

		if (d3d.device == nullptr)
		{
			// Didn't find a device that supports ray tracing.
			Utils::Validate(E_FAIL, L"Error: failed to create ray tracing device!");
		}
	}
}

/**
* Create the command queue(s).
*/
void Create_Command_Queue(D3D12Global &d3d) 
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = d3d.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d.cmdQueue));
	Utils::Validate(hr, L"Error: failed to create command queue!");
}

/**
* Create the command allocator(s) for each frame.
*/
void Create_Command_Allocator(D3D12Global &d3d) 
{
	// Create a command allocator for each frame
	for (UINT n = 0; n < 2; n++)
	{
		HRESULT hr = d3d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3d.cmdAlloc[n]));
		Utils::Validate(hr, L"Error: failed to create the command allocator!");
	}
}

/**
* Create the command list.
*/
void Create_CommandList(D3D12Global &d3d) 
{
	// Create the command list
	HRESULT hr = d3d.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d.cmdAlloc[d3d.frameIndex], nullptr, IID_PPV_ARGS(&d3d.cmdList));
	hr = d3d.cmdList->Close();
	Utils::Validate(hr, L"Error: failed to create the command list!");
}

/**
* Create a fence.
*/
void Create_Fence(D3D12Global &d3d) 
{
	HRESULT hr = d3d.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d.fence));
	Utils::Validate(hr, L"Error: failed to create fence!");
	d3d.fenceValues[d3d.frameIndex]++;

	// Create an event handle to use for frame synchronization
	d3d.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (d3d.fenceEvent == nullptr) 
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		Utils::Validate(hr, L"Error: failed to create fence event!");
	}
}

/**
* Create the swap chain.
*/
void Create_SwapChain(D3D12Global &d3d, HWND window) 
{
	// Describe the swap chain
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.BufferCount = 2;
	desc.Width = d3d.width;
	desc.Height = d3d.height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;

	IDXGISwapChain1* swapChain;

	// Create the swap chain
	HRESULT hr = d3d.factory->CreateSwapChainForHwnd(d3d.cmdQueue, window, &desc, nullptr, nullptr, &swapChain);
	Utils::Validate(hr, L"Error: failed to create swap chain!");

	// Associate the swap chain with a window
	hr = d3d.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
	Utils::Validate(hr, L"Error: failed to make window association!");

	// Get the swap chain interface
	hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&d3d.swapChain));
	Utils::Validate(hr, L"Error: failed to cast swap chain!");

	SAFE_RELEASE(swapChain);
	d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();
}

/**
* Create a root signature.
*/
ID3D12RootSignature* Create_Root_Signature(D3D12Global &d3d, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	HRESULT hr;
	ID3DBlob* sig;
	ID3DBlob* error;

	hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
	Utils::Validate(hr, L"Error: failed to serialize root signature!");

	ID3D12RootSignature* pRootSig;
	hr = d3d.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
	Utils::Validate(hr, L"Error: failed to create root signature!");

	SAFE_RELEASE(sig);
	SAFE_RELEASE(error);
	return pRootSig;
}

/**
* Reset the command list(s).
*/
void Reset_CommandList(D3D12Global &d3d) 
{
	// Reset the command allocator for the current frame
	HRESULT hr = d3d.cmdAlloc[d3d.frameIndex]->Reset();
	Utils::Validate(hr, L"Error: failed to reset command allocator!");

	// Reset the command list for the current frame
	hr = d3d.cmdList->Reset(d3d.cmdAlloc[d3d.frameIndex], nullptr);
	Utils::Validate(hr, L"Error: failed to reset command list!");
}

/*
* Submit the command list.
*/
void Submit_CmdList(D3D12Global &d3d) 
{
	d3d.cmdList->Close();

	ID3D12CommandList* pGraphicsList = { d3d.cmdList };
	d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);
	d3d.fenceValues[d3d.frameIndex]++;
	d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
}

/**
 * Swap the buffers.
 */
void Present(D3D12Global &d3d) 
{
	HRESULT hr = d3d.swapChain->Present(1, 0);
	if (FAILED(hr))
	{
		hr = d3d.device->GetDeviceRemovedReason();
		Utils::Validate(hr, L"Error: failed to present!");
	}
}

/*
* Wait for pending GPU work to complete.
*/
void WaitForGPU(D3D12Global &d3d) 
{
	// Schedule a signal command in the queue
	HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, d3d.fenceValues[d3d.frameIndex]);
	Utils::Validate(hr, L"Error: failed to signal fence!");

	// Wait until the fence has been processed
	hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
	Utils::Validate(hr, L"Error: failed to set fence event!");

	WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame
	d3d.fenceValues[d3d.frameIndex]++;
}

/**
* Prepare to render the next frame.
*/
void MoveToNextFrame(D3D12Global &d3d) 
{
	// Schedule a Signal command in the queue
	const UINT64 currentFenceValue = d3d.fenceValues[d3d.frameIndex];
	HRESULT hr = d3d.cmdQueue->Signal(d3d.fence, currentFenceValue);
	Utils::Validate(hr, L"Error: failed to signal command queue!");

	// Update the frame index
	d3d.frameIndex = d3d.swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is
	if (d3d.fence->GetCompletedValue() < d3d.fenceValues[d3d.frameIndex])
	{
		hr = d3d.fence->SetEventOnCompletion(d3d.fenceValues[d3d.frameIndex], d3d.fenceEvent);
		Utils::Validate(hr, L"Error: failed to set fence value!");

		WaitForSingleObjectEx(d3d.fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame
	d3d.fenceValues[d3d.frameIndex] = currentFenceValue + 1;
}

/**
 * Release D3D12 resources.
 */
void Destroy(D3D12Global &d3d)
{
	SAFE_RELEASE(d3d.fence);
	SAFE_RELEASE(d3d.backBuffer[1]);
	SAFE_RELEASE(d3d.backBuffer[0]);
	SAFE_RELEASE(d3d.swapChain);
	SAFE_RELEASE(d3d.cmdAlloc[0]);
	SAFE_RELEASE(d3d.cmdAlloc[1]);
	SAFE_RELEASE(d3d.cmdQueue);
	SAFE_RELEASE(d3d.cmdList);
	SAFE_RELEASE(d3d.device);
	SAFE_RELEASE(d3d.adapter);
	SAFE_RELEASE(d3d.factory);
}

}


//--------------------------------------------------------------------------------------
// DXR Functions
//--------------------------------------------------------------------------------------

namespace DXR
{

/**
* Create the bottom level acceleration structure.
*/
void Create_Bottom_Level_AS(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources, Model &model) 
{
	// Describe the geometry that goes in the bottom acceleration structure(s)
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = resources.vertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = resources.vertexBufferView.StrideInBytes;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(model.vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = resources.indexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexFormat = resources.indexBufferView.Format;
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(model.indices.size());
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get the size requirements for the BLAS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;	
	ASInputs.pGeometryDescs = &geometryDesc;
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	d3d.device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);
	ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);

	// Create the BLAS scratch buffer
	D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.BLAS.pScratch);

	// Create the BLAS buffer
	bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.BLAS.pResult);

	// Describe and build the bottom level acceleration structure
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;	
	buildDesc.ScratchAccelerationStructureData = dxr.BLAS.pScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = dxr.BLAS.pResult->GetGPUVirtualAddress();

	d3d.cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the BLAS build to complete
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = dxr.BLAS.pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3d.cmdList->ResourceBarrier(1, &uavBarrier);
}

/**
* Create the top level acceleration structure and its associated buffers.
*/
void Create_Top_Level_AS(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources) 
{
	// Describe the TLAS geometry instance(s)
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceID = 0;																		// This value is exposed to shaders as SV_InstanceID
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;		// identity transform
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc.AccelerationStructure = dxr.BLAS.pResult->GetGPUVirtualAddress();

	// Create the TLAS instance buffer
	D3D12BufferCreateInfo instanceBufferInfo;
	instanceBufferInfo.size = sizeof(instanceDesc);
	instanceBufferInfo.heapType = D3D12_HEAP_TYPE_UPLOAD;
	instanceBufferInfo.flags = D3D12_RESOURCE_FLAG_NONE;
	instanceBufferInfo.state = D3D12_RESOURCE_STATE_GENERIC_READ;
	D3DResources::Create_Buffer(d3d, instanceBufferInfo, &dxr.TLAS.pInstanceDesc);

	// Copy the instance data to the buffer
	UINT8* pData;
	dxr.TLAS.pInstanceDesc->Map(0, nullptr, (void**)&pData);
	memcpy(pData, &instanceDesc, sizeof(instanceDesc));
	dxr.TLAS.pInstanceDesc->Unmap(0, nullptr);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get the size requirements for the TLAS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.InstanceDescs = dxr.TLAS.pInstanceDesc->GetGPUVirtualAddress();
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	d3d.device->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);
	ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);

	// Set TLAS size
	dxr.tlasSize = ASPreBuildInfo.ResultDataMaxSizeInBytes;

	// Create TLAS scratch buffer
	D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.TLAS.pScratch);

	// Create the TLAS buffer
	bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.TLAS.pResult);

	// Describe and build the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;
	buildDesc.ScratchAccelerationStructureData = dxr.TLAS.pScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = dxr.TLAS.pResult->GetGPUVirtualAddress();

	d3d.cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the TLAS build to complete
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = dxr.TLAS.pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3d.cmdList->ResourceBarrier(1, &uavBarrier);
}

/**
* Load and create the DXR Ray Generation program and root signature.
*/
void Create_RayGen_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler)
{
	// Load and compile the ray generation shader
	dxr.rgs = RtProgram(D3D12ShaderInfo(L"shaders\\RayGen.hlsl", L"", L"lib_6_3"));
	D3DShaders::Compile_Shader(shaderCompiler, dxr.rgs);

	// Describe the ray generation root signature
	D3D12_DESCRIPTOR_RANGE ranges[3];

	ranges[0].BaseShaderRegister = 0;
	ranges[0].NumDescriptors = 2;
	ranges[0].RegisterSpace = 0;
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;

	ranges[1].BaseShaderRegister = 0;
	ranges[1].NumDescriptors = 1;
	ranges[1].RegisterSpace = 0;
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].OffsetInDescriptorsFromTableStart = 2;

	ranges[2].BaseShaderRegister = 0;
	ranges[2].NumDescriptors = 4;
	ranges[2].RegisterSpace = 0;
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].OffsetInDescriptorsFromTableStart = 3;

	D3D12_ROOT_PARAMETER param0 = {};
	param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param0.DescriptorTable.NumDescriptorRanges = _countof(ranges);
	param0.DescriptorTable.pDescriptorRanges = ranges;

	D3D12_ROOT_PARAMETER rootParams[1] = { param0 };

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = _countof(rootParams);
	rootDesc.pParameters = rootParams;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// Create the root signature
	dxr.rgs.pRootSignature = D3D12::Create_Root_Signature(d3d, rootDesc);
}

/**
* Load and create the DXR Miss program and root signature.
*/
void Create_Miss_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler)
{
	// Load and compile the miss shader
	dxr.miss = RtProgram(D3D12ShaderInfo(L"shaders\\Miss.hlsl", L"", L"lib_6_3"));
	D3DShaders::Compile_Shader(shaderCompiler, dxr.miss);

	// Create an empty root signature
	dxr.miss.pRootSignature = D3D12::Create_Root_Signature(d3d, {});
}

/**
* Load and create the DXR Closest Hit program and root signature.
*/
void Create_Closest_Hit_Program(D3D12Global &d3d, DXRGlobal &dxr, D3D12ShaderCompilerInfo &shaderCompiler)
{
	// Note: since all of our triangles are opaque, we will ignore the any hit program.

	// Load and compile the Closest Hit shader
	dxr.hit = HitProgram(L"Hit");
	dxr.hit.chs = RtProgram(D3D12ShaderInfo(L"shaders\\ClosestHit.hlsl", L"", L"lib_6_3"));
	D3DShaders::Compile_Shader(shaderCompiler, dxr.hit.chs);

	// Describe the root signature
	D3D12_DESCRIPTOR_RANGE ranges[3];

	ranges[0].BaseShaderRegister = 0;
	ranges[0].NumDescriptors = 2;
	ranges[0].RegisterSpace = 0;
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;

	ranges[1].BaseShaderRegister = 0;
	ranges[1].NumDescriptors = 1;
	ranges[1].RegisterSpace = 0;
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].OffsetInDescriptorsFromTableStart = 2;

	ranges[2].BaseShaderRegister = 0;
	ranges[2].NumDescriptors = 4;
	ranges[2].RegisterSpace = 0;
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].OffsetInDescriptorsFromTableStart = 3;

	D3D12_ROOT_PARAMETER param0 = {};
	param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param0.DescriptorTable.NumDescriptorRanges = _countof(ranges);
	param0.DescriptorTable.pDescriptorRanges = ranges;

	D3D12_ROOT_PARAMETER rootParams[1] = { param0 };

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = _countof(rootParams);
	rootDesc.pParameters = rootParams;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// Create the root signature
	dxr.hit.chs.pRootSignature = D3D12::Create_Root_Signature(d3d, rootDesc);
}

/**
* Create the DXR pipeline state object.
*/
void Create_Pipeline_State_Object(D3D12Global &d3d, DXRGlobal &dxr)
{
	// Need 10 subobjects:
	// 1 for RGS program
	// 1 for Miss program
	// 1 for CHS program
	// 1 for Hit Group
	// 2 for RayGen Root Signature (root-signature and association)
	// 2 for Shader Config (config and association)
	// 1 for Global Root Signature
	// 1 for Pipeline Config	
	UINT index = 0;
	vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.resize(10);
	
	// Add state subobject for the RGS
	D3D12_EXPORT_DESC rgsExportDesc = {};
	rgsExportDesc.Name = L"RayGen_12";
	rgsExportDesc.ExportToRename = L"RayGen";
	rgsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	rgsLibDesc = {};
	rgsLibDesc.DXILLibrary.BytecodeLength = dxr.rgs.blob->GetBufferSize();
	rgsLibDesc.DXILLibrary.pShaderBytecode = dxr.rgs.blob->GetBufferPointer();
	rgsLibDesc.NumExports = 1;
	rgsLibDesc.pExports = &rgsExportDesc;

	D3D12_STATE_SUBOBJECT rgs = {};
	rgs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	rgs.pDesc = &rgsLibDesc;

	subobjects[index++] = rgs;

	// Add state subobject for the Miss shader
	D3D12_EXPORT_DESC msExportDesc = {};
	msExportDesc.Name = L"Miss_5";
	msExportDesc.ExportToRename = L"Miss";
	msExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	msLibDesc = {};
	msLibDesc.DXILLibrary.BytecodeLength = dxr.miss.blob->GetBufferSize();
	msLibDesc.DXILLibrary.pShaderBytecode = dxr.miss.blob->GetBufferPointer();
	msLibDesc.NumExports = 1;
	msLibDesc.pExports = &msExportDesc;

	D3D12_STATE_SUBOBJECT ms = {};
	ms.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	ms.pDesc = &msLibDesc;

	subobjects[index++] = ms;

	// Add state subobject for the Closest Hit shader
	D3D12_EXPORT_DESC chsExportDesc = {};
	chsExportDesc.Name = L"ClosestHit_76";
	chsExportDesc.ExportToRename = L"ClosestHit";
	chsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	chsLibDesc = {};
	chsLibDesc.DXILLibrary.BytecodeLength = dxr.hit.chs.blob->GetBufferSize();
	chsLibDesc.DXILLibrary.pShaderBytecode = dxr.hit.chs.blob->GetBufferPointer();
	chsLibDesc.NumExports = 1;
	chsLibDesc.pExports = &chsExportDesc;

	D3D12_STATE_SUBOBJECT chs = {};
	chs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chs.pDesc = &chsLibDesc;

	subobjects[index++] = chs;

	// Add a state subobject for the hit group
	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHit_76";
	hitGroupDesc.HitGroupExport = L"HitGroup";

	D3D12_STATE_SUBOBJECT hitGroup = {};
	hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup.pDesc = &hitGroupDesc;

	subobjects[index++] = hitGroup;

	// Add a state subobject for the shader payload configuration
	D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
	shaderDesc.MaxPayloadSizeInBytes = sizeof(XMFLOAT4);	// only need float4 for color
	shaderDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigObject = {};
	shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigObject.pDesc = &shaderDesc;

	subobjects[index++] = shaderConfigObject;

	// Create a list of the shader export names that use the payload
	const WCHAR* shaderExports[] = { L"RayGen_12", L"Miss_5", L"HitGroup" };

	// Add a state subobject for the association between shaders and the payload
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.NumExports = _countof(shaderExports);
	shaderPayloadAssociation.pExports = shaderExports;
	shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[(index - 1)];

	D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
	shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;

	subobjects[index++] = shaderPayloadAssociationObject;

	// Add a state subobject for the shared root signature 
	D3D12_STATE_SUBOBJECT rayGenRootSigObject = {};
	rayGenRootSigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	rayGenRootSigObject.pDesc = &dxr.rgs.pRootSignature;

	subobjects[index++] = rayGenRootSigObject;

	// Create a list of the shader export names that use the root signature
	const WCHAR* rootSigExports[] = { L"RayGen_12", L"HitGroup", L"Miss_5" };

	// Add a state subobject for the association between the RayGen shader and the local root signature
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rayGenShaderRootSigAssociation = {};
	rayGenShaderRootSigAssociation.NumExports = _countof(rootSigExports);
	rayGenShaderRootSigAssociation.pExports = rootSigExports;
	rayGenShaderRootSigAssociation.pSubobjectToAssociate = &subobjects[(index - 1)];

	D3D12_STATE_SUBOBJECT rayGenShaderRootSigAssociationObject = {};
	rayGenShaderRootSigAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	rayGenShaderRootSigAssociationObject.pDesc = &rayGenShaderRootSigAssociation;

	subobjects[index++] = rayGenShaderRootSigAssociationObject;

	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSig.pDesc = &dxr.miss.pRootSignature;

	subobjects[index++] = globalRootSig;

	// Add a state subobject for the ray tracing pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;

	subobjects[index++] = pipelineConfigObject;

	// Describe the Ray Tracing Pipeline State Object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
	pipelineDesc.pSubobjects = subobjects.data();

	// Create the RT Pipeline State Object (RTPSO)
	HRESULT hr = d3d.device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&dxr.rtpso));
	Utils::Validate(hr, L"Error: failed to create state object!");
	
	// Get the RTPSO properties
	hr = dxr.rtpso->QueryInterface(IID_PPV_ARGS(&dxr.rtpsoInfo));
	Utils::Validate(hr, L"Error: failed to get RTPSO info object!");
}

/**
* Create the DXR shader table.
*/
void Create_Shader_Table(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources) 
{
	/*
	The Shader Table layout is as follows:
	Entry 0 - Ray Generation shader
	Entry 1 - Miss shader
	Entry 2 - Closest Hit shader
	All shader records in the Shader Table must have the same size, so shader record size will be based on the largest required entry.
	The ray generation program requires the largest entry: sizeof(shader identifier) + 8 bytes for a descriptor table.
	The entry size must be aligned up to D3D12_RAYTRACING_SHADER_BINDING_TABLE_RECORD_BYTE_ALIGNMENT
	*/

	uint32_t shaderIdSize = 32;
	uint32_t sbtSize = 0;

	dxr.sbtEntrySize = shaderIdSize;
	dxr.sbtEntrySize += 8;					// CBV/SRV/UAV descriptor table
	dxr.sbtEntrySize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, dxr.sbtEntrySize);

	sbtSize = (dxr.sbtEntrySize * 3);		// 3 shader records in the table
	sbtSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, sbtSize);

	// Create the shader table buffers
	D3D12BufferCreateInfo bufferInfo(sbtSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	D3DResources::Create_Buffer(d3d, bufferInfo, &dxr.sbt);

	// Map the buffer
	uint8_t* pData;
	HRESULT hr = dxr.sbt->Map(0, nullptr, (void**)&pData);
	Utils::Validate(hr, L"Error: failed to map shader table!");

	// Entry 0 - Ray Generation program and local root argument data (descriptor table with constant buffer and IB/VB pointers)
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"RayGen_12"), shaderIdSize);

	// Set the root arguments data. Point to start of descriptor heap
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = resources.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// Entry 1 - Miss program (no local root arguments to set)
	pData += dxr.sbtEntrySize;
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"Miss_5"), shaderIdSize);

	// Entry 2 - Closest Hit program and local root argument data (descriptor table with constant buffer and IB/VB pointers)
	pData += dxr.sbtEntrySize;
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"HitGroup"), shaderIdSize);

	// Set the root arg data. Point to start of descriptor heap
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = resources.cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// Unmap
	dxr.sbt->Unmap(0, nullptr);
}

/**
* Create the DXR descriptor heap for CBVs, SRVs, and the output UAV.
*/
void Create_CBVSRVUAV_Heap(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources, const Model &model)
{
	// Describe the CBV/SRV/UAV heap
	// Need 7 entries:
	// 1 CBV for the ViewCB
	// 1 CBV for the MaterialCB
	// 1 UAV for the RT output
	// 1 SRV for the Scene BVH
	// 1 SRV for the index buffer
	// 1 SRV for the vertex buffer
	// 1 SRV for the texture
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 7;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// Create the descriptor heap
	HRESULT hr = d3d.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&resources.cbvSrvUavHeap));
	Utils::Validate(hr, L"Error: failed to create DXR CBV/SRV/UAV descriptor heap!");

	// Get the descriptor heap handle and increment size
	D3D12_CPU_DESCRIPTOR_HANDLE handle = resources.cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	UINT handleIncrement = d3d.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the ViewCB CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = ALIGN(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(resources.viewCBData));
	cbvDesc.BufferLocation = resources.viewCB->GetGPUVirtualAddress();

	d3d.device->CreateConstantBufferView(&cbvDesc, handle);

	// Create the MaterialCB CBV
	cbvDesc.SizeInBytes = ALIGN(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(resources.materialCBData));
	cbvDesc.BufferLocation = resources.materialCB->GetGPUVirtualAddress();

	handle.ptr += handleIncrement;
	d3d.device->CreateConstantBufferView(&cbvDesc, handle);

	// Create the DXR output buffer UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	handle.ptr += handleIncrement;
	d3d.device->CreateUnorderedAccessView(resources.DXROutput, nullptr, &uavDesc, handle);

	// Create the DXR Top Level Acceleration Structure SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = dxr.TLAS.pResult->GetGPUVirtualAddress();

	handle.ptr += handleIncrement;
	d3d.device->CreateShaderResourceView(nullptr, &srvDesc, handle);

	// Create the index buffer SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC indexSRVDesc;
	indexSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	indexSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	indexSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	indexSRVDesc.Buffer.StructureByteStride = 0;
	indexSRVDesc.Buffer.FirstElement = 0;
	indexSRVDesc.Buffer.NumElements = (static_cast<UINT>(model.indices.size()) * sizeof(UINT)) / sizeof(float);
	indexSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	d3d.device->CreateShaderResourceView(resources.indexBuffer, &indexSRVDesc, handle);

	// Create the vertex buffer SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexSRVDesc;
	vertexSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vertexSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	vertexSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	vertexSRVDesc.Buffer.StructureByteStride = 0;
	vertexSRVDesc.Buffer.FirstElement = 0;
	vertexSRVDesc.Buffer.NumElements = (static_cast<UINT>(model.vertices.size()) * sizeof(Vertex)) / sizeof(float);
	vertexSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	d3d.device->CreateShaderResourceView(resources.vertexBuffer, &vertexSRVDesc, handle);

	// Create the material texture SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVDesc = {};
	textureSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	textureSRVDesc.Texture2D.MipLevels = 1;
	textureSRVDesc.Texture2D.MostDetailedMip = 0;
	textureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	d3d.device->CreateShaderResourceView(resources.texture, &textureSRVDesc, handle);
}

/**
* Create the DXR output buffer.
*/
void Create_DXR_Output(D3D12Global &d3d, D3D12Resources &resources)
{
	// Describe the DXR output resource (texture)
	// Dimensions and format should match the swapchain
	// Initialize as a copy source, since we will copy this buffer's contents to the swapchain
	D3D12_RESOURCE_DESC desc = {};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = d3d.width;
	desc.Height = d3d.height;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// Create the buffer resource
	HRESULT hr = d3d.device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&resources.DXROutput));
	Utils::Validate(hr, L"Error: failed to create DXR output buffer!");
}

/**
* Builds the frame's DXR command list
*/
void Build_Command_List(D3D12Global &d3d, DXRGlobal &dxr, D3D12Resources &resources) 
{
	D3D12_RESOURCE_BARRIER OutputBarriers[2] = {};
	D3D12_RESOURCE_BARRIER CounterBarriers[2] = {};
	D3D12_RESOURCE_BARRIER UAVBarriers[3] = {};

	// Transition the back buffer to a copy destination
	OutputBarriers[0].Transition.pResource = d3d.backBuffer[d3d.frameIndex];
	OutputBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	OutputBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	OutputBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// Transition the DXR output buffer to a copy source
	OutputBarriers[1].Transition.pResource = resources.DXROutput;
	OutputBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	OutputBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	OutputBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// Wait for the transitions to complete
	d3d.cmdList->ResourceBarrier(2, OutputBarriers);

	// Set the UAV/SRV/CBV and sampler heaps
	ID3D12DescriptorHeap* ppHeaps[] = { resources.cbvSrvUavHeap, resources.samplerHeap };
	d3d.cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// Dispatch rays
	D3D12_DISPATCH_RAYS_DESC desc = {};
	desc.RayGenerationShaderRecord.StartAddress = dxr.sbt->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = dxr.sbtEntrySize;

	desc.MissShaderTable.StartAddress = dxr.sbt->GetGPUVirtualAddress() + dxr.sbtEntrySize;
	desc.MissShaderTable.SizeInBytes = dxr.sbtEntrySize;		// Only a single Miss program entry
	desc.MissShaderTable.StrideInBytes = dxr.sbtEntrySize;

	desc.HitGroupTable.StartAddress = dxr.sbt->GetGPUVirtualAddress() + (dxr.sbtEntrySize * 2);
	desc.HitGroupTable.SizeInBytes = dxr.sbtEntrySize;			// Only a single Hit program entry
	desc.HitGroupTable.StrideInBytes = dxr.sbtEntrySize;

	desc.Width = d3d.width;
	desc.Height = d3d.height;
	desc.Depth = 1;

	d3d.cmdList->SetPipelineState1(dxr.rtpso);
	d3d.cmdList->DispatchRays(&desc);

	// Transition DXR output to a copy source
	OutputBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	OutputBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	
	// Wait for the transitions to complete
	d3d.cmdList->ResourceBarrier(1, &OutputBarriers[1]);

	// Copy the DXR output to the back buffer
	d3d.cmdList->CopyResource(d3d.backBuffer[d3d.frameIndex], resources.DXROutput);

	// Transition back buffer to present
	OutputBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	OutputBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	
	// Wait for the transitions to complete
	d3d.cmdList->ResourceBarrier(1, &OutputBarriers[0]);

	// Submit the command list and wait for the GPU to idle
	D3D12::Submit_CmdList(d3d);
	D3D12::WaitForGPU(d3d);
}

/**
 * Release DXR resources.
 */
void Destroy(DXRGlobal &dxr)
{
	SAFE_RELEASE(dxr.TLAS.pScratch);
	SAFE_RELEASE(dxr.TLAS.pResult);
	SAFE_RELEASE(dxr.TLAS.pInstanceDesc);
	SAFE_RELEASE(dxr.BLAS.pScratch);
	SAFE_RELEASE(dxr.BLAS.pResult);
	SAFE_RELEASE(dxr.BLAS.pInstanceDesc);
	SAFE_RELEASE(dxr.sbt);
	SAFE_RELEASE(dxr.rgs.blob);
	SAFE_RELEASE(dxr.miss.blob);
	SAFE_RELEASE(dxr.hit.chs.blob);
	SAFE_RELEASE(dxr.rtpso);
	SAFE_RELEASE(dxr.rtpsoInfo);
}

}
