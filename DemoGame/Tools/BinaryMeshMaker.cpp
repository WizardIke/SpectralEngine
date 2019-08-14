#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include "VertexType.h"
using namespace std;

int main()
{
	cout << "enter the length between vertices, mush be power of 2\n";
	float vertexLength;
	cin >> vertexLength;
	
	unsigned int lengthX, lengthZ;
	lengthX = lengthZ = 128;
	uint32_t vertexCount = (float)(lengthX * lengthZ * 6) / vertexLength / vertexLength;
	
	ofstream fout;
	
	cout << "enter the file name  ";
	string name;
	cin >> name;
	string path = name + ".mesh";

	fout.open(path, ios::binary);
	if (fout.fail())
	{
		cout << "Failed to open file: " << path;
		return -1;
	}
	uint32_t compressedVertexType = VertexType::position3f_texCoords2f_normal3f;
	fout.write((char*)&compressedVertexType, sizeof(compressedVertexType));
	uint32_t unpackedVertexType = VertexType::position3f_texCoords2f_normal3f;
	fout.write((char*)&unpackedVertexType, sizeof(unpackedVertexType));
	fout.write((char*)&vertexCount, sizeof(vertexCount));
	uint32_t indexCount = 0u;
	fout.write((char*)&indexCount, sizeof(indexCount));
	
	uint32_t numTriX = (float)lengthX / vertexLength;
	uint32_t numTriZ = (float)lengthZ / vertexLength;
	for(unsigned int i = 0; i < numTriX; i++)
	{
		for(unsigned int k = 0; k < numTriZ; ++k)
		{
			Vertex_position3f_texCoords2f_normal3f vert1{i * vertexLength, 0.0f,
				k * vertexLength, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
			Vertex_position3f_texCoords2f_normal3f vert2{(i + 1) * vertexLength, 0.0f,
				(k + 1) * vertexLength, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f};
			Vertex_position3f_texCoords2f_normal3f vert3{(i + 1) * vertexLength, 0.0f,
				k * vertexLength, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
			Vertex_position3f_texCoords2f_normal3f vert4{i * vertexLength, 0.0f,
				(k + 1) * vertexLength, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
				
			//triangle one
			fout.write((char*)&vert3, sizeof(vert3));
			fout.write((char*)&vert1, sizeof(vert1));
			fout.write((char*)&vert2, sizeof(vert2));
			
			//triangle two
			fout.write((char*)&vert2, sizeof(vert2));
			fout.write((char*)&vert1, sizeof(vert1));
			fout.write((char*)&vert4, sizeof(vert4));
		}
	}
	
	// Close the model file.
	fout.close();
	
	std::cout << "file made";
	
	return 0;
}