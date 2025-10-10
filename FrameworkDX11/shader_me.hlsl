//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World; // World transformation matrix (object to world space)
    matrix View; // Camera view matrix (world to view space)
    matrix Projection; // Camera projection matrix (view to clip space)
    float4 vOutputColor; // Color to be used as output color (e.g., for solid color rendering)
    float TextureSelector; // Texture selector (0 = albedo, 1 = normal)
    float3 padding; // Padding for alignment
}

cbuffer ConstantBuffer : register(b2)
{
    float4 vOutputColor2; // Color to be used as output color (e.g., for solid color rendering)
}

Texture2D albedoMap : register(t0); // Albedo (diffuse) texture
Texture2D MetallicMap : register(t1); // Normal map for lighting effects
Texture2D RoughnessMap : register(t2); // Roughness map (PBR)
SamplerState samLinear : register(s0); // Texture sampler for linear filtering

static const float PI = 3.14159265f; // Value of PI (used for angle calculations)

// Maximum number of lights supported
#define MAX_LIGHTS 1

// Light types (for future expansion to support more light types)
#define DIRECTIONAL_LIGHT 
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

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

// Function for diffuse lighting calculation
float4 DoDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(0, dot(N, L)); // Compute dot product between normal and light direction
    float diffuseIntensity = max(0, dot(N, L));
    
    // Adjust diffuse intensity based on the dot product value
    if (diffuseIntensity > 0.8)
        diffuseIntensity = 1.0;
    else if (diffuseIntensity > 0.4)
        diffuseIntensity = 0.6;
    else
        diffuseIntensity = 0.2;

    return light.Color * NdotL; // Apply light color and intensity
}

// Function for specular lighting calculation
float4 DoSpecular(Light lightObject, float3 pixelToEyeVectorNormalised, float3 pixelToLightVectorNormalised, float3 Normal)
{
    float lightIntensity = saturate(dot(Normal, pixelToLightVectorNormalised)); // Compute intensity
    float4 specular = float4(0, 0, 0, 0);

    if (lightIntensity > 0.0f)
    {
        // Compute reflection vector and apply specular calculation
        float3 reflection = reflect(-pixelToLightVectorNormalised, Normal);
        specular = pow(saturate(dot(reflection, pixelToEyeVectorNormalised)), 4); // Specular power of 32
    }

    return specular; // Return specular light component
}

// Function for light attenuation based on distance
float DoAttenuation(Light light, float d)
{
    return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}

// Structure to store lighting results (diffuse and specular)
struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

// Function to compute lighting for point lights
LightingResult DoPointLight(Light light, float3 pixelToLightVectorNormalised, float3 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N)
{
    LightingResult result;

    // Apply attenuation to light
    float attenuation = DoAttenuation(light, distanceFromPixelToLight);
    attenuation = 1; // (Disabling attenuation for now)

    result.Diffuse = DoDiffuse(light, pixelToLightVectorNormalised, N) * attenuation; // Diffuse lighting
    result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, pixelToLightVectorNormalised, N) * attenuation; // Specular lighting

    return result;
}

// Function to compute lighting for all lights
LightingResult ComputeLighting(float4 pixelToLightVectorNormalised, float4 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N)
{
    LightingResult totalResult = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }; // Initialize lighting results

    // Iterate through all lights (up to MAX_LIGHTS)
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        LightingResult result = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

        if (!Lights[i].Enabled) // Skip disabled lights
            continue;

        // Compute lighting for this light
        result = DoPointLight(Lights[i], pixelToLightVectorNormalised.xyz, pixelToEyeVectorNormalised.xyz, distanceFromPixelToLight, N);
        
        totalResult.Diffuse += result.Diffuse; // Add diffuse component
        totalResult.Specular += result.Specular; // Add specular component
    }

    // Saturate the lighting results to prevent overexposure
    totalResult.Diffuse = saturate(totalResult.Diffuse);
    totalResult.Specular = saturate(totalResult.Specular);

    return totalResult; // Return final lighting results
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------
// Pixel Shader: PBR Rendering
//--------------------------------------------------------------------------------------
float4 PS_PBR(PS_INPUT IN) : SV_TARGET
{
    float3 finalColour = float4(0, 0, 0, 0);
    float3 albedo = albedoMap.Sample(samLinear, IN.Tex);
    float metallic = MetallicMap.Sample(samLinear, IN.Tex).r;
    float roughness = RoughnessMap.Sample(samLinear, IN.Tex).r;
    
    float3 N = normalize(IN.Norm);
    float3 V = normalize(EyePosition - IN.worldPos);
    float3 L = normalize(Lights[0].Position - IN.worldPos); 
    float3 H = normalize(V + L); 
    
    float cosTheta = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    float3 F0 = float3(0.04, 0.04, 0.04); 
    F0 = lerp(F0, albedo, metallic);

    float3 F = FresnelSchlick(cosTheta, F0);
    float D = NormalDistrobution(roughness, N, H);
    float G = G_sub(roughness, N, V) * G_sub(roughness, N, L);
    float3 bottom = (4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0));
    float3 BRDF = (F * G * D) / bottom;
    
    float3 kD = (float3(1, 1, 1) - F) * (1 - metallic);
    float3 Diffuse = kD * (albedo / PI);
    
    float3 color = (Diffuse + BRDF) * NdotL;
    
    color = color * Lights[0].Color.xyz;
    
    float3 ambient = float3(0.01, 0.01, 0.01) * albedo;
    
    return float4(color + ambient,1);
}


//--------------------------------------------------------------------------------------
// Pixel Shader: Normal Map (Lighting Calculation)
//--------------------------------------------------------------------------------------
float4 PS_Normal(PS_INPUT IN) : SV_TARGET
{
    // Compute vectors from pixel to light and from pixel to eye (camera)
    float4 pixelToLightVectorNormalised = normalize(Lights[0].Position - IN.worldPos);
    float4 pixelToEyeVectorNormalised = normalize(EyePosition - IN.worldPos);
    float distanceFromPixelToLight = length(IN.worldPos - Lights[0].Position); // Calculate distance

    // Perform lighting computation
    LightingResult lit = ComputeLighting(pixelToLightVectorNormalised, pixelToEyeVectorNormalised, distanceFromPixelToLight, normalize(IN.Norm));

    // Ambient light color (could be set globally)
    float4 ambient = GlobalAmbient;
    
    // Add diffuse and specular components from lighting
    float4 diffuse = lit.Diffuse;
    float4 specular = lit.Specular;

    // DEBUG: Use solid colors to test TextureSelector
    float4 texColor = float4(1, 1, 1, 1); // Default white
    if (TextureSelector < 0.5)
    {
        texColor = albedoMap.Sample(samLinear, IN.Tex); // Albedo texture
    }
    else
    {
        texColor = MetallicMap.Sample(samLinear, IN.Tex); // Normal texture
    }

    // Compute final color (diffuse + specular)
    float4 diffuseColor = (ambient + diffuse + specular) * texColor;

    return diffuseColor; // Return final color
}

//--------------------------------------------------------------------------------------
// Pixel Shader: Solid Color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor2; // Return the solid color (set in constant buffer)
}