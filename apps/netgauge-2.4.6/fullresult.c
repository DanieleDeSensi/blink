/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include <string.h>	/* memset & co. */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "fullresult.h"

struct histogramm{
   int number_classes;
   unsigned long long * ranges;
   unsigned int *counts;
};

/*
 * Funktion for comparing two long long arguments
 * and returning an int value.
 * 
 */
 

static int comp(const void *arg1, const void *arg2){
   unsigned long long *a1=(unsigned long long *) arg1;
   unsigned long long *a2=(unsigned long long *) arg2;
   if(*a1 > *a2) return 1;
   if(*a1 < *a2) return -1;
   return 0;
}


/*
 * median
 * provides the medain of an given array. 
 * The median is computet by sorting the array and returning the value in the middle of the array
 * WARNING the array will be sorted after invoking this function
 * in unsigned long long *array
 * in int count	count of elements in array start counting with 1
 * return long long the median of the given array		
 * 
 */
unsigned long long median(unsigned long long *array, int count){
   int median;
   unsigned long long median_val;
   unsigned long long *tmp_arr;
   tmp_arr=(unsigned long long *) malloc(sizeof(unsigned long long)*count);
   if (NULL == tmp_arr){
      ng_error("Could not allocate buffer for Median calculation");
      return(0);
   }
   memcpy(tmp_arr,array,count*sizeof(long long));
   qsort(tmp_arr,count,sizeof(long long),comp); 
   median=(count-1)/2;
   median_val=tmp_arr[median];
   free(tmp_arr);
   return median_val;
}

/*
 * makeHistogramm
 * provides a Histogramm
 * the values array which have to contain the values for which the 
 * histogramm should be created is sorted.
 * Then the first fraction per cent values are used. 
 * The min and max of thees range is devided into number_classes classes of same size. 
 * The ranges array than contains the upper borders of each class. 
 * Counts contains the number of values which contain to one class.
 * the last values of ranges is unsigned long long max (unsigned long long int)-1
 * The ranges and counts array is created.
 * 
 */


/*int makeHistogram(unsigned long long *values, int testcount, int number_classes, int fraction, unsigned long long *ranges, int *counts){*/
int makeHistogramm(unsigned long long *values, int testcount, 
                   int fraction, struct histogramm * histo) {
   unsigned long long range;
   int limit,i,current_class;
	
   unsigned long long *tmp_val;
   tmp_val=(unsigned long long *) malloc(sizeof(unsigned long long)*testcount);
   if (NULL == tmp_val){
      ng_error("Could not allocate buffer for Histogram");
      return(0);
   }
   memcpy(tmp_val,values,testcount*sizeof(long long));
	
   /* Allocating arrays for Histogramm values */
   histo->ranges = (unsigned long long *) malloc(sizeof(long long) * (histo->number_classes+1));
   if(histo->ranges == NULL){
      ng_error("Could not allocate buffer for Histogram");
      free(tmp_val);
      return 1;
   }
   histo->counts = (unsigned int *) calloc(histo->number_classes+1,sizeof(int) );
   if(histo->counts == NULL){
      ng_error("Could not allocate buffer for Histogram");
      free(tmp_val);
      free(histo->ranges);
      return 1;
   }
	
   /* Seting ranges */
   qsort(tmp_val,testcount,sizeof(long long),comp);
   limit = (testcount / 100) * fraction;
   range = (tmp_val[limit] - tmp_val[0]) / histo->number_classes;
   for(i=0;i<histo->number_classes;i++){
      histo->ranges[i] = tmp_val[0] + range * (i+1);
   }
   histo->ranges[histo->number_classes] = (unsigned long long int)-1;
	
   /* computing counts */
   current_class = 0;
   for(i=0;i<testcount;i++){
      while(tmp_val[i] > histo->ranges[current_class]) current_class++;
      histo->counts[current_class]++;
   }
   free(tmp_val);
   return 0;
}

/*
 * Accepts two arrays with cycle values
 * Compute the time representing each value an
 * prints all times in one file
 * in char *full_output_file,  		name to use for file name generating
 * unsigned long long *blockcycles, first array with values to print
 * unsigned long long *rttcycles,	second array with values to print
 * double clock_period,				time for one cycle
 * int testcount, 					number of values in each array
 * int packetsize					packetsize which was used for the messaurment only to generate unique filename
 * return							returns 0 for succes and 1 for failure
 */

