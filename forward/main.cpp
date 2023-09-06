#include"../external/directx12-wrapper/dx12w/dx12w.hpp"
#include"../external/OBJ-Loader/Source/OBJ_Loader.h"

#include<DirectXMath.h>
#include<iostream>

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

// �萔�o�b�t�@�ɓn���f�[�^
struct ConstantBufferObject
{
	XMMATRIX model{};
	XMMATRIX view{};
	XMMATRIX proj{};
};

int main()
{
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
		frameBufferDescriptorHeapCBVSRVUAVs[i].initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

		// �萔�o�b�t�@
		dx12w::create_CBV(device.get(), frameBufferDescriptorHeapCBVSRVUAVs[i].get_CPU_handle(0),
			constantBuffers[i].first.get(), dx12w::alignment<UINT>(sizeof(ConstantBufferObject), 256));
	}


	// ���_�V�F�[�_
	std::ifstream vertexShaderCSO{ L"Shader/VertexShader.cso",std::ios::binary };
	auto vertexShader = dx12w::load_blob(vertexShaderCSO);
	vertexShaderCSO.close();

	// �s�N�Z���V�F�[�_
	std::ifstream pixelShaderCSO{ L"Shader/PixelShader.cso" ,std::ios::binary };
	auto pixelShader = dx12w::load_blob(pixelShaderCSO);
	pixelShaderCSO.close();

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p���郋�[�g�V�O�l�`��
	auto frameBufferRootSignature = dx12w::create_root_signature(device.get(),
		{ {{
				// ConstantBufferObject
				D3D12_DESCRIPTOR_RANGE_TYPE_CBV
			}} },
		{});

	// �t���[���o�b�t�@�ɕ`�悷��ۂɎg�p����O���t�B�b�N�p�C�v���C��
	auto modelGBufferGraphicsPipelineState = dx12w::create_graphics_pipeline(device.get(), frameBufferRootSignature.get(),
		{ { "POSITION",DXGI_FORMAT_R32G32B32_FLOAT },{ "NORMAL",DXGI_FORMAT_R32G32B32_FLOAT } },
		{ FRAME_BUFFER_FORMAT},
		{ {vertexShader.data(),vertexShader.size()} ,{pixelShader.data(),pixelShader.size()} }
	, false, false, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	// �r���[�|�[�g
	constexpr D3D12_VIEWPORT viewport{ 0.f,0.f, static_cast<float>(WINDOW_WIDTH),static_cast<float>(WINDOW_HEIGHT),0.f,1.f };
	// �V�U�[���N�g
	constexpr D3D12_RECT scissorRect{ 0,0,static_cast<LONG>(WINDOW_WIDTH),static_cast<LONG>(WINDOW_HEIGHT) };

	XMFLOAT3 eye{ 0.f,5.f,5.f };
	XMFLOAT3 target{ 0,0,0 };
	XMFLOAT3 up{ 0,1,0 };
	float asspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

	while (dx12w::update_window())
	{
		auto backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		// �萔�o�b�t�@�̃f�[�^���}�b�v
		constantBufferPtrs[backBufferIndex]->model = XMMatrixIdentity();
		constantBufferPtrs[backBufferIndex]->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		constantBufferPtrs[backBufferIndex]->proj = DirectX::XMMatrixPerspectiveFovLH(VIEW_ANGLE, asspect, CAMERA_NEAR_Z, CAMERA_FAR_Z);

		// �R�}���h�̃��Z�b�g
		commandManager.reset_list(0);

		// ���\�[�X�o���A
		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

		// �����_�[�^�[�Q�b�g���N���A
		commandManager.get_list()->ClearRenderTargetView(frameBufferDescriptorHeapRTV.get_CPU_handle(backBufferIndex), grayColor.data(), 0, nullptr);

		// �`��̈�̎w��
		commandManager.get_list()->RSSetViewports(1, &viewport);
		commandManager.get_list()->RSSetScissorRects(1, &scissorRect);

		// �����_�[�^�[�Q�b�g�̎w��
		auto frameBufferCPUHandle = frameBufferDescriptorHeapRTV.get_CPU_handle(backBufferIndex);
		commandManager.get_list()->OMSetRenderTargets(1, &frameBufferCPUHandle, false, nullptr);

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

		commandManager.get_list()->DrawInstanced(static_cast<UINT>(modelVertexNum), 1, 0, 0);

		//
		// ���f���̕`��I���
		//

		// ���\�[�X�o���A
		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_COMMON);

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