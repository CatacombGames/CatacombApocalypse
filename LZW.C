/* Catacomb Apocalypse Source Code
 * Copyright (C) 1993-2014 Flat Rock Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

//===========================================================================
//
//								  LZW COMPRESSION ROUTINES
//										  VERSION 1.1
//
//  				Compression algrythim by Haruhiko OKUMURA
//  						Implementation by Jim T. Row
//
//
//   Copyright (c) 1992 - Softdisk Publishing inc. - All rights reserved
//
//===========================================================================
//
//
//---------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>

#include "lzw.h"
#include "jam_io.h"


//===========================================================================
//
//											SWITCHES
//
// NOTE : Make sure the appropriate switches are set in SOFT.c for Softlib
//			 archive support.
//
//===========================================================================

#define INCLUDE_LZW_COMP			0
#define INCLUDE_LZW_DECOMP			1


//===========================================================================
//
//											DEFINES
//
//===========================================================================


#define LZW_N							4096
#define LZW_F							18

#define LZW_THRESHOLD				2		// encode string into position and
													// length if match_length is greater
													// than this

#define LZW_NIL       			LZW_N    // index for root of bin search trees


//============================================================================
//
//								GLOBAL VARIABLES NECESSARY FOR
//
//       						 COMP/DECOMP ROUTINES.
//
//============================================================================



unsigned char far LZW_ring_buffer[LZW_N + LZW_F - 1];   // ring buffer of size
																		  // LZW_N, with extra
																		  // LZW_F-1 bytes to
																		  // facilitate
																		  // string comparison

#if INCLUDE_LZW_COMP

int LZW_match_pos,	// MAtchLength of longest match.  These are set by the
							// InsertNode() procedure.
	 LZW_match_len,

	// left & right children & parents -- These constitute binary search trees. */

	LZW_left_child[LZW_N + 1],
	LZW_right_child[LZW_N + 257],
	LZW_parent[LZW_N + 1];

#endif


//============================================================================
//
// 									LZW DISPLAY VECTORS
//
// These vectors allow you to hook up any form of display you desire for
// displaying the compression/decompression status.
//
// These routines are called inside of the compression/decompression routines
// and pass the orginal size of data and current position within that
// data.  This allows for any kind of "% Done" messages.
//
// Your functions MUST have the following parameters in this order...
//
//   void VectorRoutine(unsigned long OrginalSize,unsigned long CurPosition)
//
//

void (*LZW_CompressDisplayVector)() = NULL;
void (*LZW_DecompressDisplayVector)() = NULL;




//============================================================================
//
//							SUPPORT ROUTINES FOR LZW COMPRESSION
//
//============================================================================


#if INCLUDE_LZW_COMP

static void InitLZWTree(void)  /* initialize trees */
{
	 int i;

	 /* For i = 0 to LZW_N - 1, LZW_right_child[i] and LZW_left_child[i] will be the right and
		 left children of node i.  These nodes need not be initialized.
		 Also, LZW_parent[i] is the parent of node i.  These are initialized to
		 LZW_NIL (= LZW_N), which stands for 'not used.'
		 For i = 0 to 255, LZW_right_child[LZW_N + i + 1] is the root of the tree
		 for strings that begin with character i.  These are initialized
		 to LZW_NIL.  Note there are 256 trees. */

	 for (i = LZW_N + 1; i <= LZW_N + 256; i++)
		LZW_right_child[i] = LZW_NIL;

	 for (i = 0; i < LZW_N; i++)
		LZW_parent[i] = LZW_NIL;
}


////////////////////////////////////////////////////////////////////////////

