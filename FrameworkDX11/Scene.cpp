#include "Scene.h"
#include "DDSTextureLoader.h"

// Initialization function for the scene
HRESULT Scene::init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DX11Renderer* renderer)
{
    // Store the device and context pointers for later use
    m_pd3dDevice = device;
    m_pImmediateContext = context;

    RECT rc;
    // Get the window size (client area)
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    HRESULT hr;

    // Initialize the context, renderer, and scene object
    m_ctx.Init(device.Get(), context.Get(), renderer);
    // Load a 3D model (e.g., a sphere) from a .gltf file into the scene object
    bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\sphere.gltf");

    // Create a camera with initial position, target, and up vector
    m_pCamera = new Camera(XMFLOAT3(0, 0, -6), XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

    //// Create the constant buffer for transformation matrices (view, projection, etc.)
    //D3D11_BUFFER_DESC bd = {};
    //bd.Usage = D3D11_USAGE_DEFAULT;
    //bd.ByteWidth = sizeof(ConstantBuffer);
    //bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    //bd.CPUAccessFlags = 0;
    //hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
    //if (FAILED(hr))
    //    return hr;  // If buffer creation fails, return the error

    // Create the constant buffer for transformation matrices (view, projection, etc.)
    D3D11_BUFFER_DESC bd2 = {};
    bd2.Usage = D3D11_USAGE_DEFAULT;
    bd2.ByteWidth = sizeof(ConstantBuffer2);
    bd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd2.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd2, nullptr, &m_pConstantBuffer2);
    if (FAILED(hr))
        return hr;  // If buffer creation fails, return the error

    // Set up light properties
    setupLightProperties();

    // Create the light constant buffer to send light data to the GPU
    bd2.Usage = D3D11_USAGE_DEFAULT;
    bd2.ByteWidth = sizeof(LightPropertiesConstantBuffer);
    bd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd2.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd2, nullptr, &m_pLightConstantBuffer);
    if (FAILED(hr))
        return hr;  // If buffer creation fails, return the error

    // Load texture resources
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\PavingStones_Color.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\PavingStones_NormalDX.dds", nullptr, &m_pTextureNormal);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\PavingStones_Color.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_rough.dds", nullptr, &m_pTextureRoughness);
    if (FAILED(hr))
        return hr;

    // Set up a sampler state for texture sampling (anisotropic filtering)
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

    return S_OK;  // Return success
}

// Cleanup function, deletes the camera
void Scene::cleanUp()
{
    delete m_pCamera;
}

// Function to set up lighting properties
void Scene::setupLightProperties()
{
    Light light;
    light.Enabled = static_cast<int>(true);  // Enable the light
    light.LightType = PointLight;  // Set light type to point light
    light.Color = XMFLOAT4(1, 1, 1, 1);  // Set the light color to white
    light.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
    light.ConstantAttenuation = 1.0f;  // Attenuation factors
    light.LinearAttenuation = 1;
    light.QuadraticAttenuation = 1;

    // Set up the light position based on the camera's position
    XMFLOAT4 LightPosition(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    light.Position = LightPosition;

    // Update the light properties struct
    m_lightProperties.EyePosition = LightPosition;
    m_lightProperties.Lights[0] = light;  // Store the light in the light properties
}

// Update function to update the scene's state
void Scene::update(const float deltaTime)
{
    // Bind texture resources to pixel shader stages
    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    if (m_pTextureNormal != nullptr) {
        m_pImmediateContext->PSSetShaderResources(1, 1, &m_pTextureNormal);
    }
    m_pImmediateContext->PSSetShaderResources(2, 1, &m_pTextureMetallic);
    m_pImmediateContext->PSSetShaderResources(3, 1, &m_pTextureRoughness);
    m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    // Prepare the constant buffer with the updated view and projection matrices
    //ConstantBuffer cb1;
    //cb1.mView = XMMatrixTranspose(getCamera()->getViewMatrix());  // Transpose for HLSL compatibility
    //cb1.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());  // Transpose for HLSL compatibility
    //cb1.vOutputColor = XMFLOAT4(0, 0, 0, 0);  // Placeholder for output color

    ConstantBuffer2 cb2;
    cb2.mView = XMMatrixTranspose(getCamera()->getViewMatrix());  // Transpose for HLSL compatibility
    cb2.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());  // Transpose for HLSL compatibility
    cb2.vOutputColor = XMFLOAT4(0, 0, 0, 0);  // Placeholder for output color
	cb2.textureSelect = 1;  // Placeholder for texture selection
	time += deltaTime;
    if (time > 1.0f)
    {   
		time = 0.0f;
        cb2.textureSelect += 1.0f;
        if (cb2.textureSelect > 3.0f)
			cb2.textureSelect = 0.0f;
    }

    // Update the light position based on the camera's current position
    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);

	m_pImmediateContext->UpdateSubresource(m_pConstantBuffer2.Get(), 0, nullptr, &cb2, 0, 0);

    // Update the light constant buffer on the GPU
    m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
    ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
    m_pImmediateContext->PSSetConstantBuffers(1, 1, &buf);

    // Animate and render the scene object
    m_sceneobject.AnimateFrame(m_ctx);  // Update the object's animation
    m_sceneobject.RenderFrame(m_ctx, deltaTime);  // Render the animated object to the screen
}