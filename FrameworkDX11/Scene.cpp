#include "Scene.h"
#include "DDSTextureLoader.h"
#include <iostream>
#include "DX11Renderer.h"
#include <algorithm>

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
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete_Albedo.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr)) m_pTextureDiffuse = nullptr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete_Metallic.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr)) m_pTextureMetallic = nullptr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete_Roughness.dds", nullptr, &m_pTextureRoughness);
    if (FAILED(hr))m_pTextureRoughness = nullptr;
    /*
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_albedo.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_metallic.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_rough.dds", nullptr, &m_pTextureRoughness);*/
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\SpecularCM.dds", nullptr, &m_pTextureSpecularIBL);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\DiffuseCM.dds", nullptr, &m_pTextureDiffuseIBL);
    if (FAILED(hr))
        return hr;


    // place this where other textures are sent to the pixel shader

    m_pImmediateContext->PSSetShaderResources(5, 1, &m_pTextureSpecularIBL);

    m_pImmediateContext->PSSetShaderResources(6, 1, &m_pTextureDiffuseIBL);

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

void Scene::initAnimation1() 
{
    for (auto& object : m_objects) 
    {
		object = nullptr;
    }
	m_objects = vector<SceneGraph*>(100);
    bool ok = m_sceneobject.LoadGLTF(m_ctx, L"Resources\\box.gltf");
    bool ok2 = m_sceneobject2.LoadGLTF(m_ctx, L"Resources\\sphere.gltf");
    //bool ok3 = m_sceneobject3.LoadGLTF(m_ctx, L"Resources\\box.gltf");
    m_objects[0] = &m_sceneobject;
    m_objects[1] = &m_sceneobject2;
    m_sceneobject2.AddScaleToRoots(-.75);


	// ---- animation 1 ----
    AnimationSampler sampler0;
    sampler0.interpolation = AnimationSampler::LINEAR;

    sampler0.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };

    sampler0.vec3_values = {
        XMFLOAT3(-3.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(3.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(-3.0f, 0.0f, 0.0f)
    };
    m_myAnimation1.m_samplers.push_back(sampler0);

    AnimationSampler sampler1;
    sampler1.interpolation = AnimationSampler::LINEAR;

    sampler1.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };

    sampler1.vec4_values = {
        XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
        XMFLOAT4(0.0f, 0.7071f, 0.0f, 0.7071f),
        XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f),
        XMFLOAT4(0.0f, 0.7071f, 0.0f, -0.7071f),
        XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
    };

    m_myAnimation1.m_samplers.push_back(sampler1);

    AnimationSampler sampler2;
    sampler2.interpolation = AnimationSampler::LINEAR;

    float orbitRadius = 2.0f;
    int numKeyframes = 8;
    float animDuration = 4.0f;

    for (int i = 0; i <= numKeyframes; ++i)
    {
        float t = (float)i / (float)numKeyframes;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;

        sampler2.timestamps.push_back(timeStamp);
        sampler2.vec3_values.push_back(XMFLOAT3(
            cos(angle) * orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        ));
    }

    m_myAnimation1.m_samplers.push_back(sampler2);

    m_myAnimation1.m_samplers.push_back(sampler2);

    m_animations.push_back(&m_myAnimation1);
    m_animationTimers.push_back(0.0f);

}

