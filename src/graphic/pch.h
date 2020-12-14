#pragma once

#include <graphic/dx12utils.h>
#include <graphic/gfxresourcemanager.h>
#include <graphic/shapes.h>
#include <graphic/view.h>
#include <graphic/visual.h>

#include <graphic/gfx/gfxadapter.h>
#include <graphic/gfx/gfxcommandlist.h>
#include <graphic/gfx/gfxcommonstates.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxdevice.h>
#include <graphic/gfx/gfxlightsmanager.h>
#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxmesh.h>
#include <graphic/gfx/gfxpipelinestateobject.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxshadermanager.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>
#include <graphic/gfx/gfxvertexformat.h>

#include <graphic/renderers/gfxrendererbase.h>

// Helper macro for dispatch compute shader threads
#define bbeGetCSDispatchCount(domainWidth, groupWidth) (((domainWidth) + ((groupWidth) - 1)) / (groupWidth))
