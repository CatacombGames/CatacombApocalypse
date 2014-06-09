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

#pragma inline

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <alloc.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>

#include "def.h"
#include "gelib.h"
#include "jampak.h"



//=========================================================================
//
//
//									LOCAL DEFINATIONS
//
//
//=========================================================================

//#define COMPRESSION_CODE				// Comment define in for COMPRESS routines






//=========================================================================
//
//
//									LOCAL VARIABLES
//
//
//=========================================================================


unsigned char far LZW_ring_buffer[LZW_N + LZW_F - 1];

	// ring buffer of size LZW_N, with extra LZW_F-1 bytes to facilitate
	//	string comparison


#ifdef COMPRESSION_CODE

int LZW_match_pos,
	 LZW_match_len,

	// MAtchLength of longest match.  These are set by the InsertNode()
	//	procedure.

	// left & right children & parents -- These constitute binary search trees. */

	far LZW_left_child[LZW_N + 1],
	far LZW_right_child[LZW_N + 257],
	far LZW_parent[LZW_N + 1];

#endif

memptr segptr;
BufferedIO lzwBIO;





//=========================================================================
//
//
//									COMPRESSION SUPPORT ROUTINES
//
//
//=========================================================================


#ifdef COMPRESSION_CODE

//---------------------------------------------------------------------------
// InitLZWTree()
//---------------------------------------------------------------------------
void InitLZWTree(void)  /* initialize trees */
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





//---------------------------------------------------------------------------
// InsertLZWNode()
//---------------------------------------------------------------------------
void InsertLZWNode(unsigned long r)

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



//---------------------------------------------------------------------------
// DeleteLZWNode()
//---------------------------------------------------------------------------
void DeleteLZWNode(unsigned long p)  /* deletes node p from tree */
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
#endif




//=========================================================================
//
//
//							GENERAL FILE to FILE compression routines
//
//      * Mainly for example usage of PTR/PTR (de)compression routines.
//
//
//=========================================================================



//////////////////////////////////////////////////////////////////////
//
//  CompressFILEtoFILE() -- Compresses one file stream to another file stream
//

#ifdef COMPRESSION_CODE

unsigned long CompressFILEtoFILE(FILE *infile, FILE *outfile,unsigned long DataLength)
{
	unsigned long returnval;

	fwrite(COMP,4,1,outfile);
	fwrite((char *)&DataLength,4,1,outfile);

	returnval = 8+lzwCompress(infile,outfile,DataLength,(SRC_FFILE|DEST_FFILE));

	return(returnval);
}

#endif

#if 0
/////////////////////////////////////////////////////////////////////////////
//
//  DecompressFILEtoFILE()
//
void DecompressFILEtoFILE(FILE *infile, FILE *outfile)
{
	unsigned char Buffer[8];
	unsigned long DataLength;

	fread(Buffer,1,4,infile);

	if (strncmp(Buffer,COMP,4))
	{
		printf("\nNot a JAM Compressed File!\n");
		return;
	}

	fread((void *)&DataLength,1,4,infile);

	lzwDecompress(infile,outfile,DataLength,(SRC_FFILE|DEST_FFILE));
}
#endif





//==========================================================================
//
//
//							WRITE/READ PTR ROUTINES
//
//
//==========================================================================



//---------------------------------------------------------------------------
// WritePtr()  -- Outputs data to a particular ptr type
//
//	PtrType MUST be of type DEST_TYPE.
//
// NOTE : For PtrTypes DEST_MEM a ZERO (0) is always returned.
//
//---------------------------------------------------------------------------
int WritePtr(long outfile, unsigned char data, unsigned PtrType)
{
	int returnval = 0;

	switch (PtrType & DEST_TYPES)
	{
		case DEST_FILE:
			write(*(int far *)outfile,(char *)&data,1);
		break;

		case DEST_FFILE:
			returnval = putc(data, *(FILE **)outfile);
		break;

		case DEST_MEM:
//			*(*(char far **)outfile++) = data;						// Do NOT delete
			*((char far *)*(char far **)outfile)++ = data;
		break;

		default:
			TrashProg("WritePtr() : Unknown DEST_PTR type");
		break;
	}

	return(returnval);

}


