// Kanaan Sullivan
// CS-5420
// Project 2 - Image Corpus Creator

#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;
using namespace cv;
using namespace std;


// Get user input for the image source
string getSourceInput(const string& imgPath) {
    string source;
    cout << "Enter the source for image: " << imgPath << endl;
    getline(cin, source);
    return source;
}

void procImage(const fs::path& input, const fs::path output, const fs::path& inputDir, bool presAspect, bool gray, int maxRows, int maxCols, const string& outputType, FileStorage& metadata) {
    // Attempt to read the image
    try {
        Mat image = imread(input, IMREAD_COLOR);
        if (image.empty()) {
            throw runtime_error("Could not read the image: " + input.string());
        }

            //  Gather the image's original dimensions
        int orgRows = image.rows;
        int orgCols = image.cols;

        Mat resizedImage;
        if (presAspect) {
            double aspectRatio = static_cast<double>(orgCols) / orgRows;
            if (orgCols > maxCols || orgRows > maxRows) {
                if (aspectRatio > 1) {
                    // If Width is greater 
                    resize(image, resizedImage, Size(maxCols, maxCols / aspectRatio));
                } else {
                    // If Height is greater
                    resize(image, resizedImage, Size(maxRows * aspectRatio, maxRows));
                }
            } else {
                resizedImage = image.clone();
            }
        } else {
            // Resize without preservation
            resize(image, resizedImage, Size(maxCols, maxRows));
        }

        // If specified, convert image to grayscale
        if (gray) {
            cvtColor(resizedImage, resizedImage, COLOR_BGR2GRAY);
        }

        // Create the output subdirectory structure and determine output file path
        fs::path relativePath = fs::relative(input, inputDir);

        // Create the corresponding output path by combining the output directory and relative path
        fs::path outputFilePath = output / relativePath;

        // Update the output file extension
        outputFilePath.replace_extension(outputType);

        // Create any required subdirectories in the output path
        fs::create_directories(outputFilePath.parent_path());

        // Save the processed image
        imwrite(outputFilePath.string(), resizedImage);


        // gather the image source
        string source = getSourceInput(input.string());

        // Write metadata in XML format
        metadata << "Image" << "{";
        metadata << "Filename" << outputFilePath.string();
        metadata << "Source" << source;
        metadata << "Original_Dimensions" << "[" << orgCols << orgRows << "]";
        metadata << "New_Dimensions" << "[" << resizedImage.cols << resizedImage.rows << "]";
        metadata << "}";
        


    } catch(const Exception& ex) {
        cerr << "OpenCV error: " << ex.what() << endl;
    } catch(const runtime_error& ex) {
        cerr << "Runtime Error: " << ex.what() << endl;
    }

}

int main(int argc, char** argv) {
    // Define command line arguments
    const string keys =
        "{help h    |      | Print help message}"
        "{a         |      | Preserve aspect ratio}"
        "{g         |      | Save output image as grayscale}"
        "{r rows    |480   | Maximum number of rows in the output image}"
        "{c cols    |640   | Maximum number of columns in the output image}"
        "{t type    |jpg   | Output image type (jpg, png, bmp, tif)}"
        "{@indir    |      | Input directory containing images}"
        "{@outdir   |      | Output directory for processed images (default: indir.corpus)}";

    CommandLineParser parser(argc, argv, keys);
    parser.about("Image Corpus Builder");

    if (parser.has("help") || argc == 1) {
        parser.printMessage();
        return 0;
    }

    bool preserveAspectRatio = parser.has("a");
    bool grayscale = parser.has("g");
    int maxRows = parser.get<int>("rows");
    int maxCols = parser.get<int>("cols");
    string outputType = parser.get<string>("type");
    string inputDir = parser.get<string>("@indir");
    string outputDir = parser.get<string>("@outdir");

    // Use input directory if no output directory is specified
    if (outputDir.empty()) {
        outputDir = inputDir + ".corpus";
    }



    // Create output directory if it doesn't exist
    fs::create_directories(outputDir);


    // Open Metadata File
    FileStorage metadata(outputDir + "/metadata.xml", FileStorage::WRITE);
    metadata << "Images" << "[";

    // Process all images and directories
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if(entry.is_regular_file()) {
            procImage(entry.path(), outputDir, inputDir, preserveAspectRatio, grayscale, maxRows, maxCols, outputType, metadata);
        }
    }

    // Close the metadata
    metadata << "]";
    metadata.release();

    cout << "Image corpus processing completed successfully" << endl;

    return 0;
}


