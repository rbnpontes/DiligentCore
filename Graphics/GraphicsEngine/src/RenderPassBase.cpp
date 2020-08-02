/*
 *  Copyright 2019-2020 Diligent Graphics LLC
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

#include "pch.h"
#include "RenderPassBase.hpp"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

namespace Diligent
{

void ValidateRenderPassDesc(const RenderPassDesc& Desc)
{
#define LOG_RENDER_PASS_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Render pass '", (Desc.Name ? Desc.Name : ""), "': ", ##__VA_ARGS__)

    if (Desc.AttachmentCount != 0 && Desc.pAttachments == nullptr)
    {
        // If attachmentCount is not 0, pAttachments must be a valid pointer to an
        // array of attachmentCount valid VkAttachmentDescription structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pAttachments-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("The attachment count (", Desc.AttachmentCount, ") is not zero, but pAttachments is null");
    }

    if (Desc.SubpassCount == 0)
    {
        // subpassCount must be greater than 0.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-subpassCount-arraylength
        LOG_RENDER_PASS_ERROR_AND_THROW("Render pass must have at least one subpass");
    }
    if (Desc.pSubpasses == nullptr)
    {
        // pSubpasses must be a valid pointer to an array of subpassCount valid VkSubpassDescription structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pSubpasses-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("pSubpasses must not be null");
    }

    if (Desc.DependencyCount != 0 && Desc.pDependencies == nullptr)
    {
        // If dependencyCount is not 0, pDependencies must be a valid pointer to an array of
        // dependencyCount valid VkSubpassDependency structures.
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-VkRenderPassCreateInfo-pDependencies-parameter
        LOG_RENDER_PASS_ERROR_AND_THROW("The dependency count (", Desc.DependencyCount, ") is not zero, but pDependencies is null");
    }

    for (Uint32 i = 0; i < Desc.AttachmentCount; ++i)
    {
        const auto& Attachment = Desc.pAttachments[i];
        if (Attachment.Format == TEX_FORMAT_UNKNOWN)
            LOG_RENDER_PASS_ERROR_AND_THROW("the format of attachment ", i, " is unknown");

        if (Attachment.SampleCount == 0)
            LOG_RENDER_PASS_ERROR_AND_THROW("the sample count of attachment ", i, " is zero");

        if (!IsPowerOfTwo(Attachment.SampleCount))
            LOG_RENDER_PASS_ERROR_AND_THROW("the sample count of attachment ", i, "(", Attachment.SampleCount, ") is not power of two");

        const auto& FmtInfo = GetTextureFormatAttribs(Attachment.Format);
        if (FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH ||
            FmtInfo.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (Attachment.InitialState != RESOURCE_STATE_DEPTH_WRITE &&
                Attachment.InitialState != RESOURCE_STATE_DEPTH_READ &&
                Attachment.InitialState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.InitialState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_SOURCE)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the initial state of depth-stencil attachment ", i, " (", GetResourceStateString(Attachment.InitialState), ") is invalid");
            }

            if (Attachment.FinalState != RESOURCE_STATE_DEPTH_WRITE &&
                Attachment.FinalState != RESOURCE_STATE_DEPTH_READ &&
                Attachment.FinalState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.FinalState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_SOURCE)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the final state of depth-stencil attachment ", i, " (", GetResourceStateString(Attachment.FinalState), ") is invalid");
            }
        }
        else
        {
            if (Attachment.InitialState != RESOURCE_STATE_RENDER_TARGET &&
                Attachment.InitialState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.InitialState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.InitialState != RESOURCE_STATE_RESOLVE_SOURCE)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the initial state of color attachment ", i, " (", GetResourceStateString(Attachment.InitialState), ") is invalid");
            }

            if (Attachment.FinalState != RESOURCE_STATE_RENDER_TARGET &&
                Attachment.FinalState != RESOURCE_STATE_UNORDERED_ACCESS &&
                Attachment.FinalState != RESOURCE_STATE_SHADER_RESOURCE &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_DEST &&
                Attachment.FinalState != RESOURCE_STATE_RESOLVE_SOURCE)
            {
                LOG_RENDER_PASS_ERROR_AND_THROW("the final state of color attachment ", i, " (", GetResourceStateString(Attachment.FinalState), ") is invalid");
            }
        }
    }

    for (Uint32 i = 0; i < Desc.SubpassCount; ++i)
    {
        const auto& Subpass = Desc.pSubpasses[i];
        if (Subpass.InputAttachmentCount != 0 && Subpass.pInputAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the input attachment count (", Subpass.InputAttachmentCount, ") of subpass ", i,
                                            " is not zero, while pInputAttachments is null");
        }
        if (Subpass.RenderTargetAttachmentCount != 0 && Subpass.pRenderTargetAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the render target attachment count (", Subpass.RenderTargetAttachmentCount, ") of subpass ", i,
                                            " is not zero, while pRenderTargetAttachments is null");
        }
        if (Subpass.PreserveAttachmentCount != 0 && Subpass.pPreserveAttachments == nullptr)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the preserve attachment count (", Subpass.PreserveAttachmentCount, ") of subpass ", i,
                                            " is not zero, while pPreserveAttachments is null");
        }
    }

    for (Uint32 i = 0; i < Desc.DependencyCount; ++i)
    {
        const auto& Dependency = Desc.pDependencies[i];

        if (Dependency.SrcStageMask == PIPELINE_STAGE_FLAG_UNDEFINED)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the source stage mask of subpass dependency ", i, " is undefined");
        }
        if (Dependency.DstStageMask == PIPELINE_STAGE_FLAG_UNDEFINED)
        {
            LOG_RENDER_PASS_ERROR_AND_THROW("the destination stage mask of subpass dependency ", i, " is undefined");
        }
    }
}

} // namespace Diligent
