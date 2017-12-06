RWTexture2D<float4> backBuffer;
Texture2D<float> depthBuffer;

struct Shader
{
    uint id;
};

struct ShaderMapNode
{
    uint shaderCount;
    Shader shaders[15];
};

StructuredBuffer<ShaderMapNode> shaderMap;

struct ColorShaderNode
{
    uint endIndex;
    float2 position;
    float depth;
};

void main(float2 position : TEXCOORD0)
{
    uint screenWidth, screenHight;
    depthBuffer.GetDimensions(screenWidth, screenHight);

    uint lastRenderedColorIndex = 0u;
    uint currentColorIndex = 0u;
    float4 colors[10];
    ColorShaderNode colorShaders[10u];

    uint shaderMapIndex = position.y * screenWidth + position.x;
    uint shaderCount = shaderMap[shaderMapIndex].shaderCount;
    uint shaderIndex = 0u;
    float depth = depthBuffer.Load(shaderMapIndex);
    if (shaderCount != 0u)
    {
        while(true)
        {
            do
            {
                switch (shaderMap[shaderMapIndex].shaders[shaderIndex].id)
                {
                    case 0u: //reflect
                        colorShaders[currentColorIndex].endIndex = shaderIndex + 1u;
                        colorShaders[currentColorIndex].position = position;
                        colorShaders[currentColorIndex].depth = depth;
                        ++currentColorIndex;

                        //position = new position;
                        //depth = new depth;

                        shaderMapIndex = position.y * screenWidth + position.x;
                        shaderCount = shaderMap[shaderMapIndex].shaderCount;
                        shaderIndex = 1u;

                        break;
                    case 1u: //refract
                        //position = new position;
                        //depth = new depth;
                        ++shaderIndex;
                        break;
                    case 2u: //quit
                        return;
                    default :
                        ++shaderIndex;
                        break;
                }
            } while (shaderIndex != shaderCount);

            //depth test
             {
                float testDepth = depthBuffer.Load(shaderMapIndex);
                if (depth >= testDepth)
                {
                    //we have found a good pixel so render it
                    colorShaders[currentColorIndex].endIndex = shaderIndex + 1u;
                    for (uint j = lastRenderedColorIndex; j <= currentColorIndex; ++j)
                    {
                        uint endIndex = colorShaders[currentColorIndex].endIndex;
                        for (uint k = 0u; k < endIndex; ++k)
                        {
                            switch (shaderMap[shaderMapIndex].shaders[k].id)
                            {
                                case 3 : 
                                    //color[j] = ...
                                    break;
                            }
                        }
                    }
                    lastRenderedColorIndex = currentColorIndex;

                    //backBuffer[uint2(position.x, position.y)] = colors[currentColorIndex];
                }
                if (currentColorIndex == 0u)
                {
                    //all rendering has been done
                    return;
                }
                else
                {
                    --currentColorIndex;
                    --lastRenderedColorIndex;
                    position = colorShaders[currentColorIndex].position;
                    depth = colorShaders[currentColorIndex].depth;
                    shaderIndex = colorShaders[currentColorIndex].endIndex;


                    shaderMapIndex = position.y * screenWidth + position.x;
                    shaderCount = shaderMap[shaderMapIndex].shaderCount;
                }
            }
        }
    }
}