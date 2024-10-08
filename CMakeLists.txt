cmake_minimum_required(VERSION 3.14)

set(TARGET noflicker_directx_window)
project(${TARGET})

# Global flags

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")   # No idea who removed /Zi flag from the debug conf. Putting it back
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /MDd")   # No idea who removed /Zi flag from the debug conf. Putting it back
add_subdirectory(third_party/DirectX-Headers)

# Demo for DirectX 11

set(EXE_DX11 noflicker_directx11_window)
add_executable(${EXE_DX11} WIN32
        main.cpp

        D3DContext.h
        D3DContext_DX11.cpp

        DDSTextureLoader.h
        DDSTextureLoader.cpp

        DCompContext.h
        DCompContext.cpp

        D3DContextBase.cpp Base.h GraphicContents.h)

target_compile_definitions(${EXE_DX11} PUBLIC WINVER=0x0602 UNICODE _UNICODE USE_DX11)
target_compile_features(${EXE_DX11} PUBLIC cxx_std_20)
target_link_libraries(${EXE_DX11} PUBLIC D3D11 dxgi dxguid D3DCompiler Dcomp)
add_custom_command(
        TARGET ${EXE_DX11} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/grass.dds
        $<TARGET_FILE_DIR:${EXE_DX11}>)

set_target_properties(${EXE_DX11}
        PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE  /ENTRY:WinMainCRTStartup /DDEBUG /D_DEBUG"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        )

# Demo for DirectX 12

set(EXE_DX12 noflicker_directx12_window)
add_executable(${EXE_DX12} WIN32
        main.cpp
        D3DContext.h
        D3DContext_DX12.cpp

        DDSTextureLoader12.h
        DDSTextureLoader12.cpp

        DCompContext.h
        DCompContext.cpp D3DContextBase.cpp Base.h GraphicContents.h)

add_dependencies(${EXE_DX12} DirectX-Headers)
target_include_directories(${EXE_DX12} PUBLIC ${DirectX-Headers_SOURCE_DIR}/include)
target_compile_definitions(${EXE_DX12} PUBLIC WINVER=0x0602 UNICODE _UNICODE USE_DX12 USING_DIRECTX_HEADERS)
target_compile_features(${EXE_DX12} PUBLIC cxx_std_20)
target_link_libraries(${EXE_DX12} PUBLIC D3D12 dxgi D3DCompiler Dcomp DirectX-Headers DirectX-Guids)

set_target_properties(${EXE_DX12}
        PROPERTIES
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE  /ENTRY:WinMainCRTStartup /DDEBUG /D_DEBUG"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup"
        )