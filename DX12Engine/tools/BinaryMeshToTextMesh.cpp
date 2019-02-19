#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include "VertexType.h"
using namespace std;

int main(int conut, char** strings)
{
	struct MeshType_PTN
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};
	ifstream fin(strings[1], ios::binary);
	if (fin.fail())
	{
		cout << "failed to open file" << strings[1];
		char input;
		cin >> input;
		return -1;
	}
	
	uint32_t compressedVertexType, unpackedVertexType, vertexCount, indexCount;
	fin.read(reinterpret_cast<char*>(&compressedVertexType), sizeof(compressedVertexType));
	fin.read(reinterpret_cast<char*>(&unpackedVertexType), sizeof(unpackedVertexType));
	fin.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
	fin.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
	if(compressedVertexType == VertexType::position3f)
	{
		cout << "Vertex Type: position3f\n";
		cout << "vertexCount: " << vertexCount << '\n';
		cout << "indexCount: " << indexCount << '\n' << '\n';
		
		Vertex_position3f vertex;
		while(vertexCount)
		{
			--vertexCount;
			fin.read(reinterpret_cast<char*>(&vertex), sizeof(vertex));
			cout << "position: {" << vertex.position[0] << ' ' << vertex.position[1] <<
				' ' << vertex.position[2] << "}\n";
		}
	}
	else if(compressedVertexType == VertexType::position3f_texCoords2f)
	{
		cout << "Vertex Type: position3f_texCoords2f\n";
		cout << "vertexCount: " << vertexCount << '\n';
		cout << "indexCount: " << indexCount << '\n' << '\n';
		
		Vertex_position3f_texCoords2f vertex;
		while(vertexCount)
		{
			--vertexCount;
			fin.read(reinterpret_cast<char*>(&vertex), sizeof(vertex));
			cout << "position: {" << vertex.position[0] << ' ' << vertex.position[1] <<
				' ' << vertex.position[2] << "}\n";
			cout << "texCoords: {" << vertex.texCoords[0] << ' ' << vertex.texCoords[1] << "}\n";
		}
	}
	else if(compressedVertexType == VertexType::position3f_texCoords2f_normal3f)
	{
		cout << "Vertex Type: position3f_texCoords2f_normal3f\n";
		cout << "vertexCount: " << vertexCount << '\n';
		cout << "indexCount: " << indexCount << '\n' << '\n';
		
		MeshType_PTN vertex;
		while(vertexCount)
		{
			--vertexCount;
			fin.read(reinterpret_cast<char*>(&vertex), sizeof(vertex));
			cout << "position: {" << vertex.x << ' ' << vertex.y << ' ' << vertex.z << "}\n";
			cout << "texcoords: {" << vertex.tu << ' ' << vertex.tv << "}\n";
			cout << "normal: {" << vertex.nx << ' ' << vertex.ny << ' ' << vertex.nz << "}\n";
		}
	}
	else
	{
		cout << "Vertex Type: invalid\n";
	}
	
	fin.close();
	cin.get();
}