//Definitions
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<CommCtrl.h>

//Header Files
#include "../headers/imagestack.h"
#include "../headers/fileio.h"
#include "../headers/dialog.h"
#include "../headers/main.h"

//#include "../headers/debug.h" //only used for debugging code. Remove from Release version

//Function Definitions
void morph(ImageStack& stack);

//Constructor
ImageStack::ImageStack(_3DSize& size):
stack(), _3DSize(size), thread(), listOfImages(), listOfFileNames()
{
	//A 2D array of pointers to every image's row in the image stack
	unsigned char** ptrsMtrx = new unsigned char*[depth*height];

	stack = new unsigned char**[depth];

	for(unsigned d=0; d<depth; d++)
	{
		unsigned char* image = new unsigned char[height*width];

		//Initialise a blank image stack
		memset(image, 0, height*width);

		stack[d] = &ptrsMtrx[d*height];
		ptrsMtrx[d*height] = image;

		for(unsigned h=1; h<height; h++)
			ptrsMtrx[d*height+h] = image + h*width;
	}
}

ImageStack::ImageStack(string fileName, _3DSize size):
stack(), _3DSize(size), thread(), listOfImages(), fileName(fileName)
{
	thread = CreateThread(NULL, 0, loadVGMStackThread, this, 0, NULL);
}

ImageStack::ImageStack(ListOfFileNames& lstOfFN, _2DSize& image):
stack(), _3DSize((unsigned)lstOfFN.list.size(), image), listOfImages(), listOfFileNames(lstOfFN.list)
{
	thread = CreateThread(NULL, 0, loadImageStackThread, this, 0, NULL);
}

//Destructor
ImageStack::~ImageStack()
{
	while(!isLoaded()); //SHOULD NEVER HAPPEN... Wait until images are loaded before deleting

	if(stack) 
	{
		if(*stack)
		{
			if(listOfImages)
			{
				for(unsigned d=0; d<depth; d++)
					if(listOfImages[d]) delete listOfImages[d], listOfImages[d] = NULL;
				
				delete [] listOfImages, listOfImages = NULL;
			}
			else//delete ImageStack(_3DSize)
				for(unsigned d=0; d<depth; d++)
					if(*stack[d])
						delete [] *stack[d];

			delete [] *stack, *stack = NULL;
		}
	
		delete [] stack, stack = NULL;
	}

	depth = height = width = 0;

	listOfFileNames.clear();
}

ImageStack::operator unsigned char***()
{
	return stack;
}

bool ImageStack::isLoaded()
{
	DWORD exitCode;

	return !thread || GetExitCodeThread(thread, &exitCode) && exitCode!=STILL_ACTIVE;
}

DWORD WINAPI ImageStack::loadVGMStackThread(void* param)
{
	statusIndex = 2;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);
	SendMessage(hMainWnd, UM_UPDATE, IDUM_HOURGLASS, NULL);//Set the cursor to display an hourglass

	ImageStack* imgStack = (ImageStack*)param;

	imgStack->listOfImages = new Image*[imgStack->depth];
	imgStack->stack = new unsigned char**[imgStack->depth];
	unsigned char** imageFiles = new unsigned char*[imgStack->depth*imgStack->height];

	for(unsigned d=0; d<imgStack->depth; d++)
	{
		imgStack->listOfImages[d] = NULL;
		imgStack->stack[d] = NULL;
	}

	VGM vgm(imgStack->fileName);

	for(int d=imgStack->depth-1; d>=0; d--)
	{
		Image* image = new Image(&vgm, d);

		imgStack->listOfImages[d] = image;
		imgStack->stack[d] = &imageFiles[d*imgStack->height];
		imageFiles[d*imgStack->height] = *image->image;

		for(unsigned h=1; h<imgStack->height; h++)
			imageFiles[d*imgStack->height+h] = image->image[h];

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(SCROLL_RANGE/4*(double(imgStack->depth-d)/imgStack->depth)), 0);
	}

	morph(*imgStack);
	analyse();

	return 0;
}

