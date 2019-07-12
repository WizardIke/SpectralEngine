#pragma once
#include "Vector3.h"
#include <cstddef>
#include "World.h"
#include <cmath>
#include "Zone.h"
#include <algorithm>
#include "PriorityQueue.h"
#include "HashSet.h"
#include "ResizingArray.h"
#include "WorldManagerStopRequest.h"

template<class ThreadResources>
class WorldManager
{
public:
	using StopRequest = WorldManagerStopRequest;
private:
	class SearchNode;
	class SearchNodeTracker
	{
	public:
		SearchNode* node;
		SearchNodeTracker() = default;
		SearchNodeTracker(const SearchNodeTracker& other) = delete;
		SearchNodeTracker(SearchNodeTracker&& other) : node(other.node)
		{
			other.node = nullptr;
			node->queueLocation = this;
		}

		void operator=(const SearchNodeTracker& other) = delete;
		void operator=(SearchNodeTracker&& other)
		{
			node = other.node;
			other.node = nullptr;
			node->queueLocation = this;
		}

		~SearchNodeTracker()
		{
			if(node != nullptr)
			{
				node->queueLocation = nullptr;
			}
		}

		SearchNode* operator->() const
		{
			return node;
		}
	};

	class SearchNode
	{
	public:
		SearchNode() noexcept {}

		constexpr SearchNode(float totalDistance1, unsigned long zoneX1, unsigned long zoneY1, unsigned long zoneZ1, unsigned long zoneIndex1,
			unsigned long worldIndex1, Vector3 anchor1, float distanceFromStartToAnchor1, SearchNodeTracker* queueLocation1) noexcept :
			totalDistance(totalDistance1), zoneX(zoneX1), zoneY(zoneY1), zoneZ(zoneZ1),
			zoneIndex(zoneIndex1), worldIndex(worldIndex1), anchor(anchor1),
			distanceFromStartToAnchor(distanceFromStartToAnchor1), 
			queueLocation(queueLocation1){}

		SearchNode(SearchNode&& other) noexcept : totalDistance(other.totalDistance), zoneX(other.zoneX), zoneY(other.zoneY), zoneZ(other.zoneZ), zoneIndex(other.zoneIndex),
			worldIndex(other.worldIndex), anchor(other.anchor), distanceFromStartToAnchor(other.distanceFromStartToAnchor), queueLocation(other.queueLocation)
		{
			if(queueLocation != nullptr)
			{
				queueLocation->node = this;
			}
		}

		void operator=(SearchNode&& other) noexcept
		{
			totalDistance = other.totalDistance;
			zoneX = other.zoneX;
			zoneY = other.zoneY;
			zoneZ = other.zoneZ;
			zoneIndex = other.zoneIndex;
			worldIndex = other.worldIndex;
			anchor = other.anchor;
			distanceFromStartToAnchor = other.distanceFromStartToAnchor;
			queueLocation = other.queueLocation;

			if (queueLocation != nullptr)
			{
				queueLocation->node = this;
			}
		}

		float totalDistance;
		unsigned long zoneX;
		unsigned long zoneY;
		unsigned long zoneZ;
		unsigned long zoneIndex;
		unsigned long worldIndex;
		Vector3 anchor;
		float distanceFromStartToAnchor;
		SearchNodeTracker* queueLocation;
	};

	class SearchNodeHasher
	{
	public:
		std::size_t operator()(const SearchNode& node) const noexcept
		{
			return (std::size_t)node.zoneIndex;
		}

		std::size_t operator()(unsigned long zoneIndex) const noexcept
		{
			return (std::size_t)zoneIndex;
		}
	};

	class SearchNodeEqual
	{
	public:
		constexpr bool operator()(const SearchNode& lhs, const SearchNode& rhs) const noexcept
		{
			return lhs.zoneIndex == rhs.zoneIndex;
		}

		constexpr bool operator()(const SearchNode& lhs, unsigned long zoneIndex) const noexcept
		{
			return lhs.zoneIndex == zoneIndex;
		}
	};

	class SearchNodePriority
	{
	public:
		constexpr bool operator()(const SearchNodeTracker& lhs, const SearchNodeTracker& rhs) const noexcept
		{
			return lhs->totalDistance > rhs->totalDistance;
		}
	};

	constexpr static float zoneRadius = 64.0f;
	constexpr static float zoneDiameter = 2.0f * zoneRadius;

