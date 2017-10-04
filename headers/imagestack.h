#pragma once

//System Dependencies
#include<vector>
#include<string>

using namespace std;

//Header Files
#include "fileio.h"
#include "size.h"

//Class Definitions
class ImageStack: public _3DSize
{
 public:
	
	//Attributes
	unsigned char*** stack;

	//Constructor
	ImageStack(_3DSize& size);//create empty image stack
	ImageStack(string fileName, _3DSize size);//load stack from a *.vgm file
	ImageStack(ListOfFileNames& lstOfFN, _2DSize& image);//load stakc from *.bmp sequence

	//Destructor
	~ImageStack();

	//Methods
	operator unsigned char***();
	bool isLoaded();

 private:

	//Copy Constructor
	//(an ImageStack object should not have copies of itself since it contains
	//pointers to the dynamic memory which are deleted in the destrcutor)
	explicit ImageStack(ImageStack&);

	//Attributes
	HANDLE thread;
	Image** listOfImages;
	
	string fileName;
	vector<string> listOfFileNames;

	//Methods
	static DWORD WINAPI loadImageStackThread(void* param);
	static DWORD WINAPI loadVGMStackThread(void* param);
};