//---------------------------------------------------------------------------
// ReadPtr()  -- Reads data from a particular ptr type
//
//	PtrType MUST be of type SRC_TYPE.
//
// RETURNS :
//		The char read in or EOF for SRC_FFILE type of reads.
//
//
//---------------------------------------------------------------------------
int ReadPtr(long infile, unsigned PtrType)
{
	int returnval = 0;

	switch (PtrType & SRC_TYPES)
	{
		case SRC_FILE:
			read(*(int far *)infile,(char *)&returnval,1);
		break;

		case SRC_FFILE:
// JIM - JTR - is the following correct? "fgetc()" uses a near pointer.
//
			returnval = fgetc((FILE far *)*(FILE far **)infile);
		break;

		case SRC_BFILE:
			returnval = bio_readch((BufferedIO *)*(void far **)infile);
		break;

		case SRC_MEM:
			returnval = (char)*(*(char far **)infile++);
//			returnval = *((char far *)*(char far **)infile)++;	// DO NOT DELETE!
		break;

		default:
			TrashProg("ReadPtr() : Unknown SRC_PTR type");
		break;
	}

	return(returnval);
}




//=========================================================================
//
//
//							COMPRESSION & DECOMPRESSION ROUTINES
//
//
//=========================================================================


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
#ifdef COMPRESSION_CODE
unsigned long lzwCompress(void far *infile, void far *outfile,unsigned long DataLength,unsigned PtrTypes)
{
	short i;
	short c, len, r, s, last_LZW_match_len, code_buf_ptr;
	unsigned char far code_buf[17], mask;
	unsigned long complen = 0;

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

	 return(complen);
}
#endif




//--------------------------------------------------------------------------
//
// lzwDecompress() - Compresses data from an input ptr to a dest ptr
//
// PARAMS:
//		 infile     - Pointer at the BEGINNING of the compressed data (no header!)
//		 outfile    - Pointer to the destination.
// 	 DataLength - Length of compressed data.
//     PtrTypes   - Type of pointers being used (SRC_FILE,DEST_FILE,SRC_MEM etc).
//
// RETURNS:
//	    Length of compressed data.
//
//	COMPTYPE : ct_LZW
//
// NOTES    : Does not write ANY header information!
//
void far lzwDecompress(void far *infile, void far *outfile,unsigned long DataLength,unsigned PtrTypes)
{
	int  i, j, k, r, c;
	unsigned int flags;

	for (i = 0; i < LZW_N - LZW_F; i++)
		LZW_ring_buffer[i] = ' ';

	 r = LZW_N - LZW_F;
	 flags = 0;

	 for ( ; ; )
	 {
			if (((flags >>= 1) & 256) == 0)
			{
				c = ReadPtr((long)&infile,PtrTypes);
				if (!DataLength--)
					return;

				flags = c | 0xff00;      // uses higher byte cleverly to count 8
			}

			if (flags & 1)
			{
				c = ReadPtr((long)&infile,PtrTypes);		// Could test for EOF iff FFILE type
				if (!DataLength--)
					return;

				WritePtr((long)&outfile,c,PtrTypes);

				LZW_ring_buffer[r++] = c;
				r &= (LZW_N - 1);
			}
			else
			{
				i = ReadPtr((long)&infile,PtrTypes);
			   if (!DataLength--)
					return;

				j = ReadPtr((long)&infile,PtrTypes);
			   if (!DataLength--)
					return;

				i |= ((j & 0xf0) << 4);
				j = (j & 0x0f) + LZW_THRESHOLD;

				for (k = 0; k <= j; k++)
				{
					 c = LZW_ring_buffer[(i + k) & (LZW_N - 1)];

					 WritePtr((long)&outfile,c,PtrTypes);

					 LZW_ring_buffer[r++] = c;
					 r &= (LZW_N - 1);
				}
			}
	 }
}



