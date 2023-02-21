#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

int main(int argc,char* argv[])
{
    typedef unsigned char byte;

    if(argc > 1)
    {
        // Find and Read in image properties file
        std::string input_file_name{argv[1]};
        std::ifstream input_file(input_file_name);
        std::string image_properties_string;
        if ( input_file.is_open() ) {
            std::getline(input_file, image_properties_string);
        } else {
            std::cout << "ERROR: Issue reading input file '" << input_file_name << "'. " << "Please verify path." << std::endl;
            return 0;
        }
        
        // Parse tokens from image properties file
        std::vector<std::string> image_properties;
        std::istringstream ss(image_properties_string);
        std::string del;
        while(std::getline(ss, del, ' ')) {
            if (del != "") {
                image_properties.push_back(del);
            }
        }

        if (image_properties.size() != 3) {
            std::cout << "ERROR: Incorrect number of arguements in input file. Please follow this formate: imsize width height" << std::endl;
            return 0;
        }

        if (image_properties[0] != "imsize") {
            std::cout << "ERROR: Invalid parameter " << "'imsize'" << " found in input file. Please verify." << std::endl;
            return 0;
        }

        // Extract height and width. Validate Correctness.
        int height, width;
        try
        {
            height = std::stoi(image_properties[1]);
        }
        catch(const std::exception& e)
        {
            std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
            return 0;
        }

        try
        {
            width = std::stoi(image_properties[2]);
        }
        catch(const std::exception& e)
        {
            std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
            return 0;
        } 

        if (height <= 1 || width <= 1) {
            std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
            return 0;
        }

        // Define matrix that stores image data
        using ImageMat = std::vector<std::vector<std::vector<byte>>>;
        ImageMat matt(height, std::vector<std::vector<byte>>(width, std::vector<byte>(3,0)));

        std::ofstream image_stream;		//output stream object
        image_stream.open("test.ppm");

        if (image_stream.fail())
        {
            std::cout << "ERROR: failed to create ppm image" << std::endl;
            return 0;
        }

        // Create the image header
        image_stream << "P3 " << std::endl;
		image_stream << width << " " << height << " " << std::endl;		
		image_stream << "255 " << std::endl;

        // Create the image body
        for (int j = 0u; j < height; j++) {
            for (int i = 0u; i < width; i++) {
               if (i == j) {
                    matt[j][i][0] = 125;
                    matt[j][i][1] = 125;
                    matt[j][i][2] = 125;
               }
            }
        }

        // Now print image to file
        for (int j = 0u; j < height; j++) {
            for (int i = 0u; i < width; i++) {
                image_stream << std::to_string(matt[j][i][0]) << " ";
                image_stream << std::to_string(matt[j][i][1]) << " ";
                image_stream << std::to_string(matt[j][i][2]) << " " << std::endl;
            }
        }


        image_stream.close();

    } else {
        std::cout << "Error: Incorrect number of arguements in input file. Please follow this formate: imsize width height" << std::endl;
    }

    return 0;
}
