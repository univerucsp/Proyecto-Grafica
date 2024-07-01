#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

void scaleOBJ(const std::string& inputFileName, const std::string& outputFileName, float scale) {
    std::ifstream inputFile(inputFileName);
    std::ofstream outputFile(outputFileName);

    if (!inputFile.is_open()) {
        std::cerr << "Error opening input file!" << std::endl;
        return;
    }
    if (!outputFile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            x *= scale;
            y *= scale;
            z *= scale;
            outputFile << "v " << x << " " << y << " " << z << "\n";
        } else {
            outputFile << line << "\n";
        }
    }

    inputFile.close();
    outputFile.close();
}

int main() {
    std::string inputFileName = "table.obj";
    std::string outputFileName = "table2.obj";
    float scale = 30.0f;

    scaleOBJ(inputFileName, outputFileName, scale);

    std::cout << "OBJ file scaled and saved to " << outputFileName << std::endl;

    return 0;
}
