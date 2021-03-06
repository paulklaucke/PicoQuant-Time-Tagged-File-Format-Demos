
/************************************************************************

   PicoHarp 300			File Access Demo in C

  Demo access to binary PicoHarp 300 T2 Mode Files (*.pt2)
  for file format version 2.0 only
  Read a PicoHarp data file and dump the contents in ASCII
  Michael Wahl, PicoQuant GmbH, September 2006, updated May 2007

  Tested with the following compilers:

  - MinGW 2.0.0-3 (free compiler for Win 32 bit)
  - MS Visual C++ 4.0/5.0/6.0 (Win 32 bit)
  - Borland C++ 5.5 (Win 32 bit)

  It should work with most others.
  Observe the 4-byte structure alignment!

  This is demo code. Use at your own risk. No warranties.


  Note that markers have a lower time resolution and may therefore appear 
  in the file slightly out of order with respect to regular event records.
  This is by design. Markers are designed only for relatively coarse 
  synchronization requirements such as image scanning. 


************************************************************************/


#include <stdio.h>
#include <time.h>
#include <string.h>

#define DISPCURVES 8
#define RESOL 4E-12 //4ps
#define T2WRAPAROUND 210698240 
#define MEASMODE_T2 2 


/*
The following structures are used to hold the file data
They directly reflect the file structure.
The data types used here to match the file structure are correct
for the tested compilers.
They may have to be changed for other compilers.
*/


#pragma pack(4) //structure alignment to 4 byte boundaries

/* These are substructures used below */

typedef struct{ float Start;
                float Step;
				float End;  } tParamStruct;

typedef struct{ int MapTo;
				int Show; } tCurveMapping;

/* The following represents the readable ASCII file header portion */

struct {		char Ident[16];				//"PicoHarp 300"
				char FormatVersion[6];		//file format version
				char CreatorName[18];		//name of creating software
				char CreatorVersion[12];	//version of creating software
				char FileTime[18];
				char CRLF[2];
				char CommentField[256]; } TxtHdr;

/* The following is binary file header information */

struct {		int Curves;
				int BitsPerRecord;
				int RoutingChannels;
				int NumberOfBoards;
				int ActiveCurve;
				int MeasMode;
				int SubMode;
				int RangeNo;
				int Offset;
				int Tacq;				// in ms
				int StopAt;
				int StopOnOvfl;
				int Restart;
				int DispLinLog;
				int DispTimeFrom;		// 1ns steps
				int DispTimeTo;
				int DispCountsFrom;
				int DispCountsTo;
				tCurveMapping DispCurves[DISPCURVES];	
				tParamStruct Params[3];
				int RepeatMode;
				int RepeatsPerCurve;
				int RepeatTime;
				int RepeatWaitTime;
				char ScriptName[20];	} BinHdr;

/* The next is a board specific header */

struct {		
				char HardwareIdent[16]; 
				char HardwareVersion[8]; 
				int HardwareSerial; 
				int SyncDivider;
				int CFDZeroCross0;
				int CFDLevel0;
				int CFDZeroCross1;
				int CFDLevel1;
				float Resolution;
				//below is new in format version 2.0
				int RouterModelCode;
				int RouterEnabled;
				int RtChan1_InputType; 
				int RtChan1_InputLevel;
				int RtChan1_InputEdge;
				int RtChan1_CFDPresent;
				int RtChan1_CFDLevel;
				int RtChan1_CFDZeroCross;
				int RtChan2_InputType; 
				int RtChan2_InputLevel;
				int RtChan2_InputEdge;
				int RtChan2_CFDPresent;
				int RtChan2_CFDLevel;
				int RtChan2_CFDZeroCross;
				int RtChan3_InputType; 
				int RtChan3_InputLevel;
				int RtChan3_InputEdge;
				int RtChan3_CFDPresent;
				int RtChan3_CFDLevel;
				int RtChan3_CFDZeroCross;
				int RtChan4_InputType; 
				int RtChan4_InputLevel;
				int RtChan4_InputEdge;
				int RtChan4_CFDPresent;
				int RtChan4_CFDLevel;
				int RtChan4_CFDZeroCross;		} BoardHdr;


/* The next is a TTTR mode specific header */

struct {
				int ExtDevices;
				int Reserved1;
				int Reserved2;			
				int CntRate0;
				int CntRate1;
				int StopAfter;
				int StopReason;
				int Records;
				int ImgHdrSize;		} TTTRHdr;


