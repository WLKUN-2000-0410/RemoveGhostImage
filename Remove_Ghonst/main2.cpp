#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <numeric>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace cv;

// --- ȫ�ֱ��� ---
Mat original_img;
Mat temp_img;
vector<float> yvals; // �洢����ԭʼ����

					 // ����ѡ���
Rect zoom_box;
bool drawing_zoom_box = false;

// �޸�ѡ���
Rect repair_box;
bool drawing_repair_box = false;

// �Ŵ󴰿����
bool zoom_window_active = false;
int zoom_start_index = 0;
int zoom_end_index = 0;

// �����������
double zoom_scale = 1.0;
int zoom_center_x = 0;
const double ZOOM_FACTOR = 1.2;

// �����߾� (Ϊ�̶������ռ�)
const int MARGIN_LEFT = 80;
const int MARGIN_RIGHT = 20;
const int MARGIN_TOP = 20;
const int MARGIN_BOTTOM = 50;

// �������������
mt19937 rng(random_device{}());

// --- �������� ---
void plotCurveWithScale(const vector<float>& data, Mat& targetImg, const string& windowName,
	int startIdx = 0, int endIdx = -1);
void drawScale(Mat& img, float minVal, float maxVal, int dataSize, int startIdx = 0, int endIdx = -1);
void repairArtifact(vector<float>& data, int startIndex, int endIndex);
void onMouseMain(int event, int x, int y, int flags, void* userdata);
void onMouseZoom(int event, int x, int y, int flags, void* userdata);
string formatNumber(float value);

// ��ʽ��������ʾ
string formatNumber(float value) {
	stringstream ss;
	if (abs(value) >= 1000) {
		ss << fixed << setprecision(0) << value;
	}
	else if (abs(value) >= 10) {
		ss << fixed << setprecision(1) << value;
	}
	else {
		ss << fixed << setprecision(2) << value;
	}
	return ss.str();
}

// ���ƿ̶�
void drawScale(Mat& img, float minVal, float maxVal, int dataSize, int startIdx, int endIdx) {
	if (endIdx == -1) endIdx = dataSize;

	// �����ͼ����
	int plot_width = img.cols - MARGIN_LEFT - MARGIN_RIGHT;
	int plot_height = img.rows - MARGIN_TOP - MARGIN_BOTTOM;

	// Y��̶�
	const int y_ticks = 8;
	for (int i = 0; i <= y_ticks; i++) {
		float val = minVal + (maxVal - minVal) * i / y_ticks;
		int y = MARGIN_TOP + plot_height - (int)(i * plot_height / y_ticks);

		// �̶���
		line(img, Point(MARGIN_LEFT - 5, y), Point(MARGIN_LEFT, y), Scalar(0, 0, 0), 1);

		// �̶�ֵ
		string label = formatNumber(val);
		int baseline = 0;
		Size textSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseline);
		putText(img, label, Point(MARGIN_LEFT - 10 - textSize.width, y + textSize.height / 2),
			FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
	}

	// X��̶�
	const int x_ticks = 10;
	int data_range = endIdx - startIdx;
	for (int i = 0; i <= x_ticks; i++) {
		int data_idx = startIdx + (int)(i * data_range / x_ticks);
		int x = MARGIN_LEFT + (int)(i * plot_width / x_ticks);

		// �̶���
		line(img, Point(x, MARGIN_TOP + plot_height), Point(x, MARGIN_TOP + plot_height + 5), Scalar(0, 0, 0), 1);

		// �̶�ֵ
		string label = to_string(data_idx);
		int baseline = 0;
		Size textSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseline);
		putText(img, label, Point(x - textSize.width / 2, MARGIN_TOP + plot_height + 20),
			FONT_HERSHEY_SIMPLEX, 0.4, Scalar(0, 0, 0), 1);
	}

	// ����������
	line(img, Point(MARGIN_LEFT, MARGIN_TOP), Point(MARGIN_LEFT, MARGIN_TOP + plot_height), Scalar(0, 0, 0), 2);
	line(img, Point(MARGIN_LEFT, MARGIN_TOP + plot_height), Point(MARGIN_LEFT + plot_width, MARGIN_TOP + plot_height), Scalar(0, 0, 0), 2);

	// �������ǩ
	putText(img, "Index", Point(img.cols / 2 - 20, img.rows - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1);
	putText(img, "Value", Point(10, img.rows / 2), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 1);
}