	Vector3 loadedPosition; //Center of the area that was last loaded
	const Vector3* position; //The position to load around 
	Zone<ThreadResources>* const zones;
	const World* worlds;
	unsigned long currentWorldIndex;
	const float* const levelOfDetailMaxDistances;
	const unsigned int levelOfDetailMaxDistancesCount;
	unsigned int currentLoadedZonesIndex = 0u;
	HashSet<unsigned long, std::hash<unsigned long>, std::equal_to<unsigned long>, std::allocator<unsigned long>, unsigned long> loadedZones[2];
	HashSet<SearchNode, SearchNodeHasher, SearchNodeEqual> visitedNodes;
	PriorityQueue<SearchNodeTracker, ResizingArray<SearchNodeTracker>, SearchNodePriority> frontierNodes;
	StopRequest* mStopRequest = nullptr;

	void load(ThreadResources& threadResources)
	{
		unsigned int levelOfDetail = 0u;
		const float maxTotalDistance = levelOfDetailMaxDistances[levelOfDetailMaxDistancesCount - 1u];

		{
			const World& world = worlds[currentWorldIndex];
			const unsigned long zoneX = std::min((unsigned long)std::max((long)(loadedPosition.x() / zoneDiameter), 0l), world.sizeX - 1u);
			const unsigned long zoneY = std::min((unsigned long)std::max((long)(loadedPosition.y() / zoneDiameter), 0l), world.sizeY - 1u);
			const unsigned long zoneZ = std::min((unsigned long)std::max((long)(loadedPosition.z() / zoneDiameter), 0l), world.sizeZ - 1u);
			const float totalDistance = (Vector3{zoneX * zoneDiameter + zoneRadius, zoneY * zoneDiameter + zoneRadius, zoneZ * zoneDiameter + zoneRadius} - loadedPosition).length();
			const unsigned long zoneIndex = world.zonesStartIndex + zoneX * (world.sizeY * world.sizeZ) + zoneY * world.sizeZ + zoneZ;

			SearchNodeTracker newNodeTracker;
			SearchNode newNode = {totalDistance, zoneX, zoneY, zoneZ, zoneIndex, currentWorldIndex, loadedPosition, 0.0f, &newNodeTracker};
			newNodeTracker.node = &newNode;
			visitedNodes.insert(std::move(newNode));
			frontierNodes.push(std::move(newNodeTracker));
		}

		while(!frontierNodes.empty())
		{
			SearchNodeTracker currentNodeTracker = frontierNodes.popAndGet();
			SearchNode* currentNode = currentNodeTracker.node;
			while(currentNode->totalDistance > levelOfDetailMaxDistances[levelOfDetail])
			{
				++levelOfDetail;
			}

			auto& zone = zones[currentNode->zoneIndex];
			{ //load zone
				if(zone && zone.usesLevelOfDetail(levelOfDetail))
				{
					loadedZones[currentLoadedZonesIndex].insert(currentNode->zoneIndex);
					loadedZones[currentLoadedZonesIndex ^ 1u].erase(currentNode->zoneIndex);
					zone.setState(levelOfDetail, threadResources);
				}
			}

			const World& world = worlds[currentNode->worldIndex];
			const unsigned long zoneX = currentNode->zoneX;
			const unsigned long zoneY = currentNode->zoneY;
			const unsigned long zoneZ = currentNode->zoneZ;
			const unsigned long zoneIndex = currentNode->zoneIndex;
			const unsigned long sizeX = world.sizeX;
			const unsigned long sizeY = world.sizeY;
			const unsigned long sizeZ = world.sizeZ;
			if(zoneX + 1u != sizeX) //move toward x
			{
				visitZone(zoneX + 1u, zoneY, zoneZ, zoneIndex + sizeY * sizeZ, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zoneX != 0u) //move toward -x
			{
				visitZone(zoneX - 1u, zoneY, zoneZ, zoneIndex - sizeY * sizeZ, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zoneY + 1 != sizeY) //move toward y
			{
				visitZone(zoneX, zoneY + 1u, zoneZ, zoneIndex + sizeZ, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zoneY != 0u) //move toward -y
			{
				visitZone(zoneX, zoneY - 1u, zoneZ, zoneIndex - sizeZ, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zoneZ + 1 != sizeZ) //move toward z
			{
				visitZone(zoneX, zoneY, zoneZ + 1u, zoneIndex + 1u, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zoneZ != 0u) //move toward -z
			{
				visitZone(zoneX, zoneY, zoneZ - 1u, zoneIndex - 1u, currentNode, visitedNodes, frontierNodes, maxTotalDistance);
				currentNode = currentNodeTracker.node;
			}
			if(zone)
			{
				visitPortals(currentNode, zones, worlds, visitedNodes, frontierNodes, maxTotalDistance);
			}
		}
		visitedNodes.clear();
	}

	static void visitZone(unsigned long zoneX, unsigned long zoneY, unsigned long zoneZ, unsigned long zoneIndex, SearchNode* currentNode,
		HashSet<SearchNode, SearchNodeHasher, SearchNodeEqual>& visitedNodes,
		PriorityQueue<SearchNodeTracker, ResizingArray<SearchNodeTracker>, SearchNodePriority>& frontierNodes,
		float maxTotalDistance)
	{
		const float newTotalDistance = currentNode->distanceFromStartToAnchor + (Vector3{zoneX * zoneDiameter + zoneRadius, zoneY * zoneDiameter + zoneRadius, zoneZ * zoneDiameter + zoneRadius} - currentNode->anchor).length();
		if(newTotalDistance > maxTotalDistance)
		{
			return;
		}

		auto element = visitedNodes.find(zoneIndex);
		if(element != visitedNodes.end())
		{
			if(element->totalDistance > newTotalDistance)
			{
				element->totalDistance = newTotalDistance;
				frontierNodes.priorityIncreased(*element->queueLocation);
			}
		}
		else
		{
			SearchNodeTracker newNodeTracker;
			SearchNode newNode = {newTotalDistance, zoneX, zoneY, zoneZ, zoneIndex, currentNode->worldIndex, currentNode->anchor, currentNode->distanceFromStartToAnchor, &newNodeTracker};
			newNodeTracker.node = &newNode;
			visitedNodes.insert(std::move(newNode));
			frontierNodes.push(std::move(newNodeTracker));
		}
	}

	static void visitPortals(SearchNode* currentNode,
		Zone<ThreadResources>* zones,
		const World* worlds1,
		HashSet<SearchNode, SearchNodeHasher, SearchNodeEqual>& visitedNodes,
		PriorityQueue<SearchNodeTracker, ResizingArray<SearchNodeTracker>, SearchNodePriority>& frontierNodes,
		float maxTotalDistance)
	{
		auto& zone = zones[currentNode->zoneIndex];
		auto currentDistanceFromStartToAnchor = currentNode->distanceFromStartToAnchor;
		auto currentAnchor = currentNode->anchor;
		for(const Portal& portal : zone.getPortals())
		{
			unsigned long newWorldIndex = portal.exitWorldIndex;
			const World& newWorld = worlds1[newWorldIndex];
			auto& newPosition = portal.exitPosition;
			const unsigned long newZoneX = std::min((unsigned long)std::max((long)(newPosition.x() / zoneDiameter), 0l), newWorld.sizeX - 1ul);
			const unsigned long newZoneY = std::min((unsigned long)std::max((long)(newPosition.y() / zoneDiameter), 0l), newWorld.sizeY - 1ul);
			const unsigned long newZoneZ = std::min((unsigned long)std::max((long)(newPosition.z() / zoneDiameter), 0l), newWorld.sizeZ - 1ul);
			const unsigned long newZoneIndex = newWorld.zonesStartIndex + newZoneX * (newWorld.sizeY * newWorld.sizeZ) + newZoneY * newWorld.sizeZ + newZoneZ;

			const float newDistanceFromStartToAnchor = currentDistanceFromStartToAnchor + (portal.position - currentAnchor).length();
			const float newTotalDistance = newDistanceFromStartToAnchor + (Vector3{newZoneX * zoneDiameter + zoneRadius, newZoneY * zoneDiameter + zoneRadius, newZoneZ * zoneDiameter + zoneRadius} - newPosition).length();

			if(newTotalDistance > maxTotalDistance)
			{
				continue;
			}

			auto element = visitedNodes.find(newZoneIndex);
			if(element != visitedNodes.end())
			{
				if(element->totalDistance > newTotalDistance)
				{
					element->totalDistance = newTotalDistance;
					element->anchor = newPosition;
					element->distanceFromStartToAnchor = newDistanceFromStartToAnchor;
					frontierNodes.priorityIncreased(*element->queueLocation);
				}
			}
			else
			{
				SearchNodeTracker newNodeTracker;
				SearchNode newNode = {newTotalDistance, newZoneX, newZoneY, newZoneZ, newZoneIndex, newWorldIndex, newPosition, newDistanceFromStartToAnchor, &newNodeTracker};
				newNodeTracker.node = &newNode;
				visitedNodes.insert(std::move(newNode));
				frontierNodes.push(std::move(newNodeTracker));
			}
		}
	}

	void unloadNoLongerNeededZones(ThreadResources& threadResources, unsigned long lastLoadedZonesSize)
	{
		auto& zonesToUnload = loadedZones[currentLoadedZonesIndex];
		zonesToUnload.consume([&threadResources, zones = this->zones, levelOfDetailMaxDistancesCount = this->levelOfDetailMaxDistancesCount](unsigned long zoneIndex)
		{
			zones[zoneIndex].setState(levelOfDetailMaxDistancesCount, threadResources);
		});
		zonesToUnload.shrink_to_size(lastLoadedZonesSize * 2u);
	}

	void update(ThreadResources& threadResources)
	{
		bool shouldLoad = false;
		if(position->x() > loadedPosition.x() + zoneRadius)
		{
			loadedPosition.x() = std::floor(position->x() / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		else if(position->x() < loadedPosition.x() - zoneRadius)
		{
			loadedPosition.x() = std::ceil(position->x() / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		if(position->y() > loadedPosition.y() + zoneRadius)
		{
			loadedPosition.y() = std::floor(position->y() / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		else if(position->y() < loadedPosition.y() - zoneRadius)
		{
			loadedPosition.y() = std::ceil((position->y() + zoneRadius) / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		if(position->z() > loadedPosition.z() + zoneRadius)
		{
			loadedPosition.z() = std::ceil(position->z() / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		else if(position->z() < loadedPosition.z() - zoneRadius)
		{
			loadedPosition.z() = std::floor((position->z() + zoneRadius) / zoneRadius) * zoneRadius;
			shouldLoad = true;
		}
		if(shouldLoad)
		{
			unsigned long lastLoadedZonesSize = loadedZones[currentLoadedZonesIndex ^ 1u].size();
			load(threadResources);
			currentLoadedZonesIndex ^= 1u;
			unloadNoLongerNeededZones(threadResources, lastLoadedZonesSize);
		}
	}

	void stopZones(StopRequest* stopRequest, ThreadResources& threadResources)
	{
		auto zonesTemp = zones;
		stopRequest->numberOfComponentsToUnload = loadedZones[currentLoadedZonesIndex ^ 1u].size();
		if (stopRequest->numberOfComponentsToUnload == 0u)
		{
			stopRequest->callback(*stopRequest, &threadResources);
			return;
		}
		unsigned long i = 0u;
		for(unsigned long zoneIndex : loadedZones[currentLoadedZonesIndex ^ 1u])
		{
			zonesTemp[zoneIndex].stop(stopRequest, threadResources);
			++i;
		}
	}

	void run(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, {this, [](void*const requester, ThreadResources& threadResources)
		{
			const auto manager = static_cast<WorldManager*>(requester);

			StopRequest* stopRequest = manager->mStopRequest;
			if(stopRequest != nullptr)
			{
				manager->stopZones(stopRequest, threadResources);
				return;
			}
			
			manager->update(threadResources);
			manager->run(threadResources);
		}});
	}
public:
	WorldManager(Zone<ThreadResources>* zones, const World* worlds1, const float* levelOfDetailMaxDistances,
	unsigned int levelOfDetailMaxDistancesCount) : zones(zones), worlds(worlds1), levelOfDetailMaxDistances(levelOfDetailMaxDistances), levelOfDetailMaxDistancesCount(levelOfDetailMaxDistancesCount) {}

	/*
	must be called from primary thread
	*/
	void start(ThreadResources& threadResources)
	{
		threadResources.taskShedular.pushPrimaryTask(0u, {this, [](void*const requester, ThreadResources& threadResources)
		{
			auto& manager = *static_cast<WorldManager*>(requester);

			manager.loadedPosition = (*manager.position / zoneRadius + 0.5f).floor() * zoneRadius;

			manager.load(threadResources);
			manager.currentLoadedZonesIndex ^= 1u;
			manager.run(threadResources);
		}});
	}

	/*
	Must be called from primary thread
	*/
	void stop(StopRequest& stopRequest, ThreadResources& threadResources)
	{
		stopRequest.stopRequest = &mStopRequest;
		threadResources.taskShedular.pushPrimaryTask(1u, {&stopRequest, [](void* requester, ThreadResources&)
		{
			StopRequest& stopRequest = *static_cast<StopRequest*>(requester);
			*stopRequest.stopRequest = &stopRequest;
		}});
	}

	/*
	Should be called whenever a new position should be followed.
	Must not be called from update1 when the manager is started.
	*/
	void setPosition(const Vector3& position1, unsigned long worldIndex)
	{
		currentWorldIndex = worldIndex;
		position = &position1;
	}
};