/*The following data records appear for each T2 mode event*/


 union	{ 
				 unsigned int allbits;
				 struct
				 {	
					unsigned time		:28; 		
					unsigned channel	:4;
				 } bits;

		} Record;



int main(int argc, char* argv[])
{

 FILE *fpin,*fpout; 		/* input/output file pointers */
 int i,result;
 __int64 time=0, ofltime=0;
 unsigned int cnt_0=0, cnt_1=0;
 unsigned int markers;
 
 printf("\nPicoHarp T2 Mode File Demo");
 printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~");

 if(argc!=3)
 {
  printf("\nUsage: pt2demo infile outfile");
  printf("\ninfile is a binary PicoHarp 300 T2 mode file (*.pt2)");
  printf("\noutfile will be ASCII");
  printf("\nNote that this is only a demo. Routinely converting T2 data");
  printf("\nto ASCII is inefficient and therefore discouraged.");
  goto ex;
 }

 if((fpin=fopen(argv[1],"rb"))==NULL)
 {
  printf("\ncannot open input file\n");
  goto ex;
 }

 if((fpout=fopen(argv[2],"w"))==NULL)
  {
   printf("\ncannot open output file\n");
   goto ex;
  }

 result = fread( &TxtHdr, 1, sizeof(TxtHdr) ,fpin);
 if (result!= sizeof(TxtHdr))
 {
  printf("\nerror reading txt header, aborted.");
  goto close;
 }

 fprintf(fpout,"Ident            : %.*s\n",sizeof(TxtHdr.Ident),TxtHdr.Ident);
 fprintf(fpout,"Format Version   : %.*s\n",sizeof(TxtHdr.FormatVersion),TxtHdr.FormatVersion);
 fprintf(fpout,"Creator Name     : %.*s\n",sizeof(TxtHdr.CreatorName),TxtHdr.CreatorName);
 fprintf(fpout,"Creator Version  : %.*s\n",sizeof(TxtHdr.CreatorVersion),TxtHdr.CreatorVersion);
 fprintf(fpout,"Time of Creation : %.*s\n",sizeof(TxtHdr.FileTime),TxtHdr.FileTime);
 fprintf(fpout,"File Comment     : %.*s\n",sizeof(TxtHdr.CommentField),TxtHdr.CommentField);


 if( strcmp(TxtHdr.Ident,"PicoHarp 300") )
 {
    printf("\nFile identifier not found, aborted.");
	goto close;
 }

 if(  strncmp(TxtHdr.FormatVersion,"2.0",3)  )
 {
    printf("\nError: File format version is %s. This program is for v. 2.0 only.", TxtHdr.FormatVersion);
    goto close;
 }


 result = fread( &BinHdr, 1, sizeof(BinHdr) ,fpin);
 if (result!= sizeof(BinHdr))
 {
   printf("\nerror reading bin header, aborted.");
   goto close;
 }
 fprintf(fpout,"No of Curves     : %ld\n",BinHdr.Curves);
 fprintf(fpout,"Bits per Record  : %ld\n",BinHdr.BitsPerRecord);
 fprintf(fpout,"RoutingChannels  : %ld\n",BinHdr.RoutingChannels);
 fprintf(fpout,"No of Boards     : %ld\n",BinHdr.NumberOfBoards);
 fprintf(fpout,"Active Curve     : %ld\n",BinHdr.ActiveCurve);
 fprintf(fpout,"Measurement Mode : %ld\n",BinHdr.MeasMode);
 fprintf(fpout,"Sub-Mode         : %ld\n",BinHdr.SubMode);
 fprintf(fpout,"Range No         : %ld\n",BinHdr.RangeNo);
 fprintf(fpout,"Offset           : %ld\n",BinHdr.Offset);
 fprintf(fpout,"AcquisitionTime  : %ld\n",BinHdr.Tacq);
 fprintf(fpout,"Stop at          : %ld\n",BinHdr.StopAt);
 fprintf(fpout,"Stop on Ovfl.    : %ld\n",BinHdr.StopOnOvfl);
 fprintf(fpout,"Restart          : %ld\n",BinHdr.Restart);
 fprintf(fpout,"DispLinLog       : %ld\n",BinHdr.DispLinLog);
 fprintf(fpout,"DispTimeAxisFrom : %ld\n",BinHdr.DispTimeFrom);
 fprintf(fpout,"DispTimeAxisTo   : %ld\n",BinHdr.DispTimeTo);
 fprintf(fpout,"DispCountAxisFrom: %ld\n",BinHdr.DispCountsFrom);
 fprintf(fpout,"DispCountAxisTo  : %ld\n",BinHdr.DispCountsTo);


 if(BinHdr.MeasMode != MEASMODE_T2)
 {
    printf("\nWrong measurement mode, aborted.");
    goto close;
 }


 fprintf(fpout,"---------------------\n");
 result = fread( &BoardHdr, 1, sizeof(BoardHdr) ,fpin);
 if (result!= sizeof(BoardHdr))
 {
   printf("\nerror reading board header, aborted.");
   goto close;
 }
 fprintf(fpout," HardwareIdent   : %.*s\n",sizeof(BoardHdr.HardwareIdent),BoardHdr.HardwareIdent);
 fprintf(fpout," HardwareVersion : %.*s\n",sizeof(BoardHdr.HardwareVersion),BoardHdr.HardwareVersion);
 fprintf(fpout," HardwareSerial  : %ld\n",BoardHdr.HardwareSerial);
 fprintf(fpout," SyncDivider     : %ld\n",BoardHdr.SyncDivider);
 fprintf(fpout," CFDZeroCross0   : %ld\n",BoardHdr.CFDZeroCross0);
 fprintf(fpout," CFDLevel0       : %ld\n",BoardHdr.CFDLevel0 );
 fprintf(fpout," CFDZeroCross1   : %ld\n",BoardHdr.CFDZeroCross1);
 fprintf(fpout," CFDLevel1       : %ld\n",BoardHdr.CFDLevel1);
 fprintf(fpout," Resolution      : %lf\n",BoardHdr.Resolution);

 if(BoardHdr.RouterModelCode>0) //otherwise this information is meaningless
 {
   fprintf(fpout," RouterModelCode       : %ld\n",BoardHdr.RouterModelCode);  
   fprintf(fpout," RouterEnabled         : %ld\n",BoardHdr.RouterEnabled);   

   fprintf(fpout," RtChan1_InputType     : %ld\n",BoardHdr.RtChan1_InputType);
   fprintf(fpout," RtChan1_InputLevel    : %ld\n",BoardHdr.RtChan1_InputLevel); 
   fprintf(fpout," RtChan1_InputEdge     : %ld\n",BoardHdr.RtChan1_InputEdge);
   fprintf(fpout," RtChan1_CFDPresent    : %ld\n",BoardHdr.RtChan1_CFDPresent); 
   fprintf(fpout," RtChan1_CFDLevel      : %ld\n",BoardHdr.RtChan1_CFDLevel);
   fprintf(fpout," RtChan1_CFDZeroCross  : %ld\n",BoardHdr.RtChan1_CFDZeroCross);

   fprintf(fpout," RtChan2_InputType     : %ld\n",BoardHdr.RtChan2_InputType);
   fprintf(fpout," RtChan2_InputLevel    : %ld\n",BoardHdr.RtChan2_InputLevel); 
   fprintf(fpout," RtChan2_InputEdge     : %ld\n",BoardHdr.RtChan2_InputEdge);
   fprintf(fpout," RtChan2_CFDPresent    : %ld\n",BoardHdr.RtChan2_CFDPresent); 
   fprintf(fpout," RtChan2_CFDLevel      : %ld\n",BoardHdr.RtChan2_CFDLevel);
   fprintf(fpout," RtChan2_CFDZeroCross  : %ld\n",BoardHdr.RtChan2_CFDZeroCross);

   fprintf(fpout," RtChan3_InputType     : %ld\n",BoardHdr.RtChan3_InputType);
   fprintf(fpout," RtChan3_InputLevel    : %ld\n",BoardHdr.RtChan3_InputLevel); 
   fprintf(fpout," RtChan3_InputEdge     : %ld\n",BoardHdr.RtChan3_InputEdge);
   fprintf(fpout," RtChan3_CFDPresent    : %ld\n",BoardHdr.RtChan3_CFDPresent); 
   fprintf(fpout," RtChan3_CFDLevel      : %ld\n",BoardHdr.RtChan3_CFDLevel);
   fprintf(fpout," RtChan3_CFDZeroCross  : %ld\n",BoardHdr.RtChan3_CFDZeroCross);

   fprintf(fpout," RtChan4_InputType     : %ld\n",BoardHdr.RtChan4_InputType);
   fprintf(fpout," RtChan4_InputLevel    : %ld\n",BoardHdr.RtChan4_InputLevel); 
   fprintf(fpout," RtChan4_InputEdge     : %ld\n",BoardHdr.RtChan4_InputEdge);
   fprintf(fpout," RtChan4_CFDPresent    : %ld\n",BoardHdr.RtChan4_CFDPresent); 
   fprintf(fpout," RtChan4_CFDLevel      : %ld\n",BoardHdr.RtChan4_CFDLevel);
   fprintf(fpout," RtChan4_CFDZeroCross  : %ld\n",BoardHdr.RtChan4_CFDZeroCross);
 }

 fprintf(fpout,"---------------------\n");


 result = fread( &TTTRHdr, 1, sizeof(TTTRHdr) ,fpin);
 if (result!= sizeof(TTTRHdr))
 {
   printf("\nerror reading TTTR header, aborted.");
   goto close;
 }
 fprintf(fpout,"ExtDevices      : %ld\n",TTTRHdr.ExtDevices);
 fprintf(fpout,"CntRate0        : %ld\n",TTTRHdr.CntRate0);
 fprintf(fpout,"CntRate1        : %ld\n",TTTRHdr.CntRate1);
 fprintf(fpout,"StopAfter       : %ld\n",TTTRHdr.StopAfter);
 fprintf(fpout,"StopReason      : %ld\n",TTTRHdr.StopReason);
 fprintf(fpout,"Records         : %ld\n",TTTRHdr.Records);
 fprintf(fpout,"ImgHdrSize      : %ld\n",TTTRHdr.ImgHdrSize);

 
 /* skip the imaging header (you may need to read it if you
    want to interpret an imaging file */
 fseek(fpin,TTTRHdr.ImgHdrSize*4,SEEK_CUR);


/* Now read and interpret the TTTR records */           

 printf("\nprocessing..\n");

// fprintf(fpout,"\nrecord# chan   rawtime      time/4ps   time/sec\n");


 for(i=0;i<TTTRHdr.Records;i++)
 {
	result = fread( &Record, 1, sizeof(Record) ,fpin);
	if (result!= sizeof(Record))
	{
		printf("\nUnexpected end of input file!");
		break;
	}

	fprintf(fpout,"%7u %08x ",i,Record.allbits);

	if(Record.bits.channel==0xF) //this means we have a special record
	{
		//in a special record the lower 4 bits of time are marker bits
		markers=Record.bits.time&0xF;
		if(markers==0) //this means we have an overflow record
		{
			ofltime += T2WRAPAROUND; // unwrap the time tag overflow 
			fprintf(fpout," ofl\n");
		}
		else //a marker
		{
			//Strictly, in case of a marker, the lower 4 bits of time are invalid
			//because they carry the marker bits. So one could zero them out. 
			//However, the marker resolution is only a few tens of nanoseconds anyway,
			//so we can just ignore the few picoseconds of error.
			time = ofltime + Record.bits.time;

			fprintf(fpout,"MA%1u %12u %12I64d %14.12lf\n",markers,Record.bits.time,time,(double)time*RESOL);
		}
		continue;		
	}		

	if((int)Record.bits.channel>BinHdr.RoutingChannels) //Should not occur 
	{
		printf(" Illegal Chan: #%1d %1u\n",i,Record.bits.channel);
		fprintf(fpout," illegal chan.\n");
		continue;
	}

	if(Record.bits.channel==0) cnt_0++;
	if(Record.bits.channel>=1) cnt_1++;

	time = ofltime + Record.bits.time;

	fprintf(fpout,"  %1u %12u %12I64d %14.12lf\n",Record.bits.channel,Record.bits.time,time,(double)time*RESOL);
 }

 printf("\nStatistics obtained from the data:");
 printf("\nlast tag= %1I64d ", time);
 printf("\ncnt_0=%1u cnt_1=%1u", cnt_0, cnt_1);
 printf("\nmeasurement time= %1.4lfs countrate_a = %1.0lf kHz, countrate_b = %1.0lf kHz",
	 (double)time*RESOL, 
	 (double)cnt_0/((double)time*RESOL*1E3),
	 (double)cnt_1/((double)time*RESOL*1E3));
 printf("\n");

close:
 fclose(fpin);
 fclose(fpout);
ex:
 printf("\npress return to exit");
 getchar();
 return(0);
}