DWORD WINAPI ImageStack::loadImageStackThread(void* param)
{
	statusIndex = 1;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);
	SendMessage(hMainWnd, UM_UPDATE, IDUM_HOURGLASS, NULL);//Set the cursor to display an hourglass
	
	ImageStack* imgStack = (ImageStack*)param;
	
	imgStack->listOfImages = new Image*[imgStack->depth];
	imgStack->stack = new unsigned char**[imgStack->depth];
	unsigned char**  imageFiles = new unsigned char*[imgStack->depth*imgStack->height];

	for(unsigned d=0; d<imgStack->depth; d++)
	{
		imgStack->listOfImages[d] = NULL;
		imgStack->stack[d] = NULL;
	}
	
	for(unsigned d=0; d<imgStack->depth; d++)
	{
		//if there is any problem reading any of the files,
		//interrupt the loading process, free the memory and
		//and return to the main window
		Image* image = new Image(imgStack->listOfFileNames[d].c_str());

		if(!image->image)
		{
			if(!popupQuestion("Continue loading remaining images?", true))
			{
				PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, NULL);
				return -1;
			}
		}

		if(image->height!=imgStack->height || image->width!=imgStack->width)
		{
			char msg[256];
			sprintf_s<sizeof(msg)>(msg, "The image resolution of %s is %ix%i and needs to be %ix%i. Continue Loading?",
								   imgStack->listOfFileNames[d].c_str(), image->height, image->width, imgStack->height, imgStack->width);
			if(!popupWarning(msg))
			{
				PostMessage(hMainWnd, WM_COMMAND, ID_FILE_CLOSE, NULL);
				return -1;
			}
		}

		imgStack->listOfImages[d] = image;
		imgStack->stack[d] = &imageFiles[d*imgStack->height];
		imageFiles[d*imgStack->height] = *image->image;

		for(unsigned h=1; h<imgStack->height; h++)
			imageFiles[d*imgStack->height+h] = image->image[h];

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(SCROLL_RANGE/4*(double(d)/imgStack->depth)), 0);
	}

	morph(*imgStack);

	analyse();

	return 0;
}

