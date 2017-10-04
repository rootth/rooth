#pragma once

//Definitions
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<fstream>
#include<string>
#include<vector>

//Header Files
#include "size.h"

using namespace std;

//Class Definitions
class VGM
{
 public:

	//Public Members
	_3DSize volSize;
	_3DSize roiSize;
	_3DCoorT<unsigned> toplft, btmrght;

	//Constructor
	VGM():bitCount(0), buffer(NULL){}
	VGM(string fileName);
	
	//Destructor
	~VGM();

	//Interface Methods
	unsigned char* operator[](unsigned sliceIndex);

 private:
	
	//Copy Constrcutor
	//prevents creation of copies of VGM
	explicit VGM(VGM&);
	
	//Private Memebrs
	unsigned bitCount;
	unsigned char* buffer;
	vector<streamoff> lookuptable;
	fstream file;
};

class Image: public _2DSize
{
 public:
	
	//Constructor
	Image(const char* fileName);//read *.bmp sequence
	Image(VGM* vgm, unsigned index);//read *.vgm file
	Image(_2DSize& size);		//create blank image

	//Destructor
	~Image();

	//Class Methods
	static BOOL selectImageDialog(HWND hWnd, char* fileName);

	//Attributes
	unsigned char** image;

 private:
	 
	//Copy Constrcutor
	//prevents creation of copies of image
	explicit Image(Image&);

	unsigned char* imgFile;
};

class ListOfFileNames
{
 public:

	//Attrubutes
	int index;
	int size;
	string fileName;
	vector<string> list;

	//Default Constructor
	ListOfFileNames();
	enum FORMAT{NONE, BMP, VGM} format;
	
	//Constructor
	ListOfFileNames(string fileName);
	ListOfFileNames(::VGM& vgm, string fileName);

	//Methods
	void clear();

 private:

	//Copy Constructor
	explicit ListOfFileNames(ListOfFileNames&);
};