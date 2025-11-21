#include "Scene.h"
#include "DDSTextureLoader.h"
#include <iostream>
#include "DX11Renderer.h"
#include <algorithm>

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
    m_pCamera = new Camera(XMFLOAT3(0, 0, -10), XMFLOAT3(0, 0, 1), XMFLOAT3(0.0f, 1.0f, 0.0f), width, height);

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

    ////// Load texture resources
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_diff.dds", nullptr, &m_pTextureDiffuse);
    //if (FAILED(hr)) m_pTextureDiffuse = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_metal.dds", nullptr, &m_pTextureMetallic);
    //if (FAILED(hr)) m_pTextureMetallic = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_rough.dds", nullptr, &m_pTextureRoughness);
    //if (FAILED(hr))m_pTextureRoughness = nullptr; 
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
    //if (FAILED(hr)) m_pTextureDiffuse = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
    //if (FAILED(hr)) m_pTextureMetallic = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
    //if (FAILED(hr))m_pTextureRoughness = nullptr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr)) return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr)) return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
    if (FAILED(hr)) return hr;/*
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
    //if (FAILED(hr)) m_pTextureDiffuse = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
    //if (FAILED(hr)) m_pTextureMetallic = nullptr;
    //hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
    //if (FAILED(hr))m_pTextureRoughness = nullptr;*/

    /*hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\scratched-metal_albedo.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr)) m_pTextureDiffuse = nullptr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\scratched-metal_metallic.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr)) m_pTextureMetallic = nullptr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\scratched-metal_roughness.dds", nullptr, &m_pTextureRoughness);
    if (FAILED(hr))m_pTextureRoughness = nullptr;*/
    /*
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_albedo.dds", nullptr, &m_pTextureDiffuse);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_metallic.dds", nullptr, &m_pTextureMetallic);
    if (FAILED(hr))
        return hr;
    hr = CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\space_rough.dds", nullptr, &m_pTextureRoughness);*//*
    if (FAILED(hr))
        return hr;*/
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

void Scene::SwapTextures(int id) 
{
    switch (id)
    {
    case 1:
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Concrete044D_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
        break;
    case 2:
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_diff.dds", nullptr, &m_pTextureDiffuse);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_metal.dds", nullptr, &m_pTextureMetallic);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\rusty_metal_04_rough.dds", nullptr, &m_pTextureRoughness);
        break;
    case 3:
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal053C_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
        break;
    case 4:
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\Metal054C_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
        break;
    case 5:
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\DiamondPlate008A_2K-PNG_Color.dds", nullptr, &m_pTextureDiffuse);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\DiamondPlate008A_2K-PNG_Metalness.dds", nullptr, &m_pTextureMetallic);
        CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Resources\\DiamondPlate008A_2K-PNG_Roughness.dds", nullptr, &m_pTextureRoughness);
    
    default:
    
        break;
    }
}

