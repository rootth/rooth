//System Dependencies
#include<algorithm>

//Header Files
#include "../headers/main.h"
#include "../headers/fileio.h"
#include "../headers/dialog.h"

//#include "../headers/debug.h" //only used for debugging code. Remove from Release version

//Constructor
Image::Image(const char* fileName): image(NULL), imgFile(NULL)
{
	static volatile HANDLE mutex = CreateMutex(NULL, FALSE, NULL);
	static unsigned long fileBufferSize = 0;
	static unsigned char* fileBuffer = NULL;
	
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	DWORD fileSize = GetFileSize(hFile, NULL);

	//Reallocate the fileBuffer if there is insufficient space to hold the entire image file
	WaitForSingleObject(mutex, INFINITE);//Beginning of critical section (fileBuffer)
	if(!fileBuffer) fileBuffer = new unsigned char[fileBufferSize = fileSize];
	if(fileBufferSize<fileSize)
	{
		delete [] fileBuffer;
		fileBuffer = new unsigned char[fileBufferSize = fileSize];
	}

	BOOL isFileRead = ReadFile(hFile, fileBuffer, fileSize, &fileSize, NULL) + CloseHandle(hFile);

	if(isFileRead)
		if(((BITMAPFILEHEADER*)fileBuffer)->bfType == 'MB' && fileSize > sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO))
		{
			height = ((BITMAPINFO*)(fileBuffer+sizeof(BITMAPFILEHEADER)))->bmiHeader.biHeight;
			width  = ((BITMAPINFO*)(fileBuffer+sizeof(BITMAPFILEHEADER)))->bmiHeader.biWidth;

			unsigned char pxlValWth = ((BITMAPINFO*)(fileBuffer+sizeof(BITMAPFILEHEADER)))->bmiHeader.biBitCount/8;

			if(pxlValWth != 1 && pxlValWth != 3)
			{
				popupWarning(std::string() + "Please convert " + fileName + " to 8 bit or 24 bit encoded image");
				return;
			}

			image = new unsigned char*[height];

			if(pxlValWth == 1 && pxlValWth*width%4 == 0)
			{
				*image = &fileBuffer[((BITMAPFILEHEADER*)fileBuffer)->bfOffBits];
				for(unsigned h=1; h<height; h++)
					image[height-h] = *image + h*width;

				imgFile = fileBuffer;
				fileBuffer = NULL;

				//Ensure the pixel values are either 0 or 255
				for(unsigned h=0; h<height; h++)
					for(unsigned w=0; w<width; w++)
						if(image[h][w]) image[h][w] = 0xFF;
			}
			else
			{
				*image = new unsigned char[height*width];
				for(unsigned h=1; h<height; h++)
					image[h] = *image + h*width;

				for(unsigned h=0; h<height; h++)
					for(unsigned w=0; w<width; w++)
					{
						image[h][w] = fileBuffer[((BITMAPFILEHEADER*)fileBuffer)->bfOffBits+ //offset to the pixel values;
												 pxlValWth*									 //colour depth;
												 ((height-1-h)*width+w)+					 //indexing each indivdual pixel;
												 (pxlValWth*width%4?						 //in *.bmp all lines contain multiple of 4
												 (4-pxlValWth*width%4)*(height-1-h):0)];	 //bytes irrespective of the colour depth;
					
						//Ensure the pixel values are either 0 or 255
						if(image[h][w]) image[h][w] = 0xFF;
					}
			}
		}
		else
			popupWarning(std::string() + "Invalid bitmap file (" + fileName + "). Please ensure the images are encoded in bitmap format");
	else
		popupWarning(std::string() + "The image file (" + fileName + ") could not be opened");

	ReleaseMutex(mutex);//End of critical section (fileBuffer)
}

Image::Image(VGM* vgm, unsigned index): _2DSize(vgm->volSize), image(NULL), imgFile(NULL)
{
	image  = new unsigned char*[height];
	*image = new unsigned char[height*width];

	for(unsigned h=1; h<height; h++)
		image[h] = *image + h*width;
	memset(*image, 0, height*width);
	
	if(index<vgm->toplft.z || index>=vgm->btmrght.z) return;

	const unsigned char* buffer = (*vgm)[index-vgm->toplft.z];

	if(buffer)
		for(int x=vgm->btmrght.x-1; x>=(int)vgm->toplft.x; x--)
			for(int y=vgm->toplft.y; y<(int)vgm->btmrght.y; y++)
				image[height-1-x][y] = buffer[(x-vgm->toplft.x)*vgm->roiSize.width+y-vgm->toplft.y];
}

Image::Image(_2DSize& size): _2DSize(size), image(NULL), imgFile(NULL)
{
	image  = new unsigned char*[height];
	*image = new unsigned char[height*width];

	for(unsigned h=1; h<height; h++)
		image[h] = *image + h*width;
	memset(*image, 0, height*width);
}

//Destructor
Image::~Image()
{
	if(imgFile)
		delete [] imgFile;
	else
		if(*image)
			delete [] *image;
		
	if(image) delete [] image;
}

//Displays File Selection Dialog which promts the user to specify
//the location of the image containing the mCT Scans
BOOL Image::selectImageDialog(HWND hWnd, char* fileName)
{
	if(fileName)
	{
		OPENFILENAME ofn = {};

		*fileName = L'\0';
					
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = 256; //Windows file names maximum characters limit
		ofn.lpstrFilter = "Image Sequence (*.bmp)\0*.bmp\0VG Studio Max ROI File(*.vgm)\0*.vgm\0";
		ofn.lpstrTitle = "Select an Image Stack Encoding µCT Scans of Roots";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST + OFN_FILEMUSTEXIST + OFN_EXPLORER;
					
		return GetOpenFileName(&ofn);
	}
	
	return false;
}


