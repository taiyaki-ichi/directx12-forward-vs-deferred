#include"../external/directx12-wrapper/dx12w/dx12w.hpp"


constexpr std::size_t WINDOW_WIDTH = 800;
constexpr std::size_t WINDOW_HEIGHT = 600;

int main()
{
	auto hwnd = dx12w::create_window(L"forward", WINDOW_WIDTH, WINDOW_HEIGHT);

	while (dx12w::update_window())
	{

	}
}