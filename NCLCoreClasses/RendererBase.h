/*
Part of Newcastle University's Game Engineering source code.

Use as you see fit!

Comments and queries to: richard-gordon.davison AT ncl.ac.uk
https://research.ncl.ac.uk/game/
*/
#pragma once
#include "Window.h"

namespace NCL::Rendering {
	enum class VerticalSyncState {
		VSync_ON,
		VSync_OFF,
		VSync_ADAPTIVE
	};
	class RendererBase {
	public:
		friend class NCL::Window;

		RendererBase(Window& w);
		virtual ~RendererBase();

		virtual bool HasInitialised() const {return true;}

		virtual void Update(float dt) {}

		using clock = std::chrono::high_resolution_clock;
		std::chrono::steady_clock::time_point previousTime;

		void Render() {

			auto currentTime = clock::now();
			std::chrono::duration<float> elapsedTime = currentTime - previousTime;
			previousTime = currentTime;

			frametime = elapsedTime.count();
			framerate = 1.0f / frametime;

			//std::cout << "FPS: " << framerate << std::endl;
			//std::cout << "Frame time: " << frametime * 1000.0f << "ms" << std::endl;



			//assert(HasInitialised());
			BeginFrame();
			RenderFrame();
			EndFrame();
			SwapBuffers();
		}

		virtual bool SetVerticalSync(VerticalSyncState s) {
			return false;
		}

		virtual void BeginFrame()	= 0;
		virtual void RenderFrame()	= 0;
		virtual void EndFrame()		= 0;
		virtual void SwapBuffers()	= 0;

		virtual void OnWindowResize(int w, int h) = 0;
		virtual void OnWindowDetach() {}; //Most renderers won't care about this

		float GetFrameTime() const { return frametime; }
		float GetFrameRate() const { return framerate; }

	protected:	
		Window& hostWindow;

		Vector2i windowSize;

		float frametime;
		float framerate;
	};
}