///////////////////////////////////////////////////////////////////////////////
// Orkid
// Copyright 1996-2010, Michael T. Mayers
///////////////////////////////////////////////////////////////////////////////

uniform float4x4        MatMVP;
uniform float3          Levels;

uniform texture2D	MapA;
uniform texture2D	MapB;
uniform texture2D	MapC;

///////////////////////////////////////////////////////////////////////////////

sampler2D MapASampler = sampler_state
{
    Texture = <MapA>;
    MagFilter = Linear;
    MinFilter = Linear;
    MipFilter = Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
sampler2D MapBSampler = sampler_state
{
    Texture = <MapB>;
    MagFilter = Linear;
    MinFilter = Linear;
    MipFilter = Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
sampler2D MapCSampler = sampler_state
{
    Texture = <MapC>;
    MagFilter = Linear;
    MinFilter = Linear;
    MipFilter = Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};

///////////////////////////////////////////////////////////////////////////////

struct ColorVertex
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 Uv0 : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////

struct Fragment
{
    float4 ClipPos : POSITION;
    float4 Color   : COLOR;
    float2 Uv0 : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////

float4 PSFragColor( Fragment FragIn ) : COLOR
{
    float4 PixOut;
    PixOut = float4( FragIn.Uv0, 0.0f, 1.0f);
    return PixOut;
}

float max4( float4 inp )
{
	return max(inp.w,max(inp.z,max(inp.x,inp.y)));
}

float4 PSBoverAplusC( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;//*float2(1.0f,-1.0f);
	float4 A = (tex2D( MapASampler, uv ))*Levels.x;
	float4 B = (tex2D( MapBSampler, uv ))*Levels.y;
	float4 C = (tex2D( MapCSampler, uv ))*Levels.z;
	float fl = max4(B);
    PixOut = lerp(A,B,fl)+C;
    return PixOut;
}
float4 PSAplusBplusC( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;//*float2(1.0f,-1.0f);
	float4 A = (tex2D( MapASampler, uv ))*Levels.x;
	float4 B = (tex2D( MapBSampler, uv ))*Levels.y;
	float4 C = (tex2D( MapCSampler, uv ))*Levels.z;
    PixOut = A+B+C;
    return PixOut;
}
float4 PSAlerpBwithC( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;//*float2(1.0f,-1.0f);
	float4 A = tex2D( MapASampler, uv )*Levels.x;
	float4 B = tex2D( MapBSampler, uv )*Levels.y;
	float4 C = tex2D( MapCSampler, uv )*Levels.z;
	float fl = max4(C);
    PixOut = lerp(A,B,C);
    return PixOut;
}

float4 PSA( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;
	float4 A = tex2D( MapASampler, uv );
    PixOut = A;
    return PixOut;
}
float4 PSB( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;
	float4 B = tex2D( MapBSampler, uv );
    PixOut = B;
    return PixOut;
}
float4 PSC( Fragment FragIn ) : COLOR
{
    float4 PixOut;
	float2 uv = FragIn.Uv0;
	float4 C = tex2D( MapCSampler, uv );
    PixOut = C;
    return PixOut;
}

///////////////////////////////////////////////////////////////////////////////

Fragment VSVtxColor( ColorVertex VtxIn )
{
    Fragment FragOut;
    float2 outuv = VtxIn.Uv0.xy;
    FragOut.ClipPos = mul( float4( VtxIn.Position, 1.0f ), MatMVP );
    FragOut.Color = VtxIn.Color;
    FragOut.Uv0.x = outuv.x;
    FragOut.Uv0.y = 1.0f-outuv.y;
    return FragOut;
}

///////////////////////////////////////////////////////////////////////////////

technique BoverAplusC
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSBoverAplusC();
	}
}

technique AplusBplusC
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSAplusBplusC();
	}
}

technique AlerpBwithC
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSAlerpBwithC();
	}
}

technique Asolo
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSA();
	}
}
technique Bsolo
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSB();
	}
}
technique Csolo
{
	pass p0
	{
		VertexShader = compile vs_3_0 VSVtxColor();
		PixelShader = compile ps_3_0 PSC();
	}
}



