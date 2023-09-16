#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/OBJ-Loader/Source/OBJ_Loader.h"

#include<DirectXMath.h>

#include<iostream>
#include<random>
#include<chrono>

using namespace DirectX;

// �E�B���h�E�̑傫��
constexpr std::size_t WINDOW_WIDTH = 800;
constexpr std::size_t WINDOW_HEIGHT = 600;

// �`��̈�̐��̂̋߂��Ƃ������Ƃ�
constexpr float CAMERA_NEAR_Z = 0.01f;
constexpr float CAMERA_FAR_Z = 1000.f;

// �`��̈�̐��̂̍L����
constexpr float VIEW_ANGLE = DirectX::XM_PIDIV2;

// �t���[���o�b�t�@�̐�
constexpr std::size_t FRAME_BUFFER_NUM = 2;
// �t���[���o�b�t�@�̃t�H�[�}�b�g
constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;

// �R�}���h�A���P�[�^�̐�
constexpr std::size_t COMMAND_ALLOCATORE_NUM = 1;

// �o�b�t�@�����O�̐�
constexpr std::size_t MAX_FRAMES_IN_FLIGHT = FRAME_BUFFER_NUM;

// �[�x�o�b�t�@�̃t�H�[�}�b�g
constexpr DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;

// ���f���̍ő吔
constexpr std::size_t MAX_MODEL_NUM = 1000;

// ���C�g�̍ő吔
constexpr std::size_t MAX_POINT_LIGHT_NUM = 1000;

// �萔�o�b�t�@�ɓn���f�[�^
struct ConstantBufferObject
{
	XMMATRIX model[MAX_MODEL_NUM]{};
	XMMATRIX view{};
	XMMATRIX proj{};
	XMFLOAT3 eye{};
	float _pad0{};
};

// �|�C���g���C�g�̃f�[�^
struct PointLightData
{
	XMFLOAT4 pos{};
	XMFLOAT4 color{};
};

// ���C�g�̒萔�o�b�t�@�ɓn���f�[�^
struct LightConstantBufferObject
{
	std::array<PointLightData, MAX_POINT_LIGHT_NUM> pointLights{};
	std::uint32_t pointLightNum;
};

