#include "global.h"
#include "BRC.h"
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <vector>
#include <string>

bool CanAnyScan = true;

std::vector<const char*> L_code = {
	"0001101", "0011001", "0010011",
	"0111101", "0100011", "0110001",
	"0101111", "0111011", "0110111",
			   "0001011"
};

std::vector<const char*> R_code = {
	"1110010", "1100110", "1101100",
	"1000010", "1011100", "1001110",
	"1010000", "1000100", "1001000",
			   "1110100"
};

std::vector<const char*> G_code = {
	"0100111", "0110011", "0011011",
	"0100001", "0011101", "0111001",
	"0000101", "0010001", "0001001",
			   "0010111"
};

std::vector<const char*> LG_code = {
	"LLLLLL", "LLGLGG", "LLGGLG",
	"LLGGGL", "LGLLGG", "LGGLLG",
	"LGGGLL", "LGLGLG", "LGLGGL",
			   "LGGLGL"
};

unsigned char* lastResult = 0;
cv::VideoCapture cap;
bool isShowCapWnd = false;


unsigned int CheckCodeTable(unsigned char Bits7[8], std::vector<const char*> codeTable, int maxSize = 7) {
	if (codeTable.size() < 10)
		return 0x0fffffffu;

	for (int i = 0; i < 10; i++) {
		if (std::strncmp((const char*)Bits7, codeTable[i], maxSize) == 0) {
			return i;
		}
	}

	return 0xffffffffu;
}

bool Try_EAN8(std::vector<unsigned char> input, std::vector<unsigned int>& output) {
	if (input.size() != 67)
		return false;
	
	std::vector<unsigned int> BitsArrsEAN8;
	unsigned char* Bits7 = new unsigned char[8];
	Bits7[7] = 0;
	for (unsigned int i = 3; i < input.size() - 3;) {
		Bits7[0] = input[i];
		Bits7[1] = input[i+1];
		Bits7[2] = input[i+2];
		Bits7[3] = input[i+3];
		Bits7[4] = input[i+4];
		Bits7[5] = input[i+5];
		Bits7[6] = input[i+6];
		unsigned int x;
		if ((x = CheckCodeTable(Bits7, L_code)) <= 9) {
			BitsArrsEAN8.push_back(x);
			i += 7;
		}
		else if ((x = CheckCodeTable(Bits7, R_code)) <= 9) {
			BitsArrsEAN8.push_back(x);
			i += 7;
		}
		else {
			i += 5;
		}
	}
	delete Bits7;
	if (BitsArrsEAN8.size() != 8)
		return false;

	unsigned int Control = (
		(
			10 - (
				((BitsArrsEAN8[0] + BitsArrsEAN8[2] + BitsArrsEAN8[4] + BitsArrsEAN8[6]) * 3) + 
				(BitsArrsEAN8[1] + BitsArrsEAN8[3] + BitsArrsEAN8[5])) % 10
			)
		);
	Control = Control == 10 ? 0 : Control;
	if (Control != BitsArrsEAN8[7])
		return false;

	for (unsigned int i : BitsArrsEAN8) {
		output.push_back(i);
	}
	BitsArrsEAN8.clear();
	return true;
}

bool Try_EAN13(std::vector<unsigned char> input, std::vector<unsigned int>& output) {
	if (input.size() != 95)
		return false;

	std::vector<unsigned int> BitsArrsEAN8;
	BitsArrsEAN8.push_back(0);
	std::string LG_ = "";
	unsigned char* Bits7 = new unsigned char[8];
	unsigned int x;
	Bits7[7] = 0;
	for (unsigned int i = 3; i < input.size() - 3;) {
		Bits7[0] = input[i];
		Bits7[1] = input[i + 1];
		Bits7[2] = input[i + 2];
		Bits7[3] = input[i + 3];
		Bits7[4] = input[i + 4];
		Bits7[5] = input[i + 5];
		Bits7[6] = input[i + 6];
		
		if ((x = CheckCodeTable(Bits7, L_code)) <= 9) {
			BitsArrsEAN8.push_back(x);
			LG_ += "L";
			i += 7;
		}
		else if ((x = CheckCodeTable(Bits7, R_code)) <= 9) {
			BitsArrsEAN8.push_back(x);
			i += 7;
		}
		else if ((x = CheckCodeTable(Bits7, G_code)) <= 9) {
			BitsArrsEAN8.push_back(x);
			LG_ += "G";
			i += 7;
		}
		else {
			i += 5;
		}
	}
	delete Bits7;
	if ((x = CheckCodeTable((unsigned char*)LG_.c_str(), LG_code, 6)) > 9)
		return false;

	BitsArrsEAN8[0] = x;
	if (BitsArrsEAN8.size() != 13)
		return false;

	unsigned int Control = (
		(
			10 - (
				((BitsArrsEAN8[1] + BitsArrsEAN8[3] + BitsArrsEAN8[5] + BitsArrsEAN8[7] + BitsArrsEAN8[9] + BitsArrsEAN8[11]) * 3) +
				(BitsArrsEAN8[0] + BitsArrsEAN8[2] + BitsArrsEAN8[4] + BitsArrsEAN8[6] + BitsArrsEAN8[8] + BitsArrsEAN8[10])) % 10
			)
		);
	Control = Control == 10 ? 0 : Control;
	if (Control != BitsArrsEAN8[12])
		return false;

	for (unsigned int i : BitsArrsEAN8) {
		output.push_back(i);
	}
	BitsArrsEAN8.clear();
	return true;
}

