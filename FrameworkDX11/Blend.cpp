#include "Blend.h"
#include "Skeleton.h"

using namespace DirectX;

void BlendNode::Update(float deltaTime) 
{
    int count = 0;
    for (auto& currentTime : m_currentTimes)
    {
        
        currentTime += deltaTime;

        if (currentTime >= m_endTimes[count])
            currentTime = m_startTimes[count];
        count++;
    }
    count = 0;
    
    for (auto& anim : m_playingAnimations)
    {
        DirectX::XMMATRIX rootTransform = DirectX::XMLoadFloat4x4(&skelParent->m_rootTransform);
        for (int rootIndex : skelParent->m_rootJointIndices) 
        {
            skelParent->UpdateJointTransform(rootIndex, &anim, m_currentTimes[count], rootTransform);
        }
        for (size_t i = 0; i < skelParent->m_joints.size(); ++i) {
            XMMATRIX inv = XMLoadFloat4x4(&skelParent->m_joints[i].inverseBindMatrix);
            XMMATRIX finalTransform = XMLoadFloat4x4(&skelParent->m_joints[i].finalTransform);
            XMMATRIX out = inv * finalTransform;
            XMStoreFloat4x4(&skelParent->m_skinningMatrices[i], out);
        }
        count++;
    }
}