//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World; // World transformation matrix (object to world space)
    matrix View; // Camera view matrix (world to view space)
    matrix Projection; // Camera projection matrix (view to clip space)
    float4 vOutputColor; // Color to be used as output color (e.g., for solid color rendering)
    float4 frank;
    float metal = 0;
    float rough = 0;
    float type = 2;
    float textureSelect = 1;
    float4x4 g_boneTransforms[100]; // Must match max_bones on CPU
    unsigned int bone_count;
}

cbuffer ConstantBuffer : register(b2)
{
    float4 vOutputColor2; // Color to be used as output color (e.g., for solid color rendering)
}

Texture2D albedoMap : register(t0); // Albedo (diffuse) texture
Texture2D MetallicMap : register(t1); // Normal map for lighting effects
Texture2D RoughnessMap : register(t2); // Roughness map (PBR)
TextureCube iblSpecular : register(t3);
TextureCube iblIrradiance : register(t4);
Texture2D NormalMap : register(t5); // Normal map for lighting effects
Texture2D DisplacementMap : register(t6); // Displacement map for parallax effects

SamplerState samLinear : register(s0); // Texture sampler for linear filtering

static const float PI = 3.14159265f; // Value of PI (used for angle calculations)

// Maximum number of lights supported
#define MAX_LIGHTS 10

// Light types (for future expansion to support more light types)
#define DIRECTIONAL_LIGHT 
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

static const float maxReflectionLod = 5;

// Struct to represent light properties
struct Light
{
    float4 Position; // Light position (used for point and spot lights)
    float4 Direction; // Light direction (used for directional lights)
    float4 Color; // Light color (RGBA)
    float SpotAngle; // Spot light angle
    float ConstantAttenuation; // Constant attenuation factor (for point lights)
    float LinearAttenuation; // Linear attenuation factor (for point lights)
    float QuadraticAttenuation; // Quadratic attenuation factor (for point lights)
    int LightType; // Light type (e.g., directional, point, spot)
    bool Enabled; // Flag to enable/disable the light
    int2 Padding; // Padding to align struct size to 16 bytes
}; // Total size: 80 bytes

// Constant buffer for light properties
cbuffer LightProperties : register(b1)
{
    float4 EyePosition; // Camera position (for lighting calculations)
    float4 GlobalAmbient; // Global ambient light color
    Light Lights[MAX_LIGHTS]; // Array of light sources (with max count defined by MAX_LIGHTS)
}; 

//--------------------------------------------------------------------------------------
// Vertex Shader Input and Output Structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION; // Vertex position (in object space)
    float3 Norm : NORMAL; // Normal vector (in object space)
    float4 Tangent : TANGENT; // Tangent vector (for normal mapping)
    float2 Tex : TEXCOORD0; // Texture coordinates
    uint4 Joints : BLENDINDICES0; // Bone indices for skinning
    float4 Weights : BLENDWEIGHT0; // Bone weights for skinning
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION; // Transformed vertex position (to screen space)
    float4 worldPos : POSITION; // World-space position (used for lighting)
    float3 Norm : NORMAL; // Normal vector (in world space)
    float2 Tex : TEXCOORD0; // Texture coordinates
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 finalPos;
    float3 finalNorm;
    
    if (bone_count > 0)
    {
        float4 skinnedPos = float4(0, 0, 0, 0);
        float3 skinnedNorm = float3(0, 0, 0);
        
        int index0 = input.Joints.x;
        float4x4 joint0Matrix = g_boneTransforms[index0];
        float joint0Weight = input.Weights.x;
        skinnedPos += mul(input.Pos, joint0Matrix) * joint0Weight;
        skinnedNorm += mul(input.Norm, (float3x3) joint0Matrix) * joint0Weight;
        
        int index1 = input.Joints.y;
        float4x4 joint1Matrix = g_boneTransforms[index1];
        float joint1Weight = input.Weights.y;
        skinnedPos += mul(input.Pos, joint1Matrix) * joint1Weight;
        skinnedNorm += mul(input.Norm, (float3x3) joint1Matrix) * joint1Weight;
        
        int index2 = input.Joints.z;
        float4x4 joint2Matrix = g_boneTransforms[index2];
        float joint2Weight = input.Weights.z;
        skinnedPos += mul(input.Pos, joint2Matrix) * joint2Weight;
        skinnedNorm += mul(input.Norm, (float3x3) joint2Matrix) * joint2Weight;
        
        int index3 = input.Joints.w;
        float4x4 joint3Matrix = g_boneTransforms[index3];
        float joint3Weight = input.Weights.w;
        skinnedPos += mul(input.Pos, joint3Matrix) * joint3Weight;
        skinnedNorm += mul(input.Norm, (float3x3) joint3Matrix) * joint3Weight;
        
        skinnedPos.w = 1.0f;
        
        finalPos = skinnedPos;
        finalNorm = normalize(skinnedNorm);
    }
    else
    {
        finalPos = input.Pos;
        finalNorm = input.Norm;
    }
    
    output.Pos = mul(finalPos, World);
    output.Norm = normalize(finalNorm);
    output.worldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    output.Norm = normalize(mul(float4(input.Norm, 0), World)).xyz;

    output.Tex = input.Tex;
    return output;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float NormalDistrobution(float roughness, float3 N, float3 H)
{
    float NdotH = max(dot(N, H), 0.0);
    float a = roughness * roughness;
    float a2 = a*a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * (denom * denom));
}

