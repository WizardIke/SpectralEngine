#include <fstream>
#include <iostream>
#include <string>
using namespace std;

int main()
{
	cout << "enter the length between vertices, mush be power of 2\n";
	float vertexLength;
	cin >> vertexLength;
	
	unsigned int lengthX, lengthZ;
	lengthX = lengthZ = 128;
	unsigned int vertexCount = lengthX * lengthZ * 6 / vertexLength / vertexLength;
	
	ofstream fout;
	
	cout << "enter the file name  ";
	string name;
	cin >> name;
	string path = "c:\\users\\isaac\\desktop\\" + name + ".txt";

	fout.open(path);
	if (fout.fail())
	{
		return -1;
	}
	
	fout << "Vertex Type: position3f_texCoords2f_normal3f\n";
	fout << "Vertex count: " << vertexCount << "\n\n" << "Data:\n\n";
	
	for(unsigned int i = 0; i < lengthX / vertexLength; i++)
	{
		for(unsigned int k = 0; k < lengthZ / vertexLength; ++k)
		{
			cout << "working...\n";
			//triangle one
			fout << i * vertexLength << ' ' << 0.0 << ' ' << k * vertexLength << ' ' << 0.0 << ' ' << 0.0 << ' ' << 0.0 <<  ' ' << 1.0 << ' ' << 0.0 << "\n";
			fout << (i + 1) * vertexLength << ' ' << 0.0 << ' ' << (k + 1) * vertexLength << ' ' << 1.0 << ' ' << 1.0 << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << "\n";
			fout << (i + 1) * vertexLength << ' ' << 0.0 << ' ' << k * vertexLength << ' ' << 1.0 << ' ' << 0.0 << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << "\n";
			//triangle two
			fout << i * vertexLength << ' ' << 0.0 << ' ' << k * vertexLength << ' ' << 0.0 << ' ' << 0.0 << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << "\n";
			fout << i * vertexLength << ' ' << 0.0 << ' ' << (k + 1) * vertexLength << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << "\n";
			fout << (i + 1) * vertexLength << ' ' << 0.0 << ' ' << (k + 1) * vertexLength << ' ' << 1.0 << ' ' << 1.0 << ' ' << 0.0 << ' ' << 1.0 << ' ' << 0.0 << "\n";
		}
	}
	
	// Close the model file.
	fout.close();
	
	std::cout << "file made";
	
	return 0;
}