#include "Blend.h"
#include "Skeleton.h"

using namespace DirectX;

void BlendNode::AddAnime(Animation* anime, std::string name)
{
    m_playingAnimations.push_back(*anime);
    m_startTimes.push_back(anime->GetStartTime());
    m_endTimes.push_back(anime->GetEndTime());
    m_names.push_back(anime->m_name);
    m_currentTimes.push_back(0.0f);
    m_playStates.push_back(true);
    m_weights.push_back(1.0f);
}

void BlendNode::RemoveAnime(int ID)
{
    m_playingAnimations.erase(m_playingAnimations.begin()+ID);
    m_startTimes.erase(m_startTimes.begin()+ID);
    m_endTimes.erase(m_endTimes.begin()+ID);
    m_names.erase(m_names.begin()+ID);
    m_currentTimes.erase(m_currentTimes.begin()+ID);
    m_playStates.erase(m_playStates.begin()+ID);
    m_weights.erase(m_weights.begin()+ID);
}

void BlendNode::Update(float deltaTime)
{
    
    
    if (type == BLENDTOGETHER) {
        for (size_t i = 0; i < m_currentTimes.size(); i++)
        {
            if (!m_playStates[i]) continue;
            m_currentTimes[i] += deltaTime;
            if (m_currentTimes[i] >= m_endTimes[i])
                m_currentTimes[i] = m_startTimes[i];
        }

        
        float totalWeight = 0;
        for (int i = 0; i < m_weights.size(); i++)
        {
            totalWeight += m_weights[i];
        }
        std::vector<float> normalWeights = std::vector<float>(m_weights.size());
        for (int i = 0; i < m_weights.size(); i++)
        {
            normalWeights[i] = m_weights[i]/totalWeight;
        }

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
            /*
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
            }*/
            XMVECTOR accumulatedScale = XMVectorZero();
            XMVECTOR accumulatedRot = XMVectorZero();
            XMVECTOR accumulatedTrans = XMVectorZero();
            XMVECTOR s, r, t;
            for (int animIdx = 0; animIdx < m_playingAnimations.size(); animIdx++) {
                XMMatrixDecompose(&s, &r, &t, sampledTransforms[animIdx][jointIdx]);

                accumulatedScale += s * normalWeights[animIdx];

                if (animIdx > 0) {
                    XMVECTOR firstRot, tempS, tempT;
                    XMMatrixDecompose(&tempS, &firstRot, &tempT, sampledTransforms[0][jointIdx]);
                    if (XMVectorGetX(XMQuaternionDot(firstRot, r)) < 0.0f) {
                        r = XMVectorNegate(r);
                    }
                }

                accumulatedRot += r * normalWeights[animIdx];
                accumulatedTrans += t * normalWeights[animIdx];
            }

            accumulatedRot = XMQuaternionNormalize(accumulatedRot);

            blended = XMMatrixScalingFromVector(accumulatedScale) *
                XMMatrixRotationQuaternion(accumulatedRot) *
                XMMatrixTranslationFromVector(accumulatedTrans);
            XMStoreFloat4x4(&skelParent->m_joints[jointIdx].finalTransform, blended);
        }

        for (size_t i = 0; i < skelParent->m_joints.size(); ++i) {
            XMMATRIX inv = XMLoadFloat4x4(&skelParent->m_joints[i].inverseBindMatrix);
            XMMATRIX finalTransform = XMLoadFloat4x4(&skelParent->m_joints[i].finalTransform);
            XMMATRIX out = inv * finalTransform;
            XMStoreFloat4x4(&skelParent->m_skinningMatrices[i], out);
        }
    }
    if (type == BLENDINOUT) 
    {
        for (size_t i = 0; i < m_currentTimes.size(); i++)
        {
            if (!m_playStates[i]) continue;
            m_currentTimes[i] += deltaTime;
            if (m_currentTimes[i] >= m_endTimes[i])
                m_currentTimes[i] = m_startTimes[i];
        }


        float totalWeight = 0;
        for (int i = 0; i < m_weights.size(); i++)
        {
            totalWeight += m_weights[i];
        }
        std::vector<float> normalWeights = std::vector<float>(m_weights.size());
        for (int i = 0; i < m_weights.size(); i++)
        {
            normalWeights[i] = m_weights[i] / totalWeight;
        }

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
            /*
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
            }*/
            XMVECTOR accumulatedScale = XMVectorZero();
            XMVECTOR accumulatedRot = XMVectorZero();
            XMVECTOR accumulatedTrans = XMVectorZero();
            XMVECTOR s, r, t;
            for (int animIdx = 0; animIdx < m_playingAnimations.size(); animIdx++) {
                XMMatrixDecompose(&s, &r, &t, sampledTransforms[animIdx][jointIdx]);

                accumulatedScale += s * normalWeights[animIdx];

                if (animIdx > 0) {
                    XMVECTOR firstRot, tempS, tempT;
                    XMMatrixDecompose(&tempS, &firstRot, &tempT, sampledTransforms[0][jointIdx]);
                    if (XMVectorGetX(XMQuaternionDot(firstRot, r)) < 0.0f) {
                        r = XMVectorNegate(r);
                    }
                }

                accumulatedRot += r * normalWeights[animIdx];
                accumulatedTrans += t * normalWeights[animIdx];
            }

            accumulatedRot = XMQuaternionNormalize(accumulatedRot);

            blended = XMMatrixScalingFromVector(accumulatedScale) *
                XMMatrixRotationQuaternion(accumulatedRot) *
                XMMatrixTranslationFromVector(accumulatedTrans);
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