bool IsCanAnyScanNow()
{
	return CanAnyScan;
}

bool Load_BRC(void* source, LoadType LT) {
	if (!CanAnyScan)
		return false;
	if (source == 0) {
		std::cout << "Error! The argument of function (LOAD_BRC) is null!\r\n";
		return false;
	}

	if (lastResult != 0)
	{
		lastResult = 0;
	}
	CanAnyScan = false;
	cv::Mat image;

	switch (LT)
	{
	case LT_MAT: {
		cv::cvtColor(*((cv::Mat*)(source)), image, cv::COLOR_RGB2GRAY);
	}break;
	case LT_MATGRAY: {
		image = *((cv::Mat*)(source));
	}break;
	case LT_MATP:
		CanAnyScan = true;
		return false;
		break;
	case LT_FILE:
		image = cv::imread((const char*)source, cv::IMREAD_GRAYSCALE);
		break;
	default:
		CanAnyScan = true;
		return false;
		break;
	}
	
	if (image.empty()) {
		CanAnyScan = true;
		std::cout << "Error! The barcode source is not found!\r\n";
		return false;
	}

	cv::Mat formatted(image,
		cv::Rect(0, (unsigned int)(image.rows / 2), image.cols, 1)
	);
	
	std::vector<unsigned char> fByteArray;
	cv::Point point(0, 0);
	for (int col = 0; col < formatted.cols; col++) {
		point.x = col;
		unsigned char el = formatted.at<unsigned char>(point);
		el = 255 - el;
		if (el >= 255 / 2) {
			if (fByteArray.empty())
				continue;
			fByteArray.push_back(1);
		}
		else
			fByteArray.push_back(0);
	}
	image.release();
	formatted.release();

	if (fByteArray.empty())
		return false;

	bool lastStat = fByteArray[0];
	unsigned int count = 0;
	std::vector<unsigned int> arrays;
	for (unsigned char el : fByteArray) {
		if (lastStat != (bool)el) {
			if (count != 0)
				arrays.push_back(count);
			lastStat = (bool)el;
			count = 1;
			continue;
		}
		if(count != 0)
			count++;
	}
	fByteArray.clear();
	if (arrays.empty()) {
		CanAnyScan = true;
		return false;
	}
	unsigned int CountPixelsInOne = 0;
	for (unsigned int i = 0; i < arrays.size(); i++) {
		if (i + 2 >= arrays.size())
			return false;
		if (arrays[i] == arrays[i + 1]) {
			if (arrays[i + 1] == arrays[i + 2]) {
				CountPixelsInOne = arrays[i];
				break;
			}
		}
	}

	for (unsigned int i : arrays) {
		fByteArray.push_back((i / CountPixelsInOne) + (i % CountPixelsInOne));
	}
	arrays.clear();

	std::vector<unsigned char> EncValues;
	for (unsigned int i = 1; i <= fByteArray.size(); i++) {
		unsigned int count = fByteArray[i - 1];
		for (unsigned int j = 0; j < count; j++)
		{
			if (i % 2 == 0)
				EncValues.push_back('0');
			else
				EncValues.push_back('1');
		}
	}
	fByteArray.clear();

	std::vector<unsigned int> result;
	if (Try_EAN8(EncValues, result)) {
		EncValues.clear();
	}
	else if (Try_EAN13(EncValues, result)) {
		EncValues.clear();
	}
	else {
		lastResult = 0;
		CanAnyScan = true;
		return false;
	}

	lastResult = (unsigned char*)malloc(sizeof(unsigned char) * result.size() + 1);
	ZeroMemory(lastResult, sizeof(unsigned char) * result.size() + 1);
	int i = 0;
	for (unsigned int x : result) {
		lastResult[i] = (unsigned char)(0x30 + x);
		i++;
	}
	CanAnyScan = true;
	return true;
}

void OpenCamera(unsigned int CameraID)
{
	cap = cv::VideoCapture(CameraID);
}


bool ScanCameraImage()
{
	if (!cap.isOpened())
		return false;

	cv::Mat frame;
	cap.read(frame);
	if (frame.empty())
		return false;
	cv::Rect rc1(frame.cols / 4, frame.rows / 4, (frame.cols / 4) * 2, frame.rows / 4);
	cv::Mat frame2(frame, rc1);

	if (Load_BRC(&frame2, LT_MAT))
		return true;
	frame.release();
	return false;
}


void CloseCamera()
{
	cap.release();
}

unsigned char* GetLastResult()
{
	return lastResult;
}

void FreeResult(unsigned char** res)
{
	if (res == 0)
		return;
	if (*res == 0)
		return;
	free(*res);
	*res = 0;
}
