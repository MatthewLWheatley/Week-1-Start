#include "DX11Renderer.h"
#include "Scene.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "d3dcompiler.h"

// Apparently we're doing a mode switcher because committing to one rendering system is too mainstream
// TRUE - PBR Rendering / FALSE - Animation Rendering
constexpr bool PBR_MODE = true;

#pragma region Class lifetime

// Main initialization - where everything goes right until it doesn't
HRESULT DX11Renderer::init(HWND hwnd)
{
    // Initialize the D3D11 device and swap chain - aka "please don't crash immediately"
    initDevice(hwnd);

    // Create and initialize the scene because rendering nothing would be boring
    m_pScene = new Scene;
    m_pScene->init(hwnd, m_pd3dDevice, m_pImmediateContext, this);

    // Set up projection matrix - making 3D things look 3D on your 2D screen (magic)
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    constexpr float fovAngleY = XMConvertToRadians(60.0f); // 60 degrees because that's what everyone uses
    XMStoreFloat4x4(&m_matProjection, XMMatrixPerspectiveFovLH(fovAngleY, width / (FLOAT)height, 0.01f, 100.0f));

    // Initialize ImGUI so you can have a debug menu that you'll probably never style properly
    initIMGUI(hwnd);

    HRESULT hr;

    // Compile vertex shader - choose your fighter edition
    ID3DBlob* pVSBlob = nullptr;
    if constexpr (PBR_MODE)
        hr = DX11Renderer::compileShaderFromFile(L"shader_me.hlsl", "VS", "vs_4_0", &pVSBlob);
    else
        hr = DX11Renderer::compileShaderFromFile(L"skinned_shader.hlsl", "VS", "vs_4_0", &pVSBlob);

    // If shader compilation fails, show a message box that the user will ignore anyway
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    // Create the actual vertex shader from the compiled blob
    hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Define input layout - tell the GPU what your vertex data looks like or it'll just guess (badly)
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },    // Where the vertex is
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },      // Which way it's facing
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },  // For normal mapping (fancy bumps)
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },        // UV coordinates for textures
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Which bones affect this vertex
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }  // How much each bone affects it
    };

    UINT numElements = ARRAYSIZE(layout);

    // Create input layout from the description - more GPU handholding
    hr = m_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &m_pVertexLayout);
    pVSBlob->Release(); // Clean up the blob because memory leaks are for amateurs
    if (FAILED(hr))
        return hr;

    // Bind the input layout to the pipeline
    m_pImmediateContext->IASetInputLayout(m_pVertexLayout.Get());

    // Compile pixel shader - round two of "please compile"
    ID3DBlob* pPSBlob = nullptr;

    if constexpr (PBR_MODE)
        hr = DX11Renderer::compileShaderFromFile(L"shader_me.hlsl", "PS_Normal", "ps_4_0", &pPSBlob);
    else
        hr = DX11Renderer::compileShaderFromFile(L"skinned_shader.hlsl", "PS", "ps_4_0", &pPSBlob);

    // Same error message copy-pasted because DRY is for other people
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

    // Create pixel shader - this one decides what color your pixels are
    hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
    pPSBlob->Release(); // More cleanup
    if (FAILED(hr))
        return hr;

    return hr;
}

