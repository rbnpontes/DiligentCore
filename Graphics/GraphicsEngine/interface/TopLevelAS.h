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

#pragma once

/// \file
/// Definition of the Diligent::ITopLevelAS interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "../../../Primitives/interface/FlagEnum.h"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "Buffer.h"
#include "BottomLevelAS.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {16561861-294B-4804-96FA-1717333F769A}
static const INTERFACE_ID IID_TopLevelAS =
    {0x16561861, 0x294b, 0x4804, {0x96, 0xfa, 0x17, 0x17, 0x33, 0x3f, 0x76, 0x9a}};

// clang-format off

/// Defines shader binding mode.
DILIGENT_TYPED_ENUM(SHADER_BINDING_MODE, Uint8)
{
    /// Each geometry in each instance can have a unique shader.
    SHADER_BINDING_MODE_PER_GEOMETRY = 0,

    /// Each instance can have a unique shader. In this mode SBT buffer will use less memory.
    SHADER_BINDING_MODE_PER_INSTANCE,

    /// The user must specify TLASBuildInstanceData::InstanceContributionToHitGroupIndex and only use IShaderBindingTable::BindAll().
    SHADER_BINDING_USER_DEFINED,
};


/// Top-level AS description.
struct TopLevelASDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Allocate space for specified number of instances.
    Uint32                    MaxInstanceCount DEFAULT_INITIALIZER(0);

    /// Ray tracing build flags, see Diligent::RAYTRACING_BUILD_AS_FLAGS.
    RAYTRACING_BUILD_AS_FLAGS Flags            DEFAULT_INITIALIZER(RAYTRACING_BUILD_AS_NONE);
    
    /// The size returned by IDeviceContext::WriteTLASCompactedSize(), if this acceleration structure
    /// is going to be the target of a compacting copy (IDeviceContext::CopyTLAS() with COPY_AS_MODE_COMPACT).
    Uint32                    CompactedSize    DEFAULT_INITIALIZER(0);

    /// Binding mode that i used for TLASBuildInstanceData::ContributionToHitGroupIndex calculation,
    /// see Diligent::SHADER_BINDING_MODE.
    SHADER_BINDING_MODE       BindingMode      DEFAULT_INITIALIZER(SHADER_BINDING_MODE_PER_GEOMETRY);
    
    /// Defines which command queues this BLAS can be used with.
    Uint64                    CommandQueueMask DEFAULT_INITIALIZER(1);
    
#if DILIGENT_CPP_INTERFACE
    TopLevelASDesc() noexcept {}
#endif
};
typedef struct TopLevelASDesc TopLevelASDesc;


/// Top-level AS instance description.
struct TLASInstanceDesc
{
    /// Index that corresponds to the one specified in TLASBuildInstanceData::ContributionToHitGroupIndex.
    Uint32          ContributionToHitGroupIndex DEFAULT_INITIALIZER(0);

    /// Bottom-level AS that is specified in TLASBuildInstanceData::pBLAS.
    IBottomLevelAS* pBLAS                       DEFAULT_INITIALIZER(nullptr);
    
#if DILIGENT_CPP_INTERFACE
    TLASInstanceDesc() noexcept {}
#endif
};
typedef struct TLASInstanceDesc TLASInstanceDesc;


#define DILIGENT_INTERFACE_NAME ITopLevelAS
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITopLevelASInclusiveMethods        \
    IDeviceObjectInclusiveMethods;         \
    ITopLevelASMethods TopLevelAS

/// Top-level AS interface

/// Defines the methods to manipulate a TLAS object
DILIGENT_BEGIN_INTERFACE(ITopLevelAS, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the top level AS description used to create the object
    virtual const TopLevelASDesc& DILIGENT_CALL_TYPE GetDesc() const override = 0;
#endif
    
    /// Returns instance description that can be used in shader binding table.
    
    /// \param [in] Name - Instance name that is specified in TLASBuildInstanceData::InstanceName.
    /// \return structure object.
    VIRTUAL TLASInstanceDesc METHOD(GetInstanceDesc)(THIS_
                                                     const char* Name) CONST PURE;
    
    /// Returns scratch buffer info for the current acceleration structure.
    
    /// \return structure object.
    VIRTUAL ScratchBufferSizes METHOD(GetScratchBufferSizes)(THIS) CONST PURE;

    /// Returns native acceleration structure handle specific to the underlying graphics API

    /// \return pointer to ID3D12Resource interface, for D3D12 implementation\n
    ///         VkAccelerationStructureKHR handle, for Vulkan implementation
    VIRTUAL void* METHOD(GetNativeHandle)(THIS) PURE;

    /// Sets the acceleration structure usage state.

    /// \note This method does not perform state transition, but
    ///       resets the internal acceleration structure state to the given value.
    ///       This method should be used after the application finished
    ///       manually managing the acceleration structure state and wants to hand over
    ///       state management back to the engine.
    VIRTUAL void METHOD(SetState)(THIS_
                                  RESOURCE_STATE State) PURE;

    /// Returns the internal acceleration structure state
    VIRTUAL RESOURCE_STATE METHOD(GetState)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define ITopLevelAS_GetInstanceDesc(This, ...)  CALL_IFACE_METHOD(TopLevelAS, GetInstanceDesc,       This, __VA_ARGS__)
#    define ITopLevelAS_GetScratchBufferSizes(This) CALL_IFACE_METHOD(TopLevelAS, GetScratchBufferSizes, This)
#    define ITopLevelAS_GetNativeHandle(This)       CALL_IFACE_METHOD(TopLevelAS, GetNativeHandle,       This)
#    define ITopLevelAS_SetState(This, ...)         CALL_IFACE_METHOD(TopLevelAS, SetState,              This, __VA_ARGS__)
#    define ITopLevelAS_GetState(This)              CALL_IFACE_METHOD(TopLevelAS, GetState,              This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