int print_full_results(char *full_output_file,
                       unsigned long long *blockcycles, unsigned long long *rttcycles,
                       double clock_period,
                       int testcount, int packetsize) {
				
   char *buffer;
   char *filename;
   int i=0,written=0;
   FILE *fp=NULL;
   struct histogramm *block_histo,*rtt_histo;
		
   block_histo = (struct histogramm *) malloc(sizeof(struct histogramm));
   if(block_histo == NULL){
      ng_error("Could not allocate buffer for block Histogram");
      return 1;
   }
   rtt_histo = (struct histogramm *) malloc(sizeof(struct histogramm));
   if(rtt_histo == NULL){
      ng_error("Could not allocate buffer for rtt Histogram");
      free(block_histo);
      return 1;
   }
   /*TODO number_classes muss Parameter werden*/
   block_histo->number_classes=100;
   rtt_histo->number_classes=100;
		
   filename = (char *)calloc(2048, sizeof(char)); /*  We should find a way without a fixed size buffer.   */
   if (filename == NULL) {
      ng_error("Could not allocate 2048 byte for output buffer");
      free(block_histo);
      free(rtt_histo);
      return 1;
   }
   snprintf(filename, 2048, "%s_packetsize_%d",full_output_file, packetsize);
   fp = fopen(filename, "w");
   if (!fp) {
      ng_perror("Failed to open fulloutput file %s for writing", filename);
      free(block_histo);
      free(rtt_histo);
      if (filename) free(filename);
      return 1;
   }

   buffer = (char *)calloc(1024, sizeof(char));
   if (buffer == NULL) {
      ng_error("Could not allocate 1024 byte for output buffer");
      if (block_histo){
         if(block_histo->ranges) free(block_histo->ranges);
         if(rtt_histo->counts) free(block_histo->counts);
         free(block_histo);
      }
      free(rtt_histo);
      if (filename) free(filename);
      if (fp) fclose(fp);
      return 1;
   }
				
   memset(buffer, '\0', 1024);

   if(makeHistogramm(blockcycles,testcount,95,block_histo)){	/*TODO Fraction should be a paramter */
      ng_error("generating Histogramm");
      free(block_histo);
      free(rtt_histo);
      if (filename) free(filename);
      if (fp) fclose(fp);
      free(buffer);
      return 1;
   }
   if(makeHistogramm(rttcycles,testcount,95,rtt_histo)){	/*TODO Fraction should be a paramter */
      ng_error("generating Histogramm");
      free(block_histo);
      free(rtt_histo);
      if (filename) free(filename);
      if (fp) fclose(fp);
      free(buffer);
      return 1;
   }
   snprintf(buffer, 1024,
            "# packetsize %d \n"
            "# time-measurement-1 time-measurement-2 bounds_measurment-1 count_measurment-1 bounds_measurment-1 count_measurment-1\n",
            packetsize);
   fputs(buffer, fp);
   fflush(fp);
		
   i=0;
   while(1){
      memset(buffer, '\0', 1024);			/* useless? snprintf sets the last byte by itself! according to man snprintf */
      if(i<testcount){
         written=snprintf(buffer, 1023,
                          "%.2f %.2f",
                          blockcycles[i] * clock_period * 1000000,   
                          rttcycles[i] * clock_period * 500000
                          );
         if(i<=block_histo->number_classes){			/*TODO sollt direkt der Parameter verwendet werden.*/
            written+=snprintf(buffer + written * sizeof(char), 1023-written,
                              " %.2f %d %.2f %d",
                              block_histo->ranges[i] * clock_period * 1000000,
                              block_histo->counts[i],
                              rtt_histo->ranges[i] * clock_period * 500000,
                              rtt_histo->counts[i]
                              );
         }
         snprintf(buffer + written * sizeof(char), 1023-written, " \n");
      }else 
         if(i<=block_histo->number_classes){		/*TODO sollt direkt der Parameter verwendet werden.*/
            snprintf(buffer, 1023-written,
                     " . . %.2f %d %.2f %d \n",
                     block_histo->ranges[i] * clock_period * 1000000,
                     block_histo->counts[i],
                     rtt_histo->ranges[i] * clock_period * 500000,
                     rtt_histo->counts[i]
                     );
         }else break;
      fputs(buffer, fp);
      fflush(fp);
      i++;
   }
		
   /* free all resources */
   if (block_histo){
      if(block_histo->ranges) free(block_histo->ranges);
      if(rtt_histo->counts) free(block_histo->counts);
      free(block_histo);
   }
   if (rtt_histo){
      if(rtt_histo->ranges) free(rtt_histo->ranges);
      if(rtt_histo->counts) free(rtt_histo->counts);
      free(rtt_histo);
   }
   if (buffer) free(buffer);		
   if (filename) free(filename);
   if (fp) fclose(fp);
   return 0;
}