static void InsertLZWNode(unsigned long r)

	 /* Inserts string of length LZW_F, LZW_ring_buffer[r..r+LZW_F-1], into one of the
		 trees (LZW_ring_buffer[r]'th tree) and returns the longest-match position
		 and length via the global variables LZW_match_pos and LZW_match_len.
		 If LZW_match_len = LZW_F, then removes the old node in favor of the new
		 one, because the old one will be deleted sooner.
		 Note r plays double role, as tree node and position in buffer. */
{
	 int  i, p, cmp;
	 unsigned char *key;

	 cmp = 1;
	 key = &LZW_ring_buffer[r];
	 p = LZW_N + 1 + key[0];
	 LZW_right_child[r] = LZW_left_child[r] = LZW_NIL;
	 LZW_match_len = 0;

	for ( ; ; )
	{
		if (cmp >= 0)
		{
			if (LZW_right_child[p] != LZW_NIL)
				p = LZW_right_child[p];
			else
			{
				LZW_right_child[p] = r;
				LZW_parent[r] = p;
				return;
			}
		}
		else
		{
			if (LZW_left_child[p] != LZW_NIL)
				p = LZW_left_child[p];
			else
			{
				LZW_left_child[p] = r;
				LZW_parent[r] = p;
				return;
			}
		}

		for (i = 1; i < LZW_F; i++)
			if ((cmp = key[i] - LZW_ring_buffer[p + i]) != 0)
				break;

		if (i > LZW_match_len)
		{
			LZW_match_pos = p;
			if ((LZW_match_len = i) >= LZW_F)
				break;
		}
	}

	LZW_parent[r] = LZW_parent[p];
	LZW_left_child[r] = LZW_left_child[p];
	LZW_right_child[r] = LZW_right_child[p];
	LZW_parent[LZW_left_child[p]] = r;
	LZW_parent[LZW_right_child[p]] = r;

	if (LZW_right_child[LZW_parent[p]] == p)
		LZW_right_child[LZW_parent[p]] = r;
	else
		LZW_left_child[LZW_parent[p]] = r;

	LZW_parent[p] = LZW_NIL;  /* remove p */
}

////////////////////////////////////////////////////////////////////////////

static void DeleteLZWNode(unsigned long p)  /* deletes node p from tree */
{
	int q;

	if (LZW_parent[p] == LZW_NIL)
		return; 					 /* not in tree */

	if (LZW_right_child[p] == LZW_NIL)
		q = LZW_left_child[p];
	else
	if (LZW_left_child[p] == LZW_NIL)
		q = LZW_right_child[p];
	else
	{
		q = LZW_left_child[p];
		if (LZW_right_child[q] != LZW_NIL)
		{
			do {

				q = LZW_right_child[q];

			} while (LZW_right_child[q] != LZW_NIL);

			LZW_right_child[LZW_parent[q]] = LZW_left_child[q];
			LZW_parent[LZW_left_child[q]] = LZW_parent[q];
			LZW_left_child[q] = LZW_left_child[p];
			LZW_parent[LZW_left_child[p]] = q;
		}

		LZW_right_child[q] = LZW_right_child[p];
		LZW_parent[LZW_right_child[p]] = q;
	 }

	 LZW_parent[q] = LZW_parent[p];
	 if (LZW_right_child[LZW_parent[p]] == p)
		LZW_right_child[LZW_parent[p]] = q;
	 else
		LZW_left_child[LZW_parent[p]] = q;

	 LZW_parent[p] = LZW_NIL;
}



