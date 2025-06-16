//#include <opencv2/opencv.hpp>
//#include <fstream>
//#include <vector>
//#include <iostream>
//#include <string>
//#include <algorithm>
//#include <random> // 用于生成更真实的随机噪声
//#include <numeric> // 用于计算标准差
//
//using namespace std;
//using namespace cv;
//
//// --- 全局变量 ---
//Mat original_img;
//Mat temp_img;
//vector<float> yvals; // 存储所有原始数据
//
//					 // 缩放选择框
//Rect zoom_box;
//bool drawing_zoom_box = false;
//
//// 修复选择框
//Rect repair_box;
//bool drawing_repair_box = false;
//
//// 用于生成随机数
//mt19937 rng(random_device{}()); // 高质量的随机数生成器
//
//								// --- 函数声明 ---
//void plotCurve(const vector<float>& data, Mat& targetImg, const string& windowName);
//void repairArtifact(vector<float>& data, int startIndex, int endIndex);
//
//
//// 重新绘制曲线的函数
//void plotCurve(const vector<float>& data, Mat& targetImg, const string& windowName) {
//	if (data.empty()) {
//		cerr << "没有数据可以绘制" << endl;
//		return;
//	}
//
//	// 清空图像，以防重绘
//	targetImg.setTo(Scalar(255, 255, 255));
//
//	float minVal = *min_element(data.begin(), data.end());
//	float maxVal = *max_element(data.begin(), data.end());
//
//	if (maxVal == minVal) {
//		line(targetImg, Point(0, targetImg.rows / 2), Point(targetImg.cols - 1, targetImg.rows / 2), Scalar(0, 0, 255), 1);
//	}
//	else {
//		float x_step = (float)targetImg.cols / (data.size() > 1 ? data.size() - 1 : 1);
//		for (size_t i = 1; i < data.size(); ++i) {
//			int y1 = targetImg.rows - 1 - int((data[i - 1] - minVal) / (maxVal - minVal) * (targetImg.rows - 1));
//			int y2 = targetImg.rows - 1 - int((data[i] - minVal) / (maxVal - minVal) * (targetImg.rows - 1));
//			int x1 = int((i - 1) * x_step);
//			int x2 = int(i * x_step);
//			line(targetImg, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 1);
//		}
//	}
//
//	imshow(windowName, targetImg);
//}
//
///**
//* @brief 修复数据中的异常区域(artifact)
//* @param data 完整的数据序列 (会被直接修改)
//* @param startIndex 异常区域的起始索引
//* @param endIndex 异常区域的结束索引
//*/
//void repairArtifact(vector<float>& data, int startIndex, int endIndex) {
//	// 1. 定义一个 "上下文" 窗口大小，用于分析周围的噪声特征
//	const int context_size = 30; // 可以调整这个值
//
//								 // 2. 检查边界条件，确保有足够的上下文数据
//	if (startIndex < context_size || endIndex >(int)data.size() - 1 - context_size) {
//		cout << "修复区域太靠近数据边缘，无法获取足够的上下文信息。操作取消。" << endl;
//		return;
//	}
//
//	// 3. 从上下文窗口中提取 "纯噪声" (即数据减去局部趋势)
//	vector<float> local_noise;
//	local_noise.reserve(context_size * 2);
//
//	// 分析 "之前" 的上下文
//	float trend_start_before = data[startIndex - context_size];
//	float trend_end_before = data[startIndex - 1];
//	for (int i = 0; i < context_size; ++i) {
//		float trend_val = trend_start_before + (trend_end_before - trend_start_before) * ((float)i / (context_size - 1));
//		local_noise.push_back(data[startIndex - context_size + i] - trend_val);
//	}
//
//	// 分析 "之后" 的上下文
//	float trend_start_after = data[endIndex + 1];
//	float trend_end_after = data[endIndex + context_size];
//	for (int i = 0; i < context_size; ++i) {
//		float trend_val = trend_start_after + (trend_end_after - trend_start_after) * ((float)i / (context_size - 1));
//		local_noise.push_back(data[endIndex + 1 + i] - trend_val);
//	}
//
//	// 4. 计算噪声的标准差，这是噪声“强度”的度量
//	float sum = std::accumulate(local_noise.begin(), local_noise.end(), 0.0f);
//	float mean = sum / local_noise.size();
//	float sq_sum = std::inner_product(local_noise.begin(), local_noise.end(), local_noise.begin(), 0.0f);
//	float std_dev = std::sqrt(sq_sum / local_noise.size() - mean * mean);
//
//	cout << "局部噪声标准差: " << std_dev << endl;
//
//	// 5. 进行修复
//	// 修复区域的起点和终点值
//	float repair_start_val = data[startIndex - 1];
//	float repair_end_val = data[endIndex + 1];
//	int repair_length = endIndex - startIndex + 1;
//
//	// 创建一个符合局部噪声特征的正态分布
//	normal_distribution<float> dist(0.0f, std_dev);
//
//	for (int i = 0; i < repair_length; ++i) {
//		// a. 计算线性插值的基线值 (整体趋势)
//		float baseline = repair_start_val + (repair_end_val - repair_start_val) * ((float)(i + 1) / (repair_length + 1));
//
//		// b. 添加模拟的随机噪声
//		float noise = dist(rng);
//
//		// c. 将新生成的值赋给数据
//		data[startIndex + i] = baseline + noise;
//	}
//
//	cout << "数据修复完成: " << repair_length << " 个点已被替换。" << endl;
//}
//
//
//// 鼠标回调函数
//void onMouse(int event, int x, int y, int flags, void* userdata) {
//	// 处理左键拖动 (缩放)
//	if (event == EVENT_LBUTTONDOWN) {
//		drawing_zoom_box = true;
//		zoom_box = Rect(x, y, 0, 0);
//	}
//	else if (event == EVENT_MOUSEMOVE && drawing_zoom_box) {
//		zoom_box.width = x - zoom_box.x;
//		zoom_box.height = y - zoom_box.y;
//	}
//	else if (event == EVENT_LBUTTONUP && drawing_zoom_box) {
//		drawing_zoom_box = false;
//		if (zoom_box.width < 0) { zoom_box.x += zoom_box.width; zoom_box.width *= -1; }
//		if (zoom_box.height < 0) { zoom_box.y += zoom_box.height; zoom_box.height *= -1; }
//
//		if (zoom_box.width < 5 || zoom_box.height < 5) return;
//
//		// --- 缩放逻辑 ---
//		int start_index = (int)((float)zoom_box.x / temp_img.cols * yvals.size());
//		int end_index = (int)((float)(zoom_box.x + zoom_box.width) / temp_img.cols * yvals.size());
//		start_index = max(0, start_index);
//		end_index = min((int)yvals.size(), end_index);
//
//		if (start_index >= end_index) return;
//
//		vector<float> zoomed_yvals(yvals.begin() + start_index, yvals.begin() + end_index);
//		Mat zoomed_img(600, 800, CV_8UC3);
//		plotCurve(zoomed_yvals, zoomed_img, "Zoomed View");
//	}
//
//	// 处理中键拖动 (修复)
//	else if (event == EVENT_MBUTTONDOWN) {
//		drawing_repair_box = true;
//		repair_box = Rect(x, y, 0, 0);
//	}
//	else if (event == EVENT_MOUSEMOVE && drawing_repair_box) {
//		repair_box.width = x - repair_box.x;
//		repair_box.height = y - repair_box.y;
//	}
//	else if (event == EVENT_MBUTTONUP && drawing_repair_box) {
//		drawing_repair_box = false;
//		if (repair_box.width < 0) { repair_box.x += repair_box.width; repair_box.width *= -1; }
//		// Y 轴高度不重要，我们只关心X轴范围
//
//		if (repair_box.width < 5) return;
//
//		// --- 修复逻辑 ---
//		int start_index = (int)((float)repair_box.x / temp_img.cols * yvals.size());
//		int end_index = (int)((float)(repair_box.x + repair_box.width) / temp_img.cols * yvals.size());
//
//		// 调用修复函数
//		repairArtifact(yvals, start_index, end_index);
//
//		// 修复后，原始数据已变，需要重绘主窗口
//		plotCurve(yvals, original_img, "Second Column Curve");
//	}
//}
//
//int main() {
//	// ... (文件读取部分与之前完全相同) ...
//	string filename = "Series1.txt";
//	ifstream infile(filename);
//	if (!infile) { cerr << "无法打开文件: " << filename << endl; return -1; }
//	string line;
//	while (getline(infile, line)) {
//		if (line.empty()) continue;
//		istringstream iss(line);
//		float col1, col2;
//		if (!(iss >> col1 >> col2)) continue;
//		yvals.push_back(col2);
//	}
//	infile.close();
//	if (yvals.empty()) { cerr << "没有读取到任何数据" << endl; return -1; }
//
//
//	// 定义初始画布尺寸
//	int width = 1200;
//	int height = 400;
//
//	original_img = Mat(height, width, CV_8UC3);
//	plotCurve(yvals, original_img, "Second Column Curve");
//
//	setMouseCallback("Second Column Curve", onMouse, NULL);
//
//	cout << "操作指南:" << endl;
//	cout << " - [左键] 拖动: 选择一个区域进行放大." << endl;
//	cout << " - [中键] 拖动: 选择一个异常区域进行修复." << endl;
//	cout << " - 按 'z' 键: 关闭所有放大窗口." << endl;
//	cout << " - 按 'q' 或 'Esc' 键: 退出程序." << endl;
//
//	// --- 交互式主循环 ---
//	while (true) {
//		original_img.copyTo(temp_img);
//
//		// 实时绘制选择框
//		if (drawing_zoom_box) {
//			rectangle(temp_img, zoom_box, Scalar(0, 255, 0), 1); // 绿色框用于缩放
//		}
//		if (drawing_repair_box) {
//			rectangle(temp_img, repair_box, Scalar(255, 0, 255), 1); // 紫色框用于修复
//		}
//
//		imshow("Second Column Curve", temp_img);
//
//		char key = (char)waitKey(15);
//		if (key == 'q' || key == 27) {
//			break;
//		}
//		if (key == 'z') { // 'z' for zoom-reset
//			cout << "放大窗口已关闭." << endl;
//			destroyWindow("Zoomed View");
//		}
//	}
//
//	destroyAllWindows();
//	return 0;
//}