int main()
{
	
	std::size_t modelEdgeNum = 6;
	std::size_t modelToltalNum = modelEdgeNum * modelEdgeNum * modelEdgeNum;
	constexpr float modelStrideLen = 8.f;
	std::size_t pointLightNum = 50;

	// �v���t���[����
	std::size_t frameCntNum;
	std::cout << "frame num: ";
	std::cin >> frameCntNum;

	// �E�B���h�E�n���h��
	auto hwnd = dx12w::create_window(L"forward", WINDOW_WIDTH, WINDOW_HEIGHT);

	// �f�o�C�X
	auto device = dx12w::create_device();

	// �R�}���h
	dx12w::command_manager<COMMAND_ALLOCATORE_NUM> commandManager{};
	commandManager.initialize(device.get());

	// �X���b�v�`�F�C��
	auto swapChain = dx12w::create_swap_chain(commandManager.get_queue(), hwnd, FRAME_BUFFER_FORMAT, FRAME_BUFFER_NUM);

	// �t���[���o�b�t�@���N���A����l
	constexpr std::array<float, 4> grayColor{ 0.5f,0.5f,0.5f,1.f };

	// �[�x�o�b�t�@���N���A����l
	constexpr D3D12_CLEAR_VALUE depthBufferClearValue{
		.Format = DEPTH_BUFFER_FORMAT,
		.DepthStencil = {.Depth = 1.f }
	};


	//
	// ���\�[�X
	//

	// �t���[���o�b�t�@
	std::array<dx12w::resource_and_state, FRAME_BUFFER_NUM> frameBufferResources{};
	for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
	{
		ID3D12Resource* tmp = nullptr;
		swapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&tmp));
		frameBufferResources[i] = std::make_pair(dx12w::release_unique_ptr<ID3D12Resource>{tmp}, D3D12_RESOURCE_STATE_COMMON);
	}

	// ���f���̒��_���
	dx12w::resource_and_state modelVertexBuffer{};
	D3D12_VERTEX_BUFFER_VIEW modelVertexBufferView{};
	std::size_t modelVertexNum = 0;
	{
		objl::Loader Loader;

		bool loadout = Loader.LoadFile("model/teapot.obj");
		objl::Mesh mesh = Loader.LoadedMeshes[0];

		modelVertexBuffer = dx12w::create_commited_upload_buffer_resource(device.get(), sizeof(float) * 6 * mesh.Vertices.size());
		{
			float* vertexBufferPtr = nullptr;
			modelVertexBuffer.first->Map(0, nullptr, reinterpret_cast<void**>(&vertexBufferPtr));

			for (std::size_t i = 0; i < mesh.Vertices.size(); i++)
			{
				vertexBufferPtr[i * 6] = mesh.Vertices[i].Position.X;
				vertexBufferPtr[i * 6 + 1] = mesh.Vertices[i].Position.Y;
				vertexBufferPtr[i * 6 + 2] = mesh.Vertices[i].Position.Z;
				vertexBufferPtr[i * 6 + 3] = mesh.Vertices[i].Normal.X;
				vertexBufferPtr[i * 6 + 4] = mesh.Vertices[i].Normal.Y;
				vertexBufferPtr[i * 6 + 5] = mesh.Vertices[i].Normal.Z;
			}

			modelVertexBuffer.first->Unmap(0, nullptr);
		}

		modelVertexBufferView.BufferLocation = modelVertexBuffer.first->GetGPUVirtualAddress();
		modelVertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(float) * 6 * mesh.Vertices.size());
		modelVertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(float) * 6);

		modelVertexNum = mesh.Vertices.size();
	}

	// �萔�o�b�t�@�̃��\�[�X
	std::array<dx12w::resource_and_state, MAX_FRAMES_IN_FLIGHT> constantBuffers{
		dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(ConstantBufferObject), 256)),
		dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(ConstantBufferObject), 256))
	};

	// �萔�o�b�t�@�̃��\�[�X�̃|�C���^
	std::array<ConstantBufferObject*, MAX_FRAMES_IN_FLIGHT> constantBufferPtrs{};
	for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		constantBuffers[i].first->Map(0, nullptr, reinterpret_cast<void**>(&constantBufferPtrs[i]));
	}

	// �[�x�o�b�t�@�̃��\�[�X
	auto depthBuffer = dx12w::create_commited_texture_resource(device.get(), DEPTH_BUFFER_FORMAT, WINDOW_WIDTH, WINDOW_HEIGHT,
		2, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depthBufferClearValue);

	// ���C�g�p�̒萔�o�b�t�@�̃��\�[�X
	std::array<dx12w::resource_and_state, MAX_FRAMES_IN_FLIGHT> lightConstantBuffers{
		dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(LightConstantBufferObject), 256)),
		dx12w::create_commited_upload_buffer_resource(device.get(), dx12w::alignment<UINT64>(sizeof(LightConstantBufferObject), 256))
	};

	// ���C�g�p�̒萔�o�b�t�@�̃��\�[�X�̃|�C���^
	std::array<LightConstantBufferObject*, MAX_FRAMES_IN_FLIGHT> lightConstantBufferPtrs{};
	for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		lightConstantBuffers[i].first->Map(0, nullptr, reinterpret_cast<void**>(&lightConstantBufferPtrs[i]));
	}


	// 
	// �f�B�X�N���v�^�q�[�v
	//

	// �t���[���o�b�t�@�̃����_�[�^�[�Q�b�g�r���[���쐬���邽�߂̃f�B�X�N���v�^�q�[�v
	dx12w::descriptor_heap frameBufferDescriptorHeapRTV{};
	{
		// ��O�����Ńt���[���o�b�t�@�̐����w��
		frameBufferDescriptorHeapRTV.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);

		// ���ꂼ��̃t���[���o�b�t�@�̃r���[���쐬
		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
			dx12w::create_texture2D_RTV(device.get(), frameBufferDescriptorHeapRTV.get_CPU_handle(i), frameBufferResources[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);
	}

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p����萔�o�b�t�@�p�̃r���[�쐬���邽�߂̃f�B�X�N���v�^�q�[�v
	std::array<dx12w::descriptor_heap,MAX_FRAMES_IN_FLIGHT> frameBufferDescriptorHeapCBVSRVUAVs{};
	for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		// ��O�����ł̐��̎w��͒萔�o�b�t�@�����Ȃ̂łP
		frameBufferDescriptorHeapCBVSRVUAVs[i].initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);

		// �萔�o�b�t�@
		dx12w::create_CBV(device.get(), frameBufferDescriptorHeapCBVSRVUAVs[i].get_CPU_handle(0),
			constantBuffers[i].first.get(), dx12w::alignment<UINT>(sizeof(ConstantBufferObject), 256));

		// ���C�g�̒萔�o�b�t�@
		dx12w::create_CBV(device.get(), frameBufferDescriptorHeapCBVSRVUAVs[i].get_CPU_handle(1),
			lightConstantBuffers[i].first.get(), dx12w::alignment<UINT>(sizeof(LightConstantBufferObject), 256));
	}

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p����[�x�o�b�t�@�p�̃r���[���쐬���邽�߂̃f�B�X�N���v�^�q�[�v
	dx12w::descriptor_heap frameBufferDescriptorHeapDSV{};
	{
		frameBufferDescriptorHeapDSV.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

		dx12w::create_texture2D_DSV(device.get(), frameBufferDescriptorHeapDSV.get_CPU_handle(), depthBuffer.first.get(), DEPTH_BUFFER_FORMAT, 0);
	}

	// 
	// �V�F�[�_
	// 

	// ���_�V�F�[�_
	std::ifstream vertexShaderCSO{ L"Shader/VertexShader.cso",std::ios::binary };
	auto vertexShader = dx12w::load_blob(vertexShaderCSO);
	vertexShaderCSO.close();

	// �s�N�Z���V�F�[�_
	std::ifstream pixelShaderCSO{ L"Shader/PixelShader.cso" ,std::ios::binary };
	auto pixelShader = dx12w::load_blob(pixelShaderCSO);
	pixelShaderCSO.close();


	//
	// ���[�g�V�O�l�`���ƃp�C�v���C�� 
	// 

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p���郋�[�g�V�O�l�`��
	auto frameBufferRootSignature = dx12w::create_root_signature(device.get(),
		{ {{
				// ConstantBufferObject
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				// LightConstantBufferObject
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV
			}} },
		{});

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p����O���t�B�b�N�p�C�v���C��
	auto modelGBufferGraphicsPipelineState = dx12w::create_graphics_pipeline(device.get(), frameBufferRootSignature.get(),
		{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT } },
		{ FRAME_BUFFER_FORMAT},
		{ {vertexShader.data(),vertexShader.size()} ,{pixelShader.data(),pixelShader.size()} }
	, true, false, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);


	//
	// ���̑�
	//
	
	// �r���[�|�[�g
	constexpr D3D12_VIEWPORT viewport{ 0.f,0.f, static_cast<float>(WINDOW_WIDTH),static_cast<float>(WINDOW_HEIGHT),0.f,1.f };
	// �V�U�[���N�g
	constexpr D3D12_RECT scissorRect{ 0,0,static_cast<LONG>(WINDOW_WIDTH),static_cast<LONG>(WINDOW_HEIGHT) };

	XMFLOAT3 eye{ 0.f,30.f,30.f };
	XMFLOAT3 target{ 0,0,0 };
	XMFLOAT3 up{ 0,1,0 };
	float asspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

	// ���C�g�̃f�[�^���}�b�v
	{
		std::random_device seed_gen;
		std::mt19937 engine(seed_gen());

		// ���f���̂���̈�̈�l���z
		std::uniform_real_distribution<float> posRnd(-((modelEdgeNum - 1) * modelStrideLen / 2.f), (modelEdgeNum - 1)* modelStrideLen / 2.f);
		
		std::uniform_real_distribution<float> colorRnd(0.f, 0.8f);

		for (std::size_t i = 0; i < pointLightNum; i++)
		{
			auto pos = XMFLOAT4{ posRnd(engine),posRnd(engine) ,posRnd(engine) ,0.f };
			auto color = XMFLOAT4{ colorRnd(engine),colorRnd(engine) ,colorRnd(engine) ,0.f};

			for (std::size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
			{
				lightConstantBufferPtrs[j]->pointLights[i].pos = pos;
				lightConstantBufferPtrs[j]->pointLights[i].color = color;
			}
		}

		for (std::size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
		{
			lightConstantBufferPtrs[j]->pointLightNum = pointLightNum;
		}
	}


	//
	// ���C�����[�v
	//

	auto start = std::chrono::system_clock::now();

	std::size_t frameCnt = 0;
	while (dx12w::update_window())
	{
		frameCnt++;

		auto backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		// �萔�o�b�t�@�̃f�[�^���}�b�v
		for (std::size_t x_i = 0; x_i < modelEdgeNum; x_i++)
			for (std::size_t y_i = 0; y_i < modelEdgeNum; y_i++)
				for (std::size_t z_i = 0; z_i < modelEdgeNum; z_i++)
					constantBufferPtrs[backBufferIndex]->model[x_i + y_i * modelEdgeNum + z_i * modelEdgeNum * modelEdgeNum] =
					XMMatrixRotationY(static_cast<float>(frameCnt) / 50.f) *
					XMMatrixTranslation(
						x_i * modelStrideLen - (modelEdgeNum - 1) * modelStrideLen / 2.f,
						y_i * modelStrideLen - (modelEdgeNum - 1) * modelStrideLen / 2.f,
						z_i * modelStrideLen - (modelEdgeNum - 1) * modelStrideLen / 2.f
					);
		constantBufferPtrs[backBufferIndex]->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		constantBufferPtrs[backBufferIndex]->proj = DirectX::XMMatrixPerspectiveFovLH(VIEW_ANGLE, asspect, CAMERA_NEAR_Z, CAMERA_FAR_Z);
		constantBufferPtrs[backBufferIndex]->eye = eye;

		// �R�}���h�̃��Z�b�g
		commandManager.reset_list(0);

		// ���\�[�X�o���A
		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
		dx12w::resource_barrior(commandManager.get_list(), depthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		// �����_�[�^�[�Q�b�g���N���A
		commandManager.get_list()->ClearRenderTargetView(frameBufferDescriptorHeapRTV.get_CPU_handle(backBufferIndex), grayColor.data(), 0, nullptr);

		// �[�x�o�b�t�@���N���A
		commandManager.get_list()->ClearDepthStencilView(frameBufferDescriptorHeapDSV.get_CPU_handle(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

		// �`��̈�̎w��
		commandManager.get_list()->RSSetViewports(1, &viewport);
		commandManager.get_list()->RSSetScissorRects(1, &scissorRect);

		// �����_�[�^�[�Q�b�g�̎w��
		auto frameBufferCPUHandle = frameBufferDescriptorHeapRTV.get_CPU_handle(backBufferIndex);
		auto depthBufferCPUHandle = frameBufferDescriptorHeapDSV.get_CPU_handle(0);
		commandManager.get_list()->OMSetRenderTargets(1, &frameBufferCPUHandle, false, &depthBufferCPUHandle);

		//
		// ���f���̕`��
		//

		commandManager.get_list()->SetGraphicsRootSignature(frameBufferRootSignature.get());
		{
			auto ptr = frameBufferDescriptorHeapCBVSRVUAVs[backBufferIndex].get();
			commandManager.get_list()->SetDescriptorHeaps(1, &ptr);
		}
		commandManager.get_list()->SetGraphicsRootDescriptorTable(0, frameBufferDescriptorHeapCBVSRVUAVs[backBufferIndex].get_GPU_handle(0));
		commandManager.get_list()->SetPipelineState(modelGBufferGraphicsPipelineState.get());
		commandManager.get_list()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandManager.get_list()->IASetVertexBuffers(0, 1, &modelVertexBufferView);

		commandManager.get_list()->DrawInstanced(static_cast<UINT>(modelVertexNum), modelToltalNum, 0, 0);

		//
		// ���f���̕`��I���
		//

		// ���\�[�X�o���A
		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_COMMON);
		dx12w::resource_barrior(commandManager.get_list(), depthBuffer, D3D12_RESOURCE_STATE_COMMON);

		// �R�}���h�̎��s����
		commandManager.get_list()->Close();
		commandManager.excute();
		commandManager.signal();

		// �R�}���h�̏I����҂�
		commandManager.wait(0);

		if (frameCnt >= frameCntNum) {
			break;
		}

		// �o�b�N�o�b�t�@���X���b�v
		swapChain->Present(1, 0);
	}

	auto end = std::chrono::system_clock::now();

	// �E�B���h�E�̔j��
	DestroyWindow(hwnd);

	auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	std::cout << "time: " << time << std::endl;
	std::cout << "time per frame: " << time / static_cast<double>(frameCntNum) << std::endl;

	system("PAUSE");

	return 0;
}