//--------------------------------------------------------------------------
//
// lzwCompress() - Compresses data from an input ptr to a dest ptr
//
// PARAMS:
//		 infile     - Pointer at the BEGINNING of the data to compress
//		 outfile    - Pointer to the destination (no header).
// 	 DataLength - Number of bytes to compress.
//     PtrTypes   - Type of pointers being used (SRC_FILE,DEST_FILE,SRC_MEM etc).
//
// RETURNS:
//	    Length of compressed data.
//
//	COMPTYPE : ct_LZW
//
// NOTES    : Does not write ANY header information!
//
unsigned long lzwCompress(void far *infile, void far *outfile,unsigned long DataLength,unsigned PtrTypes)
{
	short i;
	short c, len, r, s, last_LZW_match_len, code_buf_ptr;
	unsigned char code_buf[17], mask;
	unsigned long complen = 0;

	unsigned CodeCount = 0;
	unsigned long OrgDataLen = DataLength;

	// initialize trees

	InitLZWTree();

	code_buf[0] = 0;

	//
	//  code_buf[1..16] saves eight units of code, and code_buf[0] works
	//	 as eight flags, "1" representing that the unit	is an unencoded
	//	 letter (1 byte), "0" a position-and-length pair (2 bytes).  Thus,
	//	 eight units require at most 16 bytes of code.
	//

	 code_buf_ptr = mask = 1;
	 s = 0;
	 r = LZW_N - LZW_F;

	// Clear the buffer with any character that will appear often.
	//

	 for (i = s; i < r; i++)
			LZW_ring_buffer[i] = ' ';

	// Read LZW_F bytes into the last LZW_F bytes of the buffer
	//

	 for (len = 0; (len < LZW_F) && DataLength; len++)
	 {
		c = ReadPtr((long)&infile,PtrTypes);
		DataLength--;

		// text of size zero

		LZW_ring_buffer[r + len] = c;
	 }

	 if (!(len && DataLength))
		return(0);

	//
	// Insert the LZW_F strings, each of which begins with one or more
	// 'space' characters.  Note the order in which these strings
	// are inserted.  This way, degenerate trees will be less likely
	// to occur.
	//

	 for (i = 1; i <= LZW_F; i++)
			InsertLZWNode(r - i);

	//
	// Finally, insert the whole string just read.  The global
	// variables LZW_match_len and LZW_match_pos are set. */
	//

	 InsertLZWNode(r);

	 do {
			// LZW_match_len may be spuriously long near the end of text.
			//

			if (LZW_match_len > len)
				LZW_match_len = len;

			if (LZW_match_len <= LZW_THRESHOLD)
			{
				  // Not long enough match.  Send one byte.
				  //

				  LZW_match_len = 1;

				  // 'send one byte' flag
				  //

				  code_buf[0] |= mask;

				  // Send uncoded.
				  //

				  code_buf[code_buf_ptr++] = LZW_ring_buffer[r];
			}
			else
			{
				code_buf[code_buf_ptr++] = (unsigned char) LZW_match_pos;
				code_buf[code_buf_ptr++] = (unsigned char) (((LZW_match_pos >> 4) & 0xf0) | (LZW_match_len - (LZW_THRESHOLD + 1)));

				// Send position and length pair.
				// Note LZW_match_len > LZW_THRESHOLD.
			}

			if ((mask <<= 1) == 0)
			{
				// Shift mask left one bit.
				// Send at most 8 units of data

				for (i = 0; i < code_buf_ptr; i++)
					WritePtr((long)&outfile,code_buf[i],PtrTypes);

				complen += code_buf_ptr;
				code_buf[0] = 0;
				code_buf_ptr = mask = 1;
			}

			last_LZW_match_len = LZW_match_len;

			for (i = 0; i < last_LZW_match_len && DataLength; i++)
			{
				c = ReadPtr((long)&infile,PtrTypes);
				DataLength--;

				DeleteLZWNode(s);  			    // Delete old strings and
				LZW_ring_buffer[s] = c; 	  				 // read new bytes

				// If the position is near the end of buffer, extend the
				//	buffer to make string comparison easier.

				if (s < LZW_F - 1)
					LZW_ring_buffer[s + LZW_N] = c;

				// Since this is a ring buffer, inc the position modulo LZW_N.
				//

				s = (s + 1) & (LZW_N - 1);
				r = (r + 1) & (LZW_N - 1);

				// Register the string in LZW_ring_buffer[r..r+LZW_F-1]
				//

				InsertLZWNode(r);
			}


			//
			// MANAGE DISPLAY VECTOR
			//

			if (LZW_CompressDisplayVector)
			{
				// Update display every 1k!
				//

				if ((CodeCount += i) > 1024)
				{
					LZW_CompressDisplayVector(OrgDataLen,OrgDataLen-DataLength);
					CodeCount = 0;
				}
			}


			//
			// Manage Compression tree..
			//

			while (i++ < last_LZW_match_len)
			{
														  // After the end of text,
				DeleteLZWNode(s);               // no need to read, but

				s = (s + 1) & (LZW_N - 1);
				r = (r + 1) & (LZW_N - 1);

				if (--len)
					InsertLZWNode(r);          // buffer may not be empty.
			}

	 } while (len > 0);  // until length of string to be processed is zero


	 if (code_buf_ptr > 1)
	 {
		// Send remaining code.
		//

		for (i = 0; i < code_buf_ptr; i++)
			WritePtr((long)&outfile,code_buf[i],PtrTypes);

		complen += code_buf_ptr;
	 }

	if (LZW_CompressDisplayVector)
	{
		if ((CodeCount += i) > 1024)
		{
			LZW_CompressDisplayVector(OrgDataLen,OrgDataLen-DataLength);
		}
	}

	 return(complen);
}

