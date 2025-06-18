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

		void Render(float dt) {

			auto currentTime = clock::now();
			std::chrono::duration<float> elapsedTime = currentTime - previousTime;
			previousTime = currentTime;

			frametime = elapsedTime.count();
			framerate = 1.0f / frametime;

			totalFrames++;


			//assert(HasInitialised());
			BeginFrame();
			RenderFrame(dt);
			EndFrame();
			SwapBuffers();
		}

		virtual bool SetVerticalSync(VerticalSyncState s) {
			return false;
		}

		virtual void BeginFrame()	= 0;
		virtual void RenderFrame(float dt)	= 0;
		virtual void EndFrame()		= 0;
		virtual void SwapBuffers()	= 0;

		virtual void OnWindowResize(int w, int h) = 0;
		virtual void OnWindowDetach() {}; //Most renderers won't care about this

		float* GetFrameTime() const { return const_cast<float*>(&frametime); }
		float* GetFrameRate() const { return const_cast<float*>(&framerate); }
		uint32_t* GetTotalFrames() const { return const_cast<uint32_t*>(&totalFrames); }
		void ResetCounters() {
			frametime = 0;
			framerate = 0;
			totalFrames = 0;
		}

	protected:	
		Window& hostWindow;

		Vector2i windowSize;

		float frametime;
		float framerate;
		uint32_t totalFrames;
		uint32_t droppedFrames;
	};
}