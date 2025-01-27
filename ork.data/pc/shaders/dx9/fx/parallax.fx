///////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////

#include "tanspace.fxi"
#include "lighting_common.fxi"

//#define PERPIXDIFFUSE
//#define PERFTEST

///////////////////////////////////////////////////////////////////////////////

uniform float time : reltime;

///////////////////////////////////////////////////////////////////////////////
// artist parameters

uniform float3		DiffuseColor;
uniform float3		SpecularColor;

uniform float		UvScaleClr	;
uniform float		UvScaleNrm	;
uniform float		UvScaleSpc	;
uniform float		UvScaleEmi	;
uniform float		UvScaleHgt	;

uniform float		BumpLevA	;

uniform float		AmbientMix	;
uniform float		DiffuseMix	;
uniform float		SpecularMix	;
uniform float		EmissiveMix	;

uniform float		Shininess	;

uniform float		HeightBias	;
uniform float		HeightScale	;

//uniform float		dofscale	: scene_dofscale		<string scope="perframe";>;	

const float dofscale = 300.0f;

///////////////////////////////////////////////////////////////////////////////
// artist parameters

uniform texture2D ColorMap;
uniform texture2D NormalMap;
uniform texture2D HeightMap;
uniform texture2D SpecuMap;
uniform texture2D EmissiveMap;

sampler2D ColorMapSampler = sampler_state
{
	Texture = <ColorMap>;
    MagFilter = LINEAR;
    MinFilter = Anisotropic;
    MipFilter = LINEAR;
	MaxAnisotropy = 4.0f;
};

sampler2D HeightMapSampler = sampler_state
{
	Texture = <HeightMap>;
    MagFilter = LINEAR;
    MinFilter = Anisotropic;
    MipFilter = LINEAR;
	MaxAnisotropy = 4.0f;
};

sampler2D NormalMapSampler = sampler_state
{
	Texture = <NormalMap>;
    MagFilter = LINEAR;
    MinFilter = Anisotropic;
    MipFilter = LINEAR;
	MaxAnisotropy = 4.0f;
};

sampler2D SpecuMapSampler = sampler_state
{
	Texture = <SpecuMap>;
    MagFilter = LINEAR;
    MinFilter = Anisotropic;
    MipFilter = LINEAR;
	MaxAnisotropy = 4.0f;
};

sampler2D EmissiveMapSampler = sampler_state
{
	Texture = <EmissiveMap>;
    MagFilter = LINEAR;
    MinFilter = Anisotropic;
    MipFilter = LINEAR;
	MaxAnisotropy = 4.0f;
};

///////////////////////////////////////////////////////////////////////////////

struct Vertex {
    float4 Position : POSITION;   // in object space
    float4 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 Binormal : BINORMAL;
    float4 Color    : COLOR0;
};
struct LmVertex {
    float4 Position		: POSITION;   // in object space
    float4 TexCoord		: TEXCOORD0;
    float4 LightMapUv	: TEXCOORD1;
    float3 Normal		: NORMAL;
    float3 Binormal		: BINORMAL;
    float4 Color		: COLOR0;
};

///////////////////////////////////////////////////////////////////////////////

struct VertexSkinned
{
    float4 Position     : POSITION;
    float4 TexCoord     : TEXCOORD0;
    float3 Normal       : NORMAL;
    float3 Binormal		: BINORMAL;
    int4   BoneIndices	: BLENDINDICES;
    float4 BoneWeights	: BLENDWEIGHT;
    float4 Color		: COLOR0;
};

struct Fragment {

    float4 ClipPos			: POSITION;  // in clip space
    float4 Color			: COLOR;

    float3 UVW0             : TEXCOORD0;
    float3 UVW1             : TEXCOORD1;

    float3 TanLight         : TEXCOORD2;
    float3 TanHalf          : TEXCOORD3;
    float3 TanEye           : TEXCOORD4;
    float3 VtxTangent       : TEXCOORD5;
    float3 VtxSpecular	    : TEXCOORD6;
    float4 ND				: TEXCOORD7;
        
};
	
///////////////////////////////////////////////////////////////////////////////

