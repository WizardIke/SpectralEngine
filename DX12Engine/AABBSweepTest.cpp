#include <DirectXMath.h>
#include <algorithm>

class AABB
{
public:
	DirectX::XMFLOAT3 velocity;
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;
};

bool intersects(AABB& a, AABB& b) {
	float timeForAllAxesToHaveStartedOverlapping;
	float leastTimeForAnAxisToStopOverlapping;
	DirectX::XMFLOAT3 v;
	v.x = a.velocity.x - b.velocity.x;
	v.y = a.velocity.y - b.velocity.y;
	v.z = a.velocity.z - b.velocity.z;
	if(a.min.x >= b.max.x) {
		if (v.x <= 0.f) { return false; }
		else {timeForAllAxesToHaveStartedOverlapping = a.min.x - b.max.x / v.x;}
	}
	else if(b.min.x >= a.max.x) {
		if (v.x >= 0.f) { return false; }
		else {timeForAllAxesToHaveStartedOverlapping = a.max.x - b.min.x / v.x;}
	}
	else timeForAllAxesToHaveStartedOverlapping = 0.f;
	if(timeForAllAxesToHaveStartedOverlapping > 1.f) {return false;}
		
	if(a.min.z >= b.max.z) {
		if (v.z <= 0.f) { return false; }
		else {
			timeForAllAxesToHaveStartedOverlapping = 
			std::max(b.min.z - a.max.z / v.z, timeForAllAxesToHaveStartedOverlapping);
		}
	}
	else if(b.min.z >= a.max.z) {
		if (v.z >= 0.f) { return false; }
		else {
			timeForAllAxesToHaveStartedOverlapping = 
			std::max(a.max.z - b.min.z / v.z, timeForAllAxesToHaveStartedOverlapping);
		}
	}
	//else timeForAllAxesToHaveStartedOverlapping = std::max(0.f, overlapTime);
	if(timeForAllAxesToHaveStartedOverlapping > 1.f) {return false;}
	
	if(a.min.y >= b.max.y) {
		if (v.y <= 0.f) { return false; }
		else {
			timeForAllAxesToHaveStartedOverlapping = 
			std::max(b.min.y - a.max.y / v.y, timeForAllAxesToHaveStartedOverlapping);
		}
	}
	else if(b.min.y >= a.max.y) {
		if (v.y >= 0.f) { return false; }
		else {
			timeForAllAxesToHaveStartedOverlapping = 
			std::max(a.max.y - b.min.y / v.y, timeForAllAxesToHaveStartedOverlapping);
		}
	}
	if(timeForAllAxesToHaveStartedOverlapping > 1.f) {return false;}
	
	
	if(a.max.x > b.min.x) {
		if(v.x <= 0) leastTimeForAnAxisToStopOverlapping = 2.f;
		else {
			leastTimeForAnAxisToStopOverlapping = a.max.x - b.min.x / v.x;
			if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
				return false;
			}
		}
	}
	else{
		leastTimeForAnAxisToStopOverlapping = a.min.x - b.max.x / v.x;
		if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
			return false;
		}
	}
	
	if(a.max.z > b.min.z) {
		if(v.z <= 0) leastTimeForAnAxisToStopOverlapping = 2.f;
		else {
			leastTimeForAnAxisToStopOverlapping = 
				std::min(a.max.z - b.min.z / v.z, leastTimeForAnAxisToStopOverlapping);
			if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
				return false;
			}
		}
	}
	else{
		leastTimeForAnAxisToStopOverlapping = 
			std::min(a.min.z - b.max.z / v.z, leastTimeForAnAxisToStopOverlapping);
		if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
			return false;
		}
	}
	
	if(a.max.y > b.min.y) {
		if(v.y <= 0) leastTimeForAnAxisToStopOverlapping = 2.f;
		else {
			leastTimeForAnAxisToStopOverlapping = 
				std::min(a.max.y - b.min.y / v.y, leastTimeForAnAxisToStopOverlapping);
			if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
				return false;
			}
		}
	}
	else{
		leastTimeForAnAxisToStopOverlapping = 
			std::min(a.min.y - b.max.y / v.y, leastTimeForAnAxisToStopOverlapping);
		if(leastTimeForAnAxisToStopOverlapping < timeForAllAxesToHaveStartedOverlapping) {
			return false;
		}
	}
	
	return true;
}