//Default Constructor
ListOfFileNames::ListOfFileNames(): index(), size(), format(NONE){}

//Constgructor
ListOfFileNames::ListOfFileNames(string fn): index(), fileName(fn), format(BMP)
{
	HANDLE findFile;
	WIN32_FIND_DATA findFileData;

	string filePath = fileName;

	filePath = filePath.substr(0, filePath.rfind('\\')+1);
	findFile = FindFirstFile((filePath+"*.bmp").c_str(), &findFileData);

	do list.push_back(filePath + findFileData.cFileName);
	while(FindNextFile(findFile, &findFileData));

	sort(list.begin(), list.end());

	for(int i=0; i<list.size(); i++)
		if(list[i]==fileName)
			index = i;

	size = unsigned(list.size());

	FindClose(findFile);
}

ListOfFileNames::ListOfFileNames(::VGM& vgm, string fn): index(), fileName(fn), format(VGM) {size = vgm.volSize.depth;}

void ListOfFileNames::clear()
{
	format = NONE;

	index = 0;
	list.clear();
}

VGM::VGM(string fileName): file(fileName.c_str(), ios::in+ios::binary), buffer(NULL)
{
	if(!file) return;//error opening file

	unsigned qword[2];

	file.read((char*)&qword, 8);

	if(qword[0] != 'VGM ' || qword[1] != 'RLE ')
	{
		popupError("Invalid VGM file format");
		PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, 0);

		return;
	}

	//Read VGM Header File
	file.seekg(12, file.cur);//skip 12 bytes

	file.read((char*)&volSize.width,  4);
	file.read((char*)&volSize.height, 4);
	file.read((char*)&volSize.depth,  4);

	file.seekg(4, file.cur);//skip 4 bytes

	file.read((char*)&bitCount, 4);

	file.seekg(20, file.cur);//skip 20 bytes

	file.read((char*)&toplft.y, 4);
	file.read((char*)&toplft.x, 4);
	file.read((char*)&toplft.z, 4);

	file.seekg(4, file.cur);//skip 4 bytes

	file.read((char*)&btmrght.y, 4);
	file.read((char*)&btmrght.x, 4);
	file.read((char*)&btmrght.z, 4);

	file.seekg(4, file.cur);//skip 4 bytes

	btmrght = btmrght+_3DSize(1, 1, 1); 
	roiSize = btmrght-toplft;

	//Create Image Slices Look-up Table
	do
	{
		unsigned dword;

		file.read((char*)&dword, 4);

		if(dword == 'SLCE') lookuptable.push_back(file.tellg()-(streamoff)4);
	}
	while(!file.eof());

	file.clear();//reset file state

	if(lookuptable.size()!=roiSize.depth)
	{
		popupError("Some image slices could not be extracted from the VGM File");
		PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, 0);

		return;
	}

	file.seekg(92, file.beg);//go back to beginning of the first slice

	buffer = new unsigned char[roiSize.height*roiSize.width];
}

VGM::~VGM()
{
	file.close();
	if(buffer) delete buffer;
}

unsigned char* VGM::operator[](unsigned sliceIndex)
{
	bool esc = false;
	unsigned bufIndex = 0;
	unsigned dword, prvdword;
	unsigned block_size;
	
	if(!buffer || sliceIndex>roiSize.depth-1)
	{
		popupError("Error reading content of VGM file");
		PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, 0);

		return NULL;
	}

	file.seekg(lookuptable[roiSize.depth-1-sliceIndex], file.beg);
	file.read((char*)&dword, 4);//read SLICE marker

	if(dword!='SLCE' || !file.good())
	{
		char str[256];
		wsprintf(str, "The *.vgm file is corrupt at slice %i", sliceIndex);
		popupError(str);

		PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, 0);

		return NULL;
	}

	file.read((char*)&block_size, 4);

	for(unsigned i=0; i<block_size; i++)
	{
		unsigned shift;

		prvdword = dword;
		file.read((char*)&dword, 4);

		if((dword^prvdword) == 0x80 && esc)
		{
			unsigned rle;

			file.read((char*)&rle, 4); i++;

			if(rle<=0x7FFE)
			{
				if(rle)
				{
					for(unsigned r=0; r<rle; r++)
					{
						shift = 0xFFFFFFFF<<(32-bitCount);

						while(shift)
						{
							buffer[bufIndex++] = (shift&prvdword?'\xFF':'\0');
							shift >>= bitCount;

							if(bufIndex>=roiSize.height*roiSize.width) return buffer;
						}
					}
				}
				else
				{
					shift = 0xFFFFFFFF<<(32-bitCount);

					while(shift)
					{
						buffer[bufIndex++] = (shift&dword?'\xFF':'\0');
						shift >>= bitCount;

						if(bufIndex>=roiSize.height*roiSize.width) return buffer;
					}
				}

				esc = false;
				continue;
			}
			else
			{
				file.seekg(-4, file.cur); i--;
			}
		}
		else
		{
			shift = 0xFFFFFFFF<<(32-bitCount);

			while(shift)
			{
				buffer[bufIndex++] = (shift&dword?'\xFF':'\0');
				shift >>= bitCount;

				if(bufIndex>=roiSize.height*roiSize.width) return buffer;
			}
		}

		esc = true;
	}

	return buffer;
}