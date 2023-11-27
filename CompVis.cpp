// CompVis.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <chrono>

#include <iostream>
#include "filters.cpp"
#include <filesystem>
#define _USE_MATH_DEFINES
#include <math.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include <minimp3.h>
#include <minimp3_ex.h>

#define DJ_FFT_IMPLEMENTATION
#include <dj_fft.h>

using namespace cv;


static std::vector<uchar> normalizeFFT(const std::vector<std::complex<float>>& vec) {
    float sum = 0;
    for (int i = 0; i < vec.size(); i++) {
        //sum += sqrtf( pow(vec[i].real(), 2) * pow(vec[i].imag(), 2) );
        sum += abs(vec[i]);
        //float average = sum / vec.size();
    }
    std::vector<uchar> ret(vec.size());
    for (int i = 0; i < vec.size(); i++) {
        //char intensity = (pow(abs(vec[i])/sum, 2) / 3)*255;
        char intensity = (abs(vec[i])/sum) * 255;
        ret[i] = intensity;
    }
    return ret;
}

static void bands(cv::Mat m, std::vector<float> bands, int depth) {
    int i = 0;
    int j = 0;
    while (i < m.cols && j < bands.size()) {
        cv::Rect r = cv::Rect(i, 0, int(m.cols / bands.size()), (m.rows / depth) * (int)bands.at(j));
        cv::rectangle(m, r, cv::Scalar(100, j * 20, 0, 120), -1);
        i += (int)(m.cols / bands.size());
        j++;
    }

}

int main(int argc, char* argv[])
{
    std::string filename = "";
    int window = 512;
    int fps = 120;
    Size vidSize(1000, 1000);
    bool videoOutput = false;
    cv::String outputfile = "output.mp4";

    for (int i = 1; i < argc; i+=2) {
        if(argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'h':
                std::cout << "Input parameters\n-i : input file\n-f : fps of output. default : 120\n-w : window size (power of 2). default : 512\n-v : video output name";
                return 0;
            case 'i':
                filename = argv[i + 1];
                break;
            case 'w':
                window = atoi(argv[i + 1]);
                break;
            case 'f':
                fps = atoi(argv[i + 1]);
                break;
            case 'v':
                videoOutput = true;
                outputfile = argv[i + 1];
            default:
                break;
            }
        }
    }

    if (filename._Equal("")) {
        std::cout << "No input file, exiting";
        return 1;
    }
    if (fps == 0) {
        std::cout << "Fps can't be 0, exiting";
        return 1;
    }
	if (!((window & window-1) == 0 && window)) {
        std::cout << "Window is not power of 2, exiting";
        return 1;
	}

    mp3dec_t audio;
    mp3dec_file_info_t info;
    mp3dec_init(&audio);
    short bytes[MINIMP3_MAX_SAMPLES_PER_FRAME];

    

    //if (mp3dec_load(&audio, "C:/Users/zedan/Music/.mp3", &info, NULL, NULL)) {
    if (mp3dec_load(&audio, filename.c_str(), &info, NULL, NULL)) {
        std::cout << "Error opening audio file";
        return 1;
    }

	auto dir = dj::fft_dir::DIR_FWD;
    std::vector<std::vector<std::complex<float>>> out;
    //std::vector<std::vector<uchar>> out;
    int index = 0;

    std::cout << "Sample rate : " << info.hz << "\n";
    std::cout << "Samples : " << info.samples << "\n";
    std::cout << "Channels : " << info.channels << "\n";

    //const int window = 2048;
    const int increment = (info.channels*info.hz) / fps;
    int step = 0;
    const int split = 10;
    dj::fft_arg<float> vec = std::vector<std::complex<float>>(window);

    std::vector<float> max_float_found(window);
    while (index < info.samples - info.channels * window) {
        //dj::fft_arg<float> vec = std::vector<std::complex<float>>();
        vec.clear();

		for (int i = 0; i < window; i ++) {
            //float val = 20 * log10f(abs(info.buffer[index + i * info.channels]));
            vec.push_back(std::complex<float>(powf(sinf((M_PI * i)/float(window)),2) * info.buffer[index + i*info.channels],0));
            //vec.push_back(std::complex<float>(powf(sinf((M_PI * i)/float(window)),2) * val,0));
            //vec.push_back(std::complex<float>(info.buffer[index + i * info.channels], 0));
            //std::cout << info.buffer[index + i];
		}

        out.push_back(dj::fft1d(vec, dir));

        for (int i = 0; i < out.back().size(); i++) {
			//float comp_val = sqrt(pow(out.back()[i].real(),2) * pow(out.back()[i].imag(),2));
            float comp_val = abs(out.back()[i]);
            if (comp_val > max_float_found[i]) {
                max_float_found[i] = comp_val;
            }
        }
        index += increment;
        //index += window;
        if (index > step * (info.samples / split)) {
            std::cout << "FFT " << (double(100) / split)* ++step << "%\n";
        }
    }

    float avg_float = 0;
    float max_float = 0;
    for (float val : max_float_found) {
        avg_float += val;
        val > max_float ? max_float = val : max_float = max_float;
    }
    avg_float = avg_float / max_float_found.size();

    cv::Mat img(out[0].size()/2, out.size(), CV_8U);
    //const float fraction = max_float_found/20; //max_float_found/60;
    for (int i = 0; i < img.cols; i++) {
        //std::vector<uchar> val = normalizeFFT(out[i]);
        for (int j = 0; j < img.rows; j++) {
            char* pix = img.ptr<char>(j, i);

            int intensity = 40 * log10f((abs(out[i][j])/max_float)); // dbfs
            //uchar intensity = abs(out[i][j]) / max_float * 255; // linear

            intensity < -255 ? intensity = 0 : intensity = intensity;
            //*pix = { uchar((intensity/max_float_found[j]) * 255), uchar((intensity / max_float_found[j]) * 255), uchar((intensity / max_float_found[j]) * 255)};
            //*pix = { uchar(0x000000FF & intensity), uchar(0x0000FF00 & intensity), uchar(0x00FF0000 & intensity)};
            *pix = intensity;
        }
    }
    imwrite("output.png", img);
    GaussianBlur(img, img, Size(21,1), 0, 0, BorderTypes::BORDER_REFLECT);

    /*for (auto val : max_float_found) {
        std::cout << val << "\n";
    }*/
    //std::cout << "Max float found : " << max_float_found[0];
    if (videoOutput) {
        std::cout << "Generating video...\n";
        VideoWriter vw(outputfile, VideoWriter::fourcc('m', 'p', '4', 'v'), fps, vidSize, true);
        int progress = 0;
        for (int i = 0; i < img.cols; i++) {
            Mat frame(vidSize, CV_8UC3, Scalar(0));
            std::vector<float> row(20, 0);

            int k = 0;
            while (k * img.rows / row.size() < img.rows - row.size()) {
                for (int j = 0; j < row.size(); j++) {
                    uint8_t val = /*sin(j * M_PI / row.size()) * */img.at<uint8_t>((k * img.rows / row.size()) + j, i);
                    row.at(k) += val;
                }
                row.at(k) /= row.size();
                k++;
            }

            bands(frame, row, 255);
            vw.write(frame);
            if (i > progress * (img.cols / split)) {
                std::cout << "Video " << (double(100) / split)* ++progress << "%\n";
            }
        }
        vw.release();

    }


    std::cout << "Done";
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
