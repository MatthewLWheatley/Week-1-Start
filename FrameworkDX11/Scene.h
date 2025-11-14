// MIT License
// Copyright (c) 2025 David White
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// Scene class for encapsulating the responsibily for storing / initialising the world and whatever is in it

#pragma once

#include "constants.h"
#include "Camera.h"
#include <d3d11_1.h>
#include <vector>
#include "wrl.h"
#include "structures.h"
#include "scenegraph.h"

class DX11Renderer;

class Scene
{
public:

	Scene() {}

	~Scene() {}

	HRESULT		init(HWND hwnd, const Microsoft::WRL::ComPtr<ID3D11Device>& device, const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DX11Renderer* renderer);
	
	void		cleanUp();
	Camera*		getCamera() { return m_pCamera; }

	void setLightPos(int lightIndex, XMFLOAT4 pos);


	void		update(const float deltaTime);
	
	const LightPropertiesConstantBuffer& getLightProperties() { return m_lightProperties; }

	int textureIndex = 0;
	XMFLOAT3 albedo = XMFLOAT3(1.0f, 1.0f, 1.0f);
	float metal = 0.0f;
	float rough = 0.0f;
	float type = 2.0f;
	float textureSelect = 1.0f;
	int lightCount = 2;


	friend class Dx11Renderer;

	DX11Renderer* m_pRenderer = nullptr;


	void initAnimation1();
	void initAnimation2();
	void initAnimation3();
	void initAnimation3_5();
	void initAnimation4();
	void initAnimation5();
	void initAnimation6();

	void CreateWaveAnimationSampler1(int nodeIndex, Animation* anim, Skeleton* skel);

	void CreateWaveAnimationSampler2(int nodeIndex, Animation* anim, Skeleton* skel);

	Skeleton m_robotArmSkeleton;
	std::vector<SceneNode*> m_armSegmentNodes;
	std::vector<Animation> m_robotArmAnimations;


private:
	void setupLightProperties();
public:
	Camera* m_pCamera;
	
	Microsoft::WRL::ComPtr <ID3D11Device>			m_pd3dDevice;
	Microsoft::WRL::ComPtr <ID3D11DeviceContext>	m_pImmediateContext;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pConstantBufferlight;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pConstantBufferSwitch;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pConstantBuffer;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pLightConstantBuffer;
	Microsoft::WRL::ComPtr <ID3D11Buffer>			m_pCustomConstantBuffer;


	LightPropertiesConstantBuffer m_lightProperties;
	IRenderingContext m_ctx;
	SceneGraph m_sceneobject;
	SceneGraph m_sceneobject2;
	SceneGraph m_sceneobject3;

	vector<SceneGraph*> m_objects = vector<SceneGraph*>(100);

	Animation m_myAnimation1;
	Animation m_myAnimation2;
	Animation m_myAnimation3;
	Animation m_myAnimation3_5;
	Animation m_myAnimation4;
	Animation m_myAnimation6;
	vector<Animation*> m_animations = vector<Animation*>(100);
	vector<float> m_animationTimers = vector<float>(100);
	void animation1(const float deltaTime);
	void animation2(const float deltaTime);
	void animation3(const float deltaTime);
	void animation4(const float deltaTime);
	void animation5(const float deltaTime);
	void animation6(const float deltaTime);
	DirectX::XMFLOAT3 BakeTranslationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animTranslation);
	DirectX::XMFLOAT4 BakeRotationOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& axis, float angleRadians);
	DirectX::XMFLOAT3 BakeScaleOntoBindPose(const DirectX::XMMATRIX& bindPose, const DirectX::XMFLOAT3& animScale);
	int m_animationSelected = 0;
	bool m_animationPlaying = true;
	bool doOnce = true;
	Skeleton m_anim3Skeleton;
	std::vector<SceneNode*> m_anim3SceneNodes;
	Skeleton m_anim3_5Skeleton;
	std::vector<SceneNode*> m_anim3_5SceneNodes;
	bool m_anim3Initialized = true;

private:
	ID3D11ShaderResourceView* m_pTextureDiffuse;
	ID3D11ShaderResourceView* m_pTextureNormal;
	ID3D11ShaderResourceView* m_pTextureMetallic;
	ID3D11ShaderResourceView* m_pTextureRoughness;
	ID3D11ShaderResourceView* m_pTextureAmbientOcclusion;
	ID3D11ShaderResourceView* m_pTextureSpecularIBL;
	ID3D11ShaderResourceView* m_pTextureDiffuseIBL;

	ID3D11ShaderResourceView* m_pPaveTextureDiffuse;
	ID3D11ShaderResourceView* m_pPaveTextureNormal;
	ID3D11ShaderResourceView* m_pPaveTextureMetallic;
	ID3D11ShaderResourceView* m_pPaveTextureRoughness;
	ID3D11ShaderResourceView* m_pPaveTextureAmbientOcclusion;
	ID3D11ShaderResourceView* m_pPaveTextureSpecularIBL;
	ID3D11ShaderResourceView* m_pPaveTextureDiffuseIBL;

	ID3D11SamplerState* m_pSamplerLinear;
};