// Initialize DirectX device - the most painful part of graphics programming
HRESULT DX11Renderer::initDevice(HWND hwnd)
{
    HRESULT hr = S_OK;

    // Get window dimensions because we need to know how big our canvas is
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    // Enable debug layer in debug builds so you can see all your mistakes in real-time
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Try hardware first, then progressively worse fallbacks if your GPU is having a bad day
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,   // Your actual GPU
        D3D_DRIVER_TYPE_WARP,       // Software rasterizer (slow but works)
        D3D_DRIVER_TYPE_REFERENCE,  // Extremely slow but accurate reference implementation
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // Feature levels - try the newest first, fall back to older ones if needed
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // Loop through driver types until one works (fingers crossed)
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);

        // DirectX 11.0 doesn't know about 11.1, so retry without it
        if (hr == E_INVALIDARG)
        {
            hr = D3D11CreateDevice(nullptr, m_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &m_pd3dDevice, &m_featureLevel, &m_pImmediateContext);
        }

        if (SUCCEEDED(hr))
            break; // Found a working driver, we're good
    }
    if (FAILED(hr))
        return hr; // None of them worked, RIP

    // Get DXGI factory from device - needed for creating swap chain
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = m_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create device.", L"Error", MB_OK);
        return hr;
    }

    // Create swap chain - double/triple buffering so you don't see half-drawn frames
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 or later path - the "modern" way
        hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), &m_pd3dDevice1);
        if (SUCCEEDED(hr))
        {
            (void)m_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), &m_pImmediateContext1);
        }

        // Swap chain description - using HDR format for that sweet sweet color range
        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 16-bit float per channel, fancy
        sd.SampleDesc.Count = 1;   // No MSAA because who needs anti-aliasing anyway
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1; // Single buffered because we're living dangerously

        hr = dxgiFactory2->CreateSwapChainForHwnd(m_pd3dDevice.Get(), hwnd, &sd, nullptr, nullptr, &m_pSwapChain1);
        if (SUCCEEDED(hr))
        {
            hr = m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), &m_pSwapChain);
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems - the "I'm on ancient hardware" path
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Regular 8-bit per channel
        sd.BufferDesc.RefreshRate.Numerator = 60;   // 60 Hz
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE; // Fullscreen is overrated

        hr = dxgiFactory->CreateSwapChain(m_pd3dDevice.Get(), &sd, &m_pSwapChain);
    }

    // Disable ALT+ENTER for fullscreen toggle because we don't handle it
    dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create swapchain.", L"Error", MB_OK);
        return hr;
    }

    // Create render target view - where we actually draw stuff
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a back buffer.", L"Error", MB_OK);
        return hr;
    }

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a render target.", L"Error", MB_OK);
        return hr;
    }

    // Create depth stencil texture - for knowing what's in front of what
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24-bit depth, 8-bit stencil
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = m_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &m_pDepthStencil);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil texture.", L"Error", MB_OK);
        return hr;
    }

    // Create depth stencil view - so we can actually use the depth buffer
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &descDSV, &m_pDepthStencilView);
    if (FAILED(hr))
    {
        MessageBox(nullptr,
            L"Failed to create a depth / stencil view.", L"Error", MB_OK);
        return hr;
    }

    // Bind render target and depth buffer to output merger stage
    ID3D11RenderTargetView* rtv = m_pRenderTargetView.Get();
    m_pImmediateContext->OMSetRenderTargets(1, &rtv, m_pDepthStencilView.Get());

    // Setup viewport - define the area we're drawing to
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f; // Near plane
    vp.MaxDepth = 1.0f; // Far plane
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pImmediateContext->RSSetViewports(1, &vp);

    return S_OK;
}

// Cleanup - say goodbye to everything
void DX11Renderer::cleanUp()
{
    cleanupDevice();

    // Shutdown ImGUI properly or it'll complain
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Clean up scene and free memory like a responsible programmer
    m_pScene->cleanUp();
    delete m_pScene;
}

// Cleanup DirectX device - unbind everything and check for leaks
void DX11Renderer::cleanupDevice()
{
    // Unbind all render targets - important for proper cleanup
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_pImmediateContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);

    // Clear pipeline state
    if (m_pImmediateContext) m_pImmediateContext->ClearState();

    // Flush commands to GPU - make sure everything's done
    if (m_pImmediateContext1) m_pImmediateContext1->Flush();
    m_pImmediateContext->Flush();

    // ComPtr handles releasing automatically, so we just reset

    // Get debug interface to check for memory leaks (debug builds only)
    ID3D11Debug* debugDevice = nullptr;
    m_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDevice));

    m_pd3dDevice.Reset();

    if (debugDevice != nullptr)
    {
        // Print live objects to debug output - see what you forgot to release
        debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
}

// Compile HLSL shader from file - the thing that fails 90% of the time
HRESULT DX11Renderer::compileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Debug builds: enable shader debugging and disable optimizations
    // Makes shaders slower but easier to debug when they inevitably break
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            // Print shader compilation errors to debug output - actually useful
            MessageBoxA(NULL, reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()),NULL,MB_OK);
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

// Initialize Dear ImGui - for debug UI that's better than printf
void DX11Renderer::initIMGUI(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Gamepad navigation (for couch developers)

    // Setup platform/renderer backends - link ImGui to DirectX
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_pd3dDevice.Get(), m_pImmediateContext.Get());
}