void Scene::initPBRScene() 
{
    for (SceneGraph* object : m_objects)
    {
        if (object) object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\FlightHelmet.gltf");
    m_sceneobject2.LoadGLTFWithSkeleton(m_ctx, L"Resources\\Sphere.gltf");
    m_sceneobject3.LoadGLTFWithSkeleton(m_ctx, L"Resources\\Sphere.gltf");
    m_sceneobject.SetMatrixToRoots(XMMatrixScaling(5, 5, 5));
    m_sceneobject2.SetMatrixToRoots(XMMatrixIdentity());
    m_sceneobject3.SetMatrixToRoots(XMMatrixIdentity());
    m_objects[0] = &m_sceneobject;
    m_objects[1] = &m_sceneobject2;
    m_objects[2] = &m_sceneobject3;

    XMFLOAT4 pos = { 3,0,0,1 };
    XMMATRIX lightPos = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX lightScale = XMMatrixScaling(0.2, 0.2, 0.2);
    XMMATRIX out = lightScale * lightPos;
    m_sceneobject2.SetMatrixToRoots(out);

    XMFLOAT4 pos2 = m_lightProperties.Lights[1].Position;
    XMMATRIX lightPos2 = XMMatrixTranslation(pos2.x, pos2.y, pos2.z);
    XMMATRIX lightScale2 = XMMatrixScaling(0.2, 0.2, 0.2);
    XMMATRIX out2 = lightScale2 * lightPos2;
    m_sceneobject3.SetMatrixToRoots(out);
}

void Scene::initAnimation1() 
{
    m_sceneobject.Destroy();
    for (SceneGraph* object : m_objects) 
    {
        if(object)object->Destroy();
		object = nullptr;
    }
	m_objects = vector<SceneGraph*>(100);
    if (m_animations[0])
    {
        m_animations[0]->m_channels.clear();
        m_animations[0]->m_name.clear();
        m_animations[0]->m_samplers.clear();
    }
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

    m_animations[0] = &m_myAnimation1;
    m_animationTimers[0] = 0.0f;

}

void Scene::initAnimation2()
{
    for (SceneGraph* object : m_objects)
    {
        if (object)object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    if (m_animations[1]) 
    {
        m_animations[1]->m_channels.clear();
        m_animations[1]->m_name.clear();
        m_animations[1]->m_samplers.clear();
    }
    SceneNode* bodyNode = m_sceneobject.CreateRootNode();
    bodyNode->LoadSphere(m_ctx);
    bodyNode->AddTranslation({ 0.0, 0.0, 0.0 });
    bodyNode->AddScale(-.75f);
    SceneNode* headNode = bodyNode->CreateChildNode();
    headNode->LoadSphere(m_ctx);
    headNode->AddTranslation({ 0.0,3.0,0.0 });
    SceneNode* twoNode = headNode->CreateChildNode();
    twoNode->LoadSphere(m_ctx);
    twoNode->AddTranslation({ 0.0,3.0,0.0 });
    SceneNode* threeNode = twoNode->CreateChildNode();
    threeNode->LoadSphere(m_ctx);
    threeNode->AddTranslation({ 0.0,3.0,0.0 });


    m_objects[0] = &m_sceneobject;

    AnimationSampler sampler0;
    sampler0.interpolation = AnimationSampler::LINEAR;

    sampler0.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f};

    sampler0.vec3_values = {
        XMFLOAT3(-2.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(-2.0f, 0.0f, 0.0f)
    };
    m_myAnimation2.m_samplers.push_back(sampler0);

    AnimationSampler sampler1;
    sampler1.interpolation = AnimationSampler::LINEAR;

    sampler1.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };

    sampler1.vec4_values =
    {
        XMFLOAT4(0.0f, 0.0f,-0.1305262f,  0.9914449f),
        XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
        XMFLOAT4(0.0f, 0.0f, 0.1305262f,  0.9914449f),
        XMFLOAT4(0.0f, 0.0f, 0.0f,  1.0f),
        XMFLOAT4(0.0f, 0.0f,-0.1305262f, 0.9914449f),
    };
    m_myAnimation2.m_samplers.push_back(sampler1);



    m_animations[1] = &m_myAnimation2;
    m_animationTimers[1] = 0.0f;
}

void Scene::initAnimation3()
{
    m_anim3Initialized = true;
    m_anim3Skeleton = Skeleton();
    m_anim3SceneNodes.clear();
    m_sceneobject.Destroy();
    for (SceneGraph* object : m_objects)
    {
        if (object)object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);

    m_myAnimation3.m_channels.clear();
    m_myAnimation3.m_name.clear();
    m_myAnimation3.m_samplers.clear();

    Skeleton anim3Skeleton;

    XMFLOAT4X4 bodyTransform;
    XMStoreFloat4x4(&bodyTransform, XMMatrixIdentity());
    int bodyIndex = anim3Skeleton.AddJoint(-1, bodyTransform);

    XMFLOAT4X4 headTransform;
    XMStoreFloat4x4(&headTransform, XMMatrixTranslation(0.0f, 3.0f, 0.0f));
    int headIndex = anim3Skeleton.AddJoint(bodyIndex, headTransform);

    m_anim3Skeleton = anim3Skeleton;

    for (int i = 0; i < m_anim3Skeleton.GetBoneCount(); ++i) {
        m_sceneobject.CreateRootNode();
    }

    m_objects[0] = &m_sceneobject;

    m_anim3SceneNodes.clear();
    for (int i = 0; i < m_anim3Skeleton.GetBoneCount(); ++i) {
        SceneNode* node = m_sceneobject.GetRootNode(i);
        node->LoadSphere(m_ctx);
        node->SetMatrix(XMMatrixTranslation(0.0f, 2.0f * i, 0.0f));
        m_anim3SceneNodes.push_back(node);
    }

    AnimationSampler sampler0;
    sampler0.interpolation = AnimationSampler::LINEAR;
    sampler0.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler0.vec3_values = {
        XMFLOAT3(-2.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(2.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(-2.0f, 2.0f, 0.0f)
    };
    m_myAnimation3.m_samplers.push_back(sampler0);

    AnimationSampler sampler1;
    sampler1.interpolation = AnimationSampler::LINEAR;
    sampler1.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler1.vec4_values = {
        XMFLOAT4(0.0f, 0.0f, 1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, 1),
        XMFLOAT4(0.0f, 0.0f, -1, 0),  
        XMFLOAT4(0.0f, 0.0f, 0, 1),
        XMFLOAT4(0.0f, 0.0f, 1, 0),
    };
    m_myAnimation3.m_samplers.push_back(sampler1);

    AnimationChannel rootTrans;
    rootTrans.jointIndex = 0;
    rootTrans.samplerIndex = 0;
    rootTrans.path = AnimationChannel::TRANSLATION;
    m_myAnimation3.m_channels.push_back(rootTrans);

    AnimationChannel rootRot;
    rootRot.jointIndex = 0;
    rootRot.samplerIndex = 1;
    rootRot.path = AnimationChannel::ROTATION;
    m_myAnimation3.m_channels.push_back(rootRot);

    AnimationSampler sampler2;
    sampler2.interpolation = AnimationSampler::LINEAR;
    sampler2.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler2.vec3_values = {
        XMFLOAT3(0.0f, 4.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 4.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 4.0f, 0.0f)
    };
    m_myAnimation3.m_samplers.push_back(sampler2);

    AnimationSampler sampler3;
    sampler3.interpolation = AnimationSampler::LINEAR;
    sampler3.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler3.vec4_values = {
        XMFLOAT4(0.0f, 0.0f, 0, 1),
        XMFLOAT4(0.0f, 0.0f, -1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, -1),
        XMFLOAT4(0.0f, 0.0f, -1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, 1),
    };
    m_myAnimation3.m_samplers.push_back(sampler3);

    AnimationChannel headTrans;
    headTrans.jointIndex = 1;
    headTrans.samplerIndex = 2;
    headTrans.path = AnimationChannel::TRANSLATION;
    m_myAnimation3.m_channels.push_back(headTrans);

    AnimationChannel headRot;
    headRot.jointIndex = 1;
    headRot.samplerIndex = 3;
    headRot.path = AnimationChannel::ROTATION;
    m_myAnimation3.m_channels.push_back(headRot);

    m_animations[2] = &m_myAnimation3;
    m_animationTimers[2] = 0.0f;
    initAnimation3_5();
}

void Scene::initAnimation3_5()
{
    m_anim3_5Skeleton = Skeleton();
    m_anim3_5SceneNodes.clear();
    m_sceneobject2.Destroy();

    m_myAnimation3_5.m_channels.clear();
    m_myAnimation3_5.m_name.clear();
    m_myAnimation3_5.m_samplers.clear();

    XMFLOAT4X4 bodyTransform;
    XMStoreFloat4x4(&bodyTransform, XMMatrixIdentity());
    int bodyIndex = m_anim3_5Skeleton.AddJoint(-1, bodyTransform);

    XMFLOAT4X4 headTransform;
    XMStoreFloat4x4(&headTransform, XMMatrixTranslation(0.0f, 3.0f, 0.0f));
    int headIndex = m_anim3_5Skeleton.AddJoint(bodyIndex, headTransform);

    for (int i = 0; i < m_anim3_5Skeleton.GetBoneCount(); ++i) {
        m_sceneobject2.CreateRootNode();
    }

    m_objects[1] = &m_sceneobject2;

    m_anim3_5SceneNodes.clear();
    for (int i = 0; i < m_anim3_5Skeleton.GetBoneCount(); ++i) {
        SceneNode* node = m_sceneobject2.GetRootNode(i);
        node->LoadSphere(m_ctx);
        m_anim3_5SceneNodes.push_back(node);
    }

    AnimationSampler sampler0;
    sampler0.interpolation = AnimationSampler::LINEAR;
    sampler0.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler0.vec3_values = {
        XMFLOAT3(2.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(-2.0f, 0.0f, 0.0f),
        XMFLOAT3(0.0f, 0.0f, 0.0f),
        XMFLOAT3(2.0f, 0.0f, 0.0f)
    };
    m_myAnimation3_5.m_samplers.push_back(sampler0);

    AnimationSampler sampler1;
    sampler1.interpolation = AnimationSampler::LINEAR;
    sampler1.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler1.vec4_values = {
        XMFLOAT4(0.0f, 0.0f, 0, 1),
        XMFLOAT4(0.0f, 0.0f, -1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, -1),
        XMFLOAT4(0.0f, 0.0f, -1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, 1),
    };
    m_myAnimation3_5.m_samplers.push_back(sampler1);

    AnimationChannel rootTrans;
    rootTrans.jointIndex = 0;
    rootTrans.samplerIndex = 0;
    rootTrans.path = AnimationChannel::TRANSLATION;
    m_myAnimation3_5.m_channels.push_back(rootTrans);

    AnimationChannel rootRot;
    rootRot.jointIndex = 0;
    rootRot.samplerIndex = 1;
    rootRot.path = AnimationChannel::ROTATION;
    m_myAnimation3_5.m_channels.push_back(rootRot);

    AnimationSampler sampler2;
    sampler2.interpolation = AnimationSampler::LINEAR;
    sampler2.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler2.vec3_values = {
        XMFLOAT3(0.0f, 4.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 4.0f, 0.0f),
        XMFLOAT3(0.0f, 2.0f, 0.0f),
        XMFLOAT3(0.0f, 4.0f, 0.0f)
    };
    m_myAnimation3_5.m_samplers.push_back(sampler2);

    AnimationSampler sampler3;
    sampler3.interpolation = AnimationSampler::LINEAR;
    sampler3.timestamps = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    sampler3.vec4_values = {
        XMFLOAT4(0.0f, 0.0f, 0, -1),
        XMFLOAT4(0.0f, 0.0f, 1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, 1),
        XMFLOAT4(0.0f, 0.0f, 1, 0),
        XMFLOAT4(0.0f, 0.0f, 0, -1),
    };
    m_myAnimation3_5.m_samplers.push_back(sampler3);

    AnimationChannel headTrans;
    headTrans.jointIndex = 1;
    headTrans.samplerIndex = 2;
    headTrans.path = AnimationChannel::TRANSLATION;
    m_myAnimation3_5.m_channels.push_back(headTrans);

    AnimationChannel headRot;
    headRot.jointIndex = 1;
    headRot.samplerIndex = 3;
    headRot.path = AnimationChannel::ROTATION;
    m_myAnimation3_5.m_channels.push_back(headRot);
}

void Scene::initAnimation4() 
{
    m_sceneobject.Destroy();
    m_armSegmentNodes.clear();
    m_robotArmSkeleton = Skeleton();
    m_robotArmAnimations.clear();
    doOnce = true;
    for (SceneGraph* object : m_objects)
    {
        if (object)object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    if (m_animations[3])
    {
        m_animations[3]->m_channels.clear();
        m_animations[3]->m_name.clear();
        m_animations[3]->m_samplers.clear();
    }
    const float segmentLength = 2.0f;

    DirectX::XMFLOAT4X4 shoulderTransform;

    DirectX::XMStoreFloat4x4(&shoulderTransform, DirectX::XMMatrixIdentity());
    int shoulderIndex = m_robotArmSkeleton.AddJoint(-1, shoulderTransform);

    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.75, 0.75, 0.75);

    DirectX::XMFLOAT4X4 elbowTransform;
    DirectX::XMStoreFloat4x4(&elbowTransform, scale * DirectX::XMMatrixTranslation(0.0f, segmentLength, 0.0f));
    int elbowIndex = m_robotArmSkeleton.AddJoint(shoulderIndex, elbowTransform);

    DirectX::XMFLOAT4X4 handTransform;
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, segmentLength, 0.0f);

    DirectX::XMStoreFloat4x4(&handTransform, scale * translation);
    int handIndex = m_robotArmSkeleton.AddJoint(elbowIndex, handTransform);
 
    for (int i = 0; i < m_robotArmSkeleton.GetBoneCount(); ++i) {
        m_sceneobject.CreateRootNode();
    }
    m_objects[0] = &m_sceneobject;

    for (int i = 0; i < m_robotArmSkeleton.GetBoneCount(); ++i) {
        SceneNode* segmentNode = m_sceneobject.GetRootNode(i);

        segmentNode->LoadSphere(m_ctx);

        m_armSegmentNodes.push_back(segmentNode);
    }

    Animation m_myAnimation4;
    m_myAnimation4.m_name = "new Wave";
    m_robotArmAnimations.clear();

    for (int i = 0; i < 3; i++) {
        
        Animation temp;
        CreateWaveAnimationSampler1(i, &m_myAnimation4, &m_robotArmSkeleton);
    }
    m_robotArmAnimations.push_back(m_myAnimation4);
}

void Scene::initAnimation5() 
{
    for (SceneGraph* object : m_objects)
    {
        if (object) object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    if (m_animations[0])
    {
        m_animations[0]->m_channels.clear();
        m_animations[0]->m_name.clear();
        m_animations[0]->m_samplers.clear();
    }
    HRESULT hr;
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\simplerig.gltf");
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\fox.gltf");
    m_sceneobject.mRootNodes[0].SetMatrix(XMMatrixIdentity());
    m_sceneobject.mRootNodes[0].AddMatrix(XMMatrixRotationX(XMConvertToRadians(-90)));
    m_sceneobject.mRootNodes[1].SetMatrix(XMMatrixIdentity());
    m_sceneobject.mRootNodes[1].AddMatrix(XMMatrixRotationY(XMConvertToRadians(180)));
    m_sceneobject.mRootNodes[0].AddTranslation({ 2, 0, 0 });
    m_sceneobject.mRootNodes[1].AddTranslation({ -2, 0, 0 });

    m_objects[0] = &m_sceneobject;

    Animation m_myAnimation4;
    m_myAnimation4.m_name = "new Wave";

    
    CreateWaveAnimationSampler2(1, &m_myAnimation4, m_sceneobject.mRootNodes[0].GetSkeleton());
    

    m_sceneobject.mRootNodes[0].GetSkeleton()->AddAnimation(&m_myAnimation4);
}

void Scene::initAnimation6() 
{
    for (SceneGraph* object : m_objects)
    {
        if (object) object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    if (m_animations[5])
    {
        m_animations[5]->m_channels.clear();
        m_animations[5]->m_name.clear();
        m_animations[5]->m_samplers.clear();
    }
    HRESULT hr;
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\fox.gltf");
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\fox.gltf");
    m_sceneobject.mRootNodes[0].SetMatrix(XMMatrixIdentity());
    m_sceneobject.mRootNodes[0].AddMatrix(XMMatrixRotationY(XMConvertToRadians(225)));
    m_sceneobject.mRootNodes[0].AddTranslation({ 0, -2, -5 });
    m_sceneobject.mRootNodes[1].SetMatrix(XMMatrixIdentity());
    m_sceneobject.mRootNodes[1].SetMatrix(XMMatrixIdentity() * XMMatrixScaling(.25f, .25f, .25f));
    m_sceneobject.mRootNodes[1].AddMatrix(XMMatrixRotationY(XMConvertToRadians(225)));
    m_sceneobject.mRootNodes[1].AddTranslation({ 0, -2, -5 });


    m_objects[0] = &m_sceneobject;

    AnimationSampler sampler1;
    AnimationSampler sampler2;
    sampler1.interpolation = AnimationSampler::LINEAR;
    sampler2.interpolation = AnimationSampler::LINEAR;

    float orbitRadius = 3.5f;
    int numKeyframes = 64;
    float animDuration = 4.0f;

    for (int i = 0; i <= numKeyframes; ++i)
    {
        float t = (float)i / (float)numKeyframes;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;
        
        XMVECTOR rot;
        rot = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians(-(360/(float)numKeyframes)*(i)),0);
        XMFLOAT4 rot4;
        XMStoreFloat4(&rot4, rot);
        sampler2.timestamps.push_back(timeStamp);
        sampler2.vec4_values.push_back(rot4);
        
        sampler1.timestamps.push_back(timeStamp);
        sampler1.vec3_values.push_back(XMFLOAT3(
            cos(angle) * orbitRadius - orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        )
        );
    }
    for (int i = 0; i <= numKeyframes; ++i)
    {
        float t = (float)i / (float)numKeyframes;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;

        XMVECTOR rot;
        rot = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians((360 / (float)numKeyframes) * (i)), 0);
        XMFLOAT4 rot4;
        XMStoreFloat4(&rot4, rot);
        sampler2.timestamps.push_back(timeStamp + animDuration);
        sampler2.vec4_values.push_back(rot4);

        sampler1.timestamps.push_back(timeStamp + animDuration);
        sampler1.vec3_values.push_back(XMFLOAT3(
            -cos(angle) * orbitRadius + orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        ));
    }


    m_myAnimation6.m_samplers.push_back(sampler1);

    m_myAnimation6.m_samplers.push_back(sampler2);

    AnimationSampler sampler3;
    AnimationSampler sampler4;
    sampler3.interpolation = AnimationSampler::LINEAR;
    sampler4.interpolation = AnimationSampler::LINEAR;


    for (int i = numKeyframes/2; i <= numKeyframes; ++i)
    {
        float t = (float)i / (float)numKeyframes - 2;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;

        XMVECTOR rot;
        rot = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians(-(360 / (float)numKeyframes) * (i)), 0);
        XMFLOAT4 rot4;
        XMStoreFloat4(&rot4, rot);
        sampler3.timestamps.push_back(timeStamp);
        sampler3.vec4_values.push_back(rot4);

        sampler4.timestamps.push_back(timeStamp);
        sampler4.vec3_values.push_back(XMFLOAT3(
            cos(angle) * orbitRadius - orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        )
        );
    }
    for (int i = 0; i <= numKeyframes; ++i)
    {
        float t = (float)i / (float)numKeyframes;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;

        XMVECTOR rot;
        rot = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians((360 / (float)numKeyframes) * (i)), 0);
        XMFLOAT4 rot4;
        XMStoreFloat4(&rot4, rot);
        sampler3.timestamps.push_back(timeStamp + animDuration);
        sampler3.vec4_values.push_back(rot4);

        sampler4.timestamps.push_back(timeStamp + animDuration);
        sampler4.vec3_values.push_back(XMFLOAT3(
            -cos(angle) * orbitRadius + orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        ));
    }
    for (int i = 0; i <= numKeyframes/2; ++i)
    {
        float t = (float)i / (float)numKeyframes + 6;
        float angle = t * XM_2PI;
        float timeStamp = t * animDuration;

        XMVECTOR rot;
        rot = XMQuaternionRotationRollPitchYaw(0, XMConvertToRadians(-(360 / (float)numKeyframes) * (i)), 0);
        XMFLOAT4 rot4;
        XMStoreFloat4(&rot4, rot);
        sampler3.timestamps.push_back(timeStamp);
        sampler3.vec4_values.push_back(rot4);

        sampler4.timestamps.push_back(timeStamp);
        sampler4.vec3_values.push_back(XMFLOAT3(
            cos(angle) * orbitRadius - orbitRadius,
            0.0f,
            sin(angle) * orbitRadius
        )
        );
    }

    m_myAnimation6.m_samplers.push_back(sampler1);
    m_myAnimation6.m_samplers.push_back(sampler2);
    m_myAnimation6.m_samplers.push_back(sampler3);
    m_myAnimation6.m_samplers.push_back(sampler4);


    m_animations[5] = &m_myAnimation6;
    m_animationTimers[5] = 0.0f;
}

void Scene::initAnimation7()
{
    for (SceneGraph* object : m_objects)
    {
        if (object) object->Destroy();
        object = nullptr;
    }
    m_objects = vector<SceneGraph*>(100);
    m_sceneobject.LoadGLTFWithSkeleton(m_ctx, L"Resources\\fox.gltf");
    m_sceneobject.mRootNodes[0].SetMatrix(XMMatrixIdentity());
    m_sceneobject.mRootNodes[0].AddMatrix(XMMatrixRotationY(XMConvertToRadians(180)));
    m_sceneobject.mRootNodes[0].AddTranslation({ 0, -2, -1 });

    m_objects[0] = &m_sceneobject;
}

void Scene::CreateWaveAnimationSampler1(int nodeIndex, Animation* anim, Skeleton* skel)
{
    // Samplers for the hand's translation and rotation.
    AnimationSampler nodeTranslationSampler, nodeRotationSampler;

    // Get the hand's structural bind pose.
    DirectX::XMMATRIX nodeBindPose = DirectX::XMLoadFloat4x4(&skel->GetJoint(nodeIndex)->localBindTransform);

    // --- Keyframe 1: The Start Pose (t = 0.0s) ---
    // The hand is in its default, non-animated state.
    XMFLOAT3 startPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 0.0f, 0.0f });
    nodeTranslationSampler.timestamps.push_back(0.0f);
    nodeTranslationSampler.vec3_values.push_back(startPos);

    XMFLOAT4 startRot = BakeRotationOntoBindPose(nodeBindPose, { 0, 0, 1 }, 0.0f); // No rotation
    nodeRotationSampler.timestamps.push_back(0.0f);
    nodeRotationSampler.vec4_values.push_back(startRot);

    // --- Keyframe 2: The End Pose (t = 2.0s) ---
    // The hand is translated up and rotated 90 degrees to the side.
    XMFLOAT3 endPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 2.0f, 0.0f }); // Move up slightly
    nodeTranslationSampler.timestamps.push_back(2.0f);
    nodeTranslationSampler.vec3_values.push_back(endPos);

    XMFLOAT4 endRot = BakeRotationOntoBindPose(nodeBindPose, { 0, 0, 1 }, DirectX::XM_PIDIV2); // Rotate 90 degrees
    nodeRotationSampler.timestamps.push_back(2.0f);
    nodeRotationSampler.vec4_values.push_back(endRot);

    // --- Add Samplers and Channels for the node ---
    anim->m_samplers.push_back(nodeTranslationSampler); // Sampler x
    int nodeTranslationSamplerIndex = anim->m_samplers.size() - 1;
    anim->m_samplers.push_back(nodeRotationSampler);    // Sampler x+1
    int nodeRotationSamplerIndex = anim->m_samplers.size() - 1;


    AnimationChannel transChannel;
    transChannel.path = AnimationChannel::TRANSLATION;
    transChannel.samplerIndex = nodeTranslationSamplerIndex;
    transChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(transChannel);

    AnimationChannel rotChannel;
    rotChannel.path = AnimationChannel::ROTATION;
    rotChannel.samplerIndex = nodeRotationSamplerIndex;
    rotChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(rotChannel);

}

