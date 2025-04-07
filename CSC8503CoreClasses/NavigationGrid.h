#pragma once
#include "NavigationMap.h"
#include <string>
namespace NCL {
	namespace CSC8503 {
		struct GridNode {
			GridNode* parent;

			GridNode* connected[4];
			int		  costs[4];

			Vector3		position;

			float f;
			float g;

			int type;

			GridNode() {
				for (int i = 0; i < 4; ++i) {
					connected[i] = nullptr;
					costs[i] = 0;
				}
				f = 0;
				g = 0;
				type = 0;
				parent = nullptr;
			}
			~GridNode() {	}
		};

		class NavigationGrid : public NavigationMap	{
		public:
			NavigationGrid();
			NavigationGrid(const std::string&filename);
			NavigationGrid(const std::string& filename, Vector3 origin);
			~NavigationGrid();

			bool FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) override;

			const GridNode* GetNode(int x, int y) const {
				if (x < 0 || x >= gridWidth || y < 0 || y >= gridHeight) {
					return nullptr;
				}
				return &allNodes[(gridWidth * y) + x];
			}

			int GetGridWidth() const { return gridWidth; }
			int GetGridHeight() const { return gridHeight; }
			int GetNodeSize() const { return nodeSize; }
			
			Vector3 GetWorldPos(int x, int y) const {
				return *origin + Vector3(x, 0, y);
			}

			Vector3 GetLocalPos(const Vector3& worldPos) const {
				Vector3 local = worldPos - *origin;
				return local;
			}

			void SetOrigin(Vector3* origin) {
				this->origin = origin;
			}

			Vector3* GetOrigin() const{
				return origin;
			}
				
		protected:
			bool		NodeInList(GridNode* n, std::vector<GridNode*>& list) const;
			GridNode*	RemoveBestNode(std::vector<GridNode*>& list) const;
			float		Heuristic(GridNode* hNode, GridNode* endNode) const;
			int nodeSize;
			int gridWidth;
			int gridHeight;


			Vector3* origin;

			GridNode* allNodes;
		};
	}
}

