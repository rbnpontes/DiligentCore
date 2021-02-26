/*
 *  Copyright 2019-2021 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

#include "TestingEnvironment.hpp"
#include "GraphicsAccessories.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

static const char g_TrivialVSSource[] = R"(
void main(out float4 pos : SV_Position)
{
    pos = float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char g_TrivialPSSource[] = R"(
float4 main() : SV_Target
{
    return float4(0.0, 0.0, 0.0, 0.0);
}
)";

static const char g_TrivialCSSource[] = R"(
[numthreads(8,8,1)]
void main()
{
}
)";

class PSOCreationFailureTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* const pEnv    = TestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        ShaderCreateInfo Attrs;
        Attrs.Source                     = g_TrivialVSSource;
        Attrs.EntryPoint                 = "main";
        Attrs.Desc.ShaderType            = SHADER_TYPE_VERTEX;
        Attrs.Desc.Name                  = "TrivialVS (PSOCreationFailureTest)";
        Attrs.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        Attrs.ShaderCompiler             = pEnv->GetDefaultCompiler(Attrs.SourceLanguage);
        Attrs.UseCombinedTextureSamplers = true;
        pDevice->CreateShader(Attrs, &sm_pTrivialVS);
        ASSERT_TRUE(sm_pTrivialVS);

        Attrs.Source          = g_TrivialPSSource;
        Attrs.Desc.ShaderType = SHADER_TYPE_PIXEL;
        Attrs.Desc.Name       = "TrivialPS (PSOCreationFailureTest)";
        pDevice->CreateShader(Attrs, &sm_pTrivialPS);
        ASSERT_TRUE(sm_pTrivialPS);

        Attrs.Source          = g_TrivialCSSource;
        Attrs.Desc.ShaderType = SHADER_TYPE_COMPUTE;
        Attrs.Desc.Name       = "TrivialCS (PSOCreationFailureTest)";
        pDevice->CreateShader(Attrs, &sm_pTrivialCS);
        ASSERT_TRUE(sm_pTrivialCS);

        sm_DefaultGraphicsPsoCI.PSODesc.Name                      = "PSOCreationFailureTest - default graphics PSO desc";
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.NumRenderTargets = 1;
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM;
        sm_DefaultGraphicsPsoCI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_D32_FLOAT;

        sm_DefaultGraphicsPsoCI.pVS = sm_pTrivialVS;
        sm_DefaultGraphicsPsoCI.pPS = sm_pTrivialPS;

        {
            RefCntAutoPtr<IPipelineState> pGraphicsPSO;
            pDevice->CreateGraphicsPipelineState(GetGraphicsPSOCreateInfo("PSOCreationFailureTest - OK graphics PSO"), &pGraphicsPSO);
            ASSERT_TRUE(pGraphicsPSO);
        }

        sm_DefaultComputePsoCI.PSODesc.Name = "PSOCreationFailureTest - default compute PSO desc";
        sm_DefaultComputePsoCI.pCS          = sm_pTrivialCS;

        {
            RefCntAutoPtr<IPipelineState> pComputePSO;
            pDevice->CreateComputePipelineState(GetComputePSOCreateInfo("PSOCreationFailureTest - OK compute PSO"), &pComputePSO);
            ASSERT_TRUE(pComputePSO);
        }

        RenderPassDesc RPDesc;
        RPDesc.Name = "PSOCreationFailureTest - render pass";
        RenderPassAttachmentDesc Attachments[2]{};
        Attachments[0].Format       = TEX_FORMAT_RGBA8_UNORM;
        Attachments[0].InitialState = RESOURCE_STATE_RENDER_TARGET;
        Attachments[0].FinalState   = RESOURCE_STATE_RENDER_TARGET;
        Attachments[1].Format       = TEX_FORMAT_D32_FLOAT;
        Attachments[1].InitialState = RESOURCE_STATE_DEPTH_WRITE;
        Attachments[1].FinalState   = RESOURCE_STATE_DEPTH_WRITE;
        RPDesc.AttachmentCount      = _countof(Attachments);
        RPDesc.pAttachments         = Attachments;

        AttachmentReference ColorAttachmentRef{0, RESOURCE_STATE_RENDER_TARGET};
        AttachmentReference DepthAttachmentRef{1, RESOURCE_STATE_DEPTH_WRITE};
        SubpassDesc         Subpasses[1]{};
        Subpasses[0].RenderTargetAttachmentCount = 1;
        Subpasses[0].pRenderTargetAttachments    = &ColorAttachmentRef;
        Subpasses[0].pDepthStencilAttachment     = &DepthAttachmentRef;

        RPDesc.SubpassCount = _countof(Subpasses);
        RPDesc.pSubpasses   = Subpasses;

        pDevice->CreateRenderPass(RPDesc, &sm_pRenderPass);
        ASSERT_TRUE(sm_pRenderPass);

        {
            RefCntAutoPtr<IPipelineState> pGraphicsPSO;
            pDevice->CreateGraphicsPipelineState(GetGraphicsPSOCreateInfo("PSOCreationFailureTest - OK PSO with render pass", true), &pGraphicsPSO);
            ASSERT_TRUE(pGraphicsPSO);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            ImmutableSamplerDesc ImmutableSmplers[] //
                {
                    ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}} //
                };

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                 = "PRS0";
            PRSDesc.NumResources         = _countof(Resources);
            PRSDesc.Resources            = Resources;
            PRSDesc.NumImmutableSamplers = _countof(ImmutableSmplers);
            PRSDesc.ImmutableSamplers    = ImmutableSmplers;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature0);
            ASSERT_TRUE(sm_pSignature0);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture2", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS0A";
            PRSDesc.NumResources = _countof(Resources);
            PRSDesc.Resources    = Resources;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature0A);
            ASSERT_TRUE(sm_pSignature0A);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name         = "PRS1";
            PRSDesc.BindingIndex = 1;
            PRSDesc.NumResources = _countof(Resources);
            PRSDesc.Resources    = Resources;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature1);
            ASSERT_TRUE(sm_pSignature1);
        }

        {
            PipelineResourceDesc Resources[] = //
                {
                    PipelineResourceDesc{SHADER_TYPE_GEOMETRY, "g_Texture", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE} //
                };
            ImmutableSamplerDesc ImmutableSmplers[] //
                {
                    ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture_sampler", SamplerDesc{}} //
                };

            PipelineResourceSignatureDesc PRSDesc;
            PRSDesc.Name                 = "PRS1A";
            PRSDesc.BindingIndex         = 1;
            PRSDesc.NumResources         = _countof(Resources);
            PRSDesc.Resources            = Resources;
            PRSDesc.NumImmutableSamplers = _countof(ImmutableSmplers);
            PRSDesc.ImmutableSamplers    = ImmutableSmplers;
            pDevice->CreatePipelineResourceSignature(PRSDesc, &sm_pSignature1A);
            ASSERT_TRUE(sm_pSignature1A);
        }
    }

    static void TearDownTestSuite()
    {
        sm_pTrivialVS.Release();
        sm_pTrivialPS.Release();
        sm_pTrivialCS.Release();
        sm_pRenderPass.Release();
        sm_pSignature0.Release();
        sm_pSignature0A.Release();
        sm_pSignature1.Release();
        sm_pSignature1A.Release();
    }

    static GraphicsPipelineStateCreateInfo GetGraphicsPSOCreateInfo(const char* Name, bool UseRenderPass = false)
    {
        auto CI{sm_DefaultGraphicsPsoCI};
        CI.PSODesc.Name = Name;
        if (UseRenderPass)
        {
            CI.GraphicsPipeline.NumRenderTargets = 0;
            CI.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.DSVFormat        = TEX_FORMAT_UNKNOWN;
            CI.GraphicsPipeline.pRenderPass      = sm_pRenderPass;
        }
        return CI;
    }
    static ComputePipelineStateCreateInfo GetComputePSOCreateInfo(const char* Name)
    {
        auto CI{sm_DefaultComputePsoCI};
        CI.PSODesc.Name = Name;
        return CI;
    }
    static IShader* GetVS() { return sm_pTrivialVS; }
    static IShader* GetPS() { return sm_pTrivialPS; }

    static void TestCreatePSOFailure(GraphicsPipelineStateCreateInfo CI, const char* ExpectedErrorSubstring)
    {
        auto* const pEnv    = TestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        RefCntAutoPtr<IPipelineState> pPSO;
        pEnv->SetErrorAllowance(2, "Errors below are expected: testing PSO creation failure\n");
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateGraphicsPipelineState(CI, &pPSO);

        CI.PSODesc.Name = nullptr;
        pEnv->SetErrorAllowance(2);
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateGraphicsPipelineState(CI, &pPSO);

        pEnv->SetErrorAllowance(0);
        ASSERT_FALSE(pPSO);
    }

    static void TestCreatePSOFailure(ComputePipelineStateCreateInfo CI, const char* ExpectedErrorSubstring)
    {
        auto* const pEnv    = TestingEnvironment::GetInstance();
        auto* const pDevice = pEnv->GetDevice();

        RefCntAutoPtr<IPipelineState> pPSO;

        pEnv->SetErrorAllowance(2, "Errors below are expected: testing PSO creation failure\n");
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateComputePipelineState(CI, &pPSO);

        CI.PSODesc.Name = nullptr;
        pEnv->SetErrorAllowance(2);
        pEnv->PushExpectedErrorSubstring(ExpectedErrorSubstring);
        pDevice->CreateComputePipelineState(CI, &pPSO);

        pEnv->SetErrorAllowance(0);
        ASSERT_FALSE(pPSO);
    }

protected:
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature0;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature0A;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature1;
    static RefCntAutoPtr<IPipelineResourceSignature> sm_pSignature1A;

private:
    static RefCntAutoPtr<IShader>     sm_pTrivialVS;
    static RefCntAutoPtr<IShader>     sm_pTrivialPS;
    static RefCntAutoPtr<IShader>     sm_pTrivialCS;
    static RefCntAutoPtr<IRenderPass> sm_pRenderPass;

    static GraphicsPipelineStateCreateInfo sm_DefaultGraphicsPsoCI;
    static ComputePipelineStateCreateInfo  sm_DefaultComputePsoCI;
};

RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialVS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialPS;
RefCntAutoPtr<IShader>                    PSOCreationFailureTest::sm_pTrivialCS;
RefCntAutoPtr<IRenderPass>                PSOCreationFailureTest::sm_pRenderPass;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature0;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature0A;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature1;
RefCntAutoPtr<IPipelineResourceSignature> PSOCreationFailureTest::sm_pSignature1A;

GraphicsPipelineStateCreateInfo PSOCreationFailureTest::sm_DefaultGraphicsPsoCI;
ComputePipelineStateCreateInfo  PSOCreationFailureTest::sm_DefaultComputePsoCI;

TEST_F(PSOCreationFailureTest, InvalidGraphicsPipelineType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Graphics Pipeline Type")};
    PsoCI.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    TestCreatePSOFailure(PsoCI, "Pipeline type must be GRAPHICS or MESH");
}

TEST_F(PSOCreationFailureTest, NoVS)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - no VS")};
    PsoCI.pVS = nullptr;
    TestCreatePSOFailure(PsoCI, "Vertex shader must not be null");
}

TEST_F(PSOCreationFailureTest, IncorrectVSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect VS Type")};
    PsoCI.pVS = GetPS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_PIXEL is not a valid type for vertex shader");
}

TEST_F(PSOCreationFailureTest, IncorrectPSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect PS Type")};
    PsoCI.pPS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for pixel shader");
}

TEST_F(PSOCreationFailureTest, IncorrectGSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect GS Type")};
    PsoCI.pGS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for geometry shader");
}

TEST_F(PSOCreationFailureTest, IncorrectDSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect DS Type")};
    PsoCI.pDS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for domain shader");
}

TEST_F(PSOCreationFailureTest, IncorrectHSType)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - incorrect HS Type")};
    PsoCI.pHS = GetVS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_VERTEX is not a valid type for hull shader");
}

TEST_F(PSOCreationFailureTest, WrongSubpassIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - wrong subpass index")};
    PsoCI.GraphicsPipeline.SubpassIndex = 1;
    TestCreatePSOFailure(PsoCI, "Subpass index (1) must be 0");
}

TEST_F(PSOCreationFailureTest, UndefinedFillMode)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Undefined Fill Mode")};
    PsoCI.GraphicsPipeline.RasterizerDesc.FillMode = FILL_MODE_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "RasterizerDesc.FillMode must not be FILL_MODE_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, UndefinedCullMode)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Undefined Cull Mode")};
    PsoCI.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "RasterizerDesc.CullMode must not be CULL_MODE_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDepthFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Depth Func")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.DepthFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable           = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable          = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilDepthFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilDepthFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable                = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilDepthFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilDepthFailOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilDepthFailOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable               = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilDepthFailOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilPassOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilPassOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable           = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilPassOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilPassOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilPassOp")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable          = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = STENCIL_OP_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilPassOp must not be STENCIL_OP_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidFrontStencilFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Front Face StencilFunc")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable         = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.FrontFace.StencilFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidBackStencilFunc)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid Back Face StencilFunc")};
    PsoCI.GraphicsPipeline.DepthStencilDesc.StencilEnable        = True;
    PsoCI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = COMPARISON_FUNC_UNKNOWN;
    TestCreatePSOFailure(PsoCI, "DepthStencilDesc.BackFace.StencilFunc must not be COMPARISON_FUNC_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, InvalidSrcBlend)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid SrcBlend")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend    = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].SrcBlend must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDestBlend)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid DestBlend")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend   = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].DestBlend must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBlendOp)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid BlendOp")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp     = BLEND_OPERATION_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].BlendOp must not be BLEND_OPERATION_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidSrcBlendAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid SrcBlendAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable   = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].SrcBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidDestBlendAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid DestBlendAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable    = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = BLEND_FACTOR_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].DestBlendAlpha must not be BLEND_FACTOR_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, InvalidBlendOpAlpha)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Invalid BlendOpAlpha")};
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable  = True;
    PsoCI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = BLEND_OPERATION_UNDEFINED;
    TestCreatePSOFailure(PsoCI, "BlendDesc.RenderTargets[0].BlendOpAlpha must not be BLEND_OPERATION_UNDEFINED");
}

TEST_F(PSOCreationFailureTest, OverlappingVariableStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Overlapping Variable Stages")};

    ShaderResourceVariableDesc Variables[] //
        {
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
            ShaderResourceVariableDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_STATIC} //
        };
    PsoCI.PSODesc.ResourceLayout.Variables    = Variables;
    PsoCI.PSODesc.ResourceLayout.NumVariables = _countof(Variables);
    TestCreatePSOFailure(PsoCI, "'g_Texture' is defined in overlapping shader stages (SHADER_TYPE_VERTEX, SHADER_TYPE_GEOMETRY and SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL)");
}

TEST_F(PSOCreationFailureTest, OverlappingImmutableSamplerStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Overlapping Immutable Sampler Stages")};

    ImmutableSamplerDesc ImtblSamplers[] //
        {
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "g_Texture_sampler", SamplerDesc{}},
            ImmutableSamplerDesc{SHADER_TYPE_VERTEX | SHADER_TYPE_GEOMETRY, "g_Texture_sampler", SamplerDesc{}} //
        };
    PsoCI.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);
    TestCreatePSOFailure(PsoCI, "'g_Texture_sampler' is defined in overlapping shader stages (SHADER_TYPE_VERTEX, SHADER_TYPE_GEOMETRY and SHADER_TYPE_VERTEX, SHADER_TYPE_PIXEL)");
}

TEST_F(PSOCreationFailureTest, RenderPassWithNonZeroNumRenderTargets)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With non-zero NumRenderTargets", true)};
    PsoCI.GraphicsPipeline.NumRenderTargets = 1;
    TestCreatePSOFailure(PsoCI, "NumRenderTargets must be 0");
}

TEST_F(PSOCreationFailureTest, RenderPassWithDSVFormat)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With defined DSV format", true)};
    PsoCI.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
    TestCreatePSOFailure(PsoCI, "DSVFormat must be TEX_FORMAT_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, RenderPassWithRTVFormat)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With defined RTV format", true)};
    PsoCI.GraphicsPipeline.RTVFormats[1] = TEX_FORMAT_RGBA8_UNORM;
    TestCreatePSOFailure(PsoCI, "RTVFormats[1] must be TEX_FORMAT_UNKNOWN");
}

TEST_F(PSOCreationFailureTest, RenderPassWithInvalidSubpassIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Render Pass With invalid Subpass index", true)};
    PsoCI.GraphicsPipeline.SubpassIndex = 2;
    TestCreatePSOFailure(PsoCI, "Subpass index (2) exceeds the number of subpasses (1)");
}

TEST_F(PSOCreationFailureTest, NullResourceSignatures)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Null Resource Signatures", true)};
    PsoCI.ResourceSignaturesCount = 2;
    TestCreatePSOFailure(PsoCI, "ppResourceSignatures is null, but ResourceSignaturesCount (2) is not zero");
}

TEST_F(PSOCreationFailureTest, ZeroResourceSignaturesCount)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Zero Resource Signatures Count", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = 0;
    TestCreatePSOFailure(PsoCI, "ppResourceSignatures is not null, but ResourceSignaturesCount is zero.");
}


TEST_F(PSOCreationFailureTest, SignatureWithNonZeroNumVariables)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Resource Signature With non-zero NumVariables", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    PsoCI.PSODesc.ResourceLayout.NumVariables = 3;
    TestCreatePSOFailure(PsoCI, "The number of variables defined through resource layout (3) must be zero");
}

TEST_F(PSOCreationFailureTest, SignatureWithNonZeroNumImmutableSamplers)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Resource Signature With non-zero NumImmutableSamplers", true)};

    IPipelineResourceSignature* pSignatures[]         = {sm_pSignature0};
    PsoCI.ppResourceSignatures                        = pSignatures;
    PsoCI.ResourceSignaturesCount                     = _countof(pSignatures);
    PsoCI.PSODesc.ResourceLayout.NumImmutableSamplers = 4;
    TestCreatePSOFailure(PsoCI, "The number of immutable samplers defined through resource layout (4) must be zero");
}

TEST_F(PSOCreationFailureTest, NullSignature)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Null Signature", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, nullptr};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "signature at index 1 is null");
}

TEST_F(PSOCreationFailureTest, ConflictingSignatureBindIndex)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - Conflicting Signature Bind Index", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature0A};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "'PRS0A' at binding index 0 conflicts with another resource signature 'PRS0'");
}

TEST_F(PSOCreationFailureTest, ConflictingSignatureResourceStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - conflicting signature resource stages", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature1};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "Shader resource 'g_Texture' is found in more than one resource signature ('PRS1' and 'PRS0')");
}

TEST_F(PSOCreationFailureTest, ConflictingImmutableSamplerStages)
{
    auto PsoCI{GetGraphicsPSOCreateInfo("PSO Create Failure - conflicting signature immutable sampler stages", true)};

    IPipelineResourceSignature* pSignatures[] = {sm_pSignature0, sm_pSignature1A};
    PsoCI.ppResourceSignatures                = pSignatures;
    PsoCI.ResourceSignaturesCount             = _countof(pSignatures);
    TestCreatePSOFailure(PsoCI, "Immutable sampler 'g_Texture_sampler' is found in more than one resource signature ('PRS1A' and 'PRS0')");
}

TEST_F(PSOCreationFailureTest, InvalidComputePipelineType)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - Invalid Compute Pipeline Type")};
    PsoCI.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;
    TestCreatePSOFailure(PsoCI, "Pipeline type must be COMPUTE");
}

TEST_F(PSOCreationFailureTest, NoCS)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - no CS")};
    PsoCI.pCS = nullptr;
    TestCreatePSOFailure(PsoCI, "Compute shader must not be null");
}

TEST_F(PSOCreationFailureTest, InvalidCS)
{
    auto PsoCI{GetComputePSOCreateInfo("PSO Create Failure - invalid CS")};
    PsoCI.pCS = GetPS();
    TestCreatePSOFailure(PsoCI, "SHADER_TYPE_PIXEL is not a valid type for compute shader");
}

} // namespace