void Scene::CreateWaveAnimationSampler2(int nodeIndex, Animation* anim, Skeleton* skel)
{
    // Samplers for the hand's translation and rotation.
    AnimationSampler nodeTranslationSampler, nodeRotationSampler;

    // Get the hand's structural bind pose.
    DirectX::XMMATRIX nodeBindPose = DirectX::XMLoadFloat4x4(&skel->GetJoint(nodeIndex)->localBindTransform);

    // --- Keyframe 1: The Start Pose (t = 0.0s) ---
    // The hand is in its default, non-animated state.
    XMFLOAT3 startPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 0.0f, 0.0f });
    nodeTranslationSampler.timestamps.push_back(0.0f);
    nodeTranslationSampler.vec3_values.push_back(startPos);

    XMFLOAT4 startRot = BakeRotationOntoBindPose(nodeBindPose, { 0, 0, 1 }, 0.0f); // No rotation
    nodeRotationSampler.timestamps.push_back(0.0f);
    nodeRotationSampler.vec4_values.push_back(startRot);

    // --- Keyframe 2: The End Pose (t = 2.0s) ---
    // The hand is translated up and rotated 90 degrees to the side.
    XMFLOAT3 endPos = BakeTranslationOntoBindPose(nodeBindPose, { 0.0f, 0.0f, 0.0f }); // Move up slightly
    nodeTranslationSampler.timestamps.push_back(2.0f);
    nodeTranslationSampler.vec3_values.push_back(endPos);

    XMFLOAT4 endRot = BakeRotationOntoBindPose(nodeBindPose, { 1, 0, 0 }, DirectX::XM_PIDIV4); // Rotate 90 degrees
    nodeRotationSampler.timestamps.push_back(2.0f);
    nodeRotationSampler.vec4_values.push_back(endRot);

    // --- Add Samplers and Channels for the node ---
    anim->m_samplers.push_back(nodeTranslationSampler); // Sampler x
    int nodeTranslationSamplerIndex = anim->m_samplers.size() - 1;
    anim->m_samplers.push_back(nodeRotationSampler);    // Sampler x+1
    int nodeRotationSamplerIndex = anim->m_samplers.size() - 1;


    AnimationChannel transChannel;
    transChannel.path = AnimationChannel::TRANSLATION;
    transChannel.samplerIndex = nodeTranslationSamplerIndex;
    transChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(transChannel);

    AnimationChannel rotChannel;
    rotChannel.path = AnimationChannel::ROTATION;
    rotChannel.samplerIndex = nodeRotationSamplerIndex;
    rotChannel.jointIndex = nodeIndex;
    anim->m_channels.push_back(rotChannel);

}