// ���̶ȵ����߻��ƺ���
void plotCurveWithScale(const vector<float>& data, Mat& targetImg, const string& windowName,
	int startIdx, int endIdx) {
	if (data.empty()) {
		cerr << "û�����ݿ��Ի���" << endl;
		return;
	}

	if (endIdx == -1) endIdx = data.size();

	// ���ͼ��
	targetImg.setTo(Scalar(255, 255, 255));

	// �����ͼ����
	int plot_width = targetImg.cols - MARGIN_LEFT - MARGIN_RIGHT;
	int plot_height = targetImg.rows - MARGIN_TOP - MARGIN_BOTTOM;

	float minVal = *min_element(data.begin(), data.end());
	float maxVal = *max_element(data.begin(), data.end());

	// ���ƿ̶�
	drawScale(targetImg, minVal, maxVal, data.size(), startIdx, endIdx);

	// ��������
	if (maxVal == minVal) {
		int y = MARGIN_TOP + plot_height / 2;
		line(targetImg, Point(MARGIN_LEFT, y), Point(MARGIN_LEFT + plot_width, y), Scalar(0, 0, 255), 2);
	}
	else {
		float x_step = (float)plot_width / (data.size() > 1 ? data.size() - 1 : 1);
		for (size_t i = 1; i < data.size(); ++i) {
			int y1 = MARGIN_TOP + plot_height - 1 - (int)((data[i - 1] - minVal) / (maxVal - minVal) * (plot_height - 1));
			int y2 = MARGIN_TOP + plot_height - 1 - (int)((data[i] - minVal) / (maxVal - minVal) * (plot_height - 1));
			int x1 = MARGIN_LEFT + (int)((i - 1) * x_step);
			int x2 = MARGIN_LEFT + (int)(i * x_step);
			line(targetImg, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 2);
		}
	}

	imshow(windowName, targetImg);
}

void repairArtifact(vector<float>& data, int startIndex, int endIndex) {
	const int context_size = 30;

	if (startIndex < context_size || endIndex >(int)data.size() - 1 - context_size) {
		cout << "�޸�����̫�������ݱ�Ե���޷���ȡ�㹻����������Ϣ������ȡ����" << endl;
		return;
	}

	vector<float> local_noise;
	local_noise.reserve(context_size * 2);

	// ���� "֮ǰ" ��������
	float trend_start_before = data[startIndex - context_size];
	float trend_end_before = data[startIndex - 1];
	for (int i = 0; i < context_size; ++i) {
		float trend_val = trend_start_before + (trend_end_before - trend_start_before) * ((float)i / (context_size - 1));
		local_noise.push_back(data[startIndex - context_size + i] - trend_val);
	}

	// ���� "֮��" ��������
	float trend_start_after = data[endIndex + 1];
	float trend_end_after = data[endIndex + context_size];
	for (int i = 0; i < context_size; ++i) {
		float trend_val = trend_start_after + (trend_end_after - trend_start_after) * ((float)i / (context_size - 1));
		local_noise.push_back(data[endIndex + 1 + i] - trend_val);
	}

	float sum = std::accumulate(local_noise.begin(), local_noise.end(), 0.0f);
	float mean = sum / local_noise.size();
	float sq_sum = std::inner_product(local_noise.begin(), local_noise.end(), local_noise.begin(), 0.0f);
	float std_dev = std::sqrt(sq_sum / local_noise.size() - mean * mean);

	cout << "�ֲ�������׼��: " << std_dev << endl;

	float repair_start_val = data[startIndex - 1];
	float repair_end_val = data[endIndex + 1];
	int repair_length = endIndex - startIndex + 1;

	normal_distribution<float> dist(0.0f, std_dev);

	for (int i = 0; i < repair_length; ++i) {
		float baseline = repair_start_val + (repair_end_val - repair_start_val) * ((float)(i + 1) / (repair_length + 1));
		float noise = dist(rng);
		data[startIndex + i] = baseline + noise;
	}

	cout << "�����޸����: " << repair_length << " �����ѱ��滻��" << endl;
}

// ����Ļ����ת��Ϊ�������� (���Ǳ߾�)
int screenToDataIndex(int screen_x, int img_width, int data_size) {
	int plot_width = img_width - MARGIN_LEFT - MARGIN_RIGHT;
	int plot_x = screen_x - MARGIN_LEFT;
	if (plot_x < 0) plot_x = 0;
	if (plot_x > plot_width) plot_x = plot_width;

	return (int)((float)plot_x / plot_width * data_size);
}

