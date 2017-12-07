#define MAX_TEXTURES 20

#define RootSignature1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | " \
			" DENY_HULL_SHADER_ROOT_ACCESS | " \
			" DENY_DOMAIN_SHADER_ROOT_ACCESS | " \
		    " DENY_GEOMETRY_SHADER_ROOT_ACCESS); " \
            " CBV( b0, space = 0, visibility = SHADER_VISIBILITY_VERTEX ) " \
            " CBV( b1, space = 0, visibility = SHADER_VISIBILITY_PIXEL ) " \
            " DescriptorTable(SRV(t0, numDescriptors = MAX_TEXTURES, visibility = SHADER_VISIBILITY_VERTEX )) " \
            " StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_POINT, addressU = TEXTURE_ADDRESS_BORDER, addressV = TEXTURE_ADDRESS_BORDER, addressW = TEXTURE_ADDRESS_BORDER, mipLODBias = 0.f, maxAnisotropy = 0, " \
               " comparisonFunc = COMPARISON_NEVER, borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,  minLOD = 0.f, maxLOD = 3.402823466e+38f, space = 0,  visibility = SHADER_VISIBILITY_PIXEL ) " \
            " StaticSampler(s1, filter=FILTER_MIN_MAG_LINEAR_MIP_POINT, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, mipLODBias = 0.f, " \
                " maxAnisotropy = 0, comparisonFunc = COMPARISON_NEVER, borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,  minLOD = 0.f, maxLOD = 3.402823466e+38f, space = 0, " \
                "  visibility = SHADER_VISIBILITY_PIXEL ) " \
            " StaticSampler(s2, filter=FILTER_MIN_MAG_MIP_LINEAR, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, mipLODBias = 0.f, maxAnisotropy = 0, " \
               " comparisonFunc = COMPARISON_LESS_EQUAL, borderColor = STATIC_BORDER_COLOR_OPAQUE_WHITE,  minLOD = 0.f, maxLOD = 3.402823466e+38f, space = 0,  visibility = SHADER_VISIBILITY_PIXEL ) "