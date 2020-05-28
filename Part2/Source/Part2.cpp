
#include <iostream>
#include <D3D12GEPUtils.h>

int main()
{
	std::cout << "Hello from Part 2!" << std::endl;

	D3D12GEPUtils::PrintHello();

	D3D12GEPUtils::GetMainAdapter(false);


	//Wait for Enter key press before returning
	getchar();
}