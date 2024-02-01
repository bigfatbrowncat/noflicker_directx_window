cmake_minimum_required(VERSION 3.14)
set(TARGET noflicker_directx_window)
project(${TARGET})

# Global flags

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /MT")   # No idea who removed /Zi flag from the debug conf. Putting it back
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /MTd")   # No idea who removed /Zi flag from the debug conf. Putting it back
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

# Demo for DirectX 11

set(EXE_DX11 noflicker_directx11_window)
add_executable(${EXE_DX11} WIN32
        main.cpp

        D3DContext.h
        D3D11Context.cpp

        DCompContext.h
        DCompContext.cpp)

target_compile_definitions(${EXE_DX11} PUBLIC WINVER=0x0602 UNICODE _UNICODE USE_DX11)
target_compile_features(${EXE_DX11} PUBLIC cxx_std_17)
target_link_libraries(${EXE_DX11} PUBLIC D3D11 dxgi D3DCompiler Dcomp)

#set_target_properties(${EXE_DX11}
#        PROPERTIES
#        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE  /ENTRY:WinMainCRTStartup"
#        LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
#        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
#        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
#        )

# Demo for DirectX 12

set(EXE_DX12 noflicker_directx12_window)
add_executable(${EXE_DX12} WIN32
        main.cpp
        D3DContext.h
        D3D12Context.cpp

        DCompContext.h
        DCompContext.cpp)

target_compile_definitions(${EXE_DX12} PUBLIC WINVER=0x0602 UNICODE _UNICODE USE_DX12)
target_compile_features(${EXE_DX12} PUBLIC cxx_std_17)
target_link_libraries(${EXE_DX12} PUBLIC D3D12 dxgi D3DCompiler Dcomp)

