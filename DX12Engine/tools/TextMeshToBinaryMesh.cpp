#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include "VertexType.h"
using namespace std;

int main(int conut, char** strings)
{
	struct MeshType_PT
	{
		float x, y, z;
		float tu, tv;
	};
	struct MeshHeader
	{
		uint32_t vertexType;
		uint32_t vertexCount;
		uint32_t indexCount;
	};
	
	ifstream fin;
	ofstream fout;

	char input;
	fin.open(strings[1]);
	if (fin.fail())
	{
		cout << "failed to open file" << strings[1];
		cin >> input;
		return -1;
	}
	std::string inputPath = strings[1];
	std::string path = inputPath.substr(0, inputPath.size() - 3) + "mesh";
	fout.open(path, ios::binary);
	if (fout.fail())
	{
		cout << "Failed to open file: " << path;
		cin >> input;
		return -1;
	}
	
	
	do
	{
		fin >> input;
	}while(input != ':');
	std::string vertexType;
	fin.get();
	//fin >> input;
	std::getline(fin, vertexType);
	
	do
	{
		fin >> input;
	}while(input != ':');
	uint32_t vertexCount;
	fin >> vertexCount;
	
	std::string newVertexType;
	if(conut >= 3) 
	{
		newVertexType = strings[2];
	}
	else
	{
		newVertexType = vertexType;
	}
	
	MeshHeader header;
	if(newVertexType == "position3f")
	{
		header.vertexType = VertexType::position3f;
	}
	else if(newVertexType == "position3f_texCoords2f")
	{
		header.vertexType = VertexType::position3f_texCoords2f;
	}
	else if(newVertexType == "position3f_texCoords2f_normal3f")
	{
		header.vertexType = VertexType::position3f_texCoords2f_normal3f;
	}
	else
	{
		cout << "invalid new vertex type: " << newVertexType;
		return 0;
	}
	
	header.vertexCount = vertexCount;
	header.indexCount = 0u;
	fout.write((char*)&header, sizeof(header));
	
	do
	{
		fin >> input;
	} while(input != ':');
	if(newVertexType == "position3f")
	{
		Vertex_position3f vertex;
		if(vertexType == "position3f_texCoords2f_normal3f")
		{
			float temp;
			for(unsigned long i = 0u; i < vertexCount; ++i)
			{
				cout << "working..\n";
				fin >> vertex.position[0] >> vertex.position[1] >> vertex.position[2] >> 
					temp >> temp >> temp >> temp >> temp;
		
				fout.write((char*)&vertex, sizeof(vertex));
			}
			std::cout << "file converted\n";
		}
		else
		{
			std::cout << "invalid vertex type: " << vertexType;
		}
	}
	else if(newVertexType == "position3f_texCoords2f")
	{
		MeshType_PT meshInput;
		if(vertexType == "position3f_texCoords2f_normal3f")
		{
			float temp;
			for(unsigned long i = 0u; i < vertexCount; ++i)
			{
				cout << "working..\n";
				fin >> meshInput.x >> meshInput.y >> meshInput.z >> meshInput.tu >> meshInput.tv >> temp >> temp >> temp;
		
				fout.write((char*)&meshInput, sizeof(meshInput));
			}
			std::cout << "file converted\n";
		}
		else if(vertexType == "position3f_texCoords2f")
		{
			for(unsigned long i = 0u; i < vertexCount; ++i)
			{
				cout << "working..\n";
				fin >> meshInput.x >> meshInput.y >> meshInput.z >> meshInput.tu >> meshInput.tv;
		
				fout.write((char*)&meshInput, sizeof(meshInput));
			}
			std::cout << "file converted\n";
		}
		else
		{
			std::cout << "invalid vertex type: " << vertexType;
		}
	}
	else if(newVertexType == "position3f_texCoords2f_normal3f")
	{
		Vertex_position3f_texCoords2f_normal3f vertex;
		if(vertexType == "position3f_texCoords2f_normal3f")
		{
			for(unsigned long i = 0u; i < vertexCount; ++i)
			{
				cout << "working..\n";
				fin >> vertex.position[0] >> vertex.position[1] >>
					vertex.position[2] >> vertex.texCoords[0] >>
					vertex.texCoords[1] >> vertex.normal[0]
					>> vertex.normal[1] >> vertex.normal[2];
		
				fout.write((char*)&vertex, sizeof(vertex));
			}
			std::cout << "file converted\n";
		}
		else
		{
			std::cout << "invalid vertex type" << vertexType;
		}
	}
	
	
	fin.close();
	fout.close();
	
	cin.get();
	return 0;
}