#pragma once

#include <iostream>
#include <chrono>
#include "GameTechRenderer.h"
#include <ctime>


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

			std::ofstream file;


		public:

			PerfStats(GameTechRenderer* renderer) {
				this->renderer = renderer;
				framerate = this->renderer->GetFrameRate();
				frametime = this->renderer->GetFrameTime();
				totalFrames = this->renderer->GetTotalFrames();
				renderer->ResetCounters();
				droppedFrames120 = 0.0f;
				droppedFrames60 = 0.0f;

				OpenFile();
				
			}

			~PerfStats() {
				delete framerate;
				delete frametime;
				delete totalFrames;

				file.close();
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

				WriteToFile();
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

			void WriteToFile() {
				file << *frametime << "," << *framerate << "," << droppedFrames60 << "," << droppedFrames120 << std::endl;
				file.flush();
			}

			void OpenFile() {
				std::string date = std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
				std::string time = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
				std::string filename = "PerfStats_" + GetDateTime() + ".csv";
				std::string directory = "../../Documents/";

				file.open(directory + filename, std::ios::app);

				// Headers for frametime, framerate, and dropped frames
				file << "FrameTime,FrameRate,DroppedFrames60,DroppedFrames120" << std::endl;
			}

			std::string GetDateTime() {
				auto now = std::chrono::system_clock::now();
				std::time_t now_time = std::chrono::system_clock::to_time_t(now);

				std::tm now_tm = *std::localtime(&now_time);

				std::ostringstream oss;
				oss << std::put_time(&now_tm, "%d-%m~%H-%M-%S");
				return oss.str();
			}

		};
	}
}

