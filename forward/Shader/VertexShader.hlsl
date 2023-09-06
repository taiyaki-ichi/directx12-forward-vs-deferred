#include"Header.hlsli"

VSOutput main(float4 pos : POSITION, float4 norm : NORMAL)
{
	VSOutput output;
	output.pos = mul(mul(proj, view), mul(model, pos));
	output.norm = normalize(mul(model, float4(norm.xyz, 0.f)));
	output.ray = normalize(pos.xyz - eye);
	return output;
}