void morph(ImageStack& stack)
{
	const int OP_CL_SIZE = menuParam.morph;

	//Opening
	if(OP_CL_SIZE)
	{
		statusIndex = 3;
		SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

		//Dilation
		for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
		{
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 0xFF)
						for(int z=-1; z<=1; z++)
							for(int x=-1; x<=1; x++)
								for(int y=-1; y<=1; y++)
									if(stack[d+z][h+x][w+y] <= 1)
									{
										for(int a=-OP_CL_SIZE; a<=OP_CL_SIZE; a++)
											for(int b=-OP_CL_SIZE; b<=OP_CL_SIZE; b++)
												for(int c=-OP_CL_SIZE; c<=OP_CL_SIZE; c++)
													if(!stack[d+a][h+b][w+c])
														stack[d+a][h+b][w+c] = 1;

										stack[d][h][w] = 2;

										x=y=z = 2;//continue
									}

			SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(25+double(SCROLL_RANGE/8)*(d-OP_CL_SIZE)/(stack.depth-2*OP_CL_SIZE)), 0);
		}

		for(unsigned d=0; d<stack.depth; d++)
			for(unsigned h=0; h<stack.height; h++)
				for(unsigned w=0; w<stack.width; w++)
					if(stack[d][h][w])
						stack[d][h][w] = 255;

		statusIndex = 4;
		SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

		//Erosion
		for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
		{
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 0xFF)
						for(int z=-1; z<=1; z++)
							for(int x=-1; x<=1; x++)
								for(int y=-1; y<=1; y++)
									if(stack[d+z][h+x][w+y] == 0)
									{
										stack[d][h][w] = 1;
										x=y=z = 2;//continue
									}

			SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(37.5+double(SCROLL_RANGE/16)*(d-OP_CL_SIZE)/(stack.depth-2*OP_CL_SIZE)), 0);
		}

		for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
		{
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 1)
						for(int a=-OP_CL_SIZE+1; a<=OP_CL_SIZE-1; a++)
							for(int b=-OP_CL_SIZE+1; b<=OP_CL_SIZE-1; b++)
								for(int c=-OP_CL_SIZE+1; c<=OP_CL_SIZE-1; c++)
									if(stack[d+a][h+b][w+c] == 0xFF)
										stack[d+a][h+b][w+c] = 2;

			SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(43.75+double(SCROLL_RANGE/16)*(d-OP_CL_SIZE)/(stack.depth-2*OP_CL_SIZE)), 0);
		}

		for(unsigned d=0; d<stack.depth; d++)
			for(unsigned h=0; h<stack.height; h++)
				for(unsigned w=0; w<stack.width; w++)
					if(stack[d][h][w] != 0xFF)
						stack[d][h][w] = 0;

		//Closing

		//Erosion
		/*for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 0xFF)
						for(int z=-1; z<=1; z++)
							for(int x=-1; x<=1; x++)
								for(int y=-1; y<=1; y++)
									if(stack[d+z][h+x][w+y] == 0)
									{
										stack[d][h][w] = 1;
										x=y=z = 2;//continue
									}

		for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 1)
						for(int a=-OP_CL_SIZE+1; a<=OP_CL_SIZE-1; a++)
							for(int b=-OP_CL_SIZE+1; b<=OP_CL_SIZE-1; b++)
								for(int c=-OP_CL_SIZE+1; c<=OP_CL_SIZE-1; c++)
									if(stack[d+a][h+b][w+c] == 0xFF)
										stack[d+a][h+b][w+c] = 2;

		for(unsigned d=0; d<stack.depth; d++)
			for(unsigned h=0; h<stack.height; h++)
				for(unsigned w=0; w<stack.width; w++)
					if(stack[d][h][w] != 0xFF)
						stack[d][h][w] = 0;

		//Dilation
		for(unsigned d=OP_CL_SIZE; d<stack.depth-OP_CL_SIZE; d++)
			for(unsigned h=OP_CL_SIZE; h<stack.height-OP_CL_SIZE; h++)
				for(unsigned w=OP_CL_SIZE; w<stack.width-OP_CL_SIZE; w++)
					if(stack[d][h][w] == 0xFF)
						for(int z=-1; z<=1; z++)
							for(int x=-1; x<=1; x++)
								for(int y=-1; y<=1; y++)
									if(stack[d+z][h+x][w+y] <= 1)
									{
										for(int a=-OP_CL_SIZE; a<=OP_CL_SIZE; a++)
											for(int b=-OP_CL_SIZE; b<=OP_CL_SIZE; b++)
												for(int c=-OP_CL_SIZE; c<=OP_CL_SIZE; c++)
													if(!stack[d+a][h+b][w+c])
														stack[d+a][h+b][w+c] = 1;

										stack[d][h][w] = 2;

										x=y=z = 2;//continue
									}

		for(unsigned d=0; d<stack.depth; d++)
			for(unsigned h=0; h<stack.height; h++)
				for(unsigned w=0; w<stack.width; w++)
					if(stack[d][h][w])
						stack[d][h][w] = 255;*/
	}

	//File Holes in Roots
	if(menuFlags.fill)
	{
		for(unsigned d=1; d<stack.depth-1; d++)
		{
			vector<_2DCoor> outLay, innLay, rtWall;

			//Mark black regions on the edge as backgroud
			//if while(non zero) then assume root is touching the
			//wall and so do not mark
			for(unsigned w=0; w<stack.width; w++)
			{
				if(!stack[d][0][w]) stack[d][0][w] = 1;
				if(!stack[d][stack.height-1][w]) stack[d][stack.height-1][w] = 1;
			}

			for(unsigned w=1; w<stack.width-1; w++)
			{
				if(!stack[d][1][w])
				{
					stack[d][1][w] = 1;
					outLay.push_back(_2DCoor(1, w));
				}
				
				if(!stack[d][stack.height-2][w])
				{
					stack[d][stack.height-2][w] = 1;
					outLay.push_back(_2DCoor(stack.height-2, w));
				}
			}

			for(unsigned h=0; h<stack.height; h++)
			{
				if(!stack[d][h][0]) stack[d][h][0] = 1;
				if(!stack[d][h][stack.width-1]) stack[d][h][stack.width-1] = 1;
			}

			for(unsigned h=1; h<stack.height-1; h++)
			{
				if(!stack[d][h][1])
				{
					stack[d][h][1] = 1;
					outLay.push_back(_2DCoor(h, 1));
				}
				
				if(!stack[d][h][stack.width-2])
				{
					stack[d][h][stack.width-2] = 1;
					outLay.push_back(_2DCoor(h, stack.width-2));
				}
			}

			do
			{
				for(int i=0; i<outLay.size(); i++)
					for(int x=-1; x<=1; x++)
						for(int y=-1; y<=1; y++)
							if(!stack[d][outLay[i].x+x][outLay[i].y+y])
							{
								stack[d][outLay[i].x+x][outLay[i].y+y] = 1;
								innLay.push_back(_2DCoor(outLay[i].x+x, outLay[i].y+y));
							}
							else if(stack[d][outLay[i].x+x][outLay[i].y+y] == 255)
								rtWall.push_back(_2DCoor(outLay[i].x+x, outLay[i].y+y));

				outLay = innLay;
				innLay.clear();
			}
			while(outLay.size());
		}

		/*for(unsigned d=1; d<stack.depth-1; d++)
		{
			vector<_2DCoor> bgPxls;

			for(unsigned h=1; h<stack.height-1; h++)
				for(unsigned w=1; w<stack.width-1; w++)
					if(!stack[d][h][w])
						bgPxls.push_back(_2DCoor(h, w));
		
			vector<Region> bgReg = segment(bgPxls, stack);
			int maxRegSize = 0, maxRegIdx;

			for(int r=0; r<bgReg.size(); r++)
				if(maxRegSize<bgReg[r].size())
				{
					maxRegSize = (int)bgReg[r].size();
					maxRegIdx = r;
				}

			for(int r=0; r<bgReg.size(); r++)
				if(r != maxRegIdx)
					for(int i=0; i<bgReg[r].size(); i++)
						stack[d][bgReg[r][i].x][bgReg[r][i].y] = 255;
		}*/

		for(unsigned d=1; d<stack.depth-1; d++)
			for(unsigned h=1; h<stack.height-1; h++)
				for(unsigned w=1; w<stack.width-1; w++)
				{
					if(stack[d][h][w] == 0)
						stack[d][h][w] = 255;

					if(stack[d][h][w] == 1)
						stack[d][h][w] = 0;
				}
	}
}
