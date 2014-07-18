#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>   // para strings
#include <iomanip>  // para controlar presicion punto flotante
#include <sstream>  // conversion de string a numeros

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <termios.h> // para leer puerto serial
#include <fcntl.h>   // Para abrir puerto USB
#include <errno.h>   // Def de num de errores

#include "RaspiCamCV.h" // para controlar la Pi-camara

using namespace cv;
using namespace std;

/* Valor HSV de la pelota (Hue, Saturation y Value) */
int iLowH = 0;
int iHighH = 30; // Color especifico a buscar

int iLowS = 70; 
int iHighS = 231;

int iLowV = 101;
int iHighV = 255;

void crearControlesPelota();
void configurarParametrosUSB(int USB);
Mat filtrarPelota(Mat imgHSV);

int main (int argc, char ** argv) {
 
  // capturar la camara en vivo. Camara 0 
  RaspiCamCvCapture * camara = raspiCamCvCreateCameraCapture(0);
  
  /*if (!camara.isOpened()) {
    cout << "No se pudo abrir la camara" << endl;
    return -1;
    }*/
  
  crearControlesPelota();

  // Comunicacion con Arduino, abrir puerto
  int USB = open("/dev/ttyUSB0", O_RDWR| O_NOCTTY );
  configurarParametrosUSB(USB);
  
  int iLastX = -1; 
  int iLastY = -1;

  Mat imgOriginal;
  imgOriginal = raspiCamCvQueryFrame(camara);
  //por hacer> verificar si no hubo error

  Size sizeImgOrig = imgOriginal.size();

  //Imagen negra del tamano de la camara 
  Mat imgLines = Mat::zeros(  sizeImgOrig , CV_8UC3 ); 

  while (1){
    imgOriginal = raspiCamCvQueryFrame(camara);
    //por hacer> verificar si no hubo error  
 
    imgLines = Mat::zeros(  sizeImgOrig , CV_8UC3 );

    //Procesar imagen
    Mat imgHSV;
    cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);
    Mat imgPelota = filtrarPelota(imgHSV); 

    Moments pMomentos = moments(imgPelota);
    double dM01 = pMomentos.m01;
    double dM10 = pMomentos.m10;
    double dArea = pMomentos.m00;
    
    if (dArea > 10000){
      
      int posX = dM10 / dArea;
      int posY = dM01 / dArea;

      if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0){
	   	     
	circle(imgLines,Point2f(posX,posY),50,Scalar(255,0,0),1,CV_AA,0);
	write( USB, "y", 1 );
      }
      iLastX = posX;
      iLastY = posY;
    }
    
    imgOriginal = imgOriginal + imgLines;
    imshow("Original", imgOriginal);
    
    if (waitKey(30) == 27) break; // Esc
 
  }
  return 0;
  
}

void crearControlesPelota(){
// Crea una nueva ventana
  namedWindow("HSV Pelota",CV_WINDOW_AUTOSIZE); 
  
  // Crear un control para cambiar la HUE (0 - 179)
  createTrackbar("LowH", "HSV Pelota", &iLowH, 179);
  createTrackbar("HighH","HSV Pelota", &iHighH, 179);
    
  // Crear un control para cambiar la saturacion (0-255)
  createTrackbar("LowS", "HSV Pelota", &iLowS, 255);
  createTrackbar("HighS", "HSV Pelota", &iHighS, 255);
    
  // Crear un control para cambiar el valor (0-255)
  createTrackbar("LowV","HSV Pelota", &iLowV, 255);
  createTrackbar("HighV","HSV Pelota", &iHighV, 255);
}

void configurarParametrosUSB(int USB){
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  
  /* Error Handling */
  if ( tcgetattr ( USB, &tty ) != 0 ){
    cout << "Error " << errno << " from tcgetattr: " 
	 << strerror(errno) << endl;
  }
}

Mat filtrarPelota(Mat imgHSV){

  Mat imgFiltrada;

  inRange(imgHSV, Scalar(iLowH, iLowS, iLowV),
	  Scalar(iHighH, iHighS, iHighV), imgFiltrada);
    
  //morphological opening (removes small objects from the foreground)
  erode(imgFiltrada, imgFiltrada,
	getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
  dilate( imgFiltrada,imgFiltrada,
	  getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
  
  //morphological closing (removes small holes from the foreground)
  dilate( imgFiltrada, imgFiltrada, 
	  getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
  erode(imgFiltrada, imgFiltrada,
	getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

  return imgFiltrada;
}


