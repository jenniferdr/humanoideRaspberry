#include <iostream>
#include <stdio.h>
#include <string>   // para strings
#include <iomanip>  // para controlar presicion punto flotante
#include <sstream>  // conversion de string a numeros

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
//#include "opencv2/nonfree/nonfree.hpp"

#include <termios.h> // para leer puerto serial
#include <fcntl.h>   // Para abrir puerto USB
#include <errno.h>   // Def de num de errores

#include "RaspiCamCV.h" // para controlar la Pi-camara


using namespace cv;
using namespace std;

/* valor de la presicion al comparar las imagenes valores 15-30 difieren
 * en gran cantidad valores 30-infinito mas se parecen */
#define presicion 40 

/* Valor HSV de la pelota (Hue, Saturation y Value) */
int iLowH = 0;
int iHighH = 94; // Color especifico a buscar

int iLowS = 75; 
int iHighS = 255;

int iLowV = 78;
int iHighV = 255;

/* Valor HSV de la arqueria (Hue, Saturation y Value) */
int ALowH = 96 ;
int AHighH = 120; // Color especifico a buscar

int ALowS = 116; 
int AHighS = 199;

int ALowV = 71;
int AHighV = 255;

void crearControlesPelota();
void crearControlesArqueria();
void configurarParametrosUSB(int USB);
Mat filtrarPelota(Mat imgHSV);
Mat filtrarArqueria(Mat imgHSV);

int main (int argc, char ** argv) {
 
  // capturar la camara en vivo. Camara 0 
  RaspiCamCvCapture * camara = raspiCamCvCreateCameraCapture(0);
  
  /*if (!camara.isOpened()) {
    cout << "No se pudo abrir la camara" << endl;
    return -1;
    }*/
  
  crearControlesPelota();
  crearControlesArqueria();

  // Comunicacion con Arbotix, abrir puerto
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

  // Puntos para seccionar la imagen
  CvPoint horizonIni = cvPoint(00,(imgLines.size().height)/2);
  CvPoint horizonFin =
    cvPoint(imgLines.size().width,(imgLines.size().height)/2); 
  CvPoint verticalIni =
    cvPoint((imgLines.size().width)/2,(imgLines.size().height)/2);
  CvPoint verticalFin =
    cvPoint(((imgLines.size().width)/2),imgLines.size().height);

  while (1){
    imgOriginal = raspiCamCvQueryFrame(camara);
    //por hacer> verificar si no hubo error  
 
    imgLines = Mat::zeros(  sizeImgOrig , CV_8UC3 );

    // Dibujar division de la pantalla
    line(imgLines, horizonIni, horizonFin, cvScalar(0,255,0), 1);
    line(imgLines, verticalIni,verticalFin, cvScalar(0,255,0), 1);

    //Convertir el cuadro de BGR a HSV
    Mat imgHSV;
    cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);
 
    /********** Para Pelota ******/
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
	
	/*******  Seguir Pelota **********/
	if (posY < horizonIni.y){
	  write( USB, "a", 1 );
	  cout << " Camino hacia adelante" << endl ;
	  
	} else if (posX < verticalIni.x){
	  write( USB, "i", 1 );
	  cout << " Camino a la Izq"  << endl;
	  
	} else { 
	  write( USB, "d", 1 );
	  cout << " Camino a la Derecha" << endl ; 
	}
      }
      iLastX = posX;
      iLastY = posY;
    }
      
    /********* Para Arqueria *****/
    Mat imgArqueria = filtrarArqueria(imgHSV);

    Moments AMomentos = moments(imgArqueria);
    double AM01 = AMomentos.m01;
    double AM10 = AMomentos.m10;
    double AArea = AMomentos.m00;
    
    if (AArea > 10000){
      
      // Arqueria
      int posX1 = AM10 / AArea;
      int posY1 = AM01 / AArea;          
      
      circle(imgLines,Point2f(posX1,posY1),50,Scalar(255,0,0),1,CV_AA,0);
      
    }
    imshow("Imagen Filtrada Pelota", imgPelota);
    imshow("Imagen Filtrada Arqueria", imgArqueria);
    
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

void crearControlesArqueria(){
  namedWindow("HSV Arqueria",CV_WINDOW_AUTOSIZE);
  
  // Crear un control para cambiar la HUE (0 - 179)
  createTrackbar("LowH", "HSV Arqueria", &ALowH, 179);
  createTrackbar("HighH","HSV Arqueria", &AHighH, 179);
    
  // Crear un control para cambiar la saturacion (0-255)
  createTrackbar("LowS", "HSV Arqueria", &ALowS, 255);
  createTrackbar("HighS","HSV Arqueria", &AHighS, 255);
    
  // Crear un control para cambiar el valor (0-255)
  createTrackbar("LowV", "HSV Arqueria", &ALowV, 255);
  createTrackbar("HighV","HSV Arqueria", &AHighV, 255);
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

Mat filtrarArqueria(Mat imgHSV){
  Mat imgFiltrada;

  inRange(imgHSV, Scalar(ALowH, ALowS, ALowV), 
	  Scalar(AHighH, AHighS, AHighV), imgFiltrada);
  
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
