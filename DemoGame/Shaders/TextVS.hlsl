struct Input
{
    float4 pos : POSITION; //stores position x, position y, width, and height
    float4 texCoord : TEXCOORD;
    float4 color : COLOR;

    uint vertexID : SV_VertexID;
};

struct Output
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

Output main(Input input)
{
    Output output;

    // vert id 0 = 0000, uv = (0, 0)
    // vert id 1 = 0001, uv = (1, 0)
    // vert id 2 = 0010, uv = (0, 1)
    // vert id 3 = 0011, uv = (1, 1)
    float2 uv = float2(input.vertexID & 1, (input.vertexID >> 1) & 1);

    // set the position for the vertex based on which vertex it is (uv)
    output.pos = float4(input.pos.x + (input.pos.z * uv.x), input.pos.y - (input.pos.w * uv.y), 0.0f, 1.0f);
    output.color = input.color;

    // set the texture coordinate based on which vertex it is (uv)
    output.texCoord = float2(input.texCoord.x + (input.texCoord.z * uv.x), input.texCoord.y + (input.texCoord.w * uv.y));

    return output;
}