struct VSInput {
    float2 inPosition;
    float4 inColor;
};

struct VSOutput {
    float4 pos       : SV_Position;
    float  pointSize : SV_PointSize;
    float3 color     : COLOR0;
};

struct PSInput {
    float4 pos        : SV_Position;
    float3 fragColor  : COLOR0;
    float2 pointCoord : SV_PointCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pointSize = 14.0;
    output.pos       = float4(input.inPosition, 1.0, 1.0);
    output.color     = input.inColor.rgb;

    return output;
}

[shader("fragment")]
float4 fragMain(PSInput input) : SV_TARGET {
    float2 coord = input.pointCoord - float2(0.5);
    return float4(input.fragColor, 0.5 - length(coord));
}