Fragment vs_parallax_common(	float3 ObjPosition, 
								float3 ObjNormal,
								float3 ObjBinormal,
								float2 Uv0,
								float2 Uv1,
								float3 Diffuse,
								float3 Specular )
{
	Fragment FragOut;
	
    //////////////////////////////////////

    float3 worldPos = mul(float4(ObjPosition,1.0f),World).xyz;
                
    //////////////////////////////////////
         
    float3 worldNormal  = mul( ObjNormal, WorldRot3 );
    float3 worldEye     = GetEyePos();
    float3 worldToEye   = normalize( worldEye - worldPos );
    //float3 worldToLyt   = worldToEye;
     
    /////////////////////////////////////

    FragOut.Color.xyzw = float4(Diffuse,1.0f);
	FragOut.VtxSpecular = Specular;
	
    /////////////////////////////////////
    // ( Tangent Space Lighting )

	float3 nbin = ObjBinormal;
	float3 nnrm = ObjNormal;

	float3 Tangent = cross( nbin, nnrm );
	TanLytData tldata = GenTanLytData(	World,
										worldToEye, worldToEye,
										nnrm, nbin, Tangent
									 );

    FragOut.TanLight = tldata.TanLyt; 
    FragOut.TanHalf = tldata.TanHalf;
    FragOut.TanEye = tldata.TanEye;

    /////////////////////////////////////
    FragOut.UVW0.xyz = float3( Uv0.xy, 0.0f );
    FragOut.UVW1.xyz = float3( Uv1.xy, 0.0f );
    /////////////////////////////////////

	float4 ClipPos = mul(float4(ObjPosition,1.0f),WorldViewProjection);

    FragOut.VtxTangent = Tangent.xyz;

    FragOut.ClipPos = ClipPos;

	float dist = 1.0f-saturate( distance( worldEye, worldPos )/dofscale );

	FragOut.ND = float4( worldNormal, dist );
		
	return FragOut;
}

///////////////////////////////////////////////////////////////////////////////

Fragment vs_parallax_skinned( VertexSkinned VtxIn )
{
	float3 ObjectPos = SkinPosition4( VtxIn.BoneIndices, VtxIn.BoneWeights, VtxIn.Position );
	float3 ObjectNormal = SkinNormal4( VtxIn.BoneIndices, VtxIn.BoneWeights, VtxIn.Normal );
	float3 ObjectBinormal = SkinNormal4( VtxIn.BoneIndices, VtxIn.BoneWeights, VtxIn.Binormal );
    float3 worldPos = mul(float4(ObjectPos,1.0f),World).xyz;         
    float3 worldNormal  = normalize(mul( ObjectNormal, WorldRot3 ));

    float3 worldEye     = GetEyePos();

	float4 DotScaleBias = float4( 0.0f, 1.0f, 0.0f, 1.0f );
	DifAndSpec das = DiffuseSpecularLight( worldPos, worldNormal, worldEye, Shininess, DotScaleBias );
	
    Fragment frg = vs_parallax_common(	ObjectPos,
										ObjectNormal,
										ObjectBinormal,
										VtxIn.TexCoord.xy,
										float2(0.0f,0.0f),
										das.Diffuse*ModColor.xyz,
										das.Specular );
										
	return frg;
}

///////////////////////////////////////////////////////////////////////////////

Fragment vs_parallax( Vertex VtxIn )
{
    float3 worldPos = mul(float4(VtxIn.Position.xyz,1.0f),World).xyz;         
    float3 worldNormal  = mul( VtxIn.Normal.xyz, WorldRot3 );
	float3 dl = DiffuseLight( worldPos, worldNormal )*VtxIn.Color.xyz;//*ModColor.xyz;

	return vs_parallax_common( VtxIn.Position.xyz,
								 VtxIn.Normal.xyz,
								  VtxIn.Binormal.xyz,
								   VtxIn.TexCoord.xy,
								    float2(0.0f,0.0f),
								     dl,
								     float3(0.0f,0.0f,0.0f) );
}

///////////////////////////////////////////////////////////////////////////////

struct emc
{
	float3 emissive;
	float3 diffuse;
	float3 specular;
};

///////////////////////////////////////////////////////////////////////////////

emc ps_parallax_common( Fragment FragIn ) 
{	
	float3 dl = FragIn.Color.xyz;	

	/////////////////////////
	
	float2 UvC = FragIn.UVW0.xy*UvScaleClr;
	float2 UvN = FragIn.UVW0.xy*UvScaleNrm;
	float2 UvS = FragIn.UVW0.xy*UvScaleSpc;
	float2 UvE = FragIn.UVW0.xy*UvScaleEmi;
	float2 UvH = FragIn.UVW0.xy*UvScaleHgt;
	
	/////////////////////////
	// Compute Tangent Basis

	float3 TanEye = normalize( FragIn.TanEye );

	/////////////////////////
	// Get HeightMap
		
	float hs = HeightScale;
	float HeightVal = tex2D( HeightMapSampler , UvH ).z;
	float Offset = HeightBias + (hs*HeightVal);

	/////////////////////////
	// Parallax Mapping
	
	UvC = UvC + ((TanEye.xy*Offset)*float2(1.0f,-1.0f));
	
	//HeightVal += tex2D(HeightMapSampler, UvH ).z;
	//Offset = hs * (HeightVal - 1.0);
	//UvA = UvA + (TanEye.xy*Offset);

	/////////////////////////
	// Get Other Maps
	
	float3 nmap = tex2D( NormalMapSampler , UvN ).rgb;
	float3 cmap = tex2D( ColorMapSampler , UvC ).rgb;
	float3 smap = tex2D( SpecuMapSampler , UvS ).rgb;
	float3 emap = tex2D( EmissiveMapSampler , UvE ).rgb*EmissiveMix;

	/////////////////////////

	const float3 FlatNormal = float3( 0.5f, 0.5f, 1.0f );
	nmap = lerp( FlatNormal, nmap, BumpLevA );
	float3 TanNormal = normalize((nmap*2.0f)-float3(1.0f,1.0f,1.0f)); 

	/////////////////////////
	// Lighting Equation
	/////////////////////////
	
	//float3 tlight = normalize(FragIn.TanLight);
	//float fdotL = saturate(dot(TanNormal,tlight));
	//float fdotH = saturate(dot(TanNormal,normalize(FragIn.TanHalf)));
	
	/////////////////////////

	float3 DC = (AmbientMix*cmap)+(DiffuseMix*dl*cmap);
	float3 SC = FragIn.VtxSpecular*smap;
		
	emc rval;
	rval.emissive = emap;
	rval.diffuse = DC;
	rval.specular = SC;

    return rval;
}

