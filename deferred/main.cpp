#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/OBJ-Loader/Source/OBJ_Loader.h"

#include<DirectXMath.h>

#include<iostream>
#include<random>

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



	// 
	// �V�F�[�_
	// 




	//
	// ���[�g�V�O�l�`���ƃp�C�v���C�� 
	// 



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
		std::uniform_real_distribution<float> posRnd(-((modelEdgeNum - 1) * modelStrideLen / 2.f), (modelEdgeNum - 1) * modelStrideLen / 2.f);

		std::uniform_real_distribution<float> colorRnd(0.f, 0.8f);

		for (std::size_t i = 0; i < pointLightNum; i++)
		{
			auto pos = XMFLOAT4{ posRnd(engine),posRnd(engine) ,posRnd(engine) ,0.f };
			auto color = XMFLOAT4{ colorRnd(engine),colorRnd(engine) ,colorRnd(engine) ,0.f };

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

		
		//
		//
		//


		// �R�}���h�̎��s����
		commandManager.get_list()->Close();
		commandManager.excute();
		commandManager.signal();

		// �R�}���h�̏I����҂�
		commandManager.wait(0);

		// �o�b�N�o�b�t�@���X���b�v
		swapChain->Present(1, 0);
	}
}