// ��ȡ�������ź�����ݷ�Χ
void getZoomRange(int& start_idx, int& end_idx, int total_size) {
	int center_idx = (int)((float)zoom_center_x / temp_img.cols * total_size);
	int half_range = (int)(total_size / (2.0 * zoom_scale));

	start_idx = max(0, center_idx - half_range);
	end_idx = min(total_size, center_idx + half_range);

	if (end_idx - start_idx < 10) {
		// ȷ����С��ʾ��Χ
		int mid = (start_idx + end_idx) / 2;
		start_idx = max(0, mid - 5);
		end_idx = min(total_size, mid + 5);
	}
}

// ���������ص�����
void onMouseMain(int event, int x, int y, int flags, void* userdata) {
	// ��������
	if (event == EVENT_MOUSEWHEEL) {
		zoom_center_x = x;
		int delta = getMouseWheelDelta(flags);

		if (delta > 0) {
			zoom_scale *= ZOOM_FACTOR; // �Ŵ�
		}
		else {
			zoom_scale /= ZOOM_FACTOR; // ��С
		}

		// �������ŷ�Χ
		zoom_scale = max(1.0, min(zoom_scale, 50.0));

		if (zoom_scale <= 1.0) {
			// ���õ�ȫ��ͼ
			zoom_scale = 1.0;
			plotCurveWithScale(yvals, original_img, "Second Column Curve");
		}
		else {
			// ��ʾ������ͼ
			int start_idx, end_idx;
			getZoomRange(start_idx, end_idx, yvals.size());
			vector<float> zoomed_data(yvals.begin() + start_idx, yvals.begin() + end_idx);
			plotCurveWithScale(zoomed_data, original_img, "Second Column Curve", start_idx, end_idx);
		}

		cout << "���ű���: " << zoom_scale << "x" << endl;
		return;
	}

	// ��������϶� (��������)
	if (event == EVENT_LBUTTONDOWN) {
		drawing_zoom_box = true;
		zoom_box = Rect(x, y, 0, 0);
	}
	else if (event == EVENT_MOUSEMOVE && drawing_zoom_box) {
		zoom_box.width = x - zoom_box.x;
		zoom_box.height = y - zoom_box.y;
	}
	else if (event == EVENT_LBUTTONUP && drawing_zoom_box) {
		drawing_zoom_box = false;
		if (zoom_box.width < 0) { zoom_box.x += zoom_box.width; zoom_box.width *= -1; }
		if (zoom_box.height < 0) { zoom_box.y += zoom_box.height; zoom_box.height *= -1; }

		if (zoom_box.width < 5 || zoom_box.height < 5) return;

		// �������ŷ�Χ (���ǵ�ǰ��������״̬)
		int current_start, current_end;
		if (zoom_scale <= 1.0) {
			current_start = 0;
			current_end = yvals.size();
		}
		else {
			getZoomRange(current_start, current_end, yvals.size());
		}

		zoom_start_index = current_start + screenToDataIndex(zoom_box.x, temp_img.cols, current_end - current_start);
		zoom_end_index = current_start + screenToDataIndex(zoom_box.x + zoom_box.width, temp_img.cols, current_end - current_start);

		zoom_start_index = max(0, zoom_start_index);
		zoom_end_index = min((int)yvals.size(), zoom_end_index);

		if (zoom_start_index >= zoom_end_index) return;

		vector<float> zoomed_yvals(yvals.begin() + zoom_start_index, yvals.begin() + zoom_end_index);
		Mat zoomed_img(600, 800, CV_8UC3);
		plotCurveWithScale(zoomed_yvals, zoomed_img, "Zoomed View", zoom_start_index, zoom_end_index);

		setMouseCallback("Zoomed View", onMouseZoom, NULL);
		zoom_window_active = true;
	}

	// �����м��϶� (�޸�)
	else if (event == EVENT_MBUTTONDOWN) {
		drawing_repair_box = true;
		repair_box = Rect(x, y, 0, 0);
	}
	else if (event == EVENT_MOUSEMOVE && drawing_repair_box) {
		repair_box.width = x - repair_box.x;
		repair_box.height = y - repair_box.y;
	}
	else if (event == EVENT_MBUTTONUP && drawing_repair_box) {
		drawing_repair_box = false;
		if (repair_box.width < 0) { repair_box.x += repair_box.width; repair_box.width *= -1; }

		if (repair_box.width < 5) return;

		// �����޸���Χ
		int current_start, current_end;
		if (zoom_scale <= 1.0) {
			current_start = 0;
			current_end = yvals.size();
		}
		else {
			getZoomRange(current_start, current_end, yvals.size());
		}

		int start_index = current_start + screenToDataIndex(repair_box.x, temp_img.cols, current_end - current_start);
		int end_index = current_start + screenToDataIndex(repair_box.x + repair_box.width, temp_img.cols, current_end - current_start);

		repairArtifact(yvals, start_index, end_index);

		// �ػ�
		if (zoom_scale <= 1.0) {
			plotCurveWithScale(yvals, original_img, "Second Column Curve");
		}
		else {
			int start_idx, end_idx;
			getZoomRange(start_idx, end_idx, yvals.size());
			vector<float> zoomed_data(yvals.begin() + start_idx, yvals.begin() + end_idx);
			plotCurveWithScale(zoomed_data, original_img, "Second Column Curve", start_idx, end_idx);
		}

		if (zoom_window_active) {
			vector<float> zoomed_yvals(yvals.begin() + zoom_start_index, yvals.begin() + zoom_end_index);
			Mat zoomed_img(600, 800, CV_8UC3);
			plotCurveWithScale(zoomed_yvals, zoomed_img, "Zoomed View", zoom_start_index, zoom_end_index);
		}
	}
}

