#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char** argv)
{
	if(argc < 3)
	{
		std::cout << "neededs the length between vertices and the output file name\n";
		return 1;
	}
	constexpr double length = 128.0;
	const double vertexLength = std::atof(argv[1]);
	
	std::ofstream outputFile{argv[2]};
	if(!outputFile)
	{
		std::cout << "failed to open output file\n";
		return 1;
	}
	
	unsigned long long vertexCountOnASide = 0u;
	const double end = length + vertexLength;
	for(double x = 0.0;;)
	{
		for(double z = 0.0;;)
		{
			outputFile << "v " << x << " " << 0.0 << " " << z << "\n";
			
			z += vertexLength;
			if(z == end)
			{
				break;
			}
			if(z > length)
			{
				z = length;
			}
		}
		++vertexCountOnASide;
		x += vertexLength;
		if(x == end)
		{
			break;
		}
		if(x > length)
		{
			x = length;
		}
	}
	outputFile << "\n";
	for(double x = 0.0;;)
	{
		for(double z = 0.0;;)
		{
			outputFile << "vt " << x / length << " " << 0.0 << " " << z / length << "\n";
			
			z += vertexLength;
			if(z == end)
			{
				break;
			}
			if(z > length)
			{
				z = length;
			}
		}
		x += vertexLength;
		if(x == end)
		{
			break;
		}
		if(x > length)
		{
			x = length;
		}
	}
	outputFile << "\n";
	outputFile << "vn 0 1 0\n";
	outputFile << "\n";
	outputFile << "o terrain\n";
	outputFile << "\n";
	const auto sideEnd = vertexCountOnASide - 1u;
	unsigned long long i = 0u;
	for(unsigned long long x = 0u; x != sideEnd; ++x)
	{
		for(unsigned long long z = 0u; z != sideEnd; ++z)
		{
			++i;
			outputFile << "f " << i << "/" << i << "/" << 1 << " "
				<< i + 1 << "/" << i + 1 << "/" << 1 << " "
				<< i + vertexCountOnASide + 1 << "/" << i + vertexCountOnASide + 1 << "/" << 1 << "\n";
			outputFile << "f " << i + vertexCountOnASide + 1 << "/" << i + vertexCountOnASide + 1 << "/" << 1 << " "
				<< i + vertexCountOnASide << "/" << i + vertexCountOnASide << "/" << 1 << " "
				<< i << "/" << i << "/" << 1 << "\n";
		}
		++i;
	}
	outputFile.close();
	std::cout << "done\n";
}