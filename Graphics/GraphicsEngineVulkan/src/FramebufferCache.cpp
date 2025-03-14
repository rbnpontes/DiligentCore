/*
 *  Copyright 2019-2025 Diligent Graphics LLC
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
#include "FramebufferCache.hpp"

#include <array>

#include "RenderDeviceVkImpl.hpp"
#include "HashUtils.hpp"

namespace Diligent
{


bool FramebufferCache::FramebufferCacheKey::operator==(const FramebufferCacheKey& rhs) const
{
    // clang-format off
    if (GetHash()        != rhs.GetHash()        ||
        Pass             != rhs.Pass             ||
        NumRenderTargets != rhs.NumRenderTargets ||
        DSV              != rhs.DSV              ||
        ShadingRate      != rhs.ShadingRate      ||
        CommandQueueMask != rhs.CommandQueueMask)
    {
        return false;
    }
    // clang-format on

    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
        if (RTVs[rt] != rhs.RTVs[rt])
            return false;

    return true;
}

size_t FramebufferCache::FramebufferCacheKey::GetHash() const
{
    if (Hash == 0)
    {
        Hash = ComputeHash(Pass, NumRenderTargets, DSV, ShadingRate, CommandQueueMask);
        for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
            HashCombine(Hash, RTVs[rt]);
    }
    return Hash;
}

bool FramebufferCache::FramebufferCacheKey::UsesImageView(VkImageView View) const
{
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        if (RTVs[rt] == View)
            return true;
    }
    return DSV == View || ShadingRate == View;
}

VkFramebuffer FramebufferCache::GetFramebuffer(const FramebufferCacheKey& Key, uint32_t width, uint32_t height, uint32_t layers)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};

    auto it = m_Cache.find(Key);
    if (it != m_Cache.end())
    {
        return it->second;
    }
    else
    {
        VkFramebufferCreateInfo FramebufferCI{};
        FramebufferCI.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferCI.pNext      = nullptr;
        FramebufferCI.flags      = 0; // reserved for future use
        FramebufferCI.renderPass = Key.Pass;

        std::array<VkImageView, 2 + MAX_RENDER_TARGETS> Attachments;

        uint32_t& attachment = FramebufferCI.attachmentCount;
        if (Key.DSV != VK_NULL_HANDLE)
            Attachments[attachment++] = Key.DSV;

        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
        {
            if (Key.RTVs[rt] != VK_NULL_HANDLE)
                Attachments[attachment++] = Key.RTVs[rt];
        }

        if (Key.ShadingRate != VK_NULL_HANDLE)
            Attachments[attachment++] = Key.ShadingRate;

        FramebufferCI.pAttachments = Attachments.data();
        FramebufferCI.width        = width;
        FramebufferCI.height       = height;
        FramebufferCI.layers       = layers;

        VulkanUtilities::FramebufferWrapper Framebuffer = m_DeviceVk.GetLogicalDevice().CreateFramebuffer(FramebufferCI);

        VkFramebuffer fb = Framebuffer;

        auto new_it = m_Cache.insert(std::make_pair(Key, std::move(Framebuffer)));
        VERIFY(new_it.second, "New framebuffer must be inserted into the map");
        (void)new_it;

        m_RenderPassToKeyMap.emplace(Key.Pass, Key);
        if (Key.DSV != VK_NULL_HANDLE)
            m_ViewToKeyMap.emplace(Key.DSV, Key);
        if (Key.ShadingRate != VK_NULL_HANDLE)
            m_ViewToKeyMap.emplace(Key.ShadingRate, Key);
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
            if (Key.RTVs[rt] != VK_NULL_HANDLE)
                m_ViewToKeyMap.emplace(Key.RTVs[rt], Key);

        return fb;
    }
}

std::unique_ptr<VulkanUtilities::RenderingInfoWrapper> FramebufferCache::CreateDyanmicRenderInfo(const FramebufferCacheKey&            Key,
                                                                                                 const CreateDyanmicRenderInfoAttribs& Attribs)
{
    std::unique_ptr<VulkanUtilities::RenderingInfoWrapper> RI = std::make_unique<VulkanUtilities::RenderingInfoWrapper>(
        Key.GetHash(), Key.NumRenderTargets, Attribs.UseDepthAttachment, Attribs.UseStencilAttachment);

    RI->SetRenderArea({{0, 0}, Attribs.Extent})
        .SetLayerCount(Attribs.Layers)
        .SetViewMask(Attribs.ViewMask);

    auto InitAttachment = [](VkRenderingAttachmentInfoKHR& Attachment, VkImageView View, VkImageLayout Layout) {
        Attachment.imageView          = View;
        Attachment.imageLayout        = Layout;
        Attachment.resolveMode        = VK_RESOLVE_MODE_NONE_KHR;
        Attachment.resolveImageView   = VK_NULL_HANDLE;
        Attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Attachment.loadOp             = VK_ATTACHMENT_LOAD_OP_LOAD;
        Attachment.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
        Attachment.clearValue         = VkClearValue{};
    };

    for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
    {
        VkRenderingAttachmentInfoKHR& RTAttachment = RI->GetColorAttachment(rt);
        InitAttachment(RTAttachment, Key.RTVs[rt], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (Attribs.UseDepthAttachment)
    {
        VkRenderingAttachmentInfoKHR& DepthAttachment = RI->GetDepthAttachment();
        InitAttachment(DepthAttachment, Key.DSV, Attribs.ReadOnlyDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    if (Attribs.UseStencilAttachment)
    {
        VkRenderingAttachmentInfoKHR& StencilAttachment = RI->GetStencilAttachment();
        InitAttachment(StencilAttachment, Key.DSV, Attribs.ReadOnlyDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    if (Key.ShadingRate)
    {
        VkRenderingFragmentShadingRateAttachmentInfoKHR& ShadingRateAttachment = RI->GetShadingRateAttachment();

        ShadingRateAttachment.imageView                      = Key.ShadingRate;
        ShadingRateAttachment.imageLayout                    = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        ShadingRateAttachment.shadingRateAttachmentTexelSize = Attribs.ShadingRateTexelSize;
    }

    return RI;
}

FramebufferCache::~FramebufferCache()
{
    VERIFY(m_Cache.empty(), "All framebuffers must be released");
    VERIFY(m_ViewToKeyMap.empty(), "All image views must be released and the cache must be notified");
    VERIFY(m_RenderPassToKeyMap.empty(), "All render passes must be released and the cache must be notified");
}

void FramebufferCache::OnDestroyImageView(VkImageView ImgView)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};

    auto equal_range = m_ViewToKeyMap.equal_range(ImgView);
    for (auto it = equal_range.first; it != equal_range.second; ++it)
    {
        const FramebufferCacheKey& Key = it->second;

        auto fb_it = m_Cache.find(Key);
        // Multiple image views may be associated with the same key.
        // The framebuffer is deleted whenever any of the image views is deleted
        if (fb_it != m_Cache.end())
        {
            m_DeviceVk.SafeReleaseDeviceObject(std::move(fb_it->second), it->second.CommandQueueMask);
            m_Cache.erase(fb_it);
        }

        // Remove all keys from m_RenderPassToKeyMap that use the image view
        {
            auto rp_it_range = m_RenderPassToKeyMap.equal_range(Key.Pass);
            for (auto rp_it = rp_it_range.first; rp_it != rp_it_range.second;)
            {
                if (Key.UsesImageView(ImgView))
                    rp_it = m_RenderPassToKeyMap.erase(rp_it);
                else
                    ++rp_it;
            }
        }
    }
    m_ViewToKeyMap.erase(equal_range.first, equal_range.second);
}

void FramebufferCache::OnDestroyRenderPass(VkRenderPass Pass)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};

    auto equal_range = m_RenderPassToKeyMap.equal_range(Pass);
    for (auto it = equal_range.first; it != equal_range.second; ++it)
    {
        const FramebufferCacheKey& Key = it->second;

        auto fb_it = m_Cache.find(Key);
        // Multiple image views may be associated with the same key.
        // The framebuffer is deleted whenever any of the image views or render pass is destroyed
        if (fb_it != m_Cache.end())
        {
            m_DeviceVk.SafeReleaseDeviceObject(std::move(fb_it->second), it->second.CommandQueueMask);
            m_Cache.erase(fb_it);
        }

        // Remove all keys from m_ViewToKeyMap that use the render pass
        auto PurgeViewToKeyMap = [this, Pass](VkImageView vkView) {
            if (vkView == VK_NULL_HANDLE)
                return;
            auto view_it_range = m_ViewToKeyMap.equal_range(vkView);
            for (auto view_it = view_it_range.first; view_it != view_it_range.second;)
            {
                if (view_it->second.Pass == Pass)
                    view_it = m_ViewToKeyMap.erase(view_it);
                else
                    ++view_it;
            }
        };
        for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
        {
            PurgeViewToKeyMap(Key.RTVs[rt]);
        }
        PurgeViewToKeyMap(Key.DSV);
        PurgeViewToKeyMap(Key.ShadingRate);
    }
    m_RenderPassToKeyMap.erase(equal_range.first, equal_range.second);
}

} // namespace Diligent