void Scene::cleanUp()
{
    delete m_pCamera;
}

void Scene::setupLightProperties()
{

    Light light;
    light.Enabled = static_cast<int>(true);  // Enable the light
    light.LightType = PointLight;  // Set light type to point light
    light.Color = XMFLOAT4(0, 0, 1, 1);  // Set the light color to white
    light.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
    light.ConstantAttenuation = 1.0f;  // Attenuation factors
    light.LinearAttenuation = 0.0045f;
    light.QuadraticAttenuation = 0.00075f;

	Light light2;
	light2.Enabled = static_cast<int>(true);  // Enable the light
	light2.LightType = PointLight;  // Set light type to point light
	light2.Color = XMFLOAT4(1, 0, 0, 1);  // Set the light color to white
	light2.SpotAngle = XMConvertToRadians(45.0f);  // Set the spotlight's angle
	light2.ConstantAttenuation = 1.0f;  // Attenuation factors
	light2.LinearAttenuation = 0.09f;
	light2.QuadraticAttenuation = 0.032f;

    // Set up the light position based on the camera's position
    XMFLOAT4 LightPosition(3.5f, 0, 0, 1);
    light.Position = LightPosition;
    
    LightPosition = XMFLOAT4(-3.5, 0, 0, 1);
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

void Scene::setLightPos(int lightIndex, XMFLOAT4 pos)
{
    if (lightIndex >= 0 && lightIndex < MAX_LIGHTS) 
    {
		m_lightProperties.Lights[lightIndex].Position = pos;
    }
}

void Scene::PRBScene()
{
    XMFLOAT4 pos = m_lightProperties.Lights[0].Position;
    XMMATRIX lightPos = XMMatrixTranslation(pos.x, pos.y, pos.z);
    XMMATRIX lightScale = XMMatrixScaling(0.2, 0.2, 0.2);
    XMMATRIX out = lightScale * lightPos;
    m_sceneobject2.SetMatrixToRoots(out);

    if (m_lightProperties.Lights[1].Enabled)
    {

        XMFLOAT4 pos2 = m_lightProperties.Lights[1].Position;
        XMMATRIX lightPos2 = XMMatrixTranslation(pos2.x, pos2.y, pos2.z);
        XMMATRIX lightScale2 = XMMatrixScaling(0.2, 0.2, 0.2);
        XMMATRIX out2 = lightScale2 * lightPos2;
        m_sceneobject3.SetMatrixToRoots(out);
    }
    else
    {

        XMFLOAT4 pos2 = {1000,1000,1000,1};
        XMMATRIX lightPos2 = XMMatrixTranslation(pos2.x, pos2.y, pos2.z);
        XMMATRIX lightScale2 = XMMatrixScaling(0.2, 0.2, 0.2);
        XMMATRIX out2 = lightScale2 * lightPos2;
        m_sceneobject3.SetMatrixToRoots(out);
    }
}

void Scene::animation1(const float deltaTime)
{
    AnimationSampler sampler1 = m_animations[0]->m_samplers[0];
    AnimationSampler sampler1Rot = m_animations[0]->m_samplers[1];


    if (m_animationTimers[0] >= sampler1.timestamps.back())
        m_animationTimers[0] = 0; if (m_animationPlaying) m_animationTimers[0] += deltaTime;
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
        m_animationTimers[0] = sampler1.timestamps.back();

}

void Scene::animation2(const float deltaTime)
{
	float* timer = &m_animationTimers[1];
    if (m_animationPlaying) *timer += deltaTime;



    AnimationSampler sampler0 = m_animations[1]->m_samplers[0];
    AnimationSampler sampler0Rot = m_animations[1]->m_samplers[1];
    int nextKeyframe1 = -1;
    for (int i = 0; i < sampler0.timestamps.size(); ++i)
    {
        if (sampler0.timestamps[i] > m_animationTimers[1])
        {
            nextKeyframe1 = i;
            break;
        }
    }

    if (nextKeyframe1 == -1 || nextKeyframe1 == 0)
        nextKeyframe1 = 1;


    int prevKeyframe1 = nextKeyframe1 - 1;
    float prevTime1 = sampler0.timestamps[prevKeyframe1];
    float nextTime1 = sampler0.timestamps[nextKeyframe1];
    float t1 = (m_animationTimers[1] - prevTime1) / (nextTime1 - prevTime1);


    DirectX::XMVECTOR prevPos0 = DirectX::XMLoadFloat3(&sampler0.vec3_values[prevKeyframe1]);
    DirectX::XMVECTOR nextPos0 = DirectX::XMLoadFloat3(&sampler0.vec3_values[nextKeyframe1]);
    DirectX::XMVECTOR finalPos0 = DirectX::XMVectorLerp(prevPos0, nextPos0, t1);


    DirectX::XMVECTOR prevRot0 = DirectX::XMLoadFloat4(&sampler0Rot.vec4_values[prevKeyframe1]);
    DirectX::XMVECTOR nextRot0 = DirectX::XMLoadFloat4(&sampler0Rot.vec4_values[nextKeyframe1]);
    DirectX::XMVECTOR finalRot0 = DirectX::XMQuaternionSlerp(prevRot0, nextRot0, t1);


    DirectX::XMMATRIX object1Translation0 = DirectX::XMMatrixTranslationFromVector(finalPos0);
    DirectX::XMMATRIX object1Rotation0 = DirectX::XMMatrixRotationQuaternion(finalRot0);
    DirectX::XMMATRIX object1Scale0 = DirectX::XMMatrixScaling(0.25f, 0.25f, 0.25f);
	DirectX::XMMATRIX object1Transform0 = object1Scale0 * object1Rotation0* object1Translation0;
    DirectX::XMMATRIX object2Translation = DirectX::XMMatrixTranslationFromVector(XMVECTOR() = {0.0f,1.5f,0.0f});
    DirectX::XMMATRIX object2Scale = DirectX::XMMatrixScaling(0.75f, 0.75f, 0.75f);
    DirectX::XMMATRIX object2Transform = object2Scale * object1Rotation0 * object2Translation;

    m_sceneobject.GetRootNode(0)->SetMatrix(object1Transform0);
    
    SceneNode* child = m_sceneobject.GetRootNode(0)->GetChildNode(0);
    child->SetMatrix(XMMatrixIdentity());
    child->AddMatrix(object2Transform);

    child = child->GetChildNode(0);
    child->SetMatrix(XMMatrixIdentity());
    child->AddMatrix(object2Transform);

    child = child->GetChildNode(0);
    child->SetMatrix(XMMatrixIdentity());
    child->AddMatrix(object2Transform);


    if (m_animationTimers[1] >= sampler0.timestamps.back())
        m_animationTimers[1] = 0;
    if (m_animationTimers[1] < -0.01)
        m_animationTimers[1] = sampler0.timestamps.back();


    m_sceneobject.AnimateFrame(m_ctx);
    m_sceneobject.RenderFrame(m_ctx, deltaTime);
}

void Scene::animation3(const float deltaTime)
{
    if (m_anim3Initialized)
    {
        m_anim3Initialized = false;
        m_anim3Skeleton.PlayAnimation(&m_myAnimation3);
        m_anim3_5Skeleton.PlayAnimation(&m_myAnimation3_5);
    }

    if (m_animationPlaying)
    {
        m_anim3Skeleton.Update(deltaTime);
        m_anim3_5Skeleton.Update(deltaTime);
    }

    for (int i = 0; i < m_anim3Skeleton.GetBoneCount(); ++i)
    {
        Joint* joint = m_anim3Skeleton.GetJoint(i);
        XMMATRIX finalWorldTransform = XMLoadFloat4x4(&joint->finalTransform);
        m_anim3SceneNodes[i]->SetMatrix(finalWorldTransform);
    }

    for (int i = 0; i < m_anim3_5Skeleton.GetBoneCount(); ++i)
    {
        Joint* joint = m_anim3_5Skeleton.GetJoint(i);
        XMMATRIX finalWorldTransform = XMLoadFloat4x4(&joint->finalTransform);
        m_anim3_5SceneNodes[i]->SetMatrix(finalWorldTransform);
    }


    m_sceneobject.AnimateFrame(m_ctx);
    m_sceneobject.RenderFrame(m_ctx, deltaTime);


    m_sceneobject2.AnimateFrame(m_ctx);
    m_sceneobject2.RenderFrame(m_ctx, deltaTime);
}

void Scene::animation4(const float deltaTime) 
{

    if (doOnce)
    {
        doOnce = false;

        m_robotArmSkeleton.PlayAnimation(&m_robotArmAnimations[0]);
    }

    m_robotArmSkeleton.Update(deltaTime);

    for (int i = 0; i < m_robotArmSkeleton.GetBoneCount(); ++i)
    {
        Joint* joint = m_robotArmSkeleton.GetJoint(i);
        DirectX::XMMATRIX finalWorldTransform = XMLoadFloat4x4(&joint->finalTransform);

        m_armSegmentNodes[i]->SetMatrix(finalWorldTransform);
    }


    m_sceneobject.AnimateFrame(m_ctx);
    m_sceneobject.RenderFrame(m_ctx, deltaTime);
}

void Scene::animation5(const float deltaTime) 
{
}

void Scene::animation6(const float deltaTime)
{
    AnimationSampler sampler1 = m_animations[5]->m_samplers[0];
    AnimationSampler sampler2 = m_animations[5]->m_samplers[1];
    AnimationSampler sampler3 = m_animations[5]->m_samplers[2];
    AnimationSampler sampler4 = m_animations[5]->m_samplers[3];
    float* animationTimer = &m_animationTimers[5];

    if (m_animationTimers[5] >= sampler1.timestamps.back())
        m_animationTimers[5] = 0;
    if (m_animationPlaying) m_animationTimers[5] += deltaTime;

    int nextKeyframe1 = -1;
    for (int i = 0; i < sampler1.timestamps.size(); ++i)
    {
        if (sampler1.timestamps[i] > m_animationTimers[5])
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
    float t1 = (m_animationTimers[5] - prevTime1) / (nextTime1 - prevTime1);

    // First fox animation (large one)
    DirectX::XMVECTOR prevPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[prevKeyframe1]);
    DirectX::XMVECTOR nextPos1 = DirectX::XMLoadFloat3(&sampler1.vec3_values[nextKeyframe1]);
    DirectX::XMVECTOR finalPos1 = DirectX::XMVectorLerp(prevPos1, nextPos1, t1);

    DirectX::XMVECTOR prevRot1 = DirectX::XMLoadFloat4(&sampler2.vec4_values[prevKeyframe1]);
    DirectX::XMVECTOR nextRot1 = DirectX::XMLoadFloat4(&sampler2.vec4_values[nextKeyframe1]);
    DirectX::XMVECTOR finalRot1 = DirectX::XMQuaternionSlerp(prevRot1, nextRot1, t1);

    DirectX::XMMATRIX object1Rotation = XMMatrixRotationQuaternion(finalRot1);
    DirectX::XMMATRIX object1Translation = DirectX::XMMatrixTranslationFromVector(finalPos1);
    XMMATRIX object1Scale = XMMatrixIdentity();
    DirectX::XMMATRIX object1Transform = object1Scale * object1Rotation * object1Translation;

    m_sceneobject.GetRootNode(0)->SetMatrix(object1Transform);
    m_sceneobject.GetRootNode(0)->AddTranslation({ 0, -2, 0 });

    const int keyframeOffset = 8;
    int nextKeyframe2 = nextKeyframe1 + keyframeOffset;

    if (nextKeyframe2 >= sampler3.vec3_values.size())
        nextKeyframe2 -= sampler3.vec3_values.size();

    int prevKeyframe2 = nextKeyframe2 - 1;
    if (prevKeyframe2 < 0)
        prevKeyframe2 += sampler3.vec3_values.size();

    DirectX::XMVECTOR prevPos2 = DirectX::XMLoadFloat3(&sampler3.vec3_values[prevKeyframe2]);
    DirectX::XMVECTOR nextPos2 = DirectX::XMLoadFloat3(&sampler3.vec3_values[nextKeyframe2]);
    DirectX::XMVECTOR finalPos2 = DirectX::XMVectorLerp(prevPos2, nextPos2, t1);

    DirectX::XMVECTOR prevRot2 = DirectX::XMLoadFloat4(&sampler4.vec4_values[prevKeyframe2]);
    DirectX::XMVECTOR nextRot2 = DirectX::XMLoadFloat4(&sampler4.vec4_values[nextKeyframe2]);
    DirectX::XMVECTOR finalRot2 = DirectX::XMQuaternionSlerp(prevRot2, nextRot2, t1);

    DirectX::XMMATRIX object2Rotation = XMMatrixRotationQuaternion(finalRot2);
    DirectX::XMMATRIX object2Translation = DirectX::XMMatrixTranslationFromVector(finalPos2);
    XMMATRIX object2Scale = XMMatrixIdentity() * XMMatrixScaling(.25f, .25f, .25f);
    DirectX::XMMATRIX object2Transform = object2Scale * object2Rotation * object2Translation;

    m_sceneobject.GetRootNode(1)->SetMatrix(object2Transform);
    m_sceneobject.GetRootNode(1)->AddTranslation({ 0, -2, 0 });

    if (m_animationTimers[5] < -0.01)
        m_animationTimers[5] = sampler1.timestamps.back();
}

void Scene::animation7(const float deltaTime) 
{
    
}

DirectX::XMFLOAT3 Scene::BakeTranslationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animTranslation)
{
    // 1. Create the animation matrix from the vector.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixTranslation(animTranslation.x, animTranslation.y, animTranslation.z);

    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;

    // 3. Extract and return the final position.
    DirectX::XMFLOAT3 finalPosition;
    DirectX::XMStoreFloat3(&finalPosition, finalLocalMatrix.r[3]);
    return finalPosition;
}

