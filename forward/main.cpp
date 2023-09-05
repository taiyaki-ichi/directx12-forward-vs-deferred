#include"../external/directx12-wrapper/dx12w/dx12w.hpp"


constexpr std::size_t WINDOW_WIDTH = 800;
constexpr std::size_t WINDOW_HEIGHT = 600;

constexpr std::size_t FRAME_BUFFER_NUM = 2;
constexpr DXGI_FORMAT FRAME_BUFFER_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;

constexpr std::size_t COMMAND_ALLOCATORE_NUM = 1;

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

	// �t���[���o�b�t�@�̃r���[���쐬����p�̃f�X�N���v�^�q�[�v
	dx12w::descriptor_heap frameBufferDescriptorHeapRTV{};
	{
		frameBufferDescriptorHeapRTV.initialize(device.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FRAME_BUFFER_NUM);

		for (std::size_t i = 0; i < FRAME_BUFFER_NUM; i++)
			dx12w::create_texture2D_RTV(device.get(), frameBufferDescriptorHeapRTV.get_CPU_handle(i), frameBufferResources[i].first.get(), FRAME_BUFFER_FORMAT, 0, 0);
	}


	while (dx12w::update_window())
	{
		auto backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		commandManager.reset_list(0);

		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandManager.get_list()->ClearRenderTargetView(frameBufferDescriptorHeapRTV.get_CPU_handle(backBufferIndex), grayColor.data(), 0, nullptr);

		//
		//
		//

		dx12w::resource_barrior(commandManager.get_list(), frameBufferResources[backBufferIndex], D3D12_RESOURCE_STATE_COMMON);

		commandManager.get_list()->Close();
		commandManager.excute();
		commandManager.signal();

		commandManager.wait(0);

		swapChain->Present(1, 0);
	}
}