#if 0
//=========================================================================
//
//
//								 BUFFERED I/O ROUTINES
//
//
//=========================================================================


//--------------------------------------------------------------------------
// InitBufferedIO()
//--------------------------------------------------------------------------
memptr InitBufferedIO(int handle, BufferedIO *bio)
{
	bio->handle = handle;
	bio->offset = BIO_BUFFER_LEN;
	bio->status = 0;
	MM_GetPtr(&bio->buffer,BIO_BUFFER_LEN);

	return(bio->buffer);
}


//--------------------------------------------------------------------------
// FreeBufferedIO()
//--------------------------------------------------------------------------
void FreeBufferedIO(BufferedIO *bio)
{
	if (bio->buffer)
		MM_FreePtr(&bio->buffer);
}


//--------------------------------------------------------------------------
// bio_readch()
//--------------------------------------------------------------------------
byte bio_readch(BufferedIO *bio)
{
	byte far *buffer;

	if (bio->offset == BIO_BUFFER_LEN)
	{
		bio->offset = 0;
		bio_fillbuffer(bio);
	}

	buffer = MK_FP(bio->buffer,bio->offset++);

	return(*buffer);
}


//--------------------------------------------------------------------------
// bio_fillbuffer()
//
// BUGS (Not really bugs... More like RULES!)
//
//    1) This code assumes BIO_BUFFER_LEN is no smaller than
//       NEAR_BUFFER_LEN!!
//
//    2) BufferedIO.status should be altered by this code to report
//       read errors, end of file, etc... If you know how big the file
//       is you're reading, determining EOF should be no problem.
//
//--------------------------------------------------------------------------
void bio_fillbuffer(BufferedIO *bio)
{
	#define NEAR_BUFFER_LEN	(64)
	byte near_buffer[NEAR_BUFFER_LEN];
	short bio_length,bytes_read,bytes_requested;

	bytes_read = 0;
	bio_length = BIO_BUFFER_LEN;
	while (bio_length)
	{
		if (bio_length > NEAR_BUFFER_LEN-1)
			bytes_requested = NEAR_BUFFER_LEN;
		else
			bytes_requested = bio_length;

		read(bio->handle,near_buffer,bytes_requested);
		_fmemcpy(MK_FP(bio->buffer,bytes_read),near_buffer,bytes_requested);

		bio_length -= bytes_requested;
		bytes_read += bytes_requested;
	}
}


#endif

//=========================================================================
//
//
//								GENERAL LOAD ROUTINES
//
//
//=========================================================================



//--------------------------------------------------------------------------
// BLoad()
//--------------------------------------------------------------------------
unsigned long BLoad(char *SourceFile, memptr *DstPtr)
{
	int handle;

	memptr SrcPtr;
	longword i, j, k, r, c;
	word flags;
	byte Buffer[8];
	longword DstLen, SrcLen;
	boolean comp;

	if ((handle = open(SourceFile, O_RDONLY|O_BINARY)) == -1)
		return(0);

	// Look for 'COMP' header
	//
	read(handle,Buffer,4);
	comp = !strncmp(Buffer,COMP,4);

	// Get source and destination length.
	//
	if (comp)
	{
		SrcLen = Verify(SourceFile);
		read(handle,(void *)&DstLen,4);
		MM_GetPtr(DstPtr,DstLen);
		if (!*DstPtr)
			return(0);
	}
	else
		DstLen = Verify(SourceFile);

	// LZW decompress OR simply load the file.
	//
	if (comp)
	{

		if (MM_TotalFree() < SrcLen)
		{
			if (!InitBufferedIO(handle,&lzwBIO))
				TrashProg("No memory for buffered I/O.");
			lzwDecompress(&lzwBIO,MK_FP(*DstPtr,0),SrcLen,(SRC_BFILE|DEST_MEM));
			FreeBufferedIO(&lzwBIO);
		}
		else
		{
			CA_LoadFile(SourceFile,&SrcPtr);
			lzwDecompress(MK_FP(SrcPtr,8),MK_FP(*DstPtr,0),SrcLen,(SRC_MEM|DEST_MEM));
			MM_FreePtr(&SrcPtr);
		}
	}
	else
		CA_LoadFile(SourceFile,DstPtr);

	close(handle);
	return(DstLen);
}




