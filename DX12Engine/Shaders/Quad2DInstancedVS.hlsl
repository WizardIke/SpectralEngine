//#define USE_PER_VERTEX_COLOR
//#define USE_TEXTURE

struct Input
{
    float4 pos : POSITION; //stores position x, position y, width, and height
#ifdef USE_TEXTURE
    float4 texCoords : TEXCOORD;
#endif
#ifdef USE_PER_VERTEX_COLOR
    float4 color : COLOR;
#endif
    uint vertexID : SV_VertexID;
};

struct Output
{
    float4 pos : SV_POSITION;
#ifdef USE_PER_VERTEX_COLOR
    float4 color : COLOR;
#endif
#ifdef USE_TEXTURE
    float2 texCoords : TEXCOORD;
#endif
};

Output main(Input input)
{
    Output output;

    // vert id 0 = 0000, uv = (0, 0)
    // vert id 1 = 0001, uv = (1, 0)
    // vert id 2 = 0010, uv = (0, 1)
    // vert id 3 = 0011, uv = (1, 1)
    float2 uv = float2(input.vertexID & 1, (input.vertexID >> 1) & 1);

    output.pos = float4(input.pos.x + (input.pos.z * uv.x), input.pos.y - (input.pos.w * uv.y), 0.0f, 1.0f);
#ifdef USE_PER_VERTEX_COLOR
    output.color = input.color;
#endif
#ifdef USE_TEXTURE
    output.texCoords = float2(input.texCoords.x + (input.texCoords.z * uv.x), input.texCoords.y + (input.texCoords.w * uv.y));
#endif

    return output;
}