// �Ŵ󴰿����ص�����
void onMouseZoom(int event, int x, int y, int flags, void* userdata) {
	static Rect zoom_repair_box;
	static bool drawing_zoom_repair_box = false;

	// �������� (�Ŵ󴰿�)
	if (event == EVENT_MOUSEWHEEL) {
		int delta = getMouseWheelDelta(flags);
		int current_range = zoom_end_index - zoom_start_index;
		int center_data_idx = zoom_start_index + (int)((float)x / 800 * current_range);

		if (delta > 0 && current_range > 20) {
			// �Ŵ�
			int new_range = current_range / 2;
			zoom_start_index = max(0, center_data_idx - new_range / 2);
			zoom_end_index = min((int)yvals.size(), zoom_start_index + new_range);
		}
		else if (delta < 0) {
			// ��С
			int new_range = min(current_range * 2, (int)yvals.size());
			zoom_start_index = max(0, center_data_idx - new_range / 2);
			zoom_end_index = min((int)yvals.size(), zoom_start_index + new_range);
		}

		vector<float> zoomed_yvals(yvals.begin() + zoom_start_index, yvals.begin() + zoom_end_index);
		Mat zoomed_img(600, 800, CV_8UC3);
		plotCurveWithScale(zoomed_yvals, zoomed_img, "Zoomed View", zoom_start_index, zoom_end_index);
		return;
	}

	// �����м��϶� (�޸�)
	if (event == EVENT_MBUTTONDOWN) {
		drawing_zoom_repair_box = true;
		zoom_repair_box = Rect(x, y, 0, 0);
	}
	else if (event == EVENT_MOUSEMOVE && drawing_zoom_repair_box) {
		zoom_repair_box.width = x - zoom_repair_box.x;
		zoom_repair_box.height = y - zoom_repair_box.y;

		// ʵʱ��ʾѡ���
		vector<float> zoomed_yvals(yvals.begin() + zoom_start_index, yvals.begin() + zoom_end_index);
		Mat zoomed_img(600, 800, CV_8UC3);
		plotCurveWithScale(zoomed_yvals, zoomed_img, "temp", zoom_start_index, zoom_end_index);

		Rect display_box = zoom_repair_box;
		if (display_box.width < 0) { display_box.x += display_box.width; display_box.width *= -1; }
		if (display_box.height < 0) { display_box.y += display_box.height; display_box.height *= -1; }
		rectangle(zoomed_img, display_box, Scalar(255, 0, 255), 2);
		imshow("Zoomed View", zoomed_img);
	}
	else if (event == EVENT_MBUTTONUP && drawing_zoom_repair_box) {
		drawing_zoom_repair_box = false;
		if (zoom_repair_box.width < 0) {
			zoom_repair_box.x += zoom_repair_box.width;
			zoom_repair_box.width *= -1;
		}

		if (zoom_repair_box.width < 5) return;

		// �����޸���Χ
		int zoomed_data_size = zoom_end_index - zoom_start_index;
		int local_start_index = screenToDataIndex(zoom_repair_box.x, 800, zoomed_data_size);
		int local_end_index = screenToDataIndex(zoom_repair_box.x + zoom_repair_box.width, 800, zoomed_data_size);

		int global_start_index = zoom_start_index + local_start_index;
		int global_end_index = zoom_start_index + local_end_index;

		global_start_index = max(zoom_start_index, global_start_index);
		global_end_index = min(zoom_end_index, global_end_index);

		cout << "�Ŵ󴰿��޸�: �ֲ����� [" << local_start_index << ", " << local_end_index
			<< "] -> ȫ������ [" << global_start_index << ", " << global_end_index << "]" << endl;

		repairArtifact(yvals, global_start_index, global_end_index);

		// �ػ�
		if (zoom_scale <= 1.0) {
			plotCurveWithScale(yvals, original_img, "Second Column Curve");
		}
		else {
			int start_idx, end_idx;
			getZoomRange(start_idx, end_idx, yvals.size());
			vector<float> zoomed_data(yvals.begin() + start_idx, yvals.begin() + end_idx);
			plotCurveWithScale(zoomed_data, original_img, "Second Column Curve", start_idx, end_idx);
		}

		vector<float> zoomed_yvals(yvals.begin() + zoom_start_index, yvals.begin() + zoom_end_index);
		Mat zoomed_img(600, 800, CV_8UC3);
		plotCurveWithScale(zoomed_yvals, zoomed_img, "Zoomed View", zoom_start_index, zoom_end_index);
	}
}

