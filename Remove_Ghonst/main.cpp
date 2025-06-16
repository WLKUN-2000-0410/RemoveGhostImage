//#include <opencv2/opencv.hpp>
//#include <fstream>
//#include <vector>
//#include <iostream>
//#include <string>
//#include <algorithm>
//#include <random> // �������ɸ���ʵ���������
//#include <numeric> // ���ڼ����׼��
//
//using namespace std;
//using namespace cv;
//
//// --- ȫ�ֱ��� ---
//Mat original_img;
//Mat temp_img;
//vector<float> yvals; // �洢����ԭʼ����
//
//					 // ����ѡ���
//Rect zoom_box;
//bool drawing_zoom_box = false;
//
//// �޸�ѡ���
//Rect repair_box;
//bool drawing_repair_box = false;
//
//// �������������
//mt19937 rng(random_device{}()); // �������������������
//
//								// --- �������� ---
//void plotCurve(const vector<float>& data, Mat& targetImg, const string& windowName);
//void repairArtifact(vector<float>& data, int startIndex, int endIndex);
//
//
//// ���»������ߵĺ���
//void plotCurve(const vector<float>& data, Mat& targetImg, const string& windowName) {
//	if (data.empty()) {
//		cerr << "û�����ݿ��Ի���" << endl;
//		return;
//	}
//
//	// ���ͼ���Է��ػ�
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
//* @brief �޸������е��쳣����(artifact)
//* @param data �������������� (�ᱻֱ���޸�)
//* @param startIndex �쳣�������ʼ����
//* @param endIndex �쳣����Ľ�������
//*/
//void repairArtifact(vector<float>& data, int startIndex, int endIndex) {
//	// 1. ����һ�� "������" ���ڴ�С�����ڷ�����Χ����������
//	const int context_size = 30; // ���Ե������ֵ
//
//								 // 2. ���߽�������ȷ�����㹻������������
//	if (startIndex < context_size || endIndex >(int)data.size() - 1 - context_size) {
//		cout << "�޸�����̫�������ݱ�Ե���޷���ȡ�㹻����������Ϣ������ȡ����" << endl;
//		return;
//	}
//
//	// 3. �������Ĵ�������ȡ "������" (�����ݼ�ȥ�ֲ�����)
//	vector<float> local_noise;
//	local_noise.reserve(context_size * 2);
//
//	// ���� "֮ǰ" ��������
//	float trend_start_before = data[startIndex - context_size];
//	float trend_end_before = data[startIndex - 1];
//	for (int i = 0; i < context_size; ++i) {
//		float trend_val = trend_start_before + (trend_end_before - trend_start_before) * ((float)i / (context_size - 1));
//		local_noise.push_back(data[startIndex - context_size + i] - trend_val);
//	}
//
//	// ���� "֮��" ��������
//	float trend_start_after = data[endIndex + 1];
//	float trend_end_after = data[endIndex + context_size];
//	for (int i = 0; i < context_size; ++i) {
//		float trend_val = trend_start_after + (trend_end_after - trend_start_after) * ((float)i / (context_size - 1));
//		local_noise.push_back(data[endIndex + 1 + i] - trend_val);
//	}
//
//	// 4. ���������ı�׼�����������ǿ�ȡ��Ķ���
//	float sum = std::accumulate(local_noise.begin(), local_noise.end(), 0.0f);
//	float mean = sum / local_noise.size();
//	float sq_sum = std::inner_product(local_noise.begin(), local_noise.end(), local_noise.begin(), 0.0f);
//	float std_dev = std::sqrt(sq_sum / local_noise.size() - mean * mean);
//
//	cout << "�ֲ�������׼��: " << std_dev << endl;
//
//	// 5. �����޸�
//	// �޸�����������յ�ֵ
//	float repair_start_val = data[startIndex - 1];
//	float repair_end_val = data[endIndex + 1];
//	int repair_length = endIndex - startIndex + 1;
//
//	// ����һ�����Ͼֲ�������������̬�ֲ�
//	normal_distribution<float> dist(0.0f, std_dev);
//
//	for (int i = 0; i < repair_length; ++i) {
//		// a. �������Բ�ֵ�Ļ���ֵ (��������)
//		float baseline = repair_start_val + (repair_end_val - repair_start_val) * ((float)(i + 1) / (repair_length + 1));
//
//		// b. ���ģ����������
//		float noise = dist(rng);
//
//		// c. �������ɵ�ֵ��������
//		data[startIndex + i] = baseline + noise;
//	}
//
//	cout << "�����޸����: " << repair_length << " �����ѱ��滻��" << endl;
//}
//
//
//// ���ص�����
//void onMouse(int event, int x, int y, int flags, void* userdata) {
//	// ��������϶� (����)
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
//		// --- �����߼� ---
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
//	// �����м��϶� (�޸�)
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
//		// Y ��߶Ȳ���Ҫ������ֻ����X�᷶Χ
//
//		if (repair_box.width < 5) return;
//
//		// --- �޸��߼� ---
//		int start_index = (int)((float)repair_box.x / temp_img.cols * yvals.size());
//		int end_index = (int)((float)(repair_box.x + repair_box.width) / temp_img.cols * yvals.size());
//
//		// �����޸�����
//		repairArtifact(yvals, start_index, end_index);
//
//		// �޸���ԭʼ�����ѱ䣬��Ҫ�ػ�������
//		plotCurve(yvals, original_img, "Second Column Curve");
//	}
//}
//
//int main() {
//	// ... (�ļ���ȡ������֮ǰ��ȫ��ͬ) ...
//	string filename = "Series1.txt";
//	ifstream infile(filename);
//	if (!infile) { cerr << "�޷����ļ�: " << filename << endl; return -1; }
//	string line;
//	while (getline(infile, line)) {
//		if (line.empty()) continue;
//		istringstream iss(line);
//		float col1, col2;
//		if (!(iss >> col1 >> col2)) continue;
//		yvals.push_back(col2);
//	}
//	infile.close();
//	if (yvals.empty()) { cerr << "û�ж�ȡ���κ�����" << endl; return -1; }
//
//
//	// �����ʼ�����ߴ�
//	int width = 1200;
//	int height = 400;
//
//	original_img = Mat(height, width, CV_8UC3);
//	plotCurve(yvals, original_img, "Second Column Curve");
//
//	setMouseCallback("Second Column Curve", onMouse, NULL);
//
//	cout << "����ָ��:" << endl;
//	cout << " - [���] �϶�: ѡ��һ��������зŴ�." << endl;
//	cout << " - [�м�] �϶�: ѡ��һ���쳣��������޸�." << endl;
//	cout << " - �� 'z' ��: �ر����зŴ󴰿�." << endl;
//	cout << " - �� 'q' �� 'Esc' ��: �˳�����." << endl;
//
//	// --- ����ʽ��ѭ�� ---
//	while (true) {
//		original_img.copyTo(temp_img);
//
//		// ʵʱ����ѡ���
//		if (drawing_zoom_box) {
//			rectangle(temp_img, zoom_box, Scalar(0, 255, 0), 1); // ��ɫ����������
//		}
//		if (drawing_repair_box) {
//			rectangle(temp_img, repair_box, Scalar(255, 0, 255), 1); // ��ɫ�������޸�
//		}
//
//		imshow("Second Column Curve", temp_img);
//
//		char key = (char)waitKey(15);
//		if (key == 'q' || key == 27) {
//			break;
//		}
//		if (key == 'z') { // 'z' for zoom-reset
//			cout << "�Ŵ󴰿��ѹر�." << endl;
//			destroyWindow("Zoomed View");
//		}
//	}
//
//	destroyAllWindows();
//	return 0;
//}