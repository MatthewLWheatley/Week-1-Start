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

SamplerState samLinear : register(s0); // Texture sampler for linear filtering

static const float PI = 3.14159265f; // Value of PI (used for angle calculations)

// Maximum number of lights supported
#define MAX_LIGHTS 10

// Light types (for future expansion to support more light types)
#define DIRECTIONAL_LIGHT 
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

static const float maxReflectionLod = 10;

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
    PS_INPUT output = (PS_INPUT) 0;
    
    // Transform the vertex position from object space to clip space (world -> view -> projection)
    output.Pos = mul(input.Pos, World);
    output.worldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    // Transform the normal vector from object space to world space
    output.Norm = mul(float4(input.Norm, 0), World).xyz;

    output.Tex = input.Tex; // Pass the texture coordinates along

    return output;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float NormalDistrobution(float roughness, float3 N, float3 H)
{
    float NdotH = max(dot(N, H), 0.0);
    float a2 = roughness * roughness;
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
    float3 finalColour = float4(0, 0, 0, 0);
    float3 albedo = albedoMap.Sample(samLinear, IN.Tex);
    float metallic = MetallicMap.Sample(samLinear, IN.Tex).r;
    float roughness = RoughnessMap.Sample(samLinear, IN.Tex).r;
    //float3 albedo = frank.xyz;
    //float metallic = 0.0;
    //float roughness = 0.01;
    
    float3 N = normalize(IN.Norm);
    float3 V = normalize(EyePosition - IN.worldPos);
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
        float3 L = normalize(Lights[i].Position - IN.worldPos);
        float3 H = normalize(V + L);
    
        float NdotL = max(dot(N, L), 0.0);
    

        // part A
        float D = NormalDistrobution(roughness, N, H);
        float G = G_sub(roughness, N, V) * G_sub(roughness, N, L);
        float3 BRDF = BRDFFunction(cosTheta, NdotL, F, D, G);
        float3 Diffuse = diffuse(albedo, F, metallic);
    
        float3 Lo = (Diffuse + BRDF) * NdotL;
        
        float3 L2 = Lights[i].Position - IN.worldPos;
        float distance = length(L2);
        float attenuation = 1.0 / (Lights[i].ConstantAttenuation + Lights[i].LinearAttenuation * distance + Lights[i].QuadraticAttenuation * (distance * distance));
        Lo = Lo * (Lights[i].Color.xyz * attenuation);
        
        color = color + Lo;
    }
    
    
    float3 finalIBL = float3(0, 0, 0);
    int typeIBL = 2;

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
        float3 irradiance = iblIrradiance.Sample(samLinear, N).rgb;
        float3 kD = (float3(1, 1, 1) - F) * (1 - metallic);
        float3 diffuseIBL = kD * albedo * irradiance;

        float3 R = reflect(-V, N);
        float prefilteredLod = roughness * maxReflectionLod;
        float3 prefilteredColor = iblSpecular.SampleLevel(samLinear, R, prefilteredLod).rgb;
        float2 BRDF = IntergrateBRDF(cosTheta, roughness);
        float3 specularIBL = prefilteredColor * (F * BRDF.x + BRDF.y);
        finalIBL = diffuseIBL + specularIBL;
    }
    
    
    return float4(finalIBL + color, 1);
}

float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor2; // Return the solid color (set in constant buffer)
}