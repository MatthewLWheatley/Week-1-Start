#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include "tiny_gltf.h" // For loading
#include <map>
#include "Animation.h"

class Skeleton;


class BlendNode
{
public:
	std::vector<Animation> m_playingAnimations;
	std::vector<float> m_currentTimes;
	std::vector<float> m_startTimes;
	std::vector<float> m_endTimes;
	std::vector<std::string> m_names;
	std::vector<bool> m_playStates;
	std::vector<float> m_weights;
	Skeleton* skelParent;

	enum BLENDTYPE
	{
		BLENDOUT,
		BLENDINOUT,
		BLENDTOGETHER,
	};

	BLENDTYPE type = BLENDTOGETHER;

	BlendNode(Skeleton* skel, Animation ani1, Animation ani2)
	{
		skelParent = skel;
		m_playingAnimations.push_back(ani1);
		m_playingAnimations.push_back(ani2);
		m_startTimes.push_back(ani1.GetStartTime());
		m_startTimes.push_back(ani2.GetStartTime());
		m_endTimes.push_back(ani1.GetEndTime());
		m_endTimes.push_back(ani2.GetEndTime());
		m_names.push_back(ani1.m_name);
		m_names.push_back(ani2.m_name);
		m_currentTimes.push_back(0.0f);
		m_currentTimes.push_back(0.0f);
		m_playStates.push_back(true);
		m_playStates.push_back(true);
		m_weights.push_back(1.0f);
		m_weights.push_back(1.0f);
	}

	BlendNode(BlendNode blend1, Animation ani2)
	{

		skelParent = blend1.skelParent;
		for (auto& ani : blend1.m_playingAnimations)
		{
			m_playingAnimations.push_back(ani);
		}
		for (auto& start : blend1.m_startTimes)
		{
			m_startTimes.push_back(start);
		}
		for (auto& end : blend1.m_endTimes)
		{
			m_endTimes.push_back(end);
		}
		for (auto& name : blend1.m_names)
		{
			m_names.push_back(name);
		}
		for (auto& currentTime : blend1.m_currentTimes)
		{
			m_currentTimes.push_back(currentTime);
		}
		m_playingAnimations.push_back(ani2);
		m_startTimes.push_back(ani2.GetStartTime());
		m_endTimes.push_back(ani2.GetEndTime());
		m_names.push_back(ani2.m_name);
		m_currentTimes.push_back(0.0f);
		m_playStates.push_back(true);
		m_weights.push_back(1.0f);
	}

	void AddAnime(Animation* anime, std::string name);
	void RemoveAnime(int ID);

	void Update(float deltaTime); 
	void SampleJointTransformRecursive(int jointIndex, const Animation* anim, float time, const DirectX::XMMATRIX& parentTransform, std::vector<DirectX::XMMATRIX>& outTransforms);
};