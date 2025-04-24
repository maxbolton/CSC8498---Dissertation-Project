#include "RendererBase.h"

using namespace NCL;
using namespace Rendering;

RendererBase::RendererBase(Window& window) : hostWindow(window)	{
	previousTime = clock::now();
}


RendererBase::~RendererBase()
{
}