void Scene::initAnimation2()
{
    for (auto& object : m_objects)
    {
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);

	m_sceneobject




    AnimationSampler sampler3;
    sampler3.interpolation = AnimationSampler::LINEAR;

    sampler3.timestamps = { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f };

    sampler3.vec3_values = {
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 1.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, -1.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(-1.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f)
    };
    m_myAnimation2.m_samplers.push_back(sampler3);


    m_animations.push_back(&m_myAnimation2);
    m_animationTimers.push_back(0.0f);
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
    light.LinearAttenuation = 0.0045f;
    light.QuadraticAttenuation = 0.00075f;

	Light light2;
	light2.Enabled = static_cast<int>(true);  // Enable the light
	light2.LightType = PointLight;  // Set light type to point light
	light2.Color = XMFLOAT4(1, 1, 1, 1);  // Set the light color to white
	light2.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
	light2.ConstantAttenuation = 1.0f;  // Attenuation factors
	light2.LinearAttenuation = 0.0045f;
	light2.QuadraticAttenuation = 0.00075f;

    // Set up the light position based on the camera's position
    XMFLOAT4 LightPosition(5, 5, -6, 1);
    light.Position = LightPosition;
    
    LightPosition = XMFLOAT4(-5, 5, -6, 1);
    light2.Position = LightPosition;
    for (int x = 0; x < MAX_LIGHTS; x++)
    {
        m_lightProperties.Lights[x] = light2;
        m_lightProperties.Lights[x].Enabled = false;
    }

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

void Scene::animation1(const float deltaTime)
{
    
    /*
    if (m_animationTimers[0] >= sampler1.timestamps.back())
        m_animationTimers[0] = 0;if(m_animationPlaying) m_animationTimers[0] += deltaTime;

    AnimationSampler sampler1 = m_animations[0]->m_samplers[0];
    AnimationSampler sampler1Rot = m_animations[0]->m_samplers[1];

    int nextKeyframe1 = -1;
    for (int i = 0; i < sampler1.timestamps.size(); ++i)
    {
        if (sampler1.timestamps[i] > m_animationTimers[0])
        {
            nextKeyframe1 = i;
            break;
        }
    }

    if (nextKeyframe1 == -1 || nextKeyframe1 == 0)
        nextKeyframe1 = 1;

    int prevKeyframe1 = nextKeyframe1 - 1;
    float prevTime1 = sampler1.timestamps[prevKeyframe1];
    float nextTime1 = sampler1.timestamps[nextKeyframe1];
    float t1 = (m_animationTimers[0] - prevTime1) / (nextTime1 - prevTime1);

    DirectX::XMVECTOR prevPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[prevKeyframe1]);
    DirectX::XMVECTOR nextPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[nextKeyframe1]);
    DirectX::XMVECTOR finalPos1 = DirectX::XMVectorLerp(prevPos1, nextPos1, t1);

    DirectX::XMVECTOR prevRot1 = DirectX::XMLoadFloat4(&sampler1Rot.vec4_values[prevKeyframe1]);
    DirectX::XMVECTOR nextRot1 = DirectX::XMLoadFloat4(&sampler1Rot.vec4_values[nextKeyframe1]);
    DirectX::XMVECTOR finalRot1 = DirectX::XMQuaternionSlerp(prevRot1, nextRot1, t1);

    DirectX::XMMATRIX object1Rotation = DirectX::XMMatrixRotationQuaternion(finalRot1);
    DirectX::XMMATRIX object1Translation = DirectX::XMMatrixTranslationFromVector(finalPos1);

    XMMATRIX object1Scale = XMMatrixScaling(0.5f, 0.5f, 0.5f);

    DirectX::XMMATRIX object1Transform = object1Scale * object1Rotation * object1Translation;

    m_sceneobject.GetRootNode(0)->SetMatrix(object1Transform);

    AnimationSampler sampler2 = m_animations[0]->m_samplers[2];

    int nextKeyframe2 = -1;
    for (int i = 0; i < sampler2.timestamps.size(); ++i)
    {
        if (sampler2.timestamps[i] > m_animationTimers[0])
        {
            nextKeyframe2 = i;
            break;
        }
    }

    if (nextKeyframe2 == -1 || nextKeyframe2 == 0)
        nextKeyframe2 = 1;

    int prevKeyframe2 = nextKeyframe2 - 1;
    float prevTime2 = sampler2.timestamps[prevKeyframe2];
    float nextTime2 = sampler2.timestamps[nextKeyframe2];
    float t2 = (m_animationTimers[0] - prevTime2) / (nextTime2 - prevTime2);

    XMVECTOR prevOffset = XMLoadFloat3(&sampler2.vec3_values[prevKeyframe2]);
    XMVECTOR nextOffset = XMLoadFloat3(&sampler2.vec3_values[nextKeyframe2]);
    XMVECTOR orbitOffset = XMVectorLerp(prevOffset, nextOffset, t2);

    XMVECTOR object2FinalPos = DirectX::XMVectorAdd(finalPos1, orbitOffset);

    XMMATRIX object2Translation = DirectX::XMMatrixTranslationFromVector(object2FinalPos);

    XMMATRIX object2Scale = XMMatrixScaling(0.2f, 0.2f, 0.2f);

    m_sceneobject2.GetRootNode(0)->SetMatrix(object2Scale * object2Translation);
    if (m_animationTimers[0] < -0.01)
        m_animationTimers[0] = sampler1.timestamps.back();*/


}

void Scene::animation2(const float deltaTime)
{
	/*float* timer = &m_animationTimers[1];
    if (m_animationPlaying) *timer += deltaTime;


    AnimationSampler sampler1 = m_animations[1]->m_samplers[0];

    int nextKeyframe1 = -1;
    for (int i = 0; i < sampler1.timestamps.size(); ++i)
    {
        if (sampler1.timestamps[i] > m_animationTimers[1])
        {
            nextKeyframe1 = i;
            break;
        }
    }

    if (nextKeyframe1 == -1 || nextKeyframe1 == 0)
        nextKeyframe1 = 1;


    int prevKeyframe1 = nextKeyframe1 - 1;
    float prevTime1 = sampler1.timestamps[prevKeyframe1];
    float nextTime1 = sampler1.timestamps[nextKeyframe1];
    float t1 = (m_animationTimers[1] - prevTime1) / (nextTime1 - prevTime1);


    DirectX::XMVECTOR prevPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[prevKeyframe1]);
    DirectX::XMVECTOR nextPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[nextKeyframe1]);
    DirectX::XMVECTOR finalPos1 = DirectX::XMVectorLerp(prevPos1, nextPos1, t1);


    DirectX::XMMATRIX object1Translation = DirectX::XMMatrixTranslationFromVector(finalPos1);
	DirectX::XMMATRIX object1Transform = object1Translation;

    m_sceneobject.GetRootNode(0)->SetMatrix(object1Transform);

    if (m_animationTimers[1] >= sampler1.timestamps.back())
        m_animationTimers[1] = 0;
    if (m_animationTimers[1] < -0.01)
        m_animationTimers[1] = sampler1.timestamps.back();*/
}

// Update function to update the scene's state
void Scene::update(const float deltaTime)
{
    switch (m_animationSelected)
    {
        case 1:
            animation1(deltaTime);
			break;
        case 2:
            animation2(deltaTime);
            break;
    }



	//---------------rendering part---------------

    ID3D11ShaderResourceView* nullSRV = nullptr;

    // Bind texture resources to pixel shader stages
    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    m_pImmediateContext->PSSetShaderResources(1, 1, &m_pTextureMetallic);
    m_pImmediateContext->PSSetShaderResources(2, 1, &m_pTextureRoughness);
    m_pImmediateContext->PSSetShaderResources(3, 1, &m_pTextureDiffuseIBL);
    m_pImmediateContext->PSSetShaderResources(4, 1, &m_pTextureSpecularIBL);

    m_pImmediateContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    // Prepare the constant buffer with the updated view and projection matrices
    ConstantBufferSwitch cb;
    cb.mWorld = XMMatrixTranspose(XMMatrixIdentity());  // Identity world matrix
    cb.mView = XMMatrixTranspose(getCamera()->getViewMatrix());  // Transpose for HLSL compatibility
    cb.mProjection = XMMatrixTranspose(getCamera()->getProjectionMatrix());  // Transpose for HLSL compatibility
    cb.vOutputColor = XMFLOAT4(0, 0, 1, 1);  // Placeholder for output color

	cb.frank = XMFLOAT4(albedo.x, albedo.y, albedo.z, 1.0f);
    cb.metal = metal;
    cb.rough = rough;
    cb.type = type;
    cb.textureSelect = textureSelect;


    m_pImmediateContext->UpdateSubresource(m_pConstantBufferSwitch.Get(), 0, nullptr, &cb, 0, 0);

    m_lightProperties.EyePosition = XMFLOAT4(m_pCamera->getPosition().x, m_pCamera->getPosition().y, m_pCamera->getPosition().z, 1);
    
    m_pImmediateContext->UpdateSubresource(m_pLightConstantBuffer.Get(), 0, nullptr, &m_lightProperties, 0, 0);
    ID3D11Buffer* buf = m_pLightConstantBuffer.Get();
    m_pImmediateContext->PSSetConstantBuffers(1, 1, &buf);


    m_sceneobject.AnimateFrame(m_ctx);
    m_sceneobject.RenderFrame(m_ctx, deltaTime);

	m_sceneobject2.AnimateFrame(m_ctx);
	m_sceneobject2.RenderFrame(m_ctx, deltaTime);

	ConstantBufferlight cb2;
    cb2.vOutputColor2 = XMFLOAT4(0, 0, 1, 1);
    m_pImmediateContext->UpdateSubresource(m_pConstantBufferlight.Get(), 0, nullptr, &cb2, 0, 0);

    m_pImmediateContext->PSSetShader(m_pRenderer->m_pPixelSolidShader.Get(),nullptr,0);
    ID3D11Buffer* cbSwitch = m_pConstantBufferlight.Get();
    m_pImmediateContext->PSSetConstantBuffers(2, 1, &cbSwitch);
}