float4 ps_parallax( Fragment FragIn ) : COLOR
{
	emc common = ps_parallax_common( FragIn );
	
	MrtPixel mrtout;

	mrtout.DiffuseBuffer = float4( common.emissive+common.diffuse+common.specular, 1.0f );
	//mrtout.DiffuseBuffer = float4( common.specular, 1.0f );
	mrtout.SpecularBuffer = float4( common.emissive+common.diffuse+common.specular, 1.0f );
	mrtout.NormalDepthBuffer = 	float4( FragIn.ND );
	
	//mrtout.DiffuseBuffer = float4(1.0f,0.0f,0.0f,1.0f);

	return mrtout.DiffuseBuffer;//+mrtout.SpecularBuffer;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Fragment vs_parallax_lm( LmVertex VtxIn )
{
	float3 dl = ModColor.xyz;

	return vs_parallax_common( 
						VtxIn.Position.xyz,
						VtxIn.Normal.xyz,
						VtxIn.Binormal.xyz,
						VtxIn.TexCoord.xy,
						VtxIn.LightMapUv.xy,
						dl,
						float3(0.0f,0.0f,0.0f) );
}

///////////////////////////////////////////////////////////////////////////////

MrtPixel ps_parallax_lm( Fragment FragIn ) 
{
	emc common = ps_parallax_common( FragIn );
	
	MrtPixel mrtout;
	float2 luv = FragIn.UVW1.xy;
	luv.y = 1.0f-luv.y;
	
	float3 lightcol = GetLightmap(luv.xy);

	float3 dif = (common.diffuse+common.emissive)*lightcol;
	float3 spc = common.specular*lightcol;
	
	mrtout.DiffuseBuffer = float4( dif+spc, 1.0f );
	mrtout.SpecularBuffer = float4( common.emissive+dif+common.specular, 1.0f );
	mrtout.NormalDepthBuffer = 	float4( FragIn.ND );

	return mrtout;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float4 ps_uvdebugclr( Fragment FragIn ) : COLOR
{
	/////////////////////////
	
	float2 UvA = FragIn.UVW0.xy*UvScaleClr;

	float r = fmod( UvA.r, 1.0f );
	float g = fmod( UvA.g, 1.0f );
	
	float4 mrtout = float4( r, g, 0.0f, 1.0f );
    return mrtout;
	
}    

///////////////////////////////////////////////////////////////////////////////

float4 ps_tanspacedebug( Fragment FragIn ) : COLOR
{
	float3 fragtan = -normalize(FragIn.VtxTangent).rgb;
	float4 mrtout = float4( fragtan, 1.0f );
    return mrtout;
	
}    

///////////////////////////////////////////////////////////////////////////////

technique FullLighting
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_parallax();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
};
technique FullLightingLightMapped
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax_lm();
		PixelShader = compile ps_3_0 ps_parallax_lm();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
};
technique FullLightingLightPreview
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_parallax();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
};
technique FullLightingVtxCol
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_parallax();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
};

technique FullLightingSkinned
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax_skinned();
		PixelShader = compile ps_3_0 ps_parallax();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
};


technique UvDebugClr
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_uvdebugclr();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
}

technique TanSpaceDebug
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_tanspacedebug();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
}


technique main
{
	pass p0
	{
		VertexShader = compile vs_3_0 vs_parallax();
		PixelShader = compile ps_3_0 ps_tanspacedebug();
		ZWriteEnable = true;
		ZFunc = LESS;
		ZEnable = TRUE;
	}
}
///////////////////////////////////////////////////////////////////////////////

#include "pick.fxi"

///////////////////////////////////////////////////////////////////////////////