#endif




//============================================================================
//
//							SUPPORT ROUTINES FOR LZW DECOMPRESSION
//
//============================================================================

#if INCLUDE_LZW_DECOMP

//--------------------------------------------------------------------------
//
// lzwDecompress() - Compresses data from an input ptr to a dest ptr
//
// PARAMS:
//		 infile     - Pointer at the BEGINNING of the compressed data (no header!)
//		 outfile    - Pointer to the destination.
// 	 DataLength - Number of bytes to decompress.
//     PtrTypes   - Type of pointers being used (SRC_FILE,DEST_FILE,SRC_MEM etc).
//
// RETURNS:
//	    Length of compressed data.
//
//	COMPTYPE : ct_LZW
//
// NOTES    : Does not write ANY header information!
//
void lzwDecompress(void far *infile, void far *outfile,unsigned long DataLength,unsigned PtrTypes)
{
	int  i, j, k, r, c;
	unsigned int flags;
	unsigned char Buffer[8];
//	unsigned char LZW_ring_buffer[LZW_N + LZW_F - 1];

	unsigned CodeCount = 0;
	unsigned long OrgDataLen = DataLength;

	for (i = 0; i < LZW_N - LZW_F; i++)
		LZW_ring_buffer[i] = ' ';

	 r = LZW_N - LZW_F;
	 flags = 0;

	 for ( ; ; )
	 {
			if (((flags >>= 1) & 256) == 0)
			{
				c = ReadPtr((long)&infile,PtrTypes);

				flags = c | 0xff00;      // uses higher byte cleverly to count 8
			}

			if (flags & 1)
			{
				c = ReadPtr((long)&infile,PtrTypes);		// Could test for EOF iff FFILE type

				WritePtr((long)&outfile,c,PtrTypes);

				if (!--DataLength)
					return;

				LZW_ring_buffer[r++] = c;
				r &= (LZW_N - 1);
			}
			else
			{
				i = ReadPtr((long)&infile,PtrTypes);

				j = ReadPtr((long)&infile,PtrTypes);

				i |= ((j & 0xf0) << 4);
				j = (j & 0x0f) + LZW_THRESHOLD;

				for (k = 0; k <= j; k++)
				{
					 c = LZW_ring_buffer[(i + k) & (LZW_N - 1)];

					 WritePtr((long)&outfile,c,PtrTypes);

					 if (!--DataLength)
						return;

					 LZW_ring_buffer[r++] = c;
					 r &= (LZW_N - 1);
				}
			}



			//
			//	MANAGE DISPLAY VECTOR
			//

			if (LZW_DecompressDisplayVector)
			{
				//
				// Update DisplayVector every 1K
				//

				if ((CodeCount+=k) > 1024)
				{
					LZW_DecompressDisplayVector(OrgDataLen,OrgDataLen-DataLength);
					CodeCount = 0;
				}
			}

	 }
}

#endif

