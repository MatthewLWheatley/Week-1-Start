#include "Blend.h"
#include "Skeleton.h"

using namespace DirectX;

void BlendNode::Update(float deltaTime) 
{
    
    
    if (type == BLENDTOGETHER) {
        for (size_t i = 0; i < m_currentTimes.size(); i++)
        {
            m_currentTimes[i] += deltaTime;
            if (m_currentTimes[i] >= m_endTimes[i])
                m_currentTimes[i] = m_startTimes[i];
        }



        float weight = 0.5f;
        DirectX::XMMATRIX rootTransform = XMLoadFloat4x4(&skelParent->m_rootTransform);

        std::vector<std::vector<XMMATRIX>> sampledTransforms;
        sampledTransforms.resize(m_playingAnimations.size());

        for (size_t animIdx = 0; animIdx < m_playingAnimations.size(); animIdx++)
        {
            sampledTransforms[animIdx].resize(skelParent->m_joints.size());

            for (int rootIndex : skelParent->m_rootJointIndices)
            {
                SampleJointTransformRecursive(rootIndex, &m_playingAnimations[animIdx],
                    m_currentTimes[animIdx], rootTransform, sampledTransforms[animIdx]);
            }
        }

        for (size_t jointIdx = 0; jointIdx < skelParent->m_joints.size(); jointIdx++)
        {
            XMMATRIX blended = sampledTransforms[0][jointIdx];

            if (m_playingAnimations.size() > 1)
            {
                XMVECTOR scale1, rot1, trans1;
                XMVECTOR scale2, rot2, trans2;
                XMMatrixDecompose(&scale1, &rot1, &trans1, sampledTransforms[0][jointIdx]);
                XMMatrixDecompose(&scale2, &rot2, &trans2, sampledTransforms[1][jointIdx]);

                XMVECTOR blendedScale = XMVectorLerp(scale1, scale2, weight);
                XMVECTOR blendedRot = XMQuaternionSlerp(rot1, rot2, weight);
                XMVECTOR blendedTrans = XMVectorLerp(trans1, trans2, weight);

                blended = XMMatrixScalingFromVector(blendedScale) *
                    XMMatrixRotationQuaternion(blendedRot) *
                    XMMatrixTranslationFromVector(blendedTrans);
            }

            XMStoreFloat4x4(&skelParent->m_joints[jointIdx].finalTransform, blended);
        }

        for (size_t i = 0; i < skelParent->m_joints.size(); ++i) {
            XMMATRIX inv = XMLoadFloat4x4(&skelParent->m_joints[i].inverseBindMatrix);
            XMMATRIX finalTransform = XMLoadFloat4x4(&skelParent->m_joints[i].finalTransform);
            XMMATRIX out = inv * finalTransform;
            XMStoreFloat4x4(&skelParent->m_skinningMatrices[i], out);
        }
    }

}

void BlendNode::SampleJointTransformRecursive(int jointIndex, const Animation* anim, float time,
    const DirectX::XMMATRIX& parentTransform, std::vector<XMMATRIX>& outTransforms)
{
    XMMATRIX localAnimated = skelParent->GetLocalAnimatedMatrixForJoint(
        skelParent->m_joints[jointIndex], jointIndex, anim, time);
    XMMATRIX worldTransform = localAnimated * parentTransform;

    outTransforms[jointIndex] = worldTransform;

    for (int childIdx : skelParent->m_joints[jointIndex].children) {
        SampleJointTransformRecursive(childIdx, anim, time, worldTransform, outTransforms);
    }
}