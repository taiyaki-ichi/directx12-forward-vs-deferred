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

float3 CalcPointLight(float3 pos,float3 normal,float3 ray)
{
	float3 result = float3(0.f, 0.f, 0.f);

	for (int i = 0; i < pointLightNum; i++)
	{
		float3 lightDir = normalize(pos - pointLightData[i].pos.xyz);
		float distance = length(pos.xyz - pointLightData[i].pos.xyz);

		// ‰e‹¿”ÍˆÍ
		float range = 20.f;

		// ‰e‹¿—¦
		float affect = 1.f - min(1.f, distance / range);

		result += CalcDiffuse(lightDir, pointLightData[i].color, normal) * affect;
		result += CalcSpecular(lightDir, pointLightData[i].color, normal, ray, 100.f) * affect;
	}

	return result;
}

float4 main(VSOutput input) : SV_TARGET
{
	float3 lightDir = normalize(float3(1.f,0.f,1.f));
	float3 lightColor = float3(0.2f, 0.2f, 0.2f);

	float3 ambient = float3(1.f, 1.f, 1.f) * 0.4f;
	float3 diffuse = CalcDiffuse(lightDir, lightColor, input.norm.xyz);
	float3 specular = CalcSpecular(lightDir, lightColor, input.norm.xyz, input.ray, 100.f);

	float3 pointLightColor = CalcPointLight(input.worldPos.xyz, input.norm, input.ray);

	return float4(ambient + diffuse + specular + pointLightColor, 1.0f);
}