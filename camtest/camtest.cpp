#include <uEye.h>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
using namespace std;

void terminate_on_error(HIDS);

constexpr int sizeX = 640;
constexpr int sizeY = 480;
constexpr int bitsPerPixel = 8;

constexpr double cPCLK = 6;
constexpr double cFPS = 2;
constexpr double cEXP = 200;

int main()
{
	HIDS hCam = 0; // 0: Die erste freie Kamera wird geöffnet
	if(is_InitCamera(&hCam, NULL) != IS_SUCCESS){
		cout<<"Init camera error\n";
		terminate_on_error(hCam);
	}	

	// Get camera infos
	CAMINFO pInfo;
	if(is_GetCameraInfo(hCam, &pInfo) != IS_SUCCESS){
		cout<<"Get camera info error\n";
		terminate_on_error(hCam);
	}
	cout << "Kamera Infos:\n";
	cout << "--------------------\n";
	cout << "Seriennummer: " << pInfo.SerNo << endl;
	cout << "Hersteller: " << pInfo.ID << endl;
	cout << "HW Version USB Board: " << pInfo.Version << endl;
	cout << "Systemdatum des Endtests: " << pInfo.Date << endl;
	cout << "Kamera-ID: " << (int)pInfo.Select << endl; // Ab Werk steht die Kamera-ID auf 1. Kann mit is_SetCameraID geändert werden
	if(pInfo.Type == IS_CAMERA_TYPE_UEYE_USB_SE)
		cout << "Kameratyp: USB uEye SE" << endl; 
	cout << endl;  

	cout << "ueye API Info\n";
	cout << "--------------------\n";
	int version = is_GetDLLVersion();
	int build = version & 0xFFFF;
	version = version >> 16;
	int minor = version & 0xFF;
	version = version >> 8;
	int major = version & 0xFF;
	cout << "API version " << major << "." << minor << "." << build << endl;
	cout << endl;
	//Bitmap-Modus setzen (in Systemspeicher digitalisieren)
	if(is_SetDisplayMode(hCam, IS_SET_DM_DIB) != IS_SUCCESS){
		cout<<"Set display mode error\n";
		terminate_on_error(hCam);
	}

	char* ppcImgMem; // Zeiger auf Speicheranfang des Bildspeichers
	int pid; // ID für Speicher
	if(is_AllocImageMem(hCam, sizeX, sizeY, bitsPerPixel, &ppcImgMem, &pid) != IS_SUCCESS){
		cout<<"Alloc image mem error\n";
		terminate_on_error(hCam);
	}

	// Calculate image buffer size
	// Zeileninkrement berechnen
	const int line = sizeX * int((bitsPerPixel + 7) / 8); 
	const int adjust = line % 4 == 0 ? 0 : 4 - line % 4;
	// adjust ist 0 wenn line ohne Rest durch 4 teilbar ist, rest(line / 4) sonst.
	const int lineinc = line + adjust;
	const int BufferSize = ( sizeX * int((bitsPerPixel + 7) / 8) + adjust) * sizeY;	
	cout << "BufferSize = " << BufferSize << " Byte\n";
	
	//is_SetAllocatedImageMem
	char* pcMem = new char[BufferSize];
    int iRet = mlock(pcMem, BufferSize);
	int iMemID;
	if(is_SetAllocatedImageMem(hCam, sizeX, sizeY, bitsPerPixel, pcMem, &iMemID) != IS_SUCCESS){
		cout<<"Set allocated image mem error\n";
		terminate_on_error(hCam);
	}

	// Setze Bildspeicher als aktiven Speicher
	if(is_SetImageMem(hCam, pcMem, iMemID) != IS_SUCCESS){
		cout<<"Activate image memory error\n";
		terminate_on_error(hCam);
	}

	// Setze Triggermodus
	if(is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE) != IS_SUCCESS){
		cout<<"Set trigger mode error\n";
		terminate_on_error(hCam);
	}

	// Get pixel clock range
	UINT nRange[3];
	UINT nMin=0, nMax=0, nInc=0;
	ZeroMemory(nRange, sizeof(nRange));
	INT nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)nRange, sizeof(nRange));
	if (nRet == IS_SUCCESS)
	{
		nMin = nRange[0];
	    nMax = nRange[1];
	    nInc = nRange[2];
		cout<<"Pixeltakt infos:\n";
		cout << nMin << " MHz - " << nMax << " MHz\n";
		cout << "Increment: " << nInc << " MHz\n";
	}
	else
	{
		cout<<"Determine pixel clock range error\n";
		terminate_on_error(hCam);
	}

	// Set this pixel clock
	UINT nPixelClockLow = nMin;
    if(is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET, (void*)&nPixelClockLow, sizeof(nPixelClockLow)) != IS_SUCCESS) {
		cout<<"Set pixel clock error:\n";
		terminate_on_error(hCam);
	}
	cout << "Set pixel clock to: " << nPixelClockLow << endl;

	// Set framerate
	double fpsToSet=cFPS, newFPS; 
	if(is_SetFrameRate(hCam, fpsToSet, &newFPS) != IS_SUCCESS){
		cout<<"Set framerate error\n";
		terminate_on_error(hCam);
	}
	if(newFPS == fpsToSet)
		cout << "Successfully set FPS to " << newFPS << endl;
	else
	{
		cout << "Failed to set FPS to " << fpsToSet << endl;  
		cout << "FPS: " << newFPS << endl;
	}

	// Get Exposure capabilities
	UINT nCaps = 0;
	nRet = is_Exposure(hCam, IS_EXPOSURE_CMD_GET_CAPS, (void*)&nCaps, sizeof(nCaps));
	if (nRet == IS_SUCCESS)
	{	cout << "Exposure capabilities:\n";
		if(nCaps & IS_EXPOSURE_CAP_EXPOSURE)
			cout << "Die Belichtungszeiteinstellung wird unterstütz\n";
  		if (nCaps & IS_EXPOSURE_CAP_FINE_INCREMENT)
      		cout << "Das feine Belichtungszeitraster wird unterstütz\n";
		if(nCaps & IS_EXPOSURE_CAP_LONG_EXPOSURE)
			cout << "Die Langzeitbelichtung wird unterstütz\n";
		if(nCaps & IS_EXPOSURE_CAP_DUAL_EXPOSURE)
			cout << "Der Sensor unterstützt die duale Belichtun\n";
	}
	else
	{
		cout << "Get exposure caps error\n";
		terminate_on_error(hCam);
	}

		// Get Exposure Infos (values in ms)
	double ExposureRange[3];
	if(is_Exposure(hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, (void*)&ExposureRange, sizeof(ExposureRange)) != IS_SUCCESS){
		cout << "Get exposure range infos error\n";
		terminate_on_error(hCam);
	}
	double minExposure = ExposureRange[0];
	double maxExposure = ExposureRange[1];
	double incExposure = ExposureRange[2];
	cout << "Min exposure: " << minExposure << " ms" << endl;
	cout << "Max exposure: " << maxExposure << " ms" << endl;
	cout << "Exposure increment: " << incExposure << " ms" << endl;
	
	// Set min exposure time
	minExposure = cEXP; // for testing assign a value other than min 
	cout << "Set exposure to min exposure time\n";
	if(is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&minExposure, sizeof(minExposure)) != IS_SUCCESS){
		cout << "Set exposure error\n";
		terminate_on_error(hCam);
	}

	// Aktuell eingestellte Belichtungszeit:
	double nExposure;
	if(is_Exposure(hCam, IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&nExposure, sizeof(nExposure)) != IS_SUCCESS){
		cout << "Get current exposure error\n";
		terminate_on_error(hCam);
	}
	cout << "Current Exposure: " << nExposure << " ms" << endl;

	// Hardwareverst�rkung (Gain Boost) aktivieren und Gain setzen
	cout << "Enable hardware gain boost.\n";
	if(is_SetGainBoost(hCam, IS_SET_GAINBOOST_ON) != IS_SUCCESS){
		cout << "is_SetGainBoost error:\n";
		terminate_on_error(hCam);
	}

	if(is_SetHardwareGain(hCam, 30, 0, 0, 0) != IS_SUCCESS)
	{
		cout << "is_SetHardwareGain error:\n";
		terminate_on_error(hCam);
	}
	// Get Master Gain to display it.
	int mastergain;
	mastergain = is_SetHardwareGain(hCam, IS_GET_MASTER_GAIN, 0, 0, 0);
	cout << "Set hardware gain to: " << mastergain <<  endl;


	// Enable FRAME-Event
	is_EnableEvent(hCam, IS_SET_EVENT_FRAME);
	
	// fortlaufende Triggerbereitschaft im Triggermodus
	if(is_CaptureVideo(hCam, IS_WAIT) != IS_SUCCESS){
		cout << "is_CaptureVideo error:\n";
		terminate_on_error(hCam);
	}
	// Get current pixel clock
	UINT nPixelClock;
	if(is_PixelClock(hCam, IS_PIXELCLOCK_CMD_GET, (void*)&nPixelClock, sizeof(nPixelClock)) != IS_SUCCESS) {
		cout<<"Get pixel clock error\n";
		terminate_on_error(hCam);
	}
	cout << "Pixelclock = " << nPixelClock << endl;
	
	double dblFPS;
	int img_num=0;

	/*
	while(true){
	nRet = IS_NO_SUCCESS;
	nRet = is_WaitEvent(hCam, IS_SET_EVENT_FRAME, INFINITE);
	if(nRet == IS_SUCCESS)	
	*/
	HANDLE hEvent;	
	is_InitEvent(hCam, hEvent, IS_SET_EVENT_FRAME);
	is_EnableEvent(hCam, IS_SET_EVENT_FRAME);
	while(true)
	{
	is_FreezeVideo(hCam, IS_DONT_WAIT);	
	if (is_WaitEvent(hCam, IS_SET_EVENT_FRAME, 5000))		
	{
	   is_ForceTrigger(hCam);
	}
	
	// Save image to file
	// Save jpeg from active memory with quality 80 (without file open dialog)
	IMAGE_FILE_PARAMS ImageFileParams;
	
	// Sind pnImageID und ppcImageMem gleich NULL, wird das Bild aus dem aktiven Bildspeicher gespeichert 
	ImageFileParams.pnImageID = NULL;
	ImageFileParams.ppcImageMem = NULL;

	wstring filename = L"./" + to_wstring(++img_num/*time(0)*/) + L".jpg";
	ImageFileParams.pwchFileName = (wchar_t*)filename.c_str();
	ImageFileParams.nFileType = IS_IMG_JPG;
	ImageFileParams.nQuality = 80;

		if(is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*)&ImageFileParams, sizeof(ImageFileParams)) != IS_SUCCESS){
			cout<<"Save image to file error\n";
			terminate_on_error(hCam);
		}
		wcout << L"Saved image to " << filename << endl;
		
		// Get frames per second
		dblFPS;
		if(is_GetFramesPerSecond(hCam, &dblFPS) != IS_SUCCESS){
			cout<<"Get FPS error\n";
			terminate_on_error(hCam);
		}	
		cout << "FPS " << dblFPS << endl;

	//}	
	}	

	// Cleanup
	if(is_FreeImageMem(hCam, pcMem, iMemID) != IS_SUCCESS){
		cout<<"Free image memory error\n";
		terminate_on_error(hCam);
	}	
	free(pcMem);

	if(is_ExitCamera(hCam) != IS_SUCCESS){
		cout<<"Exit camera error\n";
		terminate_on_error(hCam);
	}
	
	return 0;
}

void terminate_on_error(HIDS hCam){
	INT pErr; // Fehlercode
	IS_CHAR* ppcErr; // Fehlertext
	is_GetError(hCam, &pErr, &ppcErr);
	cout<<"terminate on error\n";
	cout<<"Code: "<<pErr<<endl;
	cout<<"Text: "<<ppcErr<<endl;
	is_ExitCamera(hCam);
	exit(1);
}
