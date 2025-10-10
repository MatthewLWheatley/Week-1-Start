#include "Scene.h"
#include "DDSTextureLoader.h"
#include <iostream>
#include "DX11Renderer.h"

// Initialization function for the scene
HRESULT Scene::init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DX11Renderer* renderer)
{
	m_pRenderer = renderer;
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
    bool ok2 = m_sceneobject2.LoadGLTF(m_ctx, L"Resources\\sphere.gltf");
	m_objects[0] = &m_sceneobject;
	m_objects[1] = &m_sceneobject2;
    m_sceneobject2.AddScaleToRoots(10.0f);
    if (!ok || !ok2)
		return E_FAIL;  // If loading fails, return an error

    // Create a camera with initial position, target, and up vector
    m_pCamera = new Camera(XMFLOAT3(0, 0, -6), XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

    // Create the constant buffer for transformation matrices (view, projection, etc.)
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBufferSwitch);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBufferSwitch);
    if (FAILED(hr))
        return hr;  // If buffer creation fails, return the error

    bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBufferlight);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pConstantBufferlight);
    if (FAILED(hr))
        return hr;  // If buffer creation fails, return the error

    // Set up light properties
    setupLightProperties();

    // Create the light constant buffer to send light data to the GPU
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(LightPropertiesConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = m_pd3dDevice->CreateBuffer(&bd, nullptr, &m_pLightConstantBuffer);
    if (FAILED(hr))
        return hr;  // If buffer creation fails, return the error

    // Load texture resources
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_diff.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_metal.dds", nullptr, &m_pTextureMetallic);
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
    light.Color = XMFLOAT4(1, 0, 0, 1);  // Set the light color to white
    light.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
    light.ConstantAttenuation = 1.0f;  // Attenuation factors
    light.LinearAttenuation = 0.0045f;
    light.QuadraticAttenuation = 0.00075f;

	Light light2;
	light2.Enabled = static_cast<int>(true);  // Enable the light
	light2.LightType = PointLight;  // Set light type to point light
	light2.Color = XMFLOAT4(0, 0, 1, 1);  // Set the light color to white
	light2.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
	light2.ConstantAttenuation = 1.0f;  // Attenuation factors
	light2.LinearAttenuation = 0.0045f;
	light2.QuadraticAttenuation = 0.00075f;

    // Set up the light position based on the camera's position
    XMFLOAT4 LightPosition(5, 5, -6, 1);
    light.Position = LightPosition;
    
    LightPosition = XMFLOAT4(-5, 5, -6, 1);
    light2.Position = LightPosition;


    // Update the light properties struct
    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    m_lightProperties.Lights[0] = light;  // Store the light in the light properties
    m_lightProperties.Lights[1] = light2;  // Store the light in the light properties
}

//void Scene::setTexture(int tId)
//{
//    // This function can be expanded to change textures based on the index
//    // textureIndex is already set by the caller, so don't overwrite it
//    std::cout << "Selected texture index: " << tId << std::endl;
//}

void Scene::setLightPos(int lightIndex, XMFLOAT4 pos)
{
    if (lightIndex >= 0 && lightIndex < MAX_LIGHTS) 
    {
		m_lightProperties.Lights[lightIndex].Position = pos;
    }
}

// Update function to update the scene's state
void Scene::update(const float deltaTime)
{
    //m_sceneobject2.SetMatrixToRoots(XMMatrixScaling(.1f, .1f, .1f));
    m_sceneobject2.SetMatrixToRoots(XMMatrixTranslation( m_lightProperties.Lights[0].Position.x*10, m_lightProperties.Lights[0].Position.y * 10, m_lightProperties.Lights[0].Position.z * 10) * XMMatrixScaling(.1f, .1f, .1f));


    // Bind texture resources to pixel shader stages
    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    m_pImmediateContext->PSSetShaderResources(1, 1, &m_pTextureMetallic);
    m_pImmediateContext->PSSetShaderResources(2, 1, &m_pTextureRoughness);
    m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    // Prepare the constant buffer with the updated view and projection matrices
    ConstantBufferSwitch cb;
    cb.mWorld = XMMatrixTranspose(XMMatrixIdentity());  // Identity world matrix
    cb.mView = XMMatrixTranspose(getCamera()->getViewMatrix());  // Transpose for HLSL compatibility
    cb.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());  // Transpose for HLSL compatibility
    cb.vOutputColor = XMFLOAT4(0, 0, 1, 1);  // Placeholder for output color

    cb.TextureSelector = textureIndex;







    m_pImmediateContext->UpdateSubresource(m_pConstantBufferSwitch.Get(), 0, nullptr, &cb, 0, 0);

    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    
    m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
    ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
    m_pImmediateContext->PSSetConstantBuffers(1, 1, &buf);


    m_sceneobject.AnimateFrame(m_ctx);
    m_sceneobject.RenderFrame(m_ctx, deltaTime);

	ConstantBufferlight cb2;
    cb2.vOutputColor2 = XMFLOAT4(0, 0, 1, 1);
    m_pImmediateContext->UpdateSubresource(m_pConstantBufferlight.Get(), 0, nullptr, &cb2, 0, 0);

    m_pImmediateContext->PSSetShader(m_pRenderer->m_pPixelSolidShader.Get(),nullptr,0);
    ID3D11Buffer* cbSwitch = m_pConstantBufferlight.Get();
    m_pImmediateContext->PSSetConstantBuffers(2, 1, &cbSwitch);
    m_sceneobject2.AnimateFrame(m_ctx);
    m_sceneobject2.RenderFrame(m_ctx, deltaTime);
}