////////////////////////////////////////////////////////////////////////////
//
// LoadLIBShape()
//
int LoadLIBShape(char *SLIB_Filename, char *Filename,struct Shape *SHP)
{
	#define CHUNK(Name)	(*ptr == *Name) &&			\
								(*(ptr+1) == *(Name+1)) &&	\
								(*(ptr+2) == *(Name+2)) &&	\
								(*(ptr+3) == *(Name+3))


	int RT_CODE;
	FILE *fp;
	char CHUNK[5];
	char far *ptr;
	memptr IFFfile = NULL;
	unsigned long FileLen, size, ChunkLen;
	int loop;


	RT_CODE = 1;

	// Decompress to ram and return ptr to data and return len of data in
	//	passed variable...

	if (!LoadLIBFile(SLIB_Filename,Filename,&IFFfile))
		TrashProg("Error Loading Compressed lib shape!");

	// Evaluate the file
	//
	ptr = MK_FP(IFFfile,0);
	if (!CHUNK("FORM"))
		goto EXIT_FUNC;
	ptr += 4;

	FileLen = *(long far *)ptr;
	SwapLong((long far *)&FileLen);
	ptr += 4;

	if (!CHUNK("ILBM"))
		goto EXIT_FUNC;
	ptr += 4;

	FileLen += 4;
	while (FileLen)
	{
		ChunkLen = *(long far *)(ptr+4);
		SwapLong((long far *)&ChunkLen);
		ChunkLen = (ChunkLen+1) & 0xFFFFFFFE;

		if (CHUNK("BMHD"))
		{
			ptr += 8;
			SHP->bmHdr.w = ((struct BitMapHeader far *)ptr)->w;
			SHP->bmHdr.h = ((struct BitMapHeader far *)ptr)->h;
			SHP->bmHdr.x = ((struct BitMapHeader far *)ptr)->x;
			SHP->bmHdr.y = ((struct BitMapHeader far *)ptr)->y;
			SHP->bmHdr.d = ((struct BitMapHeader far *)ptr)->d;
			SHP->bmHdr.trans = ((struct BitMapHeader far *)ptr)->trans;
			SHP->bmHdr.comp = ((struct BitMapHeader far *)ptr)->comp;
			SHP->bmHdr.pad = ((struct BitMapHeader far *)ptr)->pad;
			SwapWord(&SHP->bmHdr.w);
			SwapWord(&SHP->bmHdr.h);
			SwapWord(&SHP->bmHdr.x);
			SwapWord(&SHP->bmHdr.y);
			ptr += ChunkLen;
		}
		else
		if (CHUNK("BODY"))
		{
			ptr += 4;
			size = *((long far *)ptr);
			ptr += 4;
			SwapLong((long far *)&size);
			SHP->BPR = (SHP->bmHdr.w+7) >> 3;
			MM_GetPtr(&SHP->Data,size);
			if (!SHP->Data)
				goto EXIT_FUNC;
			movedata(FP_SEG(ptr),FP_OFF(ptr),FP_SEG(SHP->Data),0,size);
			ptr += ChunkLen;

			break;
		}
		else
			ptr += ChunkLen+8;

		FileLen -= ChunkLen+8;
	}

	RT_CODE = 0;

EXIT_FUNC:;
	if (IFFfile)
	{
//		segptr = (memptr)FP_SEG(IFFfile);
		MM_FreePtr(&IFFfile);
	}

	return (RT_CODE);
}