// Handle input - WASD movement and mouse look
void DX11Renderer::input(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    float movement = 0.02f; // Movement speed - adjust if you want to go nyoom
    static bool mouseDown = false;

    // Let ImGui handle its own input first
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return;

    // WASD movement - because arrow keys are for boomers
    if (GetAsyncKeyState('W'))
    {
        m_pScene->getCamera()->moveForward(movement);
    }
    if (GetAsyncKeyState('A'))
    {
        m_pScene->getCamera()->strafeLeft(movement);
    }
    if (GetAsyncKeyState('S'))
    {
        m_pScene->getCamera()->moveBackward(movement);
    }
    if (GetAsyncKeyState('D'))
    {
        m_pScene->getCamera()->strafeRight(movement);
    }

    switch (message)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 27: // ESC key
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_RBUTTONDOWN:
        mouseDown = true; // Right mouse button pressed - enter look mode
        break;
    case WM_RBUTTONUP:
        mouseDown = false; // Released - exit look mode
        break;
    case WM_MOUSEMOVE:
    {
        if (!mouseDown)
        {
            break; // Only care about mouse movement when right button is held
        }

        // Get window center - we'll reset mouse here each frame
        RECT rect;
        GetClientRect(hWnd, &rect);

        POINT windowCenter;
        windowCenter.x = (rect.right - rect.left) / 2;
        windowCenter.y = (rect.bottom - rect.top) / 2;

        // Convert to screen coordinates
        ClientToScreen(hWnd, &windowCenter);

        // Get current mouse position
        POINTS mousePos = MAKEPOINTS(lParam);
        POINT cursorPos = { mousePos.x, mousePos.y };
        ClientToScreen(hWnd, &cursorPos);

        // Calculate how far mouse moved from center
        POINT delta;
        delta.x = cursorPos.x - windowCenter.x;
        delta.y = cursorPos.y - windowCenter.y;

        // Update camera rotation based on mouse movement
        m_pScene->getCamera()->updateLookAt({ static_cast<short>(delta.x), static_cast<short>(delta.y) });

        // Reset mouse to center - FPS-style mouse look
        SetCursorPos(windowCenter.x, windowCenter.y);
    }
    break;
    case WM_ACTIVATE:
        // When window gains focus, center the mouse
        if (LOWORD(wParam) != WA_INACTIVE) {
            CentreMouseInWindow(hWnd);
        }
        break;
    }
}

// Center mouse in window - for initial setup
void DX11Renderer::CentreMouseInWindow(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    POINT center;
    center.x = (rect.right - rect.left) / 2;
    center.y = (rect.bottom - rect.top) / 2;

    ClientToScreen(hWnd, &center);

    SetCursorPos(center.x, center.y);
}

// Start ImGui frame - begin drawing UI
void DX11Renderer::startIMGUIDraw(const unsigned int FPS)
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Basic debug info - FPS counter and controls
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text("FPS %d", FPS);
    ImGui::Text("Use WASD to move, RMB to look");
    ImGui::Text("%f", m_pScene->time);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    // Example commented code for more ImGui widgets - uncomment to use
    // ImGui::ShowMetricsWindow(); // Shows internal ImGui metrics
}

// Finish ImGui frame - actually render the UI
void DX11Renderer::completeIMGUIDraw()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// Main update loop - called every frame
void DX11Renderer::update(const float deltaTime)
{
    // FPS counter - updates once per second
    static float timer = 0;
    timer += deltaTime;
    static unsigned int frameCounter = 0;
    frameCounter++;
    static unsigned int FPS = 0;
    if (timer > 1)
    {
        timer -= 1.0f;
        FPS = frameCounter;
        frameCounter = 0;
    }

    // Start ImGui rendering
    startIMGUIDraw(FPS);

    // Clear back buffer to blue-ish color - your blank canvas
    float blueish[4] = { 0.2, 0.2, 1, 1 }; // RGBA
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), blueish);

    // Clear depth buffer to max depth (1.0)
    m_pImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Bind shaders to pipeline - tell GPU what to use
    m_pImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    m_pImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

    // Update and render the scene - where the actual magic happens
    m_pScene->update(deltaTime);

    // Finish ImGui rendering
    completeIMGUIDraw();

    // Present the back buffer - swap buffers and show the frame
    m_pSwapChain->Present(0, 0); // 0, 0 = no vsync, present immediately
}