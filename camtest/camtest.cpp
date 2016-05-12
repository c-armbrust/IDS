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

int main()
{
	HIDS hCam = 0; // 0: Die erste freie Kamera wird geöffnet
	if(is_InitCamera(&hCam, NULL) != IS_SUCCESS){
		cout<<"Init camera error\n";
		terminate_on_error(hCam);
	}	

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
	if(is_SetExternalTrigger(hCam, IS_SET_TRIGGER_LO_HI) != IS_SUCCESS){
		cout<<"Set trigger mode error\n";
		terminate_on_error(hCam);
	}

	// Get current pixel clock
	UINT nPixelClock;
	if(is_PixelClock(hCam, IS_PIXELCLOCK_CMD_GET, (void*)&nPixelClock, sizeof(nPixelClock)) != IS_SUCCESS) {
		cout<<"Get pixel clock error\n";
		terminate_on_error(hCam);
	}
	cout << "Pixelclock = " << nPixelClock << endl;

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
		cout<<"Min: "<<nMin<<endl;
		cout<<"Max: "<<nMax<<endl;
		cout<<"Inc: "<<nInc<<endl;
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


	// Ziehe ein Einzelbild von der Kamera ein und lege es in aktiven Bildspeicher ab
	/*if(is_FreezeVideo(hCam, IS_WAIT) != IS_SUCCESS){
		cout<<"Capture single picture error\n";
		terminate_on_error(hCam);
	}*/
	
	// Enable FRAME-Event
	is_EnableEvent(hCam, IS_SET_EVENT_FRAME);
	
	// fortlaufende Triggerbereitschaft im Triggermodus
	if(is_CaptureVideo(hCam, IS_WAIT) != IS_SUCCESS){
		cout << "is_CaptureVideo error:\n";
		terminate_on_error(hCam);
	}
	
	while(true){
	nRet = IS_NO_SUCCESS;
	nRet = is_WaitEvent(hCam, IS_SET_EVENT_FRAME, INFINITE);
	if(nRet == IS_SUCCESS)	
	{
	// Save image to file
	// Save jpeg from active memory with quality 80 (without file open dialog)
	IMAGE_FILE_PARAMS ImageFileParams;
	
	// Sind pnImageID und ppcImageMem gleich NULL, wird das Bild aus dem aktiven Bildspeicher gespeichert 
	ImageFileParams.pnImageID = NULL;
	ImageFileParams.ppcImageMem = NULL;

	wstring filename = L"image" + to_wstring(time(0)) + L".jpg";
	ImageFileParams.pwchFileName = (wchar_t*)filename.c_str();
	ImageFileParams.nFileType = IS_IMG_JPG;
	ImageFileParams.nQuality = 80;

		if(is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*)&ImageFileParams, sizeof(ImageFileParams)) != IS_SUCCESS){
			cout<<"Save image to file error\n";
			terminate_on_error(hCam);
		}
		cout << "Saved image to " << ImageFileParams.pwchFileName << endl;
	}
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