int main() {
	string filename = "Series1.txt";
	ifstream infile(filename);
	if (!infile) {
		cerr << "�޷����ļ�: " << filename << endl;
		return -1;
	}

	string line;
	while (getline(infile, line)) {
		if (line.empty()) continue;
		istringstream iss(line);
		float col1, col2;
		if (!(iss >> col1 >> col2)) continue;
		yvals.push_back(col2);
	}
	infile.close();

	if (yvals.empty()) {
		cerr << "û�ж�ȡ���κ�����" << endl;
		return -1;
	}

	// �����ʼ�����ߴ�
	int width = 1200;
	int height = 500;  // ���Ӹ߶������ɿ̶�

	original_img = Mat(height, width, CV_8UC3);
	plotCurveWithScale(yvals, original_img, "Second Column Curve");

	setMouseCallback("Second Column Curve", onMouseMain, NULL);

	cout << "=== ����ָ�� ===" << endl;
	cout << " - [����] ���¹���: �����λ��Ϊ��������." << endl;
	cout << " - [���] �϶�: ѡ���������򿪷Ŵ󴰿�." << endl;
	cout << " - [�м�] �϶�: ѡ���쳣��������޸� (�����ںͷŴ󴰿ڶ�֧��)." << endl;
	cout << " - �� 'r' ��: �������ŵ�ȫ��ͼ." << endl;
	cout << " - �� 'z' ��: �ر����зŴ󴰿�." << endl;
	cout << " - �� 'q' �� 'Esc' ��: �˳�����." << endl;
	cout << "==================" << endl;

	// ����ʽ��ѭ��
	while (true) {
		original_img.copyTo(temp_img);

		// ʵʱ����ѡ���
		if (drawing_zoom_box) {
			rectangle(temp_img, zoom_box, Scalar(0, 255, 0), 2); // ��ɫ����������
		}
		if (drawing_repair_box) {
			rectangle(temp_img, repair_box, Scalar(255, 0, 255), 2); // ��ɫ�������޸�
		}

		imshow("Second Column Curve", temp_img);

		char key = (char)waitKey(15);
		if (key == 'q' || key == 27) {
			break;
		}
		if (key == 'z') {
			cout << "�Ŵ󴰿��ѹر�." << endl;
			destroyWindow("Zoomed View");
			zoom_window_active = false;
		}
		if (key == 'r') {
			cout << "�������ŵ�ȫ��ͼ." << endl;
			zoom_scale = 1.0;
			plotCurveWithScale(yvals, original_img, "Second Column Curve");
		}
	}

	destroyAllWindows();
	return 0;
}