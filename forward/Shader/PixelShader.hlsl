#include"Header.hlsli"

float3 CalcDiffuse(float3 lightDir, float3 lightColor, float3 normal)
{
	float t = saturate(dot(normal, -lightDir));
	return lightColor * t;
}

float3 CalcSpecular(float3 lightDir, float3 lightColor, float3 normal, float3 toEye,float specPow)
{
	float3 r = reflect(lightDir, normal);
	float t = pow(saturate(dot(r, toEye)), specPow);
	return lightColor * t;
}

float4 main(VSOutput input) : SV_TARGET
{
	float3 lightDir = normalize(float3(1.f,0.f,1.f));
	float3 lightColor = float3(1.f, 0.f, 0.f);

	float3 ambient = lightColor * 0.2f;
	float3 diffuse = CalcDiffuse(lightDir, lightColor, input.norm.xyz);
	float3 specular = CalcSpecular(lightDir, lightColor, input.norm.xyz, input.ray, 100.f);

	return float4(ambient + diffuse + specular, 1.0f);
}