//----------------------------------------------------------------------------
// LoadLIBFile() -- Copies a file from an existing archive to dos.
//
// PARAMETERS :
//
//			LibName 	- Name of lib file created with SoftLib V1.0
//
//			FileName - Name of file to load from lib file.
//
//			MemPtr   - (IF !NULL) - Pointer to memory to load into ..
//						  (IF NULL)  - Routine allocates necessary memory and
//											returns a MEM(SEG) pointer to memory allocated.
//
// RETURN :
//
//   		(IF !NULL) - A pointer to the loaded data.
//			(IF NULL)  - Error!
//
//----------------------------------------------------------------------------
memptr LoadLIBFile(char *LibName,char *FileName,memptr *MemPtr)
{
	int handle;
	unsigned long header;
	struct ChunkHeader Header;
	unsigned long ChunkLen;
	short x;
	struct FileEntryHdr FileEntry;     			// Storage for file once found
	struct FileEntryHdr FileEntryHeader;		// Header used durring searching
	struct SoftLibHdr LibraryHeader;				// Library header - Version Checking
	boolean FileFound = false;
	unsigned long id_slib = ID_SLIB;
	unsigned long id_chunk = ID_CHUNK;


	//
	// OPEN SOFTLIB FILE
	//

	if ((handle = open(LibName,O_RDONLY | O_BINARY, S_IREAD)) == -1)
		return(NULL);


	//
	//	VERIFY it is a SOFTLIB (SLIB) file
	//

	if (read(handle,&header,4) == -1)
	{
		close(handle);
		return(NULL);
	}

	if (header != id_slib)
	{
		close(handle);
		return(NULL);
	}


	//
	// CHECK LIBRARY HEADER VERSION NUMBER
	//

	if (read(handle, &LibraryHeader,sizeof(struct SoftLibHdr)) == -1)
		TrashProg("read error in LoadSLIBFile()\n%c",7);

	if (LibraryHeader.Version > SOFTLIB_VER)
		TrashProg("Unsupported file ver %d",LibraryHeader.Version);


	//
	// MANAGE FILE ENTRY HEADERS...
	//

	for (x = 1;x<=LibraryHeader.FileCount;x++)
	{
		if (read(handle, &FileEntryHeader,sizeof(struct FileEntryHdr)) == -1)
		{
			close(handle);
			return(NULL);
		}

		if (!stricmp(FileEntryHeader.FileName,FileName))
		{
			FileEntry = FileEntryHeader;
			FileFound = true;
		}
	}

	//
	// IF FILE HAS BEEN FOUND THEN SEEK TO POSITION AND EXTRACT
	//	ELSE RETURN WITH ERROR CODE...
	//

	if (FileFound)
	{
		if (lseek(handle,FileEntry.Offset,SEEK_CUR) == -1)
		{
			close(handle);
			return(NULL);
		}

		//
		// READ CHUNK HEADER - Verify we are at the beginning of a chunk..
		//

		if (read(handle,(char *)&Header,sizeof(struct ChunkHeader)) == -1)
			TrashProg("LIB File - Unable to read Header!");

		if (Header.HeaderID != id_chunk)
			TrashProg("LIB File - BAD HeaderID!");

		//
		// Allocate memory if Necessary...
		//


		if (!*MemPtr)
			MM_GetPtr(MemPtr,FileEntry.OrginalLength);

		//
		//	Calculate the length of the data (without the chunk header).
		//

		ChunkLen = FileEntry.ChunkLen - sizeof(struct ChunkHeader);


		//
		// Extract Data from file
		//

		switch (Header.Compression)
		{
			case ct_LZW:
				if (!InitBufferedIO(handle,&lzwBIO))
					TrashProg("No memory for buffered I/O.");
				lzwDecompress(&lzwBIO,MK_FP(*MemPtr,0),ChunkLen,(SRC_BFILE|DEST_MEM));
				FreeBufferedIO(&lzwBIO);
				break;

			case ct_NONE:
				if (!CA_FarRead(handle,MK_FP(*MemPtr,0),ChunkLen))
				{
					close(handle);
					*MemPtr = NULL;
				}
				break;

			default:
				close(handle);
				TrashProg("Uknown Chunk.Compression Type!");
				break;
		}
	}
	else
		*MemPtr = NULL;

	close(handle);
	return(*MemPtr);
}




