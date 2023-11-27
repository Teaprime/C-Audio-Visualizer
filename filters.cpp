#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <filesystem>
#include <iostream>

namespace filters{

	static void effect(cv::Mat m, int delta, void (*func)(cv::Vec3b* ptr, int i, int j, int delta)) {
			for (int i = 0; i < m.rows; i++) {
				for (int j = 0; j < m.cols; j++) {
					cv::Vec3b* pix = m.ptr<cv::Vec3b>(i, j);
					func(pix, i, j, delta);
				}
			}

	}
	static void effect(cv::Mat m, void (*func)(cv::Vec3b* ptr, int i, int j)) {
			for (int i = 0; i < m.rows; i++) {
				for (int j = 0; j < m.cols; j++) {
					cv::Vec3b* pix = m.ptr<cv::Vec3b>(i, j);
					func(pix, i, j);
				}
			}

	}

	static void color(cv::Vec3b* ptr, int i, int j, int delta = 0) {
		double x = (j + delta) / (double)400;
		double y = ((i * 2) / (double)400) - 1;
		if (abs(sin(x) - y) < 0.01) {
			ptr[1] = 200;
		}
		else {
			ptr[0] = 0;
		}
	}

	static void bands(cv::Mat m, std::vector<float> bands) {
		int i = 0;
		int j = 0;
		while (i < m.cols && j < bands.size()) {
			cv::Rect r = cv::Rect(i, 0, int(m.cols/bands.size()), (int)bands.at(j));
			cv::rectangle(m, r, cv::Scalar(100 ,j*20,0,120),-1);
			i += (int)(m.cols / bands.size());
			j++;
		}
				
	}
}

namespace signal {
	static void getFlow(int start, int end, cv::VideoCapture m){
		cv::Mat frame1, prvs;
		m >> frame1;
		cv::cvtColor(frame1, prvs, cv::COLOR_BGR2GRAY);
		while (true) {
			cv::Mat frame2, next;
			m >> frame2;
			if (frame2.empty())
				break;
			cv::cvtColor(frame2, next, cv::COLOR_BGR2GRAY);
			cv::Mat flow(prvs.size(), CV_32FC2);
			cv::calcOpticalFlowFarneback(prvs, next, flow, 0.5, 3, 15, 3, 5, 1.2, 0);

			cv::Mat flow_parts[2];
			split(flow, flow_parts);
			cv::Mat magnitude, angle, magn_norm;
			cartToPolar(flow_parts[0], flow_parts[1], magnitude, angle, true);
			normalize(magnitude, magn_norm, 0.0f, 1.0f, cv::NORM_MINMAX);
			angle *= ((1.f / 360.f) * (180.f / 255.f));
			//build hsv image
			cv::Mat _hsv[3], hsv, hsv8, bgr;
			_hsv[0] = angle;
			_hsv[1] = cv::Mat::ones(angle.size(), CV_32F);
			_hsv[2] = magn_norm;
			merge(_hsv, 3, hsv);
			hsv.convertTo(hsv8, CV_8U, 255.0);
			cvtColor(hsv8, bgr, cv::COLOR_HSV2BGR);
			cv::imshow("frame2", bgr);
			int keyboard = cv::waitKey(30);
			if (keyboard == 'q' || keyboard == 27)
				break;
			prvs = next;
		}
	}
}