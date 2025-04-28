#pragma once

#include <iostream>
#include <chrono>
#include "GameTechRenderer.h"


namespace NCL {
	namespace CSC8503 {

		class PerfStats
		{
		private:
			float* framerate = nullptr;
			float* frametime = nullptr;

			uint32_t* totalFrames = nullptr;
			uint32_t droppedFrames60;
			uint32_t droppedFrames120;

			const float hz60Target = 1.0f / 60.0f;
			const float hz120Target = 1.0f / 120.0f;

			GameTechRenderer* renderer = nullptr;



		public:

			PerfStats(GameTechRenderer* renderer) {
				this->renderer = renderer;
				framerate = this->renderer->GetFrameRate();
				frametime = this->renderer->GetFrameTime();
				totalFrames = this->renderer->GetTotalFrames();
				renderer->ResetCounters();
				droppedFrames120 = 0.0f;
				droppedFrames60 = 0.0f;
			}

			~PerfStats() {
				delete framerate;
				delete frametime;
			}

			void PrintStats() {
				CalcDroppedFrames();

				//clear console
				system("cls");

				std::cout << "-------Stats--------" << std::endl;
				std::cout << "FPS: " << *framerate << std::endl;
				std::cout << "Frame time: " << *frametime * 1000.0f << "ms" << std::endl;
				std::cout << "Total frames: " << *totalFrames << std::endl;
				std::cout << "Dropped frames@60hz: " << droppedFrames60 << std::endl;
				std::cout << "Dropped frames@120hz: " << droppedFrames120 << std::endl;
			}

			void CalcDroppedFrames() {

				uint32_t expectedFrames60 = *frametime / hz60Target;

				if (expectedFrames60 > 1.0f) {
					droppedFrames60 += expectedFrames60 - 1.0f;
				}

				uint32_t expectedFrames120 = *frametime / hz120Target;

				if (expectedFrames120 > 1.0f) {
					droppedFrames120 += expectedFrames120 - 1.0f;
				}
			}


		};
	}
}