DirectX::XMFLOAT4 Scene::BakeRotationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& axis, float angleRadians)
{
    // 1. Create the animation matrix from the axis and angle.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&axis), angleRadians);

    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;

    // 3. Decompose to safely extract and return the final rotation quaternion.
    DirectX::XMVECTOR scale, finalRotationQuat, translation;
    DirectX::XMMatrixDecompose(&scale, &finalRotationQuat, &translation, finalLocalMatrix);

    DirectX::XMFLOAT4 finalRotation;
    DirectX::XMStoreFloat4(&finalRotation, finalRotationQuat);
    return finalRotation;
}

DirectX::XMFLOAT3 Scene::BakeScaleOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animScale)
{
    // 1. Create the animation matrix from the vector.
    DirectX::XMMATRIX animMatrix = DirectX::XMMatrixScaling(animScale.x, animScale.y, animScale.z);

    // 2. Bake the animation onto the bind pose.
    DirectX::XMMATRIX finalLocalMatrix = animMatrix * bindPose;

    // 3. Decompose to safely extract and return the final scale vector.
    DirectX::XMVECTOR finalScale, rotation, translation;
    DirectX::XMMatrixDecompose(&finalScale, &rotation, &translation, finalLocalMatrix);

    DirectX::XMFLOAT3 finalScaleVec;
    DirectX::XMStoreFloat3(&finalScaleVec, finalScale);
    return finalScaleVec;
}

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
    case 3:
        animation3(deltaTime);
        break;
    case 4:
        animation4(deltaTime);
        break;
    case 5:
        animation5(deltaTime);
        break;
    case 6:
        animation6(deltaTime);
        break;
    case 7:
        PRBScene();
    }



	//---------------rendering part---------------

    ID3D11ShaderResourceView* nullSRV = nullptr;

    // Bind texture resources to pixel shader stages
    m_pImmediateContext->PSSetShaderResources(0, 1, &m_pTextureDiffuse);
    m_pImmediateContext->PSSetShaderResources(1, 1, &m_pTextureMetallic);
    m_pImmediateContext->PSSetShaderResources(2, 1, &m_pTextureRoughness);
    m_pImmediateContext->PSSetShaderResources(3, 1, &m_pTextureSpecularIBL);
    m_pImmediateContext->PSSetShaderResources(4, 1, &m_pTextureDiffuseIBL);

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


    for(auto& var :m_objects)
    {
        if (!var) continue;
        var->AnimateFrame(m_ctx);
        var->RenderFrame(m_ctx,deltaTime);
    }

}