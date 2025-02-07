#pragma once

enum LoadType {
	LT_MAT,
	LT_MATP,
	LT_MATGRAY,
	LT_MATPGRAY,
	LT_FILE
};



bool IsCanAnyScanNow();
bool Load_BRC(void* source, LoadType LT = LT_FILE);
void OpenCamera(unsigned int CameraID = 0);
bool ScanCameraImage();
void CloseCamera();
//unsigned char* ScanFromCaptureAsync(unsigned int InerPauseMilSeconds = 500);
unsigned char* GetLastResult();
void FreeResult(unsigned char** res);