float G_sub(float roughness, float3 N, float3 V)
{
    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    float NdotV = max(dot(N, V), 0.0);
    return NdotV / (NdotV * (1.0 - k) + k);
}

float3 BRDFFunction(float cosTheta, float NdotL, float3 F,float D,float G)
{
    float3 bottom = max(4.0 * cosTheta * NdotL, 0.001);
    return (F * G * D) / bottom;
}

float3 diffuse(float3 albedoT, float3 F, float metallic)
{
    float3 kD = (float3(1, 1, 1) - F) * (1 - metallic);
    return kD * (albedoT / PI);
}

float2 IntergrateBRDF(float NdotV, float roughness)
{
    const float4 c0 = float4(-1, -0.0275, -0.572, 0.022);
    const float4 c1 = float4(1, 0.0425, 1.04, -0.04);
    
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    
    return float2(-1.04, 1.04) * a004 + r.zw;
}

float4 PS_PBR(PS_INPUT IN) : SV_TARGET
{
    float3 finalColour = float3(0, 0, 0);
    
    float3 albedo = frank.xyz;
    float metallic = metal;
    float roughness = rough;
    
    if (textureSelect == 1)
    {
        albedo = albedoMap.Sample(samLinear, IN.Tex).xyz;
        metallic = MetallicMap.Sample(samLinear, IN.Tex).r;
        roughness = RoughnessMap.Sample(samLinear, IN.Tex).r;
    }
    roughness = max(roughness, 0.001f);
    float3 N = normalize(IN.Norm);
    float3 V = normalize(EyePosition - IN.worldPos).xyz;
    float cosTheta = max(dot(N, V), 0.0);
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    float3 F = FresnelSchlick(cosTheta, F0);
    
    float3 color = float3(0, 0, 0);
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (!Lights[i].Enabled)
            continue;
        float3 L = normalize(Lights[i].Position - IN.worldPos).xyz;
        float3 H = normalize(V + L);
    
        float NdotL = max(dot(N, L), 0.0);
    

        // part A
        float D = NormalDistrobution(roughness, N, H);
        float G = G_sub(roughness, N, V) * G_sub(roughness, N, L);
        float3 BRDF = BRDFFunction(cosTheta, NdotL, F, D, G);
        float3 Diffuse = diffuse(albedo, F, metallic);
    
        float3 Lo = (Diffuse + BRDF) * NdotL;
        
        float3 L2 = Lights[i].Position.xyz - IN.worldPos.xyz;
        float distance = length(L2);
        float attenuation = 1.0f / (Lights[i].ConstantAttenuation + Lights[i].LinearAttenuation * distance + Lights[i].QuadraticAttenuation * (distance * distance));
        Lo = Lo * (Lights[i].Color.xyz);
        
        color = color + Lo;
    }
    
    
    float3 finalIBL = float3(0, 0, 0);
    int typeIBL = type;

    if (typeIBL == 0)
    {
        float3 ambient = float3(0.7, 0.7, 0.7);
        float3 kD = (float3(1, 1, 1) - F) * (1 - metallic);
        finalIBL = ambient * albedo * kD;
        
    }
    else if (typeIBL == 1)
    {
        float3 skyColor = float3(0.11, 0.11, 0.94);
        float3 groundColor = float3(0.0, 0.33, 0.0);
        
        float3 up = float3(0, 1, 0);
        
        float blend = dot(N, up) * 0.5 + 0.5;
        
        float3 diffuselight = lerp(groundColor, skyColor, blend);
        
        float3 R = reflect(-V, N);
        blend = dot(R, up) * 0.5 + 0.5;
        float3 specularlight = lerp(groundColor, skyColor, blend);
        float3 kD = (float3(1, 1, 1) - F) * (1 - metallic);
        float3 Diffuse = kD * albedo * diffuselight;
        float3 Specular = F * specularlight;
        finalIBL = Diffuse + Specular;
    }
    else if (typeIBL == 2)
    {
        
        //return float4(roughness, roughness, roughness, 1.0f);
        
        float3 irradiance = iblIrradiance.Sample(samLinear, N).rgb;
        irradiance = (irradiance.x / 2, irradiance.y / 2, irradiance.z / 2);
        float3 kD = float3(1.0f, 1.0f, 1.0f) - F * (1.0f - metallic);
        float3 diffuseIBL = kD * albedo * irradiance;

        float3 R = reflect(-V, normalize(N));
        float prefilteredLod = roughness * maxReflectionLod;
        float3 prefilteredColor = iblSpecular.SampleLevel(samLinear, R, prefilteredLod).rgb;
        float2 BRDF = clamp(IntergrateBRDF(cosTheta, roughness),0.0f, 1.0f);
        float3 specularIBL = prefilteredColor * (F * BRDF.x + BRDF.y);
        //diffuseIBL *= 0.05; // i know this is wrong but it was way to bright
        finalIBL = diffuseIBL + specularIBL;
    }
    
    
    return float4(finalIBL + color, 1);
}

