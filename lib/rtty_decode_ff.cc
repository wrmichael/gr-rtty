/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtty_decode_ff.h>
#include <gr_io_signature.h>
#include <iostream>

/*
 * Create a new instance of rtty_decode_ff and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
rtty_decode_ff_sptr 
rtty_make_decode_ff (float rate, float baud, bool polarity)
{
  return rtty_decode_ff_sptr (new rtty_decode_ff (rate, baud, polarity));
}

/*
 * Specify constraints on number of input and output streams.
 * This info is used to construct the input and output signatures
 * (2nd & 3rd args to gr_block's constructor).  The input and
 * output signatures are used by the runtime system to
 * check that a valid number and type of inputs and outputs
 * are connected to this block.  In this case, we accept
 * only 1 input and 1 output.
 */
static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams
static const int MIN_OUT = 1;	// minimum number of output streams
static const int MAX_OUT = 1;	// maximum number of output streams

/*
 * The private constructor
 */
rtty_decode_ff::rtty_decode_ff (float rate, float baud, bool polarity)
  : gr_block ("decode_ff",
	      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (float)),
	      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (char)))
{
  state = WAITING_FOR_START;
  _baud = baud;
  _rate = rate;
  _spb = rate/baud;
  charset = LETTERS;
  
  if(polarity) {
      mark = true;
      space = false;
  } else {
      mark = false;
      space = true;
  }
  set_relative_rate((baud/rate) / 7.42); //assumes 1.42 stop bits, old school
  set_history(_spb * 8);
  set_output_multiple(10);
}

/*
 * Our virtual destructor.
 */
rtty_decode_ff::~rtty_decode_ff ()
{
    //NOP
}

static unsigned char letters[] = 
  {'\0','E','\n','A',' ','S','I','U','\r','D','R','J','N','F','C','K',
   'T','Z','L','W','H','Y','P','Q','O','B','G','\0','M','X','V','\0'};
static unsigned char figures[] = 
  {'\0','3','\n','-',' ','\a','8','7','\r','$','4','\'',',','!',':',
   '(','5','\"',')','2','#','6','0','1','9','?','&','\0','.','/',';','\0'};
   
void rtty_decode_ff::forecast(int noutput_items, gr_vector_int &ninput_items_required) {
    ninput_items_required[0] = noutput_items / ((_baud/_rate) / 7.42);
}

int 
rtty_decode_ff::general_work (int noutput_items,
			       gr_vector_int &ninput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
  const float *in = (const float *) input_items[0];
  const float *instart = in;
  char *out = (char *) output_items[0];
  
  char outchar = 0;  
  int outcount = 0;
  
  while((outcount < noutput_items) && ((in - instart) < (ninput_items[0]-(_spb*8)))) {
      switch(state) {
      case WAITING_FOR_START:
	if((*in > 0) == space) {
	    state = DATA;
	    datapos = 0;
	    in += int(_spb*1.5); //move to center of first data bit
	} else in++; //advance by one sample waiting for start bit
	break;
      case DATA:
        if(datapos > 4) {
	    state = LOOKING_FOR_STOP;
	}
        else {
	    if((*in > 0) == mark) outchar += (1 << datapos);
	    datapos++;
	    in += int(_spb); //move ahead one bit
	}
	break;
      case LOOKING_FOR_STOP:
	if((*in > 0) == mark) {
	    if(outchar == 27) charset = FIGURES;
	    if(outchar == 31) charset = LETTERS;
	    out[outcount++] = (charset == LETTERS) ? letters[outchar] : figures[outchar];
	}
	
	state = WAITING_FOR_START;
	outchar = 0;
	break;
    }
  }

  // Tell runtime system how many input items we consumed on
  // each input stream.
  consume_each(in-instart);

  // Tell runtime system how many output